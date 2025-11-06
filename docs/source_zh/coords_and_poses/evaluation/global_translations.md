# 全局位移评估 (GlobalTranslations Evaluation)

全局位移评估专门用于衡量相机光心在世界坐标系中的位置估计精度。这种评估方法特别适用于全局位移估计、位移平均算法和仅位移的SfM方法验证。

## 数学定义

### 全局位移表示
在PoSDK中，全局位移是指相机光心在世界坐标系中的位置向量：

- **RwTw格式**: $\mathbf{t}_w$ 直接表示相机光心的世界坐标
- **RwTc格式**: $\mathbf{t}_c = -R_w^c \cdot \mathbf{t}_w$，需要转换得到世界坐标

```cpp
// 全局位移数据结构 (types.hpp)
using GlobalTranslations = std::vector<Vector3d>;  // 每个元素是一个相机的位移向量
```

## 评估指标

### 1. 位置误差 (Position Error)

对于真值位移 $\mathbf{t}_w^{gt}$ 和估计位移 $\mathbf{t}_w^{est}$：

$$\text{Position Error} = ||\mathbf{t}_w^{gt} - \mathbf{t}_w^{est}||_2$$

**物理意义**: 相机光心位置的欧几里得距离误差，单位与坐标系单位一致。

### 2. 方向误差 (Direction Error)

评估位移方向的准确性：

$$\text{Direction Error} = \arccos\left(\frac{\mathbf{t}_w^{gt\,T} \mathbf{t}_w^{est}}{||\mathbf{t}_w^{gt}|| \cdot ||\mathbf{t}_w^{est}||}\right) \times \frac{180°}{\pi}$$

**物理意义**: 两个位移向量之间的夹角，单位为度。适用于尺度不确定的情况。

### 3. 相对位置误差 (Relative Position Error)

评估相机间相对位置的准确性：

$$\text{Relative Error}_{ij} = ||(\mathbf{t}_w^{gt,i} - \mathbf{t}_w^{gt,j}) - (\mathbf{t}_w^{est,i} - \mathbf{t}_w^{est,j})||_2$$

**物理意义**: 相机对之间相对位置向量的误差，对全局坐标系选择不敏感。

## 评估方法

### 1. 直接位置比较

最简单的评估方法，直接比较对应相机的位置：

```cpp
std::vector<double> ComputePositionErrors(
    const GlobalTranslations& estimated,
    const GlobalTranslations& ground_truth) {
    
    std::vector<double> errors;
    size_t num_cameras = std::min(estimated.size(), ground_truth.size());
    
    for (size_t i = 0; i < num_cameras; ++i) {
        double error = (ground_truth[i] - estimated[i]).norm();
        errors.push_back(error);
    }
    return errors;
}
```

### 2. 相似变换对齐评估

通过相似变换消除坐标系差异：

```cpp
bool EvaluateWithSimilarityTransform(
    const GlobalTranslations& estimated,
    const GlobalTranslations& ground_truth,
    std::vector<double>& position_errors) {
    
    // 1. 计算质心
    Vector3d est_centroid = ComputeCentroid(estimated);
    Vector3d gt_centroid = ComputeCentroid(ground_truth);
    
    // 2. 去中心化
    auto est_centered = CenterData(estimated, est_centroid);
    auto gt_centered = CenterData(ground_truth, gt_centroid);
    
    // 3. 计算最优尺度和旋转
    double scale;
    Matrix3d R_align;
    Vector3d t_align;
    ComputeSimilarityTransform(est_centered, gt_centered, scale, R_align, t_align);
    
    // 4. 应用变换并计算误差
    for (size_t i = 0; i < estimated.size(); ++i) {
        Vector3d aligned = scale * R_align * estimated[i] + t_align;
        position_errors.push_back((ground_truth[i] - aligned).norm());
    }
    
    return true;
}
```

### 3. 方向误差评估

专门评估位移方向的准确性：

```cpp
std::vector<double> ComputeDirectionErrors(
    const GlobalTranslations& estimated,
    const GlobalTranslations& ground_truth,
    const Vector3d& reference_center = Vector3d::Zero()) {
    
    std::vector<double> errors;
    
    for (size_t i = 0; i < std::min(estimated.size(), ground_truth.size()); ++i) {
        // 计算相对于参考点的方向向量
        Vector3d est_dir = (estimated[i] - reference_center).normalized();
        Vector3d gt_dir = (ground_truth[i] - reference_center).normalized();
        
        // 计算角度误差
        double cos_angle = est_dir.dot(gt_dir);
        cos_angle = std::clamp(cos_angle, -1.0, 1.0);  // 数值稳定性
        double angle_error = std::acos(cos_angle) * 180.0 / M_PI;
        
        errors.push_back(angle_error);
    }
    
    return errors;
}
```

## 实现细节

### 相似变换计算

