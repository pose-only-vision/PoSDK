#!/bin/bash
set -e # Stop execution on error

# OpenCV Installation Script
# Supports macOS and Linux, standard OpenCV modules only

echo "========================================"
echo "  OpenCV Installation Script"
echo "========================================"
echo "This script will compile OpenCV with standard modules"
echo "Note: This version does NOT include opencv_contrib extensions"
echo "========================================"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPENDENCIES_DIR="$SCRIPT_DIR"
echo "Dependencies directory: ${DEPENDENCIES_DIR}"

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

# Detect operating system
detect_os() {
    case "$(uname -s)" in
        Darwin*) 
            OS="macOS"
            if [[ "$(uname -m)" == "arm64" ]]; then
                ARCH="arm64"
            else
                ARCH="x86_64" 
            fi
            ;;
        Linux*)  
            OS="Linux"
            if [[ "$(uname -m)" == "aarch64" ]]; then
                ARCH="aarch64"
            else
                ARCH="x86_64"
            fi
            ;;
        *)       
            echo "❌ Unsupported operating system: $(uname -s)"
            exit 1
            ;;
    esac
    
    echo "✓ Detected operating system: $OS ($ARCH)"
}

# Check system dependencies
check_system_dependencies() {
    echo "Checking system dependencies..."
    
    local missing_deps=()
    
    # Check basic build tools
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v make >/dev/null 2>&1 || missing_deps+=("make") 
    command -v git >/dev/null 2>&1 || missing_deps+=("git")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")
    
    # Check compiler
    if [[ "$OS" == "macOS" ]]; then
        if ! command -v clang >/dev/null 2>&1; then
            echo "❌ clang compiler not found, please install Xcode Command Line Tools:"
            echo "   xcode-select --install"
            exit 1
        fi
    else
        command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
        command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    fi
    
    if [[ ${#missing_deps[@]} -ne 0 ]]; then
        echo "❌ Missing system dependencies: ${missing_deps[*]}"
        if [[ "$OS" == "macOS" ]]; then
            echo "Please install with Homebrew: brew install ${missing_deps[*]}"
        else
            echo "Please install with package manager: apt-get install ${missing_deps[*]}"
        fi
        exit 1
    fi
    
    echo "✓ System dependencies check passed"
}

# Fix HDF5/MPI conflict issues
fix_hdf5_mpi_conflicts() {
    if [[ "$OS" == "Linux" ]]; then
        echo "Checking and fixing HDF5/MPI configuration conflicts..."
        
        # Check if multiple HDF5 versions exist
        if dpkg -l | grep -q libhdf5-openmpi-dev && dpkg -l | grep -q libhdf5-dev; then
            echo "⚠️  Detected both serial and OpenMPI versions of HDF5 installed"
            echo "To avoid conflicts, will remove OpenMPI version and use serial version"
            
            # Remove OpenMPI version of HDF5
            smart_sudo apt-get remove -y libhdf5-openmpi-dev || true
            
            # Ensure serial version is installed
            smart_sudo apt-get install -y libhdf5-dev || true
        fi
        
        # Check MPI installation status
        if dpkg -l | grep -q libhdf5-openmpi-dev; then
            if ! dpkg -l | grep -q libopenmpi-dev; then
                echo "Installing complete OpenMPI development packages..."
                smart_sudo apt-get install -y libopenmpi-dev openmpi-bin libopenmpi-common
            fi
        fi
        
        echo "✓ HDF5/MPI configuration check completed"
    fi
}

# Check and install/update Python 3.8+
check_and_install_python() {
    if [[ "$OS" == "Linux" ]]; then
        echo "Checking Python version..."
        
        if command -v python3 &> /dev/null; then
            PYTHON_VERSION=$(python3 --version 2>&1 | awk '{print $2}')
            PYTHON_MAJOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f1)
            PYTHON_MINOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f2)
            
            echo "Current Python version: $PYTHON_VERSION"
            
            # Check if Python version is sufficient (>= 3.8)
            if [[ "$PYTHON_MAJOR" -ge 3 ]] && [[ "$PYTHON_MINOR" -ge 8 ]]; then
                echo "✓ Python version is sufficient ($PYTHON_VERSION >= 3.8)"
            else
                echo "⚠️  Python version too old ($PYTHON_VERSION < 3.8)"
                echo "Attempting to install Python 3.10 or newer..."
                
                # Detect Ubuntu version
                if [ -f /etc/os-release ]; then
                    . /etc/os-release
                    UBUNTU_VERSION=$(echo "$VERSION_ID" | cut -d'.' -f1,2)
                    UBUNTU_CODENAME=$(lsb_release -cs 2>/dev/null || echo "")
                    
                    if [[ -z "$UBUNTU_CODENAME" ]]; then
                        # Fallback: use VERSION_CODENAME or ID_LIKE
                        UBUNTU_CODENAME=$(echo "$VERSION_CODENAME" || echo "focal")
                    fi
                    
                    echo "Detected Ubuntu version: $UBUNTU_VERSION ($UBUNTU_CODENAME)"
                    
                    # Add deadsnakes PPA for newer Python versions
                    echo "Adding deadsnakes PPA for Python 3.10+..."
                    smart_sudo apt-get install -y software-properties-common
                    smart_sudo add-apt-repository -y ppa:deadsnakes/ppa 2>/dev/null || {
                        echo "⚠️  Failed to add deadsnakes PPA, trying alternative approach..."
                        
                        # Try to install Python 3.10 from Ubuntu's default repository (Ubuntu 20.04+)
                        # Compare versions using bash arithmetic
                        UBUNTU_MAJOR=$(echo "$UBUNTU_VERSION" | cut -d'.' -f1)
                        UBUNTU_MINOR=$(echo "$UBUNTU_VERSION" | cut -d'.' -f2)
                        if [[ "$UBUNTU_MAJOR" -gt 20 ]] || [[ "$UBUNTU_MAJOR" -eq 20 && "$UBUNTU_MINOR" -ge 4 ]]; then
                            echo "Trying to install Python 3.10 from default repository..."
                            smart_sudo apt-get install -y python3.10 python3.10-dev python3.10-venv python3.10-distutils || {
                                echo "⚠️  Python 3.10 not available in default repository"
                            }
                        fi
                    }
                    
                    smart_sudo apt-get update
                    
                    # Try to install Python 3.10 or 3.11 (Ubuntu 18.04 may only have Python 3.10 in deadsnakes)
                    # Use explicit package names and check availability first
                    PYTHON_INSTALLED=false
                    INSTALLED_PYTHON_VERSION=""
                    
                    # Try Python 3.11 first (if available on this Ubuntu version)
                    if apt-cache pkgnames | grep -q "^python3\.11$"; then
                        echo "Attempting to install Python 3.11..."
                        if smart_sudo apt-get install -y python3.11 python3.11-dev python3.11-venv python3.11-distutils >/dev/null 2>&1; then
                        echo "✓ Python 3.11 installed successfully"
                            smart_sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1 2>/dev/null || true
                            smart_sudo update-alternatives --set python3 /usr/bin/python3.11 2>/dev/null || true
                            INSTALLED_PYTHON_VERSION="3.11"
                            PYTHON_INSTALLED=true
                        fi
                    fi
                    
                    # If Python 3.11 failed, try Python 3.10 (more widely available, especially for Ubuntu 18.04)
                    if [[ "$PYTHON_INSTALLED" == "false" ]]; then
                        if apt-cache pkgnames | grep -q "^python3\.10$"; then
                            echo "Attempting to install Python 3.10..."
                            if smart_sudo apt-get install -y python3.10 python3.10-dev python3.10-venv python3.10-distutils >/dev/null 2>&1; then
                        echo "✓ Python 3.10 installed successfully"
                                smart_sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.10 1 2>/dev/null || true
                                smart_sudo update-alternatives --set python3 /usr/bin/python3.10 2>/dev/null || true
                                INSTALLED_PYTHON_VERSION="3.10"
                                PYTHON_INSTALLED=true
                            fi
                        fi
                    fi
                    
                    # If Python 3.10 failed, try Python 3.9 (fallback for older Ubuntu versions)
                    if [[ "$PYTHON_INSTALLED" == "false" ]]; then
                        if apt-cache pkgnames | grep -q "^python3\.9$"; then
                            echo "Attempting to install Python 3.9..."
                            if smart_sudo apt-get install -y python3.9 python3.9-dev python3.9-venv python3.9-distutils >/dev/null 2>&1; then
                                echo "✓ Python 3.9 installed successfully"
                                smart_sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 1 2>/dev/null || true
                                smart_sudo update-alternatives --set python3 /usr/bin/python3.9 2>/dev/null || true
                                INSTALLED_PYTHON_VERSION="3.9"
                                PYTHON_INSTALLED=true
                            fi
                        fi
                    fi
                    
                    # If Python 3.9 failed, try Python 3.8 (minimum required version)
                    if [[ "$PYTHON_INSTALLED" == "false" ]]; then
                        if apt-cache pkgnames | grep -q "^python3\.8$"; then
                            echo "Attempting to install Python 3.8 (minimum required version)..."
                            if smart_sudo apt-get install -y python3.8 python3.8-dev python3.8-venv python3.8-distutils >/dev/null 2>&1; then
                                echo "✓ Python 3.8 installed successfully"
                                smart_sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 1 2>/dev/null || true
                                smart_sudo update-alternatives --set python3 /usr/bin/python3.8 2>/dev/null || true
                                INSTALLED_PYTHON_VERSION="3.8"
                                PYTHON_INSTALLED=true
                            fi
                        fi
                    fi
                    
                    # Verify installation
                    if [[ "$PYTHON_INSTALLED" == "false" ]]; then
                        echo "⚠️  Failed to install Python 3.8, 3.9, 3.10, or 3.11"
                        echo "⚠️  Checking available Python versions in repository..."
                        apt-cache pkgnames | grep "^python3\.[0-9]\+$" | grep -v "python3-doc" | sort -V | tail -5 || echo "  (unable to list versions)"
                        echo "⚠️  OpenCV Python bindings will be disabled"
                        echo "⚠️  C++ functionality will still work"
                        return 1
                    fi
                    
                    # Install pip for the installed Python version
                    PYTHON_EXEC="python${INSTALLED_PYTHON_VERSION}"
                    if ! "$PYTHON_EXEC" -m pip --version &> /dev/null; then
                        echo "Installing pip for $PYTHON_EXEC..."
                        curl -sS https://bootstrap.pypa.io/get-pip.py | "$PYTHON_EXEC" || {
                            smart_sudo apt-get install -y "python${INSTALLED_PYTHON_VERSION}-pip" || {
                                # Fallback: try to install python3-pip and use it
                                smart_sudo apt-get install -y python3-pip || echo "⚠️  Failed to install pip"
                            }
                        }
                    fi
                else
                    echo "⚠️  Cannot detect OS version for Python installation"
                fi
            fi
            
            # Verify Python version after installation/update
            PYTHON_VERSION=$(python3 --version 2>&1 | awk '{print $2}')
            PYTHON_MAJOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f1)
            PYTHON_MINOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f2)
            echo "Final Python version: $PYTHON_VERSION"
            
            # Install numpy if not available (use the correct Python version)
            echo "Checking numpy installation..."
            PYTHON_TO_USE="python3"
            if command -v python3.11 &> /dev/null; then
                PYTHON_TO_USE="python3.11"
            elif command -v python3.10 &> /dev/null; then
                PYTHON_TO_USE="python3.10"
            fi
            
            if ! "$PYTHON_TO_USE" -c "import numpy" 2>/dev/null; then
                echo "Installing numpy for $PYTHON_TO_USE..."
                # Try pip first (preferred)
                if "$PYTHON_TO_USE" -m pip --version &> /dev/null; then
                    smart_sudo "$PYTHON_TO_USE" -m pip install numpy || {
                        echo "⚠️  Failed to install numpy via pip, trying apt..."
                        # Try version-specific numpy package
                        if [[ "$PYTHON_TO_USE" == "python3.11" ]]; then
                            smart_sudo apt-get install -y python3.11-numpy || smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                        elif [[ "$PYTHON_TO_USE" == "python3.10" ]]; then
                            smart_sudo apt-get install -y python3.10-numpy || smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                        else
                            smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                        fi
                    }
                else
                    echo "⚠️  pip not found for $PYTHON_TO_USE, trying apt..."
                    if [[ "$PYTHON_TO_USE" == "python3.11" ]]; then
                        smart_sudo apt-get install -y python3.11-numpy || smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                    elif [[ "$PYTHON_TO_USE" == "python3.10" ]]; then
                        smart_sudo apt-get install -y python3.10-numpy || smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                    else
                        smart_sudo apt-get install -y python3-numpy || echo "⚠️  Failed to install numpy"
                    fi
                fi
                
                # Verify installation
                if "$PYTHON_TO_USE" -c "import numpy" 2>/dev/null; then
                    echo "✓ numpy installed successfully for $PYTHON_TO_USE"
                else
                    echo "⚠️  numpy installation verification failed"
                fi
            else
                echo "✓ numpy is already installed for $PYTHON_TO_USE"
            fi
            
        else
            echo "⚠️  python3 not found, installing Python 3..."
            smart_sudo apt-get install -y python3 python3-dev python3-pip python3-numpy
        fi
        
        echo "✓ Python environment check completed"
    fi
}

