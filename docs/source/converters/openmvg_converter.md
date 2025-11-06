# OpenMVG File Converter
(openmvg-converter)=

The OpenMVG file converter provides direct parsing of OpenMVG tool output files without requiring the OpenMVG library, enabling interoperability with OpenMVG data formats.

---

## Conversion Function Overview

### Supported Data Types
(openmvg-supported-data-types)=

| Data Type          | OpenMVG Format    | PoSDK Type         | Conversion Function                      |
| ------------------ | ----------------- | ------------------ | ---------------------------------------- |
| **Feature Points** | `.feat` binary    | `ImageFeatureInfo` | [`LoadFeatures`](openmvg-load-features)  |
| **Matches**        | `.matches` binary | `Matches`          | [`LoadMatches`](openmvg-load-matches)    |
| **SfM Data**       | `sfm_data.json`   | `GlobalPoses`      | [`LoadSfMData`](openmvg-load-sfm-data)   |
| **Image Info**     | `sfm_data.json`   | `DataImages`       | [`ToDataImages`](openmvg-to-data-images) |

---

## File Loading Functions

### `LoadFeatures`
(openmvg-load-features)=

Load OpenMVG feature point files directly into ImageFeatureInfo

**Function Signature**:
```cpp
static bool LoadFeatures(
    const std::string &features_file,
    ImageFeatureInfo &image_features);
```

**Parameter Description**:
| Parameter        | Type                 | Description                               |
| ---------------- | -------------------- | ----------------------------------------- |
| `features_file`  | `const std::string&` | [Input] OpenMVG feature file path (.feat) |
| `image_features` | `ImageFeatureInfo&`  | [Output] Image feature information        |

**File Format**: OpenMVG `.feat` binary format

**Usage Example**:
```cpp
#include "converter_openmvg_file.hpp"
using namespace PoSDK::Converter;

// Load OpenMVG feature file
types::ImageFeatureInfo image_features;
bool success = OpenMVGFileConverter::LoadFeatures(
    "features/image_0.feat", image_features);

if (success) {
    LOG_INFO_ZH << "Loaded " << image_features.GetNumFeatures() << " feature points";
}
```

### `LoadMatches`
(openmvg-load-matches)=

Load OpenMVG match files

**Function Signature**:
```cpp
static bool LoadMatches(
    const std::string &matches_file,
    types::Matches &matches);
```

**Parameter Description**:
| Parameter      | Type                 | Description                                |
| -------------- | -------------------- | ------------------------------------------ |
| `matches_file` | `const std::string&` | [Input] OpenMVG match file path (.matches) |
| `matches`      | `types::Matches&`    | [Output] Match information                 |

**File Format**: OpenMVG `.matches` binary format

**Usage Example**:
```cpp
// Load match data
types::Matches matches;
bool success = OpenMVGFileConverter::LoadMatches(
    "matches/matches_0_1.matches", matches);

if (success) {
    LOG_INFO_ZH << "Loaded " << matches.size() << " image pair matches";
}
```

### `LoadSfMData` (View Information)
(openmvg-load-sfm-data)=

Load OpenMVG SfM data file (view information)

**Function Signature**:
```cpp
static bool LoadSfMData(
    const std::string &sfm_data_file,
    std::vector<std::pair<types::IndexT, std::string>> &image_paths,
    bool views_only = true);
```

**Parameter Description**:
| Parameter       | Type                                           | Description                                   |
| --------------- | ---------------------------------------------- | --------------------------------------------- |
| `sfm_data_file` | `const std::string&`                           | [Input] SfM data file path (sfm_data.json)    |
| `image_paths`   | `std::vector<std::pair<IndexT, std::string>>&` | [Output] Image paths (index+path)             |
| `views_only`    | `bool`                                         | [Input] Whether to load only view information |

**File Format**: OpenMVG `sfm_data.json` JSON format

**Usage Example**:
```cpp
// Load image path list
std::vector<std::pair<types::IndexT, std::string>> image_paths;
bool success = OpenMVGFileConverter::LoadSfMData(
    "sfm_data.json", image_paths, true);

if (success) {
    for (const auto& [view_id, path] : image_paths) {
        LOG_INFO_ZH << "View " << view_id << ": " << path;
    }
}
```

### `LoadSfMData` (Global Poses)

Load global pose information from OpenMVG SfM data file

**Function Signature**:
```cpp
static bool LoadSfMData(
    const std::string &sfm_data_file,
    types::GlobalPoses &global_poses);
```

**Parameter Description**:
| Parameter       | Type                  | Description                      |
| --------------- | --------------------- | -------------------------------- |
| `sfm_data_file` | `const std::string&`  | [Input] SfM data file path       |
| `global_poses`  | `types::GlobalPoses&` | [Output] Global pose information |

