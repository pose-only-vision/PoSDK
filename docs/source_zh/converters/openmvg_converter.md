# OpenMVG 文件转换器
(openmvg-converter)=

OpenMVG文件转换器提供直接解析OpenMVG工具输出文件的功能，无需依赖OpenMVG库，实现与OpenMVG数据格式的互操作。

---

## 转换功能概览

### 支持的数据类型
(openmvg-supported-data-types)=

| 数据类型     | OpenMVG格式       | PoSDK类型          | 转换函数                                 |
| ------------ | ----------------- | ------------------ | ---------------------------------------- |
| **特征点**   | `.feat` 二进制    | `ImageFeatureInfo` | [`LoadFeatures`](openmvg-load-features)  |
| **匹配**     | `.matches` 二进制 | `Matches`          | [`LoadMatches`](openmvg-load-matches)    |
| **SfM数据**  | `sfm_data.json`   | `GlobalPoses`      | [`LoadSfMData`](openmvg-load-sfm-data)   |
| **图像信息** | `sfm_data.json`   | `DataImages`       | [`ToDataImages`](openmvg-to-data-images) |

---

## 文件加载函数

### `LoadFeatures`
(openmvg-load-features)=

将OpenMVG特征点文件直接加载到ImageFeatureInfo

**函数签名**:
```cpp
static bool LoadFeatures(
    const std::string &features_file,
    ImageFeatureInfo &image_features);
```

**参数说明**:
| 参数             | 类型                 | 说明                                |
| ---------------- | -------------------- | ----------------------------------- |
| `features_file`  | `const std::string&` | [输入] OpenMVG特征文件路径（.feat） |
| `image_features` | `ImageFeatureInfo&`  | [输出] 图像特征信息                 |

**文件格式**: OpenMVG `.feat` 二进制格式

**使用示例**:
```cpp
#include "converter_openmvg_file.hpp"
using namespace PoSDK::Converter;

// 加载OpenMVG特征文件
types::ImageFeatureInfo image_features;
bool success = OpenMVGFileConverter::LoadFeatures(
    "features/image_0.feat", image_features);

if (success) {
    LOG_INFO_ZH << "加载了 " << image_features.GetNumFeatures() << " 个特征点";
}
```

### `LoadMatches`
(openmvg-load-matches)=

加载OpenMVG匹配文件

**函数签名**:
```cpp
static bool LoadMatches(
    const std::string &matches_file,
    types::Matches &matches);
```

**参数说明**:
| 参数           | 类型                 | 说明                                   |
| -------------- | -------------------- | -------------------------------------- |
| `matches_file` | `const std::string&` | [输入] OpenMVG匹配文件路径（.matches） |
| `matches`      | `types::Matches&`    | [输出] 匹配信息                        |

**文件格式**: OpenMVG `.matches` 二进制格式

**使用示例**:
```cpp
// 加载匹配数据
types::Matches matches;
bool success = OpenMVGFileConverter::LoadMatches(
    "matches/matches_0_1.matches", matches);

if (success) {
    LOG_INFO_ZH << "加载了 " << matches.size() << " 对图像的匹配";
}
```

### `LoadSfMData` (视图信息)
(openmvg-load-sfm-data)=

加载OpenMVG SfM数据文件（视图信息）

**函数签名**:
```cpp
static bool LoadSfMData(
    const std::string &sfm_data_file,
    std::vector<std::pair<types::IndexT, std::string>> &image_paths,
    bool views_only = true);
```

**参数说明**:
| 参数            | 类型                                           | 说明                                    |
| --------------- | ---------------------------------------------- | --------------------------------------- |
| `sfm_data_file` | `const std::string&`                           | [输入] SfM数据文件路径（sfm_data.json） |
| `image_paths`   | `std::vector<std::pair<IndexT, std::string>>&` | [输出] 图像路径（索引+路径）            |
| `views_only`    | `bool`                                         | [输入] 是否只加载视图信息               |

**文件格式**: OpenMVG `sfm_data.json` JSON格式

**使用示例**:
```cpp
// 加载图像路径列表
std::vector<std::pair<types::IndexT, std::string>> image_paths;
bool success = OpenMVGFileConverter::LoadSfMData(
    "sfm_data.json", image_paths, true);

if (success) {
    for (const auto& [view_id, path] : image_paths) {
        LOG_INFO_ZH << "视图 " << view_id << ": " << path;
    }
}
```

### `LoadSfMData` (全局位姿)

加载OpenMVG SfM数据文件中的全局位姿信息

**函数签名**:
```cpp
static bool LoadSfMData(
    const std::string &sfm_data_file,
    types::GlobalPoses &global_poses);
```

