#!/bin/bash

# Script to install Boost locally for GLOMAP compatibility
# 脚本用于本地安装 Boost 以兼容 GLOMAP

set -e

# Color output functions
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
# In Docker containers running as root, skip sudo
# 在以 root 运行的 Docker 容器中，跳过 sudo
# If SUDO_PASSWORD_B64 is available, use it for non-interactive sudo
# 如果有SUDO_PASSWORD_B64，则用于非交互式sudo
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

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Boost version and paths
BOOST_VERSION="1.85.0"
BOOST_VERSION_UNDERSCORE="1_85_0"
BOOST_DIR="${SCRIPT_DIR}/boost_${BOOST_VERSION_UNDERSCORE}"
BOOST_INSTALL_DIR="${BOOST_DIR}/install_local"
BOOST_ARCHIVE="boost_${BOOST_VERSION_UNDERSCORE}.tar.gz"
BOOST_URL="https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST_ARCHIVE}"

echo "========================================"
echo "   Boost 本地安装脚本"
echo "   Boost Local Installation Script"
echo "========================================"
echo ""

print_info "目标版本 | Target version: Boost ${BOOST_VERSION}"
print_info "源码目录 | Source directory: ${BOOST_DIR}"
print_info "安装位置 | Install location: ${BOOST_INSTALL_DIR}"
echo ""

# =============================================================================
# Step 1: 检测本地是否有已经构建和安装的 Boost
# =============================================================================
if [ -d "${BOOST_DIR}" ] && [ -d "${BOOST_INSTALL_DIR}" ] && [ -f "${BOOST_INSTALL_DIR}/lib/libboost_system.a" ]; then
    print_success "✓ 检测到已构建和安装的 Boost ${BOOST_VERSION}"
    print_success "✓ Detected built and installed Boost ${BOOST_VERSION}"
    echo "  源码路径 | Source path: ${BOOST_DIR}"
    echo "  安装路径 | Install path: ${BOOST_INSTALL_DIR}"
    echo ""
    
    # Verify installation
    if [ -f "${BOOST_INSTALL_DIR}/include/boost/version.hpp" ]; then
        print_info "验证安装 | Verifying installation:"
        HEADER_COUNT=$(find "${BOOST_INSTALL_DIR}/include/boost" -name "*.hpp" 2>/dev/null | wc -l | tr -d ' ')
        LIB_COUNT=$(find "${BOOST_INSTALL_DIR}/lib" \( -name "*.a" -o -name "*.so" \) 2>/dev/null | wc -l | tr -d ' ')
        print_success "  ✓ Headers: ${HEADER_COUNT} files"
        print_success "  ✓ Libraries: ${LIB_COUNT} files"
    fi
    echo ""
    
    # 交互1：询问是否重新构建
    print_warning "重新构建模式将完全清理并重新构建（删除整个 boost_1_85_0 目录）"
    print_warning "Rebuild mode will completely clean and rebuild (remove entire boost_1_85_0 directory)"
    read -p "重新构建？| Rebuild? [Y/n]: " -n 1 -r
    echo ""

    # 默认值为 Y（重新构建），即默认采用重新构建模式（完全重新构建）
    # Default is Y (rebuild), i.e., default to rebuild mode (complete rebuild)
    if [[ -z "$REPLY" ]] || [[ $REPLY =~ ^[Yy]$ ]]; then
        # 分支1: 用户选择 Y 或直接回车 - 采用重新构建模式（默认）
        print_info "🔄 采用重新构建模式（默认）..."
        print_info "🔄 Using rebuild mode (default)..."
        echo ""
        
        # 完全清理：删除整个 boost_1_85_0 目录（包括源码和构建）
        print_info "完全删除旧的 Boost 目录（包括源码和构建）..."
        print_info "Completely removing old Boost directory (including source and build)..."
        smart_sudo rm -rf "${BOOST_DIR}"
        
        print_success "✓ 清理完成，准备重新解压并构建"
        print_success "✓ Cleanup completed, ready to re-extract and build"
        echo ""
        
        # 继续执行后续的解压和构建步骤（不退出）
    else
        # 分支2: 用户选择 n - 跳过构建，使用现有安装
        print_info "✅ 跳过重新构建 - 使用现有安装"
        print_info "✅ Skipping rebuild - using existing installation"
        print_success "Boost installation location: ${BOOST_INSTALL_DIR}"
        echo ""
        print_success "Boost 安装完成！可以继续安装其他依赖。"
        print_success "Boost installation complete! You can continue installing other dependencies."
        exit 0
    fi
