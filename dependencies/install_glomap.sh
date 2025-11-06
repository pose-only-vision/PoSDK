#!/bin/bash

# 脚本用于在 Ubuntu 和 Mac 上从源码安装 GLOMAP

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
GLOMAP_DIR="${SCRIPT_DIR}/glomap-main"
GLOMAP_BUILD_DIR="${GLOMAP_DIR}/build_local"
GLOMAP_INSTALL_DIR="${GLOMAP_DIR}/install_local"
POSELIB_INSTALL_DIR="${SCRIPT_DIR}/PoseLib/install_local"
LOCAL_COLMAP_DIR="${GLOMAP_DIR}/dependencies/colmap-src"

# Detect Ubuntu version for compatibility fixes | 检测Ubuntu版本用于兼容性修复
UBUNTU_VERSION=""
if [ "$OS" == "ubuntu" ] && [ -f /etc/os-release ]; then
    . /etc/os-release
    UBUNTU_VERSION="${VERSION_ID}"
    print_info "检测到 Ubuntu 版本 | Detected Ubuntu version: ${UBUNTU_VERSION}"
fi

echo "========================================"
echo "   GLOMAP 安装向导"
echo "   GLOMAP Installation Wizard"
echo "========================================"
echo ""

# Step 0: Check and install CMake 3.28+ if needed | 步骤0：检查并安装 CMake 3.28+（如需要）
print_info "步骤 0/3：检查 CMake 版本（GLOMAP 要求 3.28+）"
print_info "Step 0/3: Checking CMake version (GLOMAP requires 3.28+)"
echo ""

if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
    
    print_info "当前 CMake 版本 | Current CMake version: ${CMAKE_VERSION}"
    
    if [[ $CMAKE_MAJOR -lt 3 ]] || [[ $CMAKE_MAJOR -eq 3 && $CMAKE_MINOR -lt 28 ]]; then
        print_error "CMake 版本过旧 (${CMAKE_VERSION} < 3.28)"
        print_error "CMake version is too old (${CMAKE_VERSION} < 3.28)"
        echo ""
        print_info "GLOMAP 需要 CMake 3.28+ 以支持现代 C++ 特性"
        print_info "GLOMAP requires CMake 3.28+ for modern C++ features support"
        echo ""
        print_info "解决方案 | Solutions:"
        print_info "  1. 运行主安装脚本会自动升级 CMake | Main install.sh will auto-upgrade CMake"
        print_info "     cd .. && ./install.sh"
        print_info "  2. 手动安装 CMake | Manually install CMake:"
        print_info "     ./install_cmake.sh"
        echo ""
        exit 1
    else
        print_success "✓ CMake 版本满足要求 (${CMAKE_VERSION} >= 3.28)"
        print_success "✓ CMake version meets requirement (${CMAKE_VERSION} >= 3.28)"
    fi
else
    print_error "未检测到 CMake"
    print_error "CMake not detected"
    echo ""
    print_info "请先安装 CMake 3.28+ | Please install CMake 3.28+ first:"
    print_info "  方法1 | Option 1: 运行主安装脚本 | Run main install script"
    print_info "    cd .. && ./install.sh"
    print_info "  方法2 | Option 2: 手动安装 CMake | Manually install CMake"
    print_info "    ./install_cmake.sh"
    echo ""
    exit 1
fi

echo ""

# Pre-Step: Apply compatibility fixes automatically | 预步骤：自动应用兼容性修复
echo "========================================"
print_info "预步骤：自动应用兼容性修复"
print_info "Pre-Step: Auto-applying compatibility fixes"
echo "========================================"
echo ""

# Fix 1: Boost compatibility (COLMAP) - CMake 3.30 Policy CMP0167 and CMP0144
COLMAP_FIND_DEPS="${SCRIPT_DIR}/glomap-main/dependencies/colmap-src/cmake/FindDependencies.cmake"
if [ -f "${COLMAP_FIND_DEPS}" ]; then
    NEED_BACKUP=false
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
    # Add CMP0144=OLD if not present, so CGAL can use BOOST_ROOT
    # 如果不存在则添加CMP0144=OLD，以便CGAL可以使用BOOST_ROOT
    if ! grep -q "cmake_policy.*CMP0144" "${COLMAP_FIND_DEPS}"; then
        if [ "$BACKUP_CREATED" = false ]; then
            cp "${COLMAP_FIND_DEPS}" "${COLMAP_FIND_DEPS}.backup_auto"
            BACKUP_CREATED=true
        fi
        # Insert after first cmake_policy or cmake_minimum_required | 在第一个cmake_policy或cmake_minimum_required之后插入
        sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0144 OLD)
' "${COLMAP_FIND_DEPS}"
        print_info "应用 CMP0144 修复（BOOST_ROOT 变量支持）"
        print_info "Applying CMP0144 fix (BOOST_ROOT variable support)"
    fi
    
    # Fix CMP0074 (Boost_ROOT variable) for find_package | 修复CMP0074（Boost_ROOT变量）以支持find_package
    # Add CMP0074=NEW if not present, so find_package can use Boost_ROOT
    # 如果不存在则添加CMP0074=NEW，以便find_package可以使用Boost_ROOT
    if ! grep -q "cmake_policy.*CMP0074" "${COLMAP_FIND_DEPS}"; then
        if [ "$BACKUP_CREATED" = false ]; then
            cp "${COLMAP_FIND_DEPS}" "${COLMAP_FIND_DEPS}.backup_auto"
            BACKUP_CREATED=true
        fi
        # Insert after first cmake_policy or cmake_minimum_required | 在第一个cmake_policy或cmake_minimum_required之后插入
        sed -i.bak '/^cmake_minimum_required\|^cmake_policy/a\
cmake_policy(SET CMP0074 NEW)
' "${COLMAP_FIND_DEPS}"
        print_info "应用 CMP0074 修复（Boost_ROOT 变量支持）"
        print_info "Applying CMP0074 fix (Boost_ROOT variable support)"
    fi
    
    if [ "$BACKUP_CREATED" = true ]; then
        rm -f "${COLMAP_FIND_DEPS}.bak"
        print_success "✓ Boost 兼容性修复已应用（CMP0167=OLD, CMP0144=OLD, CMP0074=NEW）"
        print_success "✓ Boost compatibility fixes applied (CMP0167=OLD, CMP0144=OLD, CMP0074=NEW)"
    else
        print_success "✓ Boost 兼容性已修复（跳过）"
        print_success "✓ Boost compatibility already fixed (skipped)"
    fi
else
    print_warning "⚠ 未找到 COLMAP FindDependencies.cmake，跳过 Boost 修复"
    print_warning "⚠ COLMAP FindDependencies.cmake not found, skipping Boost fix"
