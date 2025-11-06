#!/bin/bash
# ==============================================================================
# PoSDK po_core Auto-Download Script | PoSDK po_core自动下载脚本
# Description: Automatically download and configure precompiled po_core for PoSDK
# 说明：自动下载并配置预编译po_core供PoSDK使用
# Author: Qi Cai
# ==============================================================================

set -e

# Script configuration | 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
POSDK_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PO_CORE_VERSION=${1:-"1.0.0_test"}
GITHUB_REPO="pose-only-vision/PoSDK"  # Using the main repository

# Color output functions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect platform and architecture
detect_platform() {
    local platform=""
    local arch=""

    case "$(uname -s)" in
        Darwin*)
            platform="macos"
            if [[ "$(uname -m)" == "arm64" ]]; then
                arch="arm64"
            else
                arch="x86_64"
            fi
            ;;
        Linux*)
            # Detect actual Ubuntu version (works in containers and physical hosts)
            # 检测实际的 Ubuntu 版本（在容器和物理主机中都有效）
            if command -v lsb_release >/dev/null 2>&1; then
                # Use lsb_release to get Ubuntu version
                UBUNTU_VERSION=$(lsb_release -rs | cut -d'.' -f1,2)
                platform="ubuntu${UBUNTU_VERSION}"
            else
                # Fallback: try /etc/os-release
                if [[ -f /etc/os-release ]]; then
                    UBUNTU_VERSION=$(grep VERSION_ID /etc/os-release | cut -d'=' -f2 | tr -d '"' | cut -d'.' -f1,2)
                    platform="ubuntu${UBUNTU_VERSION}"
                else
                    # Last resort: assume 24.04
                    print_warning "Cannot detect Ubuntu version, assuming 24.04"
                    print_warning "无法检测 Ubuntu 版本，假设为 24.04"
                    platform="ubuntu24.04"
                fi
            fi
            if [[ "$(uname -m)" == "aarch64" ]]; then
                arch="arm64"
            else
                arch="x86_64"
            fi
            ;;
        *)
            print_error "Unsupported operating system: $(uname -s)"
            exit 1
            ;;
    esac

    PLATFORM_ARCH="${arch}-${platform}"
    print_info "Detected platform: ${PLATFORM_ARCH}"
}

# Function to check if po_core is already installed
check_existing_po_core() {
    local po_core_dir="${POSDK_ROOT}/dependencies/po_core_lib"

    if [[ -d "$po_core_dir" && -f "${po_core_dir}/lib/cmake/po_core/po_core-config.cmake" ]]; then
        print_info "Found existing po_core installation at: ${po_core_dir}"

        # Try to extract version info
        local config_file="${po_core_dir}/lib/cmake/po_core/po_core-config-version.cmake"
        if [[ -f "$config_file" ]]; then
            local installed_version=$(grep "set(PACKAGE_VERSION" "$config_file" | sed 's/.*"\(.*\)".*/\1/' | head -1)
            print_info "Installed version: ${installed_version}"
        fi

        read -p "Skip reinstall and use existing po_core installation? [Y/n]: " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Nn]$ ]]; then
            print_info "Removing existing installation..."
            rm -rf "$po_core_dir"
        else
            print_info "Using existing po_core installation"
            return 0
        fi
    fi
    return 1
}

