#!/bin/bash
# Script to build and install GraphOptim from a local source directory.

set -e # Exit immediately if a command exits with a non-zero status.

# --- USER-PROVIDED LOCAL GraphOptim SOURCE ---
# Define source directory relative to the script location
# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
USER_GRAPHOPTIM_SRC_DIR="${SCRIPT_DIR}/GraphOptim" # Local dependencies path
POSELIB_INSTALL_DIR="${SCRIPT_DIR}/PoseLib/install_local" # Local PoseLib installation
BOOST_VERSION="1.85.0"
BOOST_VERSION_UNDERSCORE="1_85_0"
LOCAL_BOOST_DIR="${SCRIPT_DIR}/boost_${BOOST_VERSION_UNDERSCORE}"
LOCAL_BOOST_INSTALL="${LOCAL_BOOST_DIR}/install_local"
# --- END USER-PROVIDED LOCAL GraphOptim SOURCE ---

if [ -z "${USER_GRAPHOPTIM_SRC_DIR}" ]; then
    echo "Error: USER_GRAPHOPTIM_SRC_DIR is not set in the script."
    echo "Please edit this script and set USER_GRAPHOPTIM_SRC_DIR to the root directory of your GraphOptim source code."
    echo "You can clone GraphOptim from: https://github.com/AIBluefisher/GraphOptim.git"
    exit 1
fi

