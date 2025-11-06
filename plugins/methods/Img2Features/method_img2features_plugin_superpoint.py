#!/usr/bin/env python3
"""
SuperPoint Feature Extractor for Img2Features Plugin
使用SuperPoint深度学习模型进行特征提取
"""

import argparse
import sys
import os
import subprocess
from pathlib import Path

# 环境检查和配置
def check_and_setup_environment():
    """检查并配置LightGlue环境"""
    script_dir = Path(__file__).parent
    
    # 1. 首先检查是否有drawer环境可用
    drawer_env_paths = [
        # 从output/plugins/methods/出发查找drawer环境
        script_dir / "../../po_core/drawer/conda_env/bin/python",
        script_dir / "../../../po_core/drawer/conda_env/bin/python", 
        script_dir / "../../../../po_core/drawer/conda_env/bin/python",
        script_dir / "../../../../../po_core/drawer/conda_env/bin/python",
        
        # 绝对路径尝试（如果相对路径不工作）
        Path("/Users/caiqi/Documents/PoMVG/po_core/drawer/conda_env/bin/python"),
    ]
    
    for env_path in drawer_env_paths:
        if env_path.exists():
            print(f"Found drawer environment: {env_path}")
            # 测试环境是否包含基础包
            try:
                result = subprocess.run([str(env_path), "-c", "import numpy, torch, cv2"], 
                                      capture_output=True, timeout=10)
                if result.returncode == 0:
                    print("Drawer environment contains required packages")
                    # 更新系统路径
                    env_bin_dir = str(env_path.parent)
                    os.environ['PATH'] = f"{env_bin_dir}:{os.environ.get('PATH', '')}"
                    
                    # 重新尝试导入
                    try:
                        import sys
                        sys.path.insert(0, env_bin_dir)
                        import numpy as np
                        import torch
                        import cv2
                        print("Successfully using drawer environment")
                        return True
                    except ImportError:
                        continue
            except (subprocess.TimeoutExpired, Exception):
                continue
    
    # 2. 尝试导入必要的包（系统环境）
    try:
        import numpy as np
        import torch
        import cv2
        print("Basic packages available in system environment")
        return True
    except ImportError as e:
        print(f"Missing dependencies in system environment: {e}")
        
        # 3. 提供安装指导
        print("Please install dependencies using one of these methods:")
        print("Method 1 - pip install:")
        print("  pip install torch torchvision numpy opencv-python")
        print("")
        print("Method 2 - conda install:")
        print("  conda install pytorch torchvision numpy opencv -c pytorch")
        print("")
        print("Method 3 - use drawer environment:")
        print("  cd po_core/drawer && bash configure_drawer_env.sh")
        return False

# 在导入其他模块前先检查环境
if not check_and_setup_environment():
    print("ERROR: Failed to setup LightGlue environment")
    print("Please manually install dependencies:")
    print("  pip install torch torchvision numpy opencv-python")
    sys.exit(1)

# 现在安全地导入所需模块
import numpy as np
import torch
import cv2

# 查找LightGlue安装位置
script_dir = Path(__file__).parent
print(f"Script running from: {script_dir}")
print(f"Current working directory: {Path.cwd()}")

lightglue_search_paths = [
    # 绝对路径（最可靠）
    Path("/Users/caiqi/Documents/PoMVG/src/dependencies/LightGlue-main"),
    
    # 从script当前位置计算的相对路径（脚本在output/plugins/methods/时）
    script_dir / "../../../../../src/dependencies/LightGlue-main",
    script_dir / "../../../../src/dependencies/LightGlue-main", 
    script_dir / "../../../src/dependencies/LightGlue-main",
    script_dir / "../../src/dependencies/LightGlue-main",
    
    # 从当前工作目录的相对路径
    Path.cwd() / "src/dependencies/LightGlue-main",       
    Path.cwd() / "dependencies/LightGlue-main",           
    
    # 其他可能的位置
    script_dir / "../../../dependencies/LightGlue-main",   
    script_dir / "../../dependencies/LightGlue-main",     
    script_dir / "../dependencies/LightGlue-main",        
]

lightglue_dir = None
for path in lightglue_search_paths:
    if (path / "lightglue" / "__init__.py").exists():
        lightglue_dir = path
        print(f"Found LightGlue at: {lightglue_dir}")
        break

