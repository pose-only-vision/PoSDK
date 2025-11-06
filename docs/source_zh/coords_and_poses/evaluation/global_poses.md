# 全局位姿评估 (GlobalPoses Evaluation)

全局位姿评估用于衡量相机在世界坐标系中的全局位姿估计精度。PoSDK 提供了多种评估方法，包括完整的相似变换对齐评估和仅旋转部分的评估。

## 数学定义

### 全局位姿表示
在PoSDK中，全局位姿默认使用 `PoseFormat::RwTw` 格式：

$$\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$$

其中：
- $R_w^c$: 从世界坐标系到相机坐标系的旋转矩阵 (3×3)
- $\mathbf{t}_w$: 相机光心在世界坐标系中的位置 (3×1)
- $\mathbf{X}_w$: 世界坐标系中的3D点
- $\mathbf{x}_c$: 相机坐标系中的归一化图像坐标

## 评估方法

PoSDK 提供了三种主要的全局位姿评估方法：

### 1. 完整相似变换评估 (EvaluateWithSimilarityTransform)

**用途**: 最全面的评估方法，通过7自由度相似变换对齐估计位姿到真值。

**数学原理**:
计算最优相似变换参数 $(s, R_{align}, \mathbf{t}_{align})$：

$$\mathbf{p}_{aligned} = s \cdot R_{align} \cdot \mathbf{p}_{est} + \mathbf{t}_{align}$$

**实现特点**:
- 使用Ceres优化器求解最优变换参数
- 统一转换为`RwTw`格式进行评估
- 提供最高精度的对齐结果

```cpp
bool EvaluateWithSimilarityTransform(
    const GlobalPoses& gt_poses,
    std::vector<double>& position_errors,
    std::vector<double>& rotation_errors) const;
```

### 2. 标准相似变换评估 (EvaluateAgainst)

**用途**: 使用解析方法计算相似变换，计算效率更高。

**数学原理**:
基于SVD分解计算最优旋转和尺度：

1. **质心计算**: $\bar{\mathbf{p}}_{gt} = \frac{1}{n}\sum_{i=1}^n \mathbf{p}_{gt}^i$
2. **去中心化**: $\mathbf{p}_{gt,c}^i = \mathbf{p}_{gt}^i - \bar{\mathbf{p}}_{gt}$
3. **尺度计算**: $s = \sqrt{\frac{\sum||\mathbf{p}_{gt,c}^i||^2}{\sum||\mathbf{p}_{est,c}^i||^2}}$
4. **旋转计算**: $R_{align} = \arg\min_R \sum||s R \mathbf{p}_{est,c}^i - \mathbf{p}_{gt,c}^i||^2$

```cpp
bool EvaluateAgainst(
    const GlobalPoses& gt_poses,
    std::vector<double>& position_errors,
    std::vector<double>& rotation_errors,
    bool compute_similarity = true) const;
```

### 3. 仅旋转评估 (EvaluateRotations / EvaluateRotationsSVD)

**用途**: 仅评估旋转部分的精度，忽略位置信息。

**数学原理**:
1. **对齐第一个视图**: $R_{gt,aligned}^i = R_{gt}^i \cdot (R_{gt}^1)^{-1}$
2. **计算旋转补偿**: 通过Ceres优化或SVD方法计算最优旋转补偿 $R_{opt}$
3. **应用补偿**: $R_{est,compensated}^i = R_{est,aligned}^i \cdot R_{opt}$

```cpp
// Ceres优化方法
bool EvaluateRotations(
    const GlobalPoses& gt_poses,
    std::vector<double>& rotation_errors) const;

// SVD方法  
bool EvaluateRotationsSVD(
    const GlobalPoses& gt_poses,
    std::vector<double>& rotation_errors) const;
```

## 评估指标