if [ ! -d "${USER_GRAPHOPTIM_SRC_DIR}" ] || [ ! -f "${USER_GRAPHOPTIM_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: USER_GRAPHOPTIM_SRC_DIR (${USER_GRAPHOPTIM_SRC_DIR}) is not a valid GraphOptim source directory (CMakeLists.txt not found)."
    echo "Please ensure the path is correct and it's the root of the GraphOptim repository."
    exit 1
fi

GRAPHOPTIM_BUILD_DIR="${USER_GRAPHOPTIM_SRC_DIR}/build_scripted" # Use a distinct build directory

# Determine OS and Installation Prefix
INSTALL_PREFIX=""
SYSTEM_INSTALL_DIR=""
NPROC_CMD=""
CMAKE_OSX_ARCHITECTURES_FLAG=""
OPENMP_FLAGS=""
LIBOMP_DIR=""

if [[ "$(uname)" == "Darwin" ]]; then
    if [[ "$(uname -m)" == "arm64" ]]; then
        SYSTEM_INSTALL_DIR="/opt/homebrew"
        CMAKE_OSX_ARCHITECTURES_FLAG="-DCMAKE_OSX_ARCHITECTURES=arm64"
        LIBOMP_DIR="/opt/homebrew/opt/libomp" # Common Homebrew path for libomp on Apple Silicon
    else
        SYSTEM_INSTALL_DIR="/usr/local"
        CMAKE_OSX_ARCHITECTURES_FLAG="-DCMAKE_OSX_ARCHITECTURES=x86_64"
        LIBOMP_DIR="/usr/local/opt/libomp" # Common Homebrew path for libomp on Intel Macs
    fi
    NPROC_CMD="sysctl -n hw.ncpu"
    if [ -d "${LIBOMP_DIR}" ]; then
      OPENMP_FLAGS="-DOpenMP_ROOT='${LIBOMP_DIR}' -DOPENMP_FOUND=TRUE -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 -DCMAKE_C_FLAGS='-Xpreprocessor -fopenmp -I${LIBOMP_DIR}/include' -DCMAKE_CXX_FLAGS='-Xpreprocessor -fopenmp -I${LIBOMP_DIR}/include' -DCMAKE_EXE_LINKER_FLAGS='-L${LIBOMP_DIR}/lib -lomp' -DCMAKE_SHARED_LINKER_FLAGS='-L${LIBOMP_DIR}/lib -lomp' -DCMAKE_MODULE_LINKER_FLAGS='-L${LIBOMP_DIR}/lib -lomp'"
    else
      echo "Warning: libomp directory not found at ${LIBOMP_DIR}. OpenMP might not be enabled correctly."
    fi
elif [[ "$(uname)" == "Linux" ]]; then
    SYSTEM_INSTALL_DIR="/usr/local"
    NPROC_CMD="nproc"
    # For Linux, FindOpenMP usually works better out of the box if libomp-dev is installed
    OPENMP_FLAGS="-DOPENMP_FOUND=TRUE"
else
    echo "Unsupported OS: $(uname)"
    exit 1
fi

if [[ -w "$SYSTEM_INSTALL_DIR" ]]; then
    INSTALL_PREFIX="$SYSTEM_INSTALL_DIR"
else
    USER_LOCAL_INSTALL_DIR="$HOME/.local"
    echo "No write permission to system directory (${SYSTEM_INSTALL_DIR})."
    echo "Attempting to install to user-local directory: ${USER_LOCAL_INSTALL_DIR}"
    INSTALL_PREFIX="$USER_LOCAL_INSTALL_DIR"
    mkdir -p "$INSTALL_PREFIX/bin" "$INSTALL_PREFIX/lib" "$INSTALL_PREFIX/include"
    if [[ ":$PATH:" != *":${INSTALL_PREFIX}/bin:"* ]]; then
        echo "Please add ${INSTALL_PREFIX}/bin to your PATH."
        echo "You can do this by adding the following line to your shell configuration file (e.g., ~/.bashrc, ~/.zshrc):"
        echo "export PATH=\"${INSTALL_PREFIX}/bin:\$PATH\""
        echo "And for libraries (Linux):"
        echo "export LD_LIBRARY_PATH=\"${INSTALL_PREFIX}/lib:\$LD_LIBRARY_PATH\""
        echo "And for CMake to find the package:"
        echo "export CMAKE_PREFIX_PATH=\"${INSTALL_PREFIX}:\$CMAKE_PREFIX_PATH\""
    fi
fi

echo "--- GraphOptim Installation Script (Using Local Source) ---"
echo "Using local GraphOptim source from: ${USER_GRAPHOPTIM_SRC_DIR}"
echo "Build directory (will be created if it doesn't exist): ${GRAPHOPTIM_BUILD_DIR}"
echo "Installation prefix: ${INSTALL_PREFIX}"
echo "Requires: cmake, Eigen3, Ceres Solver, and a C++ compiler (with OpenMP support)."
if [[ "$(uname)" == "Darwin" ]]; then
    echo "On macOS, ensure libomp is installed (e.g., via 'brew install libomp'). This script attempts to configure it."
fi
echo "Ensure Eigen3 and Ceres Solver are installed and findable by CMake."
echo "You can try GraphOptim's dependency script: ./scripts/dependencies.sh (run from within GraphOptim source if needed)."

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

# Check for local PoseLib installation | 检查本地PoseLib安装
# Check for any library file (.a, .so, .dylib) to ensure PoseLib is installed
# 检查任意库文件（.a, .so, .dylib）以确保PoseLib已安装
POSELIB_LIB_FOUND=false
if [ -d "${POSELIB_INSTALL_DIR}/lib" ]; then
    # Check for static library (.a)
    if [ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a" ]; then
        POSELIB_LIB_FOUND=true
        print_info "找到PoseLib静态库: libPoseLib.a"
        print_info "Found PoseLib static library: libPoseLib.a"
    fi
    # Check for shared library on Linux (.so)
    if [ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.so" ]; then
        POSELIB_LIB_FOUND=true
        print_info "找到PoseLib动态库: libPoseLib.so"
        print_info "Found PoseLib shared library: libPoseLib.so"
    fi
    # Check for shared library on macOS (.dylib)
    if ls "${POSELIB_INSTALL_DIR}/lib"/libPoseLib*.dylib 1> /dev/null 2>&1; then
        POSELIB_LIB_FOUND=true
        print_info "找到PoseLib动态库: libPoseLib.dylib"
        print_info "Found PoseLib shared library: libPoseLib.dylib"
    fi
fi

if [ "$POSELIB_LIB_FOUND" = false ]; then
    print_error "本地PoseLib安装未找到: ${POSELIB_INSTALL_DIR}"
    print_error "Local PoseLib installation not found: ${POSELIB_INSTALL_DIR}"
    print_error "未找到 libPoseLib.a、libPoseLib.so 或 libPoseLib.dylib"
    print_error "Could not find libPoseLib.a, libPoseLib.so, or libPoseLib.dylib"
    print_error "请先运行 ./install_poselib.sh 安装PoseLib"
    print_error "Please run ./install_poselib.sh to install PoseLib first"
    exit 1
else
    print_success "✓ 找到本地PoseLib安装: ${POSELIB_INSTALL_DIR}"
    print_success "✓ Found local PoseLib installation: ${POSELIB_INSTALL_DIR}"
fi

# Check for existing installation and ask user | 检查现有安装并询问用户
if [ -d "${GRAPHOPTIM_BUILD_DIR}" ] && [ "$(find "${GRAPHOPTIM_BUILD_DIR}" -name "*.so" -o -name "*.dylib" -o -name "*.a" -o -name "CMakeCache.txt" -o -name "rotation_estimator" 2>/dev/null | head -1)" ]; then
    print_warning "Found existing GraphOptim build at: ${GRAPHOPTIM_BUILD_DIR}"
    print_warning "发现现有GraphOptim构建：${GRAPHOPTIM_BUILD_DIR}"

    # List some key files to show what's built
    echo "Existing build contains:"
    find "${GRAPHOPTIM_BUILD_DIR}" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.a" -o -name "CMakeCache.txt" -o -name "rotation_estimator" \) 2>/dev/null | head -3 | while read file; do
        echo "  - $(basename "$file")"
    done

    read -p "Skip rebuild and use existing GraphOptim build? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing GraphOptim build"
        print_info "使用现有GraphOptim构建"
        print_success "GraphOptim is already available at: ${GRAPHOPTIM_BUILD_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

echo "------------------------------------"
read -p "Press Enter to continue or Ctrl+C to abort..."

# ====================================================
# 清理旧的构建目录 - 确保干净的构建环境
# ====================================================
echo "正在检查并清理旧的构建目录..."

# 清理主构建目录
if [ -d "${GRAPHOPTIM_BUILD_DIR}" ]; then
    echo "发现旧的构建目录: ${GRAPHOPTIM_BUILD_DIR}"
    echo "为确保干净构建，正在删除旧构建目录..."
    rm -rf "${GRAPHOPTIM_BUILD_DIR}"
    echo "✓ 旧构建目录清理完成"
else
    echo "✓ 未发现旧构建目录，可以进行全新构建"
fi

# 清理可能的其他构建目录 (例如用户之前可能创建的)
OTHER_BUILD_DIRS=("${USER_GRAPHOPTIM_SRC_DIR}/build" "${USER_GRAPHOPTIM_SRC_DIR}/build_release" "${USER_GRAPHOPTIM_SRC_DIR}/build_debug" "${USER_GRAPHOPTIM_SRC_DIR}/cmake-build-release" "${USER_GRAPHOPTIM_SRC_DIR}/cmake-build-debug")
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

echo "Creating build directory: ${GRAPHOPTIM_BUILD_DIR}..."
mkdir -p "${GRAPHOPTIM_BUILD_DIR}"
cd "${GRAPHOPTIM_BUILD_DIR}"

echo "Configuring GraphOptim with CMake (Source: ${USER_GRAPHOPTIM_SRC_DIR}, Installing to ${INSTALL_PREFIX})..."

# Create temporary CMake module directory to override system modules | 创建临时CMake模块目录覆盖系统模块
TEMP_CMAKE_MODULES_DIR="${GRAPHOPTIM_BUILD_DIR}/cmake_modules_override"
mkdir -p "${TEMP_CMAKE_MODULES_DIR}"

# Set up internal COLMAP paths for GraphOptim | 为GraphOptim设置内部COLMAP路径
GRAPHOPTIM_INTERNAL_COLMAP_DIR="${USER_GRAPHOPTIM_SRC_DIR}/3rd_party/colmap-main"
INTERNAL_COLMAP_BUILD_DIR="${GRAPHOPTIM_INTERNAL_COLMAP_DIR}/build_for_graphoptim"
INTERNAL_COLMAP_INSTALL_DIR="${GRAPHOPTIM_INTERNAL_COLMAP_DIR}/install_for_graphoptim"

# Set up local Ceres path | 设置本地Ceres路径
CERES_INSTALL_DIR="${SCRIPT_DIR}/ceres-solver-2.2.0/install_local"

# Ask user if they want to build COLMAP (optional dependency) | 询问用户是否需要构建COLMAP（可选依赖）
# Default is to NOT build COLMAP in all cases | 默认在所有情况下都不构建COLMAP
BUILD_COLMAP=false

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
print_info "COLMAP 是 GraphOptim 的可选依赖"
print_info "COLMAP is an optional dependency for GraphOptim"
print_info "核心功能（rotation_estimator 等）不需要 COLMAP"
print_info "Core features (rotation_estimator, etc.) do not require COLMAP"
print_info "默认不构建 COLMAP（推荐）"
print_info "Default: NOT building COLMAP (recommended)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if running in interactive mode | 检查是否在交互式模式运行
if [ -t 0 ]; then
    # Interactive mode: ask user | 交互式模式：询问用户
    read -p "跳过构建内部 COLMAP 支持（仅构建核心功能）？Skip building internal COLMAP support (core features only)? [Y/n]: " -n 1 -r
    echo ""
    
    # 默认值为 Y（跳过COLMAP构建），即默认仅构建核心功能
    # Default is Y (skip COLMAP), i.e., build core features only by default
    if [[ -z "$REPLY" ]] || [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "✅ 跳过 COLMAP 构建（仅构建核心功能，默认）"
        print_info "✅ Skipping COLMAP build (core features only, default)"
    else
        BUILD_COLMAP=true
        print_info "🔄 将构建内部 COLMAP 支持"
        print_info "🔄 Will build internal COLMAP support"
        
        if [ ! -d "${GRAPHOPTIM_INTERNAL_COLMAP_DIR}" ]; then
            print_error "GraphOptim internal COLMAP source not found at: ${GRAPHOPTIM_INTERNAL_COLMAP_DIR}"
            print_error "错误：未找到GraphOptim内部COLMAP源码：${GRAPHOPTIM_INTERNAL_COLMAP_DIR}"
            print_error "Please ensure the internal COLMAP source is available in GraphOptim/3rd_party/"
            print_error "请确保内部COLMAP源码在GraphOptim/3rd_party/目录中可用"
            exit 1
        fi
    fi
else
    # Non-interactive mode (automated): default to NOT building COLMAP | 非交互式模式（自动化）：默认不构建COLMAP
    print_info "非交互式模式：默认跳过 COLMAP 构建（仅构建核心功能）"
    print_info "Non-interactive mode: Skipping COLMAP build by default (core features only)"
    BUILD_COLMAP=false
fi
echo ""

# Only build COLMAP if user chose to | 仅在用户选择时构建COLMAP
if [ "$BUILD_COLMAP" = true ]; then
    echo "Building GraphOptim internal COLMAP (GUI disabled for compatibility)..."
    echo "构建GraphOptim内部COLMAP（禁用GUI以保证兼容性）..."

    # Clean and create build directory | 清理并创建构建目录
    if [ -d "${INTERNAL_COLMAP_BUILD_DIR}" ]; then
        echo "Cleaning existing internal COLMAP build..."
        echo "清理现有内部COLMAP构建..."
        rm -rf "${INTERNAL_COLMAP_BUILD_DIR}"
    fi
    if [ -d "${INTERNAL_COLMAP_INSTALL_DIR}" ]; then
        echo "Cleaning existing internal COLMAP installation..."
        echo "清理现有内部COLMAP安装..."
        rm -rf "${INTERNAL_COLMAP_INSTALL_DIR}"
    fi

    mkdir -p "${INTERNAL_COLMAP_BUILD_DIR}"
    
    # Apply Boost compatibility fixes to internal COLMAP | 为内部COLMAP应用Boost兼容性修复
    COLMAP_FIND_DEPS="${GRAPHOPTIM_INTERNAL_COLMAP_DIR}/cmake/FindDependencies.cmake"
    if [ -f "${COLMAP_FIND_DEPS}" ]; then
        print_info "为内部 COLMAP 应用 Boost 兼容性修复..."
        print_info "Applying Boost compatibility fixes to internal COLMAP..."
        BACKUP_CREATED=false
        
        # Fix CMP0167 (Boost Config mode) | 修复CMP0167（Boost配置模式）
        if grep -q "cmake_policy(SET CMP0167 NEW)" "${COLMAP_FIND_DEPS}"; then
            if [ "$BACKUP_CREATED" = false ]; then
                cp "${COLMAP_FIND_DEPS}" "${COLMAP_FIND_DEPS}.backup_auto"
                BACKUP_CREATED=true
            fi
            sed -i.bak 's/cmake_policy(SET CMP0167 NEW)/cmake_policy(SET CMP0167 OLD)/' "${COLMAP_FIND_DEPS}"
            print_info "应用 CMP0167 修复（Boost Config 模式）"
            print_info "Applying CMP0167 fix (Boost Config mode)"
        fi
        
        # Fix CMP0144 (BOOST_ROOT variable) for CGAL compatibility | 修复CMP0144（BOOST_ROOT变量）以兼容CGAL
        if ! grep -q "cmake_policy.*CMP0144" "${COLMAP_FIND_DEPS}"; then
            if [ "$BACKUP_CREATED" = false ]; then
                cp "${COLMAP_FIND_DEPS}" "${COLMAP_FIND_DEPS}.backup_auto"
                BACKUP_CREATED=true
            fi
            sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0144 OLD)
' "${COLMAP_FIND_DEPS}"
            print_info "应用 CMP0144 修复（BOOST_ROOT 变量支持）"
            print_info "Applying CMP0144 fix (BOOST_ROOT variable support)"
        fi
        
        # Fix CMP0074 (Boost_ROOT variable) for find_package | 修复CMP0074（Boost_ROOT变量）以支持find_package
        if ! grep -q "cmake_policy.*CMP0074" "${COLMAP_FIND_DEPS}"; then
            if [ "$BACKUP_CREATED" = false ]; then
                cp "${COLMAP_FIND_DEPS}" "${COLMAP_FIND_DEPS}.backup_auto"
                BACKUP_CREATED=true
            fi
            sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0074 NEW)
' "${COLMAP_FIND_DEPS}"
            print_info "应用 CMP0074 修复（Boost_ROOT 变量支持）"
            print_info "Applying CMP0074 fix (Boost_ROOT variable support)"
        fi
        
        if [ "$BACKUP_CREATED" = true ]; then
            rm -f "${COLMAP_FIND_DEPS}.bak"
            print_success "✓ 内部 COLMAP Boost 兼容性修复已应用（CMP0167=OLD, CMP0144=OLD, CMP0074=NEW）"
            print_success "✓ Internal COLMAP Boost compatibility fixes applied (CMP0167=OLD, CMP0144=OLD, CMP0074=NEW)"
        else
            print_info "✓ 内部 COLMAP Boost 兼容性已修复（跳过）"
            print_info "✓ Internal COLMAP Boost compatibility already fixed (skipped)"
        fi
    else
        print_warning "⚠ 未找到内部 COLMAP FindDependencies.cmake，跳过 Boost 修复"
        print_warning "⚠ Internal COLMAP FindDependencies.cmake not found, skipping Boost fix"
    fi
    
    cd "${INTERNAL_COLMAP_BUILD_DIR}"

    # Clean environment to avoid Anaconda pollution | 清理环境以避免Anaconda污染
    ORIGINAL_PATH="${PATH}"
    ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
    ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    
    if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
        print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
        CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        export PATH="${CLEAN_PATH}"
    fi
    
    if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
        print_info "检测到Anaconda在LD_LIBRARY_PATH中，临时移除"
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
            export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
        fi
    fi
    
    if [ -n "${CONDA_PREFIX}" ]; then
        print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
        unset CONDA_PREFIX
    fi

# Configure internal COLMAP with GUI disabled for GraphOptim | 为GraphOptim配置禁用GUI的内部COLMAP
echo "Configuring internal COLMAP (GUI disabled)..."
echo "配置内部COLMAP（禁用GUI）..."
print_info "配置内部COLMAP使用本地PoseLib: ${POSELIB_INSTALL_DIR}"
print_info "Configuring internal COLMAP to use local PoseLib: ${POSELIB_INSTALL_DIR}"
print_info "配置内部COLMAP使用本地Ceres: ${CERES_INSTALL_DIR}"
print_info "Configuring internal COLMAP to use local Ceres: ${CERES_INSTALL_DIR}"

# Check for local Ceres installation | 检查本地Ceres安装
if [ ! -d "${CERES_INSTALL_DIR}" ] || [ ! -f "${CERES_INSTALL_DIR}/lib/libceres.a" ]; then
    print_error "本地Ceres Solver安装未找到: ${CERES_INSTALL_DIR}"
    print_error "Local Ceres Solver installation not found: ${CERES_INSTALL_DIR}"
    print_error "请先运行 ./install_ceres.sh"
    print_error "Please run ./install_ceres.sh first"
    exit 1
fi

# Check for local Boost installation | 检查本地Boost安装
HAS_LOCAL_BOOST=false
if [ -d "${LOCAL_BOOST_INSTALL}" ] && [ -f "${LOCAL_BOOST_INSTALL}/lib/libboost_system.a" ]; then
    print_info "找到本地Boost ${BOOST_VERSION}安装: ${LOCAL_BOOST_INSTALL}"
    print_info "Found local Boost ${BOOST_VERSION} installation: ${LOCAL_BOOST_INSTALL}"
    HAS_LOCAL_BOOST=true
else
    print_warning "本地Boost ${BOOST_VERSION}安装未找到: ${LOCAL_BOOST_INSTALL}"
    print_warning "Local Boost ${BOOST_VERSION} installation not found: ${LOCAL_BOOST_INSTALL}"
    print_info "将先安装Boost..."
    print_info "Installing Boost first..."

    # Check if install_boost.sh exists
    if [ -f "${SCRIPT_DIR}/install_boost.sh" ]; then
        print_info "运行 install_boost.sh..."
        print_info "Running install_boost.sh..."
        chmod +x "${SCRIPT_DIR}/install_boost.sh"
        "${SCRIPT_DIR}/install_boost.sh"

        if [ $? -ne 0 ]; then
            print_error "Boost安装失败"
            print_error "Boost installation failed"
            exit 1
        fi
        HAS_LOCAL_BOOST=true
    else
        print_error "install_boost.sh未找到"
        print_error "install_boost.sh not found"
        print_warning "将使用系统Boost库"
        print_warning "Will use system Boost libraries"
    fi
fi

if [ "$HAS_LOCAL_BOOST" = true ]; then
    print_info "Using local Boost path: ${LOCAL_BOOST_INSTALL}"
    print_info "使用本地Boost路径: ${LOCAL_BOOST_INSTALL}"
else
    print_info "Using system Boost libraries"
    print_info "使用系统Boost库"
fi

if [[ "$(uname)" == "Darwin" ]]; then
    # macOS specific configuration | macOS特定配置
    
    # Build CMAKE_PREFIX_PATH | 构建CMAKE_PREFIX_PATH
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR};${LOCAL_BOOST_INSTALL}"
    else
        CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR}"
    fi
    
    CMAKE_OPTS=(
        "-DCMAKE_INSTALL_PREFIX=${INTERNAL_COLMAP_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
        "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
        "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
        "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
        "-DCMAKE_LIBRARY_PATH=${POSELIB_INSTALL_DIR}/lib"
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_VALUE}"
        "-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres"
        "-DPoseLib_DIR=${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
        "-DFETCH_POSELIB=OFF"
        "-DGUI_ENABLED=OFF"
        "-DTESTS_ENABLED=OFF"
        "-DCUDA_ENABLED=OFF"
        "-DOPENGL_ENABLED=OFF"
        "${CMAKE_OSX_ARCHITECTURES_FLAG}"
    )
    
    # Add local Boost configuration if available | 如果有本地Boost则添加配置
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        # Use both BOOST_ROOT (old) and Boost_ROOT (new CMP0144) for compatibility
        # 同时使用 BOOST_ROOT (旧) 和 Boost_ROOT (新CMP0144) 以兼容
        CMAKE_OPTS+=("-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_NO_SYSTEM_PATHS=ON")
        CMAKE_OPTS+=("-DBoost_NO_BOOST_CMAKE=ON")
        CMAKE_OPTS+=("-DBOOST_INCLUDEDIR=${LOCAL_BOOST_INSTALL}/include")
        CMAKE_OPTS+=("-DBOOST_LIBRARYDIR=${LOCAL_BOOST_INSTALL}/lib")
        print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
        print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
    fi
    
    cmake .. "${CMAKE_OPTS[@]}"
