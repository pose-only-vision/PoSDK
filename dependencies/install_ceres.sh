#!/bin/bash

# 脚本用于在 Ubuntu 和 Mac 上从源码安装 Ceres Solver
# Script to install Ceres Solver from source on Ubuntu and Mac

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
        # Already root, no need for sudo | 已经是 root，不需要 sudo
        "$@"
    else
        # Not root, use sudo | 不是 root，使用 sudo
        if [[ -n "$SUDO_PASSWORD_B64" ]]; then
            # Use password from environment variable (non-interactive) | 使用环境变量中的密码（非交互式）
            printf '%s' "$SUDO_PASSWORD_B64" | base64 -d | sudo -S "$@"
        else
            # Use normal sudo (interactive) | 使用普通sudo（交互式）
            sudo "$@"
        fi
    fi
}

# Get script directory for absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CERES_DIR="${SCRIPT_DIR}/ceres-solver-2.2.0"
CERES_INSTALL_DIR="${CERES_DIR}/install_local"
CERES_TARBALL="${SCRIPT_DIR}/ceres-solver-2.2.0.tar.gz"

# Detect OS | 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="ubuntu"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="mac"
else
    print_error "不支持的操作系统: $OSTYPE"
    print_error "Unsupported OS: $OSTYPE"
    exit 1
fi

print_info "检测到操作系统: $OS"
print_info "Detected OS: $OS"

# Check for existing installation | 检查现有安装
if [ -d "${CERES_INSTALL_DIR}" ] && [ -f "${CERES_INSTALL_DIR}/lib/libceres.a" ]; then
    print_warning "Found existing Ceres Solver installation at: ${CERES_INSTALL_DIR}"
    print_warning "发现现有Ceres Solver安装：${CERES_INSTALL_DIR}"

    # Show version if possible
    if [ -f "${CERES_INSTALL_DIR}/include/ceres/version.h" ]; then
        echo "Ceres Solver version:"
        grep "define CERES_VERSION" "${CERES_INSTALL_DIR}/include/ceres/version.h" | head -3
    fi

    read -p "Skip rebuild and use existing Ceres Solver installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing Ceres Solver installation"
        print_info "使用现有Ceres Solver安装"
        print_success "Ceres Solver is already available at: ${CERES_INSTALL_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

# Check if ceres-solver-2.2.0 directory exists
if [ ! -d "${CERES_DIR}" ]; then
    # Try to extract from tarball if it exists
    if [ -f "${CERES_TARBALL}" ]; then
        print_info "解压 Ceres Solver 源码..."
        print_info "Extracting Ceres Solver source..."
        cd "${SCRIPT_DIR}"
        tar -xzf "${CERES_TARBALL}" 2>&1 | grep -v "Can't create" || true
        
        if [ ! -d "${CERES_DIR}" ]; then
            print_error "解压失败"
            print_error "Extraction failed"
            exit 1
        fi
    else
        # Try to download source code automatically
        print_warning "未找到 ceres-solver-2.2.0 目录和压缩包"
        print_warning "ceres-solver-2.2.0 directory and tarball not found"
        print_info "尝试自动下载 Ceres Solver 2.2.0 源码..."
        print_info "Attempting to download Ceres Solver 2.2.0 source code..."
        
        cd "${SCRIPT_DIR}"
        
        # Check if curl is available
        if ! command -v curl &> /dev/null; then
            print_error "curl 未安装，无法下载源码"
            print_error "curl not found, cannot download source code"
            print_info "请手动下载或安装 curl"
            print_info "Please download manually or install curl"
            exit 1
        fi
        
        # Download source code
        if curl -L -o "${CERES_TARBALL}" "https://github.com/ceres-solver/ceres-solver/archive/refs/tags/2.2.0.tar.gz"; then
            print_success "源码下载成功"
            print_success "Source code downloaded successfully"
            
            # Extract
            print_info "解压源码..."
            print_info "Extracting source code..."
            tar -xzf "${CERES_TARBALL}" 2>&1 | grep -v "Can't create" || true
            
            if [ ! -d "${CERES_DIR}" ]; then
                print_error "解压失败"
                print_error "Extraction failed"
                exit 1
            fi
            
            print_success "解压完成"
            print_success "Extraction completed"
        else
            print_error "下载失败"
            print_error "Download failed"
            print_info "请手动下载："
            print_info "Please download manually:"
            echo "  curl -L -o ${CERES_TARBALL} https://github.com/ceres-solver/ceres-solver/archive/refs/tags/2.2.0.tar.gz"
            echo "  cd ${SCRIPT_DIR} && tar -xzf ceres-solver-2.2.0.tar.gz"
            exit 1
        fi
    fi
