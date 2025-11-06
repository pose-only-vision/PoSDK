# OpenGV Data Converter
(opengv-converter)=

The OpenGV data converter provides data conversion between PoSDK and the OpenGV library for geometric vision algorithms and relative pose estimation.

---

## Conversion Function Overview

### Core Conversion Functions
(opengv-core-functions)=

| Conversion Direction | Data Type                             | Purpose                  | Conversion Function                                                   |
| -------------------- | ------------------------------------- | ------------------------ | --------------------------------------------------------------------- |
| **PoSDK → OpenGV**   | Matches → Bearing vectors             | Relative pose estimation | [`MatchesToBearingVectors`](matches-to-bearing-vectors)               |
| **PoSDK → OpenGV**   | BearingPairs → BearingVectors         | OpenGV algorithm input   | [`BearingPairsToBearingVectors`](bearing-pairs-to-bearing-vectors)    |
| **OpenGV → PoSDK**   | Transformation matrix → Relative pose | Pose result extraction   | [`OpenGVPose2RelativePose`](opengv-pose-to-relative-pose)             |
| **PoSDK → OpenGV**   | Camera model → Intrinsic matrix       | Algorithm initialization | [`CameraModel2OpenGVCalibration`](camera-model-to-opengv-calibration) |

---

## Bearing Vector Conversion
(opengv-bearing-conversion)=

### `MatchesToBearingVectors` (Match Conversion)
(matches-to-bearing-vectors)=

Convert PoSDK matches and feature data to OpenGV bearing vectors

**Function Signature**:
```cpp
static bool MatchesToBearingVectors(
    const IdMatches &matches,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    opengv::bearingVectors_t &bearingVectors1,
    opengv::bearingVectors_t &bearingVectors2);
```

**Parameter Description**:
| Parameter         | Type                        | Description                        |
| ----------------- | --------------------------- | ---------------------------------- |
| `matches`         | `const IdMatches&`          | [Input] Feature matches            |
| `features_info`   | `const FeaturesInfo&`       | [Input] Feature information        |
| `camera_models`   | `const CameraModels&`       | [Input] Camera model collection    |
| `view_pair`       | `const ViewPair&`           | [Input] View pair                  |
| `bearingVectors1` | `opengv::bearingVectors_t&` | [Output] First frame unit vectors  |
| `bearingVectors2` | `opengv::bearingVectors_t&` | [Output] Second frame unit vectors |

**Return Value**: `bool` - Whether conversion succeeded

**Conversion Process**:
```
1. Get feature point pair indices from matches
   ↓
2. Get pixel coordinates from features_info
   ↓
3. Undistort and normalize using camera_models
   ↓
4. Convert to 3D unit vectors (bearing vectors)
```

**Usage Example**:
```cpp
#include "converter_opengv.hpp"
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/relative_pose/methods.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// 1. Prepare PoSDK data
types::IdMatches matches = GetMatches();
types::FeaturesInfo features_info = GetFeaturesInfo();
types::CameraModels camera_models = GetCameraModels();
types::ViewPair view_pair(0, 1);

// 2. Convert to OpenGV bearing vectors
opengv::bearingVectors_t bearings1, bearings2;
bool success = OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);

if (success) {
    // 3. Use OpenGV for relative pose estimation
    opengv::relative_pose::CentralRelativeAdapter adapter(
        bearings1, bearings2);
    
    // 5-point algorithm
    opengv::transformation_t T = 
        opengv::relative_pose::fivept_nister(adapter);
    
    LOG_INFO_ZH << "Used " << bearings1.size() << " bearing vector pairs";
}
```

### `MatchesToBearingVectors` (Sample Conversion)

Convert PoSDK match samples to OpenGV bearing vectors (for RANSAC)

**Function Signature**:
```cpp
static bool MatchesToBearingVectors(
    const DataSample<IdMatches> &matches_sample,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    opengv::bearingVectors_t &bearingVectors1,
    opengv::bearingVectors_t &bearingVectors2);
```

**Use Case**: Subset conversion for RANSAC sampling

**Usage Example**:
```cpp
// Use in RANSAC loop
for (size_t i = 0; i < num_iterations; ++i) {
    // Sample subset
    DataSample<IdMatches> sample = SampleMatches(matches, sample_size);
    
    // Convert sampled matches
    opengv::bearingVectors_t bearings1, bearings2;
    OpenGVConverter::MatchesToBearingVectors(
        sample, features_info, camera_models, view_pair,
        bearings1, bearings2);
    
    // OpenGV pose estimation
    opengv::transformation_t T = EstimatePose(bearings1, bearings2);
    
    // Evaluate inliers
    EvaluateInliers(T);
}
```

