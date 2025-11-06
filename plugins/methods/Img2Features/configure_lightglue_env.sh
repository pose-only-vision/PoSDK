#!/bin/bash

# PoSDK LightGlue Environment Configuration Script
# Configure Python environment for LightGlue and SuperPoint
# Reference implementation from drawer/configure_drawer_env.sh

set -e  # Exit on error

# Script information
SCRIPT_NAME="PoSDK LightGlue Environment Configurator"
VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REQUIREMENTS_FILE="$SCRIPT_DIR/requirements.txt"

# Default configuration
DEFAULT_ENV_NAME="posdk_lightglue_env"
DEFAULT_PYTHON_VERSION="3.9"

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect operating system
detect_os() {
    case "$(uname -s)" in
        Darwin*)
            OS="macos"
            ;;
        Linux*)
            OS="linux"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            OS="windows"
            ;;
        *)
            OS="unknown"
            ;;
    esac
    log_info "Detected operating system: $OS"
}

# Check existing environment
check_existing_environment() {
    log_info "Checking existing Python environments..."
    
    # 1. Check if drawer environment can be reused
    local drawer_env_path="$SCRIPT_DIR/../../../po_core/drawer/conda_env"
    if [[ -d "$drawer_env_path" ]] && [[ -f "$drawer_env_path/bin/python" ]]; then
        log_info "Found drawer environment: $drawer_env_path"
        
        # Check if drawer environment contains necessary packages
        if "$drawer_env_path/bin/python" -c "import torch, numpy, cv2" >/dev/null 2>&1; then
            log_success "Drawer environment contains LightGlue dependencies, will reuse this environment"
            export CONDA_PREFIX="$drawer_env_path"
            export PATH="$drawer_env_path/bin:$PATH"
            return 0
        else
            log_info "Drawer environment lacks LightGlue dependencies, will create dedicated environment"
        fi
    fi
    
    # 2. Check local LightGlue environment
    local lightglue_env_path="$SCRIPT_DIR/conda_env"
    if [[ -d "$lightglue_env_path" ]] && [[ -f "$lightglue_env_path/bin/python" ]]; then
        log_info "Found local LightGlue environment: $lightglue_env_path"
        
        # Verify environment integrity
        if "$lightglue_env_path/bin/python" -c "import torch, numpy, cv2; from lightglue import SuperPoint, LightGlue" >/dev/null 2>&1; then
            log_success "Local LightGlue environment is complete, will use this environment"
            export CONDA_PREFIX="$lightglue_env_path"
            export PATH="$lightglue_env_path/bin:$PATH"
            return 0
        else
            log_warning "Local LightGlue environment is incomplete, will reconfigure"
        fi
    fi
    
    # 3. Check system Python environment
    if command -v python3 >/dev/null 2>&1; then
        if python3 -c "import torch, numpy, cv2; from lightglue import SuperPoint, LightGlue" >/dev/null 2>&1; then
            log_success "System Python environment contains all LightGlue dependencies"
            export PYTHON_CMD="python3"
            return 0
        fi
    fi
    
    log_info "Need to create new LightGlue environment"
    return 1
}

# Create LightGlue environment
create_lightglue_env() {
    local env_path="$SCRIPT_DIR/conda_env"
    
    log_info "Creating dedicated LightGlue environment: $env_path"
    
    # Check if conda is available
    if ! command -v conda >/dev/null 2>&1; then
        log_error "conda is not available, please install conda first"
        return 1
    fi
    
    # If environment already exists, ask whether to recreate
    if [[ -d "$env_path" ]]; then
        log_warning "LightGlue environment already exists"
        
        # Check if in non-interactive mode
        if [[ -t 0 ]]; then
            # Interactive mode: ask user
            read -p "Do you want to recreate the environment? [y/N]: " -r
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                log_info "Removing existing environment..."
                rm -rf "$env_path"
            else
                log_info "Using existing environment"
                export CONDA_PREFIX="$env_path"
                export PATH="$env_path/bin:$PATH"
                return 0
            fi
        else
            # Non-interactive mode: use existing environment
            log_info "Non-interactive mode: using existing environment"
            export CONDA_PREFIX="$env_path"
            export PATH="$env_path/bin:$PATH"
            return 0
        fi
    fi
    
    # Create new environment
    log_info "Creating new conda environment (Python $DEFAULT_PYTHON_VERSION)..."
    if ! conda create -p "$env_path" python="$DEFAULT_PYTHON_VERSION" -y; then
        log_error "Failed to create conda environment"
        return 1
    fi
    
    # Activate environment
    export CONDA_PREFIX="$env_path"
    export PATH="$env_path/bin:$PATH"
    
    log_success "LightGlue environment created successfully: $env_path"
    return 0
}

