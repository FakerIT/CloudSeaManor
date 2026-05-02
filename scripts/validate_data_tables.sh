#!/bin/bash
# =============================================================================
# validate_data_tables.sh - 数据表验证脚本
# =============================================================================
# 验证 CSV 数据表的格式、必填字段、唯一 ID 等
#
# 使用方法:
#   bash scripts/validate_data_tables.sh
#   bash scripts/validate_data_tables.sh --fix     # 尝试自动修复
#   bash scripts/validate_data_tables.sh --verbose  # 详细输出
#   bash scripts/validate_data_tables.sh --table battle  # 只检查指定表

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

VERBOSE=false
AUTO_FIX=false
TABLE_FILTER=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose) VERBOSE=true; shift ;;
        -f|--fix) AUTO_FIX=true; shift ;;
        -t|--table) TABLE_FILTER="$2"; shift 2 ;;
        -h|--help)
            echo "用法: $0 [--verbose] [--fix] [--table TABLE]"
            exit 0
            ;;
        *) shift ;;
    esac
done

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA_DIR="$PROJECT_ROOT/02_工程主代码/CloudSeaManor/assets/data"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  数据表验证工具${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

errors=0
warnings=0

# 验证单个 CSV 文件
validate_csv() {
    local file="$1"
    local name=$(basename "$file")
    local dir=$(dirname "$file")

    [ "$VERBOSE" = true ] && echo -e "${BLUE}检查: $name${NC}"

    # 检查文件是否存在
    if [ ! -f "$file" ]; then
        echo -e "${RED}✗ 文件不存在: $name${NC}"
        return 1
    fi

    # 检查是否为空
    if [ ! -s "$file" ]; then
        echo -e "${YELLOW}⚠ 空文件: $name${NC}"
        return 0
    fi

    # 检查 BOM
    if [ "$(head -c 3 "$file" | xxd -p)" = "efbbbf" ]; then
        echo -e "${YELLOW}⚠ 文件包含 BOM: $name${NC}"
        warnings=$((warnings + 1))
    fi

    # 检查行尾符
    if file "$file" | grep -q "CRLF"; then
        echo -e "${YELLOW}⚠ Windows 行尾符: $name${NC}"
        warnings=$((warnings + 1))
    fi

    # 获取列数（第一行）
    local cols=$(head -1 "$file" | awk -F',' '{print NF}')
    if [ -z "$cols" ] || [ "$cols" -eq 0 ]; then
        echo -e "${RED}✗ 无法读取列数: $name${NC}"
        errors=$((errors + 1))
        return 1
    fi

    # 检查数据行
    local line_num=1
    while IFS= read -r line; do
        line_num=$((line_num + 1))
        local line_cols=$(echo "$line" | awk -F',' '{print NF}')

        if [ "$line_cols" -ne "$cols" ]; then
            echo -e "${RED}✗ $name:$line_num 列数不匹配 (期望 $cols, 实际 $line_cols)${NC}"
            errors=$((errors + 1))
        fi
    done < <(tail -n +2 "$file")

    # 检查 ID 列（如果存在）
    if head -1 "$file" | grep -qi "id"; then
        local ids=$(tail -n +2 "$file" | cut -d',' -f1 | grep -v '^$' | sort | uniq -d)
        if [ -n "$ids" ]; then
            echo -e "${RED}✗ $name 存在重复 ID:${NC}"
            echo "$ids" | while read -r id; do
                echo -e "${RED}  - $id${NC}"
            done
            errors=$((errors + 1))
        fi
    fi

    # 检查必填字段（如果表有 Name/Name 列）
    if head -1 "$file" | grep -qi "name" && ! head -1 "$file" | grep -qi "description"; then
        local empty_names=$(awk -F',' 'NR>1 && $2 ~ /^[[:space:]]*$/' "$file" | head -5)
        if [ -n "$empty_names" ]; then
            echo -e "${YELLOW}⚠ $name 存在空名称:${NC}"
            echo -e "${YELLOW}$empty_names${NC}"
            warnings=$((warnings + 1))
        fi
    fi

    [ "$VERBOSE" = true ] && echo -e "${GREEN}✓ $name 通过验证${NC}"
    return 0
}

# 扫描所有 CSV 文件
echo -e "${BLUE}扫描目录: $DATA_DIR${NC}"
echo ""

total=0
passed=0
failed=0

while IFS= read -r file; do
    # 应用过滤器
    if [ -n "$TABLE_FILTER" ]; then
        if ! echo "$file" | grep -qi "$TABLE_FILTER"; then
            continue
        fi
    fi

    # 排除子目录中的特定文件
    if echo "$file" | grep -q "backup\|test\|archive"; then
        continue
    fi

    total=$((total + 1))

    if validate_csv "$file"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi

done < <(find "$DATA_DIR" -name "*.csv" -type f 2>/dev/null)

# 结果汇总
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  验证结果${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "检查文件: $total"
echo -e "通过: ${GREEN}$passed${NC}"
echo -e "失败: ${RED}$failed${NC}"
echo -e "警告: ${YELLOW}$warnings${NC}"
echo ""

if [ $failed -gt 0 ]; then
    echo -e "${RED}✗ 验证失败！请修复上述问题${NC}"
    exit 1
elif [ $warnings -gt 0 ]; then
    echo -e "${YELLOW}⚠ 验证通过但有警告${NC}"
    exit 0
else
    echo -e "${GREEN}✓ 所有数据表验证通过！${NC}"
    exit 0
fi
