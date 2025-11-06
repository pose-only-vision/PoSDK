#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Script to build and install Abseil locally within the dependencies folder.
# Uses existing source code in dependencies/abseil-cpp (no git operations)

echo "--- Abseil Local Build and Install Script ---"

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

ABSEIL_VERSION="20250814.1"  # Target version (should match existing source)
ABSEIL_SRC_DIR="${SCRIPT_DIR}/abseil-cpp"
ABSEIL_BUILD_DIR="${ABSEIL_SRC_DIR}/build_local"
ABSEIL_INSTALL_DIR="${ABSEIL_SRC_DIR}/install_local"

echo "Target Abseil version: ${ABSEIL_VERSION}"
echo "Source directory: ${ABSEIL_SRC_DIR}"
echo "Build directory: ${ABSEIL_BUILD_DIR}"
echo "Install directory: ${ABSEIL_INSTALL_DIR}"

# Verify Abseil source directory exists
if [ ! -d "${ABSEIL_SRC_DIR}" ]; then
    echo "Error: Abseil source directory not found at ${ABSEIL_SRC_DIR}"
    echo "Please ensure the Abseil source code is available in that location."
    exit 1
fi

# Verify we have a valid Abseil source
if [ ! -f "${ABSEIL_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${ABSEIL_SRC_DIR}. Is it a valid Abseil source directory?"
    exit 1
fi

echo "Using existing Abseil source code (no git operations)"
echo "使用现有的Abseil源码（不执行git操作）"

# Check if Abseil is already built | 检查Abseil是否已经构建
ABSEIL_CONFIG_FILE="${ABSEIL_INSTALL_DIR}/lib/cmake/absl/abslConfig.cmake"
if [ -f "${ABSEIL_CONFIG_FILE}" ]; then
    echo ""
    echo "✅ Found existing Abseil installation at: ${ABSEIL_INSTALL_DIR}"
    echo "✅ 发现已存在的Abseil安装: ${ABSEIL_INSTALL_DIR}"
    echo "Config file: ${ABSEIL_CONFIG_FILE}"
    echo ""
    echo "Do you want to skip this installation and use the existing build? [Y/n]"
    echo "是否跳过此次安装并使用现有构建？[Y/n]"
    read -p "Choice (default: Y): " skip_choice

    # Default to Y if no input | 默认选择Y
    skip_choice=${skip_choice:-Y}

    if [[ "$skip_choice" =~ ^[Yy]$ ]] || [[ -z "$skip_choice" ]]; then
        echo "✅ Skipping Abseil build - using existing installation"
        echo "✅ 跳过Abseil构建 - 使用现有安装"
        echo "Abseil installation location: ${ABSEIL_INSTALL_DIR}"
        echo "Abseil安装位置: ${ABSEIL_INSTALL_DIR}"
        exit 0
    else
        echo "🔄 Proceeding with fresh Abseil build..."
        echo "🔄 继续进行新的Abseil构建..."
    fi
fi

echo "Source directory: ${ABSEIL_SRC_DIR}"
echo "Build directory: ${ABSEIL_BUILD_DIR}"
echo "Install directory: ${ABSEIL_INSTALL_DIR}"

# Clean existing build and install directories for fresh build
if [ -d "${ABSEIL_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${ABSEIL_BUILD_DIR}"
    rm -rf "${ABSEIL_BUILD_DIR}"
fi

if [ -d "${ABSEIL_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${ABSEIL_INSTALL_DIR}"
    rm -rf "${ABSEIL_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${ABSEIL_BUILD_DIR}"
mkdir -p "${ABSEIL_INSTALL_DIR}"

echo "Configuring Abseil with CMake..."
cd "${ABSEIL_BUILD_DIR}"

# Clean environment to avoid Anaconda pollution | 清理环境以避免Anaconda污染
ORIGINAL_PATH="${PATH}"
ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"

if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
    echo "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
    echo "Detected Anaconda/Conda in PATH, temporarily removing to avoid pollution"
    CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    export PATH="${CLEAN_PATH}"
fi

if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
    echo "检测到Anaconda在LD_LIBRARY_PATH中，临时移除"
    echo "Detected Anaconda in LD_LIBRARY_PATH, temporarily removing"
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
        echo "从CMAKE_PREFIX_PATH中移除Anaconda路径"
        echo "Removing Anaconda paths from CMAKE_PREFIX_PATH"
        export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
    fi
fi

if [ -n "${CONDA_PREFIX}" ]; then
    echo "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
    echo "Temporarily unsetting CONDA_PREFIX: ${CONDA_PREFIX}"
    unset CONDA_PREFIX
fi

# Configure CMake for Abseil
# -DCMAKE_INSTALL_PREFIX: Specifies where 'make install' will place the files.
# -DCMAKE_POSITION_INDEPENDENT_CODE=ON: Important for shared libraries.
# -DABSL_PROPAGATE_CXX_STD=ON: Helps with C++ standard compatibility.
# -DCMAKE_BUILD_TYPE: Can be Release, Debug, RelWithDebInfo, MinSizeRel.
# -DCMAKE_CXX_STANDARD=17: Explicitly set C++17 standard (required by Abseil)
cmake .. \
    -DCMAKE_INSTALL_PREFIX="${ABSEIL_INSTALL_DIR}" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DABSL_PROPAGATE_CXX_STD=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON

echo "Building Abseil (using all available processors)..."
make -j$(sysctl -n hw.ncpu)

echo "Installing Abseil to ${ABSEIL_INSTALL_DIR}..."
make install

echo "--- Abseil local build and installation complete! ---"
echo "Abseil has been installed in: ${ABSEIL_INSTALL_DIR}"
echo "The necessary CMake configuration files (abslConfig.cmake, abslTargets.cmake, etc.) are located at: ${ABSEIL_INSTALL_DIR}/lib/cmake/absl/"
echo ""
echo "To use this Abseil version in your main project (e.g., PoSDK):"
echo "When running CMake for your main project, you need to tell it where to find this Abseil installation."
echo "For example, if your main project's build directory is PoSDK/build, you might run:"
echo "cd PoSDK/build"
echo "cmake .. -DCMAKE_PREFIX_PATH=\"${ABSEIL_INSTALL_DIR}\""
echo ""
echo "Alternatively, you can modify your main project's CMakeLists.txt to search in this path,"
echo "or set CMAKE_PREFIX_PATH as an environment variable."