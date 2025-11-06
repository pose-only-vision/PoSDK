# COLMAP File Converter
(COLMAP-converter)=

The COLMAP file converter provides data interoperability with COLMAP SfM tools, supporting reading COLMAP reconstruction results and exporting PoSDK data to COLMAP format.

---

## Conversion Function Overview

### Supported Operations
(COLMAP-supported-operations)=

| Operation Type     | Function Description                    | Conversion Function                                |
| ------------------ | --------------------------------------- | -------------------------------------------------- |
| **COLMAP → PoSDK** | Match data → PoSDK matches              | [`ToDataMatches`](COLMAP-to-data-matches)          |
| **COLMAP → PoSDK** | SfM data → Global poses                 | [`ToDataGlobalPoses`](COLMAP-to-data-global-poses) |
| **PoSDK → COLMAP** | Complete reconstruction → COLMAP format | [`OutputPoSDK2Colmap`](output-posdk-to-COLMAP)     |

---

## COLMAP → PoSDK Conversion

### `ToDataMatches`
(COLMAP-to-data-matches)=

Convert COLMAP match files to PoSDK match data

**Function Signature**:
```cpp
bool ToDataMatches(
    const std::string &matches_folder,
    Interface::DataPtr &matches_data,
    std::map<std::string, int> &file_name_to_id);
```

**Parameter Description**:
| Parameter         | Type                          | Description                        |
| ----------------- | ----------------------------- | ---------------------------------- |
| `matches_folder`  | `const std::string&`          | [Input] COLMAP matches folder path |
| `matches_data`    | `Interface::DataPtr&`         | [Output] PoSDK match data          |
| `file_name_to_id` | `std::map<std::string, int>&` | [Input] File name to ID mapping    |

**Return Value**: `bool` - Whether conversion succeeded

**Input Format**: COLMAP matches folder containing match files in `matches_*.txt` format

**Dependency**: Requires `file_name_to_id` mapping table to map file names to view IDs

**Usage Example**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// Prepare file name to ID mapping (needs to be extracted from COLMAP SfM data)
std::map<std::string, int> file_name_to_id;
// ... Populate file_name_to_id mapping ...

// Load match data
Interface::DataPtr matches_data;
bool success = ToDataMatches(
    "sparse/matches/",
    matches_data,
    file_name_to_id);

if (success) {
    auto matches_ptr = GetDataPtr<types::Matches>(matches_data);
    LOG_INFO_ZH << "Loaded " << matches_ptr->size() << " image pair matches";
}
```

### `ToDataGlobalPoses`
(COLMAP-to-data-global-poses)=

Convert COLMAP SfM data files to PoSDK global pose data

**Function Signature**:
```cpp
bool ToDataGlobalPoses(
    const std::string &global_poses_file,
    Interface::DataPtr &global_poses_data,
    std::map<std::string, int> &file_name_to_id);
```

**Parameter Description**:
| Parameter           | Type                          | Description                          |
| ------------------- | ----------------------------- | ------------------------------------ |
| `global_poses_file` | `const std::string&`          | [Input] COLMAP global pose file path |
| `global_poses_data` | `Interface::DataPtr&`         | [Output] PoSDK global pose data      |
| `file_name_to_id`   | `std::map<std::string, int>&` | [Input] File name to ID mapping      |

**Return Value**: `bool` - Whether conversion succeeded

**Input Format**: COLMAP `images.txt` text file (contains image poses and observation information)

**Output Format**: PoSDK `GlobalPoses`, pose format is `RwTc` (consistent with COLMAP)

**Dependency**: Requires `file_name_to_id` mapping table to map file names to view IDs

**Usage Example**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// Prepare file name to ID mapping (needs to be extracted from COLMAP SfM data)
std::map<std::string, int> file_name_to_id;
// ... Populate file_name_to_id mapping ...

// Load global poses
Interface::DataPtr global_poses_data;
bool success = ToDataGlobalPoses(
    "sparse/images.txt",
    global_poses_data,
    file_name_to_id);

if (success) {
    auto poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
    LOG_INFO_ZH << "Loaded " << poses_ptr->Size() << " camera poses";
}
```