else
    # Linux configuration | Linux配置
    
    # Build CMAKE_PREFIX_PATH | 构建CMAKE_PREFIX_PATH
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR};${LOCAL_BOOST_INSTALL}"
    else
        CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR}"
    fi
    
    CMAKE_OPTS=(
        "-DCMAKE_INSTALL_PREFIX=${INTERNAL_COLMAP_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
        "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
        "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
        "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
        "-DCMAKE_LIBRARY_PATH=${POSELIB_INSTALL_DIR}/lib"
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_VALUE}"
        "-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres"
        "-DPoseLib_DIR=${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
        "-DFETCH_POSELIB=OFF"
        "-DGUI_ENABLED=OFF"
        "-DTESTS_ENABLED=OFF"
        "-DCUDA_ENABLED=OFF"
        "-DOPENGL_ENABLED=OFF"
    )
    
    # Add local Boost configuration if available | 如果有本地Boost则添加配置
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        # Use both BOOST_ROOT (old) and Boost_ROOT (new CMP0144) for compatibility
        # 同时使用 BOOST_ROOT (旧) 和 Boost_ROOT (新CMP0144) 以兼容
        CMAKE_OPTS+=("-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_NO_SYSTEM_PATHS=ON")
        CMAKE_OPTS+=("-DBoost_NO_BOOST_CMAKE=ON")
        CMAKE_OPTS+=("-DBOOST_INCLUDEDIR=${LOCAL_BOOST_INSTALL}/include")
        CMAKE_OPTS+=("-DBOOST_LIBRARYDIR=${LOCAL_BOOST_INSTALL}/lib")
        print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
        print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
    fi
    
    cmake .. "${CMAKE_OPTS[@]}"
