#!/usr/bin/env python3
"""
简单的LightGlue环境测试脚本
"""

import sys
from pathlib import Path

def test_basic_imports():
    """测试基础包导入"""
    try:
        import numpy as np
        import torch
        import cv2
        print(f"✅ Basic packages OK")
        print(f"   NumPy: {np.__version__}")
        print(f"   PyTorch: {torch.__version__}")
        print(f"   OpenCV: {cv2.__version__}")
        return True
    except ImportError as e:
        print(f"❌ Basic packages failed: {e}")
        return False

def test_lightglue_imports():
    """测试LightGlue导入"""
    try:
        # 查找LightGlue安装位置
        script_dir = Path(__file__).parent
        lightglue_search_paths = [
            script_dir / "../../../dependencies/LightGlue-main",
            script_dir / "../../dependencies/LightGlue-main",
            script_dir / "../dependencies/LightGlue-main",
            Path.cwd() / "src/dependencies/LightGlue-main",
            Path.cwd() / "dependencies/LightGlue-main",
        ]

        lightglue_dir = None
        for path in lightglue_search_paths:
            if (path / "lightglue" / "__init__.py").exists():
                lightglue_dir = path
                print(f"✅ Found LightGlue at: {lightglue_dir}")
                break

        if lightglue_dir is None:
            print("❌ LightGlue installation not found!")
            return False

        sys.path.insert(0, str(lightglue_dir))
        
        from lightglue import SuperPoint, LightGlue
        from lightglue.utils import load_image, rbd
        
        print("✅ LightGlue imports OK")
        return True
    except ImportError as e:
        print(f"❌ LightGlue imports failed: {e}")
        return False

def main():
    print("=== LightGlue Environment Test ===")
    print(f"Python executable: {sys.executable}")
    print(f"Python version: {sys.version}")
    print("")
    
    # 测试基础包
    basic_ok = test_basic_imports()
    print("")
    
    # 测试LightGlue
    lightglue_ok = test_lightglue_imports()
    print("")
    
    if basic_ok and lightglue_ok:
        print("🎉 Environment test passed! LightGlue is ready to use.")
        return 0
    else:
        print("❌ Environment test failed!")
        if not basic_ok:
            print("   Please install: pip install torch torchvision numpy opencv-python")
        if not lightglue_ok:
            print("   Please check LightGlue installation in dependencies/")
        return 1

if __name__ == '__main__':
    sys.exit(main())
