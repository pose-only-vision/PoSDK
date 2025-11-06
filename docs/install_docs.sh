#!/bin/bash
# PoSDK Documentation Environment Setup & Build Script
# One-click setup for building PoSDK documentation with benchmark tables
# Supports both English and Chinese documentation

set -e  # Exit on any error

# Language selection
LANG_SELECTED=""
if [ -n "$1" ]; then
    LANG_SELECTED="$1"
elif [ -t 0 ]; then
    # Interactive mode: prompt user
    echo "Please select documentation language:"
    echo "  1) English (source)"
    echo "  2) Chinese (source_zh)"
    echo ""
    read -p "Enter choice [1 or 2] (default: 1): " LANG_SELECTED
    LANG_SELECTED=${LANG_SELECTED:-1}
else
    # Non-interactive mode: use default
    LANG_SELECTED="1"
    echo "Non-interactive mode detected. Using default: English (source)"
fi

# Set source directory based on language selection
case "$LANG_SELECTED" in
    1|en|EN|english|English)
        SOURCEDIR="source"
        BUILDDIR="build"
        LANG_NAME="English"
        ;;
    2|zh|ZH|chinese|Chinese)
        SOURCEDIR="source_zh"
        BUILDDIR="build_zh"
        LANG_NAME="Chinese"
        ;;
    *)
        echo "Error: Invalid language selection: $LANG_SELECTED"
        echo "Usage: $0 [1|2|en|zh]"
        echo "  1 or en: English documentation (source)"
        echo "  2 or zh: Chinese documentation (source_zh)"
        exit 1
        ;;
esac

echo "=================================================="
echo "Setting up PoSDK documentation environment..."
echo "Language: $LANG_NAME"
echo "Source directory: $SOURCEDIR"
echo "Build directory: $BUILDDIR"
echo "=================================================="
echo ""

# Verify source directory exists
if [ ! -d "$SOURCEDIR" ]; then
    echo "Error: Source directory '$SOURCEDIR' does not exist!"
    exit 1
fi

# Detect OS type
OS_TYPE="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
fi

# Check and install system dependencies
echo "Checking system dependencies..."

# Check Python3
if ! command -v python3 &> /dev/null; then
    echo "Error: Python3 is not installed!"
    if [ "$OS_TYPE" == "linux" ]; then
        echo "Installing python3..."
        if sudo apt-get update && sudo apt-get install -y python3; then
            echo "Python3 installed successfully"
        else
            echo "Failed to install Python3. Please install manually:"
            echo "   sudo apt-get install -y python3"
            exit 1
        fi
    else
        echo "Please install Python3 first: https://www.python.org/downloads/"
        exit 1
    fi
fi

