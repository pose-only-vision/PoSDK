#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# PoSDK Dependencies Installation Master Script
# Install all required dependency libraries in correct order

echo "========================================"
echo "   PoSDK Dependencies Installation"
echo "========================================"
echo "This script will:"
echo "1. 📦 Download dependencies package from GitHub Releases"
echo "2. 📥 Download po_core precompiled binaries"
echo "3. 🔧 Install all dependency libraries in correct order"
echo "========================================"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Dependencies directory: ${SCRIPT_DIR}"

# First, download dependencies if needed
if [[ -f "${SCRIPT_DIR}/download_dependencies.sh" ]]; then
    echo ""
    echo "Step 1: Downloading dependencies package..."
    chmod +x "${SCRIPT_DIR}/download_dependencies.sh"
    "${SCRIPT_DIR}/download_dependencies.sh" || {
        echo "Warning: Dependencies download failed, but will continue with existing files"
    }
else
    echo "Warning: download_dependencies.sh not found, assuming dependencies are already present"
fi

# Second, download po_core if needed
if [[ -f "${SCRIPT_DIR}/download_po_core.sh" ]]; then
    echo ""
    echo "Step 2: Downloading po_core..."
    chmod +x "${SCRIPT_DIR}/download_po_core.sh"
    "${SCRIPT_DIR}/download_po_core.sh" || {
        echo "Warning: po_core download failed, you can download it manually later"
    }
else
    echo "Warning: download_po_core.sh not found, you can download po_core manually later"
fi

echo ""
echo "Step 3: Installing dependencies..."
echo "This will install the following dependencies in order:"
echo "1. CMake 3.28+ - Build system (required by GLOMAP)"
echo "2. Boost - C++ libraries (local compatible version)"
echo "3. Abseil - Google base library"
echo "4. nlohmann/json - JSON library for Modern C++"
echo "5. Protocol Buffers - Google's data interchange format library"
echo "6. OpenCV - Computer vision library with xfeatures2d"
echo "7. Ceres Solver - Nonlinear optimization library"
echo "8. MAGSAC - Robust estimation algorithm library"
echo "9. OpenGV - Geometric vision library"
echo "10. PoseLib - Pose estimation library"
echo "11. OpenMVG - Multiple view geometry library"
echo "12. Colmap - Structure from motion library [🔄 Auto-retry on failure]"
echo "13. GraphOptim - Graph optimization library [🔄 Auto-retry on failure]"
echo "14. Glomap - Structure from motion library"
echo ""
echo "Note: CMake will be checked and upgraded if version < 3.28"
echo "      Colmap and GraphOptim will automatically retry if installation fails"
echo "      (often caused by network issues during git fetch operations)"
echo "注意：CMake 版本低于 3.28 时会自动升级"
echo "      Colmap 和 GraphOptim 如果安装失败会自动重试"
echo "      （通常是网络问题导致的 git fetch 操作失败）"
echo "========================================"

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

# Function to ask user for confirmation
ask_user() {
    local prompt="$1"
    local default="$2"
    local response

    if [[ "$default" == "Y" || "$default" == "y" ]]; then
        read -p "$prompt [Y/n]: " response
        response=${response:-Y}
    else
        read -p "$prompt [y/N]: " response
        response=${response:-N}
    fi

    case "$response" in
        [Yy]* ) return 0 ;;
        [Nn]* ) return 1 ;;
        * )
            echo "Please answer yes (y/Y) or no (n/N)."
            ask_user "$prompt" "$default"
            ;;
    esac
}

# Function to install a dependency
install_dependency() {
    local step_num="$1"
    local total_steps="$2"
    local lib_name="$3"
    local script_name="$4"

    echo ""
    echo "----------------------------------------"
    echo "Step ${step_num}/${total_steps}: Installing ${lib_name}"
    echo "----------------------------------------"

    # Run installation script (each script now handles its own skip detection)
    cd "${SCRIPT_DIR}"
    print_info "Running ${script_name}..."

    # Temporarily disable exit on error for individual script execution
    set +e
    "./${script_name}"
    local exit_code=$?
    set -e

    if [ $exit_code -eq 0 ]; then
        print_success "${lib_name} installation completed"
        return 0
    else
        print_error "${lib_name} installation failed (exit code: $exit_code)"
        if ask_user "Continue with remaining installations?" "Y"; then
            print_warning "Continuing despite ${lib_name} installation failure"
            return 1
        else
            print_error "Installation aborted by user"
            exit 1
        fi
    fi
}

# Function to install a dependency with automatic retry (for network-sensitive installations)
install_dependency_with_retry() {
    local step_num="$1"
    local total_steps="$2"
    local lib_name="$3"
    local script_name="$4"
    local max_retries=2  # 总共尝试2次

    echo ""
    echo "----------------------------------------"
    echo "Step ${step_num}/${total_steps}: Installing ${lib_name}"
    echo "----------------------------------------"

    cd "${SCRIPT_DIR}"
    
    for attempt in $(seq 1 $max_retries); do
        if [ $attempt -gt 1 ]; then
            echo ""
            print_warning "Retrying ${lib_name} installation (attempt ${attempt}/${max_retries})..."
            print_info "网络问题可能导致 fetch 失败，正在重试..."
            sleep 2  # 等待2秒后重试
        fi
        
        print_info "Running ${script_name}... (attempt ${attempt}/${max_retries})"

        # Temporarily disable exit on error for individual script execution
        set +e
        "./${script_name}"
        local exit_code=$?
        set -e

        if [ $exit_code -eq 0 ]; then
            print_success "${lib_name} installation completed"
            return 0
        else
            if [ $attempt -lt $max_retries ]; then
                print_warning "${lib_name} installation failed (exit code: $exit_code)"
                print_info "Will retry automatically (可能是网络问题导致的 fetch 失败)..."
            else
                print_error "${lib_name} installation failed after ${max_retries} attempts (exit code: $exit_code)"
                if ask_user "Continue with remaining installations?" "Y"; then
                    print_warning "Continuing despite ${lib_name} installation failure"
                    return 1
                else
                    print_error "Installation aborted by user"
                    exit 1
                fi
            fi
        fi
    done
    
    return 1
}