elif [ -d "${BOOST_DIR}" ]; then
    # 分支1.2: 源码目录存在但安装不完整（可能之前构建失败）
    print_warning "⚠ 检测到不完整的 Boost 源码目录"
    print_warning "⚠ Detected incomplete Boost source directory"
    echo "  源码路径 | Source path: ${BOOST_DIR}"
    
    if [ ! -d "${BOOST_INSTALL_DIR}" ]; then
        print_warning "  ✗ 安装目录不存在 | Install directory not found"
    elif [ ! -f "${BOOST_INSTALL_DIR}/lib/libboost_system.a" ]; then
        print_warning "  ✗ 核心库文件不存在 | Core library file not found"
    fi
    echo ""
    
    print_info "自动清理不完整的源码目录..."
    print_info "Automatically cleaning incomplete source directory..."
    rm -rf "${BOOST_DIR}"
    
    print_success "✓ 清理完成，将重新解压并构建"
    print_success "✓ Cleanup completed, will re-extract and build"
    echo ""
fi

# =============================================================================
# Step 2: 检查压缩包，如不存在则下载
# =============================================================================
if [ ! -f "${SCRIPT_DIR}/${BOOST_ARCHIVE}" ]; then
    print_info "未找到 Boost 压缩包: ${BOOST_ARCHIVE}"
    print_info "Boost archive not found: ${BOOST_ARCHIVE}"
    print_info "准备下载 Boost ${BOOST_VERSION}..."
    print_info "Preparing to download Boost ${BOOST_VERSION}..."
    print_warning "文件大小约 130 MB，请耐心等待..."
    print_warning "File size is about 130 MB, please be patient..."
    echo ""
    
    cd "${SCRIPT_DIR}"
    
    # Try multiple download methods
    DOWNLOAD_SUCCESS=false
    
    # Method 1: wget with Sourceforge
    if command -v wget &> /dev/null && [ "$DOWNLOAD_SUCCESS" = false ]; then
        print_info "尝试使用 wget 从 Sourceforge 下载..."
        print_info "Trying wget from Sourceforge..."
        
        if wget -O "${BOOST_ARCHIVE}" \
            "https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_ARCHIVE}/download"; then
            
            FILE_SIZE=$(stat -f%z "${BOOST_ARCHIVE}" 2>/dev/null || stat -c%s "${BOOST_ARCHIVE}" 2>/dev/null || echo "0")
            if [ "$FILE_SIZE" -gt 100000000 ]; then  # > 100MB
                DOWNLOAD_SUCCESS=true
                print_success "✓ wget 下载成功"
                print_success "✓ wget download successful"
            else
                print_warning "下载的文件太小 (${FILE_SIZE} bytes)，尝试其他方法..."
                print_warning "Downloaded file too small (${FILE_SIZE} bytes), trying other methods..."
                smart_sudo rm -f "${BOOST_ARCHIVE}"
            fi
        fi
    fi
    
    # Method 2: curl with jfrog
    if command -v curl &> /dev/null && [ "$DOWNLOAD_SUCCESS" = false ]; then
        print_info "尝试使用 curl 从 jfrog 下载..."
        print_info "Trying curl from jfrog..."
        
        if curl -L -o "${BOOST_ARCHIVE}" "${BOOST_URL}"; then
            FILE_SIZE=$(stat -f%z "${BOOST_ARCHIVE}" 2>/dev/null || stat -c%s "${BOOST_ARCHIVE}" 2>/dev/null || echo "0")
            if [ "$FILE_SIZE" -gt 100000000 ]; then  # > 100MB
                DOWNLOAD_SUCCESS=true
                print_success "✓ curl 下载成功"
                print_success "✓ curl download successful"
            else
                print_warning "下载的文件太小 (${FILE_SIZE} bytes)"
                print_warning "Downloaded file too small (${FILE_SIZE} bytes)"
                smart_sudo rm -f "${BOOST_ARCHIVE}"
            fi
        fi
    fi
    
    # Check download result
    if [ "$DOWNLOAD_SUCCESS" = false ]; then
        print_error "✗ 所有下载方法都失败了"
        print_error "✗ All download methods failed"
        echo ""
        print_info "请手动下载 Boost ${BOOST_VERSION} 并放置到："
        print_info "Please manually download Boost ${BOOST_VERSION} and place it at:"
        echo "  ${SCRIPT_DIR}/${BOOST_ARCHIVE}"
        echo ""
        print_info "可用的下载源 | Available download sources:"
        echo "  1. https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_ARCHIVE}/download"
        echo "  2. ${BOOST_URL}"
        echo "  3. https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_ARCHIVE}"
        exit 1
    fi
    
    print_success "✓ 下载完成"
    print_success "✓ Download completed"
    FILE_SIZE_MB=$(du -h "${BOOST_ARCHIVE}" | cut -f1)
    echo "  文件大小 | File size: ${FILE_SIZE_MB}"