### `BearingPairsToBearingVectors`
(bearing-pairs-to-bearing-vectors)=

Convert BearingPairs to BearingVectors

**Function Signature**:
```cpp
static void BearingPairsToBearingVectors(
    const BearingPairs &bearing_pairs, 
    BearingVectors &bearing_vectors1, 
    BearingVectors &bearing_vectors2);
```

**Parameter Description**:
| Parameter          | Type                  | Description                              |
| ------------------ | --------------------- | ---------------------------------------- |
| `bearing_pairs`    | `const BearingPairs&` | [Input] Bearing pair collection          |
| `bearing_vectors1` | `BearingVectors&`     | [Output] First group of bearing vectors  |
| `bearing_vectors2` | `BearingVectors&`     | [Output] Second group of bearing vectors |

**Data Format Description**:
```cpp
// BearingPairs: std::vector<std::pair<Vector3d, Vector3d>>
// BearingVectors: std::vector<Vector3d>
```

**Usage Example**:
```cpp
// PoSDK format: paired bearing vectors
types::BearingPairs bearing_pairs = GetBearingPairs();

// Separate into two groups of vectors (OpenGV format)
types::BearingVectors bearings1, bearings2;
OpenGVConverter::BearingPairsToBearingVectors(
    bearing_pairs, bearings1, bearings2);

// Use OpenGV Adapter
opengv::relative_pose::CentralRelativeAdapter adapter(
    bearings1, bearings2);
```

### `BearingVectorsToBearingPairs`
(bearing-vectors-to-bearing-pairs)=

Convert BearingVectors to BearingPairs

**Function Signature**:
```cpp
static void BearingVectorsToBearingPairs(
    const BearingVectors &bearing_vectors1, 
    const BearingVectors &bearing_vectors2, 
    BearingPairs &bearing_pairs);
```

**Use Case**: Convert OpenGV results back to PoSDK format

---

## Pose Conversion
(opengv-pose-conversion)=

### `OpenGVPose2RelativePose`
(opengv-pose-to-relative-pose)=

Convert OpenGV pose results to PoMVG relative pose

**Function Signature**:
```cpp
static bool OpenGVPose2RelativePose(
    const opengv::transformation_t &T,
    RelativePose &relative_pose);
```

**Parameter Description**:
| Parameter       | Type                              | Description                                |
| --------------- | --------------------------------- | ------------------------------------------ |
| `T`             | `const opengv::transformation_t&` | [Input] OpenGV transformation matrix (4×4) |
| `relative_pose` | `RelativePose&`                   | [Output] PoMVG relative pose               |

**Return Value**: `bool` - Whether conversion succeeded

**Transformation Matrix Format**:
```
OpenGV transformation_t (Eigen::Matrix<double, 3, 4>):
[R | t]  where R is 3×3 rotation matrix, t is 3×1 translation vector
```

**Usage Example**:
```cpp
// 1. OpenGV relative pose estimation
opengv::relative_pose::CentralRelativeAdapter adapter(bearings1, bearings2);
opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);

// 2. Convert to PoSDK format
types::RelativePose relative_pose;
bool success = OpenGVConverter::OpenGVPose2RelativePose(T, relative_pose);

if (success) {
    // 3. Access relative pose data
    const Matrix3d& R = relative_pose.GetRotation();
    const Vector3d& t = relative_pose.GetTranslation();
    
    LOG_INFO_ZH << "Relative rotation: \n" << R;
    LOG_INFO_ZH << "Relative translation: " << t.transpose();
    
    // 4. Store or further process
    relative_poses.AddPose(view_pair, relative_pose);
}
```

---

## Camera Parameter Conversion
(opengv-camera-conversion)=

### `CameraModel2OpenGVCalibration`
(camera-model-to-opengv-calibration)=

Convert PoMVG camera intrinsics to OpenGV camera parameters

**Function Signature**:
```cpp
static bool CameraModel2OpenGVCalibration(
    const CameraModel &camera_model,
    Eigen::Matrix3d &K);
```

**Parameter Description**:
| Parameter      | Type                 | Description                             |
| -------------- | -------------------- | --------------------------------------- |
| `camera_model` | `const CameraModel&` | [Input] PoMVG camera model              |
| `K`            | `Eigen::Matrix3d&`   | [Output] OpenGV camera intrinsic matrix |