# Install or check build dependencies
install_build_dependencies() {
    echo "Installing build dependencies..."
    
    if [[ "$OS" == "macOS" ]]; then
        # macOS uses Homebrew
        echo "Installing macOS build dependencies with Homebrew..."
        
        # Check if Homebrew is available
        if ! command -v brew >/dev/null 2>&1; then
            echo "❌ Homebrew not found, please install Homebrew first"
            exit 1
        fi
        
        # Install necessary dependency packages
        brew install \
            cmake ninja pkg-config \
            jpeg libpng libtiff \
            eigen tbb \
            python@3.11 numpy || echo "Some packages might already be installed"
            
        # Other dependencies OpenCV might need
        brew install \
            libavformat \
            libavcodec \
            libavdevice \
            libavutil \
            libavfilter \
            libswscale \
            libswresample || echo "Some FFmpeg components might already be installed"
            
        echo "✓ macOS build dependencies installation completed"
        
    else
        # Linux uses apt
        echo "Installing Ubuntu build dependencies with apt..."
        
        # First check and update Python if needed
        check_and_install_python
        
        # First fix possible HDF5/MPI conflicts
        fix_hdf5_mpi_conflicts
        
        smart_sudo apt-get update
        
        # Install basic build tools first
        smart_sudo apt-get install -y \
            build-essential cmake git pkg-config \
            libjpeg-dev libpng-dev libtiff-dev \
            libavcodec-dev libavformat-dev libswscale-dev \
            libv4l-dev libxvidcore-dev libx264-dev \
            libgtk-3-dev \
            libatlas-base-dev gfortran \
            libtbb12 libtbb-dev \
            libhdf5-dev || echo "Some packages might already be installed"
        
        # Install Python development packages (use detected Python version)
        if command -v python3.11 &> /dev/null; then
            echo "Installing Python 3.11 development packages..."
            smart_sudo apt-get install -y python3.11-dev python3.11-distutils || true
        elif command -v python3.10 &> /dev/null; then
            echo "Installing Python 3.10 development packages..."
            smart_sudo apt-get install -y python3.10-dev python3.10-distutils || true
        else
            # Fallback to default python3-dev
            echo "Installing default Python 3 development packages..."
            smart_sudo apt-get install -y python3-dev python3-distutils || true
        fi
        
        # Ensure numpy is installed (may have been installed by check_and_install_python)
        if ! python3 -c "import numpy" 2>/dev/null; then
            echo "Installing numpy..."
            smart_sudo apt-get install -y python3-numpy || {
                # Try pip as fallback
                if command -v pip3 &> /dev/null; then
                    smart_sudo pip3 install numpy || echo "⚠️  Failed to install numpy"
                fi
            }
        fi
        
        echo "✓ Ubuntu build dependencies installation completed"
    fi
}

