# PoSDK Core Data Types

This document provides detailed information about various core data types used in PoSDK, covering fundamental types, camera models, features and matching, tracks and observations, pose representation, and other key concepts. These types form the fundamental data structures of the entire PoSDK system.


---

## Modular File Structure

PoSDK adopts a modular type system design for better code organization and maintenance.

### File Organization
(file-organization)=

```
types/
├── types.hpp              # Main entry file, contains complex global pose types
├── base.hpp/cpp           # Fundamental types and environment variable management
├── camera_model.hpp/cpp   # Camera model related types and implementation
├── features.hpp/cpp       # Feature detection and management related types
├── matches.hpp/cpp        # Matching relationships and bearing vector types
├── tracks.hpp/cpp         # Track observation and management related types
├── relative_poses.hpp/cpp # Relative pose and relative rotation types
├── global_poses.hpp/cpp   # Global pose fundamental types and estimation state
├── world_3dpoints.hpp/cpp # 3D point cloud and world coordinate point information
└── image.hpp              # Image related type definitions
```

### Module Responsibilities
(module-responsibilities)=

| Module           | Main Content                                                                                                                            |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| `base`           | [Fundamental Types](basic-identifier-types), [Method Configuration](method-config-types)                                                |
| `camera_model`   | [Camera Intrinsics](camera-intrinsics), [Distortion Model](distortion-type), [Camera Set Management](camera-models)                     |
| `features`       | [Feature Points](feature-point), [Feature Descriptors](basic-feature-types), [Image Feature Management](features-info)                  |
| `matches`        | [Feature Matching](id-match), [Bearing Vectors](bearing-vector-types), [Geometric Transform Utilities](bearing-vector-utils)            |
| `tracks`         | [Track Observations](obs-info), [3D Point Management](track), [Normalization Processing](tracks)                                        |
| `relative_poses` | [Relative Pose](relative-pose), [Relative Rotation](relative-rotation), [Accuracy Assessment](relative-poses)                           |
| `global_poses`   | [Global Pose Fundamental Types](global-rotation-translation-types), [Pose Format](pose-format), [Estimation State Management](est-info) |
| `world_3dpoints` | [3D Point Cloud](points3d), [World Coordinate Point Information](world-point-info), [Accuracy Assessment](world-point-info)             |
| `types.hpp`      | [Complex Global Pose Classes](global-poses), [Similarity Transform](similarity-transform-functions), Unified Management                 |


---

## Quick Navigation

### Fundamental Types Quick Navigation
(basic-types-navigation)=

- **Identifier Types**: [`IndexT`](index-t) | [`ViewId`](view-id) | [`PtsId`](pts-id) | [`Size`](size-type)
- **Mathematical Types**: [`Matrix3d`](matrix3d) | [`Vector3d`](vector3d) | [`Vector2d`](vector2d) | [`VectorXd`](vectorxd) | [`MatrixXd`](matrixxd)
- **Smart Pointers**: [`Ptr<T>`](ptr-type)
- **Method Configuration**:  [`MethodsConfig`](methods-config)
- **Data Package Types**: [`DataPtr`](ptr-type) | [`Package`](package) | [`DataPackagePtr`](data-package-ptr)

### Camera System Quick Navigation
(camera-system-navigation)=

- **Camera Models**: [`CameraModel`](camera-model) | [`CameraModels`](camera-models) | [`CameraIntrinsics`](camera-intrinsics)
- **Distortion Types**: [`DistortionType`](distortion-type) | [`NO_DISTORTION`](no-distortion) | [`RADIAL_K3`](radial-k3) | [`BROWN_CONRADY`](brown-conrady)
- **Projection Models**: [`CameraModelType`](camera-model-type) | [`PINHOLE`](pinhole) | [`FISHEYE`](fisheye) | [`SPHERICAL`](spherical)
- **Intrinsic Matrices**: [`KMats`](k-mats) | [`KMatsPtr`](k-mats-ptr)

### Image Management Quick Navigation
(image-system-navigation)=

- **Image Paths**: [`ImagePaths`](image-paths) | [`ImagePathsPtr`](image-paths-ptr)


### Feature System Quick Navigation
(feature-system-navigation)=

- **Feature Types**: [`Feature`](feature) | [`FeaturePoint`](feature-point) | [`Descriptor`](descriptor)
- **Image Features**: [`ImageFeatureInfo`](image-feature-info) | [`FeaturesInfo`](features-info) | [`FeaturesInfoPtr`](features-info-ptr)

### Matching System Quick Navigation
(matching-system-navigation)=

- **Matching Relationships**: [`IdMatch`](id-match) | [`IdMatches`](id-matches) | [`ViewPair`](view-pair) | [`Matches`](matches)
- **Bearing Vectors**: [`BearingVector`](bearing-vector) | [`BearingVectors`](bearing-vectors) | [`BearingPairs`](bearing-pairs)

### Track System Quick Navigation
(tracks-system-navigation)=

- **Observation Data**: [`ObsInfo`](obs-info) | [`Track`](track) | [`TrackInfo`](track-info)
- **Track Collections**: [`Tracks`](tracks) | [`TracksPtr`](tracks-ptr)

### Pose System Quick Navigation
(pose-system-navigation)=

- **Relative Poses**: [`RelativePose`](relative-pose) | [`RelativePoses`](relative-poses) | [`RelativeRotation`](relative-rotation)
- **Global Poses**: [`GlobalPoses`](global-poses) | [`GlobalRotations`](global-rotations) | [`GlobalTranslations`](global-translations)
- **Pose Formats**: [`PoseFormat`](pose-format) | [`RwTw`](rw-tw) | [`RwTc`](rw-tc) | [`RcTw`](rc-tw) | [`RcTc`](rc-tc)
- **Estimation Information**: [`EstInfo`](est-info) | [`ViewState`](view-state)

### 3D Point Cloud System Quick Navigation
(point-cloud-system-navigation)=

- **3D Point Types**: [`Point3d`](point3d) | [`Points3d`](points3d) | [`Points3dPtr`](points3d-ptr)
- **World Coordinates**: [`WorldPointInfo`](world-point-info) | [`WorldPointInfoPtr`](world-point-info-ptr)

### Mathematical Transform Quick Navigation
(math-transform-navigation)=

- **Similarity Transform**: [`SimilarityTransformError`](similarity-transform-error) | [`CayleyParams`](cayley-params) | [`Quaternion`](quaternion)
- **Transform Functions**: [Similarity Transform Functions](similarity-transform-functions) | [Mathematical Utility Functions](similarity-transform-math-utils)

---
## Fundamental Types (types/base.hpp)

### Fundamental Identifier Types
(ptr-type)=
(basic-identifier-types)=
(index-t)=
(view-id)=
(pts-id)=
(size-type)=

**Type Alias Definitions**:
| Type Alias | Actual Type          | Default Value | Purpose                                                                     |
| ---------- | -------------------- | ------------- | --------------------------------------------------------------------------- |
| `Ptr<T>`   | `std::shared_ptr<T>` | -             | Smart pointer type template, provides automatic memory management           |
| `IndexT`   | `std::uint32_t`      | 0             | Generic index type for various index identifiers (max value: 4,294,967,295) |
| `ViewId`   | `std::uint32_t`      | 0             | View identifier type for identifying camera views                           |
| `PtsId`    | `std::uint32_t`      | 0             | Point/track identifier type for identifying 3D points or feature tracks     |
| `Size`     | `std::uint32_t`      | 0             | Size type for representing sizes, counts, etc.                              |

### Eigen Mathematical Types
(eigen-math-types)=
(sp-matrix)=
(matrix3d)=
(vector3d)=
(vector2d)=
(vector2f)=
(vectorxd)=
(rowvectorxd)=
(matrixxd)=
(info-mat-ptr)=

**Type Alias Definitions**:
| Type Alias    | Actual Type                   | Default Value | Purpose                                                                |
| ------------- | ----------------------------- | ------------- | ---------------------------------------------------------------------- |
| `SpMatrix`    | `Eigen::SparseMatrix<double>` | -             | Sparse matrix type for large sparse linear systems                     |
| `Matrix3d`    | `Eigen::Matrix3d`             | Zero matrix   | 3×3 double precision matrix for rotation matrices and other transforms |
| `Vector3d`    | `Eigen::Vector3d`             | Zero vector   | 3D double precision vector for 3D coordinates and vectors              |
| `Vector2d`    | `Eigen::Vector2d`             | Zero vector   | 2D double precision vector for image coordinates and planar vectors    |
| `Vector2f`    | `Eigen::Vector2f`             | Zero vector   | 2D single precision vector for memory-sensitive 2D operations          |
| `VectorXd`    | `Eigen::VectorXd`             | Zero vector   | Dynamic size double precision vector for variable length data          |
| `RowVectorXd` | `Eigen::RowVectorXd`          | Zero vector   | Dynamic size row vector for matrix operations                          |
| `MatrixXd`    | `Eigen::MatrixXd`             | Zero matrix   | Dynamic size matrix for arbitrary size matrix operations               |


### Method Configuration Types
(method-config-types)=
(method-type)=
(method-params)=
(params-value)=
(method-options)=
(methods-config)=

**Type Alias Definitions**:
| Type Alias      | Actual Type                                     | Default Value | Purpose                                                       |
| --------------- | ----------------------------------------------- | ------------- | ------------------------------------------------------------- |
| `MethodType`    | `std::string`                                   | ""            | Method type name, such as "SIFT", "ORB", etc.                 |
| `MethodParams`  | `std::string`                                   | ""            | Method parameter name, such as "nfeatures", "threshold", etc. |
| `ParamsValue`   | `std::string`                                   | ""            | Parameter value type, stored as string                        |
| `MethodOptions` | `std::unordered_map<MethodParams, ParamsValue>` | Empty map     | All parameter configuration for a single method               |
| `MethodsConfig` | `std::unordered_map<MethodType, MethodOptions>` | Empty map     | Configuration collection for all methods                      |

### Data Package Types
(data-package-types)=
(data-type)=
(package)=
(data-package-ptr)=

**Type Alias Definitions**:
| Type Alias       | Actual Type                             | Default Value | Purpose                                                   |
| ---------------- | --------------------------------------- | ------------- | --------------------------------------------------------- |
| `DataType`       | `std::string`                           | ""            | Data type identifier for key names in data packages       |
| `DataPtr`        | `std::shared_ptr<DataIO>`               | nullptr       | Data pointer type, smart pointer to data IO object        |
| `Package`        | `std::unordered_map<DataType, DataPtr>` | Empty map     | Basic data package type, key-value pair mapping container |
| `DataPackagePtr` | `std::shared_ptr<DataPackage>`          | nullptr       | Data package smart pointer type                           |


---

## Camera Model (types/camera_model.hpp)

The camera model system provides complete camera parameter management, supporting intelligent access for single and multi-camera configurations.

### Core Features and Usage Patterns

**PIMPL Design Pattern**:
- **Binary Compatibility**: Uses PIMPL (Pointer to Implementation) pattern to hide implementation details, providing stable ABI
- **Extensibility**: Easy to add new camera models and calibration methods without affecting client code
- **Data Encapsulation**: Private member variables hidden through std::unique_ptr<Impl>, improving data security
- **Memory Alignment**: Uses EIGEN_MAKE_ALIGNED_OPERATOR_NEW macro to ensure correct alignment of Eigen types

**Intelligent Access Mechanism**:
- **Single Camera Mode**: All views share the same intrinsics, any `view_id` returns the same model
- **Multi-Camera Mode**: Each view corresponds to independent intrinsics, accessed by `view_id` index
- **Automatic Recognition**: System automatically selects access mode based on number of models

### Typical Usage Examples

#### Basic Camera Model Creation and Configuration
```cpp
using namespace PoSDK::types;

// Create camera model collection
CameraModels cameras;

// Single camera configuration (recommended for calibration scenarios)
CameraModel single_camera;
single_camera.SetCameraIntrinsics(800.0, 800.0, 320.0, 240.0, 640, 480);
cameras.push_back(single_camera);
```

#### Intelligent Access to Camera Intrinsics (Recommended Approach)
```cpp
ViewId view_id = 1;

// Intelligent index access (returns nullptr on failure)
const CameraModel* camera = cameras[view_id];  // Recommended primary access method
if (camera) {
    // Get intrinsic matrix (efficient version)
    Matrix3d K;
    camera->GetKMat(K);

    // Get intrinsic matrix (convenient version)
    Matrix3d K_conv = camera->GetKMat();

    // Access parameters through accessor methods (recommended approach)
    const auto& intrinsics = camera->GetIntrinsics();
    double fx = intrinsics.GetFx();
    double cy = intrinsics.GetCy();
    uint32_t width = intrinsics.GetWidth();
    uint32_t height = intrinsics.GetHeight();
} else {
    // Handle access failure
}
```

#### Multi-Camera Configuration Example
```cpp
// Multi-camera mode: each view has independent intrinsics
CameraModels multi_cameras;
for (ViewId i = 0; i < 3; ++i) {
    CameraModel camera;
    // Use method 4: complete parameter setting including image size
    camera.SetCameraIntrinsics(800.0 + i*50, 800.0, 320.0, 240.0, 640, 480);
    multi_cameras.push_back(camera);
}

// Access specific camera: returns pointer, need to check nullptr
const CameraModel* cam2 = multi_cameras[2];
if (cam2) {
    // Use parameters from the 3rd camera
    Matrix3d K = cam2->GetKMat();
}
```

#### Coordinate Conversion and Distortion Handling
```cpp
const CameraModel* camera = cameras[view_id];
if (camera) {
    // Pixel coordinate and normalized coordinate conversion
    Vector2d pixel(100.0, 200.0);
    Vector2d normalized = camera->PixelToNormalized(pixel);
    Vector2d back_to_pixel = camera->NormalizedToPixel(normalized);

    // Configure distortion parameters
    std::vector<double> radial_dist = {-0.1, 0.05, -0.01};  // k1, k2, k3
    std::vector<double> tangential_dist = {};  // No tangential distortion
    camera->SetDistortionParams(DistortionType::RADIAL_K3, radial_dist, tangential_dist);

    // Set camera metadata information
    camera->SetCameraInfo("Canon", "EOS 5D", "123456789");
}
```

### `DistortionType`
(distortion-type)=
(no-distortion)=
(radial-k1)=
(radial-k3)=
(brown-conrady)=
Camera distortion type enumeration supporting multiple distortion models