**Return Value**: `bool` - Whether conversion succeeded

**Intrinsic Matrix Format**:
```
K = [fx  0  cx]
    [ 0 fy  cy]
    [ 0  0   1]
```

**Usage Example**:
```cpp
// PoSDK camera model
types::CameraModel camera_model = GetCameraModel(view_id);

// Convert to OpenGV intrinsic matrix
Eigen::Matrix3d K;
bool success = OpenGVConverter::CameraModel2OpenGVCalibration(
    camera_model, K);

if (success) {
    // Use OpenGV non-central camera algorithms
    opengv::relative_pose::NoncentralRelativeAdapter adapter(
        bearings1, bearings2,
        camOffsets1, camOffsets2,
        camRotations1, camRotations2);
    
    // Or for verification and debugging
    LOG_INFO_ZH << "Camera intrinsic matrix:\n" << K;
}
```

### `PixelToBearingVector`
(pixel-to-bearing-vector)=

Convert pixel coordinates to bearing vector

**Function Signature**:
```cpp
static opengv::bearingVector_t PixelToBearingVector(
    const Vector2d &pixel_coord,
    const CameraModel &camera_model);
```

**Parameter Description**:
| Parameter      | Type                 | Description               |
| -------------- | -------------------- | ------------------------- |
| `pixel_coord`  | `const Vector2d&`    | [Input] Pixel coordinates |
| `camera_model` | `const CameraModel&` | [Input] Camera model      |

**Return Value**: `opengv::bearingVector_t` - 3D unit vector

**Conversion Process**:
```
Pixel coordinates (u, v)
    ↓ Undistort
Normalized coordinates (x, y)
    ↓ Add z=1
3D coordinates (x, y, 1)
    ↓ Normalize
Unit vector (x/||·||, y/||·||, 1/||·||)
```

**Usage Example**:
```cpp
// Single pixel point conversion
Vector2d pixel(320.5, 240.8);
types::CameraModel camera = GetCameraModel(0);

opengv::bearingVector_t bearing = 
    OpenGVConverter::PixelToBearingVector(pixel, camera);

// bearing is a unit vector
assert(std::abs(bearing.norm() - 1.0) < 1e-6);

LOG_INFO_ZH << "Bearing vector: " << bearing.transpose();
```

### `IsPixelInImage`

Check if pixel coordinates are within image bounds

**Function Signature**:
```cpp
static bool IsPixelInImage(
    const Vector2d &pixel_coord,
    const CameraModel &camera_model);
```

**Usage Example**:
```cpp
Vector2d pixel(100, 200);
if (OpenGVConverter::IsPixelInImage(pixel, camera_model)) {
    // Pixel is within image, perform conversion
    auto bearing = OpenGVConverter::PixelToBearingVector(pixel, camera_model);
} else {
    LOG_WARNING_ZH << "Pixel coordinates out of image bounds";
}
```

---

## Complete Workflow Examples

### OpenGV-Based Relative Pose Estimation
(opengv-pose-estimation-workflow)=

```cpp
#include "converter_opengv.hpp"
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/relative_pose/methods.hpp>
#include <opengv/sac/Ransac.hpp>
#include <opengv/sac_problems/relative_pose/CentralRelativePoseSacProblem.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// ==== Step 1: Prepare data ====
types::IdMatches matches = GetMatches();
types::FeaturesInfo features_info = GetFeaturesInfo();
types::CameraModels camera_models = GetCameraModels();
types::ViewPair view_pair(0, 1);

// ==== Step 2: Convert to Bearing vectors ====
opengv::bearingVectors_t bearings1, bearings2;
OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);

LOG_INFO_ZH << "Converted " << bearings1.size() << " bearing vector pairs";

// ==== Step 3: Create OpenGV Adapter ====
opengv::relative_pose::CentralRelativeAdapter adapter(
    bearings1, bearings2);

// ==== Step 4: RANSAC pose estimation ====
typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem 
    SacProblem;

// Create RANSAC problem
opengv::sac::Ransac<SacProblem> ransac;
std::shared_ptr<SacProblem> problem(
    new SacProblem(adapter, SacProblem::NISTER));

ransac.sac_model_ = problem;
ransac.threshold_ = 1.0 - cos(atan(1.0/800.0));  // 1 pixel error threshold
ransac.max_iterations_ = 1000;

// Run RANSAC
bool ransac_success = ransac.computeModel();

if (ransac_success) {
    LOG_INFO_ZH << "RANSAC succeeded, inliers: " << ransac.inliers_.size();
    
    // ==== Step 5: Extract optimal model ====
    opengv::transformation_t T_ransac = ransac.model_coefficients_;
    
    // ==== Step 6: Nonlinear optimization (using all inliers) ====
    adapter.sett12(T_ransac.col(3));
    adapter.setR12(T_ransac.block<3,3>(0,0));
    
    opengv::transformation_t T_optimized = 
        opengv::relative_pose::optimize_nonlinear(adapter, ransac.inliers_);
    
    // ==== Step 7: Convert to PoSDK format ====
    types::RelativePose relative_pose;
    OpenGVConverter::OpenGVPose2RelativePose(T_optimized, relative_pose);
    
    // ==== Step 8: Store results ====
    relative_poses.AddPose(view_pair, relative_pose);
    
    LOG_INFO_ZH << "Relative pose estimation completed";
    LOG_INFO_ZH << "Rotation matrix:\n" << relative_pose.GetRotation();
    LOG_INFO_ZH << "Translation vector: " << relative_pose.GetTranslation().transpose();
    
} else {
    LOG_ERROR_ZH << "RANSAC failed, cannot estimate relative pose";
}
```

