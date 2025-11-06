#!/bin/bash
# ==============================================================================
# CMake Installation Script (Simplified Version) | CMake安装脚本（简化版）
# ==============================================================================
# This script installs CMake 3.28+ from pre-downloaded source code
# 此脚本从预下载的源码安装CMake 3.28+
# 
# Prerequisites | 前置条件:
# - Run download_cmake.sh first to download source code
# - 先运行 download_cmake.sh 下载源码
# 
# Why CMake 3.28+? | 为什么需要CMake 3.28+？
# - GLOMAP requires CMake 3.24+ for modern C++ features
# - GLOMAP需要CMake 3.24+以支持现代C++特性
# - Ubuntu 18.04 only has CMake 3.10 by default
# - Ubuntu 18.04默认只有CMake 3.10
# ==============================================================================

set -e

# Color output functions | 彩色输出函数
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Smart sudo wrapper | 智能 sudo 包装函数
smart_sudo() {
    if [[ $EUID -eq 0 ]]; then
        "$@"
    else
        if [[ -n "$SUDO_PASSWORD_B64" ]]; then
            printf '%s' "$SUDO_PASSWORD_B64" | base64 -d | sudo -S "$@"
        else
            sudo "$@"
        fi
    fi
}

echo "=========================================="
echo "   CMake Installation | CMake安装"
echo "=========================================="
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# CMake version and paths
CMAKE_VERSION="3.30.6"
CMAKE_REQUIRED_MAJOR=3
CMAKE_REQUIRED_MINOR=28
CMAKE_TARBALL="${SCRIPT_DIR}/cmake-${CMAKE_VERSION}.tar.gz"
CMAKE_SOURCE_DIR="${SCRIPT_DIR}/cmake-${CMAKE_VERSION}"

print_info "目标版本 | Target version: CMake ${CMAKE_VERSION}"
print_info "要求版本 | Required version: >= ${CMAKE_REQUIRED_MAJOR}.${CMAKE_REQUIRED_MINOR}"
echo ""

# Step 1: Check current CMake version
print_info "步骤 1/4：检查当前 CMake 版本"
print_info "Step 1/4: Checking current CMake version"
echo ""

SKIP_INSTALL=false

if command -v cmake &> /dev/null; then
    CURRENT_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    print_info "当前版本 | Current version: ${CURRENT_VERSION}"
    
    # Parse version numbers
    CURRENT_MAJOR=$(echo $CURRENT_VERSION | cut -d. -f1)
    CURRENT_MINOR=$(echo $CURRENT_VERSION | cut -d. -f2)
    
    # Check if current version meets requirement
    if [[ $CURRENT_MAJOR -gt $CMAKE_REQUIRED_MAJOR ]] || \
       [[ $CURRENT_MAJOR -eq $CMAKE_REQUIRED_MAJOR && $CURRENT_MINOR -ge $CMAKE_REQUIRED_MINOR ]]; then
        print_success "✓ 当前 CMake 版本满足要求 (>= ${CMAKE_REQUIRED_MAJOR}.${CMAKE_REQUIRED_MINOR})"
        print_success "✓ Current CMake version meets requirement (>= ${CMAKE_REQUIRED_MAJOR}.${CMAKE_REQUIRED_MINOR})"
        SKIP_INSTALL=true
    else
        print_warning "⚠ 当前 CMake 版本过旧 (${CURRENT_VERSION} < ${CMAKE_REQUIRED_MAJOR}.${CMAKE_REQUIRED_MINOR})"
        print_warning "⚠ Current CMake version is too old (${CURRENT_VERSION} < ${CMAKE_REQUIRED_MAJOR}.${CMAKE_REQUIRED_MINOR})"
        print_info "需要升级以支持 GLOMAP | Upgrade required for GLOMAP support"
    fi
else
    print_info "未检测到 CMake | CMake not detected"
    print_info "将进行全新安装 | Will perform fresh installation"
fi

echo ""