---

## PoSDK → COLMAP Export

### `OutputPoSDK2Colmap`
(output-posdk-to-COLMAP)=

Export PoSDK data to COLMAP format

**Function Signature**:
```cpp
void OutputPoSDK2Colmap(
    const std::string& output_path,
    const types::GlobalPosesPtr& global_poses,
    const types::CameraModelsPtr& camera_models,
    const types::FeaturesInfoPtr& features,
    const types::TracksPtr& tracks,
    const types::Points3dPtr& pts3d);
```

**Parameter Description**:
| Parameter       | Type                            | Description                          |
| --------------- | ------------------------------- | ------------------------------------ |
| `output_path`   | `const std::string&`            | [Input] COLMAP output directory path |
| `global_poses`  | `const types::GlobalPosesPtr&`  | [Input] Global pose data             |
| `camera_models` | `const types::CameraModelsPtr&` | [Input] Camera model data            |
| `features`      | `const types::FeaturesInfoPtr&` | [Input] Feature data                 |
| `tracks`        | `const types::TracksPtr&`       | [Input] Track data                   |
| `pts3d`         | `const types::Points3dPtr&`     | [Input] 3D point data                |

**Output Files**:
```
output_path/
├── cameras.bin              # Camera intrinsics (required)
├── images.bin               # Image poses and observations (required)
├── points3D.bin             # 3D point cloud (required)
└── posdk2colmap_scene.ply   # PLY visualization camera+point cloud file (optional)
└── posdk2colmap_points_only.ply # PLY visualization point cloud file (optional)
```

**Function Description**:
- Automatically performs scene scale normalization (based on minimum inter-camera distance)
- Supports optional PLY file generation (for visualization)

**Usage Example**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// Prepare PoSDK data
types::GlobalPosesPtr global_poses = GetGlobalPoses();
types::CameraModelsPtr camera_models = GetCameraModels();
types::FeaturesInfoPtr features = GetFeatures();
types::TracksPtr tracks = GetTracks();
types::Points3dPtr pts3d = GetPoints3D();

// Export to COLMAP format
std::string colmap_dir = "colmap_export/sparse/0";
std::filesystem::create_directories(colmap_dir);

OutputPoSDK2Colmap(
    colmap_dir,
    global_poses,
    camera_models,
    features,
    tracks,
    pts3d);

LOG_INFO_ZH << "PoSDK data exported to: " << colmap_dir;

// Use COLMAP visualization
// $ COLMAP gui
// Open colmap_export directory
```

---

## COLMAP File Format Support

### `WriteCameras`
(COLMAP-write-cameras)=

Write camera data to binary file

**Function Signature**:
```cpp
void WriteCameras(const std::string& path, 
                  const std::vector<Camera>& cameras);
```

**Camera Structure**:
```cpp
struct Camera {
    uint32_t camera_id;
    int model_id;          // 1=PINHOLE, 2=RADIAL, etc.
    uint64_t width;
    uint64_t height;
    std::vector<double> params;  // fx, fy, cx, cy (PINHOLE)
};
```

**COLMAP Camera Models**:
| Model ID | Name           | Parameters                     |
| -------- | -------------- | ------------------------------ |
| 1        | SIMPLE_PINHOLE | f, cx, cy                      |
| 2        | PINHOLE        | fx, fy, cx, cy                 |
| 3        | SIMPLE_RADIAL  | f, cx, cy, k                   |
| 4        | RADIAL         | f, cx, cy, k1, k2              |
| 5        | OPENCV         | fx, fy, cx, cy, k1, k2, p1, p2 |

### `WriteImages`
(COLMAP-write-images)=

Write image data to binary file

**Function Signature**:
```cpp
void WriteImages(const std::string& path, 
                 const std::vector<Image>& images);
