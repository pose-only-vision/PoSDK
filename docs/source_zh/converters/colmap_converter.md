# COLMAP 文件转换器
(COLMAP-converter)=

COLMAP文件转换器提供与COLMAP SfM工具的数据互操作功能，支持读取COLMAP重建结果和导出PoSDK数据为COLMAP格式。

---

## 转换功能概览

### 支持的操作
(COLMAP-supported-operations)=

| 操作类型           | 功能描述              | 转换函数                                           |
| ------------------ | --------------------- | -------------------------------------------------- |
| **COLMAP → PoSDK** | 匹配数据 → PoSDK匹配  | [`ToDataMatches`](COLMAP-to-data-matches)          |
| **COLMAP → PoSDK** | SfM数据 → 全局位姿    | [`ToDataGlobalPoses`](COLMAP-to-data-global-poses) |
| **PoSDK → COLMAP** | 完整重建 → COLMAP格式 | [`OutputPoSDK2Colmap`](output-posdk-to-COLMAP)     |

---

## COLMAP → PoSDK 转换

### `ToDataMatches`
(COLMAP-to-data-matches)=

将COLMAP的匹配文件转换为PoSDK的匹配数据

**函数签名**:
```cpp
bool ToDataMatches(
    const std::string &matches_folder,
    Interface::DataPtr &matches_data,
    std::map<std::string, int> &file_name_to_id);
```

**参数说明**:
| 参数              | 类型                          | 说明                        |
| ----------------- | ----------------------------- | --------------------------- |
| `matches_folder`  | `const std::string&`          | [输入] COLMAP匹配文件夹路径 |
| `matches_data`    | `Interface::DataPtr&`         | [输出] PoSDK匹配数据        |
| `file_name_to_id` | `std::map<std::string, int>&` | [输入] 文件名到ID的映射     |

**返回值**: `bool` - 转换是否成功

**输入格式**: COLMAP匹配文件夹，包含 `matches_*.txt` 格式的匹配文件

**依赖**: 需要提供 `file_name_to_id` 映射表，用于将文件名映射到视图ID

**使用示例**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// 准备文件名到ID的映射（需从COLMAP SfM数据中提取）
std::map<std::string, int> file_name_to_id;
// ... 填充 file_name_to_id 映射 ...

// 加载匹配数据
Interface::DataPtr matches_data;
bool success = ToDataMatches(
    "sparse/matches/",
    matches_data,
    file_name_to_id);

if (success) {
    auto matches_ptr = GetDataPtr<types::Matches>(matches_data);
    LOG_INFO_ZH << "加载了 " << matches_ptr->size() << " 对图像的匹配";
}
```

### `ToDataGlobalPoses`
(COLMAP-to-data-global-poses)=

将COLMAP的SfM数据文件转换为PoSDK的全局位姿数据

**函数签名**:
```cpp
bool ToDataGlobalPoses(
    const std::string &global_poses_file,
    Interface::DataPtr &global_poses_data,
    std::map<std::string, int> &file_name_to_id);
```

**参数说明**:
| 参数                | 类型                          | 说明                          |
| ------------------- | ----------------------------- | ----------------------------- |
| `global_poses_file` | `const std::string&`          | [输入] COLMAP全局位姿文件路径 |
| `global_poses_data` | `Interface::DataPtr&`         | [输出] PoSDK全局位姿数据      |
| `file_name_to_id`   | `std::map<std::string, int>&` | [输入] 文件名到ID的映射       |

**返回值**: `bool` - 转换是否成功

**输入格式**: COLMAP `images.txt` 文本文件（包含图像位姿和观测信息）

**输出格式**: PoSDK `GlobalPoses`，位姿格式为 `RwTc`（与COLMAP一致）

**依赖**: 需要提供 `file_name_to_id` 映射表，用于将文件名映射到视图ID

**使用示例**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// 准备文件名到ID的映射（需从COLMAP SfM数据中提取）
std::map<std::string, int> file_name_to_id;
// ... 填充 file_name_to_id 映射 ...

// 加载全局位姿
Interface::DataPtr global_poses_data;
bool success = ToDataGlobalPoses(
    "sparse/images.txt",
    global_poses_data,
    file_name_to_id);

if (success) {
    auto poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
    LOG_INFO_ZH << "加载了 " << poses_ptr->Size() << " 个相机位姿";
}
```

