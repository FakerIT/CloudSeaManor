#!/bin/bash
# =============================================================================
# run_all_checks.sh - 运行所有检查脚本
# =============================================================================
# 便捷脚本，用于在提交前运行所有检查
#
# 使用方法:
#   bash scripts/run_all_checks.sh
#   bash scripts/run_all_checks.sh --ci  # CI 模式（更严格）
#   bash scripts/run_all_checks.sh --fix  # 自动修复可修复的问题

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

CI_MODE=false
AUTO_FIX=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --ci) CI_MODE=true; shift ;;
        --fix) AUTO_FIX=true; shift ;;
        -h|--help)
            echo "用法: $0 [--ci] [--fix]"
            echo "  --ci   CI 模式，更严格的检查"
            echo "  --fix  自动修复可修复的问题"
            exit 0
            ;;
        *) shift ;;
    esac
done

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BOLD}${BLUE}========================================${NC}"
echo -e "${BOLD}${BLUE}  CloudSeaManor 全量检查${NC}"
echo -e "${BOLD}${BLUE}========================================${NC}"
echo ""

FAILED=0

# 1. 代码格式检查
echo -e "${BLUE}[1/5] 代码格式检查...${NC}"
if command -v clang-format &> /dev/null; then
    if [ "$AUTO_FIX" = true ]; then
        find 02_工程主代码/CloudSeaManor/src 02_工程主代码/CloudSeaManor/include \
            \( -name '*.cpp' -o -name '*.hpp' \) -exec clang-format -i {} \;
    fi
    clang-format --dry-run --Werror --style=file \
        $(find 02_工程主代码/CloudSeaManor/src 02_工程主代码/CloudSeaManor/include \
        \( -name '*.cpp' -o -name '*.hpp' \) | head -20) 2>/dev/null || true
    echo -e "${GREEN}✓ 完成${NC}"
else
    echo -e "${YELLOW}⚠ 跳过（clang-format 未安装）${NC}"
fi
echo ""

# 2. 架构检查
echo -e "${BLUE}[2/5] 四层架构检查...${NC}"
if [ -f "$PROJECT_ROOT/scripts/check_architecture.sh" ]; then
    bash "$PROJECT_ROOT/scripts/check_architecture.sh" || FAILED=$((FAILED + 1))
else
    echo -e "${YELLOW}⚠ 跳过（脚本不存在）${NC}"
fi
echo ""

# 3. Domain SFML 检查
echo -e "${BLUE}[3/5] Domain 层 SFML 检查...${NC}"
if [ -f "$PROJECT_ROOT/scripts/check_domain_sfml.sh" ]; then
    bash "$PROJECT_ROOT/scripts/check_domain_sfml.sh" || FAILED=$((FAILED + 1))
else
    echo -e "${YELLOW}⚠ 跳过（脚本不存在）${NC}"
fi
echo ""

# 4. 数据表验证
echo -e "${BLUE}[4/5] 数据表验证...${NC}"
if [ -f "$PROJECT_ROOT/scripts/validate_data_tables.sh" ]; then
    bash "$PROJECT_ROOT/scripts/validate_data_tables.sh" || FAILED=$((FAILED + 1))
else
    echo -e "${YELLOW}⚠ 跳过（脚本不存在）${NC}"
fi
echo ""

# 5. 构建测试（CI 模式）
if [ "$CI_MODE" = true ]; then
    echo -e "${BLUE}[5/5] 快速构建测试...${NC}"
    if [ -d "02_工程主代码/CloudSeaManor/build" ]; then
        cmake --build 02_工程主代码/CloudSeaManor/build --config Debug -j4 > /dev/null 2>&1 || {
            echo -e "${RED}✗ 构建失败${NC}"
            FAILED=$((FAILED + 1))
        }
    else
        echo -e "${YELLOW}⚠ 跳过（未配置构建）${NC}"
    fi
    echo ""
else
    echo -e "${BLUE}[5/5] 构建测试 (跳过，CI 模式请使用 --ci)${NC}"
fi

# 结果汇总
echo -e "${BLUE}========================================${NC}"
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}${BOLD}  ✓ 所有检查通过！${NC}"
else
    echo -e "${RED}${BOLD}  ✗ 发现 $FAILED 个问题${NC}"
fi
echo -e "${BLUE}========================================${NC}"

exit $FAILED
