#!/usr/bin/env python3
"""
SuperPoint Feature Extractor for PoSDK
使用SuperPoint深度学习模型进行特征提取
"""

import argparse
import sys
import os
import numpy as np
import torch
import cv2
from pathlib import Path

# 添加LightGlue目录到Python路径
script_dir = Path(__file__).parent
# 查找LightGlue安装位置
lightglue_search_paths = [
    script_dir / "../../../dependencies/LightGlue-main",  # 相对于插件目录
    script_dir / "../../dependencies/LightGlue-main",    # 相对于methods目录
    Path.cwd() / "src/dependencies/LightGlue-main",      # 相对于项目根目录
    Path.cwd() / "dependencies/LightGlue-main",          # 备选路径
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
    """保存特征到文件"""
    try:
        with open(output_path, 'w') as f:
            for i in range(keypoints.shape[0]):
                # 写入特征点坐标和分数
                x = keypoints[i, 0].item()
                y = keypoints[i, 1].item()
                score = scores[i].item()
                
                # 写入描述子
                desc = descriptors[i].cpu().numpy()
                
                # 格式：x y score desc0 desc1 ... desc255
                line = f"{x:.6f} {y:.6f} {score:.6f}"
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