# Check Python3 version (require >= 3.6)
PYTHON_VERSION=$(python3 -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
PYTHON_MAJOR=$(echo $PYTHON_VERSION | cut -d. -f1)
PYTHON_MINOR=$(echo $PYTHON_VERSION | cut -d. -f2)

if [ "$PYTHON_MAJOR" -lt 3 ] || ([ "$PYTHON_MAJOR" -eq 3 ] && [ "$PYTHON_MINOR" -lt 6 ]); then
    echo "Error: Python 3.6+ is required. Found: $PYTHON_VERSION"
    exit 1
fi

echo "Python3 version: $PYTHON_VERSION"

# Check and install python3-venv (Ubuntu/Debian)
if [ "$OS_TYPE" == "linux" ]; then
    if ! python3 -m venv --help &> /dev/null; then
        echo "Installing python3-venv..."
        if sudo apt-get update && sudo apt-get install -y python3-venv python3-pip; then
            echo "python3-venv installed successfully"
        else
            echo "Failed to install python3-venv. Please install manually:"
            echo "   sudo apt-get install -y python3-venv python3-pip"
            exit 1
        fi
    fi
fi

# Check and install pip if needed
if ! command -v pip3 &> /dev/null && ! python3 -m pip --version &> /dev/null; then
    echo "Installing pip..."
    if [ "$OS_TYPE" == "linux" ]; then
        if sudo apt-get update && sudo apt-get install -y python3-pip; then
            echo "pip installed successfully"
        else
            echo "Failed to install pip. Please install manually:"
            echo "   sudo apt-get install -y python3-pip"
            exit 1
        fi
    else
        echo "Please install pip first"
        exit 1
    fi
fi

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv venv
    
    # Verify venv was created successfully
    if [ ! -f "venv/bin/activate" ]; then
        echo "Error: Failed to create virtual environment!"
        echo "   venv/bin/activate file not found"
        exit 1
    fi
    echo "Virtual environment created successfully"
else
    echo "Virtual environment already exists"
fi

# Activate virtual environment
echo "Activating virtual environment..."
if [ -f "venv/bin/activate" ]; then
    source venv/bin/activate
else
    echo "Error: Virtual environment activation script not found!"
    echo "   Expected: venv/bin/activate"
    exit 1
fi

# Verify activation
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Error: Failed to activate virtual environment!"
    exit 1
fi

echo "Virtual environment activated: $VIRTUAL_ENV"

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip setuptools wheel

# Install documentation dependencies
if [ ! -f "requirements.txt" ]; then
    echo "Warning: requirements.txt not found!"
    echo "   Skipping dependency installation"
else
    echo "Installing documentation dependencies..."
    pip install -r requirements.txt
fi

echo "Documentation environment setup completed!"
echo ""

# Clean previous builds
echo "Cleaning previous builds..."
make clean SOURCEDIR="$SOURCEDIR" BUILDDIR="$BUILDDIR" || echo "Warning: make clean failed (may be normal if no previous build)"

# Build documentation with benchmark tables
echo "Building $LANG_NAME documentation with benchmark tables..."
echo "Source directory: $SOURCEDIR"
echo "Build directory: $BUILDDIR"
echo ""

# Generate benchmark tables first (always run to ensure tables are up-to-date)
    echo "Generating benchmark comparison tables..."
# Benchmark data is shared between source and source_zh, always use source directory
BENCHMARK_SOURCE_DIR="source"
TABLE_GENERATOR="$BENCHMARK_SOURCE_DIR/benchmark_comparison/scripts/generate_tables.py"
if [ -f "$TABLE_GENERATOR" ]; then
    # Use python from venv (virtual environment is already activated)
    # The script uses __file__ to calculate paths, so it can run from any directory
    if python "$TABLE_GENERATOR"; then
        echo "✓ Benchmark tables generated successfully!"
        
        # Copy generated tables to target source directory if different
        if [ "$SOURCEDIR" != "$BENCHMARK_SOURCE_DIR" ]; then
            echo "Copying generated tables to $SOURCEDIR/benchmark_comparison/processed/..."
            if [ -d "$BENCHMARK_SOURCE_DIR/benchmark_comparison/processed" ]; then
                mkdir -p "$SOURCEDIR/benchmark_comparison/processed"
                cp -r "$BENCHMARK_SOURCE_DIR/benchmark_comparison/processed"/* "$SOURCEDIR/benchmark_comparison/processed/" 2>/dev/null || true
                echo "✓ Tables copied to $SOURCEDIR"
            fi
        fi
    else
        echo "Error: Failed to generate benchmark tables!"
        echo "   Please check the error messages above"
        exit 1
    fi
else
    echo "Warning: Benchmark table generator not found at:"
    echo "   $TABLE_GENERATOR"
    echo "   Skipping table generation..."
    echo "   (This may be normal if you're building documentation without benchmark data)"
fi

# Build HTML documentation
echo "Building HTML documentation..."
make html SOURCEDIR="$SOURCEDIR" BUILDDIR="$BUILDDIR"

echo ""
echo "Documentation build completed successfully!"
echo ""
echo "View documentation:"
echo "  • Open: $BUILDDIR/html/index.html"
echo "  • Or run: make livehtml SOURCEDIR=$SOURCEDIR BUILDDIR=$BUILDDIR (for live preview)"
echo ""
echo "For collaboration: Edit Markdown files in $SOURCEDIR/ directory and submit PRs!"
