#!/bin/bash
# pre-commit hook installer for CloudSeaManor
# 自动安装 pre-commit hooks 到 .git/hooks/

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOOKS_DIR="$SCRIPT_DIR/.git/hooks"

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  CloudSeaManor Hook 安装脚本${NC}"
echo -e "${BLUE}======================================${NC}"

# 检查是否在项目根目录
if [ ! -d ".git" ]; then
    echo -e "${RED}✗ 错误：请在项目根目录运行此脚本${NC}"
    exit 1
fi

# 创建 hooks 目录
mkdir -p "$HOOKS_DIR"

# 安装 pre-commit hook
if [ "$(uname)" = "Darwin" ] || [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
    # Unix-like 系统
    cp "$SCRIPT_DIR/.git/hooks/pre-commit" "$HOOKS_DIR/pre-commit"
    chmod +x "$HOOKS_DIR/pre-commit"

    cp "$SCRIPT_DIR/.git/hooks/commit-msg" "$HOOKS_DIR/commit-msg"
    chmod +x "$HOOKS_DIR/commit-msg"

    echo -e "${GREEN}✓ 已安装 pre-commit hook (Unix)${NC}"
else
    # Windows 系统 - 复制 bat 版本
    cp "$SCRIPT_DIR/.git/hooks/pre-commit-windows.bat" "$HOOKS_DIR/pre-commit.bat"
    cp "$SCRIPT_DIR/.git/hooks/commit-msg" "$HOOKS_DIR/commit-msg"

    echo -e "${GREEN}✓ 已安装 pre-commit hook (Windows)${NC}"
    echo -e "${YELLOW}  注意：Windows 上需要手动安装 commit-msg hook${NC}"
    echo -e "${YELLOW}  或者使用 Git Bash 运行本脚本${NC}"
fi

echo ""
echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  安装完成！${NC}"
echo -e "${GREEN}======================================${NC}"
echo ""
echo "已安装的 hooks:"
ls -la "$HOOKS_DIR/pre-commit" "$HOOKS_DIR/commit-msg" 2>/dev/null || echo "  (部分 hooks 可能在 Windows 上需要额外配置)"
echo ""
echo "要卸载 hooks，请删除以下文件:"
echo "  .git/hooks/pre-commit"
echo "  .git/hooks/commit-msg"
