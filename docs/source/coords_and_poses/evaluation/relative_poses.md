# Relative Pose Evaluation (RelativePoses Evaluation)

Relative pose evaluation is used to measure the accuracy of relative pose estimation between two camera views. In PoSDK, relative poses are defined by rotation matrix $R_{ij}$ and translation vector $\mathbf{t}_{ij}$.

## Mathematical Definition

### Relative Pose Representation
The relative pose between two cameras (view $i$ and view $j$) is defined as:

$$\mathbf{x}_j \sim R_{ij} \cdot \mathbf{x}_i + \mathbf{t}_{ij}$$

where:
- $R_{ij}$: Rotation matrix from camera $i$ to camera $j$ (3×3)
- $\mathbf{t}_{ij}$: Coordinates of camera $i$'s origin in camera $j$'s coordinate system (3×1)
- $\mathbf{x}_i, \mathbf{x}_j$: Normalized image coordinates in camera $i$ and $j$ coordinate systems respectively

## Evaluation Metrics

### 1. Rotation Error

For ground truth rotation $R_{ij}^{gt}$ and estimated rotation $R_{ij}^{est}$, rotation error is calculated as:

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{ij}^{gt\,T} R_{ij}^{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

**Physical Meaning**: Minimum rotation angle between two rotation matrices, in degrees.

### 2. Translation Angular Error

For ground truth translation $\mathbf{t}_{ij}^{gt}$ and estimated translation $\mathbf{t}_{ij}^{est}$, translation angular error is calculated as:

$$\text{Translation Error} = \arccos\left(\frac{\mathbf{t}_{ij}^{gt\,T} \mathbf{t}_{ij}^{est}}{||\mathbf{t}_{ij}^{gt}|| \cdot ||\mathbf{t}_{ij}^{est}||}\right) \times \frac{180°}{\pi}$$

**Physical Meaning**: Angle between two translation directions, in degrees. Note that this evaluates direction rather than scale, as translation scale is arbitrary in two-view geometry.

## Implementation Details

### Core Evaluation Function

```cpp
// Located in types::RelativePoses class
size_t EvaluateAgainst(
    const RelativePoses& gt_poses,
    std::vector<double>& rotation_errors,
    std::vector<double>& translation_errors) const;
```

### Evaluation Process

1. **Data Matching**: Match estimated and ground truth values based on view pairs $(i,j)$
2. **Error Calculation**: Calculate rotation and translation errors for each matched view pair
3. **Result Output**: Return the number of matched view pairs and error vectors

### Data Structure

```cpp
// Relative pose data structure (types.hpp)
struct RelativePose {
    ViewId i, j;           // View pair IDs
    Matrix3d Rij;          // Rotation matrix
    Vector3d tij;          // Translation vector
    double confidence;     // Confidence (optional)
};

using RelativePoses = std::vector<RelativePose>;
```

## Usage Examples

### Basic Usage

```cpp
#include <po_core/data/data_relative_poses.hpp>

// Create relative pose data objects
DataRelativePoses estimated_poses;
DataRelativePoses ground_truth_poses;

// Load data (example)
estimated_poses.LoadFromFile("estimated_relative_poses.pb");
ground_truth_poses.LoadFromFile("gt_relative_poses.pb");

// Execute evaluation
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // Get evaluation results
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    auto translation_errors = eval_result.GetResults("translation_error_deg");
    
    // Calculate statistics
    double mean_rot_error = eval_result.GetMeanError("rotation_error_deg");
    double mean_trans_error = eval_result.GetMeanError("translation_error_deg");
    
    std::cout << "Evaluation successful!" << std::endl;
    std::cout << "Number of matched view pairs: " << rotation_errors.size() << std::endl;
    std::cout << "Mean rotation error: " << mean_rot_error << "°" << std::endl;
    std::cout << "Mean translation angular error: " << mean_trans_error << "°" << std::endl;
}
```

### Direct Low-Level API Usage

```cpp
#include <po_core/types.hpp>

using namespace PoSDK::types;

// Create relative pose data
RelativePoses estimated_poses = {
    {0, 1, R01_est, t01_est, 1.0},
    {0, 2, R02_est, t02_est, 0.9},
    // ... more pose pairs
};

RelativePoses ground_truth_poses = {
    {0, 1, R01_gt, t01_gt, 1.0},
    {0, 2, R02_gt, t02_gt, 1.0},
    // ... corresponding ground truth
};

// Direct evaluation
std::vector<double> rotation_errors, translation_errors;
size_t matched_pairs = estimated_poses.EvaluateAgainst(
    ground_truth_poses, rotation_errors, translation_errors);

std::cout << "Matched view pairs: " << matched_pairs << std::endl;
for (size_t i = 0; i < rotation_errors.size(); ++i) {
    std::cout << "View pair (" << estimated_poses[i].i << "," << estimated_poses[i].j 
              << "): rotation error=" << rotation_errors[i] << "°, "
              << "translation error=" << translation_errors[i] << "°" << std::endl;
}
```

## Typical Application Scenarios

### 1. Two-View Geometry Verification
```cpp
// Verify relative pose obtained from fundamental matrix decomposition
Matrix3d F = ComputeFundamentalMatrix(matches);
std::vector<RelativePose> candidates = DecomposeFundamentalMatrix(F);

// Compare with ground truth to select best candidate
for (const auto& candidate : candidates) {
    RelativePoses est_poses = {candidate};
    auto eval_result = EvaluateRelativePoses(est_poses, gt_poses);
    // Select candidate with minimum error
}
```

### 2. Incremental SfM Quality Monitoring
```cpp
// Monitor relative pose quality during incremental SfM process
for (const auto& new_view_pair : new_pairs) {
    RelativePose estimated = EstimateRelativePose(new_view_pair);
    RelativePose ground_truth = GetGroundTruthPose(new_view_pair);
    
    double rot_error = ComputeRotationError(estimated.Rij, ground_truth.Rij);
    if (rot_error > threshold) {
        LOG_WARNING << "View pair " << new_view_pair << " rotation error too large: " << rot_error;
    }
}
```

## Notes

### 1. Coordinate System Consistency
Ensure estimated and ground truth values use the same coordinate system convention:
```cpp
// PoSDK relative pose convention
// x_j ~ R_ij * x_i + t_ij
// where R_ij represents rotation from camera i to camera j
// t_ij represents coordinates of camera i's origin in camera j's coordinate system
```

### 2. Translation Scale Uncertainty
Absolute translation scale is uncertain in two-view geometry, so we evaluate translation direction rather than magnitude:
```cpp
// Correct: Evaluate translation direction angle
double angle_error = acos(dot(t_gt_normalized, t_est_normalized));

// Incorrect: Directly compare translation magnitude
double magnitude_error = norm(t_gt) - norm(t_est);  // Not recommended
```

### 3. Data Matching Strategy
Correct view pair matching is required during evaluation:
```cpp
// Automatic matching: Based on view IDs
size_t matched = estimated.EvaluateAgainst(ground_truth, rot_errors, trans_errors);

// Manual matching: Ensure correct data correspondence
for (const auto& est_pose : estimated_poses) {
    auto gt_it = std::find_if(gt_poses.begin(), gt_poses.end(),
        [&](const RelativePose& gt) { 
            return gt.i == est_pose.i && gt.j == est_pose.j; 
        });
    if (gt_it != gt_poses.end()) {
        // Calculate error for this view pair
    }
}
```

### 4. Outlier Handling
```cpp
// Check validity of calculation results
if (std::isnan(rotation_error) || std::isinf(rotation_error)) {
    LOG_WARNING << "Rotation error calculation abnormal, skipping this view pair";
    continue;
}

// Set reasonable error thresholds
const double MAX_ROTATION_ERROR = 180.0;  // degrees
const double MAX_TRANSLATION_ERROR = 180.0;  // degrees
```

---

**Related References**:
- [PoSDK Pose Conventions](../conventions/posdk.md)
- [Global Pose Evaluation](global_poses.md)
- [Evaluation System Overview](index.md)
