#!/bin/bash
# ==============================================================================
# PoSDK Test Data Auto-Download Script | PoSDK测试数据自动下载脚本
# Description: Download test data package from GitHub Releases
# 说明：从GitHub Releases下载测试数据包
# Author: Qi Cai
# ==============================================================================

set -e

# Script configuration | 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_VERSION=${1:-"v1.0.0"}
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

# Function to check existing test data | 检查现有测试数据函数
check_existing_test_data() {
    print_info "Checking for existing test data..."
    print_info "检查现有测试数据..."

    # Check if Strecha dataset exists | 检查Strecha数据集是否存在
    if [[ -d "${SCRIPT_DIR}/Strecha" ]]; then
        local strecha_files=$(find "${SCRIPT_DIR}/Strecha" -type f | wc -l)
        if [[ $strecha_files -gt 10 ]]; then
            print_warning "Strecha dataset already exists with ${strecha_files} files"
            print_warning "Strecha数据集已存在，包含${strecha_files}个文件"

            read -p "Do you want to re-download test data? [y/N]: " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                print_info "Re-downloading test data..."
                print_info "重新下载测试数据..."
                # Backup existing data | 备份现有数据
                if [[ -d "${SCRIPT_DIR}/Strecha_backup" ]]; then
                    rm -rf "${SCRIPT_DIR}/Strecha_backup"
                fi
                mv "${SCRIPT_DIR}/Strecha" "${SCRIPT_DIR}/Strecha_backup"
                return 1
            else
                print_info "Using existing test data"
                print_info "使用现有测试数据"
                return 0
            fi
        fi
    fi
    return 1
}

