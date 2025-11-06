# Img2Features Plugin

## 概述

Img2Features 插件是一个图像特征提取处理流水线，支持多种特征检测器，包括传统特征（SIFT、ORB等）和深度学习特征（SuperPoint）。

## 功能特性

### 支持的特征检测器
- **SIFT**: 经典SIFT特征，128维描述子
- **ORB**: 二进制特征，快速检测
- **AKAZE**: 加速KAZE特征
- **BRISK**: 二进制鲁棒不变特征
- **KAZE**: 非线性尺度空间特征
- **FAST**: 快速角点检测器
- **AGAST**: 自适应通用角点分割测试
- **SuperPoint**: 深度学习特征，256维描述子

### 运行模式
- **快速模式**: 批量处理所有图像，高效完成特征提取
- **可视化模式**: 交互式参数调整，实时预览特征提取效果

## 目录结构

```
Img2Features/
├── CMakeLists.txt                           # 构建配置文件
├── README.md                                # 本文档
├── method_img2features.ini                  # 插件配置文件
├── img2features_pipeline.hpp               # 主要类声明
├── img2features_pipeline.cpp               # 主要类实现
└── method_img2features_plugin_superpoint.py # SuperPoint特征提取Python脚本
```

## 配置参数

### 基础配置
- `ProfileCommit`: 配置修改说明
- `enable_profiling`: 是否启用性能分析
- `enable_evaluator`: 是否启用评估
- `export_features`: 是否导出特征文件
- `export_fea_path`: 特征导出路径
- `run_mode`: 运行模式（fast=快速, viewer=可视化）
- `detector_type`: 特征检测器类型

### 特征检测器配置
每种特征检测器都有独立的配置段：

#### SIFT配置
- `nfeatures`: 特征点数量限制
- `nOctaveLayers`: octave层数
- `contrastThreshold`: 对比度阈值
- `edgeThreshold`: 边缘阈值
- `sigma`: 高斯模糊系数

#### SuperPoint配置
- `max_keypoints`: 最大特征点数量
- `detection_threshold`: 检测阈值
- `nms_radius`: 非极大值抑制半径
- `remove_borders`: 移除边界像素数
- `python_executable`: Python可执行文件路径

## 使用方法

### 快速模式
```cpp
auto plugin = FactoryMethod::Create("method_img2features");
plugin->SetMethodOptions({{"run_mode", "fast"}});
auto result = plugin->Run();
```

### SuperPoint特征提取
```cpp
auto plugin = FactoryMethod::Create("method_img2features");
plugin->SetMethodOptions({
    {"detector_type", "SUPERPOINT"},
    {"run_mode", "fast"}
});
auto result = plugin->Run();
```

## 输入输出

### 输入数据
- `data_images`: 图像路径数据 (ImagePaths)

### 输出数据
- 特征数据 (FeaturesInfo)

## Python脚本集成

### SuperPoint特征提取
插件通过`method_img2features_plugin_superpoint.py`脚本调用LightGlue的SuperPoint模型：

1. **自动安装**: 构建系统会自动将Python脚本安装到`plugins/methods/`目录
2. **自动发现**: 插件会自动查找脚本位置
3. **环境检查**: 自动验证Python和PyTorch环境
4. **降级机制**: SuperPoint失败时自动降级到SIFT

### 环境要求
```bash
pip install torch torchvision numpy opencv-python
```

## 兼容性

该插件重构自原有的单文件插件结构，保持完全的向后兼容性：
- 插件名称仍为 `method_img2features`
- 配置文件格式保持不变
- API接口保持一致

## 依赖项

- OpenCV (特征检测)
- PoSDK Core (数据结构和接口)
- Common Converter (数据转换)
- Common ImageViewer (可视化支持)
- PyTorch + LightGlue (SuperPoint支持)

## 版本历史

### v2.0.0 (重构版本)
- 重构为目录结构的插件管理方式
- 添加SuperPoint深度学习特征提取支持
- 改进代码组织和维护性
- 增强Python脚本集成

### v1.x (原版本)  
- 单文件插件结构
- 基础特征提取功能