```

**Image Structure**:
```cpp
struct Image {
    uint32_t image_id;
    double qw, qx, qy, qz;  // Rotation quaternion (world-to-camera)
    double tx, ty, tz;       // Translation (world-to-camera)
    uint32_t camera_id;
    std::string name;
    std::vector<std::pair<double, double>> xys;  // 2D observation points
    std::vector<int64_t> point3D_ids;            // Corresponding 3D point IDs
};
```

**Quaternion Convention**: COLMAP uses `[qw, qx, qy, qz]` format with w first

### `WritePoints3D`
(COLMAP-write-points3d)=

Write 3D point data to binary file

**Function Signature**:
```cpp
void WritePoints3D(const std::string& path, 
                   const std::vector<Point3D>& points);
```

**Point3D Structure**:
```cpp
struct Point3D {
    uint64_t point3D_id;
    double x, y, z;
    uint8_t r, g, b;
    double error;
    std::vector<uint32_t> image_ids;      // Image IDs observing this point
    std::vector<uint32_t> point2D_idxs;   // 2D point indices in corresponding images
};
```

---

## Coordinate System Conventions

### PoSDK vs COLMAP Coordinate Conventions
(posdk-vs-COLMAP-conventions)=

| Aspect                  | PoSDK                  | COLMAP                 |
| ----------------------- | ---------------------- | ---------------------- |
| **Pose Representation** | RwTc (World-to-Camera) | RwTc (World-to-Camera) |
| **Rotation Matrix**     | Column-major (Eigen)   | Column-major           |
| **Quaternion**          | `[qw, qx, qy, qz]`     | `[qw, qx, qy, qz]`     |

**Note**: PoSDK and COLMAP use the same pose format (RwTc), no coordinate conversion needed. The `OutputPoSDK2Colmap` function directly uses PoSDK pose data.

---

## Complete Workflow Examples

### COLMAP Result Loading Workflow
(COLMAP-loading-workflow)=

```cpp
#include "converter_colmap_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter::COLMAP;

// COLMAP sparse reconstruction directory structure
// colmap_sparse/
// ├── cameras.bin
// ├── images.bin (or images.txt)
// └── points3D.bin

// ==== Step 1: Prepare file name mapping ====
// Need to extract file name to ID mapping from COLMAP SfM data
std::map<std::string, int> file_name_to_id;
// ... Extract mapping from COLMAP JSON or other methods ...

// ==== Step 2: Load global poses ====
Interface::DataPtr global_poses_data;
bool success = ToDataGlobalPoses(
    "colmap_sparse/images.txt",
    global_poses_data,
    file_name_to_id);

if (!success) {
    LOG_ERROR_ZH << "Failed to load COLMAP pose data";
    return;
}

// ==== Step 3: Load match data (if available) ====
Interface::DataPtr matches_data;
if (std::filesystem::exists("colmap_sparse/matches")) {
    success = ToDataMatches(
        "colmap_sparse/matches",
        matches_data,
        file_name_to_id);
}

// ==== Step 4: Use DataPackage management ====
auto package = std::make_shared<Interface::DataPackage>();
package->AddData(global_poses_data);
if (matches_data) {
    package->AddData(matches_data);
}

auto poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
LOG_INFO_ZH << "Successfully loaded COLMAP reconstruction: " << poses_ptr->Size() << " camera poses";
```

### PoSDK Result Export to COLMAP
(posdk-to-COLMAP-export-workflow)=

```cpp
#include "converter_colmap_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter::COLMAP;

// ==== Step 1: Prepare PoSDK data ====
// Assume complete reconstruction data is available
types::GlobalPosesPtr global_poses = GetGlobalPoses();
types::CameraModelsPtr camera_models = GetCameraModels();
types::FeaturesInfoPtr features = GetFeatures();
types::TracksPtr tracks = GetTracks();
types::Points3dPtr pts3d = GetPoints3D();

