#!/bin/bash
# Script to download, build, and install OpenMVG
# It attempts to install to a system-wide location if permissions allow,
# otherwise, it may suggest or use a user-local path.

set -e # Exit immediately if a command exits with a non-zero status.

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# --- USER-PROVIDED LOCAL OpenMVG SOURCE --- 
# Define source directory relative to the script location
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OPENMVG_SRC_DIR="${SCRIPT_DIR}/openMVG" # Local dependencies path
# --- END USER-PROVIDED LOCAL OpenMVG SOURCE ---

OPENMVG_BUILD_DIR="${OPENMVG_SRC_DIR}/build_local" # Define a specific build directory
OPENMVG_INSTALL_DIR="${OPENMVG_SRC_DIR}/install_local" # Define local install directory
OPENMVG_SOURCE_SUBDIR="${OPENMVG_SRC_DIR}/src" # OpenMVG's CMakeLists.txt is in the src subdir
# OPENMVG_BRANCH="develop" # Not needed if using local source

# Determine OS and CMake flags
CMAKE_OSX_ARCHITECTURES_FLAG=""

if [[ "$(uname)" == "Darwin" ]]; then
    # macOS
    if [[ "$(uname -m)" == "arm64" ]]; then
        # Apple Silicon (M1/M2/...)
        CMAKE_OSX_ARCHITECTURES_FLAG="-DCMAKE_OSX_ARCHITECTURES=arm64"
    else
        # Intel Mac
        CMAKE_OSX_ARCHITECTURES_FLAG="-DCMAKE_OSX_ARCHITECTURES=x86_64"
    fi
    NPROC_CMD="sysctl -n hw.ncpu"
elif [[ "$(uname)" == "Linux" ]]; then
    # Linux
    NPROC_CMD="nproc"
else
    echo "Unsupported OS: $(uname)"
    exit 1
fi

# Use local install directory (consistent with other dependencies)
# 使用本地安装目录（与其他依赖保持一致）
INSTALL_PREFIX="${OPENMVG_INSTALL_DIR}"

echo "--- OpenMVG Installation Script (Using Local Source) ---"
echo "Using local OpenMVG source from: ${OPENMVG_SRC_DIR}"
echo "Build directory (will be created if it doesn't exist): ${OPENMVG_BUILD_DIR}"
echo "Installation prefix: ${INSTALL_PREFIX}"
echo "Requires: cmake, a C++17 compiler (like GCC or Clang), and necessary build tools."
echo "The OpenMVG source at ${OPENMVG_SRC_DIR} should already have its submodules initialized."

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
if [ -d "${OPENMVG_INSTALL_DIR}" ] && [ -d "${OPENMVG_INSTALL_DIR}/bin" ] && [ "$(find "${OPENMVG_INSTALL_DIR}/bin" -name "openMVG_main_*" -type f 2>/dev/null | head -1)" ]; then
    print_success "✓ Found existing OpenMVG installation at: ${OPENMVG_INSTALL_DIR}"
    print_success "✓ 发现现有OpenMVG安装：${OPENMVG_INSTALL_DIR}"

    # List some key binaries to show what's installed
    echo "Existing installation contains:"
    find "${OPENMVG_INSTALL_DIR}/bin" -name "openMVG_main_*" -type f 2>/dev/null | head -3 | while read file; do
        echo "  - $(basename "$file")"
    done
    echo ""

    read -p "Skip rebuild and use existing OpenMVG installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing OpenMVG installation"
        print_info "使用现有OpenMVG安装"
        print_success "OpenMVG binaries location: ${OPENMVG_INSTALL_DIR}/bin"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

echo "------------------------------------"
read -p "Press Enter to continue or Ctrl+C to abort..."

# ====================================================
# Clean old build and install directories - ensure clean build environment | 清理旧的构建和安装目录 - 确保干净的构建环境
# ====================================================
echo "正在检查并清理旧的构建和安装目录..."
echo "Checking and cleaning old build and install directories..."

# Clean main build directory | 清理主构建目录
if [ -d "${OPENMVG_BUILD_DIR}" ]; then
    echo "发现旧的构建目录: ${OPENMVG_BUILD_DIR}"
    echo "为确保干净构建，正在删除旧构建目录..."
    rm -rf "${OPENMVG_BUILD_DIR}"
    echo "✓ 旧构建目录清理完成"
else
    echo "✓ 未发现旧构建目录"
fi

# Clean main install directory | 清理主安装目录
if [ -d "${OPENMVG_INSTALL_DIR}" ]; then
    echo "发现旧的安装目录: ${OPENMVG_INSTALL_DIR}"
    echo "为确保干净安装，正在删除旧安装目录..."
    rm -rf "${OPENMVG_INSTALL_DIR}"
    echo "✓ 旧安装目录清理完成"
else
    echo "✓ 未发现旧安装目录"
fi

# Clean possible other build directories (e.g., user might have created before) | 清理可能的其他构建目录（例如用户之前可能创建的）
OTHER_BUILD_DIRS=("${OPENMVG_SRC_DIR}/build_release" "${OPENMVG_SRC_DIR}/build_debug" "${OPENMVG_SRC_DIR}/cmake-build-release" "${OPENMVG_SRC_DIR}/cmake-build-debug")
for build_dir in "${OTHER_BUILD_DIRS[@]}"; do
    if [ -d "${build_dir}" ]; then
        echo "发现其他构建目录: ${build_dir}"
        read -p "是否也要清理这个目录? (y/n): " -r
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "${build_dir}"
            echo "✓ 已清理: ${build_dir}"
        fi
    fi
