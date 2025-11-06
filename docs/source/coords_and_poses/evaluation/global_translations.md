# Global Translation Evaluation (GlobalTranslations Evaluation)

Global translation evaluation is specifically designed to measure the accuracy of camera center position estimation in the world coordinate system. This evaluation method is particularly suitable for global translation estimation, translation averaging algorithms, and validation of translation-only SfM methods.

## Mathematical Definition

### Global Translation Representation
In PoSDK, global translation refers to the position vector of the camera center in the world coordinate system:

- **RwTw format**: $\mathbf{t}_w$ directly represents the world coordinates of the camera center
- **RwTc format**: $\mathbf{t}_c = -R_w^c \cdot \mathbf{t}_w$, needs conversion to get world coordinates

```cpp
// Global translation data structure (types.hpp)
using GlobalTranslations = std::vector<Vector3d>;  // Each element is a camera's translation vector
```

## Evaluation Metrics

### 1. Position Error

For ground truth translation $\mathbf{t}_w^{gt}$ and estimated translation $\mathbf{t}_w^{est}$:

$$\text{Position Error} = ||\mathbf{t}_w^{gt} - \mathbf{t}_w^{est}||_2$$

**Physical Meaning**: Euclidean distance error of camera center position, units consistent with coordinate system units.

### 2. Direction Error

Evaluates the accuracy of translation direction:

$$\text{Direction Error} = \arccos\left(\frac{\mathbf{t}_w^{gt\,T} \mathbf{t}_w^{est}}{||\mathbf{t}_w^{gt}|| \cdot ||\mathbf{t}_w^{est}||}\right) \times \frac{180°}{\pi}$$

**Physical Meaning**: Angle between two translation vectors, in degrees. Suitable for cases where scale is uncertain.

### 3. Relative Position Error

Evaluates the accuracy of relative positions between cameras:

$$\text{Relative Error}_{ij} = ||(\mathbf{t}_w^{gt,i} - \mathbf{t}_w^{gt,j}) - (\mathbf{t}_w^{est,i} - \mathbf{t}_w^{est,j})||_2$$

**Physical Meaning**: Error of relative position vectors between camera pairs, insensitive to global coordinate system selection.

## Evaluation Methods

### 1. Direct Position Comparison

The simplest evaluation method, directly comparing corresponding camera positions:

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

### 2. Similarity Transform Alignment Evaluation

Eliminate coordinate system differences through similarity transform:

```cpp
bool EvaluateWithSimilarityTransform(
    const GlobalTranslations& estimated,
    const GlobalTranslations& ground_truth,
    std::vector<double>& position_errors) {
    
    // 1. Calculate centroid
    Vector3d est_centroid = ComputeCentroid(estimated);
    Vector3d gt_centroid = ComputeCentroid(ground_truth);
    
    // 2. Center data
    auto est_centered = CenterData(estimated, est_centroid);
    auto gt_centered = CenterData(ground_truth, gt_centroid);
    
    // 3. Calculate optimal scale and rotation
    double scale;
    Matrix3d R_align;
    Vector3d t_align;
    ComputeSimilarityTransform(est_centered, gt_centered, scale, R_align, t_align);
    
    // 4. Apply transformation and calculate errors
    for (size_t i = 0; i < estimated.size(); ++i) {
        Vector3d aligned = scale * R_align * estimated[i] + t_align;
        position_errors.push_back((ground_truth[i] - aligned).norm());
    }
    
    return true;
}
```

### 3. Direction Error Evaluation

Specifically evaluates the accuracy of translation direction:

```cpp
std::vector<double> ComputeDirectionErrors(
    const GlobalTranslations& estimated,
    const GlobalTranslations& ground_truth,
    const Vector3d& reference_center = Vector3d::Zero()) {
    
    std::vector<double> errors;
    
    for (size_t i = 0; i < std::min(estimated.size(), ground_truth.size()); ++i) {
        // Calculate direction vectors relative to reference point
        Vector3d est_dir = (estimated[i] - reference_center).normalized();
        Vector3d gt_dir = (ground_truth[i] - reference_center).normalized();
        
        // Calculate angle error
        double cos_angle = est_dir.dot(gt_dir);
        cos_angle = std::clamp(cos_angle, -1.0, 1.0);  // Numerical stability
        double angle_error = std::acos(cos_angle) * 180.0 / M_PI;
        
        errors.push_back(angle_error);
    }
    
    return errors;
}
```

## Implementation Details

