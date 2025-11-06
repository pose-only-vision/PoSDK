#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Script to build and install nlohmann/json locally within the dependencies folder.
# Uses existing source code in dependencies/nlohmann (no git operations)

echo "--- nlohmann/json Local Build and Install Script ---"

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

NLOHMANN_VERSION="3.11.2"  # Target version (should match existing source)
NLOHMANN_SRC_DIR="${SCRIPT_DIR}/nlohmann"
NLOHMANN_BUILD_DIR="${NLOHMANN_SRC_DIR}/build_local"
NLOHMANN_INSTALL_DIR="${NLOHMANN_SRC_DIR}/install_local"

echo "Target nlohmann/json version: ${NLOHMANN_VERSION}"
echo "Source directory: ${NLOHMANN_SRC_DIR}"
echo "Build directory: ${NLOHMANN_BUILD_DIR}"
echo "Install directory: ${NLOHMANN_INSTALL_DIR}"

# Verify nlohmann source directory exists
if [ ! -d "${NLOHMANN_SRC_DIR}" ]; then
    echo "Error: nlohmann/json source directory not found at ${NLOHMANN_SRC_DIR}"
    echo "Please ensure the nlohmann/json source code is available in that location."
    echo ""
    echo "To download nlohmann/json source:"
    echo "  cd ${SCRIPT_DIR}"
    echo "  curl -L -o v${NLOHMANN_VERSION}.tar.gz https://github.com/nlohmann/json/archive/refs/tags/v${NLOHMANN_VERSION}.tar.gz"
    echo "  tar -xzf v${NLOHMANN_VERSION}.tar.gz"
    echo "  mv json-${NLOHMANN_VERSION} nlohmann"
    echo ""
    echo "Or run download_dependencies.sh first:"
    echo "  ./download_dependencies.sh"
    exit 1
fi

# Verify we have a valid nlohmann/json source
if [ ! -f "${NLOHMANN_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${NLOHMANN_SRC_DIR}. Is it a valid nlohmann/json source directory?"
    exit 1
fi

echo "Using existing nlohmann/json source code (no git operations)"
echo "使用现有的 nlohmann/json 源码（不执行 git 操作）"

# Check if nlohmann/json is already built | 检查 nlohmann/json 是否已经构建
NLOHMANN_CONFIG_FILE="${NLOHMANN_INSTALL_DIR}/share/cmake/nlohmann_json/nlohmann_jsonConfig.cmake"
if [ -f "${NLOHMANN_CONFIG_FILE}" ]; then
    echo ""
    echo "✅ Found existing nlohmann/json installation at: ${NLOHMANN_INSTALL_DIR}"
    echo "✅ 发现已存在的 nlohmann/json 安装: ${NLOHMANN_INSTALL_DIR}"
    echo "Config file: ${NLOHMANN_CONFIG_FILE}"
    echo ""
    echo "Do you want to skip this installation and use the existing build? [Y/n]"
    echo "是否跳过此次安装并使用现有构建？[Y/n]"
    read -p "Choice (default: Y): " skip_choice

    # Default to Y if no input | 默认选择Y
    skip_choice=${skip_choice:-Y}

    if [[ "$skip_choice" =~ ^[Yy]$ ]] || [[ -z "$skip_choice" ]]; then
        echo "✅ Skipping nlohmann/json build - using existing installation"
        echo "✅ 跳过 nlohmann/json 构建 - 使用现有安装"
        echo "nlohmann/json installation location: ${NLOHMANN_INSTALL_DIR}"
        echo "nlohmann/json 安装位置: ${NLOHMANN_INSTALL_DIR}"
        exit 0
    else
        echo "🔄 Proceeding with fresh nlohmann/json build..."
        echo "🔄 继续进行新的 nlohmann/json 构建..."
    fi
fi

echo "Source directory: ${NLOHMANN_SRC_DIR}"
echo "Build directory: ${NLOHMANN_BUILD_DIR}"
echo "Install directory: ${NLOHMANN_INSTALL_DIR}"

# Clean existing build and install directories for fresh build
if [ -d "${NLOHMANN_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${NLOHMANN_BUILD_DIR}"
    rm -rf "${NLOHMANN_BUILD_DIR}"
fi

if [ -d "${NLOHMANN_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${NLOHMANN_INSTALL_DIR}"
    rm -rf "${NLOHMANN_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${NLOHMANN_BUILD_DIR}"
mkdir -p "${NLOHMANN_INSTALL_DIR}"

echo "Configuring nlohmann/json with CMake..."
cd "${NLOHMANN_BUILD_DIR}"

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

# Detect number of CPU cores (cross-platform)
if command -v nproc &> /dev/null; then
    NPROC=$(nproc)
elif command -v sysctl &> /dev/null; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=4
fi

# Configure CMake for nlohmann/json
# nlohmann/json is a header-only library, but we still need CMake config files
# -DCMAKE_INSTALL_PREFIX: Specifies where 'make install' will place the files.
# -DJSON_BuildTests=OFF: Disable tests to speed up build
# -DJSON_Install=ON: Enable installation of CMake config files
# -DCMAKE_BUILD_TYPE: Release build
cmake .. \
    -DCMAKE_INSTALL_PREFIX="${NLOHMANN_INSTALL_DIR}" \
    -DJSON_BuildTests=OFF \
    -DJSON_Install=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "Building nlohmann/json (using ${NPROC} processors)..."
echo "Note: nlohmann/json is header-only, this mainly generates CMake config files"
echo "注意：nlohmann/json 是纯头文件库，这里主要生成 CMake 配置文件"
make -j${NPROC}

echo "Installing nlohmann/json to ${NLOHMANN_INSTALL_DIR}..."
make install

# Verify installation
echo ""
echo "Verifying installation..."
if [ -f "${NLOHMANN_CONFIG_FILE}" ]; then
    echo "✅ CMake config file found: ${NLOHMANN_CONFIG_FILE}"
else
    echo "⚠️  Warning: CMake config file not found at expected location"
fi

# Check for headers
if [ -f "${NLOHMANN_INSTALL_DIR}/include/nlohmann/json.hpp" ]; then
    echo "✅ Header file found: ${NLOHMANN_INSTALL_DIR}/include/nlohmann/json.hpp"
    HEADER_SIZE=$(du -h "${NLOHMANN_INSTALL_DIR}/include/nlohmann/json.hpp" | cut -f1)
    echo "   Header size: ${HEADER_SIZE}"
else
    echo "⚠️  Warning: Header file not found at expected location"
fi

echo ""
echo "--- nlohmann/json local build and installation complete! ---"
echo "nlohmann/json has been installed in: ${NLOHMANN_INSTALL_DIR}"
echo "The necessary CMake configuration files are located at: ${NLOHMANN_INSTALL_DIR}/share/cmake/nlohmann_json/"
echo ""
echo "To use this nlohmann/json version in your main project (e.g., PoSDK):"
echo "When running CMake for your main project, you need to tell it where to find this installation."
echo "For example, if your main project's build directory is PoSDK/build, you might run:"
echo "cd PoSDK/build"
echo "cmake .. -DCMAKE_PREFIX_PATH=\"${NLOHMANN_INSTALL_DIR}\""
echo ""
echo "Or set nlohmann_json_DIR directly:"
echo "cmake .. -Dnlohmann_json_DIR=\"${NLOHMANN_INSTALL_DIR}/share/cmake/nlohmann_json\""
echo ""
echo "Alternatively, you can modify your main project's CMakeLists.txt to search in this path,"
echo "or set CMAKE_PREFIX_PATH as an environment variable."