---

## PoSDK → COLMAP 导出

### `OutputPoSDK2Colmap`
(output-posdk-to-COLMAP)=

导出PoSDK数据为COLMAP格式

**函数签名**:
```cpp
void OutputPoSDK2Colmap(
    const std::string& output_path,
    const types::GlobalPosesPtr& global_poses,
    const types::CameraModelsPtr& camera_models,
    const types::FeaturesInfoPtr& features,
    const types::TracksPtr& tracks,
    const types::Points3dPtr& pts3d);
```

**参数说明**:
| 参数            | 类型                            | 说明                      |
| --------------- | ------------------------------- | ------------------------- |
| `output_path`   | `const std::string&`            | [输入] COLMAP输出目录路径 |
| `global_poses`  | `const types::GlobalPosesPtr&`  | [输入] 全局位姿数据       |
| `camera_models` | `const types::CameraModelsPtr&` | [输入] 相机模型数据       |
| `features`      | `const types::FeaturesInfoPtr&` | [输入] 特征数据           |
| `tracks`        | `const types::TracksPtr&`       | [输入] 轨迹数据           |
| `pts3d`         | `const types::Points3dPtr&`     | [输入] 3D点数据           |

**输出文件**:
```
output_path/
├── cameras.bin              # 相机内参（必需）
├── images.bin               # 图像位姿和观测（必需）
├── points3D.bin             # 3D点云（必需）
└── posdk2colmap_scene.ply   # PLY可视化相机+点云文件（可选）
└── posdk2colmap_points_only.ply # PLY可视化点云文件（可选）
```

**功能说明**:
- 自动执行场景尺度归一化（基于最小相机间距离）
- 支持可选的PLY文件生成（用于可视化）

**使用示例**:
```cpp
#include "converter_colmap_file.hpp"
using namespace PoSDK::Converter::COLMAP;

// 准备PoSDK数据
types::GlobalPosesPtr global_poses = GetGlobalPoses();
types::CameraModelsPtr camera_models = GetCameraModels();
types::FeaturesInfoPtr features = GetFeatures();
types::TracksPtr tracks = GetTracks();
types::Points3dPtr pts3d = GetPoints3D();

// 导出为COLMAP格式
std::string colmap_dir = "colmap_export/sparse/0";
std::filesystem::create_directories(colmap_dir);

OutputPoSDK2Colmap(
    colmap_dir,
    global_poses,
    camera_models,
    features,
    tracks,
    pts3d);

LOG_INFO_ZH << "PoSDK数据已导出到: " << colmap_dir;

// 使用COLMAP可视化
// $ COLMAP gui
// 打开 colmap_export 目录
```

---

## COLMAP文件格式支持

### `WriteCameras`
(COLMAP-write-cameras)=

写入相机数据到二进制文件

**函数签名**:
```cpp
void WriteCameras(const std::string& path, 
                  const std::vector<Camera>& cameras);
```

**Camera结构**:
```cpp
struct Camera {
    uint32_t camera_id;
    int model_id;          // 1=PINHOLE, 2=RADIAL, etc.
    uint64_t width;
    uint64_t height;
    std::vector<double> params;  // fx, fy, cx, cy (PINHOLE)
};
```