**参数说明**:
| 参数            | 类型                  | 说明                   |
| --------------- | --------------------- | ---------------------- |
| `sfm_data_file` | `const std::string&`  | [输入] SfM数据文件路径 |
| `global_poses`  | `types::GlobalPoses&` | [输出] 全局位姿信息    |

**使用示例**:
```cpp
// 加载SfM重建结果
types::GlobalPoses global_poses;
bool success = OpenMVGFileConverter::LoadSfMData(
    "sfm_data.json", global_poses);

if (success) {
    LOG_INFO_ZH << "加载了 " << global_poses.size() << " 个相机位姿";
}
```

---

## 数据转换函数
(openmvg-data-conversion)=

### `ToDataImages`
(openmvg-to-data-images)=

将OpenMVG的SfM数据文件转换为PoSDK图像数据

**函数签名**:
```cpp
static bool ToDataImages(
    const std::string &sfm_data_file,
    const std::string &images_base_dir,
    Interface::DataPtr &images_data);
```

**参数说明**:
| 参数              | 类型                  | 说明                   |
| ----------------- | --------------------- | ---------------------- |
| `sfm_data_file`   | `const std::string&`  | [输入] SfM数据文件路径 |
| `images_base_dir` | `const std::string&`  | [输入] 图像基础目录    |
| `images_data`     | `Interface::DataPtr&` | [输出] 图像数据指针    |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
// 转换为PoSDK图像数据
Interface::DataPtr images_data;
bool success = OpenMVGFileConverter::ToDataImages(
    "sfm_data.json",
    "/path/to/images",
    images_data);

if (success) {
    // 使用DataPackage管理
    auto package = std::make_shared<Interface::DataPackage>();
    package->AddData(images_data);
}
```

### `ToDataFeatures`
(openmvg-to-data-features)=

将OpenMVG的特征文件转换为PoSDK特征数据

**函数签名**:
```cpp
static bool ToDataFeatures(
    const std::string &sfm_data_file,
    const std::string &features_dir,
    const std::string &images_base_dir,
    Interface::DataPtr &features_data);
```

**参数说明**:
| 参数              | 类型                  | 说明                   |
| ----------------- | --------------------- | ---------------------- |
| `sfm_data_file`   | `const std::string&`  | [输入] SfM数据文件路径 |
| `features_dir`    | `const std::string&`  | [输入] 特征文件目录    |
| `images_base_dir` | `const std::string&`  | [输入] 图像基础目录    |
| `features_data`   | `Interface::DataPtr&` | [输出] 特征数据指针    |

**返回值**: `bool` - 转换是否成功

**工作流程**:
1. 从 `sfm_data.json` 读取图像列表
2. 从 `features_dir` 加载每张图像的特征
3. 组合为完整的特征数据集

**使用示例**:
```cpp
// 批量加载OpenMVG特征
Interface::DataPtr features_data;
bool success = OpenMVGFileConverter::ToDataFeatures(
    "sfm_data.json",
    "features/",
    "/path/to/images",
    features_data);

if (success) {
    auto* data_features = static_cast<DataFeatures*>(features_data.get());
    LOG_INFO_ZH << "加载了 " << data_features->GetFeaturesInfo().size() 
                << " 张图像的特征";
}
```

### `ToDataMatches`
(openmvg-to-data-matches)=

将OpenMVG的匹配文件转换为PoSDK匹配数据

**函数签名**:
```cpp
static bool ToDataMatches(
    const std::string &matches_file,
    Interface::DataPtr &matches_data);
```

**参数说明**:
| 参数           | 类型                  | 说明                       |
| -------------- | --------------------- | -------------------------- |
| `matches_file` | `const std::string&`  | [输入] OpenMVG匹配文件路径 |
| `matches_data` | `Interface::DataPtr&` | [输出] 匹配数据指针        |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
// 转换匹配数据
Interface::DataPtr matches_data;
bool success = OpenMVGFileConverter::ToDataMatches(
    "matches.matches", matches_data);

if (success) {
    // 使用PoSDK处理匹配数据
    auto* data_matches = static_cast<DataMatches*>(matches_data.get());
}
```

### `ToDataGlobalPoses`
(openmvg-to-data-global-poses)=

将OpenMVG的SfM数据文件转换为PoSDK全局位姿数据

**函数签名**:
```cpp
static bool ToDataGlobalPoses(
    const std::string &sfm_data_file,
    Interface::DataPtr &global_poses_data);
```

