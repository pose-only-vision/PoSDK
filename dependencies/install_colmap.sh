#!/bin/bash

# 脚本用于在 Ubuntu 和 Mac 上从源码安装 COLMAP

set -e

# 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="ubuntu"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="mac"
else
    echo "不支持的操作系统: $OSTYPE"
    exit 1
fi

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

# Get script directory for absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COLMAP_DIR="${SCRIPT_DIR}/colmap-main"
COLMAP_INSTALL_DIR="${COLMAP_DIR}/install_local"
POSELIB_INSTALL_DIR="${SCRIPT_DIR}/PoseLib/install_local"
CERES_INSTALL_DIR="${SCRIPT_DIR}/ceres-solver-2.2.0/install_local"
BOOST_VERSION="1.85.0"
BOOST_VERSION_UNDERSCORE="1_85_0"
LOCAL_BOOST_DIR="${SCRIPT_DIR}/boost_${BOOST_VERSION_UNDERSCORE}"
LOCAL_BOOST_INSTALL="${LOCAL_BOOST_DIR}/install_local"

# Check for local Ceres installation | 检查本地Ceres安装
if [ ! -d "${CERES_INSTALL_DIR}" ] || [ ! -f "${CERES_INSTALL_DIR}/lib/libceres.a" ]; then
    print_warning "本地Ceres Solver安装未找到: ${CERES_INSTALL_DIR}"
    print_warning "Local Ceres Solver installation not found: ${CERES_INSTALL_DIR}"
    print_info "将先安装Ceres Solver..."
    print_info "Installing Ceres Solver first..."

    # Check if install_ceres.sh exists
    if [ -f "${SCRIPT_DIR}/install_ceres.sh" ]; then
        print_info "运行 install_ceres.sh..."
        print_info "Running install_ceres.sh..."
        chmod +x "${SCRIPT_DIR}/install_ceres.sh"
        "${SCRIPT_DIR}/install_ceres.sh"

        if [ $? -ne 0 ]; then
            print_error "Ceres Solver安装失败"
            print_error "Ceres Solver installation failed"
            exit 1
        fi
    else
        print_error "install_ceres.sh未找到"
        print_error "install_ceres.sh not found"
        print_error "请先运行: ./install_ceres.sh"
        print_error "Please run: ./install_ceres.sh first"
        exit 1
    fi
else
    print_info "找到本地Ceres Solver安装: ${CERES_INSTALL_DIR}"
    print_info "Found local Ceres Solver installation: ${CERES_INSTALL_DIR}"
fi

