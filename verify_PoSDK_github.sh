#!/bin/bash

# ============================================
# 验证 PoSDK_github 构建（仿github模式）
# Verify PoSDK_github build (GitHub-like mode)
# ============================================

set -e

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 密码环境变量处理
if [[ -n "$SUDO_PASSWORD_B64" ]] && [[ -z "$POSDK_SUDO_PASSWORD" ]]; then
    # 解码 base64 密码
    if POSDK_SUDO_PASSWORD=$(echo "$SUDO_PASSWORD_B64" | base64 -d 2>/dev/null); then
        export POSDK_SUDO_PASSWORD
    elif POSDK_SUDO_PASSWORD=$(echo "$SUDO_PASSWORD_B64" | base64 -D 2>/dev/null); then
        export POSDK_SUDO_PASSWORD
    fi
fi

# 获取脚本所在目录（PoSDK_github根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 显示环境信息
echo ""
echo -e "${CYAN}╔════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   验证 PoSDK_github 构建                   ║${NC}"
echo -e "${CYAN}║   Verify PoSDK_github Build                 ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}工作目录: ${GREEN}$SCRIPT_DIR${NC}"
echo ""

# 步骤 1: 检查必要文件
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[1/6] 检查 PoSDK_github 项目结构${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

if [[ ! -f "./CMakeLists.txt" ]]; then
    echo -e "${RED}✗ CMakeLists.txt 不存在${NC}"
    exit 1
fi

if [[ ! -f "./install.sh" ]]; then
    echo -e "${RED}✗ install.sh 不存在${NC}"
    exit 1
fi

echo -e "${GREEN}✓ PoSDK_github 项目结构完整${NC}"
echo ""

# 步骤 2: 运行 install.sh 安装依赖
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[2/6] 运行 install.sh 安装依赖${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

chmod +x ./install.sh

# 设置环境变量
export DEBIAN_FRONTEND=noninteractive

# 如果没有 SUDO_PASSWORD_B64 但有 POSDK_SUDO_PASSWORD，则编码它
if [[ -z "$SUDO_PASSWORD_B64" ]] && [[ -n "$POSDK_SUDO_PASSWORD" ]]; then
    export SUDO_PASSWORD_B64=$(printf '%s' "$POSDK_SUDO_PASSWORD" | base64)
fi

# 使用 yes 自动回答所有交互问题
set -o pipefail
(yes 2>/dev/null || true) | ./install.sh 2>&1 | tee install.log

INSTALL_EXIT=${PIPESTATUS[1]}

echo ""
if [ $INSTALL_EXIT -eq 0 ]; then
    echo -e "${GREEN}✓ PoSDK 依赖安装完成${NC}"
else
    echo -e "${RED}✗ PoSDK 依赖安装失败（退出码: $INSTALL_EXIT）${NC}"
    echo ""
    if [ -f install.log ]; then
        LOG_SIZE=$(wc -l < install.log)
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${YELLOW}安装日志（共 $LOG_SIZE 行，显示最后 100 行）:${NC}"
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        tail -n 100 install.log
        echo ""
    fi
    exit $INSTALL_EXIT
fi

echo ""

# 步骤 3: 清理构建目录
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[3/6] 清理构建目录${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

BUILD_DIR="./build"
if [[ -d "$BUILD_DIR" ]]; then
    echo -e "${YELLOW}清理旧的构建目录...${NC}"
    rm -rf "$BUILD_DIR"/*
    # 清理可能存在的隐藏文件（如 .cmake 缓存）
    find "$BUILD_DIR" -mindepth 1 -maxdepth 1 -name ".*" ! -name "." ! -name ".." -exec rm -rf {} + 2>/dev/null || true
    echo -e "${GREEN}✓ 构建目录已清空${NC}"
    echo ""
else
    mkdir -p "$BUILD_DIR"
    echo -e "${GREEN}✓ 创建构建目录${NC}"
    echo ""
fi

# 步骤 4: 构建 PoSDK
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[4/6] 构建 PoSDK${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

cd "$BUILD_DIR"

echo -e "${CYAN}运行 CMake 配置...${NC}"
echo ""

# 配置 CMake（使用 Release 模式）
if cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tee cmake.log; then
    echo ""
    echo -e "${GREEN}✓ CMake 配置成功${NC}"
else
    CMAKE_EXIT=$?
    echo ""
    echo -e "${RED}✗ CMake 配置失败（退出码: $CMAKE_EXIT）${NC}"
    echo ""
    if [ -f cmake.log ]; then
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${YELLOW}CMake 日志的最后 50 行:${NC}"
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        tail -n 50 cmake.log
        echo ""
    fi
    exit $CMAKE_EXIT
fi

echo ""
echo -e "${CYAN}开始编译 PoSDK（使用 $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 个处理器）...${NC}"
echo ""

# 编译 PoSDK
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if make -j${NPROC} 2>&1 | tee make.log; then
    echo ""
    echo -e "${GREEN}✓ PoSDK 编译完成${NC}"
else
    MAKE_EXIT=$?
    echo ""
    echo -e "${RED}✗ PoSDK 编译失败（退出码: $MAKE_EXIT）${NC}"
    echo ""
    if [ -f make.log ]; then
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${YELLOW}编译日志的最后 100 行:${NC}"
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        tail -n 100 make.log
        echo ""
    fi
    exit $MAKE_EXIT
fi

echo ""

# 步骤 5: 验证可执行文件
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[5/6] 验证 PoSDK 可执行文件${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# 查找 PoSDK 可执行文件
POSDK_EXECUTABLE=$(find "$SCRIPT_DIR/build" -name "PoSDK" -type f -executable 2>/dev/null | head -1)

if [[ -z "$POSDK_EXECUTABLE" ]]; then
    echo -e "${RED}✗ 未找到 PoSDK 可执行文件${NC}"
    echo ""
    echo -e "${YELLOW}查找的位置:${NC}"
    find "$SCRIPT_DIR/build" -name "PoSDK" -o -name "posdk" 2>/dev/null || true
    echo ""
    echo -e "${YELLOW}构建目录内容:${NC}"
    ls -R "$SCRIPT_DIR/build" 2>/dev/null | head -50 || true
    exit 1
fi

echo -e "${GREEN}✓ 找到 PoSDK 可执行文件:${NC}"
echo -e "  ${CYAN}$POSDK_EXECUTABLE${NC}"
echo ""

# 测试执行
echo -e "${YELLOW}测试运行 PoSDK --help...${NC}"
if "$POSDK_EXECUTABLE" --help > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PoSDK 可正常运行${NC}"
else
    echo -e "${YELLOW}⚠ PoSDK --help 失败，但文件存在${NC}"
    echo -e "${YELLOW}  (某些程序 --help 返回非零退出码是正常的)${NC}"
fi

echo ""

# 步骤 6: 验证依赖库和工具
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[6/6] 验证依赖库和工具${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

DEPS_DIR="$SCRIPT_DIR/dependencies"
VERIFY_COUNT=0
SUCCESS_COUNT=0

# 检查预构建的 po_core 包
if ls "$DEPS_DIR"/po_core_*.tar.gz >/dev/null 2>&1; then
    PO_CORE_PKG=$(ls -t "$DEPS_DIR"/po_core_*.tar.gz | head -1)
    echo -e "${GREEN}✓ po_core 预构建包: $(basename $PO_CORE_PKG)${NC}"
    ((VERIFY_COUNT++))
    ((SUCCESS_COUNT++))
else
    echo -e "${YELLOW}⚠ po_core 预构建包未找到${NC}"
    ((VERIFY_COUNT++))
fi

# 检查 GLOMAP
GLOMAP_EXECUTABLE="$DEPS_DIR/glomap-main/install_local/bin/glomap"
if [[ -f "$GLOMAP_EXECUTABLE" ]]; then
    echo -e "${GREEN}✓ GLOMAP: 已安装${NC}"
    ((VERIFY_COUNT++))
    ((SUCCESS_COUNT++))
else
    echo -e "${YELLOW}⚠ GLOMAP: 未安装${NC}"
    ((VERIFY_COUNT++))
fi

# 检查 COLMAP
COLMAP_EXECUTABLE="$DEPS_DIR/colmap-main/install_local/bin/colmap"
if [[ -f "$COLMAP_EXECUTABLE" ]]; then
    echo -e "${GREEN}✓ COLMAP: 已安装${NC}"
    ((VERIFY_COUNT++))
    ((SUCCESS_COUNT++))
else
    echo -e "${YELLOW}⚠ COLMAP: 未安装${NC}"
    ((VERIFY_COUNT++))
fi

# 检查 OpenMVG
OPENMVG_BIN_DIR="$DEPS_DIR/openMVG/install_local/bin"
if [[ -d "$OPENMVG_BIN_DIR" ]] && [[ $(find "$OPENMVG_BIN_DIR" -name "openMVG_main_*" -type f 2>/dev/null | wc -l) -gt 0 ]]; then
    OPENMVG_COUNT=$(find "$OPENMVG_BIN_DIR" -name "openMVG_main_*" -type f 2>/dev/null | wc -l)
    echo -e "${GREEN}✓ OpenMVG: 已安装 ($OPENMVG_COUNT 个可执行文件)${NC}"
    ((VERIFY_COUNT++))
    ((SUCCESS_COUNT++))
else
    echo -e "${YELLOW}⚠ OpenMVG: 未安装${NC}"
    ((VERIFY_COUNT++))
fi

# 检查 GraphOptim
GRAPHOPTIM_BIN_DIR="$DEPS_DIR/GraphOptim/bin"
if [[ -d "$GRAPHOPTIM_BIN_DIR" ]] && [[ $(find "$GRAPHOPTIM_BIN_DIR" -type f -executable 2>/dev/null | wc -l) -gt 0 ]]; then
    GRAPHOPTIM_COUNT=$(find "$GRAPHOPTIM_BIN_DIR" -type f -executable 2>/dev/null | wc -l)
    echo -e "${GREEN}✓ GraphOptim: 已安装 ($GRAPHOPTIM_COUNT 个可执行文件)${NC}"
    ((VERIFY_COUNT++))
    ((SUCCESS_COUNT++))
else
    echo -e "${YELLOW}⚠ GraphOptim: 未安装${NC}"
    ((VERIFY_COUNT++))
fi

echo ""

# 最终汇总
echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   验证完成！                              ║${NC}"
echo -e "${GREEN}║   Verification Completed                   ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${CYAN}📦 构建结果:${NC}"
if [[ -n "$POSDK_EXECUTABLE" ]]; then
    echo -e "${BLUE}  PoSDK:${NC}      ${GREEN}✓ 已构建${NC} ($POSDK_EXECUTABLE)"
else
    echo -e "${BLUE}  PoSDK:${NC}      ${RED}✗ 构建失败${NC}"
fi
echo ""

echo -e "${CYAN}🔧 依赖验证 (${SUCCESS_COUNT}/${VERIFY_COUNT}):${NC}"
echo -e "${BLUE}  po_core:${NC}    $([ -f "$(ls -t $DEPS_DIR/po_core_*.tar.gz 2>/dev/null | head -1)" ] 2>/dev/null && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠${NC}")"
echo -e "${BLUE}  GLOMAP:${NC}     $([ -f "$GLOMAP_EXECUTABLE" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠${NC}")"
echo -e "${BLUE}  COLMAP:${NC}     $([ -f "$COLMAP_EXECUTABLE" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠${NC}")"
echo -e "${BLUE}  OpenMVG:${NC}    $([ -d "$OPENMVG_BIN_DIR" ] && [ $(find "$OPENMVG_BIN_DIR" -name "openMVG_main_*" -type f 2>/dev/null | wc -l) -gt 0 ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠${NC}")"
echo -e "${BLUE}  GraphOptim:${NC} $([ -d "$GRAPHOPTIM_BIN_DIR" ] && [ $(find "$GRAPHOPTIM_BIN_DIR" -type f -executable 2>/dev/null | wc -l) -gt 0 ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠${NC}")"
echo ""

if [[ -n "$POSDK_EXECUTABLE" ]]; then
    echo -e "${GREEN}✓ PoSDK_github 验证通过！${NC}"
    exit 0
else
    echo -e "${RED}✗ PoSDK_github 验证失败${NC}"
    exit 1
fi

