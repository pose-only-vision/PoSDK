#!/bin/bash
# ==============================================================================
# PoSDK Installation Script
# ==============================================================================
# Installation script for PoSDK dependencies on Ubuntu/Debian and macOS systems
# PoSDK依赖安装脚本，支持Ubuntu/Debian和macOS系统
#
# IMPORTANT | 重要说明:
# This script installs ALL dependencies required for PoSDK to work properly,
# including runtime dependencies for the embedded po_core library.
# 此脚本安装PoSDK正常工作所需的所有依赖，包括嵌入的po_core库的运行时依赖。
#
# Key dependencies | 关键依赖:
# - GCC 8+ (for C++17 <filesystem> support)
# - CMake 3.16+ (for Abseil compatibility)
# - OpenMP (libomp) - for multi-threading in po_core
# - OpenSSL - for cryptography in po_core
# - Boost, Protobuf, Abseil - shared libraries required at runtime
# - And other system libraries...
#
# Version alignment | 版本对齐:
# All dependency versions are aligned with po_core/install.sh to ensure
# binary compatibility and prevent runtime errors.
# 所有依赖版本与po_core/install.sh对齐，以确保二进制兼容性并防止运行时错误。
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status.
# Note: 使用 || true 来允许某些非关键命令失败而继续

# Detect operating system | 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="ubuntu"
    echo "Detected Ubuntu/Debian system"
    echo "检测到Ubuntu/Debian系统"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="mac"
    echo "Detected macOS system"
    echo "检测到macOS系统"
else
    echo "Unsupported operating system: $OSTYPE"
    echo "不支持的操作系统: $OSTYPE"
    echo "This script supports Ubuntu/Debian and macOS only."
    echo "此脚本仅支持Ubuntu/Debian和macOS系统。"
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

# Check if running in non-interactive mode | 检查是否在非交互式模式下运行
# This can be set by DEBIAN_FRONTEND=noninteractive or CI environments
# 可以通过 DEBIAN_FRONTEND=noninteractive 或 CI 环境设置
POSDK_NON_INTERACTIVE=false
if [[ "$DEBIAN_FRONTEND" == "noninteractive" ]] || [[ -n "$CI" ]] || [[ -n "$POSDK_AUTO_YES" ]]; then
    POSDK_NON_INTERACTIVE=true
    print_info "Running in non-interactive mode - all prompts will use default answers"
    print_info "运行在非交互模式 - 所有提示将使用默认答案"
fi

# Function to handle interactive prompts | 处理交互式提示的函数
prompt_user() {
    local prompt_text="$1"
    local default_answer="${2:-n}"  # Default to 'n' if not specified

    if [[ "$POSDK_NON_INTERACTIVE" == "true" ]]; then
        print_info "Non-interactive mode: using default answer '$default_answer' for: $prompt_text"
        print_info "非交互模式：对提示 '$prompt_text' 使用默认答案 '$default_answer'"
        REPLY="$default_answer"
        return 0
    else
        read -p "$prompt_text" -n 1 -r
        echo
        return 0
    fi
}

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