fi

    # Build and install internal COLMAP | 构建并安装内部COLMAP
    echo "Building internal COLMAP (this may take a while)..."
    echo "构建内部COLMAP（可能需要一些时间）..."
    make -j$(${NPROC_CMD})
    make install

    echo "Internal COLMAP built successfully for GraphOptim"
    echo "内部COLMAP为GraphOptim构建成功"

    # Set up paths for GraphOptim to use internal COLMAP | 设置路径让GraphOptim使用内部COLMAP
    LOCAL_COLMAP_INSTALL_DIR="${INTERNAL_COLMAP_INSTALL_DIR}"
    COLMAP_CMAKE_FLAGS="-DCMAKE_POLICY_DEFAULT_CMP0074=NEW -DCOLMAP_ROOT=${LOCAL_COLMAP_INSTALL_DIR} -Dcolmap_DIR=${LOCAL_COLMAP_INSTALL_DIR}/share/colmap -DCMAKE_IGNORE_PATH='/usr/local/share/colmap;/usr/share/colmap;/opt/local/share/colmap' -DCMAKE_MODULE_PATH=${TEMP_CMAKE_MODULES_DIR}"

    # Set environment variables | 设置环境变量
    export COLMAP_ROOT="${LOCAL_COLMAP_INSTALL_DIR}"
    export colmap_DIR="${LOCAL_COLMAP_INSTALL_DIR}/share/colmap"

    # Print CMake configuration for debugging | 打印CMake配置用于调试
    echo "CMake COLMAP configuration | CMake COLMAP配置:"
    echo "  COLMAP_ROOT: ${COLMAP_ROOT:-'<not set>'}"
    echo "  colmap_DIR: ${colmap_DIR:-'<not set>'}"
    echo "  CMAKE flags: ${COLMAP_CMAKE_FLAGS}"
    echo "------------------------------------"