**COLMAP相机模型**:
| 模型ID | 名称           | 参数                           |
| ------ | -------------- | ------------------------------ |
| 1      | SIMPLE_PINHOLE | f, cx, cy                      |
| 2      | PINHOLE        | fx, fy, cx, cy                 |
| 3      | SIMPLE_RADIAL  | f, cx, cy, k                   |
| 4      | RADIAL         | f, cx, cy, k1, k2              |
| 5      | OPENCV         | fx, fy, cx, cy, k1, k2, p1, p2 |

### `WriteImages`
(COLMAP-write-images)=

写入图像数据到二进制文件

**函数签名**:
```cpp
void WriteImages(const std::string& path, 
                 const std::vector<Image>& images);
```

**Image结构**:
```cpp
struct Image {
    uint32_t image_id;
    double qw, qx, qy, qz;  // 旋转四元数 (world-to-camera)
    double tx, ty, tz;       // 平移 (world-to-camera)
    uint32_t camera_id;
    std::string name;
    std::vector<std::pair<double, double>> xys;  // 2D观测点
    std::vector<int64_t> point3D_ids;            // 对应的3D点ID
};
```

**四元数约定**: COLMAP使用 `[qw, qx, qy, qz]` 格式，w在前

### `WritePoints3D`
(COLMAP-write-points3d)=

写入3D点数据到二进制文件

**函数签名**:
```cpp
void WritePoints3D(const std::string& path, 
                   const std::vector<Point3D>& points);
```

**Point3D结构**:
```cpp
struct Point3D {
    uint64_t point3D_id;
    double x, y, z;
    uint8_t r, g, b;
    double error;
    std::vector<uint32_t> image_ids;      // 观测到该点的图像ID
    std::vector<uint32_t> point2D_idxs;   // 在对应图像中的2D点索引
};
```

---

## 坐标系约定

### PoSDK vs COLMAP 坐标约定
(posdk-vs-COLMAP-conventions)=

| 方面         | PoSDK                  | COLMAP                 |
| ------------ | ---------------------- | ---------------------- |
| **位姿表示** | RwTc (World-to-Camera) | RwTc (World-to-Camera) |
| **旋转矩阵** | Column-major (Eigen)   | Column-major           |
| **四元数**   | `[qw, qx, qy, qz]`     | `[qw, qx, qy, qz]`     |

**说明**: PoSDK与COLMAP使用相同的位姿格式（RwTc），无需坐标转换。`OutputPoSDK2Colmap` 函数直接使用PoSDK的位姿数据。

---

## 完整工作流示例

### COLMAP结果加载流程
(COLMAP-loading-workflow)=

```cpp
#include "converter_colmap_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter::COLMAP;

// COLMAP sparse重建目录结构
// colmap_sparse/
// ├── cameras.bin
// ├── images.bin (或 images.txt)
// └── points3D.bin

// ==== 步骤1: 准备文件名映射 ====
// 需要从COLMAP SfM数据中提取文件名到ID的映射
std::map<std::string, int> file_name_to_id;
// ... 从COLMAP JSON或其他方式提取映射 ...

// ==== 步骤2: 加载全局位姿 ====
Interface::DataPtr global_poses_data;
bool success = ToDataGlobalPoses(
    "colmap_sparse/images.txt",
    global_poses_data,
    file_name_to_id);

if (!success) {
    LOG_ERROR_ZH << "无法加载COLMAP位姿数据";
    return;
}

// ==== 步骤3: 加载匹配数据（如果有）====
Interface::DataPtr matches_data;
if (std::filesystem::exists("colmap_sparse/matches")) {
    success = ToDataMatches(
        "colmap_sparse/matches",
        matches_data,
        file_name_to_id);
}

// ==== 步骤4: 使用DataPackage管理 ====
auto package = std::make_shared<Interface::DataPackage>();
package->AddData(global_poses_data);
if (matches_data) {
    package->AddData(matches_data);
}

auto poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
LOG_INFO_ZH << "成功加载COLMAP重建: " << poses_ptr->Size() << " 个相机位姿";
```

### PoSDK结果导出为COLMAP
(posdk-to-COLMAP-export-workflow)=