# Download or check OpenCV source code
setup_opencv_sources() {
    echo ""
    echo "Setting up OpenCV source code..."
    
    # OpenCV main source directory
    OPENCV_SOURCE_DIR="$DEPENDENCIES_DIR/opencv"
    
    # Check if OpenCV main source needs to be downloaded
    if [[ ! -d "$OPENCV_SOURCE_DIR" ]]; then
        echo "Downloading OpenCV 4.x main source code..."
        
        # Clone OpenCV main repository
        if git clone --depth 1 --branch 4.x https://github.com/opencv/opencv.git "$OPENCV_SOURCE_DIR"; then
            echo "✓ OpenCV main source code download completed"
        else
            echo "❌ OpenCV main source code download failed"
            echo "Please check network connection or manually download to: $OPENCV_SOURCE_DIR"
            exit 1
        fi
    else
        echo "✓ Found existing OpenCV main source directory"
        
        # Verify critical Python package files exist | 验证关键Python包文件是否存在
        # If files are missing (e.g., from incomplete push), re-download | 如果文件缺失（例如推送不完整），重新下载
        CRITICAL_PY_FILES=(
            "$OPENCV_SOURCE_DIR/modules/python/package/setup.py"
            "$OPENCV_SOURCE_DIR/modules/python/package/cv2/__init__.py"
            "$OPENCV_SOURCE_DIR/modules/python/package/cv2/load_config_py3.py"
        )
        
        MISSING_FILES=0
        for py_file in "${CRITICAL_PY_FILES[@]}"; do
            if [[ ! -f "$py_file" ]]; then
                echo "⚠️  Missing critical file: $(basename $py_file)"
                ((MISSING_FILES++))
            fi
        done
        
        if [[ $MISSING_FILES -gt 0 ]]; then
            echo "⚠️  OpenCV source directory is incomplete (missing $MISSING_FILES critical Python files)"
            echo "⚠️  This may have been caused by an incomplete push from macOS"
            echo "Re-downloading OpenCV source code..."
            echo "重新下载 OpenCV 源码..."
            
            # Backup build directory if exists
            if [[ -d "$OPENCV_SOURCE_DIR/build_local" ]]; then
                echo "Backing up existing build directory..."
                mv "$OPENCV_SOURCE_DIR/build_local" "$OPENCV_SOURCE_DIR/build_local.backup.$(date +%Y%m%d_%H%M%S)" 2>/dev/null || true
            fi
            
            # Remove incomplete directory
            rm -rf "$OPENCV_SOURCE_DIR"
            
            # Re-clone OpenCV
            if git clone --depth 1 --branch 4.x https://github.com/opencv/opencv.git "$OPENCV_SOURCE_DIR"; then
                echo "✓ OpenCV main source code re-downloaded successfully"
                echo "✓ OpenCV 主源码重新下载成功"
            else
                echo "❌ OpenCV main source code re-download failed"
                echo "❌ OpenCV 主源码重新下载失败"
                echo "Please check network connection or manually download to: $OPENCV_SOURCE_DIR"
                exit 1
            fi
        else
            # Check if update is needed (only if git repo is valid)
        cd "$OPENCV_SOURCE_DIR"
        echo "Current OpenCV version information:"
            git log --oneline -1 2>/dev/null || echo "Unable to get version information (not a git repo, but files are complete)"
        cd "$SCRIPT_DIR"
        fi
    fi
    
    # Display version information
    echo ""
    echo "Source directory information:"
    echo "- OpenCV main source: $OPENCV_SOURCE_DIR"
    echo "- Building standard modules only (no contrib extensions)"
    echo ""
}