# Check if all installation scripts exist
INSTALL_SCRIPTS=(
    "install_cmake.sh"
    "install_boost.sh"
    "install_absl.sh"
    "install_nlohmann.sh"
    "install_protobuf.sh"
    "install_opencv.sh"
    "install_ceres.sh"
    "install_magsac.sh"
    "install_opengv.sh"
    "install_poselib.sh"
    "install_openmvg.sh"
    "install_colmap.sh"
    "install_graphoptim.sh"
    "install_glomap.sh"
)

# Define dependency names
DEPENDENCY_NAMES=(
    "CMake"
    "Boost"
    "Abseil"
    "nlohmann/json"
    "Protocol Buffers"
    "OpenCV"
    "Ceres Solver"
    "MAGSAC"
    "OpenGV"
    "PoseLib"
    "OpenMVG"
    "Colmap"
    "GraphOptim"
    "Glomap"
)

echo "Checking if installation scripts exist..."
for script in "${INSTALL_SCRIPTS[@]}"; do
    if [ ! -f "${SCRIPT_DIR}/${script}" ]; then
        print_error "Installation script ${script} not found"
        echo "Please ensure you run this script from src/dependencies/ directory"
        exit 1
    fi
    echo "✓ Found: ${script}"
done

echo ""
echo "Adding execution permissions to all installation scripts..."

# Add execution permissions
for script in "${INSTALL_SCRIPTS[@]}"; do
    chmod +x "${SCRIPT_DIR}/${script}"
    echo "✓ Added execution permission: ${script}"
done

echo ""
echo "========================================"
echo "Starting dependency installation in order..."
echo "========================================"

# Initialize failure tracking
FAILED_INSTALLATIONS=()
TOTAL_STEPS=${#INSTALL_SCRIPTS[@]}

# Install each dependency
for i in "${!INSTALL_SCRIPTS[@]}"; do
    step_num=$((i + 1))
    lib_name="${DEPENDENCY_NAMES[i]}"
    script_name="${INSTALL_SCRIPTS[i]}"

    # Use retry mechanism for network-sensitive installations (COLMAP, GraphOptim)
    # 对网络敏感的安装使用重试机制（COLMAP, GraphOptim）
    if [[ "$lib_name" == "Colmap" || "$lib_name" == "GraphOptim" ]]; then
        print_info "${lib_name} 将使用自动重试机制（网络敏感）"
        if ! install_dependency_with_retry "$step_num" "$TOTAL_STEPS" "$lib_name" "$script_name"; then
            FAILED_INSTALLATIONS+=("$lib_name")
        fi
    else
        if ! install_dependency "$step_num" "$TOTAL_STEPS" "$lib_name" "$script_name"; then
            FAILED_INSTALLATIONS+=("$lib_name")
        fi
    fi
done

echo ""
echo "========================================"
echo "   Installation Summary"
echo "========================================"

if [ ${#FAILED_INSTALLATIONS[@]} -eq 0 ]; then
    print_success "All dependencies installation completed successfully!"
    echo ""
    echo "Successfully installed the following dependencies:"
    for lib_name in "${DEPENDENCY_NAMES[@]}"; do
        echo "✓ ${lib_name}"
    done
else
    print_warning "Installation completed with some failures:"
    echo ""
    echo "Successfully installed dependencies:"
    for lib_name in "${DEPENDENCY_NAMES[@]}"; do
        if [[ ! " ${FAILED_INSTALLATIONS[@]} " =~ " ${lib_name} " ]]; then
            echo "✓ ${lib_name}"
        fi
    done

    echo ""
    print_error "Failed installations:"
    for failed_lib in "${FAILED_INSTALLATIONS[@]}"; do
        echo "✗ ${failed_lib}"
    done

    echo ""
    print_info "You can re-run individual installation scripts for failed dependencies:"
    for failed_lib in "${FAILED_INSTALLATIONS[@]}"; do
        for i in "${!DEPENDENCY_NAMES[@]}"; do
            if [[ "${DEPENDENCY_NAMES[i]}" == "$failed_lib" ]]; then
                echo "  ./${INSTALL_SCRIPTS[i]}"
                break
            fi
        done
    done
fi

echo ""
echo "Next step: Build PoSDK main project"
echo "  cd ../../build"
echo "  cmake ../src"
echo "  make -j\$(nproc)"
echo ""
echo "If you encounter issues, please check:"
echo "1. Ensure all source directories exist under dependencies/ directory"
echo "2. Ensure necessary build tools are installed (cmake, make, gcc/clang)"
echo "3. Review output logs from individual installation scripts"
echo "4. For failed installations, try running individual scripts manually"
echo "========================================"

exit 0 