# PoSDK Accuracy Evaluation System

PoSDK provides a complete accuracy evaluation system for measuring the estimation accuracy of relative poses, global poses, and global translations. This chapter details the mathematical principles, implementation details, and usage methods of various evaluation methods.

## Evaluation System Overview

PoSDK's accuracy evaluation system includes the following three main components:

### 1. [Relative Pose Evaluation](relative_poses.md)
- **Evaluation Object**: Relative pose $(R_{ij}, \mathbf{t}_{ij})$ between two camera views
- **Main Metrics**: Rotation error, translation angle error
- **Use Cases**: Two-view geometry, stereo vision, incremental SfM, etc.

### 2. [Global Pose Evaluation](global_poses.md)
- **Evaluation Object**: Global pose $(R_w^c, \mathbf{t}_w)$ of camera in world coordinate system
- **Main Metrics**: Rotation error, position error
- **Evaluation Methods**:
  - Complete alignment evaluation based on similarity transform
  - Rotation-only evaluation (EvaluateRotations)
  - Rotation evaluation using SVD method (EvaluateRotationsSVD)

### 3. [Global Translation Evaluation](global_translations.md)
- **Evaluation Object**: Camera center position $\mathbf{t}_w$ in world coordinate system
- **Main Metrics**: Position error, direction error
- **Use Cases**: Global SfM, translation estimation verification, etc.

## Evaluation Process

All evaluations follow a unified process:

1. **Data Preprocessing**: Format conversion, coordinate system alignment
2. **Geometric Alignment**: Calculate optimal transformation parameters (similarity transform, rigid transform, etc.)
3. **Error Calculation**: Calculate various error metrics based on alignment results
4. **Result Output**: Output evaluation results uniformly through `EvaluatorStatus`

## Mathematical Foundation

### Rotation Error Calculation
For rotation matrices $R_{gt}$ (ground truth) and $R_{est}$ (estimated), rotation error is defined as:

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{gt}^T R_{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

### Position Error Calculation
For position vectors $\mathbf{p}_{gt}$ (ground truth) and $\mathbf{p}_{est}$ (estimated), position error is defined as:

$$\text{Position Error} = ||\mathbf{p}_{gt} - \mathbf{p}_{est}||_2$$

### Similarity Transform Alignment
To eliminate the effects of scale, rotation, and translation, PoSDK uses a 7-degree-of-freedom similarity transform for alignment:

$$\mathbf{p}_{aligned} = s \cdot R_{align} \cdot \mathbf{p}_{est} + \mathbf{t}_{align}$$

where $s$ is the scale factor, $R_{align}$ is the alignment rotation, and $\mathbf{t}_{align}$ is the alignment translation.

## Usage Example

```cpp
#include <po_core/data/data_global_poses.hpp>

// Create global pose data objects
DataGlobalPoses estimated_poses;
DataGlobalPoses ground_truth_poses;

// Execute evaluation
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // Get rotation errors
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    
    // Get position errors
    auto position_errors = eval_result.GetResults("translation_error");
    
    // Print statistics
    std::cout << "Mean rotation error: " << eval_result.GetMeanError("rotation_error_deg") << "°" << std::endl;
    std::cout << "Mean position error: " << eval_result.GetMeanError("translation_error") << std::endl;
}
```

## Notes

1. **Coordinate System Consistency**: Ensure ground truth and estimated values use the same coordinate system convention
2. **Pose Format**: PoSDK defaults to `PoseFormat::RwTw` format, and format conversion is performed automatically before evaluation
3. **Data Correspondence**: Evaluation automatically matches corresponding camera views, unmatched views will be ignored
4. **Numerical Stability**: All calculations include numerical stability checks, outliers will be recorded and handled

```{toctree}
:maxdepth: 2
:caption: Evaluation Method Details

relative_poses
global_poses
global_translations
```

---

**References**:
- Horn, B. K. P. (1987). Closed-form solution of absolute orientation using unit quaternions
- Umeyama, S. (1991). Least-squares estimation of transformation parameters between two point patterns
- Hartley, R., & Zisserman, A. (2003). Multiple view geometry in computer vision
