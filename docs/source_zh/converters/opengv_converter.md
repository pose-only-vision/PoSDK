# OpenGV 数据转换器
(opengv-converter)=

OpenGV数据转换器提供PoSDK与OpenGV库之间的数据转换，用于几何视觉算法和相对位姿估计。

---

## 转换功能概览

### 核心转换功能
(opengv-core-functions)=

| 转换方向           | 数据类型                      | 用途           | 转换函数                                                              |
| ------------------ | ----------------------------- | -------------- | --------------------------------------------------------------------- |
| **PoSDK → OpenGV** | 匹配 → Bearing向量            | 相对位姿估计   | [`MatchesToBearingVectors`](matches-to-bearing-vectors)               |
| **PoSDK → OpenGV** | BearingPairs → BearingVectors | OpenGV算法输入 | [`BearingPairsToBearingVectors`](bearing-pairs-to-bearing-vectors)    |
| **OpenGV → PoSDK** | 变换矩阵 → 相对位姿           | 位姿结果提取   | [`OpenGVPose2RelativePose`](opengv-pose-to-relative-pose)             |
| **PoSDK → OpenGV** | 相机模型 → 内参矩阵           | 算法初始化     | [`CameraModel2OpenGVCalibration`](camera-model-to-opengv-calibration) |

---

## Bearing向量转换
(opengv-bearing-conversion)=

### `MatchesToBearingVectors` (匹配转换)
(matches-to-bearing-vectors)=

将PoSDK匹配和特征数据转换为OpenGV的bearing vectors

**函数签名**:
```cpp
static bool MatchesToBearingVectors(
    const IdMatches &matches,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    opengv::bearingVectors_t &bearingVectors1,
    opengv::bearingVectors_t &bearingVectors2);
```

**参数说明**:
| 参数              | 类型                        | 说明                    |
| ----------------- | --------------------------- | ----------------------- |
| `matches`         | `const IdMatches&`          | [输入] 特征匹配         |
| `features_info`   | `const FeaturesInfo&`       | [输入] 特征信息         |
| `camera_models`   | `const CameraModels&`       | [输入] 相机模型集合     |
| `view_pair`       | `const ViewPair&`           | [输入] 视图对           |
| `bearingVectors1` | `opengv::bearingVectors_t&` | [输出] 第一帧的单位向量 |
| `bearingVectors2` | `opengv::bearingVectors_t&` | [输出] 第二帧的单位向量 |

**返回值**: `bool` - 转换是否成功

**转换流程**:
```
1. 从matches获取特征点对索引
   ↓
2. 从features_info获取像素坐标
   ↓
3. 使用camera_models去畸变并归一化
   ↓
4. 转换为3D单位向量（bearing vectors）
```

**使用示例**:
```cpp
#include "converter_opengv.hpp"
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/relative_pose/methods.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// 1. 准备PoSDK数据
types::IdMatches matches = GetMatches();
types::FeaturesInfo features_info = GetFeaturesInfo();
types::CameraModels camera_models = GetCameraModels();
types::ViewPair view_pair(0, 1);

// 2. 转换为OpenGV bearing vectors
opengv::bearingVectors_t bearings1, bearings2;
bool success = OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);

if (success) {
    // 3. 使用OpenGV进行相对位姿估计
    opengv::relative_pose::CentralRelativeAdapter adapter(
        bearings1, bearings2);
    
    // 5点算法
    opengv::transformation_t T = 
        opengv::relative_pose::fivept_nister(adapter);
    
    LOG_INFO_ZH << "使用了 " << bearings1.size() << " 对bearing vectors";
}
```

### `MatchesToBearingVectors` (样本转换)

将PoSDK匹配样本转换为OpenGV的bearing vectors（用于RANSAC）

**函数签名**:
```cpp
static bool MatchesToBearingVectors(
    const DataSample<IdMatches> &matches_sample,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    opengv::bearingVectors_t &bearingVectors1,
    opengv::bearingVectors_t &bearingVectors2);
```

**使用场景**: RANSAC采样的子集转换

**使用示例**:
```cpp
// RANSAC循环中使用
for (size_t i = 0; i < num_iterations; ++i) {
    // 采样子集
    DataSample<IdMatches> sample = SampleMatches(matches, sample_size);
    
    // 转换采样后的匹配
    opengv::bearingVectors_t bearings1, bearings2;
    OpenGVConverter::MatchesToBearingVectors(
        sample, features_info, camera_models, view_pair,
        bearings1, bearings2);
    
    // OpenGV位姿估计
    opengv::transformation_t T = EstimatePose(bearings1, bearings2);
    
    // 评估内点
    EvaluateInliers(T);
}
```