# Check for local PoseLib installation | 检查本地PoseLib安装
if [ ! -d "${POSELIB_INSTALL_DIR}" ] || [ ! -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a" ]; then
    print_warning "本地PoseLib安装未找到: ${POSELIB_INSTALL_DIR}"
    print_warning "Local PoseLib installation not found: ${POSELIB_INSTALL_DIR}"
    print_info "将先安装PoseLib..."
    print_info "Installing PoseLib first..."

    # Check if install_poselib.sh exists
    if [ -f "${SCRIPT_DIR}/install_poselib.sh" ]; then
        print_info "运行 install_poselib.sh..."
        print_info "Running install_poselib.sh..."
        chmod +x "${SCRIPT_DIR}/install_poselib.sh"
        "${SCRIPT_DIR}/install_poselib.sh"

        if [ $? -ne 0 ]; then
            print_error "PoseLib安装失败"
            print_error "PoseLib installation failed"
            exit 1
        fi
    else
        print_error "install_poselib.sh未找到"
        print_error "install_poselib.sh not found"
        print_error "请先运行: ./install_poselib.sh"
        print_error "Please run: ./install_poselib.sh first"
        exit 1
    fi
else
    print_info "找到本地PoseLib安装: ${POSELIB_INSTALL_DIR}"
    print_info "Found local PoseLib installation: ${POSELIB_INSTALL_DIR}"
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

# Check for existing installation and ask user | 检查现有安装并询问用户
if [ -d "${COLMAP_INSTALL_DIR}" ] && [ -d "${COLMAP_INSTALL_DIR}/bin" ] && [ -f "${COLMAP_INSTALL_DIR}/bin/colmap" ]; then
    print_warning "Found existing COLMAP installation at: ${COLMAP_INSTALL_DIR}"
    print_warning "发现现有COLMAP安装：${COLMAP_INSTALL_DIR}"

    # Show version if possible
    if [ -x "${COLMAP_INSTALL_DIR}/bin/colmap" ]; then
        echo "COLMAP version:"
        "${COLMAP_INSTALL_DIR}/bin/colmap" --version 2>/dev/null || echo "  - COLMAP binary is executable"
    fi

    read -p "Skip rebuild and use existing COLMAP installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing COLMAP installation"
        print_info "使用现有COLMAP安装"
        print_success "COLMAP is already available at: ${COLMAP_INSTALL_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

# Check if colmap-main directory exists
if [ ! -d "${COLMAP_DIR}" ]; then
    print_error "未找到 colmap-main 目录。请从 GitHub 下载并解压 colmap-main.zip。"
    print_error "colmap-main directory not found. Please download and extract colmap-main.zip from GitHub."
    exit 1
fi

# Apply Boost compatibility fix | 应用 Boost 兼容性修复
print_info "应用 Boost 兼容性修复..."
print_info "Applying Boost compatibility fix..."

cd "${COLMAP_DIR}"
if [ -f "cmake/FindDependencies.cmake" ]; then
    # Fix 1: CMake 3.30 Policy CMP0167
    if grep -q "cmake_policy(SET CMP0167 NEW)" cmake/FindDependencies.cmake 2>/dev/null; then
        print_info "修复 CMake 3.30 Policy CMP0167..."
        print_info "Fixing CMake 3.30 Policy CMP0167..."
        sed -i.bak 's/cmake_policy(SET CMP0167 NEW)/cmake_policy(SET CMP0167 OLD)/' cmake/FindDependencies.cmake
        rm -f cmake/FindDependencies.cmake.bak
        print_success "CMP0167 已设置为 OLD"
        print_success "CMP0167 set to OLD"
    fi
    
    # Fix 2: CMake Policy CMP0144 for CGAL compatibility
    if ! grep -q "cmake_policy.*CMP0144" cmake/FindDependencies.cmake 2>/dev/null; then
        print_info "修复 CMake Policy CMP0144（BOOST_ROOT 变量支持）..."
        print_info "Fixing CMake Policy CMP0144 (BOOST_ROOT variable support)..."
        # Create backup if not already created
        if [ ! -f "cmake/FindDependencies.cmake.backup" ]; then
            cp cmake/FindDependencies.cmake cmake/FindDependencies.cmake.backup
        fi
        sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0144 OLD)
' cmake/FindDependencies.cmake
        rm -f cmake/FindDependencies.cmake.bak
        print_success "CMP0144 已设置为 OLD"
        print_success "CMP0144 set to OLD"
    fi
    
    # Fix 3: CMake Policy CMP0074 for Boost_ROOT variable support
    if ! grep -q "cmake_policy.*CMP0074" cmake/FindDependencies.cmake 2>/dev/null; then
        print_info "修复 CMake Policy CMP0074（Boost_ROOT 变量支持）..."
        print_info "Fixing CMake Policy CMP0074 (Boost_ROOT variable support)..."
        # Create backup if not already created
        if [ ! -f "cmake/FindDependencies.cmake.backup" ]; then
            cp cmake/FindDependencies.cmake cmake/FindDependencies.cmake.backup
        fi
        sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0074 NEW)
' cmake/FindDependencies.cmake
        rm -f cmake/FindDependencies.cmake.bak
        print_success "CMP0074 已设置为 NEW"
        print_success "CMP0074 set to NEW"
    fi
    
    # Fix 4: Boost compatibility
    if ! grep -q "Fix Boost compatibility for newer versions" cmake/FindDependencies.cmake 2>/dev/null; then
        print_info "修复 Boost 兼容性..."
        print_info "Fixing Boost compatibility..."
        # Create backup | 创建备份
        cp cmake/FindDependencies.cmake cmake/FindDependencies.cmake.backup
        
        # Replace the problematic boost find_package call
        # For Ubuntu 18.04 (Boost 1.65), system component is required
        # For newer Boost (1.69+), system is header-only but still needs to be found
        # For Boost 1.89.0+, system component is removed entirely
        sed -i.tmp '/find_package(Boost.*COMPONENTS/,/system)/c\
# Fix Boost compatibility for different versions\
# Boost < 1.69: system component required (not header-only)\
# Boost 1.69-1.88: system is header-only but component still exists\
# Boost 1.89.0+: system component removed\
find_package(Boost QUIET COMPONENTS\
             graph\
             program_options\
             system)\
if(NOT Boost_FOUND)\
    message(STATUS "Boost system component not found, trying without it (likely Boost 1.89.0+)")\
    find_package(Boost ${COLMAP_FIND_TYPE} COMPONENTS\
                 graph\
                 program_options)\
else()\
    # Ensure boost_system is linked for versions that need it\
    if(TARGET Boost::system)\
        message(STATUS "Boost::system target available, will be linked")\
    endif()\
endif()' cmake/FindDependencies.cmake
        
        print_success "Boost 兼容性修复完成"
        print_success "Boost compatibility fix completed"
    else
        print_info "Boost 兼容性已经修复，跳过"
        print_info "Boost compatibility already fixed, skipping"
    fi
