#!/bin/bash
# Script to download, build, and install OpenGV locally within the dependencies folder.
# This script assumes it is located in 'PoMVG/src/dependencies/'
# and will install OpenGV to 'PoMVG/src/dependencies/opengv/install_local/'.

set -e # Exit immediately if a command exits with a non-zero status.

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

# Define directories
OPENGV_SRC_DIR="${SCRIPT_DIR}/opengv"
OPENGV_BUILD_DIR="${OPENGV_SRC_DIR}/build_local" # Build directory inside opengv
OPENGV_INSTALL_DIR="${OPENGV_SRC_DIR}/install_local" # Local install directory inside opengv

echo "--- OpenGV Local Build and Install Script ---"

# Check if OpenGV source directory exists
if [ ! -d "${OPENGV_SRC_DIR}" ]; then
    echo "Error: OpenGV source directory not found at ${OPENGV_SRC_DIR}"
    echo "Please download or clone OpenGV into that location first."
    echo "You can run: git clone https://github.com/laurentkneip/opengv.git ${OPENGV_SRC_DIR}"
    exit 1
fi

if [ ! -f "${OPENGV_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${OPENGV_SRC_DIR}. Is it a valid OpenGV source directory?"
    exit 1
fi

echo "Source directory: ${OPENGV_SRC_DIR}"
echo "Build directory: ${OPENGV_BUILD_DIR}"
echo "Install directory: ${OPENGV_INSTALL_DIR}"

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

# Check for existing installation and ask user | 检查现有安装并询问用户
if [ -d "${OPENGV_INSTALL_DIR}" ] && [ "$(find "${OPENGV_INSTALL_DIR}" -name "*.so" -o -name "*.dylib" -o -name "*.a" 2>/dev/null | head -1)" ]; then
    print_warning "Found existing OpenGV installation at: ${OPENGV_INSTALL_DIR}"
    print_warning "发现现有OpenGV安装：${OPENGV_INSTALL_DIR}"

    # List some key files to show what's installed
    echo "Existing installation contains:"
    find "${OPENGV_INSTALL_DIR}" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.a" \) | head -3 | while read file; do
        echo "  - $(basename "$file")"
    done

    read -p "Skip rebuild and use existing OpenGV installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing OpenGV installation"
        print_info "使用现有OpenGV安装"
        print_success "OpenGV is already available at: ${OPENGV_INSTALL_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

# Clean existing build and install directories for fresh build | 清理现有构建和安装目录进行全新构建
if [ -d "${OPENGV_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${OPENGV_BUILD_DIR}"
    rm -rf "${OPENGV_BUILD_DIR}"
fi

if [ -d "${OPENGV_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${OPENGV_INSTALL_DIR}"
    rm -rf "${OPENGV_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${OPENGV_BUILD_DIR}"
mkdir -p "${OPENGV_INSTALL_DIR}"

# Apply CMake compatibility fix | 应用CMake兼容性修复
OPENGV_CMAKE_FILE="${OPENGV_SRC_DIR}/CMakeLists.txt"
if [ -f "${OPENGV_CMAKE_FILE}" ]; then
    # Check current CMake version requirement | 检查当前CMake版本要求
    CURRENT_CMAKE_VERSION=$(grep "cmake_minimum_required" "${OPENGV_CMAKE_FILE}" | head -1)
    echo "Current CMake requirement in OpenGV: ${CURRENT_CMAKE_VERSION}"

    # Fix CMake compatibility issue (VERSION 3.1.3 -> VERSION 3.5) | 修复CMake兼容性问题
    if grep -q "cmake_minimum_required(VERSION 3.1.3)" "${OPENGV_CMAKE_FILE}"; then
        print_info "Applying CMake compatibility fix: VERSION 3.1.3 -> VERSION 3.5"
        print_info "应用CMake兼容性修复：VERSION 3.1.3 -> VERSION 3.5"
        sed -i.bak 's/cmake_minimum_required(VERSION 3.1.3)/cmake_minimum_required(VERSION 3.5)/' "${OPENGV_CMAKE_FILE}"
        print_success "CMake compatibility fix applied successfully"
        print_success "CMake兼容性修复应用成功"
    elif grep -q "cmake_minimum_required(VERSION 3.1)" "${OPENGV_CMAKE_FILE}"; then
        print_info "Applying CMake compatibility fix: VERSION 3.1 -> VERSION 3.5"
        print_info "应用CMake兼容性修复：VERSION 3.1 -> VERSION 3.5"
        sed -i.bak 's/cmake_minimum_required(VERSION 3.1)/cmake_minimum_required(VERSION 3.5)/' "${OPENGV_CMAKE_FILE}"
        print_success "CMake compatibility fix applied successfully"
        print_success "CMake兼容性修复应用成功"
    else
        print_info "CMake version requirement looks compatible, no fix needed"
        print_info "CMake版本要求看起来兼容，无需修复"
    fi
fi

echo "Configuring OpenGV with CMake..."
cd "${OPENGV_BUILD_DIR}"

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

# Configure CMake for OpenGV
# -DCMAKE_INSTALL_PREFIX: Specifies where 'make install' will place the files.
# -DBUILD_SHARED_LIBS=ON: Build as shared library.
# -DCMAKE_POSITION_INDEPENDENT_CODE=ON: Important for shared libraries.
# -DCMAKE_BUILD_TYPE: Can be Release, Debug, RelWithDebInfo, MinSizeRel.
# -DCMAKE_POLICY_VERSION_MINIMUM=3.5: Fix CMake compatibility issues | 修复CMake兼容性问题
cmake .. \
    -DCMAKE_INSTALL_PREFIX="${OPENGV_INSTALL_DIR}" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "Building OpenGV (using all available processors)..."
# 根据系统使用不同的并行编译命令
if [[ "$(uname)" == "Darwin" ]]; then
    # macOS 使用 sysctl 获取处理器数量
    make -j$(sysctl -n hw.ncpu)
else
    # Linux 使用 nproc
    make -j$(nproc)
fi

echo "Installing OpenGV to ${OPENGV_INSTALL_DIR}..."
make install

echo "--- OpenGV local build and installation complete! ---"
echo "OpenGV has been installed in: ${OPENGV_INSTALL_DIR}"
echo "The necessary CMake configuration files are located at: ${OPENGV_INSTALL_DIR}/lib/cmake/opengv/ (if available)"
echo "Headers: ${OPENGV_INSTALL_DIR}/include/"
echo "Libraries: ${OPENGV_INSTALL_DIR}/lib/"
echo ""
echo "To use this OpenGV version in your main project (e.g., PoMVG/src):"
echo "When running CMake for your main project, you need to tell it where to find this OpenGV installation."
echo "For example, if your main project's build directory is PoMVG/src/build, you might run:"
echo "cd PoMVG/src/build"
echo "cmake .. -DCMAKE_PREFIX_PATH=\"${OPENGV_INSTALL_DIR}\""
echo ""
echo "Alternatively, you can modify your main project's CMakeLists.txt to search in this path,"
echo "or set CMAKE_PREFIX_PATH as an environment variable." 