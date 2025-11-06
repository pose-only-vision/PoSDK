#!/bin/bash
# ==============================================================================
# PoSDK Dependencies Auto-Download Script | PoSDK依赖自动下载脚本
# Description: Download dependencies package from GitHub Releases
# 说明：从GitHub Releases下载依赖包
# Author: Qi Cai
# ==============================================================================

set -e

# Script configuration | 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_VERSION=${1:-"v1.0.0"}
GITHUB_REPO="pose-only-vision/PoSDK"

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

# Function to check local package and extract | 检查本地压缩包并解压函数
check_local_package() {
    local package_file="PoSDK_dependencies_${DEPS_VERSION}.tar.gz"
    local local_package_path="${SCRIPT_DIR}/${package_file}"

    # Check if local package exists | 检查本地压缩包是否存在
    if [[ -f "$local_package_path" ]]; then
        print_success "Found local package: ${package_file}"
        print_success "发现本地压缩包：${package_file}"
        echo ""

        read -p "Skip download and use local package? [Y/n]: " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            print_info "Using local package, extracting all contents..."
            print_info "使用本地压缩包，解压所有内容..."
            extract_package "$local_package_path"
            return 0  # Skip download
        else
            print_info "User chose to re-download, removing local package..."
            print_info "用户选择重新下载，删除本地压缩包..."
            rm -f "$local_package_path"
            return 1  # Proceed to download
        fi
    fi
    return 1  # No local package found
}