### Similarity Transform Calculation

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
    
    // Calculate centroid
    Vector3d source_centroid = Vector3d::Zero();
    Vector3d target_centroid = Vector3d::Zero();
    for (size_t i = 0; i < source.size(); ++i) {
        source_centroid += source[i];
        target_centroid += target[i];
    }
    source_centroid /= source.size();
    target_centroid /= target.size();
    
    // Center data
    std::vector<Vector3d> source_centered, target_centered;
    for (size_t i = 0; i < source.size(); ++i) {
        source_centered.push_back(source[i] - source_centroid);
        target_centered.push_back(target[i] - target_centroid);
    }
    
    // Calculate scale
    double source_scale = 0, target_scale = 0;
    for (size_t i = 0; i < source.size(); ++i) {
        source_scale += source_centered[i].squaredNorm();
        target_scale += target_centered[i].squaredNorm();
    }
    scale = std::sqrt(target_scale / source_scale);
    
    // Calculate rotation (Kabsch algorithm)
    Matrix3d H = Matrix3d::Zero();
    for (size_t i = 0; i < source.size(); ++i) {
        H += scale * source_centered[i] * target_centered[i].transpose();
    }
    
    Eigen::JacobiSVD<Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    rotation = svd.matrixV() * svd.matrixU().transpose();
    
    // Ensure rotation matrix
    if (rotation.determinant() < 0) {
        Matrix3d V = svd.matrixV();
        V.col(2) = -V.col(2);
        rotation = V * svd.matrixU().transpose();
    }
    
    // Calculate translation
    translation = target_centroid - scale * rotation * source_centroid;
    
    return true;
}
```

## Usage Examples

### Basic Position Error Evaluation

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

// Create translation data
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

// Calculate position errors
std::vector<double> position_errors;
for (size_t i = 0; i < estimated_translations.size(); ++i) {
    double error = (ground_truth_translations[i] - estimated_translations[i]).norm();
    position_errors.push_back(error);
    std::cout << "Camera " << i << " position error: " << error << std::endl;
}

// Calculate statistics
double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
double max_error = *std::max_element(position_errors.begin(), position_errors.end());

std::cout << "Mean position error: " << mean_error << std::endl;
std::cout << "Max position error: " << max_error << std::endl;
```

### Similarity Transform Alignment Evaluation

```cpp
// Evaluate after similarity transform alignment
double scale;
Matrix3d R_align;
Vector3d t_align;

bool success = ComputeSimilarityTransform(
    estimated_translations, ground_truth_translations,
    scale, R_align, t_align);

if (success) {
    std::cout << "Similarity transform parameters:" << std::endl;
    std::cout << "  Scale: " << scale << std::endl;
    std::cout << "  Rotation matrix: \n" << R_align << std::endl;
    std::cout << "  Translation vector: " << t_align.transpose() << std::endl;
    
    // Apply transformation and calculate aligned errors
    std::vector<double> aligned_errors;
    for (size_t i = 0; i < estimated_translations.size(); ++i) {
        Vector3d aligned = scale * R_align * estimated_translations[i] + t_align;
        double error = (ground_truth_translations[i] - aligned).norm();
        aligned_errors.push_back(error);
        std::cout << "Camera " << i << " aligned error: " << error << std::endl;
    }
}
```

### Direction Error Evaluation

```cpp
// Evaluate translation direction accuracy (suitable for scale-uncertain cases)
Vector3d reference_center = Vector3d::Zero();  // Use origin as reference

std::vector<double> direction_errors;
for (size_t i = 0; i < estimated_translations.size(); ++i) {
    Vector3d est_dir = (estimated_translations[i] - reference_center).normalized();
    Vector3d gt_dir = (ground_truth_translations[i] - reference_center).normalized();
    
    double cos_angle = est_dir.dot(gt_dir);
    cos_angle = std::clamp(cos_angle, -1.0, 1.0);
    double angle_error = std::acos(cos_angle) * 180.0 / M_PI;
    
    direction_errors.push_back(angle_error);
    std::cout << "Camera " << i << " direction error: " << angle_error << "°" << std::endl;
}

double mean_direction_error = std::accumulate(direction_errors.begin(), direction_errors.end(), 0.0) / direction_errors.size();
std::cout << "Mean direction error: " << mean_direction_error << "°" << std::endl;
```

### Extract Translations from GlobalPoses for Evaluation

```cpp
// Extract translation part from complete pose data
GlobalPoses estimated_poses, ground_truth_poses;
// ... Load pose data ...

// Extract translations
GlobalTranslations est_translations = estimated_poses.translations;
GlobalTranslations gt_translations = ground_truth_poses.translations;

// Ensure format consistency
if (estimated_poses.GetPoseFormat() != ground_truth_poses.GetPoseFormat()) {
    // Convert format to ensure translation meaning consistency
    GlobalPoses gt_copy = ground_truth_poses;
    gt_copy.ConvertPoseFormat(estimated_poses.GetPoseFormat());
    gt_translations = gt_copy.translations;
}

// Execute translation evaluation
std::vector<double> translation_errors = ComputePositionErrors(est_translations, gt_translations);
```

