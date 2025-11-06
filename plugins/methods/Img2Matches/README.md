# Img2Matches Plugin

## 概述

Img2Matches 插件是一个图像特征匹配处理流水线，提供从图像特征提取到特征匹配的完整处理流程。该插件支持快速模式和可视化模式两种运行方式。

## 功能特性

### 核心功能
- **特征匹配**: 支持多种匹配器类型（FLANN、BruteForce等）
- **参数调整**: 可调整比率测试阈值、匹配器类型等参数
- **数据导出**: 支持特征和匹配结果的导出
- **可视化**: 提供交互式参数调整和匹配结果可视化

### 运行模式
- **快速模式**: 批量处理所有图像对，高效完成特征匹配
- **可视化模式**: 交互式参数调整，实时预览匹配效果

## 目录结构

```
Img2Matches/
├── CMakeLists.txt                 # 构建配置文件
├── README.md                      # 本文档
├── method_img2matches.ini         # 插件配置文件
├── img2matches_pipeline.hpp       # 主要类声明
├── img2matches_pipeline.cpp       # 主要类实现
├── img2matches_fast_mode.cpp      # 快速模式实现
├── img2matches_viewer_mode.cpp    # 可视化模式实现
├── Img2MatchesParams.hpp          # 参数配置类声明
└── Img2MatchesParams.cpp          # 参数配置类实现
```

## 配置参数

### 基础配置
- `ProfileCommit`: 配置修改说明
- `enable_profiling`: 是否启用性能分析
- `enable_evaluator`: 是否启用评估
- `log_level`: 日志级别（0=无, 1=正常, 2=详细）
- `run_mode`: 运行模式（fast=快速, viewer=可视化）

### 匹配器配置
- `matcher_type`: 匹配器类型（FLANN、BF、BF_NORM_L1、BF_HAMMING）
- `cross_check`: 是否启用交叉检查
- `ratio_thresh`: Lowe's比率测试阈值
- `max_matches`: 最大匹配数量限制

### 导出配置
- `export_features`: 是否导出特征文件
- `export_fea_path`: 特征导出路径
- `export_matches`: 是否导出匹配结果
- `export_match_path`: 匹配结果导出路径

### 可视化配置
- `show_view_pair_i`: 第一幅图像索引
- `show_view_pair_j`: 第二幅图像索引

## 使用方法

### 快速模式
```cpp
auto plugin = FactoryMethod::Create("method_img2matches");
plugin->SetMethodOptions({{"run_mode", "fast"}});
auto result = plugin->Run();
```

### 可视化模式
```cpp
auto plugin = FactoryMethod::Create("method_img2matches");
plugin->SetMethodOptions({
    {"run_mode", "viewer"},
    {"show_view_pair_i", "0"},
    {"show_view_pair_j", "1"}
});
auto result = plugin->Run();
```

## 输入输出

### 输入数据
- `data_images`: 图像路径数据 (ImagePaths)
- `data_features`: 特征数据 (FeaturesInfo, 可选)

### 输出数据
- 匹配结果数据 (Matches)

## 兼容性

该插件重构自原有的单文件插件结构，保持完全的向后兼容性：
- 插件名称仍为 `method_img2matches`
- 配置文件格式保持不变
- API接口保持一致

## 依赖项

- OpenCV (特征检测和匹配)
- PoSDK Core (数据结构和接口)
- Common Converter (数据转换)
- Common ImageViewer (可视化支持)

## 版本历史

### v2.0.0 (重构版本)
- 重构为目录结构的插件管理方式
- 分离快速模式和可视化模式实现
- 引入结构化参数配置系统
- 改进代码组织和维护性

### v1.x (原版本)
- 单文件插件结构
- 基础特征匹配功能