# Function to download po_core release
download_po_core() {
    local deps_dir="${POSDK_ROOT}/dependencies"
    local package_name="po_core_${PLATFORM_ARCH}_v1.0.0"
    local package_file="${package_name}.tar.gz"
    local download_url="https://github.com/${GITHUB_REPO}/releases/download/${PO_CORE_VERSION}/${package_file}"
    local local_package_path="${deps_dir}/${package_file}"

    print_info "Setting up po_core ${PO_CORE_VERSION} for ${PLATFORM_ARCH}..."

    # Create dependencies directory
    mkdir -p "$deps_dir"
    cd "$deps_dir"

    # Check if local package exists first | 首先检查本地是否已有压缩包
    if [[ -f "$local_package_path" ]]; then
        print_success "Found local package: ${package_file}"
        print_success "发现本地压缩包：${package_file}"
        echo ""
        
        # Ask user if they want to skip downloading | 询问用户是否跳过下载
        read -p "Skip download and use local package? (使用本地压缩包跳过下载?) [Y/n]: " -n 1 -r
        echo ""
        
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            # User chose to use local package (default) | 用户选择使用本地压缩包（默认）
            print_info "Using local package, skipping download..."
            print_info "使用本地压缩包，跳过下载..."
            
            # Check and remove existing po_core_lib directory | 检查并清除已解压的目录
            local po_core_lib_dir="${deps_dir}/po_core_lib"
            if [[ -d "$po_core_lib_dir" ]]; then
                print_info "Removing existing po_core_lib directory for update..."
                print_info "清除现有po_core_lib目录以便更新..."
                rm -rf "$po_core_lib_dir"
                print_success "Existing directory removed"
                print_success "现有目录已清除"
            fi
        else
            # User chose to re-download | 用户选择重新下载
            print_info "Re-downloading package from GitHub..."
            print_info "从GitHub重新下载压缩包..."
            
            # Clean up: Remove old package and extracted directory | 清理：删除旧压缩包和解压目录
            print_info "Cleaning up old files..."
            print_info "清理旧文件..."
            
            # Remove old package | 删除旧压缩包
            if [[ -f "$local_package_path" ]]; then
                rm -f "$local_package_path"
                print_success "Removed old package: ${package_file}"
                print_success "已删除旧压缩包：${package_file}"
            fi
            
            # Remove extracted directory | 删除解压目录
            local po_core_lib_dir="${deps_dir}/po_core_lib"
            if [[ -d "$po_core_lib_dir" ]]; then
                rm -rf "$po_core_lib_dir"
                print_success "Removed extracted directory: po_core_lib/"
                print_success "已删除解压目录：po_core_lib/"
            fi
            
            # Download package file using curl | 使用curl下载包文件
            print_info "Downloading from: ${download_url}"
            print_info "正在从以下地址下载：${download_url}"

            if ! command -v curl >/dev/null 2>&1; then
                print_error "curl not found. Please install curl first."
                print_error "未找到curl。请先安装curl。"
                exit 1
            fi

            curl -L -o "$package_file" "$download_url" --fail --progress-bar || {
                print_error "Failed to download po_core package from GitHub"
                print_error "从GitHub下载po_core包失败"
                print_error ""
                print_error "=========================================="
                print_error "PLATFORM COMPATIBILITY WARNING | 平台兼容性警告"
                print_error "=========================================="
                print_error ""
                print_error "Current platform: ${PLATFORM_ARCH}"
                print_error "当前平台：${PLATFORM_ARCH}"
                print_error "Required package: ${package_file}"
                print_error "需要的包：${package_file}"
                print_error ""
                print_error "⚠️  The precompiled po_core package for your platform may not be available."
                print_error "⚠️  您平台的预编译 po_core 包可能不可用。"
                print_error ""
                print_error "This could cause compatibility issues. Please:"
                print_error "这可能导致兼容性问题。请："
                print_error ""
                print_error "1. Check available releases:"
                print_error "   检查可用的发布版本："
                print_error "   https://github.com/${GITHUB_REPO}/releases/tag/${PO_CORE_VERSION}"
                print_error ""
                print_error "2. If your platform package exists, download manually:"
                print_error "   如果您的平台包存在，请手动下载："
                print_error "   • Download file: ${package_file}"
                print_error "     下载文件：${package_file}"
                print_error "   • Place it in: ${deps_dir}/"
                print_error "     放置到目录：${deps_dir}/"
                print_error "   • Run this script again"
                print_error "     重新运行此脚本"
                print_error ""
                print_error "3. If your platform is not supported:"
                print_error "   如果您的平台不受支持："
                print_error "   • Try using a similar platform package (may have compatibility issues)"
                print_error "     尝试使用类似平台的包（可能存在兼容性问题）"
                print_error "   • Or build po_core from source"
                print_error "     或从源码构建 po_core"
                print_error ""
                print_error "Supported platforms (may vary by release):"
                print_error "支持的平台（因版本而异）："
                print_error "  • arm64-macos (Apple Silicon)"
                print_error "  • x86_64-macos (Intel Mac)"
                print_error "  • arm64-ubuntu24.04 (ARM64 Linux)"
                print_error "  • x86_64-ubuntu24.04 (x86_64 Linux)"
                print_error ""
                print_error "Expected file location: ${local_package_path}"
                print_error "期望文件位置: ${local_package_path}"
                print_error ""
                print_error "=========================================="
                exit 1
            }
            
            print_success "Download completed"
            print_success "下载完成"
        fi
    else
        print_info "Local package not found, downloading from GitHub..."
        print_info "未找到本地压缩包，从GitHub下载..."

        # Download package file using curl only | 仅使用curl下载包文件
        print_info "Downloading from: ${download_url}"
        print_info "正在从以下地址下载：${download_url}"

        if ! command -v curl >/dev/null 2>&1; then
            print_error "curl not found. Please install curl first."
            print_error "未找到curl。请先安装curl。"
            exit 1
        fi

        curl -L -o "$package_file" "$download_url" --fail --progress-bar || {
            print_error "Failed to download po_core package from GitHub"
            print_error "从GitHub下载po_core包失败"
            print_error ""
            print_error "=========================================="
            print_error "PLATFORM COMPATIBILITY WARNING | 平台兼容性警告"
            print_error "=========================================="
            print_error ""
            print_error "Current platform: ${PLATFORM_ARCH}"
            print_error "当前平台：${PLATFORM_ARCH}"
            print_error "Required package: ${package_file}"
            print_error "需要的包：${package_file}"
            print_error ""
            print_error "⚠️  The precompiled po_core package for your platform may not be available."
            print_error "⚠️  您平台的预编译 po_core 包可能不可用。"
            print_error ""
            print_error "This could cause compatibility issues. Please:"
            print_error "这可能导致兼容性问题。请："
            print_error ""
            print_error "1. Check available releases:"
            print_error "   检查可用的发布版本："
            print_error "   https://github.com/${GITHUB_REPO}/releases/tag/${PO_CORE_VERSION}"
            print_error ""
            print_error "2. If your platform package exists, download manually:"
            print_error "   如果您的平台包存在，请手动下载："
            print_error "   • Download file: ${package_file}"
            print_error "     下载文件：${package_file}"
            print_error "   • Place it in: ${deps_dir}/"
            print_error "     放置到目录：${deps_dir}/"
            print_error "   • Run this script again"
            print_error "     重新运行此脚本"
            print_error ""
            print_error "3. If your platform is not supported:"
            print_error "   如果您的平台不受支持："
            print_error "   • Try using a similar platform package (may have compatibility issues)"
            print_error "     尝试使用类似平台的包（可能存在兼容性问题）"
            print_error "   • Or build po_core from source"
            print_error "     或从源码构建 po_core"
            print_error ""
            print_error "Supported platforms (may vary by release):"
            print_error "支持的平台（因版本而异）："
            print_error "  • arm64-macos (Apple Silicon)"
            print_error "  • x86_64-macos (Intel Mac)"
            print_error "  • arm64-ubuntu24.04 (ARM64 Linux)"
            print_error "  • x86_64-ubuntu24.04 (x86_64 Linux)"
            print_error ""
            print_error "Expected file location: ${local_package_path}"
            print_error "期望文件位置: ${local_package_path}"
            print_error ""
            print_error "=========================================="
            exit 1
        }
        
        print_success "Download completed"
        print_success "下载完成"
    fi

    # Extract package
    print_info "Extracting po_core package..."
    print_info "正在解压po_core压缩包..."
    # 过滤 macOS 特定的 tar 警告信息
    tar -xzf "$package_file" 2>&1 | grep -v "Ignoring unknown extended header keyword" | grep -v "忽略未知的扩展头" || {
        # 只有在非警告错误时才退出
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            print_error "Failed to extract po_core package"
            print_error "解压po_core压缩包失败"
            exit 1
        fi
    }

    # Rename to standard directory name
    if [[ -d "po_core_lib" ]]; then
        print_success "po_core extracted successfully"
        print_success "po_core解压成功"
    else
        print_error "Unexpected package structure"
        print_error "意外的压缩包结构"
        exit 1
    fi

    # Keep the package file for future use | 保留压缩包以供将来使用
    print_info "Keeping package file: ${package_file}"
    print_info "保留压缩包文件：${package_file}"

    print_success "po_core ${PO_CORE_VERSION} installation completed"
    print_success "po_core ${PO_CORE_VERSION} 安装完成"
}