fi

# Install dependencies and build | 安装依赖并编译
if [ "$OS" == "ubuntu" ]; then
    print_info "正在 Ubuntu 上安装依赖..."
    print_info "Installing dependencies on Ubuntu..."

    # Update package list | 更新软件包列表
    smart_sudo apt-get update

    # Detect Ubuntu version | 检测 Ubuntu 版本
    UBUNTU_VERSION=""
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        UBUNTU_VERSION="${VERSION_ID}"
    fi

    print_info "检测到 Ubuntu 版本: ${UBUNTU_VERSION}"
    print_info "Detected Ubuntu version: ${UBUNTU_VERSION}"

    # Install dependencies | 安装依赖
    # Ceres Solver 依赖：
    # - Eigen3: 线性代数库
    # - glog: Google 日志库
    # - gflags: Google 命令行标志库
    # - SuiteSparse: 稀疏矩阵库 (可选，但强烈推荐)
    # - BLAS & LAPACK: 线性代数库
    
    smart_sudo apt-get install -y \
        cmake \
        build-essential \
        libeigen3-dev \
        libgoogle-glog-dev \
        libgflags-dev \
        libatlas-base-dev \
        libsuitesparse-dev

    # Ubuntu 18.04 使用 libopenblas-dev，20.04+ 使用 libopenblas-openmp-dev
    if [[ "${UBUNTU_VERSION}" == "18.04" ]]; then
        smart_sudo apt-get install -y libopenblas-dev
    else
        smart_sudo apt-get install -y libopenblas-openmp-dev || smart_sudo apt-get install -y libopenblas-dev
    fi

elif [ "$OS" == "mac" ]; then
    print_info "正在 Mac 上安装依赖..."
    print_info "Installing dependencies on Mac..."

    # Check Homebrew | 检查 Homebrew
    if ! command -v brew &> /dev/null; then
        print_warning "未找到 Homebrew，跳过依赖安装"
        print_warning "Homebrew not found, skipping dependency installation"
        print_info "Ceres Solver 需要: cmake, eigen, glog, gflags, suite-sparse"
        print_info "Ceres Solver requires: cmake, eigen, glog, gflags, suite-sparse"
    else
        # Install dependencies via Homebrew | 通过 Homebrew 安装依赖
        brew install cmake eigen glog gflags suite-sparse || true
    fi
fi

# Build Ceres Solver | 构建 Ceres Solver
print_info "开始构建 Ceres Solver 2.2.0..."
print_info "Building Ceres Solver 2.2.0..."

cd "${CERES_DIR}"

# Clean previous build | 清理之前的构建
if [ -d "build_local" ]; then
    print_info "清理之前的构建..."
    print_info "Cleaning previous build..."
    rm -rf build_local
fi

# Create build directory | 创建构建目录
mkdir -p build_local install_local
cd build_local

# Configure CMake | 配置 CMake
print_info "配置 CMake..."
print_info "Configuring CMake..."

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

# Find system glog and gflags first to avoid target conflicts later
# 先查找系统的 glog 和 gflags，避免后续的 target 冲突
print_info "查找系统 glog 和 gflags..."
print_info "Finding system glog and gflags..."

cmake .. \
    -DCMAKE_INSTALL_PREFIX="${CERES_INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_BENCHMARKS=OFF \
    -DPROVIDE_UNINSTALL_TARGET=OFF \
    -DEXPORT_BUILD_DIR=ON

# Build | 编译
print_info "编译 Ceres Solver (使用 $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 个处理器)..."
print_info "Building Ceres Solver (using $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) processors)..."

NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j${NPROC}

# Install | 安装
print_info "安装到本地目录..."
print_info "Installing to local directory..."
make install

# Post-installation: Patch CeresConfig.cmake to avoid glog/gflags target conflicts
# 安装后处理：修补 CeresConfig.cmake 以避免 glog/gflags target 冲突
print_info "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
print_info "后处理：修补 CeresConfig.cmake..."
print_info "Post-processing: Patching CeresConfig.cmake..."
print_info "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