```cpp
#include "converter_colmap_file.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter::COLMAP;

// ==== 步骤1: 准备PoSDK数据 ====
// 假设已有完整的重建数据
types::GlobalPosesPtr global_poses = GetGlobalPoses();
types::CameraModelsPtr camera_models = GetCameraModels();
types::FeaturesInfoPtr features = GetFeatures();
types::TracksPtr tracks = GetTracks();
types::Points3dPtr pts3d = GetPoints3D();

LOG_INFO_ZH << "PoSDK重建统计:";
LOG_INFO_ZH << "  - 相机: " << global_poses->Size();
LOG_INFO_ZH << "  - 特征: " << features->Size();
LOG_INFO_ZH << "  - 轨迹: " << tracks->size();
LOG_INFO_ZH << "  - 3D点: " << pts3d->cols();

// ==== 步骤2: 创建COLMAP输出目录 ====
std::string colmap_dir = "export_to_colmap/sparse/0";
std::filesystem::create_directories(colmap_dir);

// ==== 步骤3: 导出为COLMAP格式 ====
OutputPoSDK2Colmap(
    colmap_dir,
    global_poses,
    camera_models,
    features,
    tracks,
    pts3d);

LOG_INFO_ZH << "PoSDK数据已导出为COLMAP格式";
LOG_INFO_ZH << "输出目录: " << colmap_dir;

// ==== 步骤4: 验证导出（可选）====
LOG_INFO_ZH << "导出文件:";
LOG_INFO_ZH << "  - " << colmap_dir << "/cameras.bin";
LOG_INFO_ZH << "  - " << colmap_dir << "/images.bin";
LOG_INFO_ZH << "  - " << colmap_dir << "/points3D.bin";

// ==== 步骤5: 使用COLMAP工具 ====
LOG_INFO_ZH << "可以使用COLMAP GUI查看:";
LOG_INFO_ZH << "  $ COLMAP gui";
LOG_INFO_ZH << "  然后打开: " << colmap_dir;
```

---

## COLMAP二进制格式说明

### Cameras.bin格式
(COLMAP-cameras-bin-format)=

```
[uint64_t] num_cameras
for each camera:
    [uint32_t] camera_id
    [int] model_id
    [uint64_t] width
    [uint64_t] height
    [double] params[num_params]  // 参数数量取决于model_id
```

### Images.bin格式
(COLMAP-images-bin-format)=

```
[uint64_t] num_reg_images
for each image:
    [uint32_t] image_id
    [double] qw, qx, qy, qz  // 四元数
    [double] tx, ty, tz      // 平移
    [uint32_t] camera_id
    [char] image_name[...]   // null-terminated string
    [uint64_t] num_points2D
    for each point2D:
        [double] x, y
        [int64_t] point3D_id  // -1表示无对应3D点
```

### Points3D.bin格式
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

## 错误处理

### 文件格式验证
```cpp
// 检查文件是否为COLMAP二进制格式
std::ifstream file(path, std::ios::binary);
if (!file.is_open()) {
    LOG_ERROR_ZH << "无法打开文件: " << path;
    return false;
}

// 读取magic number（如果有）
// COLMAP二进制文件没有magic number，需要通过文件扩展名判断
```

### 坐标系一致性检查
```cpp
// 验证四元数归一化
double norm = std::sqrt(qw*qw + qx*qx + qy*qy + qz*qz);
if (std::abs(norm - 1.0) > 1e-6) {
    LOG_WARNING_ZH << "四元数未归一化: " << norm;
    NormalizeQuaternion(qw, qx, qy, qz);
}
```

---

**相关链接**:
- [核心数据类型 - GlobalPoses](../appendices/appendix_a_types.md#global-poses)
- [转换器总览](index.md)

**外部资源**:
- [COLMAP 官方文档](https://COLMAP.github.io/)
- [COLMAP 数据格式](https://COLMAP.github.io/format.html)