## Typical Application Scenarios

### 1. Global Translation Estimation Verification

```cpp
// Verify global translation estimation algorithm
GlobalTranslations estimated = RunGlobalTranslationEstimation(relative_translations);
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// Evaluate using similarity transform alignment
std::vector<double> position_errors;
bool success = EvaluateWithSimilarityTransform(estimated, ground_truth, position_errors);

if (success) {
    double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
    std::cout << "Global translation estimation mean error: " << mean_error << std::endl;
}
```

### 2. Translation Averaging Algorithm Comparison

```cpp
// Compare different translation averaging algorithms
GlobalTranslations l1_result = RunL1TranslationAveraging(relative_translations);
GlobalTranslations l2_result = RunL2TranslationAveraging(relative_translations);
GlobalTranslations robust_result = RunRobustTranslationAveraging(relative_translations);
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// Evaluate each algorithm
auto eval_algorithm = [&](const GlobalTranslations& result, const std::string& name) {
    std::vector<double> errors = ComputePositionErrors(result, ground_truth);
    double mean_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    std::cout << name << " mean position error: " << mean_error << std::endl;
};

eval_algorithm(l1_result, "L1 averaging");
eval_algorithm(l2_result, "L2 averaging");
eval_algorithm(robust_result, "Robust averaging");
```

### 3. Incremental Translation Accumulation Error Analysis

```cpp
// Analyze translation accumulation error in incremental SfM
std::vector<GlobalTranslations> incremental_results;
GlobalTranslations ground_truth = LoadGroundTruthTranslations();

// Simulate incremental construction process
for (size_t num_views = 3; num_views <= ground_truth.size(); ++num_views) {
    GlobalTranslations partial_result = RunIncrementalSfM(images, matches, num_views);
    incremental_results.push_back(partial_result);
    
    // Evaluate current stage error
    std::vector<double> errors = ComputePositionErrors(partial_result, 
        GlobalTranslations(ground_truth.begin(), ground_truth.begin() + num_views));
    
    double mean_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    std::cout << "Number of views: " << num_views << ", mean position error: " << mean_error << std::endl;
}
```

## Notes

### 1. Coordinate System Consistency

```cpp
// Ensure translation physical meaning consistency
if (estimated_poses.GetPoseFormat() == PoseFormat::RwTw && 
    ground_truth_poses.GetPoseFormat() == PoseFormat::RwTc) {
    
    // Format conversion needed
    GlobalPoses gt_copy = ground_truth_poses;
    gt_copy.ConvertPoseFormat(PoseFormat::RwTw);
    
    // Now can directly compare translations
    auto est_translations = estimated_poses.translations;
    auto gt_translations = gt_copy.translations;
}
```

### 2. Scale Uncertainty Handling

```cpp
// For scale-uncertain cases, use direction error evaluation
if (scale_unknown) {
    std::vector<double> direction_errors = ComputeDirectionErrors(estimated, ground_truth);
    // Analyze direction accuracy rather than absolute position accuracy
} else {
    std::vector<double> position_errors = ComputePositionErrors(estimated, ground_truth);
    // Analyze absolute position accuracy
}
```

### 3. Reference Coordinate System Selection

```cpp
// Select appropriate reference point for direction error calculation
Vector3d reference_center;

// Option 1: Use centroid as reference
reference_center = ComputeCentroid(ground_truth);

// Option 2: Use first camera as reference
reference_center = ground_truth[0];

// Option 3: Use origin as reference
reference_center = Vector3d::Zero();

std::vector<double> direction_errors = ComputeDirectionErrors(
    estimated, ground_truth, reference_center);
```

### 4. Outlier Detection

```cpp
// Detect and handle abnormal translation values
std::vector<double> position_errors = ComputePositionErrors(estimated, ground_truth);

// Calculate error statistics
double mean_error = std::accumulate(position_errors.begin(), position_errors.end(), 0.0) / position_errors.size();
double variance = 0;
for (double error : position_errors) {
    variance += (error - mean_error) * (error - mean_error);
}
variance /= position_errors.size();
double std_dev = std::sqrt(variance);

// Mark outliers (3σ principle)
for (size_t i = 0; i < position_errors.size(); ++i) {
    if (position_errors[i] > mean_error + 3 * std_dev) {
        std::cout << "Camera " << i << " translation error abnormal: " << position_errors[i] << std::endl;
    }
}
```

---

**Related References**:
- [PoSDK Pose Conventions](../conventions/posdk.md)
- [Global Pose Evaluation](global_poses.md)
- [Relative Pose Evaluation](relative_poses.md)
- [Evaluation System Overview](index.md)
