#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Script to build and install MAGSAC locally within the dependencies folder.
# This script handles OpenMP installation for macOS (via Homebrew) and common Linux distros.

echo "--- MAGSAC Local Build and Install Script ---"

# Determine the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: ${SCRIPT_DIR}"

MAGSAC_SRC_DIR="${SCRIPT_DIR}/magsac-master"
MAGSAC_BUILD_DIR="${MAGSAC_SRC_DIR}/build_local" # Build directory inside magsac-master
MAGSAC_INSTALL_DIR="${MAGSAC_SRC_DIR}/install_local" # Local install directory inside magsac-master

# Check if MAGSAC source directory exists
if [ ! -d "${MAGSAC_SRC_DIR}" ]; then
    echo "Error: MAGSAC source directory not found at ${MAGSAC_SRC_DIR}"
    echo "Please download or clone the MAGSAC repository into that location first."
    exit 1
fi

if [ ! -f "${MAGSAC_SRC_DIR}/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in ${MAGSAC_SRC_DIR}. Is it a valid MAGSAC source directory?"
    exit 1
fi

echo "Source directory: ${MAGSAC_SRC_DIR}"
echo "Build directory: ${MAGSAC_BUILD_DIR}"
echo "Install directory: ${MAGSAC_INSTALL_DIR}"

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

# Check for existing installation and ask user | 检查现有安装并询问用户
if [ -d "${MAGSAC_INSTALL_DIR}" ] && [ "$(find "${MAGSAC_INSTALL_DIR}" -name "*.so" -o -name "*.dylib" -o -name "*.a" 2>/dev/null | head -1)" ]; then
    print_warning "Found existing MAGSAC installation at: ${MAGSAC_INSTALL_DIR}"
    print_warning "发现现有MAGSAC安装：${MAGSAC_INSTALL_DIR}"

    # List some key files to show what's installed
    echo "Existing installation contains:"
    find "${MAGSAC_INSTALL_DIR}" -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.a" \) | head -3 | while read file; do
        echo "  - $(basename "$file")"
    done

    read -p "Skip rebuild and use existing MAGSAC installation? [Y/n]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_info "Using existing MAGSAC installation"
        print_info "使用现有MAGSAC安装"
        print_success "MAGSAC is already available at: ${MAGSAC_INSTALL_DIR}"
        exit 0
    else
        print_info "Proceeding with rebuild..."
        print_info "继续重新构建..."
    fi
fi

# --- OpenMP Dependency Check and Setup ---
echo "--- Checking for OpenMP support ---"

# Array to hold CMake arguments
declare -a CMAKE_ARGS

# Set up local OpenCV path | 设置本地OpenCV路径
# Priority: PoSDK local OpenCV > po_core OpenCV
# 优先级：PoSDK 本地 OpenCV > po_core OpenCV
LOCAL_OPENCV_DIR="${SCRIPT_DIR}/opencv/install_local"

# If PoSDK local OpenCV not found, try po_core's OpenCV
# 如果 PoSDK 本地没有 OpenCV，尝试使用 po_core 的 OpenCV
if [ ! -d "${LOCAL_OPENCV_DIR}" ] || [ ! -f "${LOCAL_OPENCV_DIR}/lib/cmake/opencv4/OpenCVConfig.cmake" ]; then
    PO_CORE_OPENCV_DIR="${SCRIPT_DIR}/../po_core_lib/dependencies/opencv/install_local"
    if [ -d "${PO_CORE_OPENCV_DIR}" ] && [ -f "${PO_CORE_OPENCV_DIR}/lib/cmake/opencv4/OpenCVConfig.cmake" ]; then
        LOCAL_OPENCV_DIR="${PO_CORE_OPENCV_DIR}"
        print_info "PoSDK local OpenCV not found, using po_core's OpenCV: ${LOCAL_OPENCV_DIR}"
        print_info "PoSDK 本地 OpenCV 未找到，使用 po_core 的 OpenCV: ${LOCAL_OPENCV_DIR}"
    fi
fi

if [ -d "${LOCAL_OPENCV_DIR}" ] && [ -f "${LOCAL_OPENCV_DIR}/lib/cmake/opencv4/OpenCVConfig.cmake" ]; then
    print_success "Found local OpenCV installation: ${LOCAL_OPENCV_DIR}"
    print_success "找到本地OpenCV安装: ${LOCAL_OPENCV_DIR}"
    OPENCV_CMAKE_DIR="${LOCAL_OPENCV_DIR}/lib/cmake/opencv4"
else
    print_warning "Local OpenCV installation not found"
    print_warning "本地OpenCV安装未找到"
    print_info "Please run ./install_opencv.sh first to install OpenCV"
    print_info "请先运行 ./install_opencv.sh 安装OpenCV"
    exit 1
fi

# Common CMake arguments, including enabling OpenMP which is required by MAGSAC.
CMAKE_ARGS+=(
    "-DCMAKE_INSTALL_PREFIX=${MAGSAC_INSTALL_DIR}"
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=Release
    -DCREATE_SAMPLE_PROJECT=OFF
    -DUSE_OPENMP=ON
    "-DCMAKE_PREFIX_PATH=${LOCAL_OPENCV_DIR}"
    "-DOpenCV_DIR=${OPENCV_CMAKE_DIR}"
)

if [[ "$(uname)" == "Darwin" ]]; then
    # macOS: Default AppleClang does not support OpenMP. We need to install and use LLVM from Homebrew.
    echo "Detected macOS. Checking for Homebrew and LLVM..."
    if ! command -v brew &> /dev/null; then
        echo "Error: Homebrew is not installed. Please install it from https://brew.sh/ to proceed."
        exit 1
    fi

    if ! brew list llvm &> /dev/null; then
        echo "LLVM not found. Installing LLVM via Homebrew to get OpenMP support. This may take a few minutes..."
        brew install llvm
    else
        echo "LLVM is already installed."
    fi
    
    LLVM_PREFIX=$(brew --prefix llvm)
    echo "Configuring CMake to use LLVM from ${LLVM_PREFIX} for OpenMP support."
    
    # Point CMake to the correct compiler and OpenMP libraries from the Homebrew LLVM installation.
    CMAKE_ARGS+=(
        "-DCMAKE_C_COMPILER=${LLVM_PREFIX}/bin/clang"
        "-DCMAKE_CXX_COMPILER=${LLVM_PREFIX}/bin/clang++"
        "-DCMAKE_AR=${LLVM_PREFIX}/bin/llvm-ar"
        "-DCMAKE_RANLIB=${LLVM_PREFIX}/bin/llvm-ranlib"
        "-DOpenMP_C_FLAGS=-Xpreprocessor -fopenmp -I${LLVM_PREFIX}/include"
        "-DOpenMP_CXX_FLAGS=-Xpreprocessor -fopenmp -I${LLVM_PREFIX}/include"
        "-DOpenMP_C_LIB_NAMES=libomp"
        "-DOpenMP_CXX_LIB_NAMES=libomp"
        "-DOpenMP_libomp_LIBRARY=${LLVM_PREFIX}/lib/libomp.dylib"
    )

elif [[ "$(uname)" == "Linux" ]]; then
    # Linux: Install OpenMP development library using the system's package manager.
    echo "Detected Linux. Checking for OpenMP development library..."
    if command -v apt-get &> /dev/null; then # Debian/Ubuntu
        if ! dpkg -l | grep -q libomp-dev; then
            echo "'libomp-dev' not found. Attempting to install it..."
            smart_sudo apt-get update
            smart_sudo apt-get install -y libomp-dev
        else
            echo "'libomp-dev' is already installed."
        fi
    elif command -v yum &> /dev/null; then # RedHat/CentOS/Fedora
        if ! rpm -q libomp-devel &> /dev/null; then
             echo "'libomp-devel' not found. Attempting to install it..."
             smart_sudo yum install -y libomp-devel
        else
            echo "'libomp-devel' is already installed."
        fi
    else
        echo "Warning: Could not detect 'apt' or 'yum'. Please ensure OpenMP development libraries (e.g., libomp-dev) are installed manually."
    fi
else
    echo "Warning: Unsupported OS '$(uname)'. Assuming OpenMP is available and can be found by CMake."
fi

echo "--- OpenMP setup complete ---"

# --- Python 3.8+ Dependency Check and Setup ---
echo "--- Checking for Python 3.8+ support ---"

if [[ "$(uname)" == "Linux" ]]; then
    # Check current Python 3 version
    if command -v python3 &> /dev/null; then
        PYTHON_VERSION=$(python3 --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
        PYTHON_MAJOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f1)
        PYTHON_MINOR=$(echo "$PYTHON_VERSION" | cut -d'.' -f2)
        
        print_info "Current Python 3 version: ${PYTHON_VERSION}"
        print_info "当前 Python 3 版本: ${PYTHON_VERSION}"
        
        if [[ $PYTHON_MAJOR -lt 3 ]] || [[ $PYTHON_MAJOR -eq 3 && $PYTHON_MINOR -lt 8 ]]; then
            print_warning "Python ${PYTHON_VERSION} is too old (need 3.8+)"
            print_warning "Python ${PYTHON_VERSION} 版本过旧（需要 3.8+）"
            print_info "Installing Python 3.8 from deadsnakes PPA..."
            print_info "从 deadsnakes PPA 安装 Python 3.8..."
            
            # Add deadsnakes PPA for Python 3.8 (works on Ubuntu 18.04)
            smart_sudo apt-get install -y software-properties-common
            smart_sudo add-apt-repository -y ppa:deadsnakes/ppa
            smart_sudo apt-get update
            
            # Install Python 3.8 and development headers
            smart_sudo apt-get install -y python3.8 python3.8-dev python3.8-distutils
            
            # Install pip for Python 3.8
            print_info "Installing pip for Python 3.8..."
            print_info "为 Python 3.8 安装 pip..."
            
            # Download and install pip (use Python 3.8 specific version)
            # Python 3.8 需要特定版本的 get-pip.py
            TEMP_DIR=$(mktemp -d)
            cd "$TEMP_DIR"
            curl -sS https://bootstrap.pypa.io/pip/3.8/get-pip.py -o get-pip.py
            smart_sudo python3.8 get-pip.py
            cd - > /dev/null
            rm -rf "$TEMP_DIR"
            
            # Verify pip installation
            if python3.8 -m pip --version > /dev/null 2>&1; then
                print_success "pip for Python 3.8 installed successfully"
                print_success "Python 3.8 的 pip 安装成功"
                
                # Install numpy for Python 3.8
                print_info "Installing numpy for Python 3.8..."
                smart_sudo python3.8 -m pip install numpy
            else
                print_warning "pip installation may have issues, but continuing..."
                print_warning "pip 安装可能有问题，但继续..."
            fi
            
            # Configure CMake to use Python 3.8
            CMAKE_ARGS+=(
                "-DPython_EXECUTABLE=$(which python3.8)"
                "-DPYTHON_EXECUTABLE=$(which python3.8)"
            )
            
            print_success "Python 3.8 installed successfully"
            print_success "Python 3.8 安装成功"
            print_info "CMake will use: $(which python3.8)"
        else
            print_success "Python version is sufficient (${PYTHON_VERSION} >= 3.8)"
            print_success "Python 版本满足要求 (${PYTHON_VERSION} >= 3.8)"
            
            # Ensure Python development headers and numpy are installed
            if command -v apt-get &> /dev/null; then
                smart_sudo apt-get install -y python3-dev python3-numpy || true
            fi
        fi
    else
        print_error "Python 3 not found!"
        print_error "未找到 Python 3！"
        exit 1
    fi
elif [[ "$(uname)" == "Darwin" ]]; then
    # macOS: Check Python version
    if command -v python3 &> /dev/null; then
        PYTHON_VERSION=$(python3 --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
        print_info "Python 3 version: ${PYTHON_VERSION}"
        
        # Install numpy if needed
        python3 -m pip install --user numpy || true
    fi
fi

echo "--- Python setup complete ---"

# Clean existing build and install directories for a fresh build
if [ -d "${MAGSAC_BUILD_DIR}" ]; then
    echo "Cleaning existing build directory: ${MAGSAC_BUILD_DIR}"
    rm -rf "${MAGSAC_BUILD_DIR}"
fi

if [ -d "${MAGSAC_INSTALL_DIR}" ]; then
    echo "Cleaning existing install directory: ${MAGSAC_INSTALL_DIR}"
    rm -rf "${MAGSAC_INSTALL_DIR}"
fi

echo "Creating clean build and install directories..."
mkdir -p "${MAGSAC_BUILD_DIR}"
mkdir -p "${MAGSAC_INSTALL_DIR}"

echo "Configuring MAGSAC with CMake..."
cd "${MAGSAC_BUILD_DIR}"

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

# Configure CMake for MAGSAC using the arguments defined above
echo "Running CMake with arguments: cmake .. ${CMAKE_ARGS[*]}"
cmake .. "${CMAKE_ARGS[@]}"

echo "Building MAGSAC (using all available processors)..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

echo "Installing MAGSAC to ${MAGSAC_INSTALL_DIR}..."
make install

echo "--- MAGSAC local build and installation complete! ---"
echo "MAGSAC has been installed in: ${MAGSAC_INSTALL_DIR}"
echo "Header files are located at: ${MAGSAC_INSTALL_DIR}/include/"
echo "Library files are located at: ${MAGSAC_INSTALL_DIR}/lib/"
echo "Your main project's CMake should now be able to find this local installation." 