else
    # No COLMAP support | 不启用COLMAP支持
    # Explicitly disable COLMAP finding to avoid system COLMAP interference
    # 明确禁用COLMAP查找以避免系统COLMAP干扰
    COLMAP_CMAKE_FLAGS="-DCMAKE_DISABLE_FIND_PACKAGE_COLMAP=ON -DCMAKE_IGNORE_PATH='/usr/local/share/colmap;/usr/share/colmap;/opt/local/share/colmap;/usr/local/lib/cmake/COLMAP;/usr/lib/cmake/COLMAP'"
    print_info "构建不含 COLMAP 支持的 GraphOptim"
    print_info "Building GraphOptim without COLMAP support"
    print_info "已禁用 COLMAP 查找（避免系统版本干扰）"
    print_info "COLMAP finding disabled (avoiding system version interference)"
    
    # Unset COLMAP environment variables if they exist | 清除COLMAP环境变量（如果存在）
    unset COLMAP_ROOT 2>/dev/null || true
    unset colmap_DIR 2>/dev/null || true
fi

# Return to GraphOptim build directory | 返回GraphOptim构建目录
cd "${GRAPHOPTIM_BUILD_DIR}"

# Clean environment to avoid Anaconda pollution before GraphOptim CMake configuration
# 在GraphOptim CMake配置前清理环境以避免Anaconda污染
if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
    print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
    CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    export PATH="${CLEAN_PATH}"
