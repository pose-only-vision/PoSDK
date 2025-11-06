#!/usr/bin/env python3
"""
LightGlue Feature Matching Script for PoSDK
调用LightGlue深度学习匹配器进行特征匹配
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

# 添加LightGlue目录到Python路径
script_dir = Path(__file__).parent
print(f"LightGlue script running from: {script_dir}")
print(f"Current working directory: {Path.cwd()}")

# 查找LightGlue安装位置
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
    from lightglue import LightGlue, SuperPoint, DISK, SIFT as LightGlueSIFT, ALIKED, DoGHardNet
    from lightglue.utils import load_image, rbd
except ImportError as e:
    print(f"Error importing LightGlue modules: {e}")
    print("Please ensure LightGlue is properly installed")
    sys.exit(1)


def parse_arguments():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(description='LightGlue Feature Matching')
    parser.add_argument('--img1', required=True, help='Path to first image')
    parser.add_argument('--img2', required=True, help='Path to second image')
    parser.add_argument('--features1', required=True, help='Path to first image features')
    parser.add_argument('--features2', required=True, help='Path to second image features')
    parser.add_argument('--output', required=True, help='Path to output matches file')
    
    # LightGlue参数
    parser.add_argument('--feature_type', default='superpoint', 
                       choices=['superpoint', 'disk', 'sift', 'aliked', 'doghardnet'],
                       help='Feature type')
    parser.add_argument('--max_keypoints', type=int, default=2048,
                       help='Maximum number of keypoints')
    parser.add_argument('--depth_confidence', type=float, default=0.95,
                       help='Depth confidence threshold')
    parser.add_argument('--width_confidence', type=float, default=0.99,
                       help='Width confidence threshold')
    parser.add_argument('--filter_threshold', type=float, default=0.1,
                       help='Match filter threshold')
    
    # 性能优化选项
    parser.add_argument('--flash_attention', action='store_true',
                       help='Enable FlashAttention')
    parser.add_argument('--mixed_precision', action='store_true',
                       help='Enable mixed precision')
    parser.add_argument('--compile_model', action='store_true',
                       help='Compile model (PyTorch 2.0+)')
    
    return parser.parse_args()


def load_features_from_file(features_path):
    """从文件加载特征点和描述子"""
    try:
        features = []
        descriptors = []
        
        with open(features_path, 'r') as f:
            lines = f.readlines()
            
        if not lines:
            return np.array([]), np.array([])
            
        num_features = int(lines[0].strip())
        
        for i in range(1, min(num_features + 1, len(lines))):
            parts = lines[i].strip().split()
            if len(parts) < 5:
                continue
                
            # 特征点信息: x, y, size, angle, response
            x, y, size, angle, response = map(float, parts[:5])
            features.append([x, y])
            
            # 描述子信息
            if len(parts) > 5:
                desc = list(map(float, parts[5:]))
                descriptors.append(desc)
        
        features = np.array(features, dtype=np.float32)
        descriptors = np.array(descriptors, dtype=np.float32) if descriptors else np.array([])
        
        print(f"Loaded {len(features)} features from {features_path}")
        return features, descriptors
        
    except Exception as e:
        print(f"Error loading features from {features_path}: {e}")
        return np.array([]), np.array([])


def perform_matching(args):
    """执行特征匹配"""
    try:
        # 检查输入文件
        for path in [args.img1, args.img2, args.features1, args.features2]:
            if not os.path.exists(path):
                print(f"Input file not found: {path}")
                return False
        
        # 设备选择
        device = 'cuda' if torch.cuda.is_available() else 'cpu'
        print(f"Using device: {device}")
        
        # 加载图像
        image0 = load_image(args.img1).to(device)
        image1 = load_image(args.img2).to(device)
        
        print(f"Image0 shape: {image0.shape}")
        print(f"Image1 shape: {image1.shape}")
        
        # 加载已有特征
        kpts0, desc0 = load_features_from_file(args.features1)
        kpts1, desc1 = load_features_from_file(args.features2)
        
        if len(kpts0) == 0 or len(kpts1) == 0:
            print("No features loaded from files")
            return False
            
        print(f"Loaded features0: {len(kpts0)} keypoints")
        print(f"Loaded features1: {len(kpts1)} keypoints")
        
        # 转换为torch张量
        kpts0_torch = torch.from_numpy(kpts0).float().to(device).unsqueeze(0)  # (1, N, 2)
        kpts1_torch = torch.from_numpy(kpts1).float().to(device).unsqueeze(0)  # (1, N, 2)
        
        # 如果有描述子数据，使用它们；否则重新提取
        if desc0.size > 0 and desc1.size > 0:
            print(f"Pre-computed descriptors shape: desc0={desc0.shape}, desc1={desc1.shape}")
            
            # 检查描述子维度是否与特征类型匹配
            expected_dim = 256 if args.feature_type == 'superpoint' else 128
            if desc0.shape[1] != expected_dim:
                print(f"WARNING: Descriptor dimension mismatch!")
                print(f"  Expected {expected_dim}D for {args.feature_type}, got {desc0.shape[1]}D")
                print(f"  This suggests features were extracted with a different detector")
                
                # 自动调整特征类型
                if desc0.shape[1] == 128:
                    args.feature_type = 'sift'
                    print(f"  Auto-corrected feature_type to 'sift'")
                elif desc0.shape[1] == 256:
                    args.feature_type = 'superpoint' 
                    print(f"  Auto-corrected feature_type to 'superpoint'")
            
            desc0_torch = torch.from_numpy(desc0).float().to(device).unsqueeze(0)  # (1, N, D)
            desc1_torch = torch.from_numpy(desc1).float().to(device).unsqueeze(0)  # (1, N, D)
            print("Using pre-computed descriptors")
        else:
            # 重新提取描述子
            print("Re-extracting descriptors using LightGlue extractor")
            extractor, _, _ = create_extractor_and_matcher(args)
            
            # 构造特征数据结构
            feats0 = {'keypoints': kpts0_torch, 'image_size': torch.tensor(image0.shape[-2:], device=device).unsqueeze(0)}
            feats1 = {'keypoints': kpts1_torch, 'image_size': torch.tensor(image1.shape[-2:], device=device).unsqueeze(0)}
            
            with torch.no_grad():
                # 使用提取器重新计算描述子
                feats0 = extractor.extract(image0, feats0)
                feats1 = extractor.extract(image1, feats1)
                desc0_torch = feats0['descriptors']
                desc1_torch = feats1['descriptors']
        
        # 调试信息：检查张量维度
        print(f"kpts0_torch shape: {kpts0_torch.shape}")
        print(f"kpts1_torch shape: {kpts1_torch.shape}")
        print(f"desc0_torch shape: {desc0_torch.shape}")
        print(f"desc1_torch shape: {desc1_torch.shape}")
        print(f"image0 shape: {image0.shape}")
        print(f"image1 shape: {image1.shape}")
        
        # 构造完整的特征数据
        feats0 = {
            'keypoints': kpts0_torch,
            'descriptors': desc0_torch,
            'image_size': torch.tensor(image0.shape[-2:], device=device).unsqueeze(0)  # 修复：移除额外的[]
        }
        feats1 = {
            'keypoints': kpts1_torch, 
            'descriptors': desc1_torch,
            'image_size': torch.tensor(image1.shape[-2:], device=device).unsqueeze(0)  # 修复：移除额外的[]
        }
        
        print(f"feats0 image_size: {feats0['image_size']}")
        print(f"feats1 image_size: {feats1['image_size']}")
        
        # 创建匹配器
        matcher_args = {
            'features': args.feature_type,
            'depth_confidence': args.depth_confidence,
            'width_confidence': args.width_confidence,
            'filter_threshold': args.filter_threshold,
        }
        
        if args.flash_attention:
            matcher_args['flash'] = True
        if args.mixed_precision:
            matcher_args['mp'] = True
            
        matcher = LightGlue(**matcher_args).eval().to(device)
        
        # 编译模型（如果支持）
        if args.compile_model and hasattr(torch, 'compile'):
            try:
                matcher = torch.compile(matcher, mode='reduce-overhead')
                print("Model compiled successfully")
            except Exception as e:
                print(f"Model compilation failed: {e}")
        
        # 执行匹配
        with torch.no_grad():
            print("🔥 Starting LightGlue matching...")
            print(f"   Matcher feature type: {args.feature_type}")
            print(f"   feats0 keys: {feats0.keys()}")
            print(f"   feats1 keys: {feats1.keys()}")
            
            # 最后一次检查特征数据格式
            for i, (name, feats) in enumerate([('feats0', feats0), ('feats1', feats1)]):
                print(f"   {name}:")
                for key, tensor in feats.items():
                    if isinstance(tensor, torch.Tensor):
                        print(f"     {key}: shape={tensor.shape}, dtype={tensor.dtype}")
                    else:
                        print(f"     {key}: {type(tensor)} = {tensor}")
            
            try:
                matches01 = matcher({'image0': feats0, 'image1': feats1})
                feats0, feats1, matches01 = [rbd(x) for x in [feats0, feats1, matches01]]
            except RuntimeError as e:
                print(f"❌ LightGlue matching failed with RuntimeError: {e}")
                print("This might be due to:")
                print("1. Feature dimension mismatch")
                print("2. Incorrect tensor shapes")
                print("3. Model/feature type incompatibility")
                raise
            
            matches = matches01['matches']  # 匹配索引，形状为(K, 2)
            match_confidence = matches01['matching_scores0']  # 匹配置信度
            
            print(f"Found {matches.shape[0]} matches")
        
        # 保存匹配结果
        save_matches(matches, match_confidence, args.output)
        return True
        
    except Exception as e:
        print(f"Error in matching: {e}")
        import traceback
        traceback.print_exc()
        return False


def create_extractor_and_matcher(args):
    """创建特征提取器和匹配器"""
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    print(f"Using device: {device}")
    
    # 创建特征提取器
    extractor_args = {'max_num_keypoints': args.max_keypoints}
    
    if args.feature_type == 'superpoint':
        extractor = SuperPoint(**extractor_args).eval().to(device)
    elif args.feature_type == 'disk':
        extractor = DISK(**extractor_args).eval().to(device)  
    elif args.feature_type == 'sift':
        extractor = LightGlueSIFT(**extractor_args).eval().to(device)
    elif args.feature_type == 'aliked':
        extractor = ALIKED(**extractor_args).eval().to(device)
    elif args.feature_type == 'doghardnet':
        extractor = DoGHardNet(**extractor_args).eval().to(device)
    else:
        raise ValueError(f"Unsupported feature type: {args.feature_type}")
    
    return extractor, None, device


def save_matches(matches, confidence, output_path):
    """保存匹配结果到文件"""
    try:
        with open(output_path, 'w') as f:
            for i in range(matches.shape[0]):
                if matches[i, 0] != -1 and matches[i, 1] != -1:  # 有效匹配
                    query_idx = matches[i, 0].item()
                    train_idx = matches[i, 1].item()
                    conf = confidence[i].item() if i < len(confidence) else 1.0
                    
                    # 转换置信度为距离（距离越小越好）
                    distance = 1.0 - conf
                    
                    f.write(f"{query_idx} {train_idx} {distance:.6f}\n")
        
        print(f"Matches saved to {output_path}")
        
    except Exception as e:
        print(f"Error saving matches: {e}")


def main():
    """主函数"""
    args = parse_arguments()
    
    print("LightGlue Feature Matching Started")
    print(f"Feature type: {args.feature_type}")
    print(f"Max keypoints: {args.max_keypoints}")
    print(f"Device: {'CUDA' if torch.cuda.is_available() else 'CPU'}")
    
    # 执行匹配
    if perform_matching(args):
        print("Feature matching completed successfully")
        return 0
    else:
        print("Feature matching failed")
        return 1


if __name__ == '__main__':
    sys.exit(main())