### `BearingPairsToBearingVectors`
(bearing-pairs-to-bearing-vectors)=

将BearingPairs转换为BearingVectors

**函数签名**:
```cpp
static void BearingPairsToBearingVectors(
    const BearingPairs &bearing_pairs, 
    BearingVectors &bearing_vectors1, 
    BearingVectors &bearing_vectors2);
```

**参数说明**:
| 参数               | 类型                  | 说明                     |
| ------------------ | --------------------- | ------------------------ |
| `bearing_pairs`    | `const BearingPairs&` | [输入] Bearing对集合     |
| `bearing_vectors1` | `BearingVectors&`     | [输出] 第一组Bearing向量 |
| `bearing_vectors2` | `BearingVectors&`     | [输出] 第二组Bearing向量 |

**数据格式说明**:
```cpp
// BearingPairs: std::vector<std::pair<Vector3d, Vector3d>>
// BearingVectors: std::vector<Vector3d>
```

**使用示例**:
```cpp
// PoSDK格式：配对的bearing vectors
types::BearingPairs bearing_pairs = GetBearingPairs();

// 分离为两组向量（OpenGV格式）
types::BearingVectors bearings1, bearings2;
OpenGVConverter::BearingPairsToBearingVectors(
    bearing_pairs, bearings1, bearings2);

// 使用OpenGV Adapter
opengv::relative_pose::CentralRelativeAdapter adapter(
    bearings1, bearings2);
```

### `BearingVectorsToBearingPairs`
(bearing-vectors-to-bearing-pairs)=

将BearingVectors转换为BearingPairs

**函数签名**:
```cpp
static void BearingVectorsToBearingPairs(
    const BearingVectors &bearing_vectors1, 
    const BearingVectors &bearing_vectors2, 
    BearingPairs &bearing_pairs);
```

**使用场景**: OpenGV结果转回PoSDK格式

---

## 位姿转换
(opengv-pose-conversion)=

### `OpenGVPose2RelativePose`
(opengv-pose-to-relative-pose)=

OpenGV位姿结果转换为PoMVG相对位姿

**函数签名**:
```cpp
static bool OpenGVPose2RelativePose(
    const opengv::transformation_t &T,
    RelativePose &relative_pose);
```

**参数说明**:
| 参数            | 类型                              | 说明                         |
| --------------- | --------------------------------- | ---------------------------- |
| `T`             | `const opengv::transformation_t&` | [输入] OpenGV变换矩阵（4×4） |
| `relative_pose` | `RelativePose&`                   | [输出] PoMVG相对位姿         |

**返回值**: `bool` - 转换是否成功

**变换矩阵格式**:
```
OpenGV transformation_t (Eigen::Matrix<double, 3, 4>):
[R | t]  where R是3×3旋转矩阵，t是3×1平移向量
```

**使用示例**:
```cpp
// 1. OpenGV相对位姿估计
opengv::relative_pose::CentralRelativeAdapter adapter(bearings1, bearings2);
opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);

// 2. 转换为PoSDK格式
types::RelativePose relative_pose;
bool success = OpenGVConverter::OpenGVPose2RelativePose(T, relative_pose);

if (success) {
    // 3. 访问相对位姿数据
    const Matrix3d& R = relative_pose.GetRotation();
    const Vector3d& t = relative_pose.GetTranslation();
    
    LOG_INFO_ZH << "相对旋转: \n" << R;
    LOG_INFO_ZH << "相对平移: " << t.transpose();
    
    // 4. 存储或进一步处理
    relative_poses.AddPose(view_pair, relative_pose);
}
```

---

## 相机参数转换
(opengv-camera-conversion)=

### `CameraModel2OpenGVCalibration`
(camera-model-to-opengv-calibration)=

将PoMVG相机内参转换为OpenGV相机参数

**函数签名**:
```cpp
static bool CameraModel2OpenGVCalibration(
    const CameraModel &camera_model,
    Eigen::Matrix3d &K);
```

**参数说明**:
| 参数           | 类型                 | 说明                      |
| -------------- | -------------------- | ------------------------- |
| `camera_model` | `const CameraModel&` | [输入] PoMVG相机模型      |
| `K`            | `Eigen::Matrix3d&`   | [输出] OpenGV相机内参矩阵 |

