#!/bin/bash
# =============================================================================
# check_architecture.sh - 检查四层架构合规性
# =============================================================================
# 根据 ENGINEERING_STANDARDS.md 检查代码架构
#
# 架构规则:
#   app → engine → domain
#              ↘ infrastructure
#
# 禁止:
#   - infrastructure → engine (逆向依赖)
#   - domain → engine (跨层依赖)
#   - domain → infrastructure (跨层依赖)
#   - 循环依赖
#
# 使用方法:
#   bash scripts/check_architecture.sh
#   bash scripts/check_architecture.sh --verbose
#   bash scripts/check_architecture.sh --fix  # 自动修复 include 顺序

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$PROJECT_ROOT/02_工程主代码/CloudSeaManor/src"

show_help() {
    cat << EOF
${BOLD}检查四层架构合规性${NC}

${BOLD}用法:${NC}
    $(basename "$0") [选项]

${BOLD}选项:${NC}
    -h, --help       显示帮助信息
    -v, --verbose    详细输出

${BOLD}架构规则:${NC}
    app → engine → domain
               ↘ infrastructure

    禁止:
    - infrastructure → engine (逆向依赖)
    - domain → engine (跨层依赖)
    - domain → infrastructure (跨层依赖)
    - 循环依赖

${BOLD}示例:${NC}
    $(basename "$0")
    $(basename "$0") --verbose

EOF
}

VERBOSE=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help) show_help; exit 0 ;;
        -v|--verbose) VERBOSE=true; shift ;;
        *) shift ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  四层架构合规性检查${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

violations=0

# 检查 1: domain 不应包含 engine 或 infrastructure
echo -e "${BLUE}[检查 1] domain 层依赖检查...${NC}"
for file in $(find "$SRC_DIR/domain" -type f \( -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null); do
    if grep -qE '#include.*(engine|infrastructure)' "$file" 2>/dev/null; then
        echo -e "${RED}✗ $file${NC}"
        grep -nE '#include.*(engine|infrastructure)' "$file" | head -3
        violations=$((violations + 1))
    fi
done
if [ $violations -eq 0 ]; then
    echo -e "${GREEN}✓ 通过${NC}"
fi
echo ""

# 检查 2: infrastructure 不应包含 engine
echo -e "${BLUE}[检查 2] infrastructure 层依赖检查...${NC}"
infra_engine_violations=0
for file in $(find "$SRC_DIR/infrastructure" -type f \( -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null); do
    if grep -qE '#include.*/engine/' "$file" 2>/dev/null; then
        echo -e "${RED}✗ $file${NC}"
        grep -nE '#include.*/engine/' "$file" | head -3
        infra_engine_violations=$((infra_engine_violations + 1))
        violations=$((violations + 1))
    fi
done
if [ $infra_engine_violations -eq 0 ]; then
    echo -e "${GREEN}✓ 通过${NC}"
fi
echo ""

# 检查 3: app 层只应依赖 engine
echo -e "${BLUE}[检查 3] app 层依赖检查...${NC}"
app_violations=0
for file in $(find "$SRC_DIR/app" -type f \( -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null); do
    if grep -qE '#include.*/(domain|infrastructure)/' "$file" 2>/dev/null; then
        echo -e "${YELLOW}⚠ $file (可能需要检查)${NC}"
        grep -nE '#include.*/(domain|infrastructure)/' "$file" | head -3
        app_violations=$((app_violations + 1))
    fi
done
if [ $app_violations -eq 0 ]; then
    echo -e "${GREEN}✓ 通过${NC}"
else
    echo -e "${YELLOW}⚠ app 直接依赖 domain/infrastructure，建议通过 engine 间接访问${NC}"
fi
echo ""

# 结果汇总
echo -e "${BLUE}========================================${NC}"
if [ $violations -eq 0 ]; then
    echo -e "${GREEN}  ✓ 架构检查全部通过！${NC}"
else
    echo -e "${RED}  ✗ 发现 $violations 个架构问题${NC}"
fi
echo -e "${BLUE}========================================${NC}"

exit $violations