if [ "$OS" == "ubuntu" ]; then
    print_info "Starting Ubuntu/Debian system dependencies installation..."
    print_info "开始Ubuntu/Debian系统依赖安装..."

    # 设置非交互式安装模式，避免 tzdata 等包询问配置
    # Set non-interactive mode to avoid prompts from packages like tzdata
    export DEBIAN_FRONTEND=noninteractive
    export NEEDRESTART_MODE=a
    
    # 预配置 tzdata 为亚洲/上海时区（避免交互式询问）
    # Pre-configure tzdata to Asia/Shanghai timezone (avoid interactive prompts)
    echo 'tzdata tzdata/Areas select Asia' | smart_sudo debconf-set-selections
    echo 'tzdata tzdata/Zones/Asia select Shanghai' | smart_sudo debconf-set-selections
    
    print_info "Set non-interactive installation mode"
    print_info "已设置非交互式安装模式"

    echo "Updating package lists..."
    echo "更新软件包列表..."
    smart_sudo apt update

    echo "Installing essential build tools and CMake..."
    # Install build-essential for compilers etc., cmake, curl, and ninja
    # curl命令行工具用于下载脚本的依赖项 | curl command-line tool needed for download scripts
    # ninja构建工具用于加速构建 | ninja build tool for faster builds
    smart_sudo apt install -y build-essential cmake curl ninja-build
    
    # Check GCC version and upgrade if needed (C++17 filesystem requires GCC 8+)
    # 检查 GCC 版本，如需要则升级（C++17 filesystem 需要 GCC 8+）
    print_info "Checking GCC version (po_core runtime requirement)..."
    print_info "检查 GCC 版本（po_core 运行时要求）..."
    
    GCC_VERSION=$(gcc --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    GCC_MAJOR=$(echo "$GCC_VERSION" | cut -d'.' -f1)
    
    print_info "Current GCC version: ${GCC_VERSION}"
    print_info "当前 GCC 版本: ${GCC_VERSION}"
    
    if [[ $GCC_MAJOR -lt 8 ]]; then
        print_warning "GCC ${GCC_VERSION} does not support C++17 <filesystem> (need GCC 8+)"
        print_warning "GCC ${GCC_VERSION} 不支持 C++17 <filesystem>（需要 GCC 8+）"
        print_info "Installing GCC 9 from Ubuntu toolchain PPA..."
        print_info "从 Ubuntu toolchain PPA 安装 GCC 9..."
        
        # Add Ubuntu toolchain PPA for newer GCC
        smart_sudo apt-get install -y software-properties-common
        smart_sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
        smart_sudo apt-get update -qq
        
        # Install GCC 9 and G++ 9
        smart_sudo apt-get install -y gcc-9 g++-9
        
        # Set GCC 9 as default
        smart_sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 \
            --slave /usr/bin/g++ g++ /usr/bin/g++-9 \
            --slave /usr/bin/gcov gcov /usr/bin/gcov-9
        
        # Verify upgrade
        NEW_GCC_VERSION=$(gcc --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
        print_success "GCC upgraded to: ${NEW_GCC_VERSION}"
        print_success "GCC 已升级到: ${NEW_GCC_VERSION}"
    else
        print_success "GCC version is sufficient (${GCC_VERSION} >= 8)"
        print_success "GCC 版本满足要求 (${GCC_VERSION} >= 8)"
    fi
    
    # Note: CMake 3.28+ installation is now handled by dependencies/install_cmake.sh
    # 注意：CMake 3.28+ 的安装现在由 dependencies/install_cmake.sh 处理
    # 
    # Why CMake 3.28+? | 为什么需要 CMake 3.28+？
    # - GLOMAP requires CMake 3.24+ for modern C++ features
    # - GLOMAP需要CMake 3.24+以支持现代C++特性
    # - Ubuntu 18.04 only has CMake 3.10 by default
    # - Ubuntu 18.04默认只有CMake 3.10
    # 
    # CMake will be checked and upgraded automatically when running:
    # CMake 会在运行以下脚本时自动检查和升级：
    #   dependencies/install_dependencies.sh
    print_info "CMake 3.28+ will be installed by dependencies/install_cmake.sh"
    print_info "CMake 3.28+ 将由 dependencies/install_cmake.sh 安装"

    echo "Installing Eigen3..."
    echo "安装 Eigen3..."
    
    # Check Eigen version and upgrade if needed for Ubuntu 18.04
    # Ubuntu 18.04 的 Eigen 3.3.4 启用了 EIGEN_MPL2_ONLY，导致 SparseCholesky 不可用
    
    # 优先检查 /usr/local（手动安装的位置）
    # Check /usr/local first (manually installed location)
    EIGEN_INSTALLED=false
    EIGEN_PATH=""
    
    if [[ -f /usr/local/include/eigen3/Eigen/Core ]]; then
        EIGEN_PATH="/usr/local/include/eigen3"
        EIGEN_VERSION=$(grep "EIGEN_WORLD_VERSION" ${EIGEN_PATH}/Eigen/src/Core/util/Macros.h 2>/dev/null | grep -oE '[0-9]+' | head -1)
        EIGEN_MAJOR=$(grep "EIGEN_MAJOR_VERSION" ${EIGEN_PATH}/Eigen/src/Core/util/Macros.h 2>/dev/null | grep -oE '[0-9]+' | head -1)
        
        if [[ "$EIGEN_VERSION" == "3" ]] && [[ "$EIGEN_MAJOR" -ge "4" ]]; then
            print_success "Eigen 3.4+ already installed at /usr/local"
            print_success "Eigen 3.4+ 已安装在 /usr/local"
            EIGEN_INSTALLED=true
        fi
    elif [[ -f /usr/include/eigen3/Eigen/Core ]]; then
        EIGEN_PATH="/usr/include/eigen3"
        EIGEN_VERSION=$(grep "EIGEN_WORLD_VERSION" ${EIGEN_PATH}/Eigen/src/Core/util/Macros.h 2>/dev/null | grep -oE '[0-9]+' | head -1)
        EIGEN_MAJOR=$(grep "EIGEN_MAJOR_VERSION" ${EIGEN_PATH}/Eigen/src/Core/util/Macros.h 2>/dev/null | grep -oE '[0-9]+' | head -1)
        
        if [[ "$EIGEN_VERSION" == "3" ]] && [[ "$EIGEN_MAJOR" -ge "4" ]]; then
            print_success "Eigen 3.4+ already installed at /usr"
            print_success "Eigen 3.4+ 已安装在 /usr"
            EIGEN_INSTALLED=true
        fi
    fi
    
    # 如果没有安装合适版本的 Eigen，则安装
    # If suitable Eigen version is not installed, install it
    if [[ "$EIGEN_INSTALLED" == false ]]; then
        if [[ -n "$EIGEN_PATH" ]] && [[ "$EIGEN_VERSION" == "3" ]] && [[ "$EIGEN_MAJOR" == "3" ]]; then
            print_warning "Detected Eigen 3.3.x at ${EIGEN_PATH} which has EIGEN_MPL2_ONLY enabled"
            print_warning "检测到 ${EIGEN_PATH} 的 Eigen 3.3.x，其启用了 EIGEN_MPL2_ONLY"
            print_info "Upgrading to Eigen 3.4.0 from source..."
            print_info "从源码升级到 Eigen 3.4.0..."
        else
            print_info "Installing Eigen 3.4.0 from source..."
            print_info "从源码安装 Eigen 3.4.0..."
        fi
        
        # Install Eigen 3.4.0 from source
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR"
        
        # Download Eigen 3.4.0
        print_info "Downloading Eigen 3.4.0..."
        curl -L https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz -o eigen-3.4.0.tar.gz
        tar -xzf eigen-3.4.0.tar.gz
        cd eigen-3.4.0
        
        # Install (Eigen is header-only, just copy files)
        mkdir build
        cd build
        cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
        smart_sudo make install
        
        cd /
        rm -rf "$TEMP_DIR"
        
        print_success "Eigen 3.4.0 installed to /usr/local"
        print_success "Eigen 3.4.0 已安装到 /usr/local"
        print_info "This version does not have EIGEN_MPL2_ONLY restriction"
        print_info "此版本没有 EIGEN_MPL2_ONLY 限制"
    fi

    echo "Installing SuiteSparse (sparse matrix library, includes CHOLMOD)..."
    # Required for sparse linear algebra operations in some components
    smart_sudo apt install -y libsuitesparse-dev

    echo "Installing OpenBLAS (optimized BLAS library)..."
    # May be used by other numerical libraries (Eigen, SuiteSparse, etc.) for performance
    smart_sudo apt install -y libopenblas-dev

    echo "Installing Boost (filesystem and system)..."
    # We mainly need filesystem and system, but installing all is often easier
    smart_sudo apt install -y libboost-all-dev

    echo "Installing Protobuf..."
    smart_sudo apt install -y libprotobuf-dev protobuf-compiler


    echo "Installing gflags (commandline flags library)..."
    # Required by some components for command-line argument parsing
    smart_sudo apt install -y libgflags-dev

    echo "Installing GTest (Google Test)..."
    smart_sudo apt install -y libgtest-dev
    # GTest often requires manual compilation after installing the -dev package
    echo "Compiling and installing GTest library..."
    print_info "编译并安装 GTest 库..."
    
    # Save current directory before switching to GTest directory
    # 在切换到 GTest 目录之前保存当前目录
    ORIGINAL_DIR="$(pwd)"
    
    if [[ -d /usr/src/googletest ]]; then
        cd /usr/src/googletest
        
        # Clean any previous build artifacts in the source directory
        # 清理源码目录中之前的构建文件
        print_info "Cleaning previous build artifacts..."
        print_info "清理之前的构建文件..."
        smart_sudo rm -rf CMakeCache.txt CMakeFiles/ Makefile cmake_install.cmake 2>/dev/null || true
        
        # Create and use a separate build directory
        # 创建并使用独立的构建目录
        BUILD_DIR="/usr/src/googletest/build"
        if [[ -d "$BUILD_DIR" ]]; then
            print_info "Removing existing build directory..."
            print_info "删除现有构建目录..."
            smart_sudo rm -rf "$BUILD_DIR"
        fi
        smart_sudo mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        # Configure and build
        print_info "Configuring GTest..."
        print_info "配置 GTest..."
        smart_sudo cmake .. -DCMAKE_BUILD_TYPE=Release || {
            print_error "CMake configuration failed for GTest"
            print_error "GTest CMake 配置失败"
            cd "$ORIGINAL_DIR"
            exit 1
        }
        
        print_info "Building GTest..."
        print_info "构建 GTest..."
        smart_sudo make -j$(nproc) || {
            print_error "GTest build failed"
            print_error "GTest 构建失败"
            cd "$ORIGINAL_DIR"
            exit 1
        }
        
        # Use make install to properly install headers and libraries
        # This ensures CMake's FindGTest.cmake can find everything
        print_info "Installing GTest..."
        print_info "安装 GTest..."
        smart_sudo make install || {
            # Fallback: manually copy files if make install fails
            print_warning "make install failed, manually copying files..."
            print_warning "make install 失败，手动复制文件..."
            
            # Copy libraries (Ubuntu 18.04: googlemock/gtest/, Ubuntu 20.04+: lib/)
        if [[ -d lib ]]; then
            smart_sudo cp lib/*.a /usr/lib/ 2>/dev/null || true
        fi
        if [[ -d googlemock/gtest ]]; then
            smart_sudo cp googlemock/gtest/*.a /usr/lib/ 2>/dev/null || true
            smart_sudo cp googlemock/*.a /usr/lib/ 2>/dev/null || true
        fi
        
            # Copy headers
            if [[ -d ../googletest/include ]]; then
                smart_sudo cp -r ../googletest/include/gtest /usr/include/ 2>/dev/null || true
            fi
            if [[ -d ../googlemock/include ]]; then
                smart_sudo cp -r ../googlemock/include/gmock /usr/include/ 2>/dev/null || true
            fi
        }
        
        # Restore original directory
        # 恢复原始目录
        cd "$ORIGINAL_DIR"
        print_success "GTest installation complete"
        print_success "GTest 安装完成"
    else
        print_warning "GTest source not found at /usr/src/googletest, skipping compilation"
        print_warning "未在 /usr/src/googletest 找到 GTest 源码，跳过编译"
        print_info "GTest may not work properly, but installation will continue"
        print_info "GTest 可能无法正常工作，但安装将继续"
    fi

    echo "Skipping OpenCV system package..."
    echo "跳过 OpenCV 系统包..."
    # OpenCV 将由 dependencies/install_opencv.sh 从源码安装（包含 contrib 模块）
    # OpenCV will be installed from source by dependencies/install_opencv.sh (with contrib modules)
    print_info "OpenCV will be built from source with xfeatures2d support"
    print_info "OpenCV 将从源码构建，包含 xfeatures2d 支持"

    echo "Installing Google glog (logging library)..."
    # Often a dependency for Ceres Solver
    smart_sudo apt install -y libgoogle-glog-dev

    echo "Skipping Abseil system package..."
    echo "跳过 Abseil 系统包..."
    # Abseil 在 Ubuntu 18.04 中不可用，将由 dependencies/install_absl.sh 从源码安装
    # Abseil is not available in Ubuntu 18.04, will be installed from source by dependencies/install_absl.sh
    print_info "Abseil will be built from source (required for specific version)"
    print_info "Abseil 将从源码构建（需要特定版本）"

    echo "Checking Ceres Solver availability..."
    echo "检查 Ceres Solver 可用性..."
    # Ceres Solver 在 Ubuntu 18.04 中可能不可用或版本过旧
    # Ceres Solver may not be available or too old in Ubuntu 18.04
    if ! smart_sudo apt install -y libceres-dev 2>/dev/null; then
        print_warning "Ceres Solver not available via apt (likely Ubuntu 18.04)"
        print_warning "Ceres Solver 无法通过 apt 安装（可能是 Ubuntu 18.04）"
        print_info "Ceres Solver can be built from source if needed"
        print_info "如需要可从源码构建 Ceres Solver"
    else
        print_success "Ceres Solver installed via apt"
        print_success "通过 apt 安装 Ceres Solver"
    fi

    echo "Installing CURL development library..."
    # Required for HTTP/HTTPS requests and downloads
    smart_sudo apt install -y libcurl4-openssl-dev

    echo "Installing SQLite3 development library..."
    echo "安装 SQLite3 开发库..."
    # Required by COLMAP (used by GraphOptim)
    # COLMAP 需要（GraphOptim 使用）
    smart_sudo apt install -y libsqlite3-dev
    print_success "SQLite3 development library installed"
    print_success "SQLite3 开发库安装完成"

    echo "Installing OpenSSL library (required by po_core)..."
    # OpenSSL库（加密和SSL/TLS）- po_core运行时依赖
    # OpenSSL library (cryptography and SSL/TLS) - po_core runtime dependency
    smart_sudo apt install -y libssl-dev
    print_success "OpenSSL library installed"
    print_success "OpenSSL库安装完成"

    echo "Installing OpenMP support (required by po_core)..."
    # OpenMP支持 - po_core运行时依赖（多线程优化）
    # OpenMP support - po_core runtime dependency (multi-threading optimization)
    smart_sudo apt install -y libomp-dev
    print_success "OpenMP support installed"
    print_success "OpenMP支持安装完成"

    echo "Installing GLib development library (required by OpenCV GTK-3)..."
    # GLib 开发库 - OpenCV GTK-3 支持所需
    # OpenCV uses GTK-3 on Linux, which requires GLib
    # OpenCV 在 Linux 上使用 GTK-3，需要 GLib
    smart_sudo apt install -y libglib2.0-dev
    print_success "GLib development library installed"
    print_success "GLib开发库安装完成"

    echo "Installing OpenGL development libraries..."
    # Required by some plugins or visualization components
    # Note: libglx-mesa0-dev might not exist on newer Ubuntu versions (like 24.04+)
    # Necessary components are likely included in libgl1-mesa-dev
    smart_sudo apt install -y libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev freeglut3-dev

    echo "Installing GLFW3 development library..."
    # Used for windowing and input, often with OpenGL
    smart_sudo apt install -y libglfw3-dev

    echo "Installing Qt development libraries..."
    echo "安装Qt开发库..."
    
    # 检测 Ubuntu 版本以选择合适的 Qt 版本
    # Detect Ubuntu version to choose appropriate Qt version
    if command -v lsb_release >/dev/null 2>&1; then
        UBUNTU_VERSION=$(lsb_release -rs | cut -d'.' -f1)
    else
        UBUNTU_VERSION="unknown"
    fi
    
    print_info "Ubuntu version: ${UBUNTU_VERSION}"
    print_info "Ubuntu 版本: ${UBUNTU_VERSION}"
    
    # Ubuntu 18.04 及更早版本：使用 Qt5
    # Ubuntu 20.04+：优先尝试 Qt6，失败则降级到 Qt5
    if [[ "$UBUNTU_VERSION" == "18" ]] || [[ "$UBUNTU_VERSION" == "16" ]]; then
        print_info "Ubuntu ${UBUNTU_VERSION}.04 detected, installing Qt5..."
        print_info "检测到 Ubuntu ${UBUNTU_VERSION}.04，安装 Qt5..."
        
        # Ubuntu 18.04 使用 Qt5
        if smart_sudo apt install -y qtbase5-dev qtdeclarative5-dev qt5-default 2>/dev/null; then
            print_success "Qt5 development libraries installed"
            print_success "Qt5 开发库安装完成"
        else
            # 如果 qt5-default 不可用（Ubuntu 20.04+ 已移除），尝试只安装基础包
            smart_sudo apt install -y qtbase5-dev qtdeclarative5-dev
            print_success "Qt5 development libraries installed"
            print_success "Qt5 开发库安装完成"
        fi
    else
        print_info "Ubuntu ${UBUNTU_VERSION}.04+ detected, trying Qt6..."
        print_info "检测到 Ubuntu ${UBUNTU_VERSION}.04+，尝试安装 Qt6..."
        
        # Ubuntu 20.04+ 优先尝试 Qt6
        if smart_sudo apt install -y qt6-base-dev qt6-declarative-dev 2>/dev/null; then
            print_success "Qt6 development libraries installed"
            print_success "Qt6 开发库安装完成"
        else
            print_warning "Qt6 not available, falling back to Qt5..."
            print_warning "Qt6 不可用，降级到 Qt5..."
            
            # 降级到 Qt5
            smart_sudo apt install -y qtbase5-dev qtdeclarative5-dev
            print_success "Qt5 development libraries installed"
            print_success "Qt5 开发库安装完成"
        fi
    fi

elif [ "$OS" == "mac" ]; then
    print_info "Starting macOS system dependencies installation..."
    print_info "开始macOS系统依赖安装..."

    # Check if Homebrew is installed | 检查Homebrew是否已安装
    if ! command -v brew &> /dev/null; then
        print_warning "Homebrew not found. Installing Homebrew..."
        print_warning "未找到Homebrew。正在安装Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

        # Add Homebrew to PATH for current session
        if [[ -f "/opt/homebrew/bin/brew" ]]; then
            echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [[ -f "/usr/local/bin/brew" ]]; then
            echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zprofile
            eval "$(/usr/local/bin/brew shellenv)"
        fi
        print_success "Homebrew installed successfully"
        print_success "Homebrew安装成功"
    else
        print_success "Homebrew is already installed"
        print_success "Homebrew已安装"
    fi

    # Skip brew update if HOMEBREW_NO_AUTO_UPDATE is set | 如果设置了HOMEBREW_NO_AUTO_UPDATE则跳过更新
    if [ -z "$HOMEBREW_NO_AUTO_UPDATE" ]; then
        echo "Updating Homebrew..."
        brew update || {
            print_warning "Homebrew update failed, but continuing with installation..."
            print_warning "Homebrew更新失败，但继续安装..."
            print_info "You can skip updates by setting: export HOMEBREW_NO_AUTO_UPDATE=1"
            print_info "可通过设置以下变量跳过更新: export HOMEBREW_NO_AUTO_UPDATE=1"
        }
    else
        print_info "Skipping Homebrew update (HOMEBREW_NO_AUTO_UPDATE is set)"
        print_info "跳过Homebrew更新（已设置HOMEBREW_NO_AUTO_UPDATE）"
    fi

    echo "Installing essential build tools and CMake..."
    # Install Xcode command line tools if not already installed
    if ! xcode-select -p &> /dev/null; then
        print_info "Installing Xcode command line tools..."
        print_info "安装Xcode命令行工具..."
        xcode-select --install
        print_warning "Please complete Xcode command line tools installation and re-run this script"
        print_warning "请完成Xcode命令行工具安装后重新运行此脚本"
        exit 1
    fi

    brew install cmake
    
    echo "Installing ninja build system (for faster builds)..."
    # ninja构建工具用于加速构建 | ninja build tool for faster builds
    brew install ninja
    
    echo "Installing pkg-config (manage compile and link flags)..."
    # pkg-config用于管理编译和链接标志 | pkg-config for managing compile and link flags
    brew install pkg-config

    # Check if curl is available, install if needed (usually pre-installed on macOS)
    # 检查curl是否可用，如果需要则安装（macOS通常预装curl） | Check if curl is available, install if needed
    if ! command -v curl >/dev/null 2>&1; then
        print_info "Installing curl command-line tool..."
        print_info "安装curl命令行工具..."
        brew install curl
    else
        print_success "curl command-line tool is already available"
        print_success "curl命令行工具已可用"
    fi

    # Note: CMake 3.28+ will be installed by dependencies/install_cmake.sh if needed
    # 注意：如需要，CMake 3.28+ 将由 dependencies/install_cmake.sh 安装
    # 
    # Why CMake 3.28+? | 为什么需要 CMake 3.28+？
    # - GLOMAP requires CMake 3.24+ for modern C++ features
    # - GLOMAP需要CMake 3.24+以支持现代C++特性
    # - macOS Homebrew usually provides recent CMake, but we still check in dependencies
    # - macOS Homebrew 通常提供最新的 CMake，但我们仍在 dependencies 中进行检查
    print_info "CMake 3.28+ will be checked and installed by dependencies/install_cmake.sh if needed"
    print_info "如需要，CMake 3.28+ 将由 dependencies/install_cmake.sh 检查和安装"

    echo "Installing Eigen3..."
    brew install eigen

    echo "Installing SuiteSparse (sparse matrix library, includes CHOLMOD)..."
    # Required for sparse linear algebra operations in some components
    brew install suite-sparse

    echo "Installing OpenBLAS (optimized BLAS library)..."
    # May be used by other numerical libraries (Eigen, SuiteSparse, etc.) for performance
    brew install openblas

    echo "Installing Boost (filesystem and system)..."
    # We mainly need filesystem and system, but installing all is often easier
    brew install boost

    echo "Installing Protobuf..."
    brew install protobuf

    echo "Installing nlohmann_json (JSON library for Modern C++)..."
    # Required for JSON parsing and serialization
    brew install nlohmann-json

    echo "Installing gflags (commandline flags library)..."
    # Required by some components for command-line argument parsing
    brew install gflags

    echo "Installing GTest (Google Test)..."
    brew install googletest

    echo "Installing OpenCV..."
    
    # Ensure qt@5 is unlinked to prevent conflicts with OpenCV dependencies
    # 确保qt@5未链接，以防止与OpenCV依赖冲突
    if brew list qt@5 &>/dev/null; then
        brew unlink qt@5 &>/dev/null || true
    fi
    
    brew install opencv

    echo "Installing Google glog (logging library)..."
    # Often a dependency for Ceres Solver
    brew install glog

    echo "Installing Abseil (common C++ libraries)..."
    brew install abseil

    echo "Installing OpenMP support (required by po_core)..."
    # OpenMP支持 - po_core运行时依赖（多线程优化）
    # OpenMP support - po_core runtime dependency (multi-threading optimization)
    brew install libomp
    print_success "OpenMP support installed"
    print_success "OpenMP支持安装完成"

    echo "Installing Ceres Solver (non-linear optimization library)..."
    # Depends on Eigen, glog, gflags, SuiteSparse etc.
    brew install ceres-solver

    echo "Installing CURL development library..."
    # Required for HTTP/HTTPS requests and downloads (usually pre-installed on macOS)
    brew install curl

    echo "Installing OpenGL development libraries..."
    # Required by some plugins or visualization components
    # Note: OpenGL is part of macOS system frameworks, but we might need GLEW
    brew install glew

    echo "Installing GLFW3 development library..."
    # Used for windowing and input, often with OpenGL
    brew install glfw

    echo "Checking for Qt installations..."
    echo "检查Qt安装情况..."
    
    # Check for all possible Qt installations | 检查所有可能的Qt安装
    QT_INSTALLED=false
    QT_VERSION=""
    QT_PACKAGE=""
    QT_NEEDS_UPGRADE=false
    
    # Check for qt (unversioned, usually Qt6) | 检查qt（无版本号，通常是Qt6）
    if brew list qt &>/dev/null; then
        QT_VERSION=$(brew list --versions qt | awk '{print $2}')
        QT_PACKAGE="qt"
        QT_INSTALLED=true
        print_info "Found Qt package: qt (version $QT_VERSION)"
        print_info "发现Qt包: qt (版本 $QT_VERSION)"
        
        # Check if Qt needs upgrade | 检查Qt是否需要升级
        QT_LATEST=$(brew info qt | grep "stable" | awk '{print $3}')
        if [ "$QT_VERSION" != "$QT_LATEST" ]; then
            QT_NEEDS_UPGRADE=true
            print_warning "Qt version $QT_VERSION is outdated (latest: $QT_LATEST)"
            print_warning "Qt版本 $QT_VERSION 已过时（最新版: $QT_LATEST）"
            print_warning "Outdated Qt may cause conflicts with OpenCV dependencies"
            print_warning "过时的Qt可能导致与OpenCV依赖冲突"
        fi
    fi
    
    # Check for qt6 | 检查qt6
    if brew list qt6 &>/dev/null; then
        QT_VERSION=$(brew list --versions qt6 | awk '{print $2}')
        QT_PACKAGE="qt6"
        QT_INSTALLED=true
        print_info "Found Qt package: qt6 (version $QT_VERSION)"
        print_info "发现Qt包: qt6 (版本 $QT_VERSION)"
    fi
    
    # Check for qt@6 | 检查qt@6
    if brew list qt@6 &>/dev/null; then
        QT_VERSION=$(brew list --versions qt@6 | awk '{print $2}')
        QT_PACKAGE="qt@6"
        QT_INSTALLED=true
        print_info "Found Qt package: qt@6 (version $QT_VERSION)"
        print_info "发现Qt包: qt@6 (版本 $QT_VERSION)"
    fi
    
    # Check for qt@5 | 检查qt@5
    if brew list qt@5 &>/dev/null; then
        print_warning "Found Qt5 installation (qt@5)"
        print_warning "发现Qt5安装 (qt@5)"
        print_warning "Qt5 may cause conflicts with Qt6 dependencies required by OpenCV"
        print_warning "Qt5可能会与OpenCV所需的Qt6依赖冲突"
    fi
    
    if [ "$QT_INSTALLED" = true ]; then
        print_success "Qt6 is already installed via package: $QT_PACKAGE"
        print_success "Qt6已通过包安装: $QT_PACKAGE"
        
        # Check if Qt needs upgrade and offer to upgrade | 检查Qt是否需要升级并提供升级选项
        if [ "$QT_NEEDS_UPGRADE" = true ]; then
            echo ""
            print_warning "================================"
            print_warning "Qt Upgrade Recommended"
            print_warning "建议升级Qt"
            print_warning "================================"
            print_info "Current version: $QT_VERSION"
            print_info "Latest version:  $QT_LATEST"
            print_info "当前版本: $QT_VERSION"
            print_info "最新版本: $QT_LATEST"
            echo ""
            print_warning "Outdated Qt may cause conflicts when installing OpenCV dependencies."
            print_warning "过时的Qt在安装OpenCV依赖时可能导致版本冲突。"
            print_info "It is HIGHLY RECOMMENDED to upgrade Qt before continuing."
            print_info "强烈建议在继续之前升级Qt。"
            echo ""
            read -p "Would you like to upgrade Qt now? 是否现在升级Qt? [Y/n]: " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Nn]$ ]]; then
                print_info "Upgrading Qt..."
                print_info "正在升级Qt..."
                brew upgrade qt
                # Update Qt version after upgrade | 升级后更新Qt版本
                QT_VERSION=$(brew list --versions qt | awk '{print $2}')
                print_success "Qt upgraded successfully to version $QT_VERSION"
                print_success "Qt成功升级到版本 $QT_VERSION"
                QT_NEEDS_UPGRADE=false
            else
                print_warning "Skipping Qt upgrade. You may encounter conflicts during OpenCV installation."
                print_warning "跳过Qt升级。在OpenCV安装期间您可能会遇到冲突。"
                print_warning "If installation fails, please run: brew upgrade qt"
                print_warning "如果安装失败，请运行: brew upgrade qt"
            fi
            echo ""
        else
            print_info "Qt is up-to-date. Skipping Qt6 installation to avoid conflicts"
            print_info "Qt已是最新版本。跳过Qt6安装以避免冲突"
        fi
        
        # Check if any Qt version conflicts exist and offer to fix | 检查是否存在Qt版本冲突并提供修复
        if brew list qt@5 &>/dev/null && brew list qt &>/dev/null; then
            echo ""
            print_warning "Multiple Qt versions detected (qt@5 and qt)"
            print_warning "检测到多个Qt版本 (qt@5 和 qt)"
            print_warning "This may cause linking conflicts. Recommended action:"
            print_warning "这可能导致链接冲突。建议操作："
            print_warning "  brew unlink qt@5"
            print_warning "  brew link --overwrite qt"
            echo ""
            read -p "Would you like to fix this automatically? 是否自动修复? [y/N]: " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                print_info "Unlinking qt@5..."
                brew unlink qt@5 || true
                print_info "Relinking $QT_PACKAGE..."
                brew link --overwrite $QT_PACKAGE || true
                print_success "Qt linking conflicts resolved"
                print_success "Qt链接冲突已解决"
            else
                print_warning "Skipping automatic fix. You may encounter Qt-related errors."
                print_warning "跳过自动修复。您可能会遇到Qt相关错误。"
            fi
        fi
        
        # Check if Qt environment variables need to be set | 检查是否需要设置Qt环境变量
        QT_PATH="/opt/homebrew/opt/${QT_PACKAGE}"
        if [ -d "$QT_PATH" ]; then
            if ! grep -q "${QT_PACKAGE}/bin" ~/.zprofile 2>/dev/null; then
                print_info "Setting up Qt environment variables for $QT_PACKAGE..."
                print_info "为 $QT_PACKAGE 设置环境变量..."
                echo "export PATH=\"${QT_PATH}/bin:\$PATH\"" >> ~/.zprofile
                echo "export LDFLAGS=\"-L${QT_PATH}/lib\"" >> ~/.zprofile
                echo "export CPPFLAGS=\"-I${QT_PATH}/include\"" >> ~/.zprofile
                echo "export PKG_CONFIG_PATH=\"${QT_PATH}/lib/pkgconfig\"" >> ~/.zprofile
                print_success "Qt environment variables added to ~/.zprofile"
                print_success "Qt环境变量已添加到~/.zprofile"
            else
                print_success "Qt environment variables already configured"
                print_success "Qt环境变量已配置"
            fi
        fi
    else
        print_info "No Qt6 installation found, will install qt6..."
        print_info "未找到Qt6安装，将安装qt6..."
        
        # Warn about Qt5 if present | 如果存在Qt5则警告
        if brew list qt@5 &>/dev/null; then
            print_warning "Qt5 detected. This may cause conflicts with Qt6."
            print_warning "检测到Qt5。这可能与Qt6冲突。"
            read -p "Continue with Qt6 installation? 继续安装Qt6吗? [y/N]: " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                print_info "Skipping Qt6 installation."
                print_info "跳过Qt6安装。"
                return
            fi
        fi
        
        # Install Qt6
        brew install qt6
        
        # Add Qt6 to PATH
        echo 'export PATH="/opt/homebrew/opt/qt6/bin:$PATH"' >> ~/.zprofile
        echo 'export LDFLAGS="-L/opt/homebrew/opt/qt6/lib"' >> ~/.zprofile
        echo 'export CPPFLAGS="-I/opt/homebrew/opt/qt6/include"' >> ~/.zprofile
        echo 'export PKG_CONFIG_PATH="/opt/homebrew/opt/qt6/lib/pkgconfig"' >> ~/.zprofile
        
        print_success "Qt6 development libraries installed"
        print_success "Qt6开发库安装完成"
    fi

fi

echo "-----------------------------------------------------"
echo "System dependencies installation completed."
echo "系统依赖安装完成。"
echo "-----------------------------------------------------"
echo ""
print_success "✓ All runtime dependencies for po_core are installed"
print_success "✓ po_core所有运行时依赖已安装"
print_info "Version alignment summary | 版本对齐摘要:"
if [ "$OS" == "ubuntu" ]; then
    print_info "  - GCC: $(gcc --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'unknown') (required: >= 8.0)"
    print_info "  - CMake: $(cmake --version | head -1 | grep -oP '\d+\.\d+\.\d+' || echo 'unknown') (required: >= 3.16)"
elif [ "$OS" == "mac" ]; then
    print_info "  - CMake: $(cmake --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'unknown') (required: >= 3.16)"
fi
print_info "  - OpenMP: installed (required by po_core for multi-threading)"
if [ "$OS" == "ubuntu" ]; then
    print_info "  - OpenSSL: installed (required by po_core for cryptography)"
fi
echo ""

echo ""
echo "========================================"
echo "   Installing PoSDK test data"
echo "========================================"
echo "This will download the Strecha dataset for testing PoSDK functionality."

# Get the directory where this script is located | 获取脚本所在目录
# Use multiple methods for robustness in different execution environments
# 使用多种方法确保在不同执行环境中的健壮性

# Method 0: Use environment variable if provided (for piped execution)
# 方法0：如果提供了环境变量则使用（用于管道执行）
if [[ -n "$POSDK_INSTALL_DIR" ]] && [[ -d "$POSDK_INSTALL_DIR" ]]; then
    INSTALL_SCRIPT_DIR="$POSDK_INSTALL_DIR"
    print_info "Using install directory from environment: $INSTALL_SCRIPT_DIR"
    print_info "使用环境变量提供的安装目录: $INSTALL_SCRIPT_DIR"
elif [[ -n "${BASH_SOURCE[0]}" ]] && [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    # Method 1: Use BASH_SOURCE (most reliable when available)
    # 方法1：使用BASH_SOURCE（可用时最可靠）
    INSTALL_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
elif [[ "${0}" == /* ]]; then
    # Method 2: Use $0 if it's an absolute path
    # 方法2：如果$0是绝对路径则使用它
    INSTALL_SCRIPT_DIR="$(cd "$(dirname "${0}")" && pwd)"
else
    # Method 3: Use current working directory as fallback
    # 方法3：使用当前工作目录作为备选
    INSTALL_SCRIPT_DIR="$(pwd)"
fi

# Verify the script directory is valid | 验证脚本目录有效性
if [[ -z "$INSTALL_SCRIPT_DIR" ]] || [[ ! -d "$INSTALL_SCRIPT_DIR" ]]; then
    print_error "Failed to determine script directory"
    print_error "无法确定脚本目录"
    exit 1
fi

print_info "Script directory: $INSTALL_SCRIPT_DIR"
print_info "脚本目录: $INSTALL_SCRIPT_DIR"

# Get the tests directory
TESTS_DIR="${INSTALL_SCRIPT_DIR}/tests"

# Check if tests directory exists
if [ -d "${TESTS_DIR}" ]; then
    # Check if the test data download script exists
    TEST_DATA_SCRIPT="${TESTS_DIR}/download_Strecha.sh"
    if [ -f "${TEST_DATA_SCRIPT}" ]; then
        echo "Found test data download script at: ${TEST_DATA_SCRIPT}"
        echo "Adding execution permission..."
        chmod +x "${TEST_DATA_SCRIPT}"

        echo ""
        echo "Starting test data download..."
        cd "${TESTS_DIR}"
        ./download_Strecha.sh || {
            echo "Warning: Test data download failed, but installation will continue"
            echo "You can download test data manually later by running:"
            echo "cd ${TESTS_DIR} && ./download_Strecha.sh"
        }
        cd "${INSTALL_SCRIPT_DIR}"
    else
        echo "Test data download script not found at ${TEST_DATA_SCRIPT}"
        echo "Skipping test data download..."
    fi
else
    echo "Tests directory not found at ${TESTS_DIR}"
    echo "Skipping test data download..."
fi




DEPENDENCIES_DIR="${INSTALL_SCRIPT_DIR}/dependencies"

print_info "Dependencies directory: $DEPENDENCIES_DIR"
print_info "依赖目录: $DEPENDENCIES_DIR"

# Check if dependencies directory exists
if [ ! -d "${DEPENDENCIES_DIR}" ]; then
    print_error "Error: Dependencies directory not found at ${DEPENDENCIES_DIR}"
    print_error "错误: 在 ${DEPENDENCIES_DIR} 未找到依赖目录"
    print_error "Please ensure the dependencies directory exists in the same folder as this script."
    print_error "请确保依赖目录存在于此脚本相同文件夹中。"

    # Try to help diagnose the issue | 尝试帮助诊断问题
    print_info "Current script directory contents | 当前脚本目录内容:"
    ls -la "${INSTALL_SCRIPT_DIR}" || true
    exit 1
fi

# Check if the dependencies installation script exists
DEPENDENCIES_SCRIPT="${DEPENDENCIES_DIR}/install_dependencies.sh"
if [ ! -f "${DEPENDENCIES_SCRIPT}" ]; then
    echo "Error: Dependencies installation script not found at ${DEPENDENCIES_SCRIPT}"
    echo "Please ensure install_dependencies.sh exists in the dependencies directory."
    exit 1
fi

echo "Found dependencies installation script at: ${DEPENDENCIES_SCRIPT}"
echo "Adding execution permission..."
chmod +x "${DEPENDENCIES_SCRIPT}"

echo ""
echo "Starting PoSDK dependencies installation..."
cd "${DEPENDENCIES_DIR}"
./install_dependencies.sh


echo ""
echo "========================================"
echo "   Complete installation finished!"
echo "   安装完成！"
echo "========================================"
echo "========================================"
echo "System dependencies, PoSDK dependencies, and test data have been successfully installed."
echo "系统依赖、PoSDK依赖和测试数据已成功安装。"
echo ""
echo "Next steps | 下一步："
if [ "$OS" == "ubuntu" ]; then
    echo "1. Build PoSDK main project | 构建PoSDK主项目:"
    echo "   cd build"
    echo "   cmake ../src"
    echo "   make -j\$(nproc)"
    echo ""
    echo "2. Run tests with Strecha dataset (if downloaded) | 运行Strecha数据集测试（如果已下载）:"
    echo "   # Test data is available at: tests/Strecha/"
    echo "   # 测试数据位于: tests/Strecha/"
    echo ""
    echo "3. Install Qt Creator manually using the official installer from qt.io for the best experience."
    echo "   手动从qt.io安装Qt Creator以获得最佳体验。"
elif [ "$OS" == "mac" ]; then
    echo "1. Build PoSDK main project | 构建PoSDK主项目:"
    echo "   cd build"
    echo "   cmake ../src"
    echo "   make -j\$(sysctl -n hw.ncpu)"
    echo ""
    echo "2. Run tests with Strecha dataset (if downloaded) | 运行Strecha数据集测试（如果已下载）:"
    echo "   # Test data is available at: tests/Strecha/"
    echo "   # 测试数据位于: tests/Strecha/"
    echo ""
    echo "3. For Qt development, you may want to install Qt Creator:"
    echo "   brew install --cask qt-creator"
    echo "   或者从qt.io手动安装Qt Creator以获得最佳体验。"
    echo ""
    echo "4. Reload your shell profile to use new environment variables:"
    echo "   source ~/.zprofile"
    echo "   重新加载shell配置以使用新的环境变量。"
fi
echo ""
echo "If you encounter issues | 如果遇到问题，请检查："
echo "- Ensure all source directories exist under dependencies/ directory"
echo "  确保所有源代码目录存在于dependencies/目录下"
echo "- Ensure test data is available under tests/ directory"
echo "  确保测试数据在tests/目录下可用"
if [ "$OS" == "ubuntu" ]; then
    echo "- Ensure necessary build tools are installed (cmake, make, gcc/clang)"
    echo "  确保必要的构建工具已安装 (cmake, make, gcc/clang)"
elif [ "$OS" == "mac" ]; then
    echo "- Ensure Xcode command line tools are installed"
    echo "  确保Xcode命令行工具已安装"
    echo "- Ensure Homebrew packages are accessible"
    echo "  确保Homebrew包可以访问"
fi
echo "- Review output logs from individual installation scripts"
echo "  查看各个安装脚本的输出日志"
echo "========================================"
