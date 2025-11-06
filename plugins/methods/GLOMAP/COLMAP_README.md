# COLMAP 增量式重建 Pipeline

这个工具提供了一个完整的 COLMAP 增量式三维重建流程，基于 PyColmap 实现。

## 功能特点

- **完整的增量式重建流程**: 从图像到3D模型的一站式处理
- **灵活的参数配置**: 支持不同特征类型、匹配策略等
- **密集重建支持**: 可选的密集点云生成
- **详细的日志输出**: 实时监控重建进度
- **错误处理**: 完善的异常处理和状态反馈

## 环境要求

### 系统要求
- Ubuntu 20.04+ (ARM64/aarch64)
- Python 3.8+
- 足够的存储空间（建议至少预留图像总大小的3-5倍空间）

### 依赖安装

1. **安装 COLMAP 环境**:
   ```bash
   # 运行安装脚本（会自动安装到 dependencies 目录）
   cd dependencies
   ./install_colmap.sh
   ```

2. **验证安装**:
   ```bash
   # 快速测试安装（推荐）
   cd plugins/methods
   python test_colmap_install.py
   
   # 或运行完整示例
   python colmap_example.py
   ```

**注意**: Python脚本会自动检测和激活 dependencies 目录中的conda环境，无需手动激活。

## 使用方法

### 1. 基本用法

```python
from colmap_pipeline import colmap_pipeline

# 基本重建
success = colmap_pipeline(
    image_folder="/path/to/images",    # 输入图像文件夹
    output_folder="/path/to/output"    # 输出文件夹
)
```

### 2. 完整参数示例

```python
success = colmap_pipeline(
    image_folder="/path/to/images",
    output_folder="/path/to/output",
    camera_model='PINHOLE',           # 相机模型: 'PINHOLE', 'RADIAL', 'OPENCV', etc.
    num_threads=8,                    # 线程数
    max_image_size=3200,              # 最大图像尺寸
    max_num_features=8192             # 最大特征点数
)
```

### 3. 命令行使用

```bash
# 进入方法目录
cd plugins/methods

# 基本重建（会自动激活环境）
python colmap_pipeline.py --image_folder /path/to/images --output_folder /path/to/output

# 带参数的重建
python colmap_pipeline.py \
    --image_folder /path/to/images \
    --output_folder /path/to/output \
    --camera_model PINHOLE \
    --num_threads 8 \
    --max_image_size 2000
```

### 4. 运行示例

```bash
# 进入方法目录
cd plugins/methods

# 运行交互式示例（会自动激活环境）
python colmap_example.py
```

## 输出结果结构

```
output_folder/
├── images/              # 复制的输入图像
├── database.db          # COLMAP 数据库文件
├── sparse/              # 稀疏重建结果
│   └── 0/
│       ├── cameras.txt  # 相机参数
│       ├── images.txt   # 图像信息和相机姿态
│       └── points3D.txt # 3D点云
└── dense/              # 密集重建结果 (可选)
    ├── images/         # 去畸变图像
    ├── stereo/         # 立体匹配结果
    └── fused.ply       # 融合点云
```

## 参数说明

### 特征类型 (feature_type)
- **sift**: SIFT特征 (推荐) - 精度高，适合大多数场景
- **orb**: ORB特征 - 速度快，适合实时应用

### 匹配类型 (match_type)
- **exhaustive**: 穷举匹配 - 适合小规模数据集 (<100张图像)
- **sequential**: 序列匹配 - 适合视频序列或有序图像

### 图像尺寸建议
- **稀疏重建**: 1600-3200像素 (平衡精度与速度)
- **密集重建**: 1000-2000像素 (避免内存不足)

## 使用场景

### 1. 小规模物体重建
```python
colmap_pipeline(
    image_folder="object_photos/",
    output_folder="object_3d/",
    camera_model='PINHOLE',
    max_image_size=2000,
    max_num_features=16384
)
```

### 2. 建筑物/场景重建
```python
colmap_pipeline(
    image_folder="building_photos/",
    output_folder="building_3d/",
    camera_model='RADIAL',
    max_image_size=3200,
    num_threads=16
)
```

### 3. 高质量重建
```python
colmap_pipeline(
    image_folder="high_quality_photos/",
    output_folder="hq_3d/",
    camera_model='OPENCV',
    max_image_size=4000,
    max_num_features=32768,
    max_ratio=0.6
)
```

## 注意事项

### 1. 图像质量要求
- **清晰度**: 避免模糊的图像
- **重叠度**: 相邻图像重叠度应>60%
- **光照**: 尽量保持一致的光照条件
- **纹理**: 确保目标物体有足够的纹理信息

### 2. 性能优化
- **内存**: 密集重建需要较大内存，建议16GB+
- **存储**: 预留充足的磁盘空间
- **CPU**: 多核CPU可加速特征提取和匹配

### 3. 常见问题

**问题**: 重建失败，没有生成模型
**解决**: 
- 检查图像质量和重叠度
- 降低 `min_num_matches` 参数
- 尝试不同的特征类型

**问题**: 内存不足
**解决**:
- 降低 `max_image_size` 参数
- 禁用密集重建
- 分批处理图像

**问题**: 特征匹配时间过长
**解决**:
- 减少图像数量
- 使用序列匹配 (`sequential`)
- 降低图像分辨率

## 高级用法

### 1. 自定义重建参数

```python
# 修改 incremental_reconstruction 函数中的选项
options = pycolmap.IncrementalMapperOptions()
options.min_num_matches = 10           # 降低最小匹配数要求
options.init_min_tri_angle = 8.0       # 降低初始三角化角度
options.multiple_models = True         # 允许多个模型
```

### 2. 结合其他工具

```python
# 重建完成后，可以使用其他工具进一步处理
import subprocess

def post_process_point_cloud(ply_file):
    # 使用 MeshLab 或其他工具处理点云
    subprocess.run([
        'meshlabserver', '-i', ply_file, 
        '-o', ply_file.replace('.ply', '_cleaned.ply'),
        '-s', 'cleanup.mlx'
    ])
```

## 相关链接

- [COLMAP 官方文档](https://colmap.github.io/)
- [PyColmap 文档](https://github.com/colmap/pycolmap)
- [三维重建基础教程](https://github.com/colmap/colmap/wiki)

## 技术支持

如果遇到问题，请提供以下信息：
1. 错误信息和日志
2. 输入图像数量和分辨率
3. 系统配置信息
4. 使用的参数设置 