### 1. 旋转误差 (Rotation Error)

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{gt}^T R_{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

**物理意义**: 两个旋转矩阵之间的最小旋转角度，单位为度。

### 2. 位置误差 (Position Error)

$$\text{Position Error} = ||\mathbf{t}_{gt} - \mathbf{t}_{est}||_2$$

**物理意义**: 相机光心位置的欧几里得距离误差。

## 实现细节

### 数据结构

```cpp
// 全局位姿数据结构 (types.hpp)
class GlobalPoses {
public:
    GlobalRotations rotations;       // 旋转矩阵数组
    GlobalTranslations translations; // 平移向量数组
    EstInfo est_info;               // 估计信息
    PoseFormat pose_format;         // 位姿格式
    
    // 评估方法
    bool EvaluateWithSimilarityTransform(...) const;
    bool EvaluateAgainst(...) const;
    bool EvaluateRotations(...) const;
    bool EvaluateRotationsSVD(...) const;
};
```

### 旋转补偿优化

PoSDK 使用Ceres优化器进行旋转补偿计算：

```cpp
// 旋转补偿代价函数
struct RotationCompensationError {
    template<typename T>
    bool operator()(const T* const rotation_compensation, T* residuals) const {
        // 应用旋转补偿: R_compensated = R_est * R_opt
        T compensation_matrix[9];
        ceres::AngleAxisToRotationMatrix(rotation_compensation, compensation_matrix);
        
        // 计算旋转误差: ΔR = R_gt^(-1) * R_compensated
        // 残差为旋转向量的模长
        residuals[0] = ||ω(ΔR)||;
        return true;
    }
};
```

## 使用示例

### 完整评估示例

```cpp
#include <po_core/data/data_global_poses.hpp>

// 创建全局位姿数据对象
DataGlobalPoses estimated_poses;
DataGlobalPoses ground_truth_poses;

// 加载数据
estimated_poses.LoadFromFile("estimated_global_poses.pb");
ground_truth_poses.LoadFromFile("gt_global_poses.pb");

// 方法1: 使用DataGlobalPoses的Evaluate接口
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // 获取评估结果
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    auto position_errors = eval_result.GetResults("translation_error");
    
    // 打印统计信息
    std::cout << "评估成功！" << std::endl;
    std::cout << "评估的位姿数量: " << rotation_errors.size() << std::endl;
    std::cout << "平均旋转误差: " << eval_result.GetMeanError("rotation_error_deg") << "°" << std::endl;
    std::cout << "平均位置误差: " << eval_result.GetMeanError("translation_error") << std::endl;
}
```

### 使用不同评估方法

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

GlobalPoses estimated_poses, ground_truth_poses;
// ... 加载数据 ...

std::vector<double> position_errors, rotation_errors;

// 方法1: 高精度相似变换评估
bool success1 = estimated_poses.EvaluateWithSimilarityTransform(
    ground_truth_poses, position_errors, rotation_errors);

// 方法2: 标准相似变换评估
bool success2 = estimated_poses.EvaluateAgainst(
    ground_truth_poses, position_errors, rotation_errors, true);

// 方法3: 仅旋转评估 (Ceres优化)
bool success3 = estimated_poses.EvaluateRotations(
    ground_truth_poses, rotation_errors);

// 方法4: 仅旋转评估 (SVD方法)
bool success4 = estimated_poses.EvaluateRotationsSVD(
    ground_truth_poses, rotation_errors);

// 输出结果
if (success1) {
    std::cout << "相似变换评估完成" << std::endl;
    for (size_t i = 0; i < position_errors.size(); ++i) {
        std::cout << "相机 " << i << ": 位置误差=" << position_errors[i] 
                  << ", 旋转误差=" << rotation_errors[i] << "°" << std::endl;
    }
}
```

### 格式转换与评估

```cpp
// 处理不同位姿格式
GlobalPoses poses_rwtw, poses_rwtc;

// 加载RwTw格式的位姿
poses_rwtw.LoadFromFile("poses_rwtw.pb");
poses_rwtw.SetPoseFormat(PoseFormat::RwTw);

// 加载RwTc格式的位姿  
poses_rwtc.LoadFromFile("poses_rwtc.pb");
poses_rwtc.SetPoseFormat(PoseFormat::RwTc);

// 评估时会自动进行格式转换
std::vector<double> pos_errors, rot_errors;
bool success = poses_rwtw.EvaluateWithSimilarityTransform(
    poses_rwtc, pos_errors, rot_errors);
// PoSDK会自动将poses_rwtc转换为RwTw格式进行评估
```

## 典型应用场景

### 1. 全局SfM质量评估

```cpp
// 评估全局SfM结果
GlobalPoses sfm_result = RunGlobalSfM(images, matches);
GlobalPoses ground_truth = LoadGroundTruthPoses();

std::vector<double> pos_errors, rot_errors;
bool success = sfm_result.EvaluateWithSimilarityTransform(
    ground_truth, pos_errors, rot_errors);

// 计算统计信息
double mean_pos_error = std::accumulate(pos_errors.begin(), pos_errors.end(), 0.0) / pos_errors.size();
double mean_rot_error = std::accumulate(rot_errors.begin(), rot_errors.end(), 0.0) / rot_errors.size();

std::cout << "SfM质量评估:" << std::endl;
std::cout << "  平均位置误差: " << mean_pos_error << std::endl;
std::cout << "  平均旋转误差: " << mean_rot_error << "°" << std::endl;
```

### 2. 旋转平均算法评估

```cpp
// 仅评估旋转平均的结果
GlobalPoses rotation_averaged_poses = RunRotationAveraging(relative_poses);
GlobalPoses ground_truth = LoadGroundTruthPoses();

std::vector<double> rotation_errors;

// 使用SVD方法评估旋转精度
bool success = rotation_averaged_poses.EvaluateRotationsSVD(
    ground_truth, rotation_errors);

if (success) {
    double max_error = *std::max_element(rotation_errors.begin(), rotation_errors.end());
    double mean_error = std::accumulate(rotation_errors.begin(), rotation_errors.end(), 0.0) / rotation_errors.size();
    
    std::cout << "旋转平均评估:" << std::endl;
    std::cout << "  平均旋转误差: " << mean_error << "°" << std::endl;
    std::cout << "  最大旋转误差: " << max_error << "°" << std::endl;
}
```

### 3. 增量式与全局式SfM比较

```cpp
// 比较不同SfM方法
GlobalPoses incremental_result = RunIncrementalSfM(images, matches);
GlobalPoses global_result = RunGlobalSfM(images, matches);
GlobalPoses ground_truth = LoadGroundTruthPoses();

// 评估增量式SfM
std::vector<double> inc_pos_errors, inc_rot_errors;
incremental_result.EvaluateWithSimilarityTransform(
    ground_truth, inc_pos_errors, inc_rot_errors);

// 评估全局式SfM
std::vector<double> glob_pos_errors, glob_rot_errors;
global_result.EvaluateWithSimilarityTransform(
    ground_truth, glob_pos_errors, glob_rot_errors);

// 比较结果
auto mean_inc_pos = std::accumulate(inc_pos_errors.begin(), inc_pos_errors.end(), 0.0) / inc_pos_errors.size();
auto mean_glob_pos = std::accumulate(glob_pos_errors.begin(), glob_pos_errors.end(), 0.0) / glob_pos_errors.size();

std::cout << "SfM方法比较:" << std::endl;
std::cout << "  增量式平均位置误差: " << mean_inc_pos << std::endl;
std::cout << "  全局式平均位置误差: " << mean_glob_pos << std::endl;
```

## 注意事项

### 1. 位姿格式一致性
```cpp
// 确保格式一致性
if (estimated.GetPoseFormat() != ground_truth.GetPoseFormat()) {
    LOG_INFO << "位姿格式不同，将自动转换";
    // PoSDK会自动处理格式转换
}
```

### 2. 数据有效性检查
```cpp
// 检查数据完整性
if (estimated.Size() == 0 || ground_truth.Size() == 0) {
    LOG_ERROR << "位姿数据为空";
    return false;
}

// 检查数值有效性
for (size_t i = 0; i < estimated.Size(); ++i) {
    if (!estimated.rotations[i].allFinite() || 
        !estimated.translations[i].allFinite()) {
        LOG_WARNING << "位姿 " << i << " 包含无效数值";
    }
}
```

### 3. 评估方法选择

| 评估方法                          | 适用场景     | 优势                | 劣势                |
| --------------------------------- | ------------ | ------------------- | ------------------- |
| `EvaluateWithSimilarityTransform` | 最终精度评估 | 最高精度，Ceres优化 | 计算较慢            |
| `EvaluateAgainst`                 | 常规评估     | 计算效率高          | 精度略低于Ceres方法 |
| `EvaluateRotations`               | 旋转算法评估 | 专注旋转，Ceres优化 | 忽略位置信息        |
| `EvaluateRotationsSVD`            | 快速旋转评估 | 计算最快            | 精度略低            |

### 4. 异常处理
```cpp
try {
    std::vector<double> pos_errors, rot_errors;
    bool success = estimated.EvaluateWithSimilarityTransform(
        ground_truth, pos_errors, rot_errors);
    
    if (!success) {
        LOG_ERROR << "评估失败，可能是数据格式或数值问题";
        return;
    }
    
    // 检查结果有效性
    for (double error : pos_errors) {
        if (std::isnan(error) || std::isinf(error)) {
            LOG_WARNING << "检测到无效的位置误差值";
        }
    }
} catch (const std::exception& e) {
    LOG_ERROR << "评估过程中发生异常: " << e.what();
}
```

---

**相关参考**:
- [PoSDK 位姿约定](../conventions/posdk.md)
- [相对位姿评估](relative_poses.md)
- [全局位移评估](global_translations.md)
- [评估体系概览](index.md)