**返回值**: `bool` - 转换是否成功

**内参矩阵格式**:
```
K = [fx  0  cx]
    [ 0 fy  cy]
    [ 0  0   1]
```

**使用示例**:
```cpp
// PoSDK相机模型
types::CameraModel camera_model = GetCameraModel(view_id);

// 转换为OpenGV内参矩阵
Eigen::Matrix3d K;
bool success = OpenGVConverter::CameraModel2OpenGVCalibration(
    camera_model, K);

if (success) {
    // 使用OpenGV的非中心相机算法
    opengv::relative_pose::NoncentralRelativeAdapter adapter(
        bearings1, bearings2,
        camOffsets1, camOffsets2,
        camRotations1, camRotations2);
    
    // 或者用于验证和调试
    LOG_INFO_ZH << "相机内参矩阵:\n" << K;
}
```

### `PixelToBearingVector`
(pixel-to-bearing-vector)=

将像素坐标转换为bearing vector

**函数签名**:
```cpp
static opengv::bearingVector_t PixelToBearingVector(
    const Vector2d &pixel_coord,
    const CameraModel &camera_model);
```

**参数说明**:
| 参数           | 类型                 | 说明            |
| -------------- | -------------------- | --------------- |
| `pixel_coord`  | `const Vector2d&`    | [输入] 像素坐标 |
| `camera_model` | `const CameraModel&` | [输入] 相机模型 |

**返回值**: `opengv::bearingVector_t` - 3D单位向量

**转换流程**:
```
像素坐标 (u, v)
    ↓ 去畸变
归一化坐标 (x, y)
    ↓ 添加z=1
3D坐标 (x, y, 1)
    ↓ 归一化
单位向量 (x/||·||, y/||·||, 1/||·||)
```

**使用示例**:
```cpp
// 单个像素点转换
Vector2d pixel(320.5, 240.8);
types::CameraModel camera = GetCameraModel(0);

opengv::bearingVector_t bearing = 
    OpenGVConverter::PixelToBearingVector(pixel, camera);

// bearing是单位向量
assert(std::abs(bearing.norm() - 1.0) < 1e-6);

LOG_INFO_ZH << "Bearing向量: " << bearing.transpose();
```

### `IsPixelInImage`

检查像素坐标是否在图像范围内

**函数签名**:
```cpp
static bool IsPixelInImage(
    const Vector2d &pixel_coord,
    const CameraModel &camera_model);
```

**使用示例**:
```cpp
Vector2d pixel(100, 200);
if (OpenGVConverter::IsPixelInImage(pixel, camera_model)) {
    // 像素在图像内，进行转换
    auto bearing = OpenGVConverter::PixelToBearingVector(pixel, camera_model);
} else {
    LOG_WARNING_ZH << "像素坐标超出图像范围";
}
```

---

## 完整工作流示例

### 基于OpenGV的相对位姿估计
(opengv-pose-estimation-workflow)=

```cpp
#include "converter_opengv.hpp"
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/relative_pose/methods.hpp>
#include <opengv/sac/Ransac.hpp>
#include <opengv/sac_problems/relative_pose/CentralRelativePoseSacProblem.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// ==== 步骤1: 准备数据 ====
types::IdMatches matches = GetMatches();
types::FeaturesInfo features_info = GetFeaturesInfo();
types::CameraModels camera_models = GetCameraModels();
types::ViewPair view_pair(0, 1);

// ==== 步骤2: 转换为Bearing向量 ====
opengv::bearingVectors_t bearings1, bearings2;
OpenGVConverter::MatchesToBearingVectors(
    matches, features_info, camera_models, view_pair,
    bearings1, bearings2);

LOG_INFO_ZH << "转换了 " << bearings1.size() << " 对bearing vectors";

// ==== 步骤3: 创建OpenGV Adapter ====
opengv::relative_pose::CentralRelativeAdapter adapter(
    bearings1, bearings2);

// ==== 步骤4: RANSAC位姿估计 ====
typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem 
    SacProblem;

// 创建RANSAC问题
opengv::sac::Ransac<SacProblem> ransac;
std::shared_ptr<SacProblem> problem(
    new SacProblem(adapter, SacProblem::NISTER));

ransac.sac_model_ = problem;
ransac.threshold_ = 1.0 - cos(atan(1.0/800.0));  // 1像素误差阈值
ransac.max_iterations_ = 1000;

// 运行RANSAC
bool ransac_success = ransac.computeModel();

if (ransac_success) {
    LOG_INFO_ZH << "RANSAC成功，内点数: " << ransac.inliers_.size();
    
    // ==== 步骤5: 提取最优模型 ====
    opengv::transformation_t T_ransac = ransac.model_coefficients_;
    
    // ==== 步骤6: 非线性优化（使用所有内点） ====
    adapter.sett12(T_ransac.col(3));
    adapter.setR12(T_ransac.block<3,3>(0,0));
    
    opengv::transformation_t T_optimized = 
        opengv::relative_pose::optimize_nonlinear(adapter, ransac.inliers_);
    
    // ==== 步骤7: 转换为PoSDK格式 ====
    types::RelativePose relative_pose;
    OpenGVConverter::OpenGVPose2RelativePose(T_optimized, relative_pose);
    
    // ==== 步骤8: 存储结果 ====
    relative_poses.AddPose(view_pair, relative_pose);
    
    LOG_INFO_ZH << "相对位姿估计完成";
    LOG_INFO_ZH << "旋转矩阵:\n" << relative_pose.GetRotation();
    LOG_INFO_ZH << "平移向量: " << relative_pose.GetTranslation().transpose();
    
} else {
    LOG_ERROR_ZH << "RANSAC失败，无法估计相对位姿";
}
```