# Function to extract package and overwrite existing files | 解压包并覆盖现有文件函数
extract_package() {
    local package_path="$1"
    local temp_dir="/tmp/posdk_deps_$$"

    print_info "Extracting package to temporary directory..."
    print_info "解压包到临时目录..."

    # Create temporary directory and extract | 创建临时目录并解压
    mkdir -p "$temp_dir"
    cd "$temp_dir"

    # Extract package | 解压包
    # 过滤 macOS 特定的 tar 警告信息
    # 使用绝对路径确保解压成功
    local abs_package_path="$(cd "$(dirname "$package_path")" && pwd)/$(basename "$package_path")"
    tar -xzf "$abs_package_path" 2>&1 | grep -v "Ignoring unknown extended header keyword" | grep -v "忽略未知的扩展头" || {
        # 只有在非警告错误时才退出
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            print_error "Failed to extract dependencies package"
            print_error "解压依赖包失败"
            cd "$SCRIPT_DIR"
            rm -rf "$temp_dir"
            exit 1
        fi
    }

    # Install all dependencies, overwriting existing ones | 安装所有依赖，覆盖现有的
    if [[ -d "posdk_deps_staging" ]]; then
        print_info "Installing/updating all dependencies to: ${SCRIPT_DIR}"
        print_info "安装/更新所有依赖到：${SCRIPT_DIR}"

        local installed_count=0
        local updated_count=0

        # Ensure we process all items, even if some fail | 确保处理所有项目，即使某些失败
        set +e  # Temporarily disable exit on error for loop processing | 临时禁用错误退出以便循环处理

        for item in posdk_deps_staging/*; do
            # Check if glob matched anything | 检查glob是否匹配到任何内容
            if [[ ! -e "$item" ]]; then
                continue
            fi
            
                local basename=$(basename "$item")
                local target_path="${SCRIPT_DIR}/${basename}"

                # Skip install scripts (they are handled separately) | 跳过安装脚本（它们单独处理）
                if [[ "$basename" == "install_"*.sh || "$basename" == "download_"*.sh || "$basename" == "README.md" ]]; then
                    print_info "  Skipped script: ${basename} (managed separately)"
                    print_info "  跳过脚本：${basename}（单独管理）"
                    continue
                fi

                # Special handling for boost_1_85_0.tar.gz (copy, don't extract) | boost_1_85_0.tar.gz特殊处理（拷贝，不解压）
                if [[ "$basename" == "boost_1_85_0.tar.gz" ]]; then
                    if [[ -f "$target_path" ]]; then
                        print_info "  Found existing Boost archive, updating..."
                        print_info "  发现现有Boost压缩包，正在更新..."
                        rm -f "$target_path"
                        ((updated_count++))
                    else
                        print_success "  Installing Boost archive: ${basename}"
                        print_success "  安装Boost压缩包：${basename}"
                        ((installed_count++))
                    fi
                    cp "$item" "$target_path"
                    print_success "  ✓ Boost archive ready: ${basename}"
                    print_success "  ✓ Boost压缩包就绪：${basename}"
                    continue
                fi

                # Update existing directory/file, preserving build artifacts | 更新现有目录/文件，保留构建产物
                if [[ -e "$target_path" ]]; then
                    # Create a temporary location for the new source | 为新源码创建临时位置
                    local temp_new_item="${target_path}_new_$$"
                    mv "$item" "$temp_new_item"

                    # Preserve build_local and install_local directories using rsync | 使用rsync保留build_local和install_local目录
                    if [[ -d "$target_path/build_local" || -d "$target_path/install_local" ]]; then
                        print_info "    Syncing source files while preserving build artifacts..."
                        print_info "    同步源文件，同时保留构建产物..."

                        # Use rsync to sync source files (update modified, add new, delete old)
                        # Preserve build_local and install_local via --exclude
                        # 使用rsync同步源文件（更新已修改、添加新增、删除旧的）
                        # 通过 --exclude 保留 build_local 和 install_local
                        if command -v rsync >/dev/null 2>&1; then
                            rsync -a --delete --exclude='build_local' --exclude='install_local' \
                                  "$temp_new_item/" "$target_path/" >/dev/null 2>&1 && \
                            rm -rf "$temp_new_item"
                        else
                            # Fallback if rsync not available: use mv/rm/mv method
                            # 如果rsync不可用，回退到 mv/rm/mv 方法
                            print_warning "    rsync not found, using fallback method..."
                            print_warning "    未找到rsync，使用备用方法..."
                            
                            # Preserve artifacts first | 先保留构建产物
                            if [[ -d "$target_path/build_local" ]]; then
                                mv "$target_path/build_local" "/tmp/build_local_backup_fallback_$$" 2>/dev/null || true
                            fi
                            if [[ -d "$target_path/install_local" ]]; then
                                mv "$target_path/install_local" "/tmp/install_local_backup_fallback_$$" 2>/dev/null || true
                            fi
                            
                            # Replace source | 替换源码
                            rm -rf "$target_path"
                            mv "$temp_new_item" "$target_path"
                            
                            # Restore artifacts | 恢复构建产物
                            if [[ -d "/tmp/build_local_backup_fallback_$$" ]]; then
                                mv "/tmp/build_local_backup_fallback_$$" "$target_path/build_local"
                            fi
                            if [[ -d "/tmp/install_local_backup_fallback_$$" ]]; then
                                mv "/tmp/install_local_backup_fallback_$$" "$target_path/install_local"
                            fi
                        fi
                    else
                        # No build artifacts to preserve, simple replacement | 无构建产物要保留，直接替换
                        rm -rf "$target_path"
                        mv "$temp_new_item" "$target_path"
                    fi

                    print_success "  Updated: ${basename} (build artifacts preserved)"
                    print_success "  已更新：${basename}（构建产物已保留）"
                    ((updated_count++))
                else
                    print_success "  Installed: ${basename}"
                    print_success "  已安装：${basename}"
                    mv "$item" "$target_path"
                    ((installed_count++))
            fi
        done
        
        set -e  # Re-enable exit on error | 重新启用错误退出

        rmdir posdk_deps_staging 2>/dev/null || true

        echo ""
        print_success "Dependencies extraction completed"
        print_success "依赖解压完成"
        print_info "  Newly installed: ${installed_count} dependencies"
        print_info "  Updated: ${updated_count} dependencies"
        print_info "  新安装：${installed_count}个依赖"
        print_info "  更新：${updated_count}个依赖"
    else
        print_error "Unexpected package structure"
        print_error "意外的包结构"
        exit 1
    fi

    # Clean up temporary files | 清理临时文件
    cd "$SCRIPT_DIR"
    rm -rf "$temp_dir"
}

# Function to download dependencies package | 下载依赖包函数
download_dependencies() {
    local package_name="PoSDK_dependencies_${DEPS_VERSION}"
    local package_file="${package_name}.tar.gz"
    local download_url="https://github.com/${GITHUB_REPO}/releases/download/${DEPS_VERSION}/${package_file}"
    local local_package_path="${SCRIPT_DIR}/${package_file}"

    print_info "Downloading from GitHub..."
    print_info "从GitHub下载..."

    print_info "Downloading from: ${download_url}"
    print_info "正在从以下地址下载：${download_url}"

    # Download package file using curl only | 仅使用curl下载包文件
    print_info "Using curl to download package..."
    print_info "使用curl下载包..."

    if ! command -v curl >/dev/null 2>&1; then
        print_error "curl not found. Please install curl first."
        print_error "未找到curl。请先安装curl。"
        exit 1
    fi

    curl -L -o "$local_package_path" "$download_url" --fail --progress-bar || {
        print_error "Failed to download dependencies package from GitHub"
        print_error "从GitHub下载依赖包失败"
        print_error ""
        print_error "Manual Download Instructions | 手动下载说明:"
        print_error "手动下载说明:"
        print_error "1. Please visit: https://github.com/${GITHUB_REPO}/releases/tag/${DEPS_VERSION}"
        print_error "   请访问: https://github.com/${GITHUB_REPO}/releases/tag/${DEPS_VERSION}"
        print_error "2. Download file: ${package_file}"
        print_error "   下载文件: ${package_file}"
        print_error "3. Place it in: ${SCRIPT_DIR}/"
        print_error "   放置到目录: ${SCRIPT_DIR}/"
        print_error "4. Run this script again"
        print_error "   重新运行此脚本"
        print_error ""
        print_error "Expected file location: ${local_package_path}"
        print_error "期望文件位置: ${local_package_path}"
        exit 1
    }

    print_success "Download completed: ${package_file}"
    print_success "下载完成：${package_file}"

    # Extract the downloaded package | 解压下载的包
    print_info "Extracting downloaded package..."
    print_info "解压下载的包..."
    extract_package "$local_package_path"
}

# Function to show help | 显示帮助函数
show_help() {
    echo "Usage: $0 [DEPS_VERSION]"
    echo ""
    echo "Download PoSDK dependencies package from GitHub Releases"
    echo "从GitHub Releases下载PoSDK依赖包"
    echo ""
    echo "Arguments:"
    echo "  DEPS_VERSION    Dependencies version to download (default: v1.0.0)"
    echo "                  要下载的依赖版本（默认：v1.0.0）"
    echo ""
    echo "Examples:"
    echo "  $0                    # Use default version"
    echo "  $0 v1.0.0            # Use specific version"
    echo "  $0 v1.1.0            # Use different version"
    echo ""
    echo "Requirements:"
    echo "  curl              Required for downloading packages"
    echo "                    下载包需要curl"
    echo ""
    echo "Note: This script only downloads dependencies. To install them, run:"
    echo "注意：此脚本仅下载依赖。要安装它们，请运行："
    echo "  ./install_dependencies.sh"
    echo ""
}

# Main execution | 主执行函数
main() {
    echo ""
    echo "=========================================="
    echo "PoSDK Dependencies Auto-Download"
    echo "PoSDK依赖自动下载"
    echo "=========================================="
    echo ""

    # First check if local package exists and handle it | 首先检查本地压缩包是否存在并处理
    if check_local_package; then
        # Local package was used and extracted | 本地压缩包已使用并解压
        echo ""
        print_success "Dependencies setup completed using local package!"
        print_success "使用本地压缩包完成依赖设置！"
    else
        # No local package, proceed to download | 没有本地压缩包，继续下载
        download_dependencies
        echo ""
        print_success "Dependencies download and extraction completed!"
        print_success "依赖下载和解压完成！"
    fi
    echo ""
}

# Handle command line arguments | 处理命令行参数
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# Run main function | 运行主函数
main "$@"