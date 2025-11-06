# LightGlue深度学习匹配器集成文档

## 概述

成功在Img2Matches插件中集成了LightGlue深度学习匹配器，通过C++调用Python脚本的方式实现高精度的特征匹配。

## 功能特性

### 支持的特征类型
- **SuperPoint**: 256维特征，精度高，速度中等
- **DISK**: 128维特征，速度快，精度中等  
- **SIFT**: 128维特征，经典方法，兼容性好
- **ALIKED**: 128维特征，轻量级，速度快
- **DoGHardNet**: 128维特征，精度高，计算量大

### 核心优势
- **高精度匹配**: 使用深度学习模型，匹配精度显著优于传统方法
- **自适应机制**: 根据图像对难易程度自动调整计算复杂度
- **多特征支持**: 支持多种主流特征提取器
- **性能优化**: 支持FlashAttention、混合精度等优化技术

## 文件结构

```
src/plugins/methods/Img2Matches/
├── LightGlueMatcher.hpp          # LightGlue匹配器头文件
├── LightGlueMatcher.cpp          # LightGlue匹配器实现
├── img2matches_pipeline.hpp      # 更新：添加LightGlue支持
├── img2matches_pipeline.cpp      # 更新：集成LightGlue匹配逻辑
├── Img2MatchesParams.hpp         # 更新：添加LightGlue参数
├── Img2MatchesParams.cpp         # 更新：参数加载和转换
├── method_img2matches.ini        # 更新：LightGlue配置
└── CMakeLists.txt                # 更新：构建配置

src/dependencies/LightGlue-main/
└── lightglue_matcher.py          # Python调用脚本
```

## 配置参数

### 基础配置
- `feature_type`: 特征类型（SUPERPOINT, DISK, SIFT, ALIKED, DOGHARDNET）
- `max_num_keypoints`: 最大特征点数量（默认2048）
- `depth_confidence`: 深度置信度，控制早停（0-1，默认0.95）
- `width_confidence`: 宽度置信度，控制点剪枝（0-1，默认0.99）
- `filter_threshold`: 匹配置信度阈值（0-1，默认0.1）

### 性能优化
- `flash_attention`: 启用FlashAttention优化（默认true）
- `mixed_precision`: 混合精度计算（默认false）
- `compile_model`: 编译模型优化（默认false）

### 环境配置
- `python_executable`: Python可执行文件路径（默认python3）
- `script_path`: LightGlue脚本路径（空值自动检测）

## 使用方法

### 配置文件设置
```ini
# 选择LightGlue匹配器
matcher_type=LIGHTGLUE

# LightGlue特定配置
[LIGHTGLUE]
feature_type=SUPERPOINT
max_num_keypoints=2048
depth_confidence=0.95
width_confidence=0.99
filter_threshold=0.1

flash_attention=true
mixed_precision=false
compile_model=false

python_executable=python3
script_path=
```

### 代码调用
```cpp
auto plugin = FactoryMethod::Create("method_img2matches");
plugin->SetMethodOptions({
    {"matcher_type", "LIGHTGLUE"}
});
auto result = plugin->Run();
```

## 环境要求

### Python依赖
```bash
pip install torch torchvision numpy opencv-python
```

### LightGlue安装
1. 下载LightGlue到`src/dependencies/LightGlue-main/`
2. 确保Python脚本`lightglue_matcher.py`存在
3. 验证Python环境：`python3 -c "import torch; print('Environment OK')"`

## 工作原理

### 数据流程
1. **C++端**: 准备图像、特征点、描述子数据
2. **文件交换**: 保存数据到临时文件
3. **Python调用**: 执行LightGlue匹配脚本
4. **结果返回**: 读取匹配结果文件
5. **清理**: 删除临时文件

### 匹配流程
```cpp
// 在RunFastMode中自动选择匹配器
if (matcher_type == LIGHTGLUE) {
    // 缓存图像数据（LightGlue需要）
    all_images.push_back(img.clone());
    
    // 使用LightGlue匹配
    matches = MatchFeaturesWithLightGlue(
        img1, img2, keypoints1, keypoints2,
        descriptors1, descriptors2);
} else {
    // 使用传统匹配器
    matches = MatchFeatures(descriptors1, descriptors2);
}
```

## 性能特点

### 优势
- **高匹配精度**: 深度学习模型的匹配准确率优于传统方法
- **智能自适应**: 根据场景难度自动调整计算量
- **鲁棒性强**: 对光照、角度变化等有更好的鲁棒性

### 限制
- **计算开销**: 比传统匹配器计算量更大
- **环境依赖**: 需要Python和PyTorch环境
- **GPU加速**: 建议使用GPU以获得最佳性能

## 错误处理

### 降级机制
LightGlue匹配失败时会自动降级到FASTCASCADEHASHINGL2匹配器：
```cpp
if (!success) {
    PO_LOG_ERR << "LightGlue matching failed, falling back to FASTCASCADEHASHINGL2";
    matches = MatchFeatures(descriptors1, descriptors2);
}
```

### 常见问题
1. **Python环境错误**: 检查python_executable路径和依赖安装
2. **脚本未找到**: 确认LightGlue脚本路径正确
3. **内存不足**: 减少max_num_keypoints或启用mixed_precision
4. **GPU不可用**: LightGlue会自动降级到CPU模式

## 调试建议

### 日志级别
设置`log_level=2`可查看详细执行信息：
```
[VERBOSE] Using LightGlue deep learning matcher
[VERBOSE] LightGlue matching successful, found 1234 matches
```

### 性能监控
监控匹配时间和精度：
- 首次运行会较慢（模型加载）
- 后续运行速度提升
- 可通过compile_model进一步优化

## 兼容性

### 向后兼容
- 完全兼容现有配置文件
- 不使用LightGlue时无额外开销
- 自动降级保证功能可用性

### 系统要求
- C++14及以上
- Python 3.7+
- PyTorch 1.9+
- OpenCV 4.0+

## 总结

LightGlue集成为PoSDK提供了最先进的深度学习特征匹配能力，在保持易用性的同时显著提升了匹配精度。通过合理的配置和环境准备，可以获得优异的匹配效果。