# Function to verify po_core installation
verify_installation() {
    local po_core_dir="${POSDK_ROOT}/dependencies/po_core_lib"
    local required_paths=(
        "${po_core_dir}/include"
        "${po_core_dir}/lib"
        "${po_core_dir}/lib/cmake/po_core"
        "${po_core_dir}/lib/cmake/po_core/po_core-config.cmake"
    )

    print_info "Verifying po_core installation..."

    for path in "${required_paths[@]}"; do
        if [[ ! -e "$path" ]]; then
            print_error "Required path not found: $path"
            exit 1
        fi
    done

    # Count library files
    local lib_count=$(find "${po_core_dir}/lib" -name "libpo_core*" -o -name "libpomvg*" | wc -l | tr -d ' ')
    if [[ $lib_count -eq 0 ]]; then
        print_error "No po_core library files found"
        exit 1
    fi

    print_success "po_core installation verified (${lib_count} library files found)"

    # Show installation summary
    print_info "Installation summary:"
    print_info "  Location: ${po_core_dir}"
    print_info "  Platform: ${PLATFORM_ARCH}"
    print_info "  Version: ${PO_CORE_VERSION}"
    print_info "  Libraries: ${lib_count} files"
}

# Function to show next steps
show_next_steps() {
    echo ""
    print_info "=========================================="
    print_info "Next Steps"
    print_info "=========================================="
    echo ""
    print_info "po_core is now ready for use with PoSDK!"
    echo ""
    print_info "To build PoSDK:"
    print_info "  cd ${POSDK_ROOT}"
    print_info "  mkdir -p build && cd build"
    print_info "  cmake .."
    print_info "  make -j\$(nproc)"
    echo ""
    print_info "CMake will automatically find and use the precompiled po_core."
    echo ""
    print_info "If you encounter issues:"
    print_info "  1. Check that all dependencies are installed"
    print_info "  2. Verify the po_core version matches your PoSDK requirements"
    print_info "  3. Check the build logs for specific error messages"
    echo ""
    print_info "=========================================="
}

