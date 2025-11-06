# Global Pose Evaluation (GlobalPoses Evaluation)

Global pose evaluation is used to measure the accuracy of camera global pose estimation in the world coordinate system. PoSDK provides multiple evaluation methods, including complete similarity transform alignment evaluation and rotation-only evaluation.

## Mathematical Definition

### Global Pose Representation
In PoSDK, global poses default to `PoseFormat::RwTw` format:

$$\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$$

where:
- $R_w^c$: Rotation matrix from world coordinate system to camera coordinate system (3×3)
- $\mathbf{t}_w$: Camera center position in world coordinate system (3×1)
- $\mathbf{X}_w$: 3D point in world coordinate system
- $\mathbf{x}_c$: Normalized image coordinates in camera coordinate system

## Evaluation Methods

PoSDK provides three main global pose evaluation methods:

### 1. Complete Similarity Transform Evaluation (EvaluateWithSimilarityTransform)

**Purpose**: Most comprehensive evaluation method, aligning estimated poses to ground truth through 7-degree-of-freedom similarity transform.

**Mathematical Principle**:
Calculate optimal similarity transform parameters $(s, R_{align}, \mathbf{t}_{align})$:

$$\mathbf{p}_{aligned} = s \cdot R_{align} \cdot \mathbf{p}_{est} + \mathbf{t}_{align}$$

**Implementation Features**:
- Uses Ceres optimizer to solve for optimal transform parameters
- Uniformly converts to `RwTw` format for evaluation
- Provides highest precision alignment results

```cpp
bool EvaluateWithSimilarityTransform(
    const GlobalPoses& gt_poses,
    std::vector<double>& position_errors,
    std::vector<double>& rotation_errors) const;
```

### 2. Standard Similarity Transform Evaluation (EvaluateAgainst)

**Purpose**: Uses analytical method to calculate similarity transform, higher computational efficiency.

**Mathematical Principle**:
Calculate optimal rotation and scale based on SVD decomposition:

1. **Centroid calculation**: $\bar{\mathbf{p}}_{gt} = \frac{1}{n}\sum_{i=1}^n \mathbf{p}_{gt}^i$
2. **Centering**: $\mathbf{p}_{gt,c}^i = \mathbf{p}_{gt}^i - \bar{\mathbf{p}}_{gt}$
3. **Scale calculation**: $s = \sqrt{\frac{\sum||\mathbf{p}_{gt,c}^i||^2}{\sum||\mathbf{p}_{est,c}^i||^2}}$
4. **Rotation calculation**: $R_{align} = \arg\min_R \sum||s R \mathbf{p}_{est,c}^i - \mathbf{p}_{gt,c}^i||^2$

```cpp
bool EvaluateAgainst(
    const GlobalPoses& gt_poses,
    std::vector<double>& position_errors,
    std::vector<double>& rotation_errors,
    bool compute_similarity = true) const;
```

### 3. Rotation-Only Evaluation (EvaluateRotations / EvaluateRotationsSVD)

**Purpose**: Evaluates only rotation part accuracy, ignoring position information.

**Mathematical Principle**:
1. **Align first view**: $R_{gt,aligned}^i = R_{gt}^i \cdot (R_{gt}^1)^{-1}$
2. **Calculate rotation compensation**: Calculate optimal rotation compensation $R_{opt}$ through Ceres optimization or SVD method
3. **Apply compensation**: $R_{est,compensated}^i = R_{est,aligned}^i \cdot R_{opt}$

```cpp
// Ceres optimization method
bool EvaluateRotations(
    const GlobalPoses& gt_poses,
    std::vector<double>& rotation_errors) const;

// SVD method  
bool EvaluateRotationsSVD(
    const GlobalPoses& gt_poses,
    std::vector<double>& rotation_errors) const;
```

## Evaluation Metrics

