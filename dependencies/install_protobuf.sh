#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Script to build and install Protocol Buffers locally within the dependencies folder.
# This script assumes it is located in 'PoSDK_github/dependencies/'
# Compatible with Abseil 20250814

echo "--- Protocol Buffers Local Build and Install Script ---"

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

PROTOBUF_VERSION="v33.0"  # Latest version, compatible with Abseil 20250814
PROTOBUF_SRC_DIR="${SCRIPT_DIR}/protobuf"
PROTOBUF_BUILD_DIR="${PROTOBUF_SRC_DIR}/build_local"
PROTOBUF_INSTALL_DIR="${PROTOBUF_SRC_DIR}/install_local"

echo "Target Protobuf version: ${PROTOBUF_VERSION}"
echo "Source directory: ${PROTOBUF_SRC_DIR}"
echo "Build directory: ${PROTOBUF_BUILD_DIR}"
echo "Install directory: ${PROTOBUF_INSTALL_DIR}"

# Verify Protocol Buffers source directory exists
if [ ! -d "${PROTOBUF_SRC_DIR}" ]; then
    echo "Error: Protocol Buffers source directory not found at ${PROTOBUF_SRC_DIR}"
    echo "Please ensure the Protocol Buffers source code is available in that location."
    echo "You can download it from: https://github.com/protocolbuffers/protobuf/releases/tag/${PROTOBUF_VERSION}"
    exit 1
fi

# Verify we have a valid Protocol Buffers source and determine CMake path
CMAKE_SOURCE_PATH=""
if [ -f "${PROTOBUF_SRC_DIR}/CMakeLists.txt" ]; then
    CMAKE_SOURCE_PATH=".."
    echo "Found CMakeLists.txt in root directory, using path: ${CMAKE_SOURCE_PATH}"
elif [ -f "${PROTOBUF_SRC_DIR}/cmake/CMakeLists.txt" ]; then
    CMAKE_SOURCE_PATH="../cmake"
    echo "Found CMakeLists.txt in cmake directory, using path: ${CMAKE_SOURCE_PATH}"
else
    echo "Error: CMakeLists.txt not found in ${PROTOBUF_SRC_DIR} or ${PROTOBUF_SRC_DIR}/cmake"
    echo "Is it a valid Protocol Buffers source directory?"
    exit 1
fi

echo "Using existing Protocol Buffers source code (no git operations)"
echo "使用现有的Protocol Buffers源码（不执行git操作）"

# Check if Protocol Buffers is already built | 检查Protocol Buffers是否已经构建
PROTOBUF_CONFIG_FILE="${PROTOBUF_INSTALL_DIR}/lib/cmake/protobuf/protobuf-config.cmake"
if [ -f "${PROTOBUF_CONFIG_FILE}" ]; then
    echo ""
    echo "✅ Found existing Protocol Buffers installation at: ${PROTOBUF_INSTALL_DIR}"
    echo "✅ 发现已存在的Protocol Buffers安装: ${PROTOBUF_INSTALL_DIR}"
    echo "Config file: ${PROTOBUF_CONFIG_FILE}"
    echo ""
    echo "Do you want to skip this installation and use the existing build? [Y/n]"
    echo "是否跳过此次安装并使用现有构建？[Y/n]"
    read -p "Choice (default: Y): " skip_choice

    # Default to Y if no input | 默认选择Y
    skip_choice=${skip_choice:-Y}

    if [[ "$skip_choice" =~ ^[Yy]$ ]] || [[ -z "$skip_choice" ]]; then
        echo "✅ Skipping Protocol Buffers build - using existing installation"
        echo "✅ 跳过Protocol Buffers构建 - 使用现有安装"
        echo "Protocol Buffers installation location: ${PROTOBUF_INSTALL_DIR}"
        echo "Protocol Buffers安装位置: ${PROTOBUF_INSTALL_DIR}"
        exit 0
    else
        echo "🔄 Proceeding with fresh Protocol Buffers build..."
        echo "🔄 继续进行新的Protocol Buffers构建..."
    fi
fi

# Clean existing build and install directories for fresh build
if [ -d "${PROTOBUF_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${PROTOBUF_BUILD_DIR}"
    rm -rf "${PROTOBUF_BUILD_DIR}"
fi

if [ -d "${PROTOBUF_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${PROTOBUF_INSTALL_DIR}"
    rm -rf "${PROTOBUF_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${PROTOBUF_BUILD_DIR}"
mkdir -p "${PROTOBUF_INSTALL_DIR}"

echo "Configuring Protocol Buffers with CMake..."
cd "${PROTOBUF_BUILD_DIR}"

# Check if we have a local Abseil installation
ABSEIL_INSTALL_DIR="${SCRIPT_DIR}/abseil-cpp/install_local"
ABSEIL_CMAKE_PATH=""

if [ -d "${ABSEIL_INSTALL_DIR}/lib/cmake/absl" ]; then
    echo "Found local Abseil installation at: ${ABSEIL_INSTALL_DIR}"
    ABSEIL_CMAKE_PATH="-DCMAKE_PREFIX_PATH=${ABSEIL_INSTALL_DIR}"
else
    echo "Warning: Local Abseil not found. Protobuf will use system Abseil."
    echo "For best compatibility, please run install_absl.sh first."
fi

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

# Configure CMake for Protocol Buffers
# -Dprotobuf_BUILD_TESTS=OFF: Skip building tests for faster build
# -Dprotobuf_ABSL_PROVIDER=package: Use external Abseil (either local or system)
# -DCMAKE_POSITION_INDEPENDENT_CODE=ON: Important for shared libraries
# -DCMAKE_CXX_STANDARD=17: Explicitly set C++17 standard (consistent with Abseil)
cmake ${CMAKE_SOURCE_PATH} \
    -DCMAKE_INSTALL_PREFIX="${PROTOBUF_INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -Dprotobuf_BUILD_TESTS=OFF \
    -Dprotobuf_ABSL_PROVIDER=package \
    ${ABSEIL_CMAKE_PATH}

echo "Building Protocol Buffers (using all available processors)..."
make -j4

echo "Installing Protocol Buffers to ${PROTOBUF_INSTALL_DIR}..."
make install

echo "--- Protocol Buffers local build and installation complete! ---"
echo "Protocol Buffers has been installed in: ${PROTOBUF_INSTALL_DIR}"
echo "Version: ${PROTOBUF_VERSION}"
echo ""
echo "To use this Protocol Buffers version in your main project:"
echo "Set CMAKE_PREFIX_PATH to include both Abseil and Protobuf:"
echo "cmake .. -DCMAKE_PREFIX_PATH=\"${ABSEIL_INSTALL_DIR};${PROTOBUF_INSTALL_DIR}\""
echo ""
echo "Installed components:"
echo "- Headers: ${PROTOBUF_INSTALL_DIR}/include"
echo "- Libraries: ${PROTOBUF_INSTALL_DIR}/lib"
echo "- Binaries: ${PROTOBUF_INSTALL_DIR}/bin (protoc compiler)"
echo "- CMake config: ${PROTOBUF_INSTALL_DIR}/lib/cmake/protobuf"