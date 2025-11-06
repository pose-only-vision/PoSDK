# LightGlue集成测试配置

## 🧪 测试配置

### 1. 使用SuperPoint特征 + LightGlue匹配

#### GlobalSfMPipeline配置
```ini
# globalsfm_pipeline.ini
preprocess_type=posdk
```

#### Img2Matches配置
```ini
# method_img2matches.ini
detector_type=SUPERPOINT
matcher_type=LIGHTGLUE

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
```

#### method_img2features配置
```ini
# method_img2features.ini  
detector_type=SUPERPOINT

[SUPERPOINT]
max_keypoints=2048
detection_threshold=0.0005
nms_radius=4
remove_borders=4
python_executable=python3
```

### 2. 使用SIFT特征 + LightGlue匹配

#### Img2Matches配置
```ini
# method_img2matches.ini
detector_type=SIFT
matcher_type=LIGHTGLUE

[LIGHTGLUE]
feature_type=SIFT  # 使用SIFT特征类型
```

## 🔍 预期行为

### SuperPoint + LightGlue流程
1. **特征提取阶段**：
   - `method_img2features` 使用 `superpoint_extractor.py` 提取SuperPoint特征
   - 生成256维描述子和高质量特征点
   
2. **匹配阶段**：
   - `Img2Matches` 使用 `lightglue_matcher.py` 进行深度学习匹配
   - LightGlue使用已有的SuperPoint特征进行匹配

### SIFT + LightGlue流程  
1. **特征提取阶段**：
   - `method_img2features` 使用OpenCV SIFT提取传统特征
   - 生成128维描述子
   
2. **匹配阶段**：
   - `Img2Matches` 使用LightGlue的SIFT模式进行匹配
   - 获得比传统匹配器更高的精度

## ⚠️ 环境要求

### Python依赖
```bash
pip install torch torchvision numpy opencv-python
```

### 文件检查
- ✅ `src/dependencies/LightGlue-main/lightglue/` 目录存在
- ✅ `src/dependencies/LightGlue-main/lightglue_matcher.py` 存在
- ✅ `src/dependencies/LightGlue-main/superpoint_extractor.py` 存在

## 🚨 可能的问题

### 1. 特征类型不匹配
如果配置了`detector_type=SIFT`但`feature_type=SUPERPOINT`，会导致不兼容。

**解决方案**：确保配置一致性
- SIFT特征 → `feature_type=SIFT`
- SuperPoint特征 → `feature_type=SUPERPOINT`

### 2. Python环境问题
LightGlue需要PyTorch环境，如果环境不完整会自动降级到传统匹配器。

### 3. 性能考虑
SuperPoint + LightGlue是完全的深度学习流水线，计算量较大，建议：
- 使用GPU加速（如果可用）
- 适当减少`max_keypoints`
- 启用`flash_attention`优化

## ✅ 验证方法

### 检查日志输出
成功的LightGlue集成应该显示：
```
[SuperPointStrategy] SuperPoint extraction successful, found XXXX keypoints
[LightGlueMatcher] LightGlue matching successful, found XXXX matches
```

### 性能对比
对比不同配置的匹配精度和速度：
1. SIFT + FASTCASCADEHASHINGL2 (传统方法)
2. SIFT + LIGHTGLUE (混合方法)
3. SUPERPOINT + LIGHTGLUE (深度学习方法)