**Enumeration Definitions**:
| Type            | Definition                                      | Default Value | Purpose                                                            |
| --------------- | ----------------------------------------------- | ------------- | ------------------------------------------------------------------ |
| `NO_DISTORTION` | No distortion enumeration value                 | -             | Ideal pinhole camera model, no lens distortion                     |
| `RADIAL_K1`     | First-order radial distortion enumeration value | -             | Simple radial distortion model, suitable for slight distortion     |
| `RADIAL_K3`     | Third-order radial distortion enumeration value | -             | Complete radial distortion model, suitable for moderate distortion |
| `BROWN_CONRADY` | Brown-Conrady distortion enumeration value      | -             | Complete distortion model including tangential distortion          |

### `CameraModelType`
(camera-model-type)=
(pinhole)=
(fisheye)=
(spherical)=
(omnidirectional)=
Camera model type enumeration defining different camera projection models

**Enumeration Definitions**:
| Type              | Definition                                     | Default Value | Purpose                                                            |
| ----------------- | ---------------------------------------------- | ------------- | ------------------------------------------------------------------ |
| `PINHOLE`         | Pinhole camera model enumeration value         | -             | Standard projection camera model, suitable for conventional lenses |
| `FISHEYE`         | Fisheye camera model enumeration value         | -             | Wide-angle lens model, suitable for large field-of-view cameras    |
| `SPHERICAL`       | Spherical camera model enumeration value       | -             | Spherical projection model, suitable for panoramic cameras         |
| `OMNIDIRECTIONAL` | Omnidirectional camera model enumeration value | -             | Full-range projection model, suitable for 360-degree cameras       |

### `CameraIntrinsics`
(camera-intrinsics)=
Camera intrinsic data structure (encapsulated class design, private member variables + public access interface)

**Implementation Details**:
- **PIMPL Pattern**: Uses `std::unique_ptr<Impl>` to hide implementation details, providing stable ABI
- **Memory Alignment**: Uses `alignas(8)` to optimize memory layout, double types prioritized for alignment
- **Default Construction**: Focal length fx/fy defaults to 1.0, principal point cx/cy defaults to 0.0, image size defaults to 0

**Internal Data Structure** (hidden through PIMPL):
| Name                   | Type                                   | Default Value | Purpose                                       |
| ---------------------- | -------------------------------------- | ------------- | --------------------------------------------- |
| fx_                    | `double`                               | 1.0           | x-direction focal length parameter            |
| fy_                    | `double`                               | 1.0           | y-direction focal length parameter            |
| cx_                    | `double`                               | 0.0           | x-direction principal point offset coordinate |
| cy_                    | `double`                               | 0.0           | y-direction principal point offset coordinate |
| width_                 | `uint32_t`                             | 0             | Image resolution width                        |
| height_                | `uint32_t`                             | 0             | Image resolution height                       |
| model_type_            | [`CameraModelType`](camera-model-type) | PINHOLE       | Camera model type identifier                  |
| distortion_type_       | [`DistortionType`](distortion-type)    | NO_DISTORTION | Camera distortion type identifier             |
| radial_distortion_     | `std::vector<double>`                  | Empty vector  | Radial distortion coefficient array           |
| tangential_distortion_ | `std::vector<double>`                  | Empty vector  | Tangential distortion coefficient array       |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">CameraIntrinsics</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">GetFx</span> - Get x-direction focal length
- <span style="color:#1976d2;font-weight:bold;">GetFy</span> - Get y-direction focal length
- <span style="color:#1976d2;font-weight:bold;">GetCx</span> - Get x-direction principal point offset
- <span style="color:#1976d2;font-weight:bold;">GetCy</span> - Get y-direction principal point offset
- <span style="color:#1976d2;font-weight:bold;">GetWidth</span> - Get image width
- <span style="color:#1976d2;font-weight:bold;">GetHeight</span> - Get image height
- <span style="color:#1976d2;font-weight:bold;">GetModelType</span> - Get camera model type
- <span style="color:#1976d2;font-weight:bold;">GetDistortionType</span> - Get distortion type
- <span style="color:#1976d2;font-weight:bold;">GetRadialDistortion</span> - Get radial distortion parameters
- <span style="color:#1976d2;font-weight:bold;">GetTangentialDistortion</span> - Get tangential distortion parameters
- <span style="color:#1976d2;font-weight:bold;">SetFx</span> - Set x-direction focal length
  - fx `double`: Focal length value
- <span style="color:#1976d2;font-weight:bold;">SetFy</span> - Set y-direction focal length
  - fy `double`: Focal length value
- <span style="color:#1976d2;font-weight:bold;">SetCx</span> - Set x-direction principal point offset
  - cx `double`: Principal point offset value
- <span style="color:#1976d2;font-weight:bold;">SetCy</span> - Set y-direction principal point offset
  - cy `double`: Principal point offset value
- <span style="color:#1976d2;font-weight:bold;">SetWidth</span> - Set image width
  - width `uint32_t`: Image width
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetHeight</span> - Set image height
  - height `uint32_t`: Image height
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetImageSize</span> - Batch set image size
  - width `uint32_t`: Image width
  - height `uint32_t`: Image height
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetModelType</span> - Set camera model type
  - type `CameraModelType`: Camera model type
- <span style="color:#1976d2;font-weight:bold;">SetDistortionType</span> - Set distortion type
  - type `DistortionType`: Distortion type
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - Get camera intrinsic matrix (efficient version)
  - K `Matrix3d &`: Output 3x3 intrinsic matrix reference
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - Get camera intrinsic matrix (convenient version)
  - Returns `Matrix3d`: 3x3 intrinsic matrix
- <span style="color:#1976d2;font-weight:bold;">SetKMat</span> - Set camera intrinsic matrix
  - K `const Matrix3d &`: Input 3x3 intrinsic matrix constant reference
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - Set camera intrinsic data (method 1)
  - intrinsics `const CameraIntrinsics &`: Camera intrinsic structure
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - Set camera intrinsic data (method 2)
  - fx `const double`: x-direction focal length
  - fy `const double`: y-direction focal length
  - cx `const double`: x-direction principal point offset
  - cy `const double`: y-direction principal point offset
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - Set camera intrinsic data (method 3)
  - fx `const double`: x-direction focal length
  - fy `const double`: y-direction focal length
  - width `const uint32_t`: Image width
  - height `const uint32_t`: Image height
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - Set camera intrinsic data (method 4)
  - fx `const double`: x-direction focal length
  - fy `const double`: y-direction focal length
  - cx `const double`: x-direction principal point offset
  - cy `const double`: y-direction principal point offset
  - width `const uint32_t`: Image width
  - height `const uint32_t`: Image height
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - Set camera intrinsic data (method 5, complete parameters)
  - fx `const double`: x-direction focal length
  - fy `const double`: y-direction focal length
  - cx `const double`: x-direction principal point offset
  - cy `const double`: y-direction principal point offset
  - width `const uint32_t`: Image width
  - height `const uint32_t`: Image height
  - radial_distortion `const std::vector<double> &`: Radial distortion parameters
  - tangential_distortion `const std::vector<double> &`: Tangential distortion parameters (optional, default empty)
  - model_type `const CameraModelType`: Camera model type (default [pinhole camera](pinhole))
- <span style="color:#1976d2;font-weight:bold;">SetDistortionParams</span> - Set distortion parameters
  - distortion_type `DistortionType`: Distortion type
  - radial_distortion `const std::vector<double> &`: Radial distortion parameters
  - tangential_distortion `const std::vector<double> &`: Tangential distortion parameters
- <span style="color:#1976d2;font-weight:bold;">InitDistortionParams</span> - Initialize distortion parameters to zero
- <span style="color:#1976d2;font-weight:bold;">GetFocalLengthPtr</span> - Get focal length parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">GetPrincipalPointPtr</span> - Get principal point parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">GetDistortionPtr</span> - Get distortion parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">CopyToParamArray</span> - Copy parameters to array (for Ceres optimization)
  - params `double*`: Output parameter array
  - include_distortion `bool`: Whether to include distortion parameters (default true)
- <span style="color:#1976d2;font-weight:bold;">SetFromParamArray</span> - Set parameters from array (for Ceres optimization)
  - params `const double*`: Input parameter array
  - include_distortion `bool`: Whether to include distortion parameters (default true)

### `CameraModel`
(camera-model)=
Complete camera model definition including intrinsics and metadata information (encapsulated class design, private member variables + public access interface)

**Implementation Details**:
- **PIMPL Pattern**: Uses `std::unique_ptr<Impl>` to hide implementation details, providing stable ABI
- **Memory Alignment**: Uses `EIGEN_MAKE_ALIGNED_OPERATOR_NEW` macro to ensure correct alignment of Eigen types

**Internal Data Structure** (hidden through PIMPL):
| Name             | Type                                    | Default Value | Purpose                         |
| ---------------- | --------------------------------------- | ------------- | ------------------------------- |
| `intrinsics_`    | [`CameraIntrinsics`](camera-intrinsics) | Default value | Camera intrinsic data structure |
| `camera_make_`   | `std::string`                           | ""            | Camera manufacturer name        |
| `camera_model_`  | `std::string`                           | ""            | Camera model identifier         |
| `serial_number_` | `std::string`                           | ""            | Camera serial number            |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">GetIntrinsics</span> - Get camera intrinsics reference (const version)
  - Returns `const CameraIntrinsics&`: Intrinsics constant reference
- <span style="color:#1976d2;font-weight:bold;">GetIntrinsics</span> - Get camera intrinsics reference (non-const version)
  - Returns `CameraIntrinsics&`: Intrinsics reference
- <span style="color:#1976d2;font-weight:bold;">GetCameraMake</span> - Get camera manufacturer
  - Returns `const std::string&`: Manufacturer name
- <span style="color:#1976d2;font-weight:bold;">GetCameraModel</span> - Get camera model name
  - Returns `const std::string&`: Camera model name (e.g., "EOS 5D Mark IV")
- <span style="color:#1976d2;font-weight:bold;">GetSerialNumber</span> - Get serial number
  - Returns `const std::string&`: Serial number
- <span style="color:#1976d2;font-weight:bold;">SetCameraMake</span> - Set camera manufacturer
  - make `const std::string&`: Manufacturer name
- <span style="color:#1976d2;font-weight:bold;">SetCameraModelName</span> - Set camera model name
  - model `const std::string&`: Camera model name
- <span style="color:#1976d2;font-weight:bold;">SetSerialNumber</span> - Set serial number
  - serial `const std::string&`: Serial number
- <span style="color:#1976d2;font-weight:bold;">PixelToNormalized</span> - Pixel coordinate to normalized coordinate (single point conversion)
  - pixel_coord `const Vector2d &`: Pixel coordinate
  - Returns `Vector2d`: Normalized coordinate
- <span style="color:#1976d2;font-weight:bold;">NormalizedToPixel</span> - Normalized coordinate to pixel coordinate (single point conversion)
  - normalized_coord `const Vector2d &`: Normalized coordinate
  - Returns `Vector2d`: Pixel coordinate
- <span style="color:#1976d2;font-weight:bold;">PixelToNormalizedBatch</span> - Batch pixel coordinate to normalized coordinate (vectorized)
  - pixel_coords `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`: 2×N pixel coordinate matrix
  - Returns `Eigen::Matrix<double, 2, Eigen::Dynamic>`: 2×N normalized coordinate matrix
  - Note: SIMD-friendly batch operation, better performance than loop calling single point conversion
- <span style="color:#1976d2;font-weight:bold;">NormalizedToPixelBatch</span> - Batch normalized coordinate to pixel coordinate (vectorized)
  - normalized_coords `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`: 2×N normalized coordinate matrix
  - Returns `Eigen::Matrix<double, 2, Eigen::Dynamic>`: 2×N pixel coordinate matrix
  - Note: SIMD-friendly batch operation, better performance than loop calling single point conversion
- <span style="color:#1976d2;font-weight:bold;">SetKMat</span> - Set camera intrinsic matrix
  - K `const Matrix3d &`: Input 3x3 intrinsic matrix
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - Get camera intrinsic matrix (efficient version)
  - K `Matrix3d &`: Output 3x3 intrinsic matrix reference
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - Get camera intrinsic matrix (convenient version)
  - Returns `Matrix3d`: 3x3 intrinsic matrix
- <span style="color:#1976d2;font-weight:bold;">SetDistortionParams</span> - Set distortion type and parameters
  - distortion_type `const DistortionType &`: Distortion type
  - radial_distortion `const std::vector<double> &`: Radial distortion parameters
  - tangential_distortion `const std::vector<double> &`: Tangential distortion parameters
- <span style="color:#1976d2;font-weight:bold;">SetModelType</span> - Set camera model type
  - model_type `const CameraModelType &`: Camera model type
- <span style="color:#1976d2;font-weight:bold;">SetCameraInfo</span> - Set camera metadata information
  - camera_make `const std::string &`: Camera manufacturer
  - camera_model `const std::string &`: Camera model
  - serial_number `const std::string &`: Serial number
- Other SetCameraIntrinsics methods - Multiple overloaded methods same as [`CameraIntrinsics`](camera-intrinsics) class
- <span style="color:#1976d2;font-weight:bold;">GetFocalLengthPtr</span> - Get focal length parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">GetPrincipalPointPtr</span> - Get principal point parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">GetDistortionPtr</span> - Get distortion parameter pointer (for Ceres optimization)
- <span style="color:#1976d2;font-weight:bold;">CopyToParamArray</span> - Copy parameters to array (for Ceres optimization)
  - params `double*`: Output parameter array
  - include_distortion `bool`: Whether to include distortion parameters (default true)
- <span style="color:#1976d2;font-weight:bold;">SetFromParamArray</span> - Set parameters from array (for Ceres optimization)
  - params `const double*`: Input parameter array
  - include_distortion `bool`: Whether to include distortion parameters (default true)

### `CameraModels`
(camera-models)=
Intelligent camera model container class supporting single and multi-camera configurations, inherits from std::vector<[`CameraModel`](camera-model)>

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Intelligent index access to camera model (non-const version)
  - view_id `ViewId`: View identifier
  - Returns `CameraModel*`: Camera model pointer, returns nullptr on access failure
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Intelligent index access to camera model (const version)
  - view_id `ViewId`: View identifier
  - Returns `const CameraModel*`: Camera model constant pointer, returns nullptr on access failure