fi

if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
    print_info "检测到Anaconda在LD_LIBRARY_PATH中，临时移除"
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
        export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
    fi
fi

if [ -n "${CONDA_PREFIX}" ]; then
    print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
    unset CONDA_PREFIX
fi

# Set environment variables to disable COLMAP GUI before CMake configuration | 在CMake配置前设置环境变量禁用COLMAP GUI
export GUI_ENABLED=OFF
export TESTS_ENABLED=OFF
export CUDA_ENABLED=OFF

# The OPENMP_FLAGS are unquoted to allow for shell expansion of the flags string
eval cmake "${USER_GRAPHOPTIM_SRC_DIR}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_MACOSX_RPATH=ON \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE \
    -DCMAKE_INSTALL_RPATH="@loader_path/../lib" \
    -DGUI_ENABLED=OFF \
    -DTESTS_ENABLED=OFF \
    -DCUDA_ENABLED=OFF \
    ${CMAKE_OSX_ARCHITECTURES_FLAG} \
    ${OPENMP_FLAGS} \
    ${COLMAP_CMAKE_FLAGS}
    # Add other GraphOptim specific CMake options if needed

echo "Building GraphOptim (this may take a while, using $(${NPROC_CMD}) cores)..."
make -j$(${NPROC_CMD})

