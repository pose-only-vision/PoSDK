#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Script to build and install PoseLib locally within the dependencies folder.
# This script assumes it is located in 'PoMVG/src/dependencies/'
# and will create or use the PoseLib source code in 'PoMVG/src/dependencies/PoseLib/'.

echo "--- PoseLib Local Build and Install Script ---"

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

POSELIB_SRC_DIR="${SCRIPT_DIR}/PoseLib"
POSELIB_BUILD_DIR="${POSELIB_SRC_DIR}/build_local"
POSELIB_INSTALL_DIR="${POSELIB_SRC_DIR}/install_local"

# Check if PoseLib source directory exists
if [ ! -d "${POSELIB_SRC_DIR}" ]; then
    echo "PoseLib source directory not found at ${POSELIB_SRC_DIR}"
    echo "Cloning PoseLib from GitHub..."
    
    # Clone PoseLib repository
    git clone https://github.com/PoseLib/PoseLib.git "${POSELIB_SRC_DIR}"
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to clone PoseLib repository"
        exit 1
    fi
    
    echo "✅ PoseLib cloned successfully"
else
    echo "✅ PoseLib source directory found at ${POSELIB_SRC_DIR}"
fi

# Verify this is a valid PoseLib source directory
if [ ! -f "${POSELIB_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${POSELIB_SRC_DIR}. Is it a valid PoseLib source directory?"
    exit 1
fi

echo "Source directory: ${POSELIB_SRC_DIR}"
echo "Build directory: ${POSELIB_BUILD_DIR}"
echo "Install directory: ${POSELIB_INSTALL_DIR}"

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
if [ -d "${POSELIB_INSTALL_DIR}" ] && [ "$(find "${POSELIB_INSTALL_DIR}" -name "*.so" -o -name "*.dylib" -o -name "*.a" 2>/dev/null | head -1)" ]; then
    print_warning "Found existing PoseLib installation at: ${POSELIB_INSTALL_DIR}"
    print_warning "发现现有PoseLib安装：${POSELIB_INSTALL_DIR}"

    # List some key files to show what's installed
    echo "Existing installation contains:"
    find "${POSELIB_INSTALL_DIR}" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.a" \) | head -3 | while read file; do
        echo "  - $(basename "$file")"
    done

    read -p "Skip rebuild and use existing PoseLib installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing PoseLib installation"
        print_info "使用现有PoseLib安装"
        print_success "PoseLib is already available at: ${POSELIB_INSTALL_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

# Clean existing build and install directories for fresh build | 清理现有构建和安装目录进行全新构建
if [ -d "${POSELIB_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${POSELIB_BUILD_DIR}"
    rm -rf "${POSELIB_BUILD_DIR}"
fi

if [ -d "${POSELIB_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${POSELIB_INSTALL_DIR}"
    rm -rf "${POSELIB_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${POSELIB_BUILD_DIR}"
mkdir -p "${POSELIB_INSTALL_DIR}"

# ========== BUILD AND INSTALL POSELIB ==========
echo ""
echo "========== Building PoseLib =========="
cd "${POSELIB_BUILD_DIR}"

# Check for required dependencies
echo "Checking dependencies..."

# Check for Eigen3
if ! pkg-config --exists eigen3; then
    echo "Warning: Eigen3 not found via pkg-config. Trying to find it via CMake..."
fi

# Check for OpenCV (if needed by PoseLib)
if ! pkg-config --exists opencv4; then
    echo "Warning: OpenCV4 not found via pkg-config. Trying to find it via CMake..."
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

echo "Configuring PoseLib with CMake..."
echo ""
print_info "Building both static and shared libraries..."
print_info "同时构建静态库和共享库..."
echo ""

# First pass: Build and install shared library | 第一遍：构建并安装共享库
print_info "Step 1: Building shared library (dynamic linking)..."
print_info "步骤 1: 构建共享库（动态链接）..."
cmake .. \
    -DCMAKE_INSTALL_PREFIX="${POSELIB_INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for shared library"
    echo "Please check if all dependencies are installed:"
    echo "- Eigen3 (≥3.3)"
    echo "- OpenCV (≥3.0, optional)"
    echo ""
    echo "On macOS with Homebrew:"
    echo "  brew install eigen opencv"
    echo ""
    echo "On Ubuntu/Debian:"
    echo "  sudo apt-get install libeigen3-dev libopencv-dev"
    exit 1
fi

echo "Building shared library..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "Error: Shared library build failed"
    exit 1
fi

echo "Installing shared library to ${POSELIB_INSTALL_DIR}..."
make install

if [ $? -ne 0 ]; then
    echo "Error: Shared library installation failed"
    exit 1
fi

print_success "Shared library built successfully"
print_success "共享库构建成功"
echo ""

# Second pass: Build and install static library | 第二遍：构建并安装静态库
print_info "Step 2: Building static library (static linking)..."
print_info "步骤 2: 构建静态库（静态链接）..."
echo ""

# Clean build artifacts but keep install directory
echo "Cleaning build artifacts..."
cd "${POSELIB_SRC_DIR}"
rm -rf "${POSELIB_BUILD_DIR}"
mkdir -p "${POSELIB_BUILD_DIR}"

cd "${POSELIB_BUILD_DIR}"

cmake .. \
    -DCMAKE_INSTALL_PREFIX="${POSELIB_INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for static library"
    exit 1
fi

echo "Building static library..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "Error: Static library build failed"
    exit 1
fi

echo "Installing static library to ${POSELIB_INSTALL_DIR}..."
make install

if [ $? -ne 0 ]; then
    echo "Error: Static library installation failed"
    exit 1
fi

print_success "Static library built successfully"
print_success "静态库构建成功"
echo ""

echo ""
echo "--- PoseLib local build and installation complete! ---"
echo "PoseLib has been installed in: ${POSELIB_INSTALL_DIR}"
echo "  - Shared Library: ${POSELIB_INSTALL_DIR}/lib/libPoseLib.so"
echo "  - Headers: ${POSELIB_INSTALL_DIR}/include/"
echo "  - CMake config: ${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib/"
echo ""

# Verify installation (cross-platform: check for .so, .dylib, or .a)
# 验证安装（跨平台：检查 .so、.dylib 或 .a）
echo "Verifying PoseLib installation..."
LIBRARY_FOUND=false

# Check for shared library on Linux (.so)
if [ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.so" ]; then
    echo "✅ Shared library found (Linux): ${POSELIB_INSTALL_DIR}/lib/libPoseLib.so"
    LIBRARY_FOUND=true
fi

# Check for shared library on macOS (.dylib)
if ls "${POSELIB_INSTALL_DIR}/lib"/libPoseLib*.dylib 1> /dev/null 2>&1; then
    DYLIB_FILE=$(ls "${POSELIB_INSTALL_DIR}/lib"/libPoseLib*.dylib | head -1)
    echo "✅ Shared library found (macOS): $(basename "$DYLIB_FILE")"
    LIBRARY_FOUND=true
fi

# Check for static library (.a)
if [ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a" ]; then
    echo "✅ Static library found: ${POSELIB_INSTALL_DIR}/lib/libPoseLib.a"
    LIBRARY_FOUND=true
fi

if [ "$LIBRARY_FOUND" = false ]; then
    echo "❌ No PoseLib library found, checking alternative locations..."
    # PoseLib might install with different naming
    if [ -f "${POSELIB_BUILD_DIR}/libPoseLib.so" ] || ls "${POSELIB_BUILD_DIR}"/libPoseLib*.dylib 1> /dev/null 2>&1; then
        echo "✅ Library found in build directory: ${POSELIB_BUILD_DIR}/"
        ls -la "${POSELIB_BUILD_DIR}"/libPoseLib* 2>/dev/null || true
    else
        echo "❌ Library not found in expected locations"
        echo "Listing contents of lib directory:"
        ls -la "${POSELIB_INSTALL_DIR}/lib/" 2>/dev/null || echo "Install lib directory not found"
        ls -la "${POSELIB_BUILD_DIR}/" 2>/dev/null | head -20 || echo "Build directory not found"
    fi
fi

if [ -d "${POSELIB_INSTALL_DIR}/include" ]; then
    echo "✅ Headers found: ${POSELIB_INSTALL_DIR}/include/"
else
    echo "❌ Headers not found, checking alternative locations..."
    if [ -d "${POSELIB_SRC_DIR}/include" ]; then
        echo "✅ Headers found in source directory: ${POSELIB_SRC_DIR}/include/"
    else
        echo "❌ Headers not found in expected locations"
    fi
fi

echo ""
echo "To use this PoseLib version in your main project:"
echo "The CMake configuration in src/plugins/methods/CMakeLists.txt should automatically"
echo "detect this installation and configure the poselib_model_estimator plugin."
echo ""
echo "If you encounter issues, try rebuilding your main project:"
echo "cd ${SCRIPT_DIR}/../.."
echo "rm -rf build && mkdir build && cd build"
echo "cmake .. && make"
echo ""
echo "Or if using Qt Creator, simply clean and rebuild the project." 