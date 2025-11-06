#!/bin/bash
# ==============================================================================
# CMake Source Code Download Script | CMake源码下载脚本
# ==============================================================================
# This script downloads CMake source code from GitHub
# 此脚本从GitHub下载CMake源码
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

echo "=========================================="
echo "   CMake Source Download | CMake源码下载"
echo "=========================================="
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# CMake version to download (aligned with install_cmake.sh)
CMAKE_VERSION="3.30.6"
CMAKE_MAJOR_MINOR="3.30"

# Target tarball name
CMAKE_TARBALL="cmake-${CMAKE_VERSION}.tar.gz"

print_info "目标版本 | Target version: CMake ${CMAKE_VERSION}"
print_info "目标文件 | Target file: ${CMAKE_TARBALL}"
echo ""

cd "${SCRIPT_DIR}"

# Step 1: Download CMake source tarball
print_info "步骤 1/2：下载 CMake 源码包"
print_info "Step 1/2: Downloading CMake source tarball"
echo ""

CMAKE_DOWNLOAD_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${CMAKE_TARBALL}"
print_info "下载地址 | Download URL: ${CMAKE_DOWNLOAD_URL}"

if [ -f "${CMAKE_TARBALL}" ]; then
    print_info "发现已下载的源码包 | Found existing tarball"
    read -p "使用现有文件（不重新下载）？| Use existing file (skip re-download)? [Y/n]: " -n 1 -r
    echo ""
    # 默认值为 Y（使用现有文件），即默认不重新下载
    # Default is Y (use existing), i.e., skip re-download by default
    if [[ -z "$REPLY" ]] || [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "✅ 使用现有文件（默认） | Using existing file (default)"
    else
        print_info "🔄 重新下载 | Re-downloading"
        rm -f "${CMAKE_TARBALL}"
    fi
fi

if [ ! -f "${CMAKE_TARBALL}" ]; then
    print_info "正在下载... | Downloading..."
    print_info "文件大小约 10-15 MB，请耐心等待..."
    print_info "File size is about 10-15 MB, please be patient..."
    echo ""
    
    # Try to download with wget or curl
    if command -v wget &> /dev/null; then
        wget --no-check-certificate "${CMAKE_DOWNLOAD_URL}" -O "${CMAKE_TARBALL}" || {
            print_error "下载失败 | Download failed"
            exit 1
        }
    elif command -v curl &> /dev/null; then
        curl -L "${CMAKE_DOWNLOAD_URL}" -o "${CMAKE_TARBALL}" || {
            print_error "下载失败 | Download failed"
            exit 1
        }
    else
        print_error "未找到 wget 或 curl | wget or curl not found"
        print_info "请手动下载 ${CMAKE_TARBALL} 到 ${SCRIPT_DIR}"
        print_info "Please manually download ${CMAKE_TARBALL} to ${SCRIPT_DIR}"
        exit 1
    fi
    
    print_success "下载完成 | Download completed"
else
    print_success "源码包已存在 | Tarball already exists"
fi

echo ""

# Step 2: Verify and show summary
print_info "步骤 2/2：验证下载"
print_info "Step 2/2: Verifying download"
echo ""

if [ -f "${CMAKE_TARBALL}" ]; then
    TARBALL_SIZE=$(du -h "${CMAKE_TARBALL}" | cut -f1)
    
    print_success "✅ CMake 源码包下载成功！"
    print_success "✅ CMake tarball downloaded successfully!"
    echo ""
    print_info "文件信息 | File info:"
    echo "  文件名 | Filename: ${CMAKE_TARBALL}"
    echo "  大小 | Size: ${TARBALL_SIZE}"
    echo "  路径 | Path: ${SCRIPT_DIR}/${CMAKE_TARBALL}"
    echo ""
    
    print_info "下一步 | Next steps:"
    echo "  1. 运行安装脚本（会自动解压和编译）："
    echo "     Run install script (will extract and compile automatically):"
    echo "     ./install_cmake.sh"
    echo ""
    echo "  2. 或者打包到发布包："
    echo "     Or package for release:"
    echo "     ./package_deps_for_release.sh"
    echo ""
    
    print_info "说明 | Note:"
    echo "  • 压缩包将被保留，install_cmake.sh 会自动解压"
    echo "  • Tarball will be kept, install_cmake.sh will extract automatically"
    echo "  • 每次安装时会清理旧的解压目录，保持环境干净"
    echo "  • Old extracted directories will be cleaned before each installation"
    
else
    print_error "验证失败 | Verification failed"
    exit 1
fi

echo ""
echo "=========================================="
print_success "完成 | Completed"
echo "=========================================="
echo ""