# Compile and install OpenCV
build_opencv() {
    echo "Compiling OpenCV..."
    
    # Build directory
    BUILD_DIR="$OPENCV_SOURCE_DIR/build_local"
    INSTALL_DIR="$OPENCV_SOURCE_DIR/install_local"
    
    # Check if OpenCV is already built | 检查OpenCV是否已经构建
    OPENCV_CONFIG_FILE="$INSTALL_DIR/lib/cmake/opencv4/OpenCVConfig.cmake"
    if [ -f "$OPENCV_CONFIG_FILE" ]; then
        echo ""
        echo "✅ Found existing OpenCV installation at: $INSTALL_DIR"
        echo "✅ 发现已存在的OpenCV安装: $INSTALL_DIR"
        echo "Config file: $OPENCV_CONFIG_FILE"
        echo ""
        
        # Try to detect OpenCV version
        if [ -f "$INSTALL_DIR/include/opencv4/opencv2/core/version.hpp" ]; then
            OPENCV_VERSION=$(grep "#define CV_VERSION_MAJOR" "$INSTALL_DIR/include/opencv4/opencv2/core/version.hpp" | awk '{print $3}')
            OPENCV_VERSION_MINOR=$(grep "#define CV_VERSION_MINOR" "$INSTALL_DIR/include/opencv4/opencv2/core/version.hpp" | awk '{print $3}')
            OPENCV_VERSION_REVISION=$(grep "#define CV_VERSION_REVISION" "$INSTALL_DIR/include/opencv4/opencv2/core/version.hpp" | awk '{print $3}')
            echo "Installed version: $OPENCV_VERSION.$OPENCV_VERSION_MINOR.$OPENCV_VERSION_REVISION"
            echo "已安装版本: $OPENCV_VERSION.$OPENCV_VERSION_MINOR.$OPENCV_VERSION_REVISION"
        fi
        
        echo ""
        echo "Do you want to skip this installation and use the existing build? [Y/n]"
        echo "是否跳过此次安装并使用现有构建？[Y/n]"
        read -p "Choice (default: Y): " skip_choice
        
        # Default to Y if no input | 默认选择Y
        skip_choice=${skip_choice:-Y}
        
        if [[ "$skip_choice" =~ ^[Yy]$ ]] || [[ -z "$skip_choice" ]]; then
            echo "✅ Skipping OpenCV build - using existing installation"
            echo "✅ 跳过OpenCV构建 - 使用现有安装"
            echo "OpenCV installation location: $INSTALL_DIR"
            echo "OpenCV安装位置: $INSTALL_DIR"
            return 0
        else
            echo "🔄 Proceeding with fresh OpenCV build..."
            echo "🔄 继续进行新的OpenCV构建..."
        fi
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Clean previous build
    echo "Cleaning previous build files..."
    rm -rf ./*
    rm -rf CMakeCache.txt CMakeFiles
    
    # ============================================================================
    # Clean environment to avoid Anaconda pollution | 清理环境以避免Anaconda污染
    # ============================================================================
    echo ""
    echo "Cleaning environment to avoid Anaconda/Conda interference..."
    echo "清理环境以避免 Anaconda/Conda 干扰..."
    
    # Save original environment variables | 保存原始环境变量
    ORIGINAL_PATH="${PATH}"
    ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
    ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    ORIGINAL_PKG_CONFIG_PATH="${PKG_CONFIG_PATH:-}"
    
    # Remove Anaconda from PATH | 从PATH中移除Anaconda
    if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
        echo "  Removing Anaconda/Conda from PATH..."
        echo "  从 PATH 中移除 Anaconda/Conda..."
        # Remove anaconda and conda paths | 移除anaconda和conda路径
        CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        export PATH="${CLEAN_PATH}"
        echo "  ✓ PATH cleaned (Anaconda paths removed)"
    fi
    
    # Remove Anaconda from LD_LIBRARY_PATH | 从LD_LIBRARY_PATH中移除Anaconda
    if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
        echo "  Removing Anaconda from LD_LIBRARY_PATH..."
        echo "  从 LD_LIBRARY_PATH 中移除 Anaconda..."
        CLEAN_LD_LIBRARY_PATH=$(echo "${LD_LIBRARY_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        if [ -n "${CLEAN_LD_LIBRARY_PATH}" ]; then
            export LD_LIBRARY_PATH="${CLEAN_LD_LIBRARY_PATH}"
        else
            unset LD_LIBRARY_PATH
        fi
        echo "  ✓ LD_LIBRARY_PATH cleaned (Anaconda paths removed)"
    fi
    
    # Clean CMAKE_PREFIX_PATH to exclude Anaconda | 清理CMAKE_PREFIX_PATH以排除Anaconda
    if [ -n "${CMAKE_PREFIX_PATH}" ]; then
        CLEAN_CMAKE_PREFIX_PATH=$(echo "${CMAKE_PREFIX_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        if [ -n "${CLEAN_CMAKE_PREFIX_PATH}" ] && [ "${CLEAN_CMAKE_PREFIX_PATH}" != "${CMAKE_PREFIX_PATH}" ]; then
            echo "  Removing Anaconda from CMAKE_PREFIX_PATH..."
            echo "  从 CMAKE_PREFIX_PATH 中移除 Anaconda..."
            export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
            echo "  ✓ CMAKE_PREFIX_PATH cleaned (Anaconda paths removed)"
        fi
    fi
    
    # Clean PKG_CONFIG_PATH to exclude Anaconda | 清理PKG_CONFIG_PATH以排除Anaconda
    if [ -n "${PKG_CONFIG_PATH}" ]; then
        CLEAN_PKG_CONFIG_PATH=$(echo "${PKG_CONFIG_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        if [ -n "${CLEAN_PKG_CONFIG_PATH}" ] && [ "${CLEAN_PKG_CONFIG_PATH}" != "${PKG_CONFIG_PATH}" ]; then
            echo "  Removing Anaconda from PKG_CONFIG_PATH..."
            echo "  从 PKG_CONFIG_PATH 中移除 Anaconda..."
            export PKG_CONFIG_PATH="${CLEAN_PKG_CONFIG_PATH}"
            echo "  ✓ PKG_CONFIG_PATH cleaned (Anaconda paths removed)"
        fi
    fi
    
    # Unset CONDA_PREFIX if set (prevents conda activation scripts from interfering)
    # 如果设置了CONDA_PREFIX则取消设置（防止conda激活脚本干扰）
    if [ -n "${CONDA_PREFIX}" ]; then
        echo "  Unsetting CONDA_PREFIX: ${CONDA_PREFIX}"
        echo "  取消设置 CONDA_PREFIX: ${CONDA_PREFIX}"
        unset CONDA_PREFIX
        echo "  ✓ CONDA_PREFIX unset"
    fi
    
    echo "✓ Environment cleaning completed"
    echo "✓ 环境清理完成"
    echo ""
    
    # Set environment variables (using cleaned paths, no Anaconda)
    # 设置环境变量（使用清理后的路径，无 Anaconda）
    if [[ "$OS" == "macOS" ]]; then
        # macOS environment configuration
        # Build clean CMAKE_PREFIX_PATH (exclude Anaconda)
        # 构建干净的 CMAKE_PREFIX_PATH（排除 Anaconda）
        CLEAN_CMAKE_PREFIX="/opt/homebrew:/usr/local"
        if [ -n "${CMAKE_PREFIX_PATH}" ]; then
            CLEAN_CMAKE_PREFIX="${CLEAN_CMAKE_PREFIX}:${CMAKE_PREFIX_PATH}"
        fi
        export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX}"
        
        # Build clean PKG_CONFIG_PATH (exclude Anaconda)
        # 构建干净的 PKG_CONFIG_PATH（排除 Anaconda）
        CLEAN_PKG_CONFIG="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig"
        if [ -n "${PKG_CONFIG_PATH}" ]; then
            CLEAN_PKG_CONFIG="${CLEAN_PKG_CONFIG}:${PKG_CONFIG_PATH}"
        fi
        export PKG_CONFIG_PATH="${CLEAN_PKG_CONFIG}"
        
        # Python path (ensure not from Anaconda)
        # Python 路径（确保不是来自 Anaconda）
        PYTHON3_EXECUTABLE=$(which python3 2>/dev/null)
        if [[ -z "$PYTHON3_EXECUTABLE" ]] || [[ "$PYTHON3_EXECUTABLE" == *"anaconda"* ]] || [[ "$PYTHON3_EXECUTABLE" == *"conda"* ]]; then
            # Fallback to Homebrew Python
            PYTHON3_EXECUTABLE="/opt/homebrew/bin/python3"
            if [[ ! -f "$PYTHON3_EXECUTABLE" ]]; then
                PYTHON3_EXECUTABLE="/usr/local/bin/python3"
            fi
        fi
        
        # Verify Python is not from Anaconda
        if [[ -n "$PYTHON3_EXECUTABLE" ]] && [[ "$PYTHON3_EXECUTABLE" == *"anaconda"* ]]; then
            echo "⚠️  Warning: Python executable is from Anaconda: $PYTHON3_EXECUTABLE"
            echo "⚠️  警告：Python 可执行文件来自 Anaconda: $PYTHON3_EXECUTABLE"
            echo "   Using system Python instead..."
            echo "   改用系统 Python..."
            PYTHON3_EXECUTABLE="/usr/bin/python3"
        fi
    else
        # Linux environment configuration
        # Try to find the best Python 3 version (prefer 3.11, then 3.10, then default)
        # Ensure Python is NOT from Anaconda
        # 尝试找到最佳 Python 3 版本（优先 3.11，然后 3.10，然后默认）
        # 确保 Python 不是来自 Anaconda
        PYTHON3_EXECUTABLE=""
        
        # Check python3.11 first (not from Anaconda)
        if command -v python3.11 &> /dev/null; then
            CANDIDATE_PYTHON=$(which python3.11 2>/dev/null)
            if [[ -n "$CANDIDATE_PYTHON" ]] && [[ "$CANDIDATE_PYTHON" != *"anaconda"* ]] && [[ "$CANDIDATE_PYTHON" != *"conda"* ]]; then
                PYTHON3_EXECUTABLE="$CANDIDATE_PYTHON"
            echo "Using Python 3.11: $PYTHON3_EXECUTABLE"
            fi
        fi
        
        # Check python3.10 if 3.11 not found
        if [[ -z "$PYTHON3_EXECUTABLE" ]]; then
            if command -v python3.10 &> /dev/null; then
                CANDIDATE_PYTHON=$(which python3.10 2>/dev/null)
                if [[ -n "$CANDIDATE_PYTHON" ]] && [[ "$CANDIDATE_PYTHON" != *"anaconda"* ]] && [[ "$CANDIDATE_PYTHON" != *"conda"* ]]; then
                    PYTHON3_EXECUTABLE="$CANDIDATE_PYTHON"
            echo "Using Python 3.10: $PYTHON3_EXECUTABLE"
                fi
            fi
        fi
        
        # Check default python3 if 3.11 and 3.10 not found
        if [[ -z "$PYTHON3_EXECUTABLE" ]]; then
            if command -v python3 &> /dev/null; then
                CANDIDATE_PYTHON=$(which python3 2>/dev/null)
                if [[ -n "$CANDIDATE_PYTHON" ]] && [[ "$CANDIDATE_PYTHON" != *"anaconda"* ]] && [[ "$CANDIDATE_PYTHON" != *"conda"* ]]; then
                    PYTHON3_EXECUTABLE="$CANDIDATE_PYTHON"
            echo "Using default python3: $PYTHON3_EXECUTABLE"
                else
                    echo "⚠️  Warning: Default python3 is from Anaconda, will disable Python bindings"
                    echo "⚠️  警告：默认 python3 来自 Anaconda，将禁用 Python 绑定"
                    PYTHON3_EXECUTABLE=""
                fi
            fi
        fi
        
        # Fallback to system Python paths if still not found
        if [[ -z "$PYTHON3_EXECUTABLE" ]]; then
            if [[ -f "/usr/bin/python3.11" ]]; then
                PYTHON3_EXECUTABLE="/usr/bin/python3.11"
                echo "Using system Python 3.11: $PYTHON3_EXECUTABLE"
            elif [[ -f "/usr/bin/python3.10" ]]; then
                PYTHON3_EXECUTABLE="/usr/bin/python3.10"
                echo "Using system Python 3.10: $PYTHON3_EXECUTABLE"
            elif [[ -f "/usr/bin/python3" ]]; then
                PYTHON3_EXECUTABLE="/usr/bin/python3"
                echo "Using system python3: $PYTHON3_EXECUTABLE"
            fi
        fi
        
        # Avoid HDF5 OpenMPI version issues
        export HDF5_USE_MPI=OFF
        export HDF5_PREFER_SERIAL=ON
    fi
    
    # Check Python environment before enabling Python support
    # Python bindings are optional for po_core (C++ library)
    PYTHON_SUPPORT_ENABLED="OFF"
    if [[ "$OS" == "macOS" ]]; then
        # macOS: Check for Python 3 with numpy
        if [[ -n "$PYTHON3_EXECUTABLE" ]]; then
            if python3 -c "import numpy" 2>/dev/null; then
                PYTHON_SUPPORT_ENABLED="ON"
                echo "✓ Python 3 environment detected with numpy - enabling Python bindings"
            else
                echo "⚠️  Python 3 detected but numpy not available - disabling Python bindings"
            fi
        fi
    else
        # Linux: Check for Python 3.8+ with numpy
        # Use the detected Python executable instead of system python3
        if [[ -n "$PYTHON3_EXECUTABLE" ]] && [[ -f "$PYTHON3_EXECUTABLE" ]]; then
            PYTHON_VERSION=$("$PYTHON3_EXECUTABLE" --version 2>&1 | awk '{print $2}')
            PYTHON_MAJOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f1)
            PYTHON_MINOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f2)
            
            echo "Detected Python version: $PYTHON_VERSION (from $PYTHON3_EXECUTABLE)"
            
            # Verify Python is not from Anaconda (double-check)
            # 验证 Python 不是来自 Anaconda（再次检查）
            if [[ "$PYTHON3_EXECUTABLE" == *"anaconda"* ]] || [[ "$PYTHON3_EXECUTABLE" == *"conda"* ]]; then
                echo "⚠️  Warning: Python executable is from Anaconda: $PYTHON3_EXECUTABLE"
                echo "⚠️  警告：Python 可执行文件来自 Anaconda: $PYTHON3_EXECUTABLE"
                echo "   Disabling Python bindings to avoid library conflicts"
                echo "   禁用 Python 绑定以避免库冲突"
                PYTHON_SUPPORT_ENABLED="OFF"
                PYTHON3_EXECUTABLE=""
            elif [[ "$PYTHON_MAJOR" -ge 3 ]] && [[ "$PYTHON_MINOR" -ge 8 ]]; then
                if "$PYTHON3_EXECUTABLE" -c "import numpy" 2>/dev/null; then
                    PYTHON_SUPPORT_ENABLED="ON"
                    echo "✓ Python 3.8+ environment detected with numpy - enabling Python bindings"
                    echo "   Python: $PYTHON3_EXECUTABLE ($PYTHON_VERSION)"
                    echo "   ✓ Verified: Python is NOT from Anaconda"
                else
                    echo "⚠️  Python 3.8+ detected but numpy not available - disabling Python bindings"
                    echo "   Attempting to install numpy for $PYTHON3_EXECUTABLE..."
                    # Use system pip, not Anaconda pip
                    # 使用系统 pip，而非 Anaconda pip
                    SYSTEM_PIP3=""
                    if [[ -f "/usr/bin/pip3" ]]; then
                        SYSTEM_PIP3="/usr/bin/pip3"
                    elif command -v pip3 &> /dev/null && [[ "$(which pip3 2>/dev/null)" != *"anaconda"* ]]; then
                        SYSTEM_PIP3=$(which pip3)
                    fi
                    
                    if [[ -n "$SYSTEM_PIP3" ]]; then
                        echo "   Using system pip3: $SYSTEM_PIP3"
                        "$PYTHON3_EXECUTABLE" -m pip install numpy --user || sudo "$PYTHON3_EXECUTABLE" -m pip install numpy || echo "⚠️  Failed to install numpy"
                    else
                        echo "⚠️  pip3 not found or is from Anaconda, skipping numpy installation"
                    fi
                    
                    # Re-check after installation attempt
                    if "$PYTHON3_EXECUTABLE" -c "import numpy" 2>/dev/null; then
                        PYTHON_SUPPORT_ENABLED="ON"
                        echo "✓ numpy installed successfully - enabling Python bindings"
                    fi
                fi
            else
                echo "⚠️  Python 3 version too old ($PYTHON_VERSION < 3.8) - disabling Python bindings"
                echo "   OpenCV requires Python 3.8+ with numpy for Python bindings"
                echo "   The script should have upgraded Python automatically. Please check installation logs."
            fi
        else
            echo "⚠️  Python 3 executable not found or invalid - disabling Python bindings"
            echo "   Path: $PYTHON3_EXECUTABLE"
            PYTHON3_EXECUTABLE=""
        fi
    fi
    
    # CMake configuration options
    CMAKE_OPTIONS=(
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR"
        
        # Python support (conditional)
        "-DBUILD_opencv_python3=$PYTHON_SUPPORT_ENABLED"
    )
    
    # Only set Python executable if support is enabled
    if [[ "$PYTHON_SUPPORT_ENABLED" == "ON" ]]; then
        CMAKE_OPTIONS+=("-DPYTHON3_EXECUTABLE=$PYTHON3_EXECUTABLE")
        echo "   Using Python: $PYTHON3_EXECUTABLE"
    else
        echo "   Python bindings disabled (not required for po_core C++ library)"
    fi
    
    CMAKE_OPTIONS+=(
        # Core functionality
        "-DWITH_TBB=ON"
        "-DWITH_EIGEN=ON"
        
        # Image format support
        "-DWITH_JPEG=ON"
        "-DWITH_PNG=ON"
        "-DWITH_TIFF=ON"
        
        # Video support (optional, check if available)
        "-DWITH_V4L=ON"
        
        # Disable some unnecessary components to speed up compilation
        "-DBUILD_TESTS=OFF"
        "-DBUILD_PERF_TESTS=OFF"
        "-DBUILD_EXAMPLES=OFF"
        "-DBUILD_opencv_apps=OFF"
        
        # HDF5 configuration - explicitly use serial version to avoid OpenMPI conflicts
        "-DWITH_HDF5=ON"
        "-DHDF5_USE_STATIC_LIBRARIES=OFF"
        
        # IPPICV: Disable if download fails (network issue)
        # This is an optional performance optimization
        "-DWITH_IPP=OFF"
        "-DBUILD_IPP_IW=OFF"
        
        # Installation path configuration
        "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON"
        
        # CMake policies to avoid warnings and ensure correct Boost/package finding
        # CMake 策略以避免警告并确保正确的 Boost/包查找
        "-DCMAKE_POLICY_DEFAULT_CMP0144=OLD"  # CGAL compatibility: allow BOOST_ROOT
        "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"  # CGAL compatibility: use old Boost finding
        "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"  # Allow find_package to use Boost_ROOT variable
    )
    
    # Check FFMPEG availability (optional for po_core)
    # FFMPEG is only needed for video encoding/decoding
    # Use cleaned PKG_CONFIG_PATH to avoid Anaconda's FFMPEG
    # 使用清理后的 PKG_CONFIG_PATH 以避免 Anaconda 的 FFMPEG
    if pkg-config --exists libavcodec libavformat libavutil libswscale 2>/dev/null; then
        # Verify FFMPEG is from system, not Anaconda
        # 验证 FFMPEG 来自系统，而非 Anaconda
        FFMPEG_PREFIX=$(pkg-config --variable=prefix libavcodec 2>/dev/null || echo "")
        if [[ -n "$FFMPEG_PREFIX" ]] && [[ "$FFMPEG_PREFIX" != *"anaconda"* ]] && [[ "$FFMPEG_PREFIX" != *"conda"* ]]; then
        CMAKE_OPTIONS+=("-DWITH_FFMPEG=ON")
            echo "✓ FFMPEG libraries detected (system version) - enabling video support"
        else
            CMAKE_OPTIONS+=("-DWITH_FFMPEG=OFF")
            echo "⚠️  FFMPEG found but may be from Anaconda - disabling video support"
        fi
    else
        CMAKE_OPTIONS+=("-DWITH_FFMPEG=OFF")
        echo "⚠️  FFMPEG not found - disabling video support (image processing still works)"
    fi
    
    # Linux-specific HDF5 configuration
    if [[ "$OS" == "Linux" ]]; then
        # Explicitly specify HDF5 serial version paths
        # Use cleaned PKG_CONFIG_PATH to ensure system HDF5 is found
        # 使用清理后的 PKG_CONFIG_PATH 以确保找到系统 HDF5
        if pkg-config --exists hdf5; then
            HDF5_INCLUDE_DIR=$(pkg-config --variable=includedir hdf5)
            HDF5_LIBRARY_DIR=$(pkg-config --variable=libdir hdf5)
            
            # Verify HDF5 is from system, not Anaconda
            # 验证 HDF5 来自系统，而非 Anaconda
            if [[ "$HDF5_INCLUDE_DIR" == *"anaconda"* ]] || [[ "$HDF5_INCLUDE_DIR" == *"conda"* ]]; then
                echo "⚠️  Warning: HDF5 found from Anaconda, using system paths instead"
                echo "⚠️  警告：HDF5 来自 Anaconda，改用系统路径"
                HDF5_INCLUDE_DIR="/usr/include/hdf5/serial"
                HDF5_LIBRARY_DIR="/usr/lib/x86_64-linux-gnu/hdf5/serial"
                if [[ ! -d "$HDF5_INCLUDE_DIR" ]]; then
                    HDF5_INCLUDE_DIR="/usr/include"
                    HDF5_LIBRARY_DIR="/usr/lib/x86_64-linux-gnu"
                fi
            fi
            
            CMAKE_OPTIONS+=(
                "-DHDF5_ROOT=/usr"
                "-DHDF5_INCLUDE_DIRS=$HDF5_INCLUDE_DIR"
                "-DHDF5_LIBRARIES=$HDF5_LIBRARY_DIR"
                "-DHDF5_IS_PARALLEL=OFF"
                "-DHDF5_PREFER_SERIAL=ON"
            )
            echo "✓ Using serial version of HDF5: $HDF5_INCLUDE_DIR"
        else
            echo "⚠️  HDF5 pkg-config not found, disabling HDF5 support"
            CMAKE_OPTIONS+=(
                "-DWITH_HDF5=OFF"
            )
        fi
    fi
    
    # macOS-specific configuration
    if [[ "$OS" == "macOS" ]]; then
        CMAKE_OPTIONS+=(
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
            "-DWITH_QT=OFF"  # Avoid Qt dependency conflicts
            "-DWITH_COCOA=ON"
        )
        
        if [[ "$ARCH" == "arm64" ]]; then
            CMAKE_OPTIONS+=(
                "-DCMAKE_OSX_ARCHITECTURES=arm64"
            )
        fi
    else
        # Linux-specific configuration
        CMAKE_OPTIONS+=(
            "-DWITH_GTK=ON"
            "-DWITH_QT=OFF"
        )
    fi
    
    echo ""
    echo "CMake configuration options:"
    printf '%s\n' "${CMAKE_OPTIONS[@]}"
    echo ""
    
    # Run CMake configuration
    echo "Running CMake configuration..."
    if cmake .. "${CMAKE_OPTIONS[@]}"; then
        echo "✓ CMake configuration successful"
    else
        echo "❌ CMake configuration failed"
        echo "Please check error messages and dependencies"
        echo ""
        echo "Common solutions:"
        echo "1. If HDF5/MPI related errors occur, try completely disabling HDF5:"
        echo "   Add -DWITH_HDF5=OFF to CMAKE_OPTIONS"
        echo "2. Check if all necessary development packages are installed"
        echo "3. Clean build directory and retry: rm -rf * && cmake .."
        exit 1
    fi
    
    # Detect CPU core count
    if [[ "$OS" == "macOS" ]]; then
        NPROC=$(sysctl -n hw.ncpu)
    else
        NPROC=$(nproc)
    fi
    
    # Limit parallel compilation to avoid memory exhaustion
    if [[ "$NPROC" -gt 4 ]]; then
        COMPILE_JOBS=4
    else
        COMPILE_JOBS="$NPROC"
    fi
    
    echo ""
    echo "Starting OpenCV compilation (using $COMPILE_JOBS parallel jobs, system has $NPROC cores)..."
    echo "Note: Compilation may take 30-60 minutes, please be patient..."
    echo ""
    
    # Compile
    if make -j"$COMPILE_JOBS"; then
        echo "✓ OpenCV compilation successful"
    else
        echo "❌ OpenCV compilation failed"
        echo ""
        echo "Possible solutions:"
        echo "1. Check if compiler and dependencies are correctly installed"
        echo "2. Reduce parallel jobs: make -j1" 
        echo "3. Check specific error messages"
        echo "4. Try cleaning and recompiling: rm -rf * && cmake .. && make"
        echo "5. If memory insufficient, increase swap space or reduce parallel jobs"
        exit 1
    fi
    
    # Install
    echo ""
    echo "Installing OpenCV to $INSTALL_DIR..."
    if make install; then
        echo "✓ OpenCV installation successful"
    else
        echo "❌ OpenCV installation failed"
        exit 1
    fi
    
    # Return to script directory
    cd "$SCRIPT_DIR"
    
    echo ""
    echo "OpenCV compilation and installation completed!"
    echo "Installation location: $INSTALL_DIR"
}

# Test OpenCV installation
test_opencv_installation() {
    echo ""
    echo "Testing OpenCV installation..."
    
    # Test header files
    INSTALL_DIR="$OPENCV_SOURCE_DIR/install_local"
    OPENCV_HEADER="$INSTALL_DIR/include/opencv4/opencv2/opencv.hpp"
    
    if [[ -f "$OPENCV_HEADER" ]]; then
        echo "✓ Found OpenCV header file: $OPENCV_HEADER"
    else
        echo "❌ OpenCV header file not found"
    fi
    
    # Test library files
    echo "Searching for OpenCV library files..."
    if [[ "$OS" == "macOS" ]]; then
        LIB_PATTERN="*.dylib"
    else
        LIB_PATTERN="*.so*"
    fi
    
    OPENCV_LIBS=$(find "$INSTALL_DIR/lib" -name "libopencv_*$LIB_PATTERN" 2>/dev/null | wc -l)
    if [[ "$OPENCV_LIBS" -gt 0 ]]; then
        echo "✓ Found $OPENCV_LIBS OpenCV library files"
    else
        echo "❌ OpenCV library files not found"
    fi
    
    # Test pkg-config
    export PKG_CONFIG_PATH="$INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"
    if pkg-config --exists opencv4; then
        echo "✓ pkg-config configured correctly"
        OPENCV_VERSION=$(pkg-config --modversion opencv4)
        echo "  OpenCV version: $OPENCV_VERSION"
    else
        echo "⚠️  pkg-config configuration may have issues"
    fi
    
    # Create simple test program
    echo ""
    echo "Creating C++ test program..."
    TEST_FILE="$INSTALL_DIR/test_opencv.cpp"
    cat > "$TEST_FILE" << 'EOF'
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    
    // Test basic matrix operations
    try {
        cv::Mat mat = cv::Mat::eye(3, 3, CV_64F);
        std::cout << "✓ Created 3x3 identity matrix successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Matrix creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test ORB feature detector (part of standard OpenCV)
    try {
        auto orb = cv::ORB::create();
        std::cout << "✓ ORB detector created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ ORB detector creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✓ OpenCV standard modules test passed!" << std::endl;
    return 0;
}
EOF

    # Compile test program
    echo "Compiling test program..."
    TEST_EXEC="$INSTALL_DIR/test_opencv"
    
    # Compilation command
    if [[ "$OS" == "macOS" ]]; then
        COMPILE_CMD="clang++ -std=c++11 -I$INSTALL_DIR/include/opencv4 -L$INSTALL_DIR/lib -lopencv_core -lopencv_imgproc -lopencv_features2d $TEST_FILE -o $TEST_EXEC"
    else
        COMPILE_CMD="g++ -std=c++11 -I$INSTALL_DIR/include/opencv4 -L$INSTALL_DIR/lib -lopencv_core -lopencv_imgproc -lopencv_features2d $TEST_FILE -o $TEST_EXEC"
    fi
    
    echo "Compilation command: $COMPILE_CMD"
    if eval "$COMPILE_CMD"; then
        echo "✓ Test program compiled successfully"
        
        # Run test
        echo "Running test program..."
        export LD_LIBRARY_PATH="$INSTALL_DIR/lib:$LD_LIBRARY_PATH"
        export DYLD_LIBRARY_PATH="$INSTALL_DIR/lib:$DYLD_LIBRARY_PATH"  # macOS
        
        if "$TEST_EXEC"; then
            echo "✓ OpenCV standard modules functionality test passed!"
        else
            echo "⚠️  Test program execution failed, but library files exist"
        fi
        
        # Clean up test files
        rm -f "$TEST_FILE" "$TEST_EXEC"
    else
        echo "⚠️  Test program compilation failed, may need to adjust linking parameters"
    fi
    
    echo ""
}

# Generate usage information
generate_usage_info() {
    echo ""
    echo "========================================"
    echo "  OpenCV Installation Completed!"
    echo "========================================"
    echo ""
    
    INSTALL_DIR="$OPENCV_SOURCE_DIR/install_local"
    
    echo "Installation information:"
    echo "- Installation location: $INSTALL_DIR"
    echo "- Header files: $INSTALL_DIR/include/opencv4/"
    echo "- Library files: $INSTALL_DIR/lib/"
    echo "- pkg-config: $INSTALL_DIR/lib/pkgconfig/"
    echo ""
    
    echo "Usage instructions:"
    echo ""
    
    echo "1. CMake project configuration:"
    echo "   Add to CMakeLists.txt:"
    echo "   set(CMAKE_PREFIX_PATH \"$INSTALL_DIR\" \${CMAKE_PREFIX_PATH})"
    echo "   find_package(OpenCV REQUIRED)"
    echo "   target_link_libraries(your_target \${OpenCV_LIBS})"
    echo ""
    
    echo "2. pkg-config method:"
    echo "   export PKG_CONFIG_PATH=\"$INSTALL_DIR/lib/pkgconfig:\$PKG_CONFIG_PATH\""
    echo "   pkg-config --cflags --libs opencv4"
    echo ""
    
    echo "3. Direct compilation:"
    if [[ "$OS" == "macOS" ]]; then
        echo "   clang++ -std=c++11 \\"
    else
        echo "   g++ -std=c++11 \\"
    fi
    echo "     -I$INSTALL_DIR/include/opencv4 \\"
    echo "     -L$INSTALL_DIR/lib \\"
    echo "     -lopencv_core -lopencv_imgproc -lopencv_features2d \\"
    echo "     your_code.cpp -o your_program"
    echo ""
    
    echo "4. Environment variable setup:"
    echo "   export LD_LIBRARY_PATH=\"$INSTALL_DIR/lib:\$LD_LIBRARY_PATH\""
    if [[ "$OS" == "macOS" ]]; then
        echo "   export DYLD_LIBRARY_PATH=\"$INSTALL_DIR/lib:\$DYLD_LIBRARY_PATH\""
    fi
    echo "   export PKG_CONFIG_PATH=\"$INSTALL_DIR/lib/pkgconfig:\$PKG_CONFIG_PATH\""
    echo ""
    
    echo "5. Using OpenCV in code:"
    echo "   #include <opencv2/opencv.hpp>"
    echo ""
    echo "   // Standard feature detectors"
    echo "   auto orb = cv::ORB::create();"
    echo "   auto akaze = cv::AKAZE::create();"
    echo ""
    
    echo "Important notes:"
    echo "- This version includes standard OpenCV modules only"
    echo "- Does NOT include opencv_contrib extensions (xfeatures2d, etc.)"
    echo "- For SURF/SIFT, you need to install opencv_contrib separately"
    echo "- Can coexist with system-installed OpenCV"
    echo "- When compiling projects, ensure correct header and library paths are used"
    echo "- HDF5/MPI conflict issues have been fixed"
    echo ""
}

# Main function
main() {
    echo "Starting OpenCV installation..."
    echo ""
    
    detect_os
    check_system_dependencies
    install_build_dependencies
    setup_opencv_sources
    build_opencv
    test_opencv_installation
    generate_usage_info
    
    echo "OpenCV installation script completed!"
    echo ""
    echo "If you encounter problems during compilation, please check error messages and:"
    echo "1. Check if system dependencies are completely installed"
    echo "2. Check network connection (for downloading source code)"
    echo "3. Ensure sufficient disk space (at least 3GB required)"
    echo "4. If memory insufficient, reduce parallel compilation: make -j1"
    echo "5. HDF5 related issues have been automatically handled by the script"
}

# Run main function
main "$@"