# Function to show help
show_help() {
    echo "Usage: $0 [VERSION]"
    echo ""
    echo "Download and install precompiled po_core for PoSDK development"
    echo ""
    echo "Arguments:"
    echo "  VERSION    po_core version to download (default: 1.0.0_test)"
    echo ""
    echo "Examples:"
    echo "  $0 1.0.0_test    # Download specific version"
    echo "  $0               # Download default version (1.0.0_test)"
    echo ""
    echo "Environment:"
    echo "  The script automatically detects your platform and downloads"
    echo "  the appropriate precompiled po_core package."
    echo ""
    echo "Supported platforms:"
    echo "  - arm64-macos (Apple Silicon)"
    echo "  - x86_64-macos (Intel Mac)"
    echo "  - arm64-ubuntu24.04 (ARM64 Linux)"
    echo "  - x86_64-ubuntu24.04 (x86_64 Linux)"
}

# Main execution
main() {
    print_info "PoSDK Dependencies Setup - po_core Auto-Download"
    print_info "Version: ${PO_CORE_VERSION}"

    detect_platform

    if ! check_existing_po_core; then
        download_po_core
    fi

    verify_installation
    show_next_steps

    print_success "po_core setup completed successfully!"
}

# Handle command line arguments
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# Run main function
main "$@"