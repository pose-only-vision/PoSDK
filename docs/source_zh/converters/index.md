# PoSDK 数据转换器

本文档详细介绍了PoSDK提供的各种数据转换器，用于实现PoSDK与其他主流计算机视觉库/工具之间的数据格式转换。

---

## 转换器模块结构

PoSDK转换器采用模块化设计，每个转换器负责与特定的外部库或工具进行数据交互。

### 文件组织
(converter-file-organization)=

```
common/converter/
├── converter_base.hpp         # 转换器基类
├── converter_opencv.hpp/cpp    # OpenCV数据转换器
├── converter_openmvg_file.hpp/cpp # OpenMVG文件转换器
├── converter_opengv.hpp/cpp    # OpenGV数据转换器
├── converter_colmap_file.hpp/cpp  # COLMAP文件转换器
└── CMakeLists.txt             # 构建配置
```

### 模块职责分工
(converter-module-responsibilities)=

| 模块                   | 主要功能                                                                                                                                                           | 文档链接                            |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------- |
| `OpenCVConverter`      | [特征点转换](opencv-feature-conversion)、[描述子转换](opencv-descriptor-conversion)、[匹配转换](opencv-match-conversion)、[相机模型转换](opencv-camera-conversion) | [OpenCV 转换器](opencv-converter)   |
| `OpenMVGFileConverter` | [SfM数据加载](openmvg-sfm-loading)、[特征文件转换](openmvg-feature-conversion)、[匹配文件转换](openmvg-match-conversion)                                           | [OpenMVG 转换器](openmvg-converter) |
| `OpenGVConverter`      | [Bearing向量转换](opengv-bearing-conversion)、[位姿转换](opengv-pose-conversion)、[相机参数转换](opengv-camera-conversion)                                         | [OpenGV 转换器](opengv-converter)   |
| `ColmapConverter`      | [SfM数据转换](COLMAP-to-data-global-poses)、[匹配数据转换](COLMAP-to-data-matches)、[数据导出](output-posdk-to-COLMAP)                                             | [COLMAP 转换器](COLMAP-converter)   |


---

## 转换器功能对比

| 转换功能     | [OpenCV](opencv_converter.md) | [OpenMVG](openmvg_converter.md) | [OpenGV](opengv_converter.md) | [COLMAP](colmap_converter.md) |
| ------------ | ----------------------------- | ------------------------------- | ----------------------------- | ----------------------------- |
| **特征数据** |                               |                                 |                               |                               |
| 特征点       | ✓ 双向转换                    | ✓ 文件读取                      | -                             | -                             |
| 描述子       | ✓ 双向转换                    | -                               | -                             | -                             |
| 匹配         | ✓ 双向转换                    | ✓ 文件读取                      | ✓ 匹配转Bearing               | ✓ 文件读取                    |
| **几何数据** |                               |                                 |                               |                               |
| Bearing向量  | -                             | -                               | ✓ 像素/匹配转换               | -                             |
| 相机模型     | ✓ 标定转换                    | ✓ SfM文件读取                   | ✓ 内参提取                    | ✓ 文件读取/导出               |
| 相对位姿     | -                             | -                               | ✓ 算法结果转换                | -                             |
| 全局位姿     | -                             | ✓ SfM文件读取                   | -                             | ✓ 文件读取                    |
| **3D数据**   |                               |                                 |                               |                               |
| 3D点云       | -                             | ✓ SfM文件读取                   | -                             | ✓ 文件读取/导出               |
| **其他数据** |                               |                                 |                               |                               |
| 图像路径     | -                             | ✓ SfM文件读取                   | -                             | -                             |
| PLY导出      | -                             | -                               | -                             | ✓ 点云/场景导出               |

**图例说明**:
- **✓** : 支持该功能
- **-** : 不支持


---

## 详细文档

转换器使用说明、API参考和示例代码请参阅各子文档：

```{toctree}
:maxdepth: 2

opencv_converter
openmvg_converter
opengv_converter
colmap_converter
```

---

## 快速入门示例

### OpenCV特征提取与转换
```cpp
#include <opencv2/features2d.hpp>
#include "converter_opencv.hpp"

using namespace PoSDK;
using namespace PoSDK::Converter;

// OpenCV特征提取
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

// 转换为PoSDK格式
ImageFeatureInfo image_features;
Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs, 
    image_path, "SIFT");
```

### OpenMVG数据加载
```cpp
#include "converter_openmvg_file.hpp"

using namespace PoSDK::Converter;

// 加载OpenMVG SfM结果
Interface::DataPtr global_poses_data;
OpenMVGFileConverter::ToDataGlobalPoses(
    "sfm_data.json", global_poses_data);
```

### OpenGV位姿估计
```cpp
#include "converter_opengv.hpp"

using namespace PoSDK::Converter;

// 转换匹配为Bearing向量
opengv::bearingVectors_t bearings1, bearings2;
OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);
```

### COLMAP数据加载
```cpp
#include "converter_colmap_file.hpp"

using namespace PoSDK::Converter::COLMAP;

// 1. 创建文件名到ID的映射（从OpenMVG SfM文件）
std::map<std::string, int> file_name_to_id;
SfMFileToIdMap("sfm_data.json", file_name_to_id);

// 2. 加载全局位姿（从COLMAP images.txt文件）
types::GlobalPoses global_poses;
Interface::DataPtr global_poses_data = 
    std::make_shared<Interface::DataMap<types::GlobalPoses>>(global_poses);
ToDataGlobalPoses("images.txt", global_poses_data, file_name_to_id);

// 3. 加载匹配数据（从COLMAP匹配文件夹）
types::Matches matches;
Interface::DataPtr matches_data = 
    std::make_shared<Interface::DataMap<types::Matches>>(matches);
ToDataMatches("matches/", matches_data, file_name_to_id);

```

### 导出为COLMAP数据格式
```cpp
#include "converter_colmap_file.hpp"

using namespace PoSDK::Converter::COLMAP;

// 导出PoSDK结果为COLMAP格式
OutputPoSDK2Colmap(output_path, global_poses, camera_models,
                   features, tracks, pts3d);
```

---

**相关文档**:
- [核心数据类型](../appendices/appendix_a_types.md) - 了解PoSDK数据类型定义
- [插件开发](../basic_development/index.md) - 学习如何开发自定义插件
- [高级功能](../advanced_development/index.md) - 探索高级开发特性

