# SIFT特征检测增强功能说明

## 概述
本次更新为method_img2matches插件的SIFT特征检测器增加了完整的OpenMVG兼容参数控制，实现了RootSIFT归一化、first_octave图像预处理等高级功能。

## 新增参数详解

### 1. OpenCV标准参数
这些参数与OpenCV的cv::SIFT::create()直接对应：

- **nfeatures**: 检测特征点数量，0表示不限制
- **nOctaveLayers**: octave层数，对应OpenMVG的num_scales  
- **contrastThreshold**: 对比度阈值，对应OpenMVG的peak_threshold
- **edgeThreshold**: 边缘阈值，防止提取边缘特征点
- **sigma**: 高斯模糊系数，输入图像假定的模糊水平
- **enable_precise_upscale**: 是否启用精确上采样（OpenCV 4.x特有功能）

### 2. OpenMVG扩展参数
这些参数参考OpenMVG的SIFT实现，提供更强大的控制能力：

- **first_octave**: 起始octave层级控制
  - `-1`: 上采样，图像尺寸翻倍，检测更多细节特征
  - `0`: 使用原始图像（默认）
  - `1`: 下采样，图像尺寸减半，减少计算量

- **num_octaves**: 最大octave数量，OpenCV会自动根据图像尺寸限制

- **root_sift**: 是否使用RootSIFT归一化
  - 对SIFT描述子进行开平方根归一化
  - 显著提升匹配性能和鲁棒性
  - 算法：L1归一化 → 开平方根 → L2归一化

### 3. 预设配置
提供四种预设配置，简化参数调整：

- **NORMAL**: 标准质量，速度最快
  - `contrastThreshold=0.04`, `first_octave=0`
  
- **HIGH**: 高质量，平衡速度和质量（推荐）
  - `contrastThreshold=0.01`, `first_octave=0`
  
- **ULTRA**: 超高质量，速度较慢
  - `contrastThreshold=0.01`, `first_octave=-1`（启用上采样）
  
- **CUSTOM**: 完全自定义配置

## 实现功能

### 1. RootSIFT归一化
```cpp
// 实现步骤：
1. L1归一化：descriptor /= l1_norm
2. 开平方根：sqrt(descriptor)  
3. L2归一化：descriptor /= l2_norm
```

### 2. first_octave图像预处理
```cpp
switch (first_octave) {
    case -1: // 上采样2倍，检测更多细节
        cv::resize(img, processed_img, Size(), 2.0, 2.0, INTER_CUBIC);
    case 1:  // 下采样0.5倍，减少计算
        cv::resize(img, processed_img, Size(), 0.5, 0.5, INTER_AREA);  
    case 0:  // 原始图像
        processed_img = img.clone();
}
```

### 3. 特征点坐标调整
当图像被缩放时，自动调整特征点坐标回到原图尺寸：
```cpp
// 上采样时：坐标缩小0.5倍
// 下采样时：坐标放大2倍
kp.pt.x *= scale_factor;
kp.pt.y *= scale_factor; 
kp.size *= scale_factor;
```

## 参数对比：OpenCV vs OpenMVG

| 功能       | OpenCV参数        | OpenMVG参数    | 说明        |
| ---------- | ----------------- | -------------- | ----------- |
| 特征数量   | nfeatures         | -              | OpenCV独有  |
| 层数       | nOctaveLayers     | num_scales     | 对应关系    |
| 对比度阈值 | contrastThreshold | peak_threshold | 对应关系    |
| 边缘阈值   | edgeThreshold     | edge_threshold | 对应关系    |
| 高斯sigma  | sigma             | sigma_min      | 类似功能    |
| 起始层级   | -                 | first_octave   | OpenMVG独有 |
| RootSIFT   | -                 | root_sift      | OpenMVG独有 |

## 配置示例

```ini
[method_img2matches]
detector_type=SIFT

[SIFT]
# 使用HIGH预设（推荐）
preset=HIGH
root_sift=true

# 或者完全自定义
preset=CUSTOM
nfeatures=0
nOctaveLayers=3
contrastThreshold=0.01
edgeThreshold=10
sigma=1.6
first_octave=0
root_sift=true
```

## 性能影响

- **RootSIFT**: 轻微增加计算量（~5%），显著提升匹配质量
- **first_octave=-1**: 计算量增加4倍，特征点数量增加2-3倍
- **first_octave=1**: 计算量减少4倍，特征点数量减少一半

## 兼容性

- 完全兼容现有配置文件
- 默认参数与OpenMVG HIGH预设一致
- 自动参数同步到父类MethodImg2FeaturesPlugin
- 支持已有特征数据的描述子重计算

## 调试信息

设置`log_level=2`可查看详细的处理信息：
```
[VERBOSE] Applied upsampling (first_octave=-1): [640x480] -> [1280x960]  
[VERBOSE] Applying RootSIFT normalization to 1247 descriptors
[VERBOSE] Adjusting 1247 keypoints with scale factor: 0.5
[VERBOSE] RootSIFT normalization completed
```