**Usage Example**:
```cpp
// Load SfM reconstruction results
types::GlobalPoses global_poses;
bool success = OpenMVGFileConverter::LoadSfMData(
    "sfm_data.json", global_poses);

if (success) {
    LOG_INFO_ZH << "Loaded " << global_poses.size() << " camera poses";
}
```

---

## Data Conversion Functions
(openmvg-data-conversion)=

### `ToDataImages`
(openmvg-to-data-images)=

Convert OpenMVG SfM data file to PoSDK image data

**Function Signature**:
```cpp
static bool ToDataImages(
    const std::string &sfm_data_file,
    const std::string &images_base_dir,
    Interface::DataPtr &images_data);
```

**Parameter Description**:
| Parameter         | Type                  | Description                  |
| ----------------- | --------------------- | ---------------------------- |
| `sfm_data_file`   | `const std::string&`  | [Input] SfM data file path   |
| `images_base_dir` | `const std::string&`  | [Input] Image base directory |
| `images_data`     | `Interface::DataPtr&` | [Output] Image data pointer  |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
// Convert to PoSDK image data
Interface::DataPtr images_data;
bool success = OpenMVGFileConverter::ToDataImages(
    "sfm_data.json",
    "/path/to/images",
    images_data);

if (success) {
    // Use DataPackage management
    auto package = std::make_shared<Interface::DataPackage>();
    package->AddData(images_data);
}
```

### `ToDataFeatures`
(openmvg-to-data-features)=

Convert OpenMVG feature files to PoSDK feature data

**Function Signature**:
```cpp
static bool ToDataFeatures(
    const std::string &sfm_data_file,
    const std::string &features_dir,
    const std::string &images_base_dir,
    Interface::DataPtr &features_data);
```

**Parameter Description**:
| Parameter         | Type                  | Description                    |
| ----------------- | --------------------- | ------------------------------ |
| `sfm_data_file`   | `const std::string&`  | [Input] SfM data file path     |
| `features_dir`    | `const std::string&`  | [Input] Feature file directory |
| `images_base_dir` | `const std::string&`  | [Input] Image base directory   |
| `features_data`   | `Interface::DataPtr&` | [Output] Feature data pointer  |

**Return Value**: `bool` - Whether conversion succeeded

**Workflow**:
1. Read image list from `sfm_data.json`
2. Load features for each image from `features_dir`
3. Combine into complete feature dataset

**Usage Example**:
```cpp
// Batch load OpenMVG features
Interface::DataPtr features_data;
bool success = OpenMVGFileConverter::ToDataFeatures(
    "sfm_data.json",
    "features/",
    "/path/to/images",
    features_data);

if (success) {
    auto* data_features = static_cast<DataFeatures*>(features_data.get());
    LOG_INFO_ZH << "Loaded features for " << data_features->GetFeaturesInfo().size() 
                << " images";
}
```

### `ToDataMatches`
(openmvg-to-data-matches)=

Convert OpenMVG match files to PoSDK match data

**Function Signature**:
```cpp
static bool ToDataMatches(
    const std::string &matches_file,
    Interface::DataPtr &matches_data);
```

**Parameter Description**:
| Parameter      | Type                  | Description                     |
| -------------- | --------------------- | ------------------------------- |
| `matches_file` | `const std::string&`  | [Input] OpenMVG match file path |
| `matches_data` | `Interface::DataPtr&` | [Output] Match data pointer     |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
// Convert match data
Interface::DataPtr matches_data;
bool success = OpenMVGFileConverter::ToDataMatches(
    "matches.matches", matches_data);

if (success) {
    // Use PoSDK to process match data
    auto* data_matches = static_cast<DataMatches*>(matches_data.get());
}
```

### `ToDataGlobalPoses`
(openmvg-to-data-global-poses)=

Convert OpenMVG SfM data file to PoSDK global pose data

**Function Signature**:
```cpp
static bool ToDataGlobalPoses(
    const std::string &sfm_data_file,
    Interface::DataPtr &global_poses_data);
```

**Parameter Description**:
| Parameter           | Type                  | Description                       |
| ------------------- | --------------------- | --------------------------------- |
| `sfm_data_file`     | `const std::string&`  | [Input] SfM data file path        |
| `global_poses_data` | `Interface::DataPtr&` | [Output] Global pose data pointer |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
// Load OpenMVG SfM reconstruction results
Interface::DataPtr global_poses_data;
bool success = OpenMVGFileConverter::ToDataGlobalPoses(
    "sfm_data.json", global_poses_data);