else
    print_error "无法找到 cmake/FindDependencies.cmake 文件"
    print_error "Cannot find cmake/FindDependencies.cmake file"
fi

# 安装依赖并编译
if [ "$OS" == "ubuntu" ]; then
    echo "正在 Ubuntu 上安装..."

    # 更新软件包列表
    smart_sudo apt-get update

    # 检测 Ubuntu 版本
    UBUNTU_VERSION=""
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        UBUNTU_VERSION="${VERSION_ID}"
    fi

    print_info "检测到 Ubuntu 版本: ${UBUNTU_VERSION}"

    # 基础依赖（所有版本都需要）
    DEPS_COMMON="git cmake ninja-build build-essential \
        libboost-program-options-dev \
        libboost-graph-dev \
        libboost-system-dev \
        libeigen3-dev \
        libfreeimage-dev \
        libmetis-dev \
        libgoogle-glog-dev \
        libgtest-dev \
        libsqlite3-dev \
        libglew-dev \
        qtbase5-dev \
        libqt5opengl5-dev \
        libcgal-dev \
        libceres-dev \
        libcurl4-openssl-dev"
    
    # Ubuntu 18.04 需要额外的 Qt5 私有头文件来支持 OpenGL 功能
    if [[ "${UBUNTU_VERSION}" == "18.04" ]]; then
        print_info "Ubuntu 18.04: 添加 Qt5 私有头文件和 OpenGL 支持"
        print_info "Ubuntu 18.04: Adding Qt5 private headers and OpenGL support"
        DEPS_COMMON="${DEPS_COMMON} libgl1-mesa-dev libglu1-mesa-dev qtbase5-private-dev"
    fi

    # Ubuntu 版本特定的依赖
    if [[ "${UBUNTU_VERSION}" == "18.04" ]]; then
        # Ubuntu 18.04 特定的包
        print_info "使用 Ubuntu 18.04 兼容的包"
        DEPS_OPENBLAS="libopenblas-dev"  # 18.04 使用 libopenblas-dev
        # libgmock-dev 在 18.04 中包含在 libgtest-dev 中，不需要单独安装
    else
        # Ubuntu 20.04+ 的包
        print_info "使用 Ubuntu 20.04+ 的包"
        DEPS_OPENBLAS="libopenblas-openmp-dev"
        DEPS_COMMON="${DEPS_COMMON} libgmock-dev"
    fi

    # 安装所有依赖
    smart_sudo apt-get install -y ${DEPS_COMMON} ${DEPS_OPENBLAS}

    # 可选：安装 CUDA
    # sudo apt-get install -y nvidia-cuda-toolkit nvidia-cuda-toolkit-gcc

    cd "${COLMAP_DIR}"
    # 先删除 build_local 文件夹以避免路径冲突
    if [ -d "build_local" ]; then
        rm -rf build_local
    fi
    # 创建本地安装目录
    mkdir -p install_local
    mkdir -p build_local
    cd build_local

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

    print_info "Using Ceres path: ${CERES_INSTALL_DIR}"
    print_info "使用Ceres路径: ${CERES_INSTALL_DIR}"
    print_info "Using PoseLib path: ${POSELIB_INSTALL_DIR}"
    print_info "使用PoseLib路径: ${POSELIB_INSTALL_DIR}"
    
    # Boost configuration | Boost配置
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        print_info "使用本地Boost路径 | Using local Boost path: ${LOCAL_BOOST_INSTALL}"
        # Set RPATH for runtime library finding with local Boost
        # 设置RPATH以便用本地Boost库进行运行时查找
        INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
    else
        print_info "使用系统Boost库 | Using system Boost libraries"
        # Set RPATH for COLMAP's own lib directory only
        # 仅为COLMAP自己的lib目录设置RPATH
        INSTALL_RPATH="\$ORIGIN/../lib"
    fi

    # Print configuration info | 打印配置信息
    print_info "配置摘要 | Configuration Summary:"
    print_info "  Ceres: ${CERES_INSTALL_DIR}"
    print_info "  PoseLib: ${POSELIB_INSTALL_DIR}"
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        print_info "  Boost: ${LOCAL_BOOST_INSTALL} (local)"
    else
        print_info "  Boost: (system)"
    fi
    echo ""

    # Ubuntu 18.04: Disable GUI due to Qt5 5.9.5 OpenGL compatibility issues | Ubuntu 18.04: 由于Qt5 5.9.5的OpenGL兼容性问题，禁用GUI
    GUI_OPTION=""
    if [[ "${UBUNTU_VERSION}" == "18.04" ]]; then
        print_warning "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        print_warning "Ubuntu 18.04: 禁用 GUI 支持 | Ubuntu 18.04: Disabling GUI support"
        print_warning "原因 | Reason: Qt5 5.9.5 对 OpenGL 3.2+ 支持不完整 | has incomplete OpenGL 3.2+ support"
        print_warning "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        GUI_OPTION="-DGUI_ENABLED=OFF"
    fi

    # Build CMAKE_PREFIX_PATH (exclude Anaconda, only include needed paths)
    # 构建CMAKE_PREFIX_PATH（排除Anaconda，只包含所需路径）
    CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR}"
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        CMAKE_PREFIX_PATH_VALUE="${LOCAL_BOOST_INSTALL};${CMAKE_PREFIX_PATH_VALUE}"
    fi

    # Prepare CMake options | 准备CMake选项
    CMAKE_OPTS=(
        "-GNinja"
        "-DCMAKE_INSTALL_PREFIX=../install_local"
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
        "-DTESTS_ENABLED=OFF"
        "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
        "-DCMAKE_INSTALL_RPATH=${INSTALL_RPATH}"
        "-DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE"
        "-DCMAKE_SKIP_BUILD_RPATH=FALSE"
    )

    # Add local Boost configuration if available | 如果有本地Boost则添加配置
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        # Use both BOOST_ROOT (old) and Boost_ROOT (new CMP0144) for compatibility
        # 同时使用 BOOST_ROOT (旧) 和 Boost_ROOT (新CMP0144) 以兼容
        # Note: CGAL uses old policy, so BOOST_ROOT is still needed
        # 注意：CGAL使用旧策略，所以仍然需要BOOST_ROOT
        CMAKE_OPTS+=("-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_ROOT=${LOCAL_BOOST_INSTALL}")
        CMAKE_OPTS+=("-DBoost_NO_SYSTEM_PATHS=ON")
        CMAKE_OPTS+=("-DBoost_NO_BOOST_CMAKE=ON")
        # Explicitly set Boost paths | 显式设置Boost路径
        CMAKE_OPTS+=("-DBOOST_INCLUDEDIR=${LOCAL_BOOST_INSTALL}/include")
        CMAKE_OPTS+=("-DBOOST_LIBRARYDIR=${LOCAL_BOOST_INSTALL}/lib")
        print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
        print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
    fi
    
    # Add GUI option if set | 如果设置了GUI选项则添加
    if [ -n "$GUI_OPTION" ]; then
        CMAKE_OPTS+=("$GUI_OPTION")
    fi
    
    # Configure CMake | 配置CMake
    cmake .. "${CMAKE_OPTS[@]}"
    
    ninja
    ninja install