LOG_INFO_ZH << "PoSDK reconstruction statistics:";
LOG_INFO_ZH << "  - Cameras: " << global_poses->Size();
LOG_INFO_ZH << "  - Features: " << features->Size();
LOG_INFO_ZH << "  - Tracks: " << tracks->size();
LOG_INFO_ZH << "  - 3D points: " << pts3d->cols();

// ==== Step 2: Create COLMAP output directory ====
std::string colmap_dir = "export_to_colmap/sparse/0";
std::filesystem::create_directories(colmap_dir);

// ==== Step 3: Export to COLMAP format ====
OutputPoSDK2Colmap(
    colmap_dir,
    global_poses,
    camera_models,
    features,
    tracks,
    pts3d);

LOG_INFO_ZH << "PoSDK data exported to COLMAP format";
LOG_INFO_ZH << "Output directory: " << colmap_dir;

// ==== Step 4: Verify export (optional) ====
LOG_INFO_ZH << "Exported files:";
LOG_INFO_ZH << "  - " << colmap_dir << "/cameras.bin";
LOG_INFO_ZH << "  - " << colmap_dir << "/images.bin";
LOG_INFO_ZH << "  - " << colmap_dir << "/points3D.bin";

// ==== Step 5: Use COLMAP tools ====
LOG_INFO_ZH << "Can use COLMAP GUI to view:";
LOG_INFO_ZH << "  $ COLMAP gui";
LOG_INFO_ZH << "  Then open: " << colmap_dir;
```

---

## COLMAP Binary Format Description

### Cameras.bin Format
(COLMAP-cameras-bin-format)=

```
[uint64_t] num_cameras
for each camera:
    [uint32_t] camera_id
    [int] model_id
    [uint64_t] width
    [uint64_t] height
    [double] params[num_params]  // Number of parameters depends on model_id
```

### Images.bin Format
(COLMAP-images-bin-format)=

```
[uint64_t] num_reg_images
for each image:
    [uint32_t] image_id
    [double] qw, qx, qy, qz  // Quaternion
    [double] tx, ty, tz      // Translation
    [uint32_t] camera_id
    [char] image_name[...]   // null-terminated string
    [uint64_t] num_points2D
    for each point2D:
        [double] x, y
        [int64_t] point3D_id  // -1 means no corresponding 3D point
```

### Points3D.bin Format
(COLMAP-points3d-bin-format)=

```
[uint64_t] num_points3D
for each point3D:
    [uint64_t] point3D_id
    [double] xyz[3]
    [uint8_t] rgb[3]
    [double] error
    [uint64_t] track_length
    for each track element:
        [uint32_t] image_id
        [uint32_t] point2D_idx
```

---

## Error Handling

### File Format Validation
```cpp
// Check if file is COLMAP binary format
std::ifstream file(path, std::ios::binary);
if (!file.is_open()) {
    LOG_ERROR_ZH << "Cannot open file: " << path;
    return false;
}

// Read magic number (if any)
// COLMAP binary files have no magic number, need to judge by file extension
```

### Coordinate System Consistency Check
```cpp
// Verify quaternion normalization
double norm = std::sqrt(qw*qw + qx*qx + qy*qy + qz*qz);
if (std::abs(norm - 1.0) > 1e-6) {
    LOG_WARNING_ZH << "Quaternion not normalized: " << norm;
    NormalizeQuaternion(qw, qx, qy, qz);
}
```

---

**Related Links**:
- [Core Data Types - GlobalPoses](../appendices/appendix_a_types.md#global-poses)
- [Converter Overview](index.md)

**External Resources**:
- [COLMAP Official Documentation](https://COLMAP.github.io/)
- [COLMAP Data Format](https://COLMAP.github.io/format.html)