# Function to download test data package | 下载测试数据包函数
download_test_data() {
    local package_name="PoSDK_test_data_${TESTS_VERSION}"
    local package_file="${package_name}.tar.gz"
    local download_url="https://github.com/${GITHUB_REPO}/releases/download/${TESTS_VERSION}/${package_file}"
    local temp_dir="/tmp/posdk_test_$$"
    local local_package_path="${SCRIPT_DIR}/${package_file}"

    # Check if local package exists first | 首先检查本地是否已有压缩包
    if [[ -f "$local_package_path" ]]; then
        print_warning "Found local package: ${package_file}"
        print_warning "发现本地压缩包：${package_file}"

        read -p "Skip download and use existing local package? [Y/n]: " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            print_info "Using existing local package"
            print_info "使用现有本地压缩包"

            # Use local package | 使用本地压缩包
            mkdir -p "$temp_dir"
            cp "$local_package_path" "$temp_dir/$package_file"
            cd "$temp_dir"
        else
            print_info "Proceeding with download..."
            print_info "继续下载..."

            # Remove existing local package and download fresh copy | 删除现有本地包并下载新副本
            rm -f "$local_package_path"

            # Create temporary directory | 创建临时目录
            mkdir -p "$temp_dir"
            cd "$temp_dir"

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

            curl -L -o "$package_file" "$download_url" --fail --progress-bar || {
                print_error "Failed to download test data package from GitHub"
                print_error "从GitHub下载测试数据包失败"
                print_error ""
                print_error "Manual Download Instructions | 手动下载说明:"
                print_error "手动下载说明:"
                print_error "1. Please visit: https://github.com/${GITHUB_REPO}/releases/tag/${TESTS_VERSION}"
                print_error "   请访问: https://github.com/${GITHUB_REPO}/releases/tag/${TESTS_VERSION}"
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
        fi
    else
        print_info "Local package not found, downloading from GitHub..."
        print_info "未找到本地压缩包，从GitHub下载..."

        # Create temporary directory | 创建临时目录
        mkdir -p "$temp_dir"
        cd "$temp_dir"

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

        curl -L -o "$package_file" "$download_url" --fail --progress-bar || {
            print_error "Failed to download test data package from GitHub"
            print_error "从GitHub下载测试数据包失败"
            print_error ""
            print_error "Manual Download Instructions | 手动下载说明:"
            print_error "手动下载说明:"
            print_error "1. Please visit: https://github.com/${GITHUB_REPO}/releases/tag/${TESTS_VERSION}"
            print_error "   请访问: https://github.com/${GITHUB_REPO}/releases/tag/${TESTS_VERSION}"
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
    fi

    # Extract package | 解压包
    print_info "Extracting test data package..."
    print_info "解压测试数据包..."
    # 过滤 macOS 特定的 tar 警告信息
    tar -xzf "$package_file" 2>&1 | grep -v "Ignoring unknown extended header keyword" | grep -v "忽略未知的扩展头" || {
        # 只有在非警告错误时才退出
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            print_error "Failed to extract test data package"
            print_error "解压测试数据包失败"
            exit 1
        fi
    }

    # Move to tests directory | 移动到tests目录
    if [[ -d "posdk_test_staging" ]]; then
        print_info "Installing test data to: ${SCRIPT_DIR}"
        print_info "将测试数据安装到：${SCRIPT_DIR}"

        # Move all test data directories | 移动所有测试数据目录
        for item in posdk_test_staging/*; do
            if [[ -e "$item" ]]; then
                local basename=$(basename "$item")
                print_info "Installing ${basename}..."
                mv "$item" "${SCRIPT_DIR}/"
            fi
        done

        rmdir posdk_test_staging

        print_success "Test data extracted successfully"
        print_success "测试数据解压成功"
    else
        print_error "Unexpected package structure"
        print_error "意外的包结构"
        exit 1
    fi

    # Clean up temporary files | 清理临时文件
    cd "$SCRIPT_DIR"
    rm -rf "$temp_dir"
}

# Function to show help | 显示帮助函数
show_help() {
    echo "Usage: $0 [TESTS_VERSION]"
    echo ""
    echo "Download PoSDK test data package from GitHub Releases"
    echo "从GitHub Releases下载PoSDK测试数据包"
    echo ""
    echo "Arguments:"
    echo "  TESTS_VERSION   Test data version to download (default: v1.0.0)"
    echo "                  要下载的测试数据版本（默认：v1.0.0）"
    echo ""
    echo "Examples:"
    echo "  $0                       # Use default version"
    echo "  $0 v1.0.0               # Use specific version"
    echo "  $0 v1.1.0               # Use different version"
    echo ""
    echo "Note: This script downloads Strecha dataset and other test data"
    echo "注意：此脚本下载Strecha数据集和其他测试数据"
    echo ""
    echo "Test data includes:"
    echo "测试数据包括："
    echo "  - Strecha dataset (castle, fountain, entry, Herz-Jesus scenes)"
    echo "  - Strecha数据集（castle、fountain、entry、Herz-Jesus场景）"
    echo "  - Other test datasets if available"
    echo "  - 其他可用的测试数据集"
}

# Main execution | 主执行函数
main() {
    echo ""
    echo "=========================================="
    echo "PoSDK Test Data Auto-Download"
    echo "PoSDK测试数据自动下载"
    echo "=========================================="
    echo ""

    if ! check_existing_test_data; then
        download_test_data
    fi

    echo ""
    print_success "Test data download completed!"
    print_success "测试数据下载完成！"
    echo ""
    print_info "Available test datasets:"
    print_info "可用的测试数据集："

    # List available datasets | 列出可用数据集
    if [[ -d "${SCRIPT_DIR}/Strecha" ]]; then
        local scenes=$(ls -1 "${SCRIPT_DIR}/Strecha" | grep -v "\.DS_Store" | wc -l)
        print_info "  📊 Strecha dataset: ${scenes} scenes"
        print_info "  📊 Strecha数据集：${scenes}个场景"
    fi

    if [[ -d "${SCRIPT_DIR}/LiGT" ]]; then
        print_info "  📊 LiGT dataset available"
        print_info "  📊 LiGT数据集可用"
    fi

    if [[ -d "${SCRIPT_DIR}/OpenMVG_test" ]]; then
        print_info "  📊 OpenMVG test dataset available"
        print_info "  📊 OpenMVG测试数据集可用"
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