# If version is sufficient, ask user if they want to skip
if [[ "$SKIP_INSTALL" == "true" ]]; then
    read -p "版本已满足要求，跳过安装？| Version sufficient, skip installation? [Y/n]: " -n 1 -r
    echo ""
    # 默认值为 Y（跳过安装），即默认不重新安装
    # Default is Y (skip), i.e., skip installation by default
    if [[ -z "$REPLY" ]] || [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "✅ 跳过安装（默认） | Skipping installation (default)"
        exit 0
    fi
    print_info "🔄 继续安装 | Proceeding with installation"
    echo ""
fi

# Step 2: Check CMake tarball and extract
print_info "步骤 2/4：检查并解压 CMake 源码包"
print_info "Step 2/4: Checking and extracting CMake tarball"
echo ""

if [ ! -f "${CMAKE_TARBALL}" ]; then
    print_error "未找到 CMake 源码包: ${CMAKE_TARBALL}"
    print_error "CMake tarball not found: ${CMAKE_TARBALL}"
    echo ""
    print_info "请先运行下载脚本："
    print_info "Please run download script first:"
    echo "  ./download_cmake.sh"
    exit 1
fi

print_success "✓ 找到 CMake 源码包"
print_success "✓ Found CMake tarball"

# Clean up any existing extracted directory (keep clean) | 清理现有解压目录（保持干净）
if [ -d "${CMAKE_SOURCE_DIR}" ]; then
    print_info "清理旧的解压目录 | Cleaning old extracted directory..."
    rm -rf "${CMAKE_SOURCE_DIR}"
fi

print_info "解压 CMake 源码包 | Extracting CMake tarball..."
tar -xzf "${CMAKE_TARBALL}" -C "${SCRIPT_DIR}"

if [ ! -f "${CMAKE_SOURCE_DIR}/bootstrap" ]; then
    print_error "CMake 源码解压失败或不完整（缺少 bootstrap 脚本）"
    print_error "CMake extraction failed or incomplete (bootstrap script missing)"
    exit 1
fi

print_success "✓ CMake 源码解压完成"
print_success "✓ CMake source extracted"
echo ""

# Step 3: Install build dependencies (Ubuntu only)
print_info "步骤 3/4：安装编译依赖"
print_info "Step 3/4: Installing build dependencies"
echo ""

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    print_info "检测到 Linux 系统，安装必要的编译工具..."
    print_info "Detected Linux system, installing necessary build tools..."
    
    smart_sudo apt-get update -qq
    smart_sudo apt-get install -y -qq \
        build-essential \
        libssl-dev \
        libcurl4-openssl-dev \
        zlib1g-dev
    
    print_success "编译依赖安装完成"
    print_success "Build dependencies installed"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    print_info "检测到 macOS 系统"
    print_info "Detected macOS system"
    print_info "假设已安装 Xcode Command Line Tools"
    print_info "Assuming Xcode Command Line Tools are installed"
else
    print_warning "未知操作系统: $OSTYPE"
    print_warning "Unknown OS: $OSTYPE"
fi

echo ""

# Step 3.5: Clean up old CMake installations (avoid conflicts)
print_info "步骤 3.5/4：清理旧的 CMake 安装（避免冲突）"
print_info "Step 3.5/4: Cleaning up old CMake installations (avoid conflicts)"
echo ""

# Remove old CMake from /usr/local
if [[ -d "/usr/local/share/cmake-3.25" ]]; then
    print_warning "Found old CMake 3.25 installation, removing..."
    smart_sudo rm -rf /usr/local/share/cmake-3.25
fi

# Clean up any other old CMake versions in /usr/local/share
for old_cmake_dir in /usr/local/share/cmake-3.{0..27}; do
    if [[ -d "$old_cmake_dir" ]]; then
        print_info "Removing old CMake directory: $old_cmake_dir"
        smart_sudo rm -rf "$old_cmake_dir"
    fi
done

print_success "Old CMake installations cleaned up"
print_success "旧 CMake 安装已清理"
echo ""

# Step 4: Configure, build and install CMake
print_info "步骤 4/4：配置、编译和安装 CMake"
print_info "Step 4/4: Configuring, building and installing CMake"
echo ""

cd "${CMAKE_SOURCE_DIR}"

# Detect number of CPU cores
if command -v nproc &> /dev/null; then
    NPROC=$(nproc)
elif command -v sysctl &> /dev/null; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=4
fi

# Clean environment to avoid Anaconda pollution | 清理环境以避免Anaconda污染
ORIGINAL_PATH="${PATH}"
ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"

if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
    print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
    print_info "Detected Anaconda/Conda in PATH, temporarily removing to avoid pollution"
    CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    export PATH="${CLEAN_PATH}"
fi

if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
    print_info "检测到Anaconda在LD_LIBRARY_PATH中，临时移除"
    print_info "Detected Anaconda in LD_LIBRARY_PATH, temporarily removing"
    CLEAN_LD_LIBRARY_PATH=$(echo "${LD_LIBRARY_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    if [ -n "${CLEAN_LD_LIBRARY_PATH}" ]; then
        export LD_LIBRARY_PATH="${CLEAN_LD_LIBRARY_PATH}"
    else
        unset LD_LIBRARY_PATH
    fi
fi

if [ -n "${CMAKE_PREFIX_PATH}" ]; then
    CLEAN_CMAKE_PREFIX_PATH=$(echo "${CMAKE_PREFIX_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    if [ -n "${CLEAN_CMAKE_PREFIX_PATH}" ] && [ "${CLEAN_CMAKE_PREFIX_PATH}" != "${CMAKE_PREFIX_PATH}" ]; then
        print_info "从CMAKE_PREFIX_PATH中移除Anaconda路径"
        print_info "Removing Anaconda paths from CMAKE_PREFIX_PATH"
        export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
    fi
fi

if [ -n "${CONDA_PREFIX}" ]; then
    print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
    print_info "Temporarily unsetting CONDA_PREFIX: ${CONDA_PREFIX}"
    unset CONDA_PREFIX
fi

print_info "配置 CMake (使用 ${NPROC} 个并行任务)..."
print_info "Configuring CMake (using ${NPROC} parallel jobs)..."
./bootstrap --prefix=/usr/local --parallel=${NPROC} --  \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_USE_OPENSSL=ON

print_info "编译 CMake..."
print_info "Building CMake..."
print_warning "这可能需要 10-30 分钟，请耐心等待..."
print_warning "This may take 10-30 minutes, please be patient..."
echo ""

make -j${NPROC}

print_info "安装 CMake..."
print_info "Installing CMake..."
smart_sudo make install

print_success "CMake 安装完成"
print_success "CMake installation completed"
echo ""

# Verify installation
print_info "验证安装 | Verifying installation..."
NEW_VERSION=$(/usr/local/bin/cmake --version | head -n1 | awk '{print $3}')
print_success "✓ 安装的 CMake 版本 | Installed CMake version: ${NEW_VERSION}"
echo ""

# Update PATH notice
if ! echo "$PATH" | grep -q "/usr/local/bin"; then
    print_warning "⚠️  /usr/local/bin 不在 PATH 中"
    print_warning "⚠️  /usr/local/bin is not in PATH"
    echo ""
    print_info "请运行以下命令："
    print_info "Please run the following command:"
    echo "  export PATH=/usr/local/bin:\$PATH"
    echo ""
    print_info "或将其添加到 ~/.bashrc 或 ~/.zshrc"
    print_info "Or add it to ~/.bashrc or ~/.zshrc"
fi

# Update hash table
hash -r 2>/dev/null || true

# Cleanup extracted directory (automatic - keeps environment clean) | 自动清理解压目录（保持环境干净）
print_info "清理解压目录以保持环境干净 | Cleaning extracted directory to keep clean..."
cd "${SCRIPT_DIR}"
rm -rf "${CMAKE_SOURCE_DIR}"
print_success "✓ 解压目录已清理（保留压缩包用于重新安装）"
print_success "✓ Extracted directory cleaned (tarball kept for reinstallation)"

echo ""
echo "=========================================="
print_success "✅ CMake ${NEW_VERSION} 安装成功！"
print_success "✅ CMake ${NEW_VERSION} installed successfully!"
echo "=========================================="
echo ""

print_info "使用方法 | Usage:"
echo "  cmake --version"
echo "  # 如果显示旧版本，请运行："
echo "  # If showing old version, run:"
echo "  hash -r"
echo "  # 或重新登录终端 | Or re-login to terminal"
echo ""

print_info "为什么需要 CMake 3.28+？| Why CMake 3.28+ is needed?"
echo "  • GLOMAP requires CMake 3.24+ for modern C++ features"
echo "  • GLOMAP需要CMake 3.24+以支持现代C++特性"
echo "  • Better C++17/20 support and improved performance"
echo "  • 更好的C++17/20支持和性能改进"
echo ""