if lightglue_dir is None:
    print("ERROR: LightGlue installation not found!")
    print("Please ensure LightGlue is installed at one of these locations:")
    for path in lightglue_search_paths:
        print(f"  - {path}")
    sys.exit(1)

sys.path.insert(0, str(lightglue_dir))

try:
    from lightglue import SuperPoint
    from lightglue.utils import load_image, rbd
except ImportError as e:
    print(f"Error importing LightGlue modules: {e}")
    print("Please ensure LightGlue is properly installed")
    sys.exit(1)


def parse_arguments():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(description='SuperPoint Feature Extraction')
    parser.add_argument('--image', required=True, help='Path to input image')
    parser.add_argument('--output', required=True, help='Path to output features file')
    
    # SuperPoint参数
    parser.add_argument('--max_keypoints', type=int, default=2048,
                       help='Maximum number of keypoints')
    parser.add_argument('--detection_threshold', type=float, default=0.0005,
                       help='Detection threshold')
    parser.add_argument('--nms_radius', type=int, default=4,
                       help='NMS radius')
    parser.add_argument('--remove_borders', type=int, default=4,
                       help='Remove borders')
    
    return parser.parse_args()


def extract_superpoint_features(args):
    """提取SuperPoint特征"""
    try:
        # 检查输入文件
        if not os.path.exists(args.image):
            print(f"Input image not found: {args.image}")
            return False
            
        # 设备选择
        device = 'cuda' if torch.cuda.is_available() else 'cpu'
        print(f"Using device: {device}")
        
        # 创建SuperPoint提取器
        extractor = SuperPoint(
            max_num_keypoints=args.max_keypoints,
            detection_threshold=args.detection_threshold,
            nms_radius=args.nms_radius,
            remove_borders=args.remove_borders
        ).eval().to(device)
        
        print(f"SuperPoint extractor created with max_keypoints={args.max_keypoints}")
        
        # 加载图像
        image = load_image(args.image).to(device)
        print(f"Image loaded with shape: {image.shape}")
        
        # 提取特征
        with torch.no_grad():
            features = extractor.extract(image)
            features = rbd(features)  # 移除batch维度
            
            keypoints = features['keypoints']  # (N, 2)
            descriptors = features['descriptors']  # (N, 256)
            scores = features['keypoint_scores']  # (N,)
            
            print(f"Extracted {keypoints.shape[0]} keypoints")
        
        # 保存特征
        save_features(keypoints, descriptors, scores, args.output)
        return True
        
    except Exception as e:
        print(f"Error in feature extraction: {e}")
        import traceback
        traceback.print_exc()
        return False


def save_features(keypoints, descriptors, scores, output_path):
    """保存特征到文件，格式与method_img2features_plugin兼容"""
    try:
        with open(output_path, 'w') as f:
            # 第一行：特征点数量
            f.write(f"{keypoints.shape[0]}\n")
            
            # 后续行：特征点信息和描述子
            for i in range(keypoints.shape[0]):
                x = keypoints[i, 0].item()
                y = keypoints[i, 1].item()
                score = scores[i].item()
                
                # 写入特征点信息: x, y, size, angle, response + 256维描述子
                # 格式：x y size angle response desc0 desc1 ... desc255
                desc = descriptors[i].cpu().numpy()
                
                line = f"{x:.6f} {y:.6f} 1.0 -1.0 {score:.6f}"
                for d in desc:
                    line += f" {d:.6f}"
                line += "\n"
                
                f.write(line)
        
        print(f"Features saved to {output_path}")
        
    except Exception as e:
        print(f"Error saving features: {e}")


def main():
    """主函数"""
    args = parse_arguments()
    
    print("SuperPoint Feature Extraction Started")
    print(f"Input image: {args.image}")
    print(f"Output file: {args.output}")
    print(f"Max keypoints: {args.max_keypoints}")
    print(f"Detection threshold: {args.detection_threshold}")
    
    # 执行特征提取
    if extract_superpoint_features(args):
        print("SuperPoint feature extraction completed successfully")
        return 0
    else:
        print("SuperPoint feature extraction failed")
        return 1


if __name__ == '__main__':
    sys.exit(main())