- <span style="color:#1976d2;font-weight:bold;">at</span> - Intelligent index access to camera model (non-const version, with bounds checking)
  - view_id `ViewId`: View identifier
  - Returns `CameraModel&`: Camera model reference
  - Throws exception: `std::out_of_range` (when access is invalid)
- <span style="color:#1976d2;font-weight:bold;">at</span> - Intelligent index access to camera model (const version, with bounds checking)
  - view_id `ViewId`: View identifier
  - Returns `const CameraModel&`: Camera model constant reference
  - Throws exception: `std::out_of_range` (when access is invalid)
- All standard container methods inherited from `std::vector<CameraModel>` (`size`, `empty`, `push_back`, `clear`, etc.)


### Camera Model Related Type Definitions
(camera-model-related-types)=
(camera-model-ptr)=
(camera-models-ptr)=
(k-mats)=
(k-mats-ptr)=

**Type Definitions**:
| Type              | Definition                                                                                  | Default Value | Purpose                                                     |
| ----------------- | ------------------------------------------------------------------------------------------- | ------------- | ----------------------------------------------------------- |
| `CameraModelPtr`  | `std::shared_ptr<`[`CameraModel`](camera-model)`>`                                          | nullptr       | Camera model smart pointer type                             |
| `CameraModelsPtr` | `std::shared_ptr<`[`CameraModels`](camera-models)`>`                                        | nullptr       | Camera model collection smart pointer type                  |
| `KMats`           | `std::vector<`[`Matrix3d`](matrix3d)`, Eigen::aligned_allocator<`[`Matrix3d`](matrix3d)`>>` | Empty vector  | Intrinsic matrix sequence container using aligned allocator |
| `KMatsPtr`        | `std::shared_ptr<`[`KMats`](k-mats)`>`                                                      | nullptr       | Intrinsic matrix sequence smart pointer                     |

---

## Image Management (types/image.hpp)

### Image Path Type Definitions
(image-path-types)=
(image-paths)=
(image-paths-ptr)=

**Type Definitions**:
| Type            | Definition                                       | Default Value | Purpose                                                           |
| --------------- | ------------------------------------------------ | ------------- | ----------------------------------------------------------------- |
| `ImagePaths`    | `std::vector<std::pair<std::string, bool>>`      | Empty vector  | Type definition for storing image paths and their validity status |
| `ImagePathsPtr` | `std::shared_ptr<`[`ImagePaths`](image-paths)`>` | nullptr       | Smart pointer type for image path collection                      |

---

## Features (types/features.hpp)

The feature system adopts SIMD-optimized SOA (Structure of Arrays) design, providing high-performance feature data management.

### Core Features and Usage Patterns

**SIMD-Optimized SOA Design Advantages**:
- **SOA Memory Layout**: Data stored grouped by type, improving cache locality and SIMD vectorization efficiency
- **AVX2/AVX-512 Support**: Batch operations fully vectorized, significantly improving performance
- **Natural Alignment Optimization**: Removes excessive 64-byte alignment, reducing 90%+ memory usage
- **Batch-First API**: Provides efficient batch operation interfaces (SetSizesBlock, SetAnglesBlock, etc.)
- **Zero-Copy Compatibility**: Zero-copy compatible with cv::Mat and protobuf, 8-9x faster conversion
- **Compact Layout**: Single continuous memory allocation, reducing memory fragmentation

### Typical Usage Examples

#### FeaturePoints Batch Operations (High Performance)
```cpp
using namespace PoSDK::types;

// Create feature point collection (recommended to reserve capacity)
FeaturePoints features(1000);  // Reserve 1000 feature points

// Batch add features (efficient)
Eigen::Matrix<double, 2, Eigen::Dynamic> coords_block(2, 100);
std::vector<float> sizes_block(100, 5.0f);
std::vector<float> angles_block(100, 0.0f);
// ... Fill coords_block, sizes_block, angles_block ...
features.AddFeaturesBlock(coords_block, sizes_block.data(), angles_block.data());

// Access individual feature point
Feature coord = features.GetCoord(0);
float size = features.GetSize(0);
float angle = features.GetAngle(0);

// Batch set attributes (SIMD optimized)
features.SetSizesBlock(0, 100, sizes_block.data());
features.SetAnglesBlock(0, 100, angles_block.data());

// Batch statistical operations (SIMD optimized)
features.IncrementTotalCountsBlock(0, 100);
features.IncrementOutlierCountsBlock(0, 50);

// Zero counters (SIMD optimized)
features.ZeroCounters();

// Get valid feature count (SIMD optimized)
Size valid_count = features.GetNumValid();

// Iterator access
for (auto&& feat_proxy : features) {
    if (feat_proxy.IsUsed()) {
        Feature coord = feat_proxy.GetCoord();
        // Process feature
    }
}
```

#### Descriptor Management (SOA Format)
```cpp
// Create SOA descriptor container (high performance)
DescriptorsSOA descs_soa(1000, 128);  // 1000 features, 128-dimensional SIFT descriptor

// Set single descriptor
float descriptor_data[128];
// ... Fill descriptor_data ...
descs_soa.SetDescriptor(0, descriptor_data);

// Direct access to descriptor data (zero-copy)
float* desc_ptr = descs_soa[0];  // Pointer to the 0th descriptor

// Batch copy (SIMD optimized)
float* all_descs = descs_soa.data();  // Raw data pointer
size_t total_size = descs_soa.total_size();  // Total number of elements

// Convert from old AOS format (automatic)
Descs old_format_descs;  // std::vector<std::vector<float>>
descs_soa.FromDescs(old_format_descs);

// Convert back to AOS format (compatibility)
Descs converted_descs;
descs_soa.ToDescs(converted_descs);
```

#### Image Feature Information Management
```cpp
// Create image feature information
ImageFeatureInfo image_features("image1.jpg", true);

// Reserve capacity (recommended)
image_features.ReserveFeatures(1000);

// Add single feature point
image_features.AddFeature(100.0f, 200.0f, 5.0f, 45.0f);

// Batch add features (efficient)
Eigen::Matrix<double, 2, Eigen::Dynamic> coords_block(2, 100);
std::vector<float> sizes_block(100, 5.0f);
std::vector<float> angles_block(100, 0.0f);
// ... Fill data ...
image_features.AddFeaturesBlock(coords_block, sizes_block.data(), angles_block.data());

// Access feature point collection (direct access to underlying FeaturePoints)
FeaturePoints& features = image_features.GetFeaturePoints();
Size num_features = features.size();

// Access single feature through proxy (compatibility method)
auto feat_proxy = image_features.GetFeature(0);
Feature coord = feat_proxy.GetCoord();
float size = feat_proxy.GetSize();

// Zero feature counters (SIMD optimized)
image_features.ZeroFeatureCounters();

// Get valid feature count
Size valid_features = image_features.GetNumValidFeatures();

// Clear unused features
image_features.ClearUnusedFeatures();

// Descriptor management
auto descs_soa = std::make_shared<DescriptorsSOA>(1000, 128);
image_features.SetDescriptorsSOA(descs_soa);  // Set SOA format descriptor

if (image_features.HasDescriptorsSOA()) {
    auto& desc_ref = image_features.GetDescriptorsSOAPtr();
    // Use descriptor...
}

// Check descriptor reference count (memory debugging)
long soa_ref_count = image_features.GetDescriptorsSOARefCount();
```

#### Feature Collection Management
```cpp
// Create feature information collection
FeaturesInfo features_info;

// Add image features
features_info.AddImageFeatures("image1.jpg", true);
features_info.AddImageFeatures("image2.jpg", true);

// Intelligent access to image features
ImageFeatureInfo* img_feat = features_info[0];  // Returns pointer, need to check nullptr
if (img_feat) {
    img_feat->AddFeature(FeaturePoint(10.0f, 20.0f));
    std::string path = img_feat->GetImagePath();
}

// Safe access to image features
const ImageFeatureInfo* safe_img_feat = features_info.GetImageFeaturePtr(1);
if (safe_img_feat) {
    Size num_features = safe_img_feat->GetNumFeatures();
}

// Management operations
Size valid_images = features_info.GetNumValidImages();
features_info.ClearUnusedImages();
features_info.ClearAllUnusedFeatures();
```

### Fundamental Feature Types
(basic-feature-types)=
(feature)=
(descriptor)=
(descs)=
(descriptorssoa)=
(descs-soa)=
(feature-points)=
(feature-points-ptr)=

**Type Definitions**:
| Type               | Definition                                  | Default Value | Purpose                                                              |
| ------------------ | ------------------------------------------- | ------------- | -------------------------------------------------------------------- |
| `Feature`          | `Eigen::Matrix<double, 2, 1>`               | Zero vector   | Basic 2D feature point, contains only position information           |
| `Descriptor`       | `std::vector<float>`                        | Empty vector  | Legacy AOS format descriptor (backward compatible)                   |
| `Descs`            | `std::vector<`[`Descriptor`](descriptor)`>` | Empty vector  | Legacy AOS format descriptor collection (backward compatible)        |
| `DescriptorsSOA`   | SOA format descriptor container class       | -             | High-performance SOA format descriptor, SIMD optimized               |
| `DescsSOA`         | `DescriptorsSOA`                            | -             | Type alias for DescriptorsSOA                                        |
| `DescsSOAPtr`      | `Ptr<DescriptorsSOA>`                       | nullptr       | Smart pointer for DescriptorsSOA                                     |
| `FeaturePoints`    | SOA format feature point container class    | -             | High-performance SOA format feature point collection, SIMD optimized |
| `FeaturePointsPtr` | `Ptr<FeaturePoints>`                        | nullptr       | Smart pointer for FeaturePoints                                      |

### `DescriptorsSOA` (Recommended)
(descriptorssoa-class)=
SIMD-optimized descriptor container using SOA (Structure of Arrays) format, providing zero-copy compatibility and 8-9x faster CV↔PoSDK conversion

**Key Features**:
- **Single Memory Allocation**: All descriptors in one contiguous memory block (vs 10K scattered allocations)
- **SIMD-Friendly**: Supports AVX2/AVX-512 batch operations
- **Zero-Copy Compatibility**: Seamless integration with cv::Mat and protobuf
- **Compact Layout**: num_features × descriptor_dim × sizeof(float)

**Memory Layout**:
```
[desc0[0], desc0[1], ..., desc0[dim-1], desc1[0], desc1[1], ..., desc1[dim-1], ...]
```

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">DescriptorsSOA</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">DescriptorsSOA</span> - Parameterized constructor
  - num_features `size_t`: Number of features
  - descriptor_dim `size_t`: Descriptor dimension
- <span style="color:#1976d2;font-weight:bold;">resize</span> - Resize
  - num_features `size_t`: Number of features
  - descriptor_dim `size_t`: Descriptor dimension
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - Reserve capacity
  - num_features `size_t`: Number of features
  - descriptor_dim `size_t`: Descriptor dimension
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">clear</span> - Clear data
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get number of features
  - Returns `size_t`: Number of features
- <span style="color:#1976d2;font-weight:bold;">dim</span> - Get descriptor dimension
  - Returns `size_t`: Descriptor dimension
- <span style="color:#1976d2;font-weight:bold;">total_size</span> - Get total data size
  - Returns `size_t`: Total data size (num_features × descriptor_dim)
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
  - Returns `bool`: Whether empty
- <span style="color:#1976d2;font-weight:bold;">data</span> - Get raw data pointer (batch operations)
  - Returns `float*` / `const float*`: Raw data pointer
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Get descriptor pointer
  - feature_idx `size_t`: Feature index
  - Returns `float*` / `const float*`: Pointer to descriptor_dim floats
- <span style="color:#1976d2;font-weight:bold;">SetDescriptor</span> - Set single descriptor (SIMD optimized)
  - feature_idx `size_t`: Feature index
  - descriptor_data `const float*`: Descriptor data pointer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptor</span> - Get single descriptor (SIMD optimized)
  - feature_idx `size_t`: Feature index
  - descriptor_out `float*`: Output buffer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">FromDescs</span> - Convert from legacy AOS format
  - descs `const Descs&`: Legacy descriptor vector
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ToDescs</span> - Convert to legacy AOS format
  - descs `Descs&`: Output legacy descriptor vector
  - Returns `void`

### `FeaturePoints` (Recommended)
(featurepoints-class)=
SIMD-optimized SOA feature point container with natural alignment, providing batch-first API

**Key Features**:
- **SOA Memory Layout**: Coordinates, sizes, and angles stored separately, SIMD-friendly
- **Natural Alignment**: Reduces 90%+ memory usage, improves cache efficiency
- **Batch Operations**: Fully vectorized batch set/update operations (AVX2/AVX-512)
- **Iterator Support**: Provides STL-style iterators and range-based for loops

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">FeaturePoints</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">FeaturePoints</span> - Constructor with reserved capacity
  - reserve_size `Size`: Reserved size
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - Reserve capacity
  - capacity `Size`: Reserved size
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">resize</span> - Resize
  - new_size `Size`: New size
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">clear</span> - Clear data
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get number of features
  - Returns `Size`: Number of features
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
  - Returns `bool`: Whether empty
- <span style="color:#1976d2;font-weight:bold;">GetSize</span> - Get single feature size
  - idx `Size`: Feature index
  - Returns `float&` / `const float&`: Feature size reference
- <span style="color:#1976d2;font-weight:bold;">GetAngle</span> - Get single feature angle
  - idx `Size`: Feature index
  - Returns `float&` / `const float&`: Feature angle reference
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - Get single feature usage status
  - idx `Size`: Feature index
  - Returns `uint8_t&` / `const uint8_t&`: Usage status reference
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - Get single feature coordinate
  - idx `Size`: Feature index
  - Returns `Feature`: Feature coordinate
- <span style="color:#1976d2;font-weight:bold;">SetCoord</span> - Set single feature coordinate
  - idx `Size`: Feature index
  - coord `const Feature&`: Coordinate
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetCoordsRef</span> - Get coordinate matrix reference (batch operations)
  - Returns `Eigen::Matrix<double, 2, Eigen::Dynamic>&`
- <span style="color:#1976d2;font-weight:bold;">GetSizesRef</span> - Get size vector reference (batch operations)
  - Returns `std::vector<float>&`
- <span style="color:#1976d2;font-weight:bold;">GetAnglesRef</span> - Get angle vector reference (batch operations)
  - Returns `std::vector<float>&`
- <span style="color:#1976d2;font-weight:bold;">GetIsUsedRef</span> - Get usage flag vector reference (batch operations)
  - Returns `std::vector<uint8_t>&`
