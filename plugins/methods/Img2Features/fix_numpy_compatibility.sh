#!/bin/bash

# Fix NumPy version compatibility issues
# PyTorch 2.2.2 is incompatible with NumPy 2.0.2, need to downgrade to NumPy 1.x

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIGHTGLUE_ENV="$SCRIPT_DIR/conda_env/bin/python"
LIGHTGLUE_PIP="$SCRIPT_DIR/conda_env/bin/pip"

echo "=== Fixing NumPy compatibility issues ==="

# Check if environment exists
if [[ ! -f "$LIGHTGLUE_ENV" ]]; then
    echo "❌ LightGlue environment not found: $LIGHTGLUE_ENV"
    echo "Please run first: bash configure_lightglue_env.sh"
    exit 1
fi

echo "✅ Found LightGlue environment: $LIGHTGLUE_ENV"

# Check current NumPy version
echo "📋 Checking current NumPy version..."
NUMPY_VERSION=$("$LIGHTGLUE_ENV" -c "import numpy; print(numpy.__version__)" 2>/dev/null || echo "Not installed")
echo "   Current NumPy version: $NUMPY_VERSION"

# If NumPy version is 2.x, downgrade to 1.x
if [[ "$NUMPY_VERSION" =~ ^2\. ]]; then
    echo "⚠️  Detected NumPy 2.x version, incompatible with PyTorch"
    echo "🔧 Downgrading NumPy to 1.x version..."
    
    # Downgrade NumPy
    "$LIGHTGLUE_PIP" install "numpy>=1.21.0,<2.0.0" --force-reinstall
    
    echo "✅ NumPy downgrade completed"
else
    echo "✅ NumPy version compatible"
fi

# Install missing dependencies
echo "📦 Checking and installing missing dependencies..."
MISSING_PACKAGES=()

# Check kornia
if ! "$LIGHTGLUE_ENV" -c "import kornia" 2>/dev/null; then
    echo "⚠️  Missing kornia package, installing..."
    "$LIGHTGLUE_PIP" install kornia
    MISSING_PACKAGES+=("kornia")
fi

# Check other potentially missing packages
for package in "pillow"; do
    if ! "$LIGHTGLUE_ENV" -c "import $package" 2>/dev/null; then
        echo "📦 Installing $package..."
        "$LIGHTGLUE_PIP" install "$package"
        MISSING_PACKAGES+=("$package")
    fi
done

if [[ ${#MISSING_PACKAGES[@]} -gt 0 ]]; then
    echo "✅ Installed missing packages: ${MISSING_PACKAGES[*]}"
fi

# Verify fix results
echo "🧪 Verifying environment fix..."
if "$LIGHTGLUE_ENV" -c "import torch, numpy, cv2; print('Environment test passed')" 2>/dev/null; then
    echo "✅ Environment fix successful!"
    
    # Display version information
    echo "📋 Environment information:"
    "$LIGHTGLUE_ENV" -c "
import torch, numpy, cv2
print(f'   PyTorch: {torch.__version__}')
print(f'   NumPy: {numpy.__version__}')  
print(f'   OpenCV: {cv2.__version__}')
"
else
    echo "❌ Environment fix failed"
    exit 1
fi

# Test LightGlue availability
echo "🧪 Testing LightGlue availability..."
LIGHTGLUE_PATH="/Users/caiqi/Documents/PoMVG/src/dependencies/LightGlue-main"
if [[ -d "$LIGHTGLUE_PATH" ]]; then
    if PYTHONPATH="$LIGHTGLUE_PATH" "$LIGHTGLUE_ENV" -c "from lightglue import SuperPoint, LightGlue; print('LightGlue test passed')" 2>/dev/null; then
        echo "✅ LightGlue test successful"
    else
        echo "⚠️  LightGlue test failed, but basic environment has been fixed"
    fi
else
    echo "⚠️  LightGlue source directory not found: $LIGHTGLUE_PATH"
fi

echo ""
echo "🎉 Fix completed! You can now re-run the PoSDK program"
