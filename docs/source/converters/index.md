# PoSDK Data Converters

This document details various data converters provided by PoSDK for data format conversion between PoSDK and other mainstream computer vision libraries/tools.

---

## Converter Module Structure

PoSDK converters adopt a modular design, with each converter responsible for data interaction with specific external libraries or tools.

### File Organization
(converter-file-organization)=

```
common/converter/
├── converter_base.hpp         # Converter base class
├── converter_opencv.hpp/cpp    # OpenCV data converter
├── converter_openmvg_file.hpp/cpp # OpenMVG file converter
├── converter_opengv.hpp/cpp    # OpenGV data converter
├── converter_colmap_file.hpp/cpp  # COLMAP file converter
└── CMakeLists.txt             # Build configuration
```

### Module Responsibilities
(converter-module-responsibilities)=

| Module                 | Main Functions                                                                                                                                                                                                 | Documentation Link                     |
| ---------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- |
| `OpenCVConverter`      | [Feature point conversion](opencv-feature-conversion), [Descriptor conversion](opencv-descriptor-conversion), [Match conversion](opencv-match-conversion), [Camera model conversion](opencv-camera-conversion) | [OpenCV Converter](opencv-converter)   |
| `OpenMVGFileConverter` | [SfM data loading](openmvg-sfm-loading), [Feature file conversion](openmvg-feature-conversion), [Match file conversion](openmvg-match-conversion)                                                              | [OpenMVG Converter](openmvg-converter) |
| `OpenGVConverter`      | [Bearing vector conversion](opengv-bearing-conversion), [Pose conversion](opengv-pose-conversion), [Camera parameter conversion](opengv-camera-conversion)                                                     | [OpenGV Converter](opengv-converter)   |
| `ColmapConverter`      | [SfM data conversion](COLMAP-to-data-global-poses), [Match data conversion](COLMAP-to-data-matches), [Data export](output-posdk-to-COLMAP)                                                                     | [COLMAP Converter](COLMAP-converter)   |


---

## Converter Feature Comparison

| Conversion Feature | [OpenCV](opencv_converter.md) | [OpenMVG](openmvg_converter.md) | [OpenGV](opengv_converter.md) | [COLMAP](colmap_converter.md) |
| ------------------ | ----------------------------- | ------------------------------- | ----------------------------- | ----------------------------- |
| **Feature Data**   |                               |                                 |                               |                               |
| Feature points     | ✓ Bidirectional conversion    | ✓ File reading                  | -                             | -                             |
| Descriptors        | ✓ Bidirectional conversion    | -                               | -                             | -                             |
| Matches            | ✓ Bidirectional conversion    | ✓ File reading                  | ✓ Match to Bearing            | ✓ File reading                |
| **Geometric Data** |                               |                                 |                               |                               |
| Bearing vectors    | -                             | -                               | ✓ Pixel/match conversion      | -                             |
| Camera models      | ✓ Calibration conversion      | ✓ SfM file reading              | ✓ Intrinsic extraction        | ✓ File reading/export         |
| Relative poses     | -                             | -                               | ✓ Algorithm result conversion | -                             |
| Global poses       | -                             | ✓ SfM file reading              | -                             | ✓ File reading                |
| **3D Data**        |                               |                                 |                               |                               |
| 3D point clouds    | -                             | ✓ SfM file reading              | -                             | ✓ File reading/export         |
| **Other Data**     |                               |                                 |                               |                               |
| Image paths        | -                             | ✓ SfM file reading              | -                             | -                             |
| PLY export         | -                             | -                               | -                             | ✓ Point cloud/scene export    |

**Legend**:
- **✓** : Feature supported
- **-** : Not supported


---

## Detailed Documentation

For converter usage instructions, API reference, and example code, please refer to the following sub-documents:

```{toctree}
:maxdepth: 2

opencv_converter
openmvg_converter
opengv_converter
colmap_converter
```

---

## Quick Start Examples

### OpenCV Feature Extraction and Conversion
```cpp
#include <opencv2/features2d.hpp>
#include "converter_opencv.hpp"

using namespace PoSDK;
using namespace PoSDK::Converter;

// OpenCV feature extraction
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

// Convert to PoSDK format
ImageFeatureInfo image_features;
Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs, 
    image_path, "SIFT");
```

### OpenMVG Data Loading
```cpp
#include "converter_openmvg_file.hpp"

using namespace PoSDK::Converter;

// Load OpenMVG SfM results
Interface::DataPtr global_poses_data;
OpenMVGFileConverter::ToDataGlobalPoses(
    "sfm_data.json", global_poses_data);
```

### OpenGV Pose Estimation
```cpp
#include "converter_opengv.hpp"

using namespace PoSDK::Converter;

// Convert matches to Bearing vectors
opengv::bearingVectors_t bearings1, bearings2;
OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);
```

### COLMAP Data Loading
```cpp
#include "converter_colmap_file.hpp"

using namespace PoSDK::Converter::COLMAP;

// 1. Create file name to ID mapping (from OpenMVG SfM file)
std::map<std::string, int> file_name_to_id;
SfMFileToIdMap("sfm_data.json", file_name_to_id);

// 2. Load global poses (from COLMAP images.txt file)
types::GlobalPoses global_poses;
Interface::DataPtr global_poses_data = 
    std::make_shared<Interface::DataMap<types::GlobalPoses>>(global_poses);
ToDataGlobalPoses("images.txt", global_poses_data, file_name_to_id);

// 3. Load match data (from COLMAP matches folder)
types::Matches matches;
Interface::DataPtr matches_data = 
    std::make_shared<Interface::DataMap<types::Matches>>(matches);
ToDataMatches("matches/", matches_data, file_name_to_id);

```

### Export to COLMAP Data Format
```cpp
#include "converter_colmap_file.hpp"

using namespace PoSDK::Converter::COLMAP;

// Export PoSDK results to COLMAP format
OutputPoSDK2Colmap(output_path, global_poses, camera_models,
                   features, tracks, pts3d);
```

---

**Related Documentation**:
- [Core Data Types](../appendices/appendix_a_types.md) - Learn PoSDK data type definitions
- [Plugin Development](../basic_development/index.md) - Learn how to develop custom plugins
- [Advanced Features](../advanced_development/index.md) - Explore advanced development features