**参数说明**:
| 参数                | 类型                  | 说明                    |
| ------------------- | --------------------- | ----------------------- |
| `sfm_data_file`     | `const std::string&`  | [输入] SfM数据文件路径  |
| `global_poses_data` | `Interface::DataPtr&` | [输出] 全局位姿数据指针 |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
// 加载OpenMVG SfM重建结果
Interface::DataPtr global_poses_data;
bool success = OpenMVGFileConverter::ToDataGlobalPoses(
    "sfm_data.json", global_poses_data);

if (success) {
    // 转换为GlobalPoses类型
    auto* data_global_poses = static_cast<DataGlobalPoses*>(
        global_poses_data.get());
    
    const auto& poses = data_global_poses->GetGlobalPosesConst();
    LOG_INFO_ZH << "加载了 " << poses.size() << " 个全局位姿";
}
```

---

## 完整工作流示例

### OpenMVG数据加载流程
(openmvg-loading-workflow)=

```cpp
#include "converter_openmvg_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// OpenMVG数据目录结构
// openm vg_project/
// ├── sfm_data.json
// ├── features/
// │   ├── image_0.feat
// │   ├── image_1.feat
// │   └── ...
// └── matches/
//     └── matches.matches

// 1. 加载图像信息
Interface::DataPtr images_data;
OpenMVGFileConverter::ToDataImages(
    "openmvg_project/sfm_data.json",
    "/path/to/images",
    images_data);

// 2. 加载特征数据
Interface::DataPtr features_data;
OpenMVGFileConverter::ToDataFeatures(
    "openmvg_project/sfm_data.json",
    "openmvg_project/features/",
    "/path/to/images",
    features_data);

// 3. 加载匹配数据
Interface::DataPtr matches_data;
OpenMVGFileConverter::ToDataMatches(
    "openmvg_project/matches/matches.matches",
    matches_data);

// 4. 加载SfM结果（全局位姿）
Interface::DataPtr global_poses_data;
OpenMVGFileConverter::ToDataGlobalPoses(
    "openmvg_project/sfm_data.json",
    global_poses_data);

// 5. 使用DataPackage统一管理
auto package = std::make_shared<Interface::DataPackage>();
package->AddData(images_data);
package->AddData(features_data);
package->AddData(matches_data);
package->AddData(global_poses_data);

// 6. 保存为PoSDK格式
package->Save("posdk_project.pb");
```

### OpenMVG → PoSDK → 进一步处理
```cpp
// 加载OpenMVG全局位姿
types::GlobalPoses openmvg_poses;
OpenMVGFileConverter::LoadSfMData("sfm_data.json", openmvg_poses);

// 使用PoSDK进行位姿优化
auto optimizer = CreateMethod<GlobalPoseOptimizer>();
optimizer->SetInputPoses(openmvg_poses);
auto optimized_poses = optimizer->Optimize();

// 评估优化效果（如有真值）
if (has_ground_truth) {
    auto evaluator = CreateEvaluator<GlobalPosesEvaluator>();
    auto status = evaluator->Evaluate(optimized_poses, ground_truth);
    
    LOG_INFO_ZH << "旋转误差: " << status.rotation_error << "度";
    LOG_INFO_ZH << "平移误差: " << status.translation_error;
}
```

---

## OpenMVG文件格式说明

### SfM Data JSON格式
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

### Features 二进制格式
(openmvg-features-format)=

```
[uint64_t] num_features
for each feature:
    [float] x
    [float] y
    [float] scale
    [float] orientation
```

### Matches 二进制格式
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

## 错误处理

### 文件不存在
```cpp
if (!std::filesystem::exists(sfm_data_file)) {
    LOG_ERROR_ZH << "SfM数据文件不存在: " << sfm_data_file;
    return false;
}
```

### JSON解析错误
```cpp
try {
    json data = json::parse(file_stream);
} catch (const json::exception& e) {
    LOG_ERROR_ZH << "JSON解析失败: " << e.what();
    return false;
}
```

### 特征文件损坏
```cpp
if (num_features_read != expected_num_features) {
    LOG_WARNING_ZH << "特征文件可能损坏，读取了 " << num_features_read 
                   << " 个特征，期望 " << expected_num_features << " 个";
}
```

---

**相关链接**:
- [核心数据类型 - GlobalPoses](../appendices/appendix_a_types.md#global-poses)
- [核心数据类型 - FeaturesInfo](../appendices/appendix_a_types.md#features-info)
- [转换器总览](index.md)

**外部资源**:
- [OpenMVG 官方文档](https://openmvg.readthedocs.io/)
- [OpenMVG 文件格式说明](https://github.com/openMVG/openMVG/wiki/SfM-Data-File-Format)