# 不再需要安装到系统目录，直接使用构建生成的二进制文件
echo "构建完成，二进制文件生成在: ${GRAPHOPTIM_BUILD_DIR}/bin"
echo "注意：GraphOptim 现在使用构建目录中的二进制文件，无需安装到系统目录"

# Do not remove the user-provided source directory
echo "------------------------------------"
echo "GraphOptim 构建完成!"
echo "源代码目录: ${USER_GRAPHOPTIM_SRC_DIR}"
echo "构建目录: ${GRAPHOPTIM_BUILD_DIR}"
echo "二进制文件: ${GRAPHOPTIM_BUILD_DIR}/bin"
echo "库文件: ${GRAPHOPTIM_BUILD_DIR}/lib (如果有的话)"

if [ "$BUILD_COLMAP" = true ]; then
    echo "COLMAP 支持: ✅ 已启用"
else
    echo "COLMAP 支持: ⏭ 未构建（核心功能正常）"
fi

echo ""
echo "使用说明："
echo "- PoSDK 会自动检测并使用构建目录中的二进制文件"
echo "- 无需手动配置路径或安装到系统目录"
echo "- 如需手动测试，可直接运行: ${GRAPHOPTIM_BUILD_DIR}/bin/rotation_estimator"
if [ "$BUILD_COLMAP" = false ]; then
    echo "- 注意：当前构建不包含 COLMAP 扩展功能"
fi
echo "------------------------------------"

exit 0
