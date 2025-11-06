# 相对位姿评估 (RelativePoses Evaluation)

相对位姿评估用于衡量两个相机视图之间相对位姿估计的精度。在PoSDK中，相对位姿由旋转矩阵 $R_{ij}$ 和平移向量 $\mathbf{t}_{ij}$ 定义。

## 数学定义

### 相对位姿表示
两个相机（视图 $i$ 和视图 $j$）之间的相对位姿定义为：

$$\mathbf{x}_j \sim R_{ij} \cdot \mathbf{x}_i + \mathbf{t}_{ij}$$

其中：
- $R_{ij}$: 从相机 $i$ 到相机 $j$ 的旋转矩阵 (3×3)
- $\mathbf{t}_{ij}$: 相机 $i$ 的原点在相机 $j$ 坐标系下的坐标 (3×1)
- $\mathbf{x}_i, \mathbf{x}_j$: 分别为相机 $i$ 和 $j$ 坐标系下的归一化图像坐标

## 评估指标

### 1. 旋转误差 (Rotation Error)

对于真值旋转 $R_{ij}^{gt}$ 和估计旋转 $R_{ij}^{est}$，旋转误差计算为：

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{ij}^{gt\,T} R_{ij}^{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

**物理意义**: 两个旋转矩阵之间的最小旋转角度，单位为度。

### 2. 平移角度误差 (Translation Angular Error)

对于真值平移 $\mathbf{t}_{ij}^{gt}$ 和估计平移 $\mathbf{t}_{ij}^{est}$，平移角度误差计算为：

$$\text{Translation Error} = \arccos\left(\frac{\mathbf{t}_{ij}^{gt\,T} \mathbf{t}_{ij}^{est}}{||\mathbf{t}_{ij}^{gt}|| \cdot ||\mathbf{t}_{ij}^{est}||}\right) \times \frac{180°}{\pi}$$

**物理意义**: 两个平移方向之间的夹角，单位为度。注意这里评估的是方向而非尺度，因为双视图几何中平移的尺度是任意的。

## 实现细节

### 核心评估函数

```cpp
// 位于 types::RelativePoses 类中
size_t EvaluateAgainst(
    const RelativePoses& gt_poses,
    std::vector<double>& rotation_errors,
    std::vector<double>& translation_errors) const;
```

### 评估流程

1. **数据匹配**: 根据视图对 $(i,j)$ 匹配估计值和真值
2. **误差计算**: 对每个匹配的视图对计算旋转和平移误差
3. **结果输出**: 返回匹配的视图对数量和误差向量

### 数据结构

```cpp
// 相对位姿数据结构 (types.hpp)
struct RelativePose {
    ViewId i, j;           // 视图对ID
    Matrix3d Rij;          // 旋转矩阵
    Vector3d tij;          // 平移向量
    double confidence;     // 置信度 (可选)
};

using RelativePoses = std::vector<RelativePose>;
```

## 使用示例

### 基本使用

```cpp
#include <po_core/data/data_relative_poses.hpp>

// 创建相对位姿数据对象
DataRelativePoses estimated_poses;
DataRelativePoses ground_truth_poses;

// 加载数据 (示例)
estimated_poses.LoadFromFile("estimated_relative_poses.pb");
ground_truth_poses.LoadFromFile("gt_relative_poses.pb");

// 执行评估
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // 获取评估结果
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    auto translation_errors = eval_result.GetResults("translation_error_deg");
    
    // 计算统计信息
    double mean_rot_error = eval_result.GetMeanError("rotation_error_deg");
    double mean_trans_error = eval_result.GetMeanError("translation_error_deg");
    
    std::cout << "评估成功！" << std::endl;
    std::cout << "匹配的视图对数量: " << rotation_errors.size() << std::endl;
    std::cout << "平均旋转误差: " << mean_rot_error << "°" << std::endl;
    std::cout << "平均平移角度误差: " << mean_trans_error << "°" << std::endl;
}
```

### 直接使用底层API

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

// 创建相对位姿数据
RelativePoses estimated_poses = {
    {0, 1, R01_est, t01_est, 1.0},
    {0, 2, R02_est, t02_est, 0.9},
    // ... 更多位姿对
};

RelativePoses ground_truth_poses = {
    {0, 1, R01_gt, t01_gt, 1.0},
    {0, 2, R02_gt, t02_gt, 1.0},
    // ... 对应的真值
};

// 直接评估
std::vector<double> rotation_errors, translation_errors;
size_t matched_pairs = estimated_poses.EvaluateAgainst(
    ground_truth_poses, rotation_errors, translation_errors);

std::cout << "匹配的视图对: " << matched_pairs << std::endl;
for (size_t i = 0; i < rotation_errors.size(); ++i) {
    std::cout << "视图对 (" << estimated_poses[i].i << "," << estimated_poses[i].j 
              << "): 旋转误差=" << rotation_errors[i] << "°, "
              << "平移误差=" << translation_errors[i] << "°" << std::endl;
}
```

## 典型应用场景

### 1. 双视图几何验证
```cpp
// 验证基础矩阵分解得到的相对位姿
Matrix3d F = ComputeFundamentalMatrix(matches);
std::vector<RelativePose> candidates = DecomposeFundamentalMatrix(F);

// 与真值比较选择最佳候选
for (const auto& candidate : candidates) {
    RelativePoses est_poses = {candidate};
    auto eval_result = EvaluateRelativePoses(est_poses, gt_poses);
    // 选择误差最小的候选
}
```

### 2. 增量式SfM质量监控
```cpp
// 在增量式SfM过程中监控相对位姿质量
for (const auto& new_view_pair : new_pairs) {
    RelativePose estimated = EstimateRelativePose(new_view_pair);
    RelativePose ground_truth = GetGroundTruthPose(new_view_pair);
    
    double rot_error = ComputeRotationError(estimated.Rij, ground_truth.Rij);
    if (rot_error > threshold) {
        LOG_WARNING << "视图对 " << new_view_pair << " 旋转误差过大: " << rot_error;
    }
}
```

## 注意事项

### 1. 坐标系一致性
确保估计值和真值使用相同的坐标系约定：
```cpp
// PoSDK 相对位姿约定
// x_j ~ R_ij * x_i + t_ij
// 其中 R_ij 表示从相机i到相机j的旋转
// t_ij 表示相机i原点在相机j坐标系下的坐标
```

### 2. 平移尺度不确定性
双视图几何中平移的绝对尺度是不确定的，因此评估的是平移方向而非大小：
```cpp
// 正确：评估平移方向角度
double angle_error = acos(dot(t_gt_normalized, t_est_normalized));

// 错误：直接比较平移大小
double magnitude_error = norm(t_gt) - norm(t_est);  // 不推荐
```

### 3. 数据匹配策略
评估时需要正确匹配视图对：
```cpp
// 自动匹配：基于视图ID
size_t matched = estimated.EvaluateAgainst(ground_truth, rot_errors, trans_errors);

// 手动匹配：确保数据对应关系正确
for (const auto& est_pose : estimated_poses) {
    auto gt_it = std::find_if(gt_poses.begin(), gt_poses.end(),
        [&](const RelativePose& gt) { 
            return gt.i == est_pose.i && gt.j == est_pose.j; 
        });
    if (gt_it != gt_poses.end()) {
        // 计算该视图对的误差
    }
}
```

### 4. 异常值处理
```cpp
// 检查计算结果的有效性
if (std::isnan(rotation_error) || std::isinf(rotation_error)) {
    LOG_WARNING << "旋转误差计算异常，跳过该视图对";
    continue;
}

// 设置合理的误差阈值
const double MAX_ROTATION_ERROR = 180.0;  // 度
const double MAX_TRANSLATION_ERROR = 180.0;  // 度
```

---

**相关参考**:
- [PoSDK 位姿约定](../conventions/posdk.md)
- [全局位姿评估](global_poses.md)
- [评估体系概览](index.md)