done

echo "构建目录清理完成，准备开始构建..."
echo "------------------------------------"

# --- Skip Git Clone and Submodule Update --- 
# echo "Cloning OpenMVG repository (branch: ${OPENMVG_BRANCH}) into ${OPENMVG_SRC_DIR}..."
# git clone --branch "${OPENMVG_BRANCH}" https://github.com/openMVG/openMVG.git "${OPENMVG_SRC_DIR}"

# echo "Initializing and updating OpenMVG submodules..."
# cd "${OPENMVG_SRC_DIR}"
# git submodule update --init --recursive
# cd "${SCRIPT_DIR}" # Go back to script dir before creating build dir
# --- End Skip --- 

if [ ! -d "${OPENMVG_SOURCE_SUBDIR}" ]; then
    echo "Error: OpenMVG source subdirectory not found at ${OPENMVG_SOURCE_SUBDIR}"
    echo "Please ensure the path ${OPENMVG_SRC_DIR} contains the OpenMVG source, with a 'src' subdirectory."
    exit 1
fi
if [ ! -f "${OPENMVG_SOURCE_SUBDIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${OPENMVG_SOURCE_SUBDIR}"
    exit 1
fi

echo "Creating build directory: ${OPENMVG_BUILD_DIR}..."
mkdir -p "${OPENMVG_BUILD_DIR}"
cd "${OPENMVG_BUILD_DIR}"

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

echo "Configuring OpenMVG with CMake (Source: ${OPENMVG_SOURCE_SUBDIR}, Installing to ${INSTALL_PREFIX})..."
# Note: ConverterPoSDK is now merged into openMVG_sfm, can build shared libraries
# Add CMake policy version minimum to fix compatibility issues | 添加CMake策略最低版本以修复兼容性问题
# Disable EIGEN_MPL2_ONLY to allow SparseCholesky module (fixes Ubuntu 18.04 Eigen compatibility)
# 禁用 EIGEN_MPL2_ONLY 以允许使用 SparseCholesky 模块（修复 Ubuntu 18.04 Eigen 兼容性）

# Set RPATH based on OS | 根据操作系统设置RPATH
if [[ "$(uname)" == "Darwin" ]]; then
    # macOS: Use @loader_path
    INSTALL_RPATH="@loader_path/../lib"
    CMAKE_MACOSX_RPATH_FLAG="-DCMAKE_MACOSX_RPATH=ON"
else
    # Linux: Use $ORIGIN
    INSTALL_RPATH="\$ORIGIN/../lib"
    CMAKE_MACOSX_RPATH_FLAG=""
fi

cmake "${OPENMVG_SOURCE_SUBDIR}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DOpenMVG_BUILD_SHARED=ON \
    -DOpenMVG_BUILD_EXAMPLES=OFF \
    -DOpenMVG_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-UEIGEN_MPL2_ONLY -DEIGEN_MAX_ALIGN_BYTES=0" \
    ${CMAKE_MACOSX_RPATH_FLAG} \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE \
    -DCMAKE_INSTALL_RPATH="${INSTALL_RPATH}" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE \
    -DCMAKE_SKIP_BUILD_RPATH=FALSE \
    ${CMAKE_OSX_ARCHITECTURES_FLAG}

echo "Building OpenMVG (this may take a while, using $(${NPROC_CMD}) cores)..."
make -j$(${NPROC_CMD})

echo "Installing OpenMVG to local directory: ${INSTALL_PREFIX}..."
make install

if [ $? -ne 0 ]; then
    print_error "Installation failed"
    print_error "安装失败"
    exit 1
fi

print_success "✓ OpenMVG build and installation completed!"
print_success "✓ OpenMVG构建和安装完成！"

echo "------------------------------------"
echo "Installation Summary | 安装总结"
echo "------------------------------------"
echo "Source directory:  ${OPENMVG_SRC_DIR}"
echo "Build directory:   ${OPENMVG_BUILD_DIR}"
echo "Install directory: ${OPENMVG_INSTALL_DIR}"
echo "Binaries location: ${OPENMVG_INSTALL_DIR}/bin"
echo ""
echo "Main OpenMVG tools | 主要OpenMVG工具:"
echo "  - openMVG_main_SfMInit_ImageListing"
echo "  - openMVG_main_ComputeFeatures"
echo "  - openMVG_main_ComputeMatches"
echo "  - openMVG_main_GeometricFilter"
echo "  - openMVG_main_SfM"
echo ""
echo "Usage instructions | 使用说明:"
echo "- PoSDK will automatically detect binaries from: ${OPENMVG_INSTALL_DIR}/bin"
echo "- PoSDK会自动检测二进制文件：${OPENMVG_INSTALL_DIR}/bin"
echo "- For manual testing, you can run: ${OPENMVG_INSTALL_DIR}/bin/openMVG_main_SfMInit_ImageListing"
echo "- 手动测试可以运行：${OPENMVG_INSTALL_DIR}/bin/openMVG_main_SfMInit_ImageListing"
echo "------------------------------------"

exit 0