elif [ "$OS" == "mac" ]; then
    echo "正在 Mac 上安装..."

    # 检查 Homebrew 是否已安装
    if ! command -v brew &> /dev/null; then
        echo "未找到 Homebrew。正在安装 Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    # 通过 Homebrew 安装依赖
    brew install \
        cmake \
        ninja \
        boost \
        eigen \
        freeimage \
        curl \
        libomp \
        metis \
        glog \
        googletest \
        ceres-solver \
        qt@5 \
        glew \
        cgal \
        sqlite3
    brew link --force libomp

    cd "${COLMAP_DIR}"
    # 先删除 build_local 文件夹以避免路径冲突
    if [ -d "build_local" ]; then
        rm -rf build_local
    fi
    # 创建本地安装目录
    mkdir -p install_local
    mkdir -p build_local
    cd build_local

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

    # Configure Qt path for macOS | 为macOS配置Qt路径
    QT_PREFIX_PATH=""
    if [[ "$(uname -m)" == "arm64" ]]; then
        # Apple Silicon (M1/M2/...)
        QT_PREFIX_PATH="/opt/homebrew/opt/qt@5"
    else
        # Intel Mac
        QT_PREFIX_PATH="/usr/local/opt/qt@5"
    fi

    print_info "Using Qt path: ${QT_PREFIX_PATH}"
    print_info "使用Qt路径: ${QT_PREFIX_PATH}"
    print_info "Using Ceres path: ${CERES_INSTALL_DIR}"
    print_info "使用Ceres路径: ${CERES_INSTALL_DIR}"
    print_info "Using PoseLib path: ${POSELIB_INSTALL_DIR}"
    print_info "使用PoseLib路径: ${POSELIB_INSTALL_DIR}"
    
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        print_info "Using local Boost path: ${LOCAL_BOOST_INSTALL}"
        print_info "使用本地Boost路径: ${LOCAL_BOOST_INSTALL}"
    else
        print_info "Using system Boost libraries"
        print_info "使用系统Boost库"
    fi

    # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
    # COLMAP needs to find its own libraries and Boost libraries
    # COLMAP 需要找到自己的库和 Boost 库
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        # Include both COLMAP's lib and Boost's lib in RPATH
        # RPATH 中包含 COLMAP 的 lib 和 Boost 的 lib
        INSTALL_RPATH="@loader_path/../lib:@loader_path/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
        print_info "设置 RPATH 包含本地 Boost 库"
        print_info "Setting RPATH to include local Boost libraries"
    else
        # Only include COLMAP's own lib directory
        # 仅包含 COLMAP 自己的 lib 目录
        INSTALL_RPATH="@loader_path/../lib"
    fi

    # Build CMAKE_PREFIX_PATH (exclude Anaconda, only include needed paths)
    # 构建CMAKE_PREFIX_PATH（排除Anaconda，只包含所需路径）
    CMAKE_PREFIX_PATH_VALUE="${QT_PREFIX_PATH};${CERES_INSTALL_DIR};${POSELIB_INSTALL_DIR}"
    if [ "$HAS_LOCAL_BOOST" = true ]; then
        CMAKE_PREFIX_PATH_VALUE="${LOCAL_BOOST_INSTALL};${CMAKE_PREFIX_PATH_VALUE}"
    fi

    # Prepare CMake options | 准备CMake选项
    CMAKE_OPTS=(
        "-GNinja"
        "-DCMAKE_INSTALL_PREFIX=../install_local"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
        "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
        "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_VALUE}"
        "-DQt5_DIR=${QT_PREFIX_PATH}/lib/cmake/Qt5"
        "-DQt5Core_DIR=${QT_PREFIX_PATH}/lib/cmake/Qt5Core"
        "-DQt5Widgets_DIR=${QT_PREFIX_PATH}/lib/cmake/Qt5Widgets"
        "-DQt5OpenGL_DIR=${QT_PREFIX_PATH}/lib/cmake/Qt5OpenGL"
        "-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres"
        "-DPoseLib_DIR=${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
        "-DFETCH_POSELIB=OFF"
        "-DTESTS_ENABLED=OFF"
        "-DCMAKE_MACOSX_RPATH=ON"
        "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
        "-DCMAKE_INSTALL_RPATH=${INSTALL_RPATH}"
        "-DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE"
        "-DCMAKE_SKIP_BUILD_RPATH=FALSE"
    )
    
    # Add local Boost if available | 如果有本地Boost则添加
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
    
    # Configure CMake | 配置CMake
    cmake .. "${CMAKE_OPTS[@]}"
    
    ninja
    ninja install
fi

echo "COLMAP 安装完成。"
echo "本地安装路径：${COLMAP_INSTALL_DIR}"
echo "可以通过以下方式验证安装："
echo "  ${COLMAP_INSTALL_DIR}/bin/colmap -h"
echo "GraphOptim 现在可以找到这个本地 COLMAP 安装。"