### 5点算法 + 三角化
```cpp
// 1. 5点算法估计本质矩阵
opengv::relative_pose::CentralRelativeAdapter adapter(bearings1, bearings2);
opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);

// 2. 转换为PoSDK格式
types::RelativePose relative_pose;
OpenGVConverter::OpenGVPose2RelativePose(T, relative_pose);

// 3. 使用PoSDK进行三角化
auto triangulator = CreateMethod<Triangulator>();
triangulator->SetRelativePose(relative_pose);
triangulator->SetBearingVectors(bearings1, bearings2);

types::Points3d points_3d = triangulator->Triangulate();

LOG_INFO_ZH << "三角化了 " << points_3d.size() << " 个3D点";
```

---

## OpenGV算法支持

### 相对位姿算法
(opengv-relative-pose-algorithms)=

| 算法                     | OpenGV函数           | 最小点数 | 特点             |
| ------------------------ | -------------------- | -------- | ---------------- |
| **5点算法（Nister）**    | `fivept_nister`      | 5        | 标准算法，最常用 |
| **5点算法（Stewenius）** | `fivept_stewenius`   | 5        | 更数值稳定       |
| **7点算法**              | `sevenpt`            | 7        | 用于基础矩阵     |
| **8点算法**              | `eightpt`            | 8        | 传统算法         |
| **非线性优化**           | `optimize_nonlinear` | ≥5       | 精化位姿         |

### 绝对位姿算法
(opengv-absolute-pose-algorithms)=

| 算法             | OpenGV函数  | 最小点数 | 用途         |
| ---------------- | ----------- | -------- | ------------ |
| **P3P（Kneip）** | `p3p_kneip` | 3        | 最小3点      |
| **P3P（Gao）**   | `p3p_gao`   | 3        | 更快版本     |
| **EPnP**         | `epnp`      | ≥4       | 非迭代，快速 |
| **UPnP**         | `upnp`      | ≥4       | 未标定相机   |

---

---

## 错误处理

### 无效相机模型
```cpp
const CameraModel* camera = camera_models[view_id];
if (!camera) {
    LOG_ERROR_ZH << "无效的相机模型，view_id=" << view_id;
    return false;
}
```

### Bearing向量验证
```cpp
for (const auto& bearing : bearings1) {
    if (std::abs(bearing.norm() - 1.0) > 1e-6) {
        LOG_WARNING_ZH << "Bearing向量未归一化: " << bearing.norm();
    }
}
```

### OpenGV异常捕获
```cpp
try {
    opengv::transformation_t T = opengv::relative_pose::fivept_nister(adapter);
} catch (const std::exception& e) {
    LOG_ERROR_ZH << "OpenGV算法异常: " << e.what();
    return false;
}
```

---

**相关链接**:
- [核心数据类型 - RelativePose](../appendices/appendix_a_types.md#relative-pose)
- [核心数据类型 - BearingVectors](../appendices/appendix_a_types.md#bearing-vectors)
- [转换器总览](index.md)

**外部资源**:
- [OpenGV 官方文档](https://laurentkneip.github.io/opengv/)
- [OpenGV GitHub](https://github.com/laurentkneip/opengv)