```cpp
bool ComputeSimilarityTransform(
    const std::vector<Vector3d>& source,
    const std::vector<Vector3d>& target,
    double& scale,
    Matrix3d& rotation,
    Vector3d& translation) {
    
    if (source.size() != target.size() || source.size() < 3) {
        return false;
    }
    
    // 计算质心
    Vector3d source_centroid = Vector3d::Zero();
    Vector3d target_centroid = Vector3d::Zero();
    for (size_t i = 0; i < source.size(); ++i) {
        source_centroid += source[i];
        target_centroid += target[i];
    }
    source_centroid /= source.size();
    target_centroid /= target.size();
    
    // 去中心化
    std::vector<Vector3d> source_centered, target_centered;
    for (size_t i = 0; i < source.size(); ++i) {
        source_centered.push_back(source[i] - source_centroid);
        target_centered.push_back(target[i] - target_centroid);
    }
    
    // 计算尺度
    double source_scale = 0, target_scale = 0;
    for (size_t i = 0; i < source.size(); ++i) {
        source_scale += source_centered[i].squaredNorm();
        target_scale += target_centered[i].squaredNorm();
    }
    scale = std::sqrt(target_scale / source_scale);
    
    // 计算旋转 (Kabsch算法)
    Matrix3d H = Matrix3d::Zero();
    for (size_t i = 0; i < source.size(); ++i) {
        H += scale * source_centered[i] * target_centered[i].transpose();
    }
    
    Eigen::JacobiSVD<Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    rotation = svd.matrixV() * svd.matrixU().transpose();
    
    // 确保是旋转矩阵
    if (rotation.determinant() < 0) {
        Matrix3d V = svd.matrixV();
        V.col(2) = -V.col(2);
        rotation = V * svd.matrixU().transpose();
    }
    
    // 计算平移
    translation = target_centroid - scale * rotation * source_centroid;
    
    return true;
}
```

## 使用示例

### 基本位置误差评估

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

// 创建位移数据
GlobalTranslations estimated_translations = {
    Vector3d(1.0, 0.0, 0.0),
    Vector3d(2.0, 1.0, 0.0),
    Vector3d(3.0, 0.0, 1.0)
};

GlobalTranslations ground_truth_translations = {
    Vector3d(1.1, 0.1, 0.0),
    Vector3d(2.1, 0.9, 0.1),
    Vector3d(2.9, 0.1, 1.0)
};

// 计算位置误差
std::vector<double> position_errors;
for (size_t i = 0; i < estimated_translations.size(); ++i) {
    double error = (ground_truth_translations[i] - estimated_translations[i]).norm();
    position_errors.push_back(error);
    std::cout << "相机 " << i << " 位置误差: " << error << std::endl;
}

// 计算统计信息
double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
double max_error = *std::max_element(position_errors.begin(), position_errors.end());

std::cout << "平均位置误差: " << mean_error << std::endl;
std::cout << "最大位置误差: " << max_error << std::endl;
```

### 相似变换对齐评估

```cpp
// 使用相似变换对齐后评估
double scale;
Matrix3d R_align;
Vector3d t_align;

bool success = ComputeSimilarityTransform(
    estimated_translations, ground_truth_translations,
    scale, R_align, t_align);

if (success) {
    std::cout << "相似变换参数:" << std::endl;
    std::cout << "  尺度: " << scale << std::endl;
    std::cout << "  旋转矩阵: \n" << R_align << std::endl;
    std::cout << "  平移向量: " << t_align.transpose() << std::endl;
    
    // 应用变换并计算对齐后的误差
    std::vector<double> aligned_errors;
    for (size_t i = 0; i < estimated_translations.size(); ++i) {
        Vector3d aligned = scale * R_align * estimated_translations[i] + t_align;
        double error = (ground_truth_translations[i] - aligned).norm();
        aligned_errors.push_back(error);
        std::cout << "相机 " << i << " 对齐后误差: " << error << std::endl;
    }
}
```

### 方向误差评估

```cpp
// 评估位移方向精度 (适用于尺度不确定的情况)
Vector3d reference_center = Vector3d::Zero();  // 使用原点作为参考

std::vector<double> direction_errors;
for (size_t i = 0; i < estimated_translations.size(); ++i) {
    Vector3d est_dir = (estimated_translations[i] - reference_center).normalized();
    Vector3d gt_dir = (ground_truth_translations[i] - reference_center).normalized();
    
    double cos_angle = est_dir.dot(gt_dir);
    cos_angle = std::clamp(cos_angle, -1.0, 1.0);
    double angle_error = std::acos(cos_angle) * 180.0 / M_PI;
    
    direction_errors.push_back(angle_error);
    std::cout << "相机 " << i << " 方向误差: " << angle_error << "°" << std::endl;
}

double mean_direction_error = std::accumulate(direction_errors.begin(), direction_errors.end(), 0.0) / direction_errors.size();
std::cout << "平均方向误差: " << mean_direction_error << "°" << std::endl;
```

### 从GlobalPoses提取位移进行评估

```cpp
// 从完整位姿数据中提取位移部分
GlobalPoses estimated_poses, ground_truth_poses;
// ... 加载位姿数据 ...