if (success) {
    // Convert to GlobalPoses type
    auto* data_global_poses = static_cast<DataGlobalPoses*>(
        global_poses_data.get());
    
    const auto& poses = data_global_poses->GetGlobalPosesConst();
    LOG_INFO_ZH << "Loaded " << poses.size() << " global poses";
}
```

---

## Complete Workflow Examples

### OpenMVG Data Loading Workflow
(openmvg-loading-workflow)=

```cpp
#include "converter_openmvg_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// OpenMVG data directory structure
// openmvg_project/
// ├── sfm_data.json
// ├── features/
// │   ├── image_0.feat
// │   ├── image_1.feat
// │   └── ...
// └── matches/
//     └── matches.matches

// 1. Load image information
Interface::DataPtr images_data;
OpenMVGFileConverter::ToDataImages(
    "openmvg_project/sfm_data.json",
    "/path/to/images",
    images_data);

// 2. Load feature data
Interface::DataPtr features_data;
OpenMVGFileConverter::ToDataFeatures(
    "openmvg_project/sfm_data.json",
    "openmvg_project/features/",
    "/path/to/images",
    features_data);

// 3. Load match data
Interface::DataPtr matches_data;
OpenMVGFileConverter::ToDataMatches(
    "openmvg_project/matches/matches.matches",
    matches_data);

// 4. Load SfM results (global poses)
Interface::DataPtr global_poses_data;
OpenMVGFileConverter::ToDataGlobalPoses(
    "openmvg_project/sfm_data.json",
    global_poses_data);

// 5. Use DataPackage for unified management
auto package = std::make_shared<Interface::DataPackage>();
package->AddData(images_data);
package->AddData(features_data);
package->AddData(matches_data);
package->AddData(global_poses_data);

// 6. Save as PoSDK format
package->Save("posdk_project.pb");
```

### OpenMVG → PoSDK → Further Processing
```cpp
// Load OpenMVG global poses
types::GlobalPoses openmvg_poses;
OpenMVGFileConverter::LoadSfMData("sfm_data.json", openmvg_poses);

// Use PoSDK for pose optimization
auto optimizer = CreateMethod<GlobalPoseOptimizer>();
optimizer->SetInputPoses(openmvg_poses);
auto optimized_poses = optimizer->Optimize();

// Evaluate optimization effect (if ground truth available)
if (has_ground_truth) {
    auto evaluator = CreateEvaluator<GlobalPosesEvaluator>();
    auto status = evaluator->Evaluate(optimized_poses, ground_truth);
    
    LOG_INFO_ZH << "Rotation error: " << status.rotation_error << " degrees";
    LOG_INFO_ZH << "Translation error: " << status.translation_error;
}
```

---

## OpenMVG File Format Description

### SfM Data JSON Format
(openmvg-sfm-data-format)=

```json
{
  "sfm_data_version": "0.3",
  "root_path": "/path/to/images",
  "views": [
    {
      "key": 0,
      "value": {
        "polymorphic_id": 1073741824,
        "ptr_wrapper": {
          "id": 0,
          "data": {
            "local_path": "image_0.jpg",
            "filename": "image_0.jpg",
            "width": 1920,
            "height": 1080,
            "id_view": 0,
            "id_intrinsic": 0,
            "id_pose": 0
          }
        }
      }
    }
  ],
  "intrinsics": [...],
  "extrinsics": [...]
}
```

### Features Binary Format
(openmvg-features-format)=

```
[uint64_t] num_features
for each feature:
    [float] x
    [float] y
    [float] scale
    [float] orientation
```

### Matches Binary Format
(openmvg-matches-format)=

```
[uint64_t] num_view_pairs
for each view pair:
    [uint32_t] view_id_1
    [uint32_t] view_id_2
    [uint64_t] num_matches
    for each match:
        [uint32_t] feature_id_1
        [uint32_t] feature_id_2
```

---

## Error Handling

### File Not Found
```cpp
if (!std::filesystem::exists(sfm_data_file)) {
    LOG_ERROR_ZH << "SfM data file does not exist: " << sfm_data_file;
    return false;
}
```

### JSON Parsing Error
```cpp
try {
    json data = json::parse(file_stream);
} catch (const json::exception& e) {
    LOG_ERROR_ZH << "JSON parsing failed: " << e.what();
    return false;
}
```

### Feature File Corruption
```cpp
if (num_features_read != expected_num_features) {
    LOG_WARNING_ZH << "Feature file may be corrupted, read " << num_features_read 
                   << " features, expected " << expected_num_features;
}
```

---

**Related Links**:
- [Core Data Types - GlobalPoses](../appendices/appendix_a_types.md#global-poses)
- [Core Data Types - FeaturesInfo](../appendices/appendix_a_types.md#features-info)
- [Converter Overview](index.md)

**External Resources**:
- [OpenMVG Official Documentation](https://openmvg.readthedocs.io/)
- [OpenMVG File Format Description](https://github.com/openMVG/openMVG/wiki/SfM-Data-File-Format)