- <span style="color:#1976d2;font-weight:bold;">GetTotalCountsRef</span> - Get total count vector reference (batch operations)
  - Returns `std::vector<IndexT>&`
- <span style="color:#1976d2;font-weight:bold;">GetOutlierCountsRef</span> - Get outlier count vector reference (batch operations)
  - Returns `std::vector<IndexT>&`
- <span style="color:#1976d2;font-weight:bold;">SetCoordsBlock</span> - Batch set coordinates (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`: Coordinate block
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetSizesBlock</span> - Batch set sizes (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - sizes_block `const float*`: Size data pointer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetAnglesBlock</span> - Batch set angles (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - angles_block `const float*`: Angle data pointer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetIsUsedBlock</span> - Batch set usage flags (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - flags `const uint8_t*`: Flag data pointer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">IncrementTotalCountsBlock</span> - Batch increment total counts (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">IncrementOutlierCountsBlock</span> - Batch increment outlier counts (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetNumValid</span> - Get valid feature count (SIMD optimized)
  - Returns `Size`: Valid feature count
- <span style="color:#1976d2;font-weight:bold;">GetOutlierRatiosBlock</span> - Batch get outlier ratios (SIMD optimized)
  - start `Size`: Start index
  - count `Size`: Count
  - ratios_out `double*`: Output buffer
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - Add single feature (method 1)
  - coord `const Feature&`: Coordinate
  - size `float`: Size (default 0.0f)
  - angle `float`: Angle (default 0.0f)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - Add single feature (method 2)
  - x `float`: x coordinate
  - y `float`: y coordinate
  - size `float`: Size (default 0.0f)
  - angle `float`: Angle (default 0.0f)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeaturesBlock</span> - Batch add features (SIMD optimized)
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`: Coordinate block
  - sizes_block `const float*`: Size data pointer (optional)
  - angles_block `const float*`: Angle data pointer (optional)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">RemoveUnused</span> - Remove unused features
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ZeroCounters</span> - Batch zero counters (SIMD optimized)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (returns proxy object)
  - index `Size`: Feature index
  - Returns `FeaturePointProxy`: Proxy object
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - STL-style iterators
  - Returns `SimpleIterator`: Iterator object

```{tip}
**Performance Tips | 性能建议**

1. **Prioritize Batch Operations**: Use batch APIs like `AddFeaturesBlock`, `SetSizesBlock`, etc., to fully utilize SIMD acceleration
2. **Reserve Capacity**: Use `reserve()` to avoid multiple memory reallocations
3. **Direct Access**: Use reference interfaces like `GetCoordsRef()` for batch processing
4. **SOA Descriptors**: Prefer `DescriptorsSOA` over legacy `Descs` for 8-9x speedup
```

### `FeaturePoint` (Legacy, Retained for Compatibility)
(feature-point)=
Legacy feature point class (replaced by FeaturePoints, retained for compatibility)

```{warning}
**Deprecated | 已弃用**

The `FeaturePoint` class has been replaced by the high-performance `FeaturePoints` SOA container. New code should directly use `FeaturePoints` and its proxy object `FeaturePointProxy`.

The legacy `FeaturePoint` is retained only for backward compatibility and is not recommended for use in new code.
```

### `ImageFeatureInfo`
(image-feature-info)=
Single image feature information encapsulation class with SIMD-optimized SOA design, including optional descriptor management

**Private Member Variables**:
| Name                   | Type                                        | Default Value | Purpose                                                          |
| ---------------------- | ------------------------------------------- | ------------- | ---------------------------------------------------------------- |
| `image_path_`          | `std::string`                               | ""            | Image file path string                                           |
| `features_`            | [`FeaturePoints`](featurepoints-class)      | Default value | Feature point collection (SOA format, SIMD optimized)            |
| `is_used_`             | `bool`                                      | true          | Usage status of this image feature information                   |
| `descriptors_ptr_`     | `Ptr<`[`Descs`](descs)`>`                   | nullptr       | Legacy AOS descriptor pointer (backward compatible, optional)    |
| `descriptors_soa_ptr_` | `Ptr<`[`DescriptorsSOA`](descriptorssoa)`>` | nullptr       | SOA descriptor pointer (high performance, optional, recommended) |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">ImageFeatureInfo</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">ImageFeatureInfo</span> - Parameterized constructor
  - path `const std::string &`: Image file path
  - used `bool`: Usage flag (default true)
- <span style="color:#1976d2;font-weight:bold;">GetImagePath</span> - Get image path
  - Returns `const std::string&`: Image path reference
- <span style="color:#1976d2;font-weight:bold;">SetImagePath</span> - Set image path
  - path `const std::string&`: Image path
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - Get usage status
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - Set usage status
  - used `bool`: Whether used
- <span style="color:#1976d2;font-weight:bold;">GetFeaturePoints</span> - Get feature point collection (non-const version)
  - Returns `FeaturePoints&`: Feature point collection reference
- <span style="color:#1976d2;font-weight:bold;">GetFeaturePoints</span> - Get feature point collection (const version)
  - Returns `const FeaturePoints&`: Feature point collection const reference
- <span style="color:#1976d2;font-weight:bold;">GetNumFeatures</span> - Get feature point count
- <span style="color:#1976d2;font-weight:bold;">GetNumValidFeatures</span> - Get valid feature point count (SIMD optimized)
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - Add single feature (method 1)
  - coord `const Feature&`: Coordinate
  - size `float`: Size (default 0.0f)
  - angle `float`: Angle (default 0.0f)
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - Add single feature (method 2)
  - x `float`: x coordinate
  - y `float`: y coordinate
  - size `float`: Size (default 0.0f)
  - angle `float`: Angle (default 0.0f)
- <span style="color:#1976d2;font-weight:bold;">AddFeaturesBlock</span> - Batch add features (SIMD optimized)
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`: Coordinate block
  - sizes_block `const float*`: Size data pointer (optional)
  - angles_block `const float*`: Angle data pointer (optional)
- <span style="color:#1976d2;font-weight:bold;">ClearUnusedFeatures</span> - Clear unused feature points
- <span style="color:#1976d2;font-weight:bold;">ClearAllFeatures</span> - Clear all feature points
- <span style="color:#1976d2;font-weight:bold;">ReserveFeatures</span> - Reserve feature point capacity
  - capacity `Size`: Reserved capacity
- <span style="color:#1976d2;font-weight:bold;">ZeroFeatureCounters</span> - Zero feature counters (SIMD optimized)
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get feature point count
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - Iterator access
- <span style="color:#1976d2;font-weight:bold;">HasDescriptors</span> - Check if descriptors are stored (AOS or SOA)
- <span style="color:#1976d2;font-weight:bold;">HasDescriptorsSOA</span> - Check if SOA descriptors are stored
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsPtr</span> - Get legacy AOS descriptor pointer (const version)
  - Returns `const Ptr<Descs>&`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsPtr</span> - Get legacy AOS descriptor pointer (non-const version)
  - Returns `Ptr<Descs>&`
- <span style="color:#1976d2;font-weight:bold;">SetDescriptors</span> - Set legacy AOS descriptors
  - descs_ptr `const Ptr<Descs>&`: Descriptor pointer
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOAPtr</span> - Get SOA descriptor pointer (const version) (recommended)
  - Returns `const Ptr<DescriptorsSOA>&`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOAPtr</span> - Get SOA descriptor pointer (non-const version) (recommended)
  - Returns `Ptr<DescriptorsSOA>&`
- <span style="color:#1976d2;font-weight:bold;">SetDescriptorsSOA</span> - Set SOA descriptors (recommended)
  - descs_soa_ptr `const Ptr<DescriptorsSOA>&`: SOA descriptor pointer
- <span style="color:#1976d2;font-weight:bold;">ClearDescriptors</span> - Clear descriptors and release memory
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsRefCount</span> - Get AOS descriptor reference count (for debugging)
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOARefCount</span> - Get SOA descriptor reference count (for debugging)
- <span style="color:#1976d2;font-weight:bold;">GetFeature</span> - Get feature proxy object (compatibility method)
  - index `Size`: Feature index
  - Returns `FeatureProxy`: Feature proxy object

### `FeaturesInfo`
(features-info)=
Feature information collection for all images, enhanced container class inheriting from std::vector<[`ImageFeatureInfo`](image-feature-info)>

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (non-const version)
  - index `Size`: Image index
  - Returns `ImageFeatureInfo*`: Image feature information pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (const version)
  - index `Size`: Image index
  - Returns `const ImageFeatureInfo*`: Image feature information const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetImageFeaturePtr</span> - Safely get image feature information pointer (non-const version)
  - index `Size`: Image index
  - Returns `ImageFeatureInfo*`: Image feature information pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetImageFeaturePtr</span> - Safely get image feature information pointer (const version)
  - index `Size`: Image index
  - Returns `const ImageFeatureInfo*`: Image feature information const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">AddImageFeatures</span> - Add image feature information
  - image_path `const std::string &`: Image path
  - is_used `bool`: Usage flag (default true)
- <span style="color:#1976d2;font-weight:bold;">GetNumValidImages</span> - Get valid image count
- <span style="color:#1976d2;font-weight:bold;">ClearUnusedImages</span> - Clear unused images
- <span style="color:#1976d2;font-weight:bold;">ClearAllUnusedFeatures</span> - Clear all unused feature points
- <span style="color:#1976d2;font-weight:bold;">Print</span> - Print feature information details
  - print_unused `bool`: Whether to print unused features (default false)

### Feature-Related Type Definitions
(feature-related-types)=
(features-info-ptr)=

**Type Definitions**:
| Type              | Definition                                           | Default Value | Purpose                             |
| ----------------- | ---------------------------------------------------- | ------------- | ----------------------------------- |
| `FeaturesInfoPtr` | `std::shared_ptr<`[`FeaturesInfo`](features-info)`>` | nullptr       | Smart pointer type for feature info |

---

## Matching (types/matches.hpp)

### `IdMatch`
(id-match)=
Feature point matching structure

**Member Variables**:
| Name        | Type                | Default Value | Purpose                                        |
| ----------- | ------------------- | ------------- | ---------------------------------------------- |
| `i`         | [`IndexT`](index-t) | 0             | Index identifier of the first feature          |
| `j`         | [`IndexT`](index-t) | 0             | Index identifier of the second feature         |
| `is_inlier` | `bool`              | false         | RANSAC inlier flag, indicates matching quality |

### Match Collection Type Definitions
(matches-collection-types)=
(id-matches)=
(view-pair)=
(matches)=
(matches-ptr)=

**Type Definitions**:
| Type         | Definition                                                         | Default Value | Purpose                                                               |
| ------------ | ------------------------------------------------------------------ | ------------- | --------------------------------------------------------------------- |
| `IdMatches`  | `std::vector<`[`IdMatch`](id-match)`>`                             | Empty vector  | Feature matching collection, stores all matches between an image pair |
| `ViewPair`   | `std::pair<`[`IndexT`](index-t)`, `[`IndexT`](index-t)`>`          | (0,0)         | View index pair, identifies a combination of two views                |
| `Matches`    | `std::map<`[`ViewPair`](view-pair)`, `[`IdMatches`](id-matches)`>` | Empty map     | Match relationship mapping between all view pairs                     |
| `MatchesPtr` | `std::shared_ptr<`[`Matches`](matches)`>`                          | nullptr       | Smart pointer type for match collection                               |

### Bearing Vector Type Definitions
(bearing-vector-types)=
(bearing-vector)=
(bearing-vectors)=
(bearing-pairs)=

**Type Definitions**:
| Type             | Definition                               | Default Value | Purpose                                                            |
| ---------------- | ---------------------------------------- | ------------- | ------------------------------------------------------------------ |
| `BearingVector`  | [`Vector3d`](vector3d)                   | Zero vector   | Unit observation vector type, from camera center to 3D point       |
| `BearingVectors` | `Eigen::Matrix<double,3,Eigen::Dynamic>` | Zero matrix   | 3×N observation vector matrix, stores multiple observation vectors |
| `BearingPairs`   | `std::vector<Eigen::Matrix<double,6,1>>` | Empty vector  | Matched bearing vector pairs for relative pose estimation          |

### Bearing Vector Utility Functions
(bearing-vector-utils)=

**Available Functions**:
- <span style="color:#1976d2;font-weight:bold;">PixelToBearingVector</span> - Convert pixel coordinates to unit observation vector
  - pixel_coord `const Vector2d &`: Pixel coordinates
  - camera_model `const CameraModel &`: Camera model
  - Returns `BearingVector`: Unit observation vector
- <span style="color:#1976d2;font-weight:bold;">PixelToBearingVector</span> - Convert pixel coordinates to unit observation vector (overloaded version)
  - pixel_coord `const Vector2d &`: Pixel coordinates
  - fx `double`: x-direction focal length
  - fy `double`: y-direction focal length
  - cx `double`: x-direction principal point offset
  - cy `double`: y-direction principal point offset
  - Returns `BearingVector`: Unit observation vector
- <span style="color:#1976d2;font-weight:bold;">MatchesToBearingPairs</span> - Convert matches to bearing vector pairs
  - matches `const IdMatches &`: Feature matches
  - features_info `const FeaturesInfo &`: Feature information for all views
  - camera_models `const CameraModels &`: Camera models for all views
  - view_pair `const ViewPair &`: View pair index
  - bearing_pairs `BearingPairs &`: Output bearing vector pairs
  - Returns `bool`: Whether successful
- <span style="color:#1976d2;font-weight:bold;">MatchesToBearingPairsInliersOnly</span> - Convert inlier matches to bearing vector pairs
  - matches `const IdMatches &`: Feature matches
  - features_info `const FeaturesInfo &`: Feature information for all views
  - camera_models `const CameraModels &`: Camera models for all views
  - view_pair `const ViewPair &`: View pair index
  - bearing_pairs `BearingPairs &`: Output bearing vector pairs (inliers only)
  - Returns `bool`: Whether successful

---

## Tracks and Observations (types/tracks.hpp)

The track system uses an encapsulation class design, providing better data security and access control.

### Core Features and Usage Patterns

**Encapsulation Design Advantages**:
- **Private Member Variables**: Prevent accidental modification, improve data security
- **Public Access Interfaces**: Provide unified data access and modification methods
- **Memory Alignment Optimization**: Use Eigen aligned_allocator for improved performance, includes EIGEN_MAKE_ALIGNED_OPERATOR_NEW macro
- **Container-Style Interface**: Support STL container usage patterns

### Typical Usage Examples

#### Basic Observation Information Creation and Operations
```cpp
using namespace PoSDK::types;

// Create observation information
ObsInfo obs1;  // Default construction
ObsInfo obs2(0, 1, Vector2d(100.0, 200.0));  // View ID, feature ID, coordinates

// Set and get properties
obs1.SetViewId(1);
obs1.SetFeatureId(5);  // Set feature index in FeaturesInfo[view_id]
obs1.SetObsId(123);
obs1.SetCoord(Vector2d(150.0, 250.0));
obs1.SetUsed(true);

// Get properties
ViewId view_id = obs1.GetViewId();
IndexT feature_id = obs1.GetFeatureId();  // Get feature ID
IndexT obs_id = obs1.GetObsId();
const Vector2d& coord = obs1.GetCoord();
bool is_used = obs1.IsUsed();

// Get homogeneous coordinates
Vector3d homo_coord = obs1.GetHomoCoord();
```

#### Track Information Management
```cpp
// Create track information
TrackInfo track_info;

// Add observations
track_info.AddObservation(0, 1, Vector2d(100.0, 200.0));  // View ID, feature ID, coordinates
track_info.AddObservation(obs1);  // Directly add observation object

// Access observations
Size obs_count = track_info.GetObservationCount();
Size valid_count = track_info.GetValidObservationCount();

// Array-style access (returns pointer)
const ObsInfo* obs_ptr = track_info[0];
if (obs_ptr) {
    Vector2d coord = obs_ptr->GetCoord();
}

// Safe access (reference access, throws exception if out of bounds)
try {
    const ObsInfo& obs_ref = track_info.GetObservation(1);
    ViewId view_id = obs_ref.GetViewId();
} catch (const std::out_of_range& e) {
    // Handle out-of-bounds access
}

// Get track data
const Track& track = track_info.GetTrack();
Track& mutable_track = track_info.GetTrack();

// Set track usage status
track_info.SetUsed(true);
bool track_used = track_info.IsUsed();

// Set observation status
track_info.SetObservationUsed(0, false);
track_info.SetObservationFeatureId(0, 10);
```

#### Track Collection Management
```cpp
// Create track collection
Tracks tracks;

// Add tracks
std::vector<ObsInfo> observations = {obs1, obs2};
tracks.AddTrack(observations);
tracks.AddTrack(track_info);

// Direct addition (compatibility method)
tracks.push_back(track_info);
tracks.emplace_back(observations, true);

// Access tracks
Size track_count = tracks.size();
Size valid_track_count = tracks.GetValidTrackCount();

// Array-style access (returns pointer)
const TrackInfo* track_ptr = tracks[0];
if (track_ptr) {
    Size obs_count = track_ptr->GetObservationCount();
}

// Safe access (reference access, throws exception if out of bounds)
try {
    const TrackInfo& track_ref = tracks.GetTrack(1);
    bool is_used = track_ref.IsUsed();
} catch (const std::out_of_range& e) {
    // Handle out-of-bounds access
}

// Quick observation access: Tracks[point ID, view ID]
const ObsInfo* obs_ptr = tracks(1, 0);  // Point ID=1, View ID=0
if (obs_ptr) {
    Vector2d coord = obs_ptr->GetCoord();
}

// Iterator access
for (const auto& track_info : tracks) {
    if (track_info.IsUsed()) {
        for (const auto& obs : track_info.GetTrack()) {
            if (obs.IsUsed()) {
                Vector2d coord = obs.GetCoord();
                // Process observation
            }
        }
    }
}

// Normalization processing
CameraModels camera_models;
// ... Set camera models ...
bool normalized = tracks.NormalizeTracks(camera_models);
bool is_normalized = tracks.IsNormalized();
tracks.SetNormalized(true);
```

### `ObsInfo`
(obs-info)=
Single observation information encapsulation class for a 3D point, providing private members and public access interfaces

**Private Member Variables**:
| Name              | Type                     | Default Value | Purpose                                                 |
| ----------------- | ------------------------ | ------------- | ------------------------------------------------------- |
| `coord_`          | [`Vector2d`](vector2d)   | Zero vector   | Current 2D coordinates (may be original or reprojected) |
| `original_coord_` | `std::vector<double>`    | Empty vector  | Original image coordinates (empty if not stored)        |
| `view_id_`        | [`ViewId`](view-id)      | 0             | Image ID (consistent with index in ImagePaths)          |
| `feature_id_`     | [`IndexT`](index-t)      | 0             | Feature point ID index in FeaturesInfo[view_id]         |
| `obs_id_`         | [`IndexT`](index-t)      | 0             | Observation ID                                          |
| `is_used_`        | `bool`                   | true          | Whether the current observation is used                 |
| `color_rgb_`      | `std::array<uint8_t, 3>` | {0,0,0}       | RGB color values (0-255)                                |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">ObsInfo</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">ObsInfo</span> - Parameterized constructor
  - vid `ViewId`: View ID
  - fid `IndexT`: Feature ID
  - c `const Vector2d &`: Observation coordinates
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - Get observation coordinates
  - Returns `const Vector2d&`: Observation coordinate reference
- <span style="color:#1976d2;font-weight:bold;">SetCoord</span> - Set observation coordinates
  - coord `const Vector2d&`: Observation coordinates
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetViewId</span> - Get view ID
  - Returns `ViewId`: View ID
- <span style="color:#1976d2;font-weight:bold;">SetViewId</span> - Set view ID
  - view_id `ViewId`: View ID
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetFeatureId</span> - Get feature ID
  - Returns `IndexT`: Feature ID (index in FeaturesInfo[view_id])
- <span style="color:#1976d2;font-weight:bold;">SetFeatureId</span> - Set feature ID
  - feature_id `IndexT`: Feature ID (index in FeaturesInfo[view_id])
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetObsId</span> - Get observation ID
  - Returns `IndexT`: Observation ID
- <span style="color:#1976d2;font-weight:bold;">SetObsId</span> - Set observation ID
  - obs_id `IndexT`: Observation ID
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - Get usage status
  - Returns `bool`: Usage status flag
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - Set observation usage status
  - used `bool`: Whether used flag
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetColorRGB</span> - Get RGB color
  - Returns `const std::array<uint8_t, 3>&`: RGB color array reference
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - Set RGB color (method 1)
  - r `uint8_t`: Red component (0-255)
  - g `uint8_t`: Green component (0-255)
  - b `uint8_t`: Blue component (0-255)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - Set RGB color (method 2)
  - rgb `const std::array<uint8_t, 3>&`: RGB color array
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">HasColor</span> - Check if color information exists
  - Returns `bool`: Whether there are non-zero color values
- <span style="color:#1976d2;font-weight:bold;">HasOriginalCoord</span> - Check if original coordinates are stored
  - Returns `bool`: Whether original coordinates are stored
- <span style="color:#1976d2;font-weight:bold;">StoreOriginalCoord</span> - Store current coordinates as original coordinates
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ResetToOriginalCoord</span> - Reset current coordinates to original coordinates
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetOriginalCoord</span> - Get original coordinates
  - Returns `Vector2d`: Original coordinates (returns current coordinates if not stored)
- <span style="color:#1976d2;font-weight:bold;">GetReprojectionError</span> - Calculate reprojection error
  - Returns `double`: Euclidean distance between current and original coordinates (returns -1.0 if no original data)
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - Get observation coordinates (compatibility version)
  - coord_out `Vector2d &`: Output coordinate reference
  - Returns `bool`: Whether successfully retrieved
- <span style="color:#1976d2;font-weight:bold;">GetHomoCoord</span> - Get homogeneous coordinates
  - Returns `Vector3d`: Homogeneous coordinates

### `Track`
(track)=
3D point observation information collection, type alias: `std::vector<`[`ObsInfo`](obs-info)`, Eigen::aligned_allocator<`[`ObsInfo`](obs-info)`>>`

### `TrackInfo`
(track-info)=
Track information encapsulation class providing private members and public access interfaces

**Private Member Variables**:
| Name       | Type             | Default Value | Purpose                    |
| ---------- | ---------------- | ------------- | -------------------------- |
| `track_`   | [`Track`](track) | Default value | Track observation data     |
| `is_used_` | `bool`           | true          | Whether this track is used |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - Parameterized constructor (method 1)
  - t `const Track &`: Track data
  - used `bool`: Usage flag (default true)
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - Parameterized constructor (method 2)
  - observations `const std::vector<ObsInfo> &`: Observation array
  - used `bool`: Usage flag (default true)
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - Get track data (non-const version)
  - Returns `Track&`: Track data reference
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - Get track data (const version)
  - Returns `const Track&`: Track data const reference
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - Get usage status
  - Returns `bool`: Usage status flag
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - Set track usage status
  - used `bool`: Whether to use this track
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (non-const version)
  - index `Size`: Observation index
  - Returns `ObsInfo*`: Observation pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (const version)
  - index `Size`: Observation index
  - Returns `const ObsInfo*`: Observation const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - Get observation information (non-const version)
  - index `IndexT`: Observation index
  - Returns `ObsInfo&`: Observation reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - Get observation information (const version)
  - index `IndexT`: Observation index
  - Returns `const ObsInfo&`: Observation const reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - Add observation (method 1)
  - view_id `ViewId`: View ID
  - feature_id `IndexT`: Feature ID (index in FeaturesInfo[view_id])
  - coord `const Vector2d &`: Observation coordinates
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - Add observation (method 2)
  - obs `const ObsInfo &`: Observation information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetObservationCount</span> - Get observation count
  - Returns `Size`: Observation count
- <span style="color:#1976d2;font-weight:bold;">GetValidObservationCount</span> - Get valid observation count
  - Returns `Size`: Valid observation count
- <span style="color:#1976d2;font-weight:bold;">GetObservationCoord</span> - Get coordinates of specified observation
  - index `IndexT`: Observation index
  - coord_out `Vector2d &`: Output coordinate reference
  - Returns `bool`: Whether successfully retrieved
- <span style="color:#1976d2;font-weight:bold;">SetObservationUsed</span> - Set usage status of specified observation
  - index `IndexT`: Observation index
  - used `bool`: Whether to use this observation
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetObservationFeatureId</span> - Set feature ID of specified observation
  - index `IndexT`: Observation index
  - feature_id `IndexT`: Feature ID (index in FeaturesInfo[view_id])
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ClearObservations</span> - Clear observation data
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ReserveObservations</span> - Reserve observation capacity
  - capacity `Size`: Reserved capacity
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get observation count
  - Returns `Size`: Observation count
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
  - Returns `bool`: Whether empty
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - Iterator access
  - Returns iterator object

### `Tracks`
(tracks)=
Track collection encapsulation class providing complete track management interface

**Private Member Variables**:
| Name             | Type                                                                                              | Default Value | Purpose                   |
| ---------------- | ------------------------------------------------------------------------------------------------- | ------------- | ------------------------- |
| `tracks_`        | `std::vector<`[`TrackInfo`](track-info)`, Eigen::aligned_allocator<`[`TrackInfo`](track-info)`>>` | Empty vector  | Track information vector  |
| `is_normalized_` | `bool`                                                                                            | false         | Normalization status flag |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">IsNormalized</span> - Get normalization status
  - Returns `bool`: Normalization status flag
- <span style="color:#1976d2;font-weight:bold;">SetNormalized</span> - Set normalization status
  - normalized `bool`: Normalization flag
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get track count
  - Returns `Size`: Track count
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
  - Returns `bool`: Whether empty
- <span style="color:#1976d2;font-weight:bold;">clear</span> - Clear all tracks
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - Reserve capacity
  - n `Size`: Reserved size
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (non-const version)
  - index `Size`: Track index
  - Returns `TrackInfo*`: Track pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (const version)
  - index `Size`: Track index
  - Returns `const TrackInfo*`: Track const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - Get track (non-const version)
  - index `Size`: Track index
  - Returns `TrackInfo&`: Track reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - Get track (const version)
  - index `Size`: Track index
  - Returns `const TrackInfo&`: Track const reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - Quick observation access (non-const version)
  - pts_id `PtsId`: Point ID
  - view_id `ViewId`: View ID
  - Returns `ObsInfo*`: Observation pointer, returns nullptr if not found
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - Quick observation access (const version)
  - pts_id `PtsId`: Point ID
  - view_id `ViewId`: View ID
  - Returns `const ObsInfo*`: Observation const pointer, returns nullptr if not found
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - Get observation (non-const version)
  - pts_id `PtsId`: Point ID
  - view_id `ViewId`: View ID
  - Returns `ObsInfo*`: Observation pointer, returns nullptr if not found
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - Get observation (const version)
  - pts_id `PtsId`: Point ID
  - view_id `ViewId`: View ID
  - Returns `const ObsInfo*`: Observation const pointer, returns nullptr if not found
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - Iterator access
  - Returns iterator object
- <span style="color:#1976d2;font-weight:bold;">cbegin</span>, <span style="color:#1976d2;font-weight:bold;">cend</span> - Const iterator access
  - Returns const iterator object
- <span style="color:#1976d2;font-weight:bold;">AddTrack</span> - Add new track (method 1)
  - observations `const std::vector<ObsInfo> &`: Observation information array
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddTrack</span> - Add new track (method 2)
  - track_info `const TrackInfo &`: Track information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - Add observation to specified track
  - track_id `PtsId`: Track ID (index in tracks vector)
  - obs `const ObsInfo &`: Observation information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetTrackCount</span> - Get track count
  - Returns `Size`: Track count
- <span style="color:#1976d2;font-weight:bold;">GetObservationCount</span> - Get observation count of specified track
  - track_id `PtsId`: Track ID
  - Returns `Size`: Observation count
- <span style="color:#1976d2;font-weight:bold;">GetObservationByTrackIndex</span> - Get observation by track index
  - track_id `PtsId`: Track ID
  - index `IndexT`: Observation index
  - Returns `const ObsInfo&`: Observation reference
- <span style="color:#1976d2;font-weight:bold;">GetValidTrackCount</span> - Get valid track count
  - Returns `Size`: Valid track count
- <span style="color:#1976d2;font-weight:bold;">NormalizeTracks</span> - Normalize entire track collection using camera models
  - camera_models `const CameraModels &`: Camera model collection
  - Returns `bool`: Whether normalization succeeded
- <span style="color:#1976d2;font-weight:bold;">OriginalizeTracks</span> - Restore normalized tracks to original pixel coordinates using camera models
  - camera_models `const CameraModels &`: Camera model collection
  - Returns `bool`: Whether restoration succeeded
- <span style="color:#1976d2;font-weight:bold;">StoreOriginalCoords</span> - Store original coordinates for all observations
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ResetToOriginalCoords</span> - Reset all observations to original coordinates
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">push_back</span> - Add track (compatibility method)
  - track `const TrackInfo &`: Track information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">push_back</span> - Add track (move version)
  - track `TrackInfo &&`: Track information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - Emplace construct track
  - args `Args &&...`: Construction parameters
  - Returns `void`

### Track-Related Type Definitions
(tracks-related-types)=
(tracks-ptr)=

**Type Definitions**:
| Type        | Definition                              | Default Value | Purpose                                 |
| ----------- | --------------------------------------- | ------------- | --------------------------------------- |
| `TracksPtr` | `std::shared_ptr<`[`Tracks`](tracks)`>` | nullptr       | Smart pointer type for track collection |

---

## Relative Poses (types/relative_poses.hpp)

The relative pose system uses an encapsulation class design, implemented with PIMPL pattern, providing better data security and binary compatibility.

### Core Features and Usage Patterns

**Encapsulation Design Advantages**:
- **PIMPL Pattern**: Hide implementation details, improve binary compatibility
- **Candidate Solution Support**: Support management of multiple candidate pose solutions
- **Private Member Variables**: Prevent accidental modification, improve data security
- **Public Access Interfaces**: Provide unified data access and modification methods
- **Memory Alignment Optimization**: Use Eigen aligned_allocator for improved performance

### Typical Usage Examples

#### Basic Relative Rotation Creation and Operations
```cpp
using namespace PoSDK::types;

// Create relative rotation
RelativeRotation rel_rot1;  // Default construction
RelativeRotation rel_rot2(0, 1, Matrix3d::Identity(), 1.0f);  // Full parameter construction

// Set and get view IDs
rel_rot1.SetViewIdI(0);
rel_rot1.SetViewIdJ(1);
ViewId view_i = rel_rot1.GetViewIdI();
ViewId view_j = rel_rot1.GetViewIdJ();

// Set and get rotation matrix
Matrix3d R = Matrix3d::Identity();
rel_rot1.SetRotation(R);
Matrix3d R_out = rel_rot1.GetRotation();

// Set and get weight
rel_rot1.SetWeight(0.8f);
float weight = rel_rot1.GetWeight();

// Candidate solution management
rel_rot1.AddCandidateRotation(R);
rel_rot1.AddCandidateRotation(Matrix3d::Identity());
size_t num_candidates = rel_rot1.GetCandidateCount();
Matrix3d candidate_R = rel_rot1.GetCandidateRotation(0);
rel_rot1.SetPrimaryCandidateIndex(1);
size_t primary_idx = rel_rot1.GetPrimaryCandidateIndex();
rel_rot1.ClearCandidates();
```

#### Relative Pose Management
```cpp
// Create relative pose
RelativePose rel_pose1;  // Default construction
RelativePose rel_pose2(0, 1, Matrix3d::Identity(), Vector3d::Zero(), 1.0f);

// Basic accessors
Matrix3d R = rel_pose1.GetRotation();
Vector3d t = rel_pose1.GetTranslation();
rel_pose1.SetRotation(R);
rel_pose1.SetTranslation(t);

// Candidate solution management
rel_pose1.AddCandidate(Matrix3d::Identity(), Vector3d::Zero());
size_t num_candidates = rel_pose1.GetCandidateCount();
Matrix3d candidate_R = rel_pose1.GetCandidateRotation(0);
Vector3d candidate_t = rel_pose1.GetCandidateTranslation(0);

// Pose operations
Matrix3d E = rel_pose1.GetEssentialMatrix();
double rot_err, trans_err;
bool success = rel_pose1.ComputeErrors(rel_pose2, rot_err, trans_err, false);
```

#### Relative Pose Collection Management
```cpp
// Create relative pose collection
RelativePoses poses;

// Add poses
poses.AddPose(rel_pose1);  // Copy add
poses.AddPose(std::move(rel_pose2));  // Move add
poses.EmplacePose(0, 1, Matrix3d::Identity(), Vector3d::Zero(), 1.0f);  // In-place construction

// Access poses
size_t num_poses = poses.size();
const RelativePose* pose_ptr = poses[0];  // Array-style access, returns pointer
if (pose_ptr) {
    Matrix3d R = pose_ptr->GetRotation();
}

// Safe access
try {
    const RelativePose& pose_ref = poses.GetPose(1);  // Reference access, throws exception if out of bounds
    Vector3d t = pose_ref.GetTranslation();
} catch (const std::out_of_range& e) {
    // Handle out-of-bounds access
}

// Iterator access
for (const auto& pose : poses) {
    ViewId i = pose.GetViewIdI();
    ViewId j = pose.GetViewIdJ();
    // Process pose
}

// Specific operations
std::vector<double> rot_errors, trans_errors;
size_t matched = poses.EvaluateAgainst(gt_poses, rot_errors, trans_errors);

Matrix3d R;
Vector3d t;
ViewPair vp(0, 1);
bool found = poses.GetRelativePose(vp, R, t);
```

### `RelativeRotation`
(relative-rotation)=
Encapsulated relative rotation class supporting candidate solution management

**Private Members (implemented via PIMPL)**:
| Name                  | Type                    | Default Value   | Purpose                                        |
| --------------------- | ----------------------- | --------------- | ---------------------------------------------- |
| view_id_i             | [`ViewId`](view-id)     | 0               | Index of source camera view                    |
| view_id_j             | [`ViewId`](view-id)     | 0               | Index of target camera view                    |
| Rij                   | [`Matrix3d`](matrix3d)  | Identity matrix | Relative rotation matrix from view i to view j |
| weight                | `float`                 | 1.0f            | Weight factor for relative rotation            |
| candidates            | `std::vector<Matrix3d>` | Empty vector    | Candidate rotation solution collection         |
| primary_candidate_idx | `size_t`                | 0               | Primary candidate solution index               |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">RelativeRotation</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">RelativeRotation</span> - Parameterized constructor
  - view_id_i `ViewId`: Source view index
  - view_id_j `ViewId`: Target view index
  - Rij `const Matrix3d &`: Relative rotation matrix (default identity matrix)
  - weight `float`: Weight coefficient (default 1.0f)
- <span style="color:#1976d2;font-weight:bold;">GetViewIdI</span> - Get source view ID
- <span style="color:#1976d2;font-weight:bold;">GetViewIdJ</span> - Get target view ID
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - Get rotation matrix
- <span style="color:#1976d2;font-weight:bold;">GetWeight</span> - Get weight
- <span style="color:#1976d2;font-weight:bold;">SetViewIdI</span> - Set source view ID
  - view_id_i `ViewId`: Source view ID
- <span style="color:#1976d2;font-weight:bold;">SetViewIdJ</span> - Set target view ID
  - view_id_j `ViewId`: Target view ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - Set rotation matrix
  - Rij `const Matrix3d &`: Rotation matrix
- <span style="color:#1976d2;font-weight:bold;">SetWeight</span> - Set weight
  - weight `float`: Weight value
- <span style="color:#1976d2;font-weight:bold;">GetCandidateCount</span> - Get candidate solution count
- <span style="color:#1976d2;font-weight:bold;">GetCandidateRotation</span> - Get candidate rotation at specified index
  - index `size_t`: Candidate solution index
- <span style="color:#1976d2;font-weight:bold;">AddCandidateRotation</span> - Add candidate rotation solution
  - Rij `const Matrix3d &`: Candidate rotation matrix
- <span style="color:#1976d2;font-weight:bold;">SetPrimaryCandidateIndex</span> - Set primary candidate solution index
  - index `size_t`: Candidate solution index
- <span style="color:#1976d2;font-weight:bold;">GetPrimaryCandidateIndex</span> - Get primary candidate solution index
- <span style="color:#1976d2;font-weight:bold;">ClearCandidates</span> - Clear all candidate solutions

### `RelativePose`
(relative-pose)=
Encapsulated relative pose class supporting candidate solution management

**Private Members (implemented via PIMPL)**:
| Name                  | Type                    | Default Value   | Purpose                                           |
| --------------------- | ----------------------- | --------------- | ------------------------------------------------- |
| view_id_i             | [`ViewId`](view-id)     | 0               | Index of source camera view                       |
| view_id_j             | [`ViewId`](view-id)     | 0               | Index of target camera view                       |
| Rij                   | [`Matrix3d`](matrix3d)  | Identity matrix | Relative rotation matrix from view i to view j    |
| tij                   | [`Vector3d`](vector3d)  | Zero vector     | Relative translation vector from view i to view j |
| weight                | `float`                 | 1.0f            | Weight factor for pose estimation                 |
| candidates_R          | `std::vector<Matrix3d>` | Empty vector    | Candidate rotation solution collection            |
| candidates_t          | `std::vector<Vector3d>` | Empty vector    | Candidate translation solution collection         |
| primary_candidate_idx | `size_t`                | 0               | Primary candidate solution index                  |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">RelativePose</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">RelativePose</span> - Parameterized constructor
  - view_id_i `ViewId`: Source view index
  - view_id_j `ViewId`: Target view index
  - Rij `const Matrix3d &`: Relative rotation matrix (default identity matrix)
  - tij `const Vector3d &`: Relative translation vector (default zero vector)
  - weight `float`: Weight coefficient (default 1.0f)
- <span style="color:#1976d2;font-weight:bold;">GetViewIdI</span> - Get source view ID
- <span style="color:#1976d2;font-weight:bold;">GetViewIdJ</span> - Get target view ID
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - Get rotation matrix
- <span style="color:#1976d2;font-weight:bold;">GetTranslation</span> - Get translation vector
- <span style="color:#1976d2;font-weight:bold;">GetWeight</span> - Get weight
- <span style="color:#1976d2;font-weight:bold;">SetViewIdI</span> - Set source view ID
  - view_id_i `ViewId`: Source view ID
- <span style="color:#1976d2;font-weight:bold;">SetViewIdJ</span> - Set target view ID
  - view_id_j `ViewId`: Target view ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - Set rotation matrix
  - Rij `const Matrix3d &`: Rotation matrix
- <span style="color:#1976d2;font-weight:bold;">SetTranslation</span> - Set translation vector
  - tij `const Vector3d &`: Translation vector
- <span style="color:#1976d2;font-weight:bold;">SetWeight</span> - Set weight
  - weight `float`: Weight value
- <span style="color:#1976d2;font-weight:bold;">GetCandidateCount</span> - Get candidate solution count
- <span style="color:#1976d2;font-weight:bold;">GetCandidateRotation</span> - Get candidate rotation at specified index
  - index `size_t`: Candidate solution index
- <span style="color:#1976d2;font-weight:bold;">GetCandidateTranslation</span> - Get candidate translation at specified index
  - index `size_t`: Candidate solution index
- <span style="color:#1976d2;font-weight:bold;">AddCandidate</span> - Add candidate solution
  - Rij `const Matrix3d &`: Candidate rotation matrix
  - tij `const Vector3d &`: Candidate translation vector
- <span style="color:#1976d2;font-weight:bold;">SetPrimaryCandidateIndex</span> - Set primary candidate solution index
  - index `size_t`: Candidate solution index
- <span style="color:#1976d2;font-weight:bold;">GetPrimaryCandidateIndex</span> - Get primary candidate solution index
- <span style="color:#1976d2;font-weight:bold;">ClearCandidates</span> - Clear all candidate solutions
- <span style="color:#1976d2;font-weight:bold;">GetEssentialMatrix</span> - Get essential matrix
  - Returns `Matrix3d`: Essential matrix
- <span style="color:#1976d2;font-weight:bold;">ComputeErrors</span> - Compute errors with another relative pose
  - other `const RelativePose &`: Another relative pose
  - rotation_error `double &`: Output rotation error (angle)
  - translation_error `double &`: Output translation direction error (angle)
  - is_opengv_format `bool`: Whether OpenGV format (default false)
  - Returns `bool`: Whether computation succeeded

### `RelativeRotations`
(relative-rotations)=
Relative rotation container class (encapsulation class design, private member variables + public access interfaces)

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get element count
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
- <span style="color:#1976d2;font-weight:bold;">clear</span> - Clear all elements
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - Reserve capacity
  - n `size_t`: Reserved size
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (non-const version)
  - index `size_t`: Index
  - Returns `RelativeRotation*`: Rotation pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (const version)
  - index `size_t`: Index
  - Returns `const RelativeRotation*`: Rotation const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - Get rotation (non-const version)
  - index `size_t`: Index
  - Returns `RelativeRotation&`: Rotation reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - Get rotation (const version)
  - index `size_t`: Index
  - Returns `const RelativeRotation&`: Rotation const reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">AddRotation</span> - Add rotation (copy version)
  - rotation `const RelativeRotation &`: Rotation object
- <span style="color:#1976d2;font-weight:bold;">AddRotation</span> - Add rotation (move version)
  - rotation `RelativeRotation &&`: Rotation object
- <span style="color:#1976d2;font-weight:bold;">EmplaceRotation</span> - Emplace construct rotation
  - args `Args &&...`: Construction parameters
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - Iterator access
- <span style="color:#1976d2;font-weight:bold;">push_back</span>, <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - Compatibility methods

### `RelativePoses`
(relative-poses)=
Relative pose container class (encapsulation class design, private member variables + public access interfaces)

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get element count
- <span style="color:#1976d2;font-weight:bold;">empty</span> - Check if empty
- <span style="color:#1976d2;font-weight:bold;">clear</span> - Clear all elements
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - Reserve capacity
  - n `size_t`: Reserved size
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (non-const version)
  - index `size_t`: Index
  - Returns `RelativePose*`: Pose pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access (const version)
  - index `size_t`: Index
  - Returns `const RelativePose*`: Pose const pointer, returns nullptr if out of bounds
- <span style="color:#1976d2;font-weight:bold;">GetPose</span> - Get pose (non-const version)
  - index `size_t`: Index
  - Returns `RelativePose&`: Pose reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">GetPose</span> - Get pose (const version)
  - index `size_t`: Index
  - Returns `const RelativePose&`: Pose const reference (throws exception if out of bounds)
- <span style="color:#1976d2;font-weight:bold;">AddPose</span> - Add pose (copy version)
  - pose `const RelativePose &`: Pose object
- <span style="color:#1976d2;font-weight:bold;">AddPose</span> - Add pose (move version)
  - pose `RelativePose &&`: Pose object
- <span style="color:#1976d2;font-weight:bold;">EmplacePose</span> - Emplace construct pose
  - args `Args &&...`: Construction parameters
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - Iterator access
- <span style="color:#1976d2;font-weight:bold;">EvaluateAgainst</span> - Evaluate relative pose accuracy
  - gt_poses `const RelativePoses &`: Ground truth relative poses
  - rotation_errors `std::vector<double> &`: Output rotation errors (angle)
  - translation_errors `std::vector<double> &`: Output translation direction errors (angle)
  - Returns `size_t`: Number of matched pose pairs
- <span style="color:#1976d2;font-weight:bold;">GetRelativePose</span> - Get relative pose based on ViewPair
  - view_pair `const ViewPair &`: View pair (i,j)
  - R `Matrix3d &`: Output rotation matrix
  - t `Vector3d &`: Output translation vector
  - Returns `bool`: Whether corresponding pose was found
- <span style="color:#1976d2;font-weight:bold;">push_back</span>, <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - Compatibility methods

### Relative Pose-Related Type Definitions
(relative-pose-related-types)=
(relative-rotations-ptr)=
(relative-poses-ptr)=

**Type Definitions**:
| Type                   | Definition                                                     | Default Value | Purpose                                            |
| ---------------------- | -------------------------------------------------------------- | ------------- | -------------------------------------------------- |
| `RelativeRotationsPtr` | `std::shared_ptr<`[`RelativeRotations`](relative-rotations)`>` | nullptr       | Smart pointer type for relative rotation container |
| `RelativePosesPtr`     | `std::shared_ptr<`[`RelativePoses`](relative-poses)`>`         | nullptr       | Smart pointer type for relative pose container     |

### Utility Functions
(relative-pose-utils)=

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">Global2RelativePoses</span> - Convert global poses to relative poses
  - global_poses `const GlobalPoses &`: Input global poses
  - relative_poses_output `RelativePoses &`: Output relative poses
  - Returns `bool`: Whether conversion succeeded

---

## Global Poses (types/global_poses.hpp)

### Global Rotation and Translation Types
(global-rotation-translation-types)=
(global-rotations)=
(global-rotations-ptr)=
(global-translations)=
(global-translations-ptr)=

**Type Definitions**:
| Type                    | Definition                                                                                  | Description                                   |
| ----------------------- | ------------------------------------------------------------------------------------------- | --------------------------------------------- |
| `GlobalRotations`       | `std::vector<`[`Matrix3d`](matrix3d)`, Eigen::aligned_allocator<`[`Matrix3d`](matrix3d)`>>` | Global rotation matrix sequence               |
| `GlobalRotationsPtr`    | `std::shared_ptr<`[`GlobalRotations`](global-rotations)`>`                                  | Smart pointer for global rotation sequence    |
| `GlobalTranslations`    | `std::vector<`[`Vector3d`](vector3d)`, Eigen::aligned_allocator<`[`Vector3d`](vector3d)`>>` | Global translation vector sequence            |
| `GlobalTranslationsPtr` | `std::shared_ptr<`[`GlobalTranslations`](global-translations)`>`                            | Smart pointer for global translation sequence |

### `PoseFormat`
(pose-format)=
(rw-tw)=
(rw-tc)=
(rc-tw)=
(rc-tc)=
Pose coordinate system format enumeration defining different camera pose representation methods:

**Format Definitions**:
| Format | Formula                 | Description                                                         |
| ------ | ----------------------- | ------------------------------------------------------------------- |
| `RwTw` | `xc = Rw * (Xw - tw)`   | World-to-camera rotation, camera position in world coordinates      |
| `RwTc` | `xc = Rw * Xw + tc`     | World-to-camera rotation, translation vector in camera coordinates  |
| `RcTw` | `xc = Rc^T * (Xw - tw)` | Camera-to-camera rotation, camera position in world coordinates     |
| `RcTc` | `xc = Rc^T * Xw + tc`   | Camera-to-camera rotation, translation vector in camera coordinates |

### `EstInfo`
(est-info)=
(view-state)=
(invalid-state)=
(valid-state)=
(estimated-state)=
(optimized-state)=
(filtered-state)=
View estimation state information encapsulation class (encapsulation class design, private member variables + public access interfaces), managing view estimation states and ID mapping relationships.

**View State Enumeration (`ViewState`)**:
- `INVALID` - Invalid or uninitialized view
- `VALID` - Valid view not participating in estimation
- `ESTIMATED` - View that has completed estimation
- `OPTIMIZED` - View that has completed optimization
- `FILTERED` - Filtered view (e.g., outliers)

**Private Member Variables**:
| Name             | Type                                       | Default Value | Purpose                                                             |
| ---------------- | ------------------------------------------ | ------------- | ------------------------------------------------------------------- |
| `est2origin_id_` | `std::vector<`[`ViewId`](view-id)`>`       | Empty vector  | Estimated ID to original ID mapping (only includes estimated views) |
| `origin2est_id_` | `std::vector<`[`ViewId`](view-id)`>`       | Empty vector  | Original ID to estimated ID mapping (all views)                     |
| `view_states_`   | `std::vector<`[`ViewState`](view-state)`>` | Empty vector  | View state table (all views)                                        |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">GetEst2OriginId</span> - Get est2origin_id mapping (const version)
  - Returns `const std::vector<ViewId>&`: Const reference to est2origin_id vector
- <span style="color:#1976d2;font-weight:bold;">GetOrigin2EstId</span> - Get origin2est_id mapping (const version)
  - Returns `const std::vector<ViewId>&`: Const reference to origin2est_id vector
- <span style="color:#1976d2;font-weight:bold;">GetViewStates</span> - Get view_states table (const version)
  - Returns `const std::vector<ViewState>&`: Const reference to view_states vector
- <span style="color:#1976d2;font-weight:bold;">Init</span> - Initialize EstInfo
  - num_views `size_t`: Number of views
- <span style="color:#1976d2;font-weight:bold;">AddEstimatedView</span> - Add estimated view
  - original_id `ViewId`: Original view ID
- <span style="color:#1976d2;font-weight:bold;">SetViewState</span> - Set view state
  - original_id `ViewId`: Original view ID
  - state `ViewState`: View state
- <span style="color:#1976d2;font-weight:bold;">GetViewState</span> - Get view state
  - original_id `ViewId`: Original view ID
- <span style="color:#1976d2;font-weight:bold;">IsEstimated</span> - Check if view is estimated
  - original_id `ViewId`: Original view ID
- <span style="color:#1976d2;font-weight:bold;">GetNumEstimatedViews</span> - Get number of estimated views
- <span style="color:#1976d2;font-weight:bold;">GetViewsInState</span> - Get all view IDs in specific state
  - state `ViewState`: View state
- <span style="color:#1976d2;font-weight:bold;">BuildFromTracks</span> - Build EstInfo from Tracks
  - tracks_ptr `TracksPtr`: Track collection smart pointer
  - fixed_id `ViewId`: Fixed view ID
- <span style="color:#1976d2;font-weight:bold;">CollectValidViews</span> - Collect valid view IDs
  - tracks_ptr `TracksPtr`: Track collection smart pointer

### `GlobalPoses`
(global-poses)=
Global pose management class (encapsulation class design, private member variables + public access interfaces), storing global pose information for all views

**Private Member Variables**:
| Name            | Type                                        | Default Value   | Purpose                                |
| --------------- | ------------------------------------------- | --------------- | -------------------------------------- |
| `rotations_`    | [`GlobalRotations`](global-rotations)       | Empty vector    | Rotation matrix array for all views    |
| `translations_` | [`GlobalTranslations`](global-translations) | Empty vector    | Translation vector array for all views |
| `est_info_`     | [`EstInfo`](est-info)                       | Default value   | Estimation state information           |
| `pose_format_`  | [`PoseFormat`](pose-format)                 | [`RwTw`](rw-tw) | Pose format, default is RwTw           |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">GetRotations</span> - Get rotation matrix collection (const version)
  - Returns `const GlobalRotations&`: Const reference to rotation matrix vector
- <span style="color:#1976d2;font-weight:bold;">GetRotations</span> - Get rotation matrix collection (non-const version)
  - Returns `GlobalRotations&`: Reference to rotation matrix vector
- <span style="color:#1976d2;font-weight:bold;">GetTranslations</span> - Get translation vector collection (const version)
  - Returns `const GlobalTranslations&`: Const reference to translation vector
- <span style="color:#1976d2;font-weight:bold;">GetTranslations</span> - Get translation vector collection (non-const version)
  - Returns `GlobalTranslations&`: Reference to translation vector
- <span style="color:#1976d2;font-weight:bold;">Init</span> - Initialize pose data
  - num_views `size_t`: Number of views
- <span style="color:#1976d2;font-weight:bold;">GetPoseFormat</span> - Get current pose format
  - Returns `PoseFormat`: Pose format enumeration value
- <span style="color:#1976d2;font-weight:bold;">SetPoseFormat</span> - Set pose format
  - format `PoseFormat`: Pose format
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - Get view rotation matrix (using original ID)
  - original_id `ViewId`: Original view ID
- <span style="color:#1976d2;font-weight:bold;">GetTranslation</span> - Get view translation vector (using original ID)
  - original_id `ViewId`: Original view ID
- <span style="color:#1976d2;font-weight:bold;">GetRotationByEstId</span> - Get view rotation matrix (using estimated ID)
  - est_id `ViewId`: Estimated view ID
- <span style="color:#1976d2;font-weight:bold;">GetTranslationByEstId</span> - Get view translation vector (using estimated ID)
  - est_id `ViewId`: Estimated view ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - Set view rotation matrix
  - original_id `ViewId`: Original view ID
  - rotation `const Matrix3d&`: Rotation matrix
- <span style="color:#1976d2;font-weight:bold;">SetTranslation</span> - Set view translation vector
  - original_id `ViewId`: Original view ID
  - translation `const Vector3d&`: Translation vector
- <span style="color:#1976d2;font-weight:bold;">SetRotationByEstId</span> - Set rotation using estimated ID
  - est_id `ViewId`: Estimated view ID
  - rotation `const Matrix3d&`: Rotation matrix
- <span style="color:#1976d2;font-weight:bold;">SetTranslationByEstId</span> - Set translation using estimated ID
  - est_id `ViewId`: Estimated view ID
  - translation `const Vector3d&`: Translation vector
- <span style="color:#1976d2;font-weight:bold;">Size</span> - Get pose count
- <span style="color:#1976d2;font-weight:bold;">GetEstInfo</span> - Get EstInfo reference
- <span style="color:#1976d2;font-weight:bold;">BuildEstInfoFromTracks</span> - Build EstInfo from Tracks
  - tracks_ptr `const TracksPtr&`: Track collection smart pointer
  - fixed_id `const ViewId`: Fixed view ID (default 0)
- <span style="color:#1976d2;font-weight:bold;">ConvertPoseFormat</span> - Member function for converting pose data of current object between different pose formats
  - target_format `PoseFormat`: Target pose format
  - ref_id `ViewId`: Reference view ID (default 0)
  - fixed_id `ViewId`: Fixed view ID (default maximum value)
  - Returns `bool`: Whether conversion succeeded
- <span style="color:#1976d2;font-weight:bold;">RwTw_to_RwTc</span> - Member function for converting pose of current object from RwTw to RwTc format
  - ref_id `ViewId`: Reference view ID (default 0)
  - fixed_id `ViewId`: Fixed view ID (default maximum value)
  - Returns `bool`: Whether conversion succeeded
- <span style="color:#1976d2;font-weight:bold;">RwTc_to_RwTw</span> - Member function for converting pose of current object from RwTc to RwTw format
  - ref_id `ViewId`: Reference view ID (default 0)
  - fixed_id `ViewId`: Fixed view ID (default maximum value)
  - Returns `bool`: Whether conversion succeeded
- <span style="color:#1976d2;font-weight:bold;">EvaluateWithSimilarityTransform</span> - Evaluate pose accuracy using similarity transform
  - gt_poses `const GlobalPoses &`: Ground truth global poses
  - rotation_errors `std::vector<double> &`: Output rotation errors (angle)
  - translation_errors `std::vector<double> &`: Output translation errors
  - Returns `bool`: Whether evaluation succeeded
- <span style="color:#1976d2;font-weight:bold;">EvaluateRotations</span> - Evaluate rotation accuracy
  - gt_rotations `const GlobalRotations &`: Ground truth rotation matrices
  - rotation_errors `std::vector<double> &`: Output rotation errors (angle)
  - Returns `bool`: Whether evaluation succeeded
- <span style="color:#1976d2;font-weight:bold;">EvaluateRotationsSVD</span> - Evaluate rotation accuracy using SVD method
  - gt_rotations `const GlobalRotations &`: Ground truth rotation matrices
  - rotation_errors `std::vector<double> &`: Output rotation errors (angle)
  - Returns `bool`: Whether evaluation succeeded
- <span style="color:#1976d2;font-weight:bold;">NormalizeCameraPositions</span> - Normalize camera positions
  - center_before `Vector3d &`: Center position before normalization
  - center_after `Vector3d &`: Center position after normalization
  - Returns `bool`: Whether normalization succeeded
- <span style="color:#1976d2;font-weight:bold;">ComputeCameraPositionCenter</span> - Compute camera position center
  - Returns `Vector3d`: Camera position center vector

**Usage Example**:
```cpp
using namespace PoSDK::types;

// Create global pose object
GlobalPoses global_poses;

// Initialize poses for 3 cameras
global_poses.Init(3);

// Set pose format
global_poses.SetPoseFormat(PoseFormat::RwTw);

// Set first camera pose (reference camera, usually identity matrix)
Matrix3d R0 = Matrix3d::Identity();
Vector3d t0 = Vector3d::Zero();
global_poses.SetRotation(0, R0);
global_poses.SetTranslation(0, t0);

// Set second camera pose
Matrix3d R1;
R1 << 0.9998, -0.0174, 0.0087,
      0.0175,  0.9998, -0.0052,
     -0.0087,  0.0052,  1.0000;
Vector3d t1(1.0, 0.0, 0.0);
global_poses.SetRotation(1, R1);
global_poses.SetTranslation(1, t1);

// Set third camera pose
Matrix3d R2;
R2 << 0.9994, -0.0349, 0.0000,
      0.0349,  0.9994, 0.0000,
      0.0000,  0.0000, 1.0000;
Vector3d t2(2.0, 0.0, 0.0);
global_poses.SetRotation(2, R2);
global_poses.SetTranslation(2, t2);

// Mark views as estimated
global_poses.AddEstimatedView(0);
global_poses.AddEstimatedView(1);
global_poses.AddEstimatedView(2);

// Get pose information
const Matrix3d& R_cam1 = global_poses.GetRotation(1);
const Vector3d& t_cam1 = global_poses.GetTranslation(1);

// Check view estimation status
bool is_estimated = global_poses.IsEstimated(1);
size_t num_estimated = global_poses.GetNumEstimatedViews();

// Get all estimated view IDs
std::vector<ViewId> estimated_views = global_poses.GetViewsInState(EstInfo::ViewState::ESTIMATED);

// Pose format conversion
global_poses.ConvertPoseFormat(PoseFormat::RwTc);

// Direct access to rotation and translation vectors (for batch operations)
GlobalRotations& rotations = global_poses.GetRotations();
GlobalTranslations& translations = global_poses.GetTranslations();

// Iterate through all poses
for (size_t i = 0; i < global_poses.Size(); ++i) {
    if (global_poses.IsEstimated(i)) {
        const Matrix3d& R = global_poses.GetRotation(i);
        const Vector3d& t = global_poses.GetTranslation(i);
        // Process pose data...
    }
}
```

---

## 3D Point Cloud Types (types/world_3dpoints.hpp)

### Basic 3D Point Type Definitions
(basic-3d-point-types)=
(points3d)=
(points3d-ptr)=
(point3d)=

**Type Definitions**:
| Type          | Definition                                  | Default Value | Purpose                                                        |
| ------------- | ------------------------------------------- | ------------- | -------------------------------------------------------------- |
| `Points3d`    | `Eigen::Matrix<double, 3, Eigen::Dynamic>`  | Zero matrix   | 3D point matrix, 3×N matrix, each column represents a 3D point |
| `Points3dPtr` | `std::shared_ptr<`[`Points3d`](points3d)`>` | nullptr       | Smart pointer for 3D point matrix                              |
| `Point3d`     | [`Vector3d`](vector3d)                      | Zero vector   | Single 3D point type                                           |

### `WorldPointInfo`
(world-point-info)=
3D point information encapsulation class in world coordinates, providing private members and public access interfaces

**Private Member Variables**:
| Name            | Type                                  | Default Value | Purpose                                            |
| --------------- | ------------------------------------- | ------------- | -------------------------------------------------- |
| `world_points_` | [`Points3d`](points3d)                | Zero matrix   | World coordinate point collection (3×N matrix)     |
| `ids_used_`     | `std::vector<bool>`                   | Empty vector  | Flag array indicating whether points are used      |
| `colors_rgb_`   | `std::vector<std::array<uint8_t, 3>>` | Empty vector  | RGB color for each point (optional, default empty) |

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">WorldPointInfo</span> - Default constructor
- <span style="color:#1976d2;font-weight:bold;">WorldPointInfo</span> - Constructor initializing specified number of points
  - num_points `size_t`: Number of points
- <span style="color:#1976d2;font-weight:bold;">resize</span> - Resize
  - num_points `size_t`: New number of points
- <span style="color:#1976d2;font-weight:bold;">size</span> - Get point count
- <span style="color:#1976d2;font-weight:bold;">setPoint</span> - Set point coordinates
  - index `size_t`: Point index
  - point `const Point3d &`: 3D point coordinates
- <span style="color:#1976d2;font-weight:bold;">getPoint</span> - Get point coordinates
  - index `size_t`: Point index
- <span style="color:#1976d2;font-weight:bold;">setUsed</span> - Set point usage status
  - index `size_t`: Point index
  - used `bool`: Whether used
- <span style="color:#1976d2;font-weight:bold;">isUsed</span> - Get point usage status
  - index `size_t`: Point index
- <span style="color:#1976d2;font-weight:bold;">getValidPointsCount</span> - Get valid point count
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access returns pointer (const version)
  - index `size_t`: Point index
  - Returns `const Vector3d*`: Point coordinate pointer, returns nullptr if index is invalid or point is unused
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - Array-style access returns pointer (non-const version)
  - index `size_t`: Point index
  - Returns `Vector3d*`: Point coordinate pointer, returns nullptr if index is invalid or point is unused
- <span style="color:#1976d2;font-weight:bold;">GetWorldPoints</span> - Get world coordinate point matrix (const version)
  - Returns `const Points3d&`: Const reference to world coordinate point matrix
- <span style="color:#1976d2;font-weight:bold;">GetWorldPoints</span> - Get world coordinate point matrix (non-const version)
  - Returns `Points3d&`: Reference to world coordinate point matrix
- <span style="color:#1976d2;font-weight:bold;">GetIdsUsed</span> - Get ids_used vector (const version)
  - Returns `const std::vector<bool>&`: Const reference to ids_used vector
- <span style="color:#1976d2;font-weight:bold;">GetIdsUsed</span> - Get ids_used vector (non-const version)
  - Returns `std::vector<bool>&`: Reference to ids_used vector
- <span style="color:#1976d2;font-weight:bold;">HasColors</span> - Check if color information exists
  - Returns `bool`: Whether color information exists
- <span style="color:#1976d2;font-weight:bold;">GetColorCount</span> - Get color count
  - Returns `size_t`: Color count
- <span style="color:#1976d2;font-weight:bold;">GetColorRGB</span> - Get color of specified point
  - index `size_t`: Point index
  - Returns `const std::array<uint8_t, 3>&`: RGB color array
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - Set color of specified point (method 1)
  - index `size_t`: Point index
  - r `uint8_t`: Red component (0-255)
  - g `uint8_t`: Green component (0-255)
  - b `uint8_t`: Blue component (0-255)
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - Set color of specified point (method 2)
  - index `size_t`: Point index
  - rgb `const std::array<uint8_t, 3>&`: RGB color array
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">InitializeColors</span> - Initialize colors for all points
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">ClearColors</span> - Clear all color information
  - Returns `void`
- <span style="color:#1976d2;font-weight:bold;">GetColorsRGB</span> - Get colors_rgb vector (const version)
  - Returns `const std::vector<std::array<uint8_t, 3>>&`: Const reference to colors_rgb vector
- <span style="color:#1976d2;font-weight:bold;">GetColorsRGB</span> - Get colors_rgb vector (non-const version)
  - Returns `std::vector<std::array<uint8_t, 3>>&`: Reference to colors_rgb vector
- <span style="color:#1976d2;font-weight:bold;">EvaluateWith3DPoints</span> - Evaluate 3D point reconstruction accuracy
  - gt_points `const WorldPointInfo &`: Ground truth WorldPointInfo
  - position_errors `std::vector<double> &`: Output position errors
  - Returns `bool`: Whether evaluation succeeded

**Usage Example**:
```cpp
using namespace PoSDK::types;

// Method 1: Set points after default construction
WorldPointInfo point_cloud;
point_cloud.resize(5);  // Initialize 5 points

// Set point coordinates
point_cloud.setPoint(0, Point3d(1.0, 0.0, 0.0));
point_cloud.setPoint(1, Point3d(0.0, 1.0, 0.0));
point_cloud.setPoint(2, Point3d(0.0, 0.0, 1.0));
point_cloud.setPoint(3, Point3d(1.0, 1.0, 0.0));
point_cloud.setPoint(4, Point3d(1.0, 0.0, 1.0));

// Set point usage status
point_cloud.setUsed(0, true);
point_cloud.setUsed(1, true);
point_cloud.setUsed(2, false);  // Mark 3rd point as unused
point_cloud.setUsed(3, true);
point_cloud.setUsed(4, true);

// Method 2: Specify count during construction
WorldPointInfo point_cloud2(10);  // Directly create 10 points (all enabled by default)

// Get point information
size_t total_points = point_cloud.size();              // Total points: 5
size_t valid_points = point_cloud.getValidPointsCount();  // Valid points: 4

// Get and check single point
Point3d point0 = point_cloud.getPoint(0);
bool is_used = point_cloud.isUsed(2);  // false

// Use operator[] for convenient access (recommended when nullptr check is needed)
const Vector3d* pt0 = point_cloud[0];  // Returns pointer, point exists and is used
if (pt0) {
    std::cout << "Point 0 coordinates: " << pt0->transpose() << std::endl;
    double x = (*pt0)(0);  // Access x coordinate
}

const Vector3d* pt2 = point_cloud[2];  // Returns nullptr, point is unused
if (!pt2) {
    std::cout << "Point 2 is unused, returns nullptr" << std::endl;
}

const Vector3d* pt_invalid = point_cloud[100];  // Returns nullptr, index out of bounds
if (!pt_invalid) {
    std::cout << "Index out of bounds, returns nullptr" << std::endl;
}

// Iterate through all valid points (using operator[])
for (size_t i = 0; i < point_cloud.size(); ++i) {
    const Vector3d* pt = point_cloud[i];
    if (pt) {
        // Process valid point...
        double distance = pt->norm();
    }
}

// Iterate through all valid points (traditional method)
for (size_t i = 0; i < point_cloud.size(); ++i) {
    if (point_cloud.isUsed(i)) {
        Point3d point = point_cloud.getPoint(i);
        // Process valid point...
    }
}

// Direct access to underlying data (for batch operations)
const Points3d& points_matrix = point_cloud.GetWorldPoints();  // 3×N matrix
const std::vector<bool>& usage_flags = point_cloud.GetIdsUsed();

// Batch set point coordinates (efficient way - SIMD optimized)
Points3d& points_ref = point_cloud.GetWorldPoints();
for (int i = 0; i < points_ref.cols(); ++i) {
    points_ref.col(i) = Point3d::Random();  // Set random coordinates
}

// Evaluate reconstruction accuracy (compare with ground truth)
WorldPointInfo gt_points(5);
// ... Set ground truth points ...

std::vector<double> position_errors;
bool success = point_cloud.EvaluateWith3DPoints(gt_points, position_errors);
if (success) {
    // Calculate average error
    double avg_error = 0.0;
    for (double err : position_errors) {
        avg_error += err;
    }
    avg_error /= position_errors.size();
}
```

### 3D Point Cloud-Related Type Definitions
(world-point-related-types)=
(world-point-info-ptr)=

**Type Definitions**:
| Type                | Definition                                                | Default Value | Purpose                                              |
| ------------------- | --------------------------------------------------------- | ------------- | ---------------------------------------------------- |
| `WorldPointInfoPtr` | `std::shared_ptr<`[`WorldPointInfo`](world-point-info)`>` | nullptr       | Smart pointer for world coordinate point information |

---

## Similarity Transform (types/similarity_transform.hpp)

### Basic Type Definitions
(similarity-transform-types)=
(cayley-params)=
(quaternion)=

**Type Definitions**:
| Type           | Definition             | Default Value | Purpose                       |
| -------------- | ---------------------- | ------------- | ----------------------------- |
| `CayleyParams` | [`Vector3d`](vector3d) | Zero vector   | Cayley parameters (3D vector) |
| `Quaternion`   | `Eigen::Vector4d`      | Zero vector   | Quaternion (4D vector)        |

### `SimilarityTransformError`
(similarity-transform-error)=
Ceres cost function for similarity transform optimization

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">SimilarityTransformError</span> - Constructor
  - src_point `const Vector3d &`: Source point
  - dst_point `const Vector3d &`: Target point
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - Cost function computation operator
  - scale `const T *const`: Scale parameter
  - rotation `const T *const`: Rotation parameter
  - translation `const T *const`: Translation parameter
  - residuals `T *`: Residual output
- <span style="color:#1976d2;font-weight:bold;">Create</span> - Factory function
  - src_point `const Vector3d &`: Source point
  - dst_point `const Vector3d &`: Target point

### Similarity Transform Template Functions
(similarity-transform-functions)=

**Available Methods**:
- <span style="color:#1976d2;font-weight:bold;">ComputeSimilarityTransform</span> - Template function: Compute optimal similarity transform between two sets of 3D points (using Ceres optimization)
  - src_points `const PointContainer &`: Source point collection
  - dst_points `const PointContainer &`: Target point collection
  - src_points_transformed `PointContainer &`: Transformed source point collection
  - scale `double &`: Output scale factor
  - rotation `Matrix3d &`: Output rotation matrix
  - translation `Vector3d &`: Output translation vector
  - Returns `bool`: Whether succeeded
- <span style="color:#1976d2;font-weight:bold;">ComputeSimilarityTransform</span> - Compute optimal similarity transform between two sets of poses (for backward compatibility)
  - src_centers `const std::vector<Vector3d> &`: Source position vectors
  - dst_centers `const std::vector<Vector3d> &`: Target position vectors
  - src_centers_transformed `std::vector<Vector3d> &`: Transformed source position vectors
  - scale `double &`: Output scale factor
  - rotation `Matrix3d &`: Output rotation matrix
  - translation `Vector3d &`: Output translation vector
  - Returns `bool`: Whether succeeded

### Mathematical Utility Functions
(similarity-transform-math-utils)=

**Available Functions**:
- <span style="color:#1976d2;font-weight:bold;">CayleyToRotation</span> - Convert Cayley parameters to rotation matrix
  - cayley `const CayleyParams &`: Cayley parameters (3D vector)
  - Returns `Matrix3d`: Rotation matrix
- <span style="color:#1976d2;font-weight:bold;">RotationToCayley</span> - Convert rotation matrix to Cayley parameters
  - R `const Matrix3d &`: Rotation matrix
  - Returns `CayleyParams`: Cayley parameters (3D vector)
- <span style="color:#1976d2;font-weight:bold;">RelativePoseToParams</span> - Convert relative pose to parameter vector (6 degrees of freedom: 3 rotation + 3 translation)
  - pose `const RelativePose &`: Relative pose
  - Returns `Eigen::Matrix<double, 6, 1>`: 6D parameter vector (first 3 dimensions are Cayley parameters, last 3 are translation)
- <span style="color:#1976d2;font-weight:bold;">ParamsToRelativePose</span> - Convert parameter vector to relative pose
  - params `const Eigen::Matrix<double, 6, 1> &`: 6D parameter vector (first 3 dimensions are Cayley parameters, last 3 are translation)
  - Returns `RelativePose`: Relative pose

---

