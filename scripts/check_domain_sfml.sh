#!/bin/bash
# =============================================================================
# check_domain_sfml.sh - 检查 domain 层是否违规使用 SFML
# =============================================================================
# 根据 ENGINEERING_STANDARDS.md，domain 层禁止使用 SFML
# 此脚本用于 CI 和本地检查
#
# 使用方法:
#   bash scripts/check_domain_sfml.sh
#   bash scripts/check_domain_sfml.sh --fix    # 自动修复（不建议）
#   bash scripts/check_domain_sfml.sh --verbose # 详细输出
#
# 退出码:
#   0 - 检查通过
#   1 - 检查失败，发现违规
#   2 - 参数错误

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# 默认设置
VERBOSE=false
AUTO_FIX=false
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOMAIN_DIR="$PROJECT_ROOT/02_工程主代码/CloudSeaManor/src/domain"

# =============================================================================
# 帮助信息
# =============================================================================
show_help() {
    cat << EOF
${BOLD}检查 domain 层 SFML 使用情况${NC}

${BOLD}用法:${NC}
    $(basename "$0") [选项]

${BOLD}选项:${NC}
    -h, --help       显示帮助信息
    -v, --verbose    详细输出
    -f, --fix        自动修复（不推荐，可能破坏代码）
    -p, --path PATH  指定项目根目录
    --dry-run        只报告问题，不修复

${BOLD}说明:${NC}
    根据 ENGINEERING_STANDARDS.md，domain 层：
    - 禁止使用 SFML 命名空间 (sf::)
    - 禁止直接包含 SFML 头文件
    - 允许使用标准库和项目内部接口

    此脚本会扫描 domain 目录，报告任何 SFML 使用情况。

${BOLD}示例:${NC}
    # 基本检查
    $(basename "$0")

    # 详细输出
    $(basename "$0") --verbose

    # 检查但不修复
    $(basename "$0") --dry-run

EOF
}

# =============================================================================
# 解析参数
# =============================================================================
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -f|--fix)
            AUTO_FIX=true
            shift
            ;;
        -p|--path)
            PROJECT_ROOT="$2"
            DOMAIN_DIR="$PROJECT_ROOT/02_工程主代码/CloudSeaManor/src/domain"
            shift 2
            ;;
        --dry-run)
            AUTO_FIX=false
            shift
            ;;
        *)
            echo -e "${RED}错误: 未知参数 '$1'${NC}"
            show_help
            exit 2
            ;;
    esac
done

# =============================================================================
# 检查函数
# =============================================================================
check_domain_sfml() {
    local found_violations=0
    local violations=()

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  Domain 层 SFML 依赖检查${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # 检查目录是否存在
    if [ ! -d "$DOMAIN_DIR" ]; then
        echo -e "${YELLOW}⚠ domain 目录不存在: $DOMAIN_DIR${NC}"
        echo -e "${GREEN}✓ 检查通过（无 domain 目录）${NC}"
        return 0
    fi

    echo -e "${BLUE}检查目录: $DOMAIN_DIR${NC}"
    echo ""

    # SFML 相关模式
    local patterns=(
        'sf::'                    # SFML 命名空间
        '#include.*<SFML/'       # SFML 头文件包含
        '#include.*<Graphics/'   # SFML Graphics
        '#include.*<Window/'     # SFML Window
        '#include.*<Audio/'      # SFML Audio
        '#include.*<Network/'    # SFML Network
        '#include.*<System/'     # SFML System
        'sf::Vector2'            # SFML 向量类型
        'sf::Sprite'             # SFML 精灵
        'sf::Texture'            # SFML 纹理
        'sf::Font'               # SFML 字体
        'sf::Sound'              # SFML 声音
        'sf::Music'              # SFML 音乐
        'sf::Shape'              # SFML 形状
        'sf::CircleShape'        # SFML 圆形
        'sf::RectangleShape'     # SFML 矩形
        'sf::RenderWindow'       # SFML 窗口
        'sf::Event'              # SFML 事件
        'sf::Clock'              # SFML 时钟
        'sf::Time'               # SFML 时间
    )

    # 允许的例外模式（某些 SFML 类型实际上不依赖渲染）
    local allowed_patterns=(
        'sf::NonCopyable'        # SFML 基类
        'sf::String'             # SFML 字符串（纯数据）
        'sf::Color'              # SFML 颜色（纯数据）
    )

    echo -e "${BLUE}扫描文件...${NC}"
    echo ""

    # 搜索所有违规
    for pattern in "${patterns[@]}"; do
        # 使用 grep 搜索，排除允许的模式
        local matches=$(find "$DOMAIN_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) \
            -exec grep -Hn "$pattern" {} \; 2>/dev/null || true)

        if [ -n "$matches" ]; then
            # 过滤掉允许的模式
            local filtered_matches=""
            while IFS= read -r line; do
                local is_allowed=false
                for allowed in "${allowed_patterns[@]}"; do
                    if echo "$line" | grep -q "$allowed"; then
                        is_allowed=true
                        break
                    fi
                done
                if [ "$is_allowed" = false ]; then
                    filtered_matches="$filtered_matches$line"$'\n'
                fi
            done <<< "$matches"

            if [ -n "$filtered_matches" ]; then
                found_violations=1
                violations+=("$pattern")
                echo -e "${RED}✗ 发现违规: $pattern${NC}"
                if [ "$VERBOSE" = true ]; then
                    echo -e "${RED}$filtered_matches${NC}"
                    echo ""
                fi
            fi
        fi
    done

    # 输出详细报告
    if [ "$found_violations" -eq 1 ]; then
        echo ""
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}  ✗ 检查失败！${NC}"
        echo -e "${RED}========================================${NC}"
        echo ""
        echo -e "${RED}发现 ${#violations[@]} 种违规的 SFML 使用:${NC}"
        echo ""
        for violation in "${violations[@]}"; do
            echo -e "${RED}  • $violation${NC}"
        done
        echo ""
        echo -e "${YELLOW}根据 ENGINEERING_STANDARDS.md:${NC}"
        echo -e "${YELLOW}  domain 层禁止使用 SFML 相关类型和函数${NC}"
        echo -e "${YELLOW}  纯玩法规则应该只依赖标准库和 domain 内部接口${NC}"
        echo ""
        echo -e "${BLUE}修复建议:${NC}"
        echo -e "${BLUE}  1. 将 SFML 相关代码移到 engine 或 infrastructure 层${NC}"
        echo -e "${BLUE}  2. 使用抽象接口解耦 domain 和 SFML${NC}"
        echo -e "${BLUE}  3. 将渲染相关逻辑移到 engine/rendering/ 目录${NC}"
        echo ""

        if [ "$VERBOSE" = true ]; then
            echo -e "${BLUE}详细违规列表:${NC}"
            for pattern in "${violations[@]}"; do
                echo ""
                echo -e "${RED}  模式: $pattern${NC}"
                find "$DOMAIN_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) \
                    -exec grep -Hn "$pattern" {} \; 2>/dev/null || true
            done
        fi

        return 1
    else
        echo ""
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  ✓ 检查通过！${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo ""
        echo -e "${GREEN}domain 层未发现 SFML 违规使用${NC}"
        echo ""

        if [ "$VERBOSE" = true ]; then
            echo -e "${BLUE}检查的模式:${NC}"
            for pattern in "${patterns[@]}"; do
                echo -e "${BLUE}  ✓ $pattern${NC}"
            done
        fi

        return 0
    fi
}

# =============================================================================
# 主程序
# =============================================================================
main() {
    check_domain_sfml
    exit $?
}

main