fi
echo ""

# Fix 2: Ceres compatibility - Deferred to later (will build local Ceres if needed)
# 修复2：Ceres兼容性 - 延迟处理（如果需要将构建本地Ceres）
print_info "✓ Ceres 兼容性修复将在后续步骤中处理"
print_info "✓ Ceres compatibility fix will be handled in later steps"
echo ""

# Fix 3: Install missing dependencies (LZ4, FLANN) - will be done later in the script
print_info "依赖检查将在后续步骤中进行"
print_info "Dependency check will be performed in later steps"
echo ""

echo "========================================"
print_success "✓ 兼容性修复完成"
print_success "✓ Compatibility fixes completed"
echo "========================================"
echo ""

# Step 1: Check for existing GLOMAP installation | 步骤1：检查现有GLOMAP安装
print_info "步骤 1/3：检查现有 GLOMAP 本地安装"
print_info "Step 1/3: Checking existing GLOMAP local installation"
echo ""

# 检查本地安装（类似 install_absl.sh）
GLOMAP_BINARY="${GLOMAP_INSTALL_DIR}/bin/glomap"
if [ -f "${GLOMAP_BINARY}" ]; then
    print_success "✓ 发现已安装的 GLOMAP（本地）"
    print_success "✓ Found existing GLOMAP installation (local)"
    echo "  路径 | Path: ${GLOMAP_BINARY}"
    echo ""
    
    # Show version if possible
    print_info "GLOMAP 版本信息 | GLOMAP version:"
    "${GLOMAP_BINARY}" --version 2>/dev/null || "${GLOMAP_BINARY}" -h 2>/dev/null | head -3 || echo "  - GLOMAP binary is available"
    echo ""
    
    read -p "是否跳过此次安装并使用现有构建？[Y/n]：" -n 1 -r
    echo ""
    
    # Default to Y if no input | 默认选择Y
    if [[ -z "$REPLY" ]] || [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "✅ 跳过 GLOMAP 构建 - 使用现有安装"
        print_info "✅ Skipping GLOMAP build - using existing installation"
        print_success "GLOMAP installation location: ${GLOMAP_INSTALL_DIR}"
        print_success "GLOMAP 安装位置: ${GLOMAP_INSTALL_DIR}"
        exit 0
    else
        print_info "🔄 继续进行新的 GLOMAP 构建..."
        print_info "🔄 Proceeding with fresh GLOMAP build..."
        echo ""
    fi
else
    print_info "未发现本地 GLOMAP 安装，将进行全新安装"
    print_info "No local GLOMAP installation found, proceeding with fresh installation"
    echo ""
fi

# Step 2: Check for local dependencies | 步骤2：检查本地依赖
echo "========================================"
print_info "步骤 2/3：检查本地依赖"
print_info "Step 2/3: Checking local dependencies"
echo "========================================"
echo ""

# Check if glomap-main directory exists
if [ ! -d "${GLOMAP_DIR}" ]; then
    print_error "✗ 未找到 glomap-main 目录"
    print_error "✗ glomap-main directory not found"
    print_error "路径 | Path: ${GLOMAP_DIR}"
    print_error ""
    print_error "请从 https://github.com/colmap/glomap 下载并解压"
    print_error "Please download and extract from https://github.com/colmap/glomap"
    exit 1
fi

# Check local COLMAP
HAS_LOCAL_COLMAP=false
COLMAP_STATUS="❌ 未找到"
if [ -d "${LOCAL_COLMAP_DIR}" ] && [ -f "${LOCAL_COLMAP_DIR}/CMakeLists.txt" ]; then
    HAS_LOCAL_COLMAP=true
    COLMAP_STATUS="✅ 已就绪"
    
    # Check if it's a git repo and get commit
    if [ -d "${LOCAL_COLMAP_DIR}/.git" ]; then
        COLMAP_COMMIT=$(cd "${LOCAL_COLMAP_DIR}" && git rev-parse HEAD 2>/dev/null | cut -c1-8)
        COLMAP_STATUS="✅ 已就绪 (commit: ${COLMAP_COMMIT})"
    fi
fi

# Check local PoseLib
HAS_LOCAL_POSELIB=false
POSELIB_STATUS="❌ 未找到"
# PoseLib现在被构建为共享库（.so）和/或静态库（.a）
if [ -d "${POSELIB_INSTALL_DIR}" ] && ([ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.so" ] || [ -f "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a" ]); then
    HAS_LOCAL_POSELIB=true
    POSELIB_STATUS="✅ 已就绪"
fi

# Check local Ceres - important for Ubuntu 18.04 which has old system Ceres 1.13.0
# 检查本地Ceres - 对Ubuntu 18.04很重要，它有旧的系统Ceres 1.13.0
HAS_LOCAL_CERES=false
CERES_STATUS="❌ 未找到"
CERES_INSTALL_DIR="${SCRIPT_DIR}/ceres-solver-2.2.0/install_local"

if [ -f "${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake" ]; then
    HAS_LOCAL_CERES=true
    CERES_STATUS="✅ 本地 2.2.0"
else
    # Check system Ceres version | 检查系统Ceres版本
    if command -v pkg-config &> /dev/null; then
        CERES_VERSION=$(pkg-config --modversion ceres-solver 2>/dev/null || echo "")
        if [ -n "${CERES_VERSION}" ]; then
            CERES_STATUS="✅ 系统 ${CERES_VERSION}"

            # Check if it's an old version (< 2.0.0) that lacks Ceres::ceres target
            # 检查是否是缺少Ceres::ceres target的旧版本 (< 2.0.0)
            MAJOR_VERSION=$(echo "${CERES_VERSION}" | cut -d'.' -f1)
            if [ "${MAJOR_VERSION}" -lt 2 ]; then
                CERES_STATUS="⚠️  系统 ${CERES_VERSION} (旧版)"
            fi
        fi
    fi
fi

# Display detection results
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   本地依赖检测结果 | Local Dependencies Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 COLMAP:  ${COLMAP_STATUS}"
if [ "$HAS_LOCAL_COLMAP" = true ]; then
    echo "   路径 | Path: ${LOCAL_COLMAP_DIR}"
fi
echo ""
echo "📦 PoseLib: ${POSELIB_STATUS}"
if [ "$HAS_LOCAL_POSELIB" = true ]; then
    echo "   路径 | Path: ${POSELIB_INSTALL_DIR}"
fi
echo ""
echo "📦 Ceres:   ${CERES_STATUS}"
if [ "$HAS_LOCAL_CERES" = true ]; then
    echo "   路径 | Path: ${CERES_INSTALL_DIR}"
fi
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Determine build mode
USE_LOCAL_BUILD=false
BUILD_MODE="FetchContent 模式"

if [ "$HAS_LOCAL_COLMAP" = true ] && [ "$HAS_LOCAL_POSELIB" = true ]; then
    print_success "✓ 检测到本地 COLMAP 和 PoseLib！"
    print_success "✓ Local COLMAP and PoseLib detected!"
    echo ""
    print_info "本地构建模式优势 | Benefits of local build:"
    print_info "  • 无需网络下载 | No network required"
    print_info "  • 构建速度更快 | Faster build"
    print_info "  • 已应用必要补丁 | Patches already applied"
    echo ""
    
    read -p "是否使用本地依赖构建? | Use local dependencies for build? [Y/n]: " -n 1 -r
    echo ""
    
    if [[ -z "$REPLY" ]] || [[ ! $REPLY =~ ^[Nn]$ ]]; then
        USE_LOCAL_BUILD=true
        BUILD_MODE="本地构建模式 | Local Build Mode"
        print_success "✓ 将使用本地依赖构建"
        print_success "✓ Will use local dependencies"
    else
        print_info "将使用 FetchContent 模式"
        print_info "Will use FetchContent mode"
    fi
elif [ "$HAS_LOCAL_POSELIB" = true ]; then
    print_warning "⚠ 仅检测到 PoseLib，COLMAP 将使用 FetchContent"
    print_warning "⚠ Only PoseLib detected, COLMAP will use FetchContent"
    BUILD_MODE="混合模式 (本地 PoseLib + FetchContent COLMAP)"
else
    print_warning "⚠ 未检测到本地依赖"
    print_warning "⚠ No local dependencies detected"
    print_info "将使用 GLOMAP 内置的 FetchContent 下载"
    print_info "Will use GLOMAP built-in FetchContent"
    echo ""
    print_warning "注意：需要稳定的网络连接！"
    print_warning "Note: Stable network connection required!"
    echo ""
    print_info "提示：SDK 通常已包含 COLMAP 源码"
    print_info "Tip: SDK usually includes COLMAP source code"
    print_info "请确认 glomap-main/dependencies/colmap-src/ 是否存在"
    print_info "Please check if glomap-main/dependencies/colmap-src/ exists"
fi

echo ""
echo "========================================"
print_info "构建模式 | Build Mode: ${BUILD_MODE}"
echo "========================================"
echo ""

# Check if Ceres needs to be built locally
# 检查是否需要在本地构建Ceres
if [ -z "${MAJOR_VERSION}" ] || [ "${MAJOR_VERSION}" -ge 2 ] || [ "$HAS_LOCAL_CERES" = true ]; then
    # Ceres is either not found, new version (>= 2.0.0), or already installed locally
    # Ceres要么没有找到，要么是新版本（>= 2.0.0），要么已在本地安装
    CERES_READY=true

    # Print status message | 打印状态信息
    if [ "$HAS_LOCAL_CERES" = true ]; then
        print_success "✓ 本地 Ceres 2.2.0 已就绪"
        print_success "✓ Local Ceres 2.2.0 is ready"
    elif [ -n "${MAJOR_VERSION}" ] && [ "${MAJOR_VERSION}" -ge 2 ]; then
        print_success "✓ 系统 Ceres ${CERES_VERSION} 已就绪"
        print_success "✓ System Ceres ${CERES_VERSION} is ready"
    else
        print_info "ℹ Ceres 不可用，将由 COLMAP 的 FetchContent 处理"
        print_info "ℹ Ceres not found, will be handled by COLMAP FetchContent"
    fi
else
    # Old system Ceres detected (< 2.0.0) - need to build local version
    # 检测到旧系统Ceres（< 2.0.0）- 需要构建本地版本
    CERES_READY=false
fi

if [ "$CERES_READY" = false ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    print_warning "⚠ Ceres 兼容性问题"
    print_warning "⚠ Ceres Compatibility Issue"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    print_warning "检测到旧版本系统 Ceres (${CERES_VERSION})"
    print_warning "Detected old system Ceres version (${CERES_VERSION})"
    echo ""
    print_info "问题 | Problem:"
    print_info "  • 旧版 Ceres (<2.0) 不提供 Ceres::ceres CMake target"
    print_info "  • Old Ceres (<2.0) lacks Ceres::ceres CMake target"
    print_info "  • GLOMAP 和 COLMAP 需要此 target 来正确链接"
    print_info "  • GLOMAP and COLMAP require this target for proper linking"
    echo ""
    print_info "解决方案 | Solution:"
    print_info "  • 构建本地 Ceres 2.2.0（推荐）"
    print_info "  • Build local Ceres 2.2.0 (recommended)"
    print_info "  或升级系统 Ceres 至 2.0+ 版本"
    print_info "  Or upgrade system Ceres to 2.0+ version"
    echo ""

    read -p "是否构建本地 Ceres 2.2.0? [Y/n]: " -n 1 -r
    echo ""

    if [[ -z "$REPLY" ]] || [[ ! $REPLY =~ ^[Nn]$ ]]; then
        if [ -f "${SCRIPT_DIR}/install_ceres.sh" ]; then
            print_info "正在调用 install_ceres.sh 构建本地 Ceres..."
            print_info "Calling install_ceres.sh to build local Ceres..."
            echo ""

            # Run install_ceres.sh | 运行 install_ceres.sh
            bash "${SCRIPT_DIR}/install_ceres.sh"

            if [ -f "${CERES_INSTALL_DIR}/lib/cmake/Ceres/CeresConfig.cmake" ]; then
                print_success "✓ 本地 Ceres 2.2.0 构建成功"
                print_success "✓ Local Ceres 2.2.0 built successfully"
                HAS_LOCAL_CERES=true
                CERES_READY=true
            else
                print_error "✗ Ceres 构建失败"
                print_error "✗ Ceres build failed"
                exit 1
            fi
        else
            print_error "✗ 未找到 install_ceres.sh"
            print_error "✗ install_ceres.sh not found"
            exit 1
        fi
    else
        print_warning "⚠ 继续使用旧系统 Ceres，构建可能失败"
        print_warning "⚠ Will continue with old system Ceres, build may fail"
        echo ""
    fi

    echo ""
fi

# Step 3a: Install local Boost if needed | 步骤 3a：安装本地 Boost（如需要）
echo "========================================"
print_info "步骤 3a/4：检查 Boost 依赖"
print_info "Step 3a/4: Checking Boost dependency"
echo "========================================"
echo ""

BOOST_VERSION_TARGET="1.85.0"
BOOST_VERSION_UNDERSCORE="1_85_0"
LOCAL_BOOST_DIR="${SCRIPT_DIR}/boost_${BOOST_VERSION_UNDERSCORE}"
LOCAL_BOOST_INSTALL="${LOCAL_BOOST_DIR}/install_local"

if [ -d "${LOCAL_BOOST_INSTALL}" ] && [ -f "${LOCAL_BOOST_INSTALL}/lib/libboost_system.a" ]; then
    print_success "✓ 找到本地 Boost ${BOOST_VERSION_TARGET}"
    print_success "✓ Found local Boost ${BOOST_VERSION_TARGET}"
    echo "  路径 | Path: ${LOCAL_BOOST_INSTALL}"
    HAS_LOCAL_BOOST=true
else
    print_info "未找到本地 Boost ${BOOST_VERSION_TARGET}"
    print_info "Local Boost ${BOOST_VERSION_TARGET} not found"
    echo ""
    print_info "GLOMAP 需要 Boost（Ubuntu 18.04 的系统 Boost 1.65.1 太旧）"
    print_info "GLOMAP requires Boost (system Boost 1.65.1 on Ubuntu 18.04 is too old)"
    echo ""
    
    if [ -f "${SCRIPT_DIR}/install_boost.sh" ]; then
        read -p "是否安装本地 Boost ${BOOST_VERSION_TARGET}？[Y/n]: " -n 1 -r
        echo ""
        
        if [[ -z "$REPLY" ]] || [[ ! $REPLY =~ ^[Nn]$ ]]; then
            print_info "开始安装本地 Boost..."
            print_info "Starting local Boost installation..."
            echo ""
            
            bash "${SCRIPT_DIR}/install_boost.sh"
            
            if [ -f "${LOCAL_BOOST_INSTALL}/lib/libboost_system.a" ]; then
                print_success "✓ 本地 Boost 安装成功"
                print_success "✓ Local Boost installed successfully"
                HAS_LOCAL_BOOST=true
            else
                print_error "✗ Boost 安装失败"
                print_error "✗ Boost installation failed"
                exit 1
            fi
        else
            print_warning "⚠ 跳过 Boost 安装，使用系统 Boost（可能失败）"
            print_warning "⚠ Skipping Boost installation, will use system Boost (may fail)"
            HAS_LOCAL_BOOST=false
        fi
    else
        print_error "✗ 未找到 install_boost.sh 脚本"
        print_error "✗ install_boost.sh script not found"
        print_warning "⚠ 将尝试使用系统 Boost"
        print_warning "⚠ Will try to use system Boost"
        HAS_LOCAL_BOOST=false
    fi
fi

echo ""

# Step 3b: Configure for local build if needed | 步骤 3b：配置本地构建（如需要）
if [ "$USE_LOCAL_BUILD" = true ]; then
    print_info "步骤 3b/4：配置本地构建"
    print_info "Step 3b/4: Local build configuration"
    echo ""
    print_info "本地 COLMAP 将由 CMake 自动检测"
    print_info "Local COLMAP will be automatically detected by CMake"
    echo ""
else
    print_info "步骤 3b/4：使用 FetchContent 模式"
    print_info "Step 3b/4: Using FetchContent mode"
    echo ""
fi

# 开始构建流程
cd "${GLOMAP_DIR}"

# 安装依赖并编译（本地安装模式）
if [ "$OS" == "ubuntu" ]; then
    echo "正在 Ubuntu 上安装到本地..."
    print_info "Installing to local directory on Ubuntu..."
    echo ""

    # 更新软件包列表
    print_info "更新软件包列表..."
    print_info "Updating package lists..."
    smart_sudo apt-get update

    # 安装所需依赖
    print_info "安装 GLOMAP 编译依赖..."
    print_info "Installing GLOMAP build dependencies..."
    smart_sudo apt-get install -y \
        cmake \
        ninja-build \
        build-essential \
        libflann-dev \
        liblz4-dev

    # 清理现有的构建和安装目录（类似 install_absl.sh）
    if [ -d "${GLOMAP_BUILD_DIR}" ]; then
        print_info "清理旧的 build 目录 | Cleaning old build directory..."
        rm -rf "${GLOMAP_BUILD_DIR}"
    fi
    
    if [ -d "${GLOMAP_INSTALL_DIR}" ]; then
        print_info "清理旧的 install 目录 | Cleaning old install directory..."
        rm -rf "${GLOMAP_INSTALL_DIR}"
    fi

    cd "${GLOMAP_DIR}"
    mkdir -p "${GLOMAP_BUILD_DIR}"
    cd "${GLOMAP_BUILD_DIR}"

    # Handle GUI compatibility for Ubuntu 18.04 | 处理Ubuntu 18.04的GUI兼容性
    # Ubuntu 18.04 has Qt5 5.9.5 which lacks complete OpenGL 3.2+ support
    # Ubuntu 18.04有Qt5 5.9.5，它缺乏完整的OpenGL 3.2+支持
    GUI_OPTION=""
    if [[ "${UBUNTU_VERSION}" == "18.04" ]]; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        print_info "Ubuntu 18.04: 禁用 GUI 支持"
        print_info "Ubuntu 18.04: Disabling GUI support"
        print_info "原因: Qt5 5.9.5 对 OpenGL 3.2+ 支持不完整"
        print_info "Reason: Qt5 5.9.5 has incomplete OpenGL 3.2+ support"
        print_info "这避免了 QOpenGLFunctions_3_2_Core 编译错误"
        print_info "This avoids QOpenGLFunctions_3_2_Core compilation errors"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        GUI_OPTION="-DGUI_ENABLED=OFF"
        echo ""
    fi

    # Clean environment to avoid Anaconda pollution | 清理环境以避免Anaconda污染
    # Save original environment variables | 保存原始环境变量
    ORIGINAL_PATH="${PATH}"
    ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
    ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    ORIGINAL_PKG_CONFIG_PATH="${PKG_CONFIG_PATH:-}"
    
    # Remove Anaconda from PATH | 从PATH中移除Anaconda
    if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
        print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
        print_info "Detected Anaconda/Conda in PATH, temporarily removing to avoid pollution"
        # Remove anaconda and conda paths | 移除anaconda和conda路径
        CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        export PATH="${CLEAN_PATH}"
    fi
    
    # Remove Anaconda from LD_LIBRARY_PATH | 从LD_LIBRARY_PATH中移除Anaconda
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
    
    # Detect Anaconda base directory | 检测Anaconda基础目录
    ANACONDA_BASE_DIRS=""
    if [[ -n "${CONDA_PREFIX}" ]]; then
        ANACONDA_BASE_DIRS="${CONDA_PREFIX}"
        print_info "检测到CONDA_PREFIX: ${CONDA_PREFIX}"
        print_info "Detected CONDA_PREFIX: ${CONDA_PREFIX}"
    fi
    # Also check common Anaconda locations | 同时检查常见的Anaconda位置
    for anaconda_path in "${HOME}/anaconda3" "${HOME}/miniconda3" "/opt/anaconda3" "/opt/miniconda3"; do
        if [ -d "${anaconda_path}" ]; then
            if [ -z "${ANACONDA_BASE_DIRS}" ]; then
                ANACONDA_BASE_DIRS="${anaconda_path}"
            else
                ANACONDA_BASE_DIRS="${ANACONDA_BASE_DIRS};${anaconda_path}"
            fi
            print_info "检测到Anaconda路径: ${anaconda_path}"
            print_info "Detected Anaconda path: ${anaconda_path}"
        fi
    done
    
    # Clean CMAKE_PREFIX_PATH to exclude Anaconda | 清理CMAKE_PREFIX_PATH以排除Anaconda
    if [ -n "${CMAKE_PREFIX_PATH}" ]; then
        CLEAN_CMAKE_PREFIX_PATH=$(echo "${CMAKE_PREFIX_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        if [ -n "${CLEAN_CMAKE_PREFIX_PATH}" ] && [ "${CLEAN_CMAKE_PREFIX_PATH}" != "${CMAKE_PREFIX_PATH}" ]; then
            print_info "从CMAKE_PREFIX_PATH中移除Anaconda路径"
            print_info "Removing Anaconda paths from CMAKE_PREFIX_PATH"
            export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
        fi
    fi
    
    # Unset CONDA_PREFIX if set (prevents conda activation scripts from interfering)
    # 如果设置了CONDA_PREFIX则取消设置（防止conda激活脚本干扰）
    if [ -n "${CONDA_PREFIX}" ]; then
        print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
        print_info "Temporarily unsetting CONDA_PREFIX: ${CONDA_PREFIX}"
        unset CONDA_PREFIX
    fi
    echo ""

    # Configure CMake based on build mode
    if [ "$USE_LOCAL_BUILD" = true ]; then
        print_info "配置GLOMAP使用本地依赖和本地安装"
        print_info "Configuring GLOMAP to use local dependencies and local installation"
        print_info "  • COLMAP: 本地源码 (通过 add_subdirectory)"
        print_info "  • PoseLib: 本地安装 (通过 find_package)"
        if [ "$HAS_LOCAL_CERES" = true ]; then
            print_info "  • Ceres: 本地 2.2.0 (通过 find_package)"
        elif [ -n "${CERES_VERSION}" ]; then
            print_info "  • Ceres: 系统 ${CERES_VERSION}"
        fi
        print_info "  • Install: ${GLOMAP_INSTALL_DIR}"
        echo ""

        # 本地构建模式：
        # - 设置 CMAKE_INSTALL_PREFIX 指定本地��装目录
        # - 设置 FETCH_POSELIB=OFF，让 FindDependencies.cmake 使用 find_package(PoseLib)
        # - 设置 FETCH_COLMAP=OFF，让 FindDependencies.cmake 自动检测本地 COLMAP | Set FETCH_COLMAP=OFF for auto-detection
        # - 通过 CMAKE_PREFIX_PATH 和 PoseLib_DIR 让 find_package 找到本地 PoseLib
        # - 如果有本地Boost，通过 BOOST_ROOT 让 CMake 找到本地 Boost
        # - 如果有本地Ceres，通过 Ceres_DIR 让 find_package 找到本地 Ceres
        # - CMAKE_FIND_PACKAGE_PREFER_CONFIG 优先使用CMake配置文件
        
        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        # GLOMAP needs to find PoseLib and Boost libraries
        # GLOMAP 需要找到 PoseLib 和 Boost 库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            # Include GLOMAP's lib, PoseLib's lib, and Boost's lib in RPATH
            # RPATH 中包含 GLOMAP 的 lib、PoseLib 的 lib 和 Boost 的 lib
            INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../PoseLib/install_local/lib:\$ORIGIN/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 和 PoseLib 库"
            print_info "Setting RPATH to include local Boost and PoseLib libraries"
        else
            # Include GLOMAP's lib and PoseLib's lib in RPATH
            # RPATH 中包含 GLOMAP 的 lib 和 PoseLib 的 lib
            INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../PoseLib/install_local/lib"
        fi
        
        # Build CMAKE_PREFIX_PATH (exclude Anaconda, only include needed paths)
        # 构建CMAKE_PREFIX_PATH（排除Anaconda，只包含所需路径）
        CMAKE_PREFIX_PATH_VALUE="${POSELIB_INSTALL_DIR}"
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_PREFIX_PATH_VALUE="${LOCAL_BOOST_INSTALL};${CMAKE_PREFIX_PATH_VALUE}"
        fi
        if [ "$HAS_LOCAL_CERES" = true ]; then
            CMAKE_PREFIX_PATH_VALUE="${CERES_INSTALL_DIR};${CMAKE_PREFIX_PATH_VALUE}"
        fi
        
        CMAKE_OPTS=(
            "-GNinja"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
            "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
            "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
            "-DGUI_ENABLED=OFF"
            "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
            "-DCMAKE_INSTALL_PREFIX=${GLOMAP_INSTALL_DIR}"
            "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_VALUE}"
            "-DCMAKE_LIBRARY_PATH=${POSELIB_INSTALL_DIR}/lib"
            "-DCMAKE_EXE_LINKER_FLAGS=-L${POSELIB_INSTALL_DIR}/lib -Wl,-rpath,${POSELIB_INSTALL_DIR}/lib"
            "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
            "-DCMAKE_INSTALL_RPATH=${INSTALL_RPATH}"
            "-DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE"
            "-DCMAKE_SKIP_BUILD_RPATH=FALSE"
            "-DPoseLib_DIR=${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
            "-DFETCH_POSELIB=OFF"
            "-DFETCH_COLMAP=OFF"
        )

        # Add local Boost if available | 如果有本地Boost则添加
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
            print_info "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH_VALUE}"
        fi

        # Add local Ceres if available | 如果有本地Ceres则添加
        if [ "$HAS_LOCAL_CERES" = true ]; then
            CMAKE_OPTS+=("-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres")
        fi

        # Add GUI option for Ubuntu 18.04 compatibility | 为Ubuntu 18.04兼容性添加GUI选项
        if [ -n "${GUI_OPTION}" ]; then
            CMAKE_OPTS+=("${GUI_OPTION}")
        fi

        cmake .. "${CMAKE_OPTS[@]}"
    elif [ "$HAS_LOCAL_POSELIB" = true ]; then
        print_info "配置GLOMAP：本地 PoseLib + FetchContent COLMAP + 本地安装"
        print_info "Configuring GLOMAP: Local PoseLib + FetchContent COLMAP + Local Installation"
        print_info "  • PoseLib: ${POSELIB_INSTALL_DIR}"
        print_info "  • COLMAP: FetchContent (网络下载)"
        if [ "$HAS_LOCAL_CERES" = true ]; then
            print_info "  • Ceres: 本地 2.2.0"
        elif [ -n "${CERES_VERSION}" ]; then
            print_info "  • Ceres: 系统 ${CERES_VERSION}"
        fi
        print_info "  • Install: ${GLOMAP_INSTALL_DIR}"
        echo ""

        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../PoseLib/install_local/lib:\$ORIGIN/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 和 PoseLib 库"
            print_info "Setting RPATH to include local Boost and PoseLib libraries"
        else
            INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../PoseLib/install_local/lib"
        fi

        CMAKE_OPTS=(
            "-GNinja"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
            "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
            "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
            "-DGUI_ENABLED=OFF"
            "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
            "-DCMAKE_INSTALL_PREFIX=${GLOMAP_INSTALL_DIR}"
            "-DCMAKE_PREFIX_PATH=${POSELIB_INSTALL_DIR}"
            "-DCMAKE_LIBRARY_PATH=${POSELIB_INSTALL_DIR}/lib"
            "-DCMAKE_EXE_LINKER_FLAGS=-L${POSELIB_INSTALL_DIR}/lib -Wl,-rpath,${POSELIB_INSTALL_DIR}/lib"
            "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
            "-DCMAKE_INSTALL_RPATH=${INSTALL_RPATH}"
            "-DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE"
            "-DCMAKE_SKIP_BUILD_RPATH=FALSE"
            "-DPoseLib_DIR=${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
            "-DFETCH_COLMAP=ON"
            "-DFETCH_POSELIB=OFF"
        )

        # Add local Boost if available | 如果有本地Boost则添加
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_OPTS+=("-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}")
            CMAKE_OPTS+=("-DBoost_ROOT=${LOCAL_BOOST_INSTALL}")
            CMAKE_OPTS+=("-DBoost_NO_SYSTEM_PATHS=ON")
            CMAKE_OPTS+=("-DBoost_NO_BOOST_CMAKE=ON")
            CMAKE_OPTS+=("-DBOOST_INCLUDEDIR=${LOCAL_BOOST_INSTALL}/include")
            CMAKE_OPTS+=("-DBOOST_LIBRARYDIR=${LOCAL_BOOST_INSTALL}/lib")
            print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
            print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
        fi

        # Add local Ceres if available | 如果有本地Ceres则添加
        if [ "$HAS_LOCAL_CERES" = true ]; then
            CMAKE_OPTS+=("-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres")
        fi

        # Add GUI option for Ubuntu 18.04 compatibility | 为Ubuntu 18.04兼容性添加GUI选项
        if [ -n "${GUI_OPTION}" ]; then
            CMAKE_OPTS+=("${GUI_OPTION}")
        fi

        cmake .. "${CMAKE_OPTS[@]}"
    else
        print_info "配置GLOMAP：完全使用 FetchContent + 本地安装"
        print_info "Configuring GLOMAP: Full FetchContent mode + Local Installation"
        print_info "  • COLMAP: FetchContent (网络下载)"
        print_info "  • PoseLib: FetchContent (网络下载)"
        if [ "$HAS_LOCAL_CERES" = true ]; then
            print_info "  • Ceres: 本地 2.2.0"
        elif [ -n "${CERES_VERSION}" ]; then
            print_info "  • Ceres: 系统 ${CERES_VERSION}"
        fi
        print_info "  • Install: ${GLOMAP_INSTALL_DIR}"
        echo ""

        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            # FetchContent mode: GLOMAP will download PoseLib, but Boost is local
            # FetchContent 模式：GLOMAP 会下载 PoseLib，但 Boost 是本地的
            INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 库"
            print_info "Setting RPATH to include local Boost libraries"
        else
            INSTALL_RPATH="\$ORIGIN/../lib"
        fi

        CMAKE_OPTS=(
            "-GNinja"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"
            "-DCMAKE_POLICY_DEFAULT_CMP0167=OLD"
            "-DCMAKE_POLICY_DEFAULT_CMP0074=NEW"
            "-DGUI_ENABLED=OFF"
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
            "-DCMAKE_INSTALL_PREFIX=${GLOMAP_INSTALL_DIR}"
            "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
            "-DCMAKE_INSTALL_RPATH=${INSTALL_RPATH}"
            "-DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE"
            "-DCMAKE_SKIP_BUILD_RPATH=FALSE"
            "-DFETCH_COLMAP=ON"
            "-DFETCH_POSELIB=ON"
        )

        # Add local Boost if available | 如果有本地Boost则添加
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_OPTS+=("-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}")
            CMAKE_OPTS+=("-DBoost_ROOT=${LOCAL_BOOST_INSTALL}")
            CMAKE_OPTS+=("-DBoost_NO_SYSTEM_PATHS=ON")
            CMAKE_OPTS+=("-DBoost_NO_BOOST_CMAKE=ON")
            CMAKE_OPTS+=("-DBOOST_INCLUDEDIR=${LOCAL_BOOST_INSTALL}/include")
            CMAKE_OPTS+=("-DBOOST_LIBRARYDIR=${LOCAL_BOOST_INSTALL}/lib")
            print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
            print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
        fi

        # Add local Ceres if available | 如果有本地Ceres则添加
        if [ "$HAS_LOCAL_CERES" = true ]; then
            CMAKE_OPTS+=("-DCeres_DIR=${CERES_INSTALL_DIR}/lib/cmake/Ceres")
        fi

        # Add GUI option for Ubuntu 18.04 compatibility | 为Ubuntu 18.04兼容性添加GUI选项
        if [ -n "${GUI_OPTION}" ]; then
            CMAKE_OPTS+=("${GUI_OPTION}")
        fi

        cmake .. "${CMAKE_OPTS[@]}"
    fi

    echo ""
    print_info "开始编译..."
    print_info "Starting compilation..."
    ninja
    
    echo ""
    print_info "安装到本地目录..."
    print_info "Installing to local directory..."
    ninja install

elif [ "$OS" == "mac" ]; then
    echo "正在 Mac 上安装到本地..."
    print_info "Installing to local directory on macOS..."
    echo ""

    # 检查 Homebrew 是否已安装
    if ! command -v brew &> /dev/null; then
        echo "未找到 Homebrew。正在安装 Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    # 通过 Homebrew 安装依赖
    print_info "安装 GLOMAP 编译依赖..."
    print_info "Installing GLOMAP build dependencies..."
    brew install \
        cmake \
        ninja \
        flann \
        lz4

    # 清理现有的构建和安装目录（类似 install_absl.sh）
    if [ -d "${GLOMAP_BUILD_DIR}" ]; then
        print_info "清理旧的 build 目录 | Cleaning old build directory..."
        rm -rf "${GLOMAP_BUILD_DIR}"
    fi
    
    if [ -d "${GLOMAP_INSTALL_DIR}" ]; then
        print_info "清理旧的 install 目录 | Cleaning old install directory..."
        rm -rf "${GLOMAP_INSTALL_DIR}"
    fi

    cd "${GLOMAP_DIR}"
    mkdir -p "${GLOMAP_BUILD_DIR}"
    cd "${GLOMAP_BUILD_DIR}"

    # Clean environment to avoid Anaconda pollution (macOS) | 清理环境以避免Anaconda污染（macOS）
    # Save original environment variables | 保存原始环境变量
    ORIGINAL_PATH_MAC="${PATH}"
    ORIGINAL_DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:-}"
    ORIGINAL_CMAKE_PREFIX_PATH_MAC="${CMAKE_PREFIX_PATH:-}"
    
    # Remove Anaconda from PATH | 从PATH中移除Anaconda
    if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
        print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
        print_info "Detected Anaconda/Conda in PATH, temporarily removing to avoid pollution"
        CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        export PATH="${CLEAN_PATH}"
    fi
    
    # Detect Anaconda base directory | 检测Anaconda基础目录
    ANACONDA_BASE_DIRS_MAC=""
    if [[ -n "${CONDA_PREFIX}" ]]; then
        ANACONDA_BASE_DIRS_MAC="${CONDA_PREFIX}"
        print_info "检测到CONDA_PREFIX: ${CONDA_PREFIX}"
        print_info "Detected CONDA_PREFIX: ${CONDA_PREFIX}"
    fi
    for anaconda_path in "${HOME}/anaconda3" "${HOME}/miniconda3" "/opt/anaconda3" "/opt/miniconda3"; do
        if [ -d "${anaconda_path}" ]; then
            if [ -z "${ANACONDA_BASE_DIRS_MAC}" ]; then
                ANACONDA_BASE_DIRS_MAC="${anaconda_path}"
            else
                ANACONDA_BASE_DIRS_MAC="${ANACONDA_BASE_DIRS_MAC};${anaconda_path}"
            fi
            print_info "检测到Anaconda路径: ${anaconda_path}"
            print_info "Detected Anaconda path: ${anaconda_path}"
        fi
    done
    
    # Clean CMAKE_PREFIX_PATH to exclude Anaconda | 清理CMAKE_PREFIX_PATH以排除Anaconda
    if [ -n "${CMAKE_PREFIX_PATH}" ]; then
        CLEAN_CMAKE_PREFIX_PATH=$(echo "${CMAKE_PREFIX_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
        if [ -n "${CLEAN_CMAKE_PREFIX_PATH}" ] && [ "${CLEAN_CMAKE_PREFIX_PATH}" != "${CMAKE_PREFIX_PATH}" ]; then
            print_info "从CMAKE_PREFIX_PATH中移除Anaconda路径"
            print_info "Removing Anaconda paths from CMAKE_PREFIX_PATH"
            export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
        fi
    fi
    
    # Unset CONDA_PREFIX if set (prevents conda activation scripts from interfering)
    # 如果设置了CONDA_PREFIX则取消设置（防止conda激活脚本干扰）
    if [ -n "${CONDA_PREFIX}" ]; then
        print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
        print_info "Temporarily unsetting CONDA_PREFIX: ${CONDA_PREFIX}"
        unset CONDA_PREFIX
    fi
    echo ""

    # Configure Qt path for macOS | 为macOS配置Qt路径
    QT_PREFIX_PATH=""
    if [[ "$(uname -m)" == "arm64" ]]; then
        # Apple Silicon (M1/M2/...)
        QT_PREFIX_PATH="/opt/homebrew/opt/qt@5"
    else
        # Intel Mac
        QT_PREFIX_PATH="/usr/local/opt/qt@5"
    fi

    print_info "使用Qt路径 | Using Qt path: ${QT_PREFIX_PATH}"
    echo ""

    # Configure CMake based on build mode
    if [ "$USE_LOCAL_BUILD" = true ]; then
        print_info "配置GLOMAP使用本地依赖"
        print_info "Configuring GLOMAP to use local dependencies"
        print_info "  • COLMAP: 本地源码 (通过 add_subdirectory)"
        print_info "  • PoseLib: 本地安装 (通过 find_package)"
        echo ""
        
        # 本地构建模式：
        # - 设置 FETCH_POSELIB=OFF，让 FindDependencies.cmake 使用 find_package(PoseLib)
        # - 设置 FETCH_COLMAP=OFF，让 FindDependencies.cmake 自动检测本地 COLMAP | Set FETCH_COLMAP=OFF for auto-detection
        # - 通过 CMAKE_PREFIX_PATH 和 PoseLib_DIR 让 find_package 找到本地 PoseLib
        # - 如果有本地Boost，通过 BOOST_ROOT 让 CMake 找到本地 Boost
        
        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            INSTALL_RPATH="@loader_path/../lib:@loader_path/../../PoseLib/install_local/lib:@loader_path/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 和 PoseLib 库"
            print_info "Setting RPATH to include local Boost and PoseLib libraries"
        else
            INSTALL_RPATH="@loader_path/../lib:@loader_path/../../PoseLib/install_local/lib"
        fi
        
        CMAKE_CONFIG=(
            -GNinja
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_POLICY_DEFAULT_CMP0144=NEW
            -DCMAKE_POLICY_DEFAULT_CMP0167=OLD
            -DGUI_ENABLED=OFF
            -DCMAKE_INSTALL_PREFIX="${GLOMAP_INSTALL_DIR}"
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH};${POSELIB_INSTALL_DIR}"
            "-DCMAKE_EXE_LINKER_FLAGS=-L${POSELIB_INSTALL_DIR}/lib -Wl,-rpath,${POSELIB_INSTALL_DIR}/lib"
            -DCMAKE_MACOSX_RPATH=ON
            -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE
            -DCMAKE_INSTALL_RPATH="${INSTALL_RPATH}"
            -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE
            -DCMAKE_SKIP_BUILD_RPATH=FALSE
            -DQt5_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5"
            -DQt5Core_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Core"
            -DQt5Widgets_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Widgets"
            -DQt5OpenGL_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5OpenGL"
            -DPoseLib_DIR="${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
            -DFETCH_POSELIB=OFF
            -DFETCH_COLMAP=OFF
        )
        
        # Add local Boost if available
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_CONFIG+=(-DBOOST_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_NO_SYSTEM_PATHS=ON)
            CMAKE_CONFIG+=(-DBoost_NO_BOOST_CMAKE=ON)
            CMAKE_CONFIG+=(-DBOOST_INCLUDEDIR="${LOCAL_BOOST_INSTALL}/include")
            CMAKE_CONFIG+=(-DBOOST_LIBRARYDIR="${LOCAL_BOOST_INSTALL}/lib")
            print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
            print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
        fi
        
        cmake .. "${CMAKE_CONFIG[@]}"
    elif [ "$HAS_LOCAL_POSELIB" = true ]; then
        print_info "配置GLOMAP：本地 PoseLib + FetchContent COLMAP"
        print_info "Configuring GLOMAP: Local PoseLib + FetchContent COLMAP"
        print_info "  • PoseLib: ${POSELIB_INSTALL_DIR}"
        print_info "  • COLMAP: FetchContent (网络下载)"
        echo ""
        
        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            INSTALL_RPATH="@loader_path/../lib:@loader_path/../../PoseLib/install_local/lib:@loader_path/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 和 PoseLib 库"
            print_info "Setting RPATH to include local Boost and PoseLib libraries"
        else
            INSTALL_RPATH="@loader_path/../lib:@loader_path/../../PoseLib/install_local/lib"
        fi
        
        CMAKE_CONFIG=(
            -GNinja
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_POLICY_DEFAULT_CMP0144=NEW
            -DCMAKE_POLICY_DEFAULT_CMP0167=OLD
            -DGUI_ENABLED=OFF
            -DCMAKE_INSTALL_PREFIX="${GLOMAP_INSTALL_DIR}"
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH};${POSELIB_INSTALL_DIR}"
            "-DCMAKE_EXE_LINKER_FLAGS=-L${POSELIB_INSTALL_DIR}/lib -Wl,-rpath,${POSELIB_INSTALL_DIR}/lib"
            -DCMAKE_MACOSX_RPATH=ON
            -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE
            -DCMAKE_INSTALL_RPATH="${INSTALL_RPATH}"
            -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE
            -DCMAKE_SKIP_BUILD_RPATH=FALSE
            -DQt5_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5"
            -DQt5Core_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Core"
            -DQt5Widgets_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Widgets"
            -DQt5OpenGL_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5OpenGL"
            -DPoseLib_DIR="${POSELIB_INSTALL_DIR}/lib/cmake/PoseLib"
            -DFETCH_COLMAP=ON
            -DFETCH_POSELIB=OFF
        )
        
        # Add local Boost if available
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_CONFIG+=(-DBOOST_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_NO_SYSTEM_PATHS=ON)
            CMAKE_CONFIG+=(-DBoost_NO_BOOST_CMAKE=ON)
            CMAKE_CONFIG+=(-DBOOST_INCLUDEDIR="${LOCAL_BOOST_INSTALL}/include")
            CMAKE_CONFIG+=(-DBOOST_LIBRARYDIR="${LOCAL_BOOST_INSTALL}/lib")
            print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
            print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
        fi
        
        cmake .. "${CMAKE_CONFIG[@]}"
    else
        print_info "配置GLOMAP：完全使用 FetchContent"
        print_info "Configuring GLOMAP: Full FetchContent mode"
        print_info "  • COLMAP: FetchContent (网络下载)"
        print_info "  • PoseLib: FetchContent (网络下载)"
        echo ""
        
        # Set RPATH for runtime library finding | 设置RPATH用于运行时查找库
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            # FetchContent mode: GLOMAP will download everything, but Boost is local
            # FetchContent 模式：GLOMAP 会下载所有内容，但 Boost 是本地的
            INSTALL_RPATH="@loader_path/../lib:@loader_path/../../boost_${BOOST_VERSION_UNDERSCORE}/install_local/lib"
            print_info "设置 RPATH 包含本地 Boost 库"
            print_info "Setting RPATH to include local Boost libraries"
        else
            INSTALL_RPATH="@loader_path/../lib"
        fi
        
        CMAKE_CONFIG=(
            -GNinja
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_POLICY_DEFAULT_CMP0144=NEW
            -DCMAKE_POLICY_DEFAULT_CMP0167=OLD
            -DGUI_ENABLED=OFF
            -DCMAKE_INSTALL_PREFIX="${GLOMAP_INSTALL_DIR}"
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH}"
            -DCMAKE_MACOSX_RPATH=ON
            -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE
            -DCMAKE_INSTALL_RPATH="${INSTALL_RPATH}"
            -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE
            -DCMAKE_SKIP_BUILD_RPATH=FALSE
            -DQt5_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5"
            -DQt5Core_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Core"
            -DQt5Widgets_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5Widgets"
            -DQt5OpenGL_DIR="${QT_PREFIX_PATH}/lib/cmake/Qt5OpenGL"
            -DFETCH_COLMAP=ON
            -DFETCH_POSELIB=ON
        )
        
        # Add local Boost if available
        if [ "$HAS_LOCAL_BOOST" = true ]; then
            CMAKE_CONFIG+=(-DBOOST_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_ROOT="${LOCAL_BOOST_INSTALL}")
            CMAKE_CONFIG+=(-DBoost_NO_SYSTEM_PATHS=ON)
            CMAKE_CONFIG+=(-DBoost_NO_BOOST_CMAKE=ON)
            CMAKE_CONFIG+=(-DBOOST_INCLUDEDIR="${LOCAL_BOOST_INSTALL}/include")
            CMAKE_CONFIG+=(-DBOOST_LIBRARYDIR="${LOCAL_BOOST_INSTALL}/lib")
            print_info "强制使用本地Boost: ${LOCAL_BOOST_INSTALL}"
            print_info "Forcing local Boost: ${LOCAL_BOOST_INSTALL}"
        fi
        
        cmake .. "${CMAKE_CONFIG[@]}"
    fi

    echo ""
    print_info "开始编译..."
    print_info "Starting compilation..."
    ninja
    
    echo ""
    print_info "安装到本地目录..."
    print_info "Installing to local directory..."
    ninja install
fi

echo ""
echo "========================================"
print_success "GLOMAP 安装完成！"
print_success "GLOMAP Installation Completed!"
echo "========================================"
echo ""
print_info "验证安装 | Verify installation:"
echo "  glomap -h"
echo ""
print_info "构建模式 | Build mode used: ${BUILD_MODE}"
echo "========================================"