CERES_CONFIG_FILE="${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake"
if [ -f "${CERES_CONFIG_FILE}" ]; then
    # Create backup | 创建备份
    cp "${CERES_CONFIG_FILE}" "${CERES_CONFIG_FILE}.original"
    
    # Patch the find_package calls to check if targets already exist
    # 修补 find_package 调用，使其检查 target 是否已存在
    # This prevents conflicts when COLMAP has already found glog/gflags
    # 这可以防止当 COLMAP 已经找到 glog/gflags 时发生冲突
    
    print_info "添加 target 存在性检查..."
    print_info "Adding target existence checks..."
    
    # Use perl for reliable multi-line replacement (sed's \n doesn't work on all systems)
    # 使用 perl 进行可靠的多行替换（sed 的 \n 在所有系统上都不能正常工作）
    
    # Check if perl is available | 检查 perl 是否可用
    if command -v perl &> /dev/null; then
        print_info "使用 perl 进行修补..."
        print_info "Using perl for patching..."
        
        # Wrap find_package(Glog ...) with conditional check (matches QUIET or REQUIRED)
        # 包装 find_package(Glog ...) 并添加条件检查（匹配 QUIET 或 REQUIRED）
        perl -i.tmp -pe 's/find_package\(Glog\s+(?:QUIET|REQUIRED)\)/if(NOT TARGET glog::glog)\n  $&\nendif()/g' "${CERES_CONFIG_FILE}"
        
        # Wrap find_package(gflags ...) with conditional check (matches any arguments)
        # 包装 find_package(gflags ...) 并添加条件检查（匹配任何参数）
        perl -i.tmp -pe 's/find_package\(gflags\s+[^\)]+\)/if(NOT TARGET gflags)\n  $&\nendif()/g' "${CERES_CONFIG_FILE}"
        
        # Clean up temporary files | 清理临时文件
        rm -f "${CERES_CONFIG_FILE}.tmp"
    else
        # Fallback: use awk for systems without perl
        # 后备方案：对于没有 perl 的系统使用 awk
        print_warning "perl 不可用，使用 awk 作为后备方案..."
        print_warning "perl not available, using awk as fallback..."
        
        # Create temporary output file | 创建临时输出文件
        awk '
        {
            # Replace find_package(Glog QUIET) or find_package(Glog REQUIRED)
            # 替换 find_package(Glog QUIET) 或 find_package(Glog REQUIRED)
            if ($0 ~ /find_package\(Glog (QUIET|REQUIRED)\)/) {
                print "if(NOT TARGET glog::glog)"
                print "  " $0
                print "endif()"
                next
            }
            # Replace find_package(gflags ...) with any arguments
            # 替换 find_package(gflags ...) 及其任何参数
            if ($0 ~ /find_package\(gflags /) {
                print "if(NOT TARGET gflags)"
                print "  " $0
                print "endif()"
                next
            }
            # Print other lines as-is
            print $0
        }
        ' "${CERES_CONFIG_FILE}" > "${CERES_CONFIG_FILE}.new"
        
        # Replace original file | 替换原始文件
        mv "${CERES_CONFIG_FILE}.new" "${CERES_CONFIG_FILE}"
    fi
    
    print_success "✓ CeresConfig.cmake 已成功修补"
    print_success "✓ CeresConfig.cmake patched successfully"
    print_info "原始文件备份: ${CERES_CONFIG_FILE}.original"
    print_info "Original file backup: ${CERES_CONFIG_FILE}.original"
else
    print_warning "未找到 CeresConfig.cmake，跳过修补"
    print_warning "CeresConfig.cmake not found, skipping patch"
fi

echo ""

# Verify installation | 验证安装
print_info "验证安装..."
print_info "Verifying installation..."

if [ -f "${CERES_INSTALL_DIR}/lib/libceres.a" ]; then
    print_success "✓ Ceres Solver 静态库已安装"
    print_success "✓ Ceres Solver static library installed"
fi

if [ -f "${CERES_INSTALL_DIR}/include/ceres/ceres.h" ]; then
    print_success "✓ Ceres Solver 头文件已安装"
    print_success "✓ Ceres Solver headers installed"
fi

if [ -f "${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake" ]; then
    print_success "✓ Ceres Solver CMake 配置已安装"
    print_success "✓ Ceres Solver CMake config installed"
fi

print_success "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
print_success "Ceres Solver 2.2.0 安装完成"
print_success "Ceres Solver 2.2.0 installation complete"
print_success "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
print_info "本地安装路径: ${CERES_INSTALL_DIR}"
print_info "Local installation path: ${CERES_INSTALL_DIR}"
echo ""
print_info "CMake 配置文件: ${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake"
print_info "CMake config file: ${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake"
echo ""
print_info "在 CMakeLists.txt 中使用："
print_info "To use in CMakeLists.txt:"
echo "  set(Ceres_DIR \"${CERES_INSTALL_DIR}/lib/cmake/Ceres\")"
echo "  find_package(Ceres REQUIRED)"
echo ""