// 提取位移
GlobalTranslations est_translations = estimated_poses.translations;
GlobalTranslations gt_translations = ground_truth_poses.translations;

// 确保格式一致性
if (estimated_poses.GetPoseFormat() != ground_truth_poses.GetPoseFormat()) {
    // 转换格式以确保位移含义一致
    GlobalPoses gt_copy = ground_truth_poses;
    gt_copy.ConvertPoseFormat(estimated_poses.GetPoseFormat());
    gt_translations = gt_copy.translations;
}

// 执行位移评估
std::vector<double> translation_errors = ComputePositionErrors(est_translations, gt_translations);
```

## 典型应用场景

### 1. 全局位移估计验证

```cpp
// 验证全局位移估计算法
GlobalTranslations estimated = RunGlobalTranslationEstimation(relative_translations);
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// 使用相似变换对齐评估
std::vector<double> position_errors;
bool success = EvaluateWithSimilarityTransform(estimated, ground_truth, position_errors);

if (success) {
    double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
    std::cout << "全局位移估计平均误差: " << mean_error << std::endl;
}
```

### 2. 位移平均算法比较

```cpp
// 比较不同的位移平均算法
GlobalTranslations l1_result = RunL1TranslationAveraging(relative_translations);
GlobalTranslations l2_result = RunL2TranslationAveraging(relative_translations);
GlobalTranslations robust_result = RunRobustTranslationAveraging(relative_translations);
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// 评估各算法
auto eval_algorithm = [&](const GlobalTranslations& result, const std::string& name) {
    std::vector<double> errors = ComputePositionErrors(result, ground_truth);
    double mean_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    std::cout << name << " 平均位置误差: " << mean_error << std::endl;
};

eval_algorithm(l1_result, "L1平均");
eval_algorithm(l2_result, "L2平均");
eval_algorithm(robust_result, "鲁棒平均");
```

### 3. 增量式位移累积误差分析

```cpp
// 分析增量式SfM中的位移累积误差
std::vector<GlobalTranslations> incremental_results;
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// 模拟增量式构建过程
for (size_t num_views = 3; num_views <= ground_truth.size(); ++num_views) {
    GlobalTranslations partial_result = RunIncrementalSfM(images, matches, num_views);
    incremental_results.push_back(partial_result);
    
    // 评估当前阶段的误差
    std::vector<double> errors = ComputePositionErrors(partial_result, 
        GlobalTranslations(ground_truth.begin(), ground_truth.begin() + num_views));
    
    double mean_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    std::cout << "视图数: " << num_views << ", 平均位置误差: " << mean_error << std::endl;
}
```

## 注意事项

### 1. 坐标系一致性

```cpp
// 确保位移的物理含义一致
if (estimated_poses.GetPoseFormat() == PoseFormat::RwTw && 
    ground_truth_poses.GetPoseFormat() == PoseFormat::RwTc) {
    
    // 需要格式转换
    GlobalPoses gt_copy = ground_truth_poses;
    gt_copy.ConvertPoseFormat(PoseFormat::RwTw);
    
    // 现在可以直接比较位移
    auto est_translations = estimated_poses.translations;
    auto gt_translations = gt_copy.translations;
}
```

### 2. 尺度不确定性处理

```cpp
// 对于尺度不确定的情况，使用方向误差评估
if (scale_unknown) {
    std::vector<double> direction_errors = ComputeDirectionErrors(estimated, ground_truth);
    // 分析方向精度而非绝对位置精度
} else {
    std::vector<double> position_errors = ComputePositionErrors(estimated, ground_truth);
    // 分析绝对位置精度
}
```

### 3. 参考坐标系选择

```cpp
// 选择合适的参考点进行方向误差计算
Vector3d reference_center;

// 选项1: 使用质心作为参考
reference_center = ComputeCentroid(ground_truth);

// 选项2: 使用第一个相机作为参考
reference_center = ground_truth[0];

// 选项3: 使用原点作为参考
reference_center = Vector3d::Zero();

std::vector<double> direction_errors = ComputeDirectionErrors(
    estimated, ground_truth, reference_center);
```

### 4. 异常值检测

```cpp
// 检测和处理异常的位移值
std::vector<double> position_errors = ComputePositionErrors(estimated, ground_truth);

// 计算误差统计信息
double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
double variance = 0;
for (double error : position_errors) {
    variance += (error - mean_error) * (error - mean_error);
}
variance /= position_errors.size();
double std_dev = std::sqrt(variance);

// 标记异常值 (3σ原则)
for (size_t i = 0; i < position_errors.size(); ++i) {
    if (position_errors[i] > mean_error + 3 * std_dev) {
        std::cout << "相机 " << i << " 的位移误差异常: " << position_errors[i] << std::endl;
    }
}
```

---

**相关参考**:
- [PoSDK 位姿约定](../conventions/posdk.md)
- [全局位姿评估](global_poses.md)
- [相对位姿评估](relative_poses.md)
- [评估体系概览](index.md)