else
    print_success "✓ 找到已下载的 Boost 压缩包"
    print_success "✓ Found downloaded Boost archive"
    echo "  位置 | Location: ${SCRIPT_DIR}/${BOOST_ARCHIVE}"
    
    # Verify file size
    FILE_SIZE=$(stat -f%z "${SCRIPT_DIR}/${BOOST_ARCHIVE}" 2>/dev/null || stat -c%s "${SCRIPT_DIR}/${BOOST_ARCHIVE}" 2>/dev/null || echo "0")
    if [ "$FILE_SIZE" -lt 100000000 ]; then
        print_warning "⚠ 压缩包文件可能损坏（大小: ${FILE_SIZE} bytes < 100 MB）"
        print_warning "⚠ Archive may be corrupted (size: ${FILE_SIZE} bytes < 100 MB)"
        read -p "是否删除并重新下载？[Y/n]: " -n 1 -r
        echo ""
        if [[ -z "$REPLY" ]] || [[ ! $REPLY =~ ^[Nn]$ ]]; then
            smart_sudo rm -f "${SCRIPT_DIR}/${BOOST_ARCHIVE}"
            print_info "已删除旧文件，请重新运行脚本"
            print_info "Old file removed, please re-run the script"
            exit 0
        fi
    fi
fi

echo ""

# =============================================================================
# Step 3: 解压 Boost 压缩包
# =============================================================================
# 最后检查：确保 boost_1_85_0 目录不存在
# （如果还存在，可能是并发/竞争条件，自动清理）
if [ -d "${BOOST_DIR}" ]; then
    print_warning "⚠ Boost 源码目录意外存在（可能是并发构建或清理失败）"
    print_warning "⚠ Boost source directory unexpectedly exists (possible concurrent build or cleanup failure)"
    echo "  源码路径 | Source path: ${BOOST_DIR}"
    echo ""
    
    print_info "自动清理源码目录..."
    print_info "Automatically cleaning source directory..."
    rm -rf "${BOOST_DIR}"
    
    print_success "✓ 清理完成"
    print_success "✓ Cleanup completed"
    echo ""
fi

print_info "解压 Boost ${BOOST_VERSION}..."
print_info "Extracting Boost ${BOOST_VERSION}..."

cd "${SCRIPT_DIR}"
tar -xzf "${BOOST_ARCHIVE}"