# Install LightGlue dependencies
install_lightglue_dependencies() {
    local python_cmd="${CONDA_PREFIX}/bin/python"
    local pip_cmd="${CONDA_PREFIX}/bin/pip"
    
    if [[ ! -f "$python_cmd" ]]; then
        python_cmd="python3"
        pip_cmd="pip3"
    fi
    
    log_info "Installing LightGlue dependencies..."
    
    # 1. Install basic dependencies
    if [[ -f "$REQUIREMENTS_FILE" ]]; then
        log_info "Installing dependencies using requirements.txt..."
        if ! "$pip_cmd" install -r "$REQUIREMENTS_FILE"; then
            log_error "Failed to install dependencies from requirements.txt"
            return 1
        fi
    else
        log_info "Installing packages individually..."
        local packages=("torch" "torchvision" "numpy" "opencv-python")
        for package in "${packages[@]}"; do
            log_info "Installing $package..."
            if ! "$pip_cmd" install "$package"; then
                log_error "Failed to install $package"
                return 1
            fi
        done
    fi
    
    # 2. Verify PyTorch installation
    if ! "$python_cmd" -c "import torch; print(f'PyTorch {torch.__version__} installed')" 2>/dev/null; then
        log_error "PyTorch installation verification failed"
        return 1
    fi
    
    # 3. Verify OpenCV installation
    if ! "$python_cmd" -c "import cv2; print(f'OpenCV {cv2.__version__} installed')" 2>/dev/null; then
        log_error "OpenCV installation verification failed"
        return 1
    fi
    
    # 4. Check LightGlue availability
    local lightglue_path="$SCRIPT_DIR/../../../dependencies/LightGlue-main"
    if [[ -d "$lightglue_path" ]]; then
        log_info "Verifying LightGlue availability..."
        if PYTHONPATH="$lightglue_path" "$python_cmd" -c "from lightglue import SuperPoint, LightGlue; print('LightGlue available')" 2>/dev/null; then
            log_success "LightGlue verification successful"
        else
            log_warning "LightGlue verification failed, please check installation"
        fi
    else
        log_warning "LightGlue source directory does not exist: $lightglue_path"
    fi
    
    log_success "Dependency installation completed!"
    return 0
}

# Main configuration function
configure_lightglue_environment() {
    log_info "Starting LightGlue environment configuration..."
    
    # Detect operating system
    detect_os
    
    # Check existing environment
    if check_existing_environment; then
        log_success "Found usable existing environment"
        
        # Verify dependency completeness
        local python_cmd="${CONDA_PREFIX}/bin/python"
        if [[ ! -f "$python_cmd" ]]; then
            python_cmd="${PYTHON_CMD:-python3}"
        fi
        
        if "$python_cmd" -c "import torch, numpy, cv2" >/dev/null 2>&1; then
            log_success "Existing environment dependency verification passed"
            return 0
        else
            log_info "Existing environment dependencies are incomplete, will supplement installation"
        fi
    else
        # Create new environment
        if ! create_lightglue_env; then
            log_error "Failed to create LightGlue environment"
            return 1
        fi
    fi
    
    # Install dependencies
    if ! install_lightglue_dependencies; then
        log_error "Failed to install LightGlue dependencies"
        return 1
    fi
    
    log_success "LightGlue environment configuration completed!"
    
    # Output usage information
    local python_cmd="${CONDA_PREFIX}/bin/python"
    if [[ ! -f "$python_cmd" ]]; then
        python_cmd="${PYTHON_CMD:-python3}"
    fi
    
    log_info "Environment information:"
    log_info "  Python interpreter: $python_cmd"
    log_info "  Environment path: ${CONDA_PREFIX:-"system environment"}"
    
    return 0
}

# Main function
main() {
    # Check help option
    if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
        cat << EOF
$SCRIPT_NAME v$VERSION

Usage: $0

Function: Check and configure Python environment required for LightGlue deep learning feature matching

Return values:
  0  - Environment configuration successful
  1  - Configuration failed

EOF
        exit 0
    fi
    
    log_info "Starting $SCRIPT_NAME v$VERSION"
    
    # Execute environment configuration
    if configure_lightglue_environment; then
        log_success "LightGlue environment configuration completed successfully!"
        exit 0
    else
        log_error "LightGlue environment configuration failed"
        exit 1
    fi
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