### 1. Rotation Error

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{gt}^T R_{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

**Physical Meaning**: Minimum rotation angle between two rotation matrices, in degrees.

### 2. Position Error

$$\text{Position Error} = ||\mathbf{t}_{gt} - \mathbf{t}_{est}||_2$$

**Physical Meaning**: Euclidean distance error of camera center position.

## Implementation Details

### Data Structure

```cpp
// Global pose data structure (types.hpp)
class GlobalPoses {
public:
    GlobalRotations rotations;       // Rotation matrix array
    GlobalTranslations translations; // Translation vector array
    EstInfo est_info;               // Estimation information
    PoseFormat pose_format;         // Pose format
    
    // Evaluation methods
    bool EvaluateWithSimilarityTransform(...) const;
    bool EvaluateAgainst(...) const;
    bool EvaluateRotations(...) const;
    bool EvaluateRotationsSVD(...) const;
};
```

### Rotation Compensation Optimization

PoSDK uses Ceres optimizer for rotation compensation calculation:

```cpp
// Rotation compensation cost function
struct RotationCompensationError {
    template<typename T>
    bool operator()(const T* const rotation_compensation, T* residuals) const {
        // Apply rotation compensation: R_compensated = R_est * R_opt
        T compensation_matrix[9];
        ceres::AngleAxisToRotationMatrix(rotation_compensation, compensation_matrix);
        
        // Calculate rotation error: ΔR = R_gt^(-1) * R_compensated
        // Residual is the magnitude of rotation vector
        residuals[0] = ||ω(ΔR)||;
        return true;
    }
};
```

## Usage Examples

### Complete Evaluation Example

```cpp
#include <po_core/data/data_global_poses.hpp>

// Create global pose data objects
DataGlobalPoses estimated_poses;
DataGlobalPoses ground_truth_poses;

// Load data
estimated_poses.LoadFromFile("estimated_global_poses.pb");
ground_truth_poses.LoadFromFile("gt_global_poses.pb");

// Method 1: Use DataGlobalPoses Evaluate interface
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // Get evaluation results
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    auto position_errors = eval_result.GetResults("translation_error");
    
    // Print statistics
    std::cout << "Evaluation successful!" << std::endl;
    std::cout << "Number of evaluated poses: " << rotation_errors.size() << std::endl;
    std::cout << "Mean rotation error: " << eval_result.GetMeanError("rotation_error_deg") << "°" << std::endl;
    std::cout << "Mean position error: " << eval_result.GetMeanError("translation_error") << std::endl;
}
```

### Using Different Evaluation Methods

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

GlobalPoses estimated_poses, ground_truth_poses;
// ... Load data ...

std::vector<double> position_errors, rotation_errors;

// Method 1: High-precision similarity transform evaluation
bool success1 = estimated_poses.EvaluateWithSimilarityTransform(
    ground_truth_poses, position_errors, rotation_errors);

// Method 2: Standard similarity transform evaluation
bool success2 = estimated_poses.EvaluateAgainst(
    ground_truth_poses, position_errors, rotation_errors, true);

// Method 3: Rotation-only evaluation (Ceres optimization)
bool success3 = estimated_poses.EvaluateRotations(
    ground_truth_poses, rotation_errors);

// Method 4: Rotation-only evaluation (SVD method)
bool success4 = estimated_poses.EvaluateRotationsSVD(
    ground_truth_poses, rotation_errors);

// Output results
if (success1) {
    std::cout << "Similarity transform evaluation completed" << std::endl;
    for (size_t i = 0; i < position_errors.size(); ++i) {
        std::cout << "Camera " << i << ": position error=" << position_errors[i] 
                  << ", rotation error=" << rotation_errors[i] << "°" << std::endl;
    }
}
```

### Format Conversion and Evaluation

```cpp
// Handle different pose formats
GlobalPoses poses_rwtw, poses_rwtc;

// Load poses in RwTw format
poses_rwtw.LoadFromFile("poses_rwtw.pb");
poses_rwtw.SetPoseFormat(PoseFormat::RwTw);

// Load poses in RwTc format  
poses_rwtc.LoadFromFile("poses_rwtc.pb");
poses_rwtc.SetPoseFormat(PoseFormat::RwTc);

// Format conversion is performed automatically during evaluation
std::vector<double> pos_errors, rot_errors;
bool success = poses_rwtw.EvaluateWithSimilarityTransform(
    poses_rwtc, pos_errors, rot_errors);
// PoSDK will automatically convert poses_rwtc to RwTw format for evaluation
```

## Typical Application Scenarios

### 1. Global SfM Quality Evaluation

```cpp
// Evaluate global SfM results
GlobalPoses sfm_result = RunGlobalSfM(images, matches);
GlobalPoses ground_truth = LoadGroundTruthPoses();

std::vector<double> pos_errors, rot_errors;
bool success = sfm_result.EvaluateWithSimilarityTransform(
    ground_truth, pos_errors, rot_errors);

// Calculate statistics
double mean_pos_error = std::accumulate(pos_errors.begin(), pos_errors.end(), 0.0) / pos_errors.size();
double mean_rot_error = std::accumulate(rot_errors.begin(), rot_errors.end(), 0.0) / rot_errors.size();

std::cout << "SfM quality evaluation:" << std::endl;
std::cout << "  Mean position error: " << mean_pos_error << std::endl;
std::cout << "  Mean rotation error: " << mean_rot_error << "°" << std::endl;
```

### 2. Rotation Averaging Algorithm Evaluation

```cpp
// Evaluate only rotation averaging results
GlobalPoses rotation_averaged_poses = RunRotationAveraging(relative_poses);
GlobalPoses ground_truth = LoadGroundTruthPoses();

std::vector<double> rotation_errors;

// Use SVD method to evaluate rotation accuracy
bool success = rotation_averaged_poses.EvaluateRotationsSVD(
    ground_truth, rotation_errors);

if (success) {
    double max_error = *std::max_element(rotation_errors.begin(), rotation_errors.end());
    double mean_error = std::accumulate(rotation_errors.begin(), rotation_errors.end(), 0.0) / rotation_errors.size();
    
    std::cout << "Rotation averaging evaluation:" << std::endl;
    std::cout << "  Mean rotation error: " << mean_error << "°" << std::endl;
    std::cout << "  Max rotation error: " << max_error << "°" << std::endl;
}
```

### 3. Incremental vs Global SfM Comparison

```cpp
// Compare different SfM methods
GlobalPoses incremental_result = RunIncrementalSfM(images, matches);
GlobalPoses global_result = RunGlobalSfM(images, matches);
GlobalPoses ground_truth = LoadGroundTruthPoses();

// Evaluate incremental SfM
std::vector<double> inc_pos_errors, inc_rot_errors;
incremental_result.EvaluateWithSimilarityTransform(
    ground_truth, inc_pos_errors, inc_rot_errors);

// Evaluate global SfM
std::vector<double> glob_pos_errors, glob_rot_errors;
global_result.EvaluateWithSimilarityTransform(
    ground_truth, glob_pos_errors, glob_rot_errors);

// Compare results
auto mean_inc_pos = std::accumulate(inc_pos_errors.begin(), inc_pos_errors.end(), 0.0) / inc_pos_errors.size();
auto mean_glob_pos = std::accumulate(glob_pos_errors.begin(), glob_pos_errors.end(), 0.0) / glob_pos_errors.size();

std::cout << "SfM method comparison:" << std::endl;
std::cout << "  Incremental mean position error: " << mean_inc_pos << std::endl;
std::cout << "  Global mean position error: " << mean_glob_pos << std::endl;
```

## Notes

### 1. Pose Format Consistency
```cpp
// Ensure format consistency
if (estimated.GetPoseFormat() != ground_truth.GetPoseFormat()) {
    LOG_INFO << "Pose formats differ, will automatically convert";
    // PoSDK will automatically handle format conversion
}
```

### 2. Data Validity Check
```cpp
// Check data completeness
if (estimated.Size() == 0 || ground_truth.Size() == 0) {
    LOG_ERROR << "Pose data is empty";
    return false;
}

// Check numerical validity
for (size_t i = 0; i < estimated.Size(); ++i) {
    if (!estimated.rotations[i].allFinite() || 
        !estimated.translations[i].allFinite()) {
        LOG_WARNING << "Pose " << i << " contains invalid values";
    }
}
```

### 3. Evaluation Method Selection

| Evaluation Method                 | Use Case            | Advantages                    | Disadvantages                       |
| --------------------------------- | ------------------- | ----------------------------- | ----------------------------------- |
| `EvaluateWithSimilarityTransform` | Final accuracy eval | Highest precision, Ceres opt  | Slower computation                  |
| `EvaluateAgainst`                 | Regular evaluation  | High computational efficiency | Slightly lower precision than Ceres |
| `EvaluateRotations`               | Rotation algo eval  | Focus on rotation, Ceres opt  | Ignores position information        |
| `EvaluateRotationsSVD`            | Fast rotation eval  | Fastest computation           | Slightly lower precision            |

### 4. Exception Handling
```cpp
try {
    std::vector<double> pos_errors, rot_errors;
    bool success = estimated.EvaluateWithSimilarityTransform(
        ground_truth, pos_errors, rot_errors);
    
    if (!success) {
        LOG_ERROR << "Evaluation failed, possibly data format or numerical issue";
        return;
    }
    
    // Check result validity
    for (double error : pos_errors) {
        if (std::isnan(error) || std::isinf(error)) {
            LOG_WARNING << "Detected invalid position error value";
        }
    }
} catch (const std::exception& e) {
    LOG_ERROR << "Exception occurred during evaluation: " << e.what();
}
```

---

**Related References**:
- [PoSDK Pose Conventions](../conventions/posdk.md)
- [Relative Pose Evaluation](relative_poses.md)
- [Global Translation Evaluation](global_translations.md)
- [Evaluation System Overview](index.md)