if [ $? -ne 0 ]; then
    print_error "✗ 解压失败"
    print_error "✗ Extraction failed"
    exit 1
fi

if [ ! -d "${BOOST_DIR}" ]; then
    print_error "✗ 解压后未找到预期的目录: ${BOOST_DIR}"
    print_error "✗ Expected directory not found after extraction: ${BOOST_DIR}"
    exit 1
fi

print_success "✓ 解压完成"
print_success "✓ Extraction completed"
echo ""

# =============================================================================
# Step 4: 构建并安装 Boost
# =============================================================================
print_info "配置 Boost..."
print_info "Configuring Boost..."
echo ""

cd "${BOOST_DIR}"

# 验证 bootstrap.sh 存在
if [ ! -f "./bootstrap.sh" ]; then
    print_error "✗ bootstrap.sh 不存在！解压可能失败"
    print_error "✗ bootstrap.sh not found! Extraction may have failed"
    exit 1
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

print_info "运行 bootstrap.sh..."
print_info "Running bootstrap.sh..."

./bootstrap.sh --prefix="${BOOST_INSTALL_DIR}" \
    --with-libraries=system,filesystem,program_options,graph,test

if [ $? -ne 0 ]; then
    print_error "✗ Bootstrap 失败"
    print_error "✗ Bootstrap failed"
    print_error "可能原因："
    print_error "Possible reasons:"
    print_error "  1. 缺少必要的构建工具 | Missing required build tools (gcc/g++, make)"
    print_error "  2. 权限问题 | Permission issues"
    print_error "  3. 压缩包损坏 | Archive corrupted"
    exit 1
fi

print_success "✓ Bootstrap 完成"
print_success "✓ Bootstrap completed"
echo ""

# Detect number of CPU cores
if command -v nproc &> /dev/null; then
    NPROC=$(nproc)
elif command -v sysctl &> /dev/null; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=4
fi

print_info "开始编译 Boost（使用 ${NPROC} 个核心）..."
print_info "Starting Boost compilation (using ${NPROC} cores)..."
print_warning "这可能需要 10-30 分钟，请耐心等待..."
print_warning "This may take 10-30 minutes, please be patient..."
echo ""

# Build and install
./b2 -j${NPROC} \
    --prefix="${BOOST_INSTALL_DIR}" \
    --build-dir="${BOOST_DIR}/build" \
    variant=release \
    link=static,shared \
    threading=multi \
    install

if [ $? -ne 0 ]; then
    print_error "✗ 编译失败"
    print_error "✗ Compilation failed"
    exit 1
fi

echo ""
echo "========================================"
print_success "✓ Boost ${BOOST_VERSION} 安装完成！"
print_success "✓ Boost ${BOOST_VERSION} Installation Completed!"
echo "========================================"
echo ""
print_info "安装位置 | Installation location:"
echo "  ${BOOST_INSTALL_DIR}"
echo ""

# Verify installation
if [ -f "${BOOST_INSTALL_DIR}/include/boost/version.hpp" ]; then
    print_info "验证安装 | Verifying installation:"
    HEADER_COUNT=$(find "${BOOST_INSTALL_DIR}/include/boost" -name "*.hpp" 2>/dev/null | wc -l | tr -d ' ')
    LIB_COUNT=$(find "${BOOST_INSTALL_DIR}/lib" \( -name "*.a" -o -name "*.so" \) 2>/dev/null | wc -l | tr -d ' ')
    print_success "  ✓ Headers: ${HEADER_COUNT} files"
    print_success "  ✓ Libraries: ${LIB_COUNT} files"
    echo ""
fi

print_info "使用方法 | Usage:"
echo "  在 CMake 配置中添加："
echo "  Add to CMake configuration:"
echo "    -DBOOST_ROOT=${BOOST_INSTALL_DIR}"
echo "    -DBoost_NO_SYSTEM_PATHS=ON"
echo ""
print_success "Boost 安装完成！可以继续安装其他依赖。"
print_success "Boost installation complete! You can continue installing other dependencies."