### 5-Point Algorithm + Triangulation
```cpp
// 1. 5-point algorithm estimates essential matrix
opengv::relative_pose::CentralRelativeAdapter adapter(bearings1, bearings2);
opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);

// 2. Convert to PoSDK format
types::RelativePose relative_pose;
OpenGVConverter::OpenGVPose2RelativePose(T, relative_pose);

// 3. Use PoSDK for triangulation
auto triangulator = CreateMethod<Triangulator>();
triangulator->SetRelativePose(relative_pose);
triangulator->SetBearingVectors(bearings1, bearings2);

types::Points3d points_3d = triangulator->Triangulate();

LOG_INFO_ZH << "Triangulated " << points_3d.size() << " 3D points";
```

---

## OpenGV Algorithm Support

### Relative Pose Algorithms
(opengv-relative-pose-algorithms)=

| Algorithm                  | OpenGV Function      | Min Points | Characteristics                 |
| -------------------------- | -------------------- | ---------- | ------------------------------- |
| **5-Point (Nister)**       | `fivept_nister`      | 5          | Standard algorithm, most common |
| **5-Point (Stewenius)**    | `fivept_stewenius`   | 5          | More numerically stable         |
| **7-Point**                | `sevenpt`            | 7          | For fundamental matrix          |
| **8-Point**                | `eightpt`            | 8          | Traditional algorithm           |
| **Nonlinear Optimization** | `optimize_nonlinear` | ≥5         | Refine pose                     |

### Absolute Pose Algorithms
(opengv-absolute-pose-algorithms)=

| Algorithm       | OpenGV Function | Min Points | Use Case            |
| --------------- | --------------- | ---------- | ------------------- |
| **P3P (Kneip)** | `p3p_kneip`     | 3          | Minimum 3 points    |
| **P3P (Gao)**   | `p3p_gao`       | 3          | Faster version      |
| **EPnP**        | `epnp`          | ≥4         | Non-iterative, fast |
| **UPnP**        | `upnp`          | ≥4         | Uncalibrated camera |

---

---

## Error Handling

### Invalid Camera Model
```cpp
const CameraModel* camera = camera_models[view_id];
if (!camera) {
    LOG_ERROR_ZH << "Invalid camera model, view_id=" << view_id;
    return false;
}
```

### Bearing Vector Validation
```cpp
for (const auto& bearing : bearings1) {
    if (std::abs(bearing.norm() - 1.0) > 1e-6) {
        LOG_WARNING_ZH << "Bearing vector not normalized: " << bearing.norm();
    }
}
```

### OpenGV Exception Handling
```cpp
try {
    opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);
} catch (const std::exception& e) {
    LOG_ERROR_ZH << "OpenGV algorithm exception: " << e.what();
    return false;
}
```

---

**Related Links**:
- [Core Data Types - RelativePose](../appendices/appendix_a_types.md#relative-pose)
- [Core Data Types - BearingVectors](../appendices/appendix_a_types.md#bearing-vectors)
- [Converter Overview](index.md)

**External Resources**:
- [OpenGV Official Documentation](https://laurentkneip.github.io/opengv/)
- [OpenGV GitHub](https://github.com/laurentkneip/opengv)



