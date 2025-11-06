# PoSDK 核心数据类型

本文档详细介绍了PoSDK中使用的各种核心数据类型，涵盖基础类型、相机模型、特征与匹配、轨迹与观测、位姿表示等关键概念。这些类型构成了整个PoSDK系统的基础数据结构。


---

## 模块化文件结构

为了更好的代码组织和维护，PoSDK采用了模块化的类型系统设计。

### 文件组织
(file-organization)=

```
types/
├── types.hpp              # 主入口文件，包含复杂全局位姿类型
├── base.hpp/cpp           # 基础类型和环境变量管理
├── camera_model.hpp/cpp   # 相机模型相关类型和实现
├── features.hpp/cpp       # 特征检测和管理相关类型
├── matches.hpp/cpp        # 匹配关系和视线向量类型
├── tracks.hpp/cpp         # 轨迹观测和管理相关类型
├── relative_poses.hpp/cpp # 相对位姿和相对旋转类型
├── global_poses.hpp/cpp   # 全局位姿基础类型和估计状态
├── world_3dpoints.hpp/cpp # 3D点云和世界坐标点信息
└── image.hpp              # 图像相关类型定义
```

### 模块职责分工
(module-responsibilities)=

| 模块             | 主要内容                                                                                                 |
| ---------------- | -------------------------------------------------------------------------------------------------------- |
| `base`           | [基础类型](basic-identifier-types)、[方法配置](method-config-types)                                      |
| `camera_model`   | [相机内参](camera-intrinsics)、[畸变模型](distortion-type)、[相机集合管理](camera-models)                |
| `features`       | [特征点](feature-point)、[特征描述](basic-feature-types)、[图像特征管理](features-info)                  |
| `matches`        | [特征匹配](id-match)、[视线向量](bearing-vector-types)、[几何变换工具](bearing-vector-utils)             |
| `tracks`         | [轨迹观测](obs-info)、[3D点管理](track)、[归一化处理](tracks)                                            |
| `relative_poses` | [相对位姿](relative-pose)、[相对旋转](relative-rotation)、[精度评估](relative-poses)                     |
| `global_poses`   | [全局位姿基础类型](global-rotation-translation-types)、[位姿格式](pose-format)、[估计状态管理](est-info) |
| `world_3dpoints` | [3D点云](points3d)、[世界坐标点信息](world-point-info)、[精度评估](world-point-info)                     |
| `types.hpp`      | [复杂全局位姿类](global-poses)、[相似变换](similarity-transform-functions)、统一管理                     |


---

## 快速导航

### 基础类型快速导航
(basic-types-navigation)=

- **标识符类型**: [`IndexT`](index-t) | [`ViewId`](view-id) | [`PtsId`](pts-id) | [`Size`](size-type)
- **数学类型**: [`Matrix3d`](matrix3d) | [`Vector3d`](vector3d) | [`Vector2d`](vector2d) | [`VectorXd`](vectorxd) | [`MatrixXd`](matrixxd)
- **智能指针**: [`Ptr<T>`](ptr-type)
- **方法配置**:  [`MethodsConfig`](methods-config)
- **数据包类型**: [`DataPtr`](ptr-type) | [`Package`](package) | [`DataPackagePtr`](data-package-ptr)

### 相机系统快速导航
(camera-system-navigation)=

- **相机模型**: [`CameraModel`](camera-model) | [`CameraModels`](camera-models) | [`CameraIntrinsics`](camera-intrinsics)
- **畸变类型**: [`DistortionType`](distortion-type) | [`NO_DISTORTION`](no-distortion) | [`RADIAL_K3`](radial-k3) | [`BROWN_CONRADY`](brown-conrady)
- **投影模型**: [`CameraModelType`](camera-model-type) | [`PINHOLE`](pinhole) | [`FISHEYE`](fisheye) | [`SPHERICAL`](spherical)
- **内参矩阵**: [`KMats`](k-mats) | [`KMatsPtr`](k-mats-ptr)

### 图像管理快速导航
(image-system-navigation)=

- **图像路径**: [`ImagePaths`](image-paths) | [`ImagePathsPtr`](image-paths-ptr)


### 特征系统快速导航
(feature-system-navigation)=

- **特征类型**: [`Feature`](feature) | [`FeaturePoint`](feature-point) | [`Descriptor`](descriptor)
- **图像特征**: [`ImageFeatureInfo`](image-feature-info) | [`FeaturesInfo`](features-info) | [`FeaturesInfoPtr`](features-info-ptr)

### 匹配系统快速导航
(matching-system-navigation)=

- **匹配关系**: [`IdMatch`](id-match) | [`IdMatches`](id-matches) | [`ViewPair`](view-pair) | [`Matches`](matches)
- **视线向量**: [`BearingVector`](bearing-vector) | [`BearingVectors`](bearing-vectors) | [`BearingPairs`](bearing-pairs)

### 轨迹系统快速导航
(tracks-system-navigation)=

- **观测数据**: [`ObsInfo`](obs-info) | [`Track`](track) | [`TrackInfo`](track-info)
- **轨迹集合**: [`Tracks`](tracks) | [`TracksPtr`](tracks-ptr)

### 位姿系统快速导航
(pose-system-navigation)=

- **相对位姿**: [`RelativePose`](relative-pose) | [`RelativePoses`](relative-poses) | [`RelativeRotation`](relative-rotation)
- **全局位姿**: [`GlobalPoses`](global-poses) | [`GlobalRotations`](global-rotations) | [`GlobalTranslations`](global-translations)
- **位姿格式**: [`PoseFormat`](pose-format) | [`RwTw`](rw-tw) | [`RwTc`](rw-tc) | [`RcTw`](rc-tw) | [`RcTc`](rc-tc)
- **估计信息**: [`EstInfo`](est-info) | [`ViewState`](view-state)

### 3D点云系统快速导航
(point-cloud-system-navigation)=

- **3D点类型**: [`Point3d`](point3d) | [`Points3d`](points3d) | [`Points3dPtr`](points3d-ptr)
- **世界坐标**: [`WorldPointInfo`](world-point-info) | [`WorldPointInfoPtr`](world-point-info-ptr)

### 数学变换快速导航
(math-transform-navigation)=

- **相似变换**: [`SimilarityTransformError`](similarity-transform-error) | [`CayleyParams`](cayley-params) | [`Quaternion`](quaternion)
- **变换函数**: [相似变换函数](similarity-transform-functions) | [数学工具函数](similarity-transform-math-utils)

---
## 基础类型 (types/base.hpp)

### 基础标识符类型
(ptr-type)=
(basic-identifier-types)=
(index-t)=
(view-id)=
(pts-id)=
(size-type)=

**类型别名定义**:
| 类型别名 | 实际类型             | 默认值 | 用途                                                    |
| -------- | -------------------- | ------ | ------------------------------------------------------- |
| `Ptr<T>` | `std::shared_ptr<T>` | -      | 智能指针类型模板，提供自动内存管理                      |
| `IndexT` | `std::uint32_t`      | 0      | 通用索引类型，用于各种索引标识（最大值：4,294,967,295） |
| `ViewId` | `std::uint32_t`      | 0      | 视图标识符类型，用于标识相机视图                        |
| `PtsId`  | `std::uint32_t`      | 0      | 点/轨迹标识符类型，用于标识3D点或特征轨迹               |
| `Size`   | `std::uint32_t`      | 0      | 尺寸类型，用于表示大小、数量等                          |

### Eigen数学类型
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

**类型别名定义**:
| 类型别名      | 实际类型                      | 默认值 | 用途                                 |
| ------------- | ----------------------------- | ------ | ------------------------------------ |
| `SpMatrix`    | `Eigen::SparseMatrix<double>` | -      | 稀疏矩阵类型，用于大型稀疏线性系统   |
| `Matrix3d`    | `Eigen::Matrix3d`             | 零矩阵 | 3×3双精度矩阵，用于旋转矩阵等变换    |
| `Vector3d`    | `Eigen::Vector3d`             | 零向量 | 3D双精度向量，用于三维坐标和向量     |
| `Vector2d`    | `Eigen::Vector2d`             | 零向量 | 2D双精度向量，用于图像坐标和平面向量 |
| `Vector2f`    | `Eigen::Vector2f`             | 零向量 | 2D单精度向量，用于内存敏感的2D操作   |
| `VectorXd`    | `Eigen::VectorXd`             | 零向量 | 动态大小双精度向量，用于可变长度数据 |
| `RowVectorXd` | `Eigen::RowVectorXd`          | 零向量 | 动态大小行向量，用于矩阵运算         |
| `MatrixXd`    | `Eigen::MatrixXd`             | 零矩阵 | 动态大小矩阵，用于任意大小的矩阵运算 |


### 方法配置类型
(method-config-types)=
(method-type)=
(method-params)=
(params-value)=
(method-options)=
(methods-config)=

**类型别名定义**:
| 类型别名        | 实际类型                                        | 默认值 | 用途                                       |
| --------------- | ----------------------------------------------- | ------ | ------------------------------------------ |
| `MethodType`    | `std::string`                                   | ""     | 方法类型名称，如"SIFT", "ORB"等            |
| `MethodParams`  | `std::string`                                   | ""     | 方法参数名称，如"nfeatures", "threshold"等 |
| `ParamsValue`   | `std::string`                                   | ""     | 参数值类型，以字符串形式保存               |
| `MethodOptions` | `std::unordered_map<MethodParams, ParamsValue>` | 空映射 | 单个方法的所有参数配置                     |
| `MethodsConfig` | `std::unordered_map<MethodType, MethodOptions>` | 空映射 | 所有方法的配置集合                         |

### 数据包类型
(data-package-types)=
(data-type)=
(package)=
(data-package-ptr)=

**类型别名定义**:
| 类型别名         | 实际类型                                | 默认值  | 用途                                 |
| ---------------- | --------------------------------------- | ------- | ------------------------------------ |
| `DataType`       | `std::string`                           | ""      | 数据类型标识符，用于数据包中的键名   |
| `DataPtr`        | `std::shared_ptr<DataIO>`               | nullptr | 数据指针类型，智能指针指向数据IO对象 |
| `Package`        | `std::unordered_map<DataType, DataPtr>` | 空映射  | 基础数据包类型，键值对映射容器       |
| `DataPackagePtr` | `std::shared_ptr<DataPackage>`          | nullptr | 数据包智能指针类型                   |


---

## 相机模型 (types/camera_model.hpp)

相机模型系统提供完整的相机参数管理，支持单相机和多相机配置的智能访问。

### 核心特性与使用模式

**PIMPL设计模式**:
- **二进制兼容性**: 使用PIMPL (Pointer to Implementation) 模式隐藏实现细节，提供稳定的ABI
- **可扩展性**: 便于添加新的相机模型和标定方式，不影响客户端代码
- **数据封装**: 私有成员变量通过std::unique_ptr<Impl>隐藏，提高数据安全性
- **内存对齐**: 使用EIGEN_MAKE_ALIGNED_OPERATOR_NEW宏确保Eigen类型的正确对齐

**智能访问机制**:
- **单相机模式**: 所有视图共享同一内参，任意`view_id`都返回相同模型
- **多相机模式**: 每个视图对应独立内参，按`view_id`索引访问对应模型
- **自动识别**: 系统根据模型数量自动选择访问模式

### 典型使用示例

#### 基本相机模型创建和配置
```cpp
using namespace PoSDK::types;

// 创建相机模型集合
CameraModels cameras;

// 单相机配置（推荐用于标定场景）
CameraModel single_camera;
single_camera.SetCameraIntrinsics(800.0, 800.0, 320.0, 240.0, 640, 480);
cameras.push_back(single_camera);
```

#### 智能访问相机内参（推荐方式）
```cpp
ViewId view_id = 1;

// 智能索引访问（返回nullptr表示失败）
const CameraModel* camera = cameras[view_id];  // 推荐主要访问方式
if (camera) {
    // 获取内参矩阵（高效版本）
    Matrix3d K;
    camera->GetKMat(K);

    // 获取内参矩阵（便利版本）
    Matrix3d K_conv = camera->GetKMat();

    // 通过访问器方法访问参数（推荐方式）
    const auto& intrinsics = camera->GetIntrinsics();
    double fx = intrinsics.GetFx();
    double cy = intrinsics.GetCy();
    uint32_t width = intrinsics.GetWidth();
    uint32_t height = intrinsics.GetHeight();
} else {
    // 处理访问失败
}
```

#### 多相机配置示例
```cpp
// 多相机模式：每个视图独立内参
CameraModels multi_cameras;
for (ViewId i = 0; i < 3; ++i) {
    CameraModel camera;
    // 使用方法4：包含图像尺寸的完整参数设置
    camera.SetCameraIntrinsics(800.0 + i*50, 800.0, 320.0, 240.0, 640, 480);
    multi_cameras.push_back(camera);
}

// 访问特定相机：返回指针，需要检查nullptr
const CameraModel* cam2 = multi_cameras[2];
if (cam2) {
    // 使用第3个相机的参数
    Matrix3d K = cam2->GetKMat();
}
```

#### 坐标转换和畸变处理
```cpp
const CameraModel* camera = cameras[view_id];
if (camera) {
    // 像素坐标与归一化坐标转换
    Vector2d pixel(100.0, 200.0);
    Vector2d normalized = camera->PixelToNormalized(pixel);
    Vector2d back_to_pixel = camera->NormalizedToPixel(normalized);

    // 配置畸变参数
    std::vector<double> radial_dist = {-0.1, 0.05, -0.01};  // k1, k2, k3
    std::vector<double> tangential_dist = {};  // 无切向畸变
    camera->SetDistortionParams(DistortionType::RADIAL_K3, radial_dist, tangential_dist);

    // 设置相机元数据信息
    camera->SetCameraInfo("Canon", "EOS 5D", "123456789");
}
```

### `DistortionType`
(distortion-type)=
(no-distortion)=
(radial-k1)=
(radial-k3)=
(brown-conrady)=
相机畸变类型枚举，支持多种畸变模型

**枚举定义**:
| 类型            | 定义                    | 默认值 | 用途                             |
| --------------- | ----------------------- | ------ | -------------------------------- |
| `NO_DISTORTION` | 无畸变枚举值            | -      | 理想针孔相机模型，无镜头畸变     |
| `RADIAL_K1`     | 一阶径向畸变枚举值      | -      | 简单径向畸变模型，适用于轻微畸变 |
| `RADIAL_K3`     | 三阶径向畸变枚举值      | -      | 完整径向畸变模型，适用于中等畸变 |
| `BROWN_CONRADY` | Brown-Conrady畸变枚举值 | -      | 包含切向畸变的完整畸变模型       |

### `CameraModelType`
(camera-model-type)=
(pinhole)=
(fisheye)=
(spherical)=
(omnidirectional)=
相机模型类型枚举，定义不同的相机投影模型

**枚举定义**:
| 类型              | 定义               | 默认值 | 用途                             |
| ----------------- | ------------------ | ------ | -------------------------------- |
| `PINHOLE`         | 针孔相机模型枚举值 | -      | 标准投影相机模型，适用于常规镜头 |
| `FISHEYE`         | 鱼眼相机模型枚举值 | -      | 广角镜头模型，适用于大视场角相机 |
| `SPHERICAL`       | 球面相机模型枚举值 | -      | 球面投影模型，适用于全景相机     |
| `OMNIDIRECTIONAL` | 全向相机模型枚举值 | -      | 全方位投影模型，适用于360度相机  |

### `CameraIntrinsics`
(camera-intrinsics)=
相机内参数据结构（封装类设计，私有成员变量+公有访问接口）

**实现细节**:
- **PIMPL模式**: 使用`std::unique_ptr<Impl>`隐藏实现细节，提供稳定的ABI
- **内存对齐**: 使用`alignas(8)`优化内存布局，double类型优先对齐
- **默认构造**: 焦距fx/fy默认为1.0，主点cx/cy默认为0.0，图像尺寸默认为0

**内部数据结构** (通过PIMPL隐藏):
| 名称                   | 类型                                   | 默认值        | 用途              |
| ---------------------- | -------------------------------------- | ------------- | ----------------- |
| fx_                    | `double`                               | 1.0           | x方向焦距参数     |
| fy_                    | `double`                               | 1.0           | y方向焦距参数     |
| cx_                    | `double`                               | 0.0           | x方向主点偏移坐标 |
| cy_                    | `double`                               | 0.0           | y方向主点偏移坐标 |
| width_                 | `uint32_t`                             | 0             | 图像分辨率宽度    |
| height_                | `uint32_t`                             | 0             | 图像分辨率高度    |
| model_type_            | [`CameraModelType`](camera-model-type) | PINHOLE       | 相机模型类型标识  |
| distortion_type_       | [`DistortionType`](distortion-type)    | NO_DISTORTION | 相机畸变类型标识  |
| radial_distortion_     | `std::vector<double>`                  | 空向量        | 径向畸变系数数组  |
| tangential_distortion_ | `std::vector<double>`                  | 空向量        | 切向畸变系数数组  |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">CameraIntrinsics</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">GetFx</span> - 获取x方向焦距
- <span style="color:#1976d2;font-weight:bold;">GetFy</span> - 获取y方向焦距
- <span style="color:#1976d2;font-weight:bold;">GetCx</span> - 获取x方向主点偏移
- <span style="color:#1976d2;font-weight:bold;">GetCy</span> - 获取y方向主点偏移
- <span style="color:#1976d2;font-weight:bold;">GetWidth</span> - 获取图像宽度
- <span style="color:#1976d2;font-weight:bold;">GetHeight</span> - 获取图像高度
- <span style="color:#1976d2;font-weight:bold;">GetModelType</span> - 获取相机模型类型
- <span style="color:#1976d2;font-weight:bold;">GetDistortionType</span> - 获取畸变类型
- <span style="color:#1976d2;font-weight:bold;">GetRadialDistortion</span> - 获取径向畸变参数
- <span style="color:#1976d2;font-weight:bold;">GetTangentialDistortion</span> - 获取切向畸变参数
- <span style="color:#1976d2;font-weight:bold;">SetFx</span> - 设置x方向焦距
  - fx `double`：焦距值
- <span style="color:#1976d2;font-weight:bold;">SetFy</span> - 设置y方向焦距
  - fy `double`：焦距值
- <span style="color:#1976d2;font-weight:bold;">SetCx</span> - 设置x方向主点偏移
  - cx `double`：主点偏移值
- <span style="color:#1976d2;font-weight:bold;">SetCy</span> - 设置y方向主点偏移
  - cy `double`：主点偏移值
- <span style="color:#1976d2;font-weight:bold;">SetWidth</span> - 设置图像宽度
  - width `uint32_t`：图像宽度
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetHeight</span> - 设置图像高度
  - height `uint32_t`：图像高度
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetImageSize</span> - 批量设置图像尺寸
  - width `uint32_t`：图像宽度
  - height `uint32_t`：图像高度
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetModelType</span> - 设置相机模型类型
  - type `CameraModelType`：相机模型类型
- <span style="color:#1976d2;font-weight:bold;">SetDistortionType</span> - 设置畸变类型
  - type `DistortionType`：畸变类型
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - 获取相机内参矩阵（高效版本）
  - K `Matrix3d &`：输出的3x3内参矩阵引用
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - 获取相机内参矩阵（便利版本）
  - 返回 `Matrix3d`：3x3内参矩阵
- <span style="color:#1976d2;font-weight:bold;">SetKMat</span> - 设置相机内参矩阵
  - K `const Matrix3d &`：输入的3x3内参矩阵常量引用
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - 设置相机内参数据（方式1）
  - intrinsics `const CameraIntrinsics &`：相机内参结构体
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - 设置相机内参数据（方式2）
  - fx `const double`：x方向焦距
  - fy `const double`：y方向焦距
  - cx `const double`：x方向主点偏移
  - cy `const double`：y方向主点偏移
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - 设置相机内参数据（方式3）
  - fx `const double`：x方向焦距
  - fy `const double`：y方向焦距
  - width `const uint32_t`：图像宽度
  - height `const uint32_t`：图像高度
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - 设置相机内参数据（方式4）
  - fx `const double`：x方向焦距
  - fy `const double`：y方向焦距
  - cx `const double`：x方向主点偏移
  - cy `const double`：y方向主点偏移
  - width `const uint32_t`：图像宽度
  - height `const uint32_t`：图像高度
- <span style="color:#1976d2;font-weight:bold;">SetCameraIntrinsics</span> - 设置相机内参数据（方式5，完整参数）
  - fx `const double`：x方向焦距
  - fy `const double`：y方向焦距
  - cx `const double`：x方向主点偏移
  - cy `const double`：y方向主点偏移
  - width `const uint32_t`：图像宽度
  - height `const uint32_t`：图像高度
  - radial_distortion `const std::vector<double> &`：径向畸变参数
  - tangential_distortion `const std::vector<double> &`：切向畸变参数（可选，默认空）
  - model_type `const CameraModelType`：相机模型类型（默认[针孔相机](pinhole)）
- <span style="color:#1976d2;font-weight:bold;">SetDistortionParams</span> - 设置畸变参数
  - distortion_type `DistortionType`：畸变类型
  - radial_distortion `const std::vector<double> &`：径向畸变参数
  - tangential_distortion `const std::vector<double> &`：切向畸变参数
- <span style="color:#1976d2;font-weight:bold;">InitDistortionParams</span> - 初始化畸变参数为零
- <span style="color:#1976d2;font-weight:bold;">GetFocalLengthPtr</span> - 获取焦距参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">GetPrincipalPointPtr</span> - 获取主点参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">GetDistortionPtr</span> - 获取畸变参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">CopyToParamArray</span> - 复制参数到数组（Ceres优化用）
  - params `double*`：输出参数数组
  - include_distortion `bool`：是否包含畸变参数（默认true）
- <span style="color:#1976d2;font-weight:bold;">SetFromParamArray</span> - 从数组设置参数（Ceres优化用）
  - params `const double*`：输入参数数组
  - include_distortion `bool`：是否包含畸变参数（默认true）

### `CameraModel`
(camera-model)=
相机模型完整定义，包含内参和元数据信息（封装类设计，私有成员变量+公有访问接口）

**实现细节**:
- **PIMPL模式**: 使用`std::unique_ptr<Impl>`隐藏实现细节，提供稳定的ABI
- **内存对齐**: 使用`EIGEN_MAKE_ALIGNED_OPERATOR_NEW`宏确保Eigen类型的正确对齐

**内部数据结构** (通过PIMPL隐藏):
| 名称             | 类型                                    | 默认值 | 用途             |
| ---------------- | --------------------------------------- | ------ | ---------------- |
| `intrinsics_`    | [`CameraIntrinsics`](camera-intrinsics) | 默认值 | 相机内参数据结构 |
| `camera_make_`   | `std::string`                           | ""     | 相机制造商名称   |
| `camera_model_`  | `std::string`                           | ""     | 相机型号标识     |
| `serial_number_` | `std::string`                           | ""     | 相机序列号       |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">GetIntrinsics</span> - 获取相机内参引用（const版本）
  - 返回 `const CameraIntrinsics&`：内参常量引用
- <span style="color:#1976d2;font-weight:bold;">GetIntrinsics</span> - 获取相机内参引用（非const版本）
  - 返回 `CameraIntrinsics&`：内参引用
- <span style="color:#1976d2;font-weight:bold;">GetCameraMake</span> - 获取相机制造商
  - 返回 `const std::string&`：制造商名称
- <span style="color:#1976d2;font-weight:bold;">GetCameraModel</span> - 获取相机型号名称
  - 返回 `const std::string&`：相机型号名称（如"EOS 5D Mark IV"）
- <span style="color:#1976d2;font-weight:bold;">GetSerialNumber</span> - 获取序列号
  - 返回 `const std::string&`：序列号
- <span style="color:#1976d2;font-weight:bold;">SetCameraMake</span> - 设置相机制造商
  - make `const std::string&`：制造商名称
- <span style="color:#1976d2;font-weight:bold;">SetCameraModelName</span> - 设置相机型号名称
  - model `const std::string&`：相机型号名称
- <span style="color:#1976d2;font-weight:bold;">SetSerialNumber</span> - 设置序列号
  - serial `const std::string&`：序列号
- <span style="color:#1976d2;font-weight:bold;">PixelToNormalized</span> - 像素坐标转归一化坐标（单点转换）
  - pixel_coord `const Vector2d &`：像素坐标
  - 返回 `Vector2d`：归一化坐标
- <span style="color:#1976d2;font-weight:bold;">NormalizedToPixel</span> - 归一化坐标转像素坐标（单点转换）
  - normalized_coord `const Vector2d &`：归一化坐标
  - 返回 `Vector2d`：像素坐标
- <span style="color:#1976d2;font-weight:bold;">PixelToNormalizedBatch</span> - 批量像素坐标转归一化坐标（向量化）
  - pixel_coords `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`：2×N像素坐标矩阵
  - 返回 `Eigen::Matrix<double, 2, Eigen::Dynamic>`：2×N归一化坐标矩阵
  - 说明：SIMD友好的批量操作，性能优于循环调用单点转换
- <span style="color:#1976d2;font-weight:bold;">NormalizedToPixelBatch</span> - 批量归一化坐标转像素坐标（向量化）
  - normalized_coords `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`：2×N归一化坐标矩阵
  - 返回 `Eigen::Matrix<double, 2, Eigen::Dynamic>`：2×N像素坐标矩阵
  - 说明：SIMD友好的批量操作，性能优于循环调用单点转换
- <span style="color:#1976d2;font-weight:bold;">SetKMat</span> - 设置相机内参矩阵
  - K `const Matrix3d &`：输入的3x3内参矩阵
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - 获取相机内参矩阵（高效版本）
  - K `Matrix3d &`：输出的3x3内参矩阵引用
- <span style="color:#1976d2;font-weight:bold;">GetKMat</span> - 获取相机内参矩阵（便利版本）
  - 返回 `Matrix3d`：3x3内参矩阵
- <span style="color:#1976d2;font-weight:bold;">SetDistortionParams</span> - 设置畸变类型和参数
  - distortion_type `const DistortionType &`：畸变类型
  - radial_distortion `const std::vector<double> &`：径向畸变参数
  - tangential_distortion `const std::vector<double> &`：切向畸变参数
- <span style="color:#1976d2;font-weight:bold;">SetModelType</span> - 设置相机模型类型
  - model_type `const CameraModelType &`：相机模型类型
- <span style="color:#1976d2;font-weight:bold;">SetCameraInfo</span> - 设置相机元数据信息
  - camera_make `const std::string &`：相机制造商
  - camera_model `const std::string &`：相机型号
  - serial_number `const std::string &`：序列号
- 其他SetCameraIntrinsics方法 - 与[`CameraIntrinsics`](camera-intrinsics)类相同的多种重载方法
- <span style="color:#1976d2;font-weight:bold;">GetFocalLengthPtr</span> - 获取焦距参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">GetPrincipalPointPtr</span> - 获取主点参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">GetDistortionPtr</span> - 获取畸变参数指针（Ceres优化用）
- <span style="color:#1976d2;font-weight:bold;">CopyToParamArray</span> - 复制参数到数组（Ceres优化用）
  - params `double*`：输出参数数组
  - include_distortion `bool`：是否包含畸变参数（默认true）
- <span style="color:#1976d2;font-weight:bold;">SetFromParamArray</span> - 从数组设置参数（Ceres优化用）
  - params `const double*`：输入参数数组
  - include_distortion `bool`：是否包含畸变参数（默认true）

### `CameraModels`
(camera-models)=
智能相机模型容器类，支持单相机和多相机配置，继承自std::vector<[`CameraModel`](camera-model)>

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 智能索引访问相机模型（非const版本）
  - view_id `ViewId`：视图标识符
  - 返回 `CameraModel*`：相机模型指针，访问失败时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 智能索引访问相机模型（const版本）
  - view_id `ViewId`：视图标识符
  - 返回 `const CameraModel*`：相机模型常量指针，访问失败时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">at</span> - 智能索引访问相机模型（非const版本，带边界检查）
  - view_id `ViewId`：视图标识符
  - 返回 `CameraModel&`：相机模型引用
  - 抛出异常：`std::out_of_range`（访问无效时）
- <span style="color:#1976d2;font-weight:bold;">at</span> - 智能索引访问相机模型（const版本，带边界检查）
  - view_id `ViewId`：视图标识符
  - 返回 `const CameraModel&`：相机模型常量引用
  - 抛出异常：`std::out_of_range`（访问无效时）
- 继承自`std::vector<CameraModel>`的所有标准容器方法（`size`, `empty`, `push_back`, `clear`等）


### 相机模型相关类型定义
(camera-model-related-types)=
(camera-model-ptr)=
(camera-models-ptr)=
(k-mats)=
(k-mats-ptr)=

**类型定义**:
| 类型              | 定义                                                                                        | 默认值  | 用途                             |
| ----------------- | ------------------------------------------------------------------------------------------- | ------- | -------------------------------- |
| `CameraModelPtr`  | `std::shared_ptr<`[`CameraModel`](camera-model)`>`                                          | nullptr | 相机模型智能指针类型             |
| `CameraModelsPtr` | `std::shared_ptr<`[`CameraModels`](camera-models)`>`                                        | nullptr | 相机模型集合智能指针类型         |
| `KMats`           | `std::vector<`[`Matrix3d`](matrix3d)`, Eigen::aligned_allocator<`[`Matrix3d`](matrix3d)`>>` | 空向量  | 内参矩阵序列容器，使用对齐分配器 |
| `KMatsPtr`        | `std::shared_ptr<`[`KMats`](k-mats)`>`                                                      | nullptr | 内参矩阵序列智能指针             |

---

## 图像管理 (types/image.hpp)

### 图像路径类型定义
(image-path-types)=
(image-paths)=
(image-paths-ptr)=

**类型定义**:
| 类型            | 定义                                             | 默认值  | 用途                                 |
| --------------- | ------------------------------------------------ | ------- | ------------------------------------ |
| `ImagePaths`    | `std::vector<std::pair<std::string, bool>>`      | 空向量  | 存储图像路径及其有效性状态的类型定义 |
| `ImagePathsPtr` | `std::shared_ptr<`[`ImagePaths`](image-paths)`>` | nullptr | 图像路径集合的智能指针类型           |

---

## 特征 (types/features.hpp)

特征系统采用SIMD优化的SOA (Structure of Arrays) 设计，提供高性能的特征数据管理。

### 核心特性与使用模式

**SIMD优化的SOA设计优势**:
- **SOA内存布局**: 数据按类型分组存储，提高缓存局部性和SIMD向量化效率
- **AVX2/AVX-512支持**: 批量操作完全向量化，显著提升性能
- **自然对齐优化**: 移除过度64字节对齐，减少90%+内存占用
- **批量优先API**: 提供高效的批量操作接口（SetSizesBlock, SetAnglesBlock等）
- **零拷贝兼容**: 与cv::Mat和protobuf零拷贝兼容，8-9x加速转换
- **紧凑布局**: 单次连续内存分配，减少内存碎片

### 典型使用示例

#### FeaturePoints批量操作（高性能）
```cpp
using namespace PoSDK::types;

// 创建特征点集合（推荐预留容量）
FeaturePoints features(1000);  // 预留1000个特征点

// 批量添加特征（高效）
Eigen::Matrix<double, 2, Eigen::Dynamic> coords_block(2, 100);
std::vector<float> sizes_block(100, 5.0f);
std::vector<float> angles_block(100, 0.0f);
// ... 填充coords_block, sizes_block, angles_block ...
features.AddFeaturesBlock(coords_block, sizes_block.data(), angles_block.data());

// 访问单个特征点
Feature coord = features.GetCoord(0);
float size = features.GetSize(0);
float angle = features.GetAngle(0);

// 批量设置属性（SIMD优化）
features.SetSizesBlock(0, 100, sizes_block.data());
features.SetAnglesBlock(0, 100, angles_block.data());

// 批量统计操作（SIMD优化）
features.IncrementTotalCountsBlock(0, 100);
features.IncrementOutlierCountsBlock(0, 50);

// 清零计数器（SIMD优化）
features.ZeroCounters();

// 获取有效特征数量（SIMD优化）
Size valid_count = features.GetNumValid();

// 迭代器访问
for (auto&& feat_proxy : features) {
    if (feat_proxy.IsUsed()) {
        Feature coord = feat_proxy.GetCoord();
        // 处理特征
    }
}
```

#### 描述子管理（SOA格式）
```cpp
// 创建SOA描述子容器（高性能）
DescriptorsSOA descs_soa(1000, 128);  // 1000个特征，128维SIFT描述子

// 设置单个描述子
float descriptor_data[128];
// ... 填充descriptor_data ...
descs_soa.SetDescriptor(0, descriptor_data);

// 直接访问描述子数据（零拷贝）
float* desc_ptr = descs_soa[0];  // 指向第0个描述子的指针

// 批量拷贝（SIMD优化）
float* all_descs = descs_soa.data();  // 原始数据指针
size_t total_size = descs_soa.total_size();  // 总元素数

// 从旧版AOS格式转换（自动）
Descs old_format_descs;  // std::vector<std::vector<float>>
descs_soa.FromDescs(old_format_descs);

// 转回AOS格式（兼容性）
Descs converted_descs;
descs_soa.ToDescs(converted_descs);
```

#### 图像特征信息管理
```cpp
// 创建图像特征信息
ImageFeatureInfo image_features("image1.jpg", true);

// 预留容量（推荐）
image_features.ReserveFeatures(1000);

// 添加单个特征点
image_features.AddFeature(100.0f, 200.0f, 5.0f, 45.0f);

// 批量添加特征（高效）
Eigen::Matrix<double, 2, Eigen::Dynamic> coords_block(2, 100);
std::vector<float> sizes_block(100, 5.0f);
std::vector<float> angles_block(100, 0.0f);
// ... 填充数据 ...
image_features.AddFeaturesBlock(coords_block, sizes_block.data(), angles_block.data());

// 访问特征点集合（直接访问底层FeaturePoints）
FeaturePoints& features = image_features.GetFeaturePoints();
Size num_features = features.size();

// 通过代理访问单个特征（兼容性方式）
auto feat_proxy = image_features.GetFeature(0);
Feature coord = feat_proxy.GetCoord();
float size = feat_proxy.GetSize();

// 清零特征计数器（SIMD优化）
image_features.ZeroFeatureCounters();

// 获取有效特征数量
Size valid_features = image_features.GetNumValidFeatures();

// 清理未使用的特征
image_features.ClearUnusedFeatures();

// 描述子管理
auto descs_soa = std::make_shared<DescriptorsSOA>(1000, 128);
image_features.SetDescriptorsSOA(descs_soa);  // 设置SOA格式描述子

if (image_features.HasDescriptorsSOA()) {
    auto& desc_ref = image_features.GetDescriptorsSOAPtr();
    // 使用描述子...
}

// 检查描述子引用计数（内存调试）
long soa_ref_count = image_features.GetDescriptorsSOARefCount();
```

#### 特征集合管理
```cpp
// 创建特征信息集合
FeaturesInfo features_info;

// 添加图像特征
features_info.AddImageFeatures("image1.jpg", true);
features_info.AddImageFeatures("image2.jpg", true);

// 智能访问图像特征
ImageFeatureInfo* img_feat = features_info[0];  // 返回指针，需检查nullptr
if (img_feat) {
    img_feat->AddFeature(FeaturePoint(10.0f, 20.0f));
    std::string path = img_feat->GetImagePath();
}

// 安全访问图像特征
const ImageFeatureInfo* safe_img_feat = features_info.GetImageFeaturePtr(1);
if (safe_img_feat) {
    Size num_features = safe_img_feat->GetNumFeatures();
}

// 管理操作
Size valid_images = features_info.GetNumValidImages();
features_info.ClearUnusedImages();
features_info.ClearAllUnusedFeatures();
```

### 基础特征类型
(basic-feature-types)=
(feature)=
(descriptor)=
(descs)=
(descriptorssoa)=
(descs-soa)=
(feature-points)=
(feature-points-ptr)=

**类型定义**:
| 类型               | 定义                                        | 默认值  | 用途                              |
| ------------------ | ------------------------------------------- | ------- | --------------------------------- |
| `Feature`          | `Eigen::Matrix<double, 2, 1>`               | 零向量  | 基础2D特征点，仅包含位置信息      |
| `Descriptor`       | `std::vector<float>`                        | 空向量  | 旧版AOS格式描述子（向后兼容）     |
| `Descs`            | `std::vector<`[`Descriptor`](descriptor)`>` | 空向量  | 旧版AOS格式描述子集合（向后兼容） |
| `DescriptorsSOA`   | SOA格式描述子容器类                         | -       | 高性能SOA格式描述子，SIMD优化     |
| `DescsSOA`         | `DescriptorsSOA`                            | -       | DescriptorsSOA的类型别名          |
| `DescsSOAPtr`      | `Ptr<DescriptorsSOA>`                       | nullptr | DescriptorsSOA的智能指针          |
| `FeaturePoints`    | SOA格式特征点容器类                         | -       | 高性能SOA格式特征点集合，SIMD优化 |
| `FeaturePointsPtr` | `Ptr<FeaturePoints>`                        | nullptr | FeaturePoints的智能指针           |

### `DescriptorsSOA` (推荐使用)
(descriptorssoa-class)=
SIMD优化的描述子容器，使用SOA (Structure of Arrays) 格式，提供零拷贝兼容和8-9x加速的CV↔PoSDK转换

**关键特性**:
- **单次内存分配**: 所有描述子在一个连续内存块中（vs 10K次分散分配）
- **SIMD友好**: 支持AVX2/AVX-512批量操作
- **零拷贝兼容**: 与cv::Mat和protobuf无缝对接
- **紧凑布局**: num_features × descriptor_dim × sizeof(float)

**内存布局**:
```
[desc0[0], desc0[1], ..., desc0[dim-1], desc1[0], desc1[1], ..., desc1[dim-1], ...]
```

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">DescriptorsSOA</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">DescriptorsSOA</span> - 带参数构造函数
  - num_features `size_t`：特征数量
  - descriptor_dim `size_t`：描述子维度
- <span style="color:#1976d2;font-weight:bold;">resize</span> - 调整大小
  - num_features `size_t`：特征数量
  - descriptor_dim `size_t`：描述子维度
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - 预留容量
  - num_features `size_t`：特征数量
  - descriptor_dim `size_t`：描述子维度
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">clear</span> - 清空数据
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取特征数量
  - 返回 `size_t`：特征数量
- <span style="color:#1976d2;font-weight:bold;">dim</span> - 获取描述子维度
  - 返回 `size_t`：描述子维度
- <span style="color:#1976d2;font-weight:bold;">total_size</span> - 获取总数据大小
  - 返回 `size_t`：总数据大小（num_features × descriptor_dim）
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
  - 返回 `bool`：是否为空
- <span style="color:#1976d2;font-weight:bold;">data</span> - 获取原始数据指针（批量操作）
  - 返回 `float*` / `const float*`：原始数据指针
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 获取描述子指针
  - feature_idx `size_t`：特征索引
  - 返回 `float*` / `const float*`：指向descriptor_dim个浮点数的指针
- <span style="color:#1976d2;font-weight:bold;">SetDescriptor</span> - 设置单个描述子（SIMD优化）
  - feature_idx `size_t`：特征索引
  - descriptor_data `const float*`：描述子数据指针
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptor</span> - 获取单个描述子（SIMD优化）
  - feature_idx `size_t`：特征索引
  - descriptor_out `float*`：输出缓冲区
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">FromDescs</span> - 从旧版AOS格式转换
  - descs `const Descs&`：旧版描述子向量
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ToDescs</span> - 转换为旧版AOS格式
  - descs `Descs&`：输出旧版描述子向量
  - 返回 `void`

### `FeaturePoints` (推荐使用)
(featurepoints-class)=
SIMD优化的SOA特征点容器，采用自然对齐，提供批量优先API

**关键特性**:
- **SOA内存布局**: 坐标、大小、角度分别存储，SIMD友好
- **自然对齐**: 减少90%+内存占用，提高缓存效率
- **批量操作**: 完全向量化的批量设置/更新（AVX2/AVX-512）
- **迭代器支持**: 提供STL风格的迭代器和范围for循环

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">FeaturePoints</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">FeaturePoints</span> - 带预留容量构造
  - reserve_size `Size`：预留大小
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - 预留容量
  - capacity `Size`：预留大小
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">resize</span> - 调整大小
  - new_size `Size`：新大小
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">clear</span> - 清空数据
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取特征数量
  - 返回 `Size`：特征数量
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
  - 返回 `bool`：是否为空
- <span style="color:#1976d2;font-weight:bold;">GetSize</span> - 获取单个特征大小
  - idx `Size`：特征索引
  - 返回 `float&` / `const float&`：特征大小引用
- <span style="color:#1976d2;font-weight:bold;">GetAngle</span> - 获取单个特征角度
  - idx `Size`：特征索引
  - 返回 `float&` / `const float&`：特征角度引用
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - 获取单个特征使用状态
  - idx `Size`：特征索引
  - 返回 `uint8_t&` / `const uint8_t&`：使用状态引用
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - 获取单个特征坐标
  - idx `Size`：特征索引
  - 返回 `Feature`：特征坐标
- <span style="color:#1976d2;font-weight:bold;">SetCoord</span> - 设置单个特征坐标
  - idx `Size`：特征索引
  - coord `const Feature&`：坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetCoordsRef</span> - 获取坐标矩阵引用（批量操作）
  - 返回 `Eigen::Matrix<double, 2, Eigen::Dynamic>&`
- <span style="color:#1976d2;font-weight:bold;">GetSizesRef</span> - 获取大小向量引用（批量操作）
  - 返回 `std::vector<float>&`
- <span style="color:#1976d2;font-weight:bold;">GetAnglesRef</span> - 获取角度向量引用（批量操作）
  - 返回 `std::vector<float>&`
- <span style="color:#1976d2;font-weight:bold;">GetIsUsedRef</span> - 获取使用标志向量引用（批量操作）
  - 返回 `std::vector<uint8_t>&`
- <span style="color:#1976d2;font-weight:bold;">GetTotalCountsRef</span> - 获取总计数向量引用（批量操作）
  - 返回 `std::vector<IndexT>&`
- <span style="color:#1976d2;font-weight:bold;">GetOutlierCountsRef</span> - 获取异常计数向量引用（批量操作）
  - 返回 `std::vector<IndexT>&`
- <span style="color:#1976d2;font-weight:bold;">SetCoordsBlock</span> - 批量设置坐标（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`：坐标块
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetSizesBlock</span> - 批量设置大小（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - sizes_block `const float*`：大小数据指针
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetAnglesBlock</span> - 批量设置角度（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - angles_block `const float*`：角度数据指针
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetIsUsedBlock</span> - 批量设置使用标志（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - flags `const uint8_t*`：标志数据指针
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">IncrementTotalCountsBlock</span> - 批量增加总计数（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">IncrementOutlierCountsBlock</span> - 批量增加异常计数（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetNumValid</span> - 获取有效特征数量（SIMD优化）
  - 返回 `Size`：有效特征数量
- <span style="color:#1976d2;font-weight:bold;">GetOutlierRatiosBlock</span> - 批量获取异常比率（SIMD优化）
  - start `Size`：起始索引
  - count `Size`：数量
  - ratios_out `double*`：输出缓冲区
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - 添加单个特征（方式1）
  - coord `const Feature&`：坐标
  - size `float`：大小（默认0.0f）
  - angle `float`：角度（默认0.0f）
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - 添加单个特征（方式2）
  - x `float`：x坐标
  - y `float`：y坐标
  - size `float`：大小（默认0.0f）
  - angle `float`：角度（默认0.0f）
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddFeaturesBlock</span> - 批量添加特征（SIMD优化）
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`：坐标块
  - sizes_block `const float*`：大小数据指针（可选）
  - angles_block `const float*`：角度数据指针（可选）
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">RemoveUnused</span> - 移除未使用的特征
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ZeroCounters</span> - 批量清零计数器（SIMD优化）
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（返回代理对象）
  - index `Size`：特征索引
  - 返回 `FeaturePointProxy`：代理对象
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - STL风格迭代器
  - 返回 `SimpleIterator`：迭代器对象

```{tip}
**性能建议 | Performance Tips**

1. **批量操作优先**: 使用`AddFeaturesBlock`、`SetSizesBlock`等批量API，充分利用SIMD加速
2. **预留容量**: 使用`reserve()`避免多次内存重分配
3. **直接访问**: 使用`GetCoordsRef()`等引用接口进行批量处理
4. **SOA描述子**: 优先使用`DescriptorsSOA`而非旧版`Descs`，获得8-9x加速
```

### `FeaturePoint` （旧版，保留兼容性）
(feature-point)=
旧版特征点类（已被FeaturePoints替代，保留用于兼容性）

```{warning}
**已弃用 | Deprecated**

`FeaturePoint`类已被高性能的`FeaturePoints` SOA容器替代。新代码建议直接使用`FeaturePoints`及其代理对象`FeaturePointProxy`。

旧版`FeaturePoint`仅保留用于向后兼容，不推荐在新代码中使用。
```

### `ImageFeatureInfo`
(image-feature-info)=
单张图像的特征信息封装类，采用SIMD优化的SOA设计，包含可选的描述子管理

**私有成员变量**:
| 名称                   | 类型                                        | 默认值  | 用途                                |
| ---------------------- | ------------------------------------------- | ------- | ----------------------------------- |
| `image_path_`          | `std::string`                               | ""      | 图像文件路径字符串                  |
| `features_`            | [`FeaturePoints`](featurepoints-class)      | 默认值  | 特征点集合（SOA格式，SIMD优化）     |
| `is_used_`             | `bool`                                      | true    | 该图像特征信息的使用状态            |
| `descriptors_ptr_`     | `Ptr<`[`Descs`](descs)`>`                   | nullptr | 旧版AOS描述子指针（向后兼容，可选） |
| `descriptors_soa_ptr_` | `Ptr<`[`DescriptorsSOA`](descriptorssoa)`>` | nullptr | SOA描述子指针（高性能，可选，推荐） |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">ImageFeatureInfo</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">ImageFeatureInfo</span> - 带参数构造函数
  - path `const std::string &`：图像文件路径
  - used `bool`：是否使用标志（默认true）
- <span style="color:#1976d2;font-weight:bold;">GetImagePath</span> - 获取图像路径
  - 返回 `const std::string&`：图像路径引用
- <span style="color:#1976d2;font-weight:bold;">SetImagePath</span> - 设置图像路径
  - path `const std::string&`：图像路径
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - 获取使用状态
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - 设置使用状态
  - used `bool`：是否使用
- <span style="color:#1976d2;font-weight:bold;">GetFeaturePoints</span> - 获取特征点集合（非const版本）
  - 返回 `FeaturePoints&`：特征点集合引用
- <span style="color:#1976d2;font-weight:bold;">GetFeaturePoints</span> - 获取特征点集合（const版本）
  - 返回 `const FeaturePoints&`：特征点集合常量引用
- <span style="color:#1976d2;font-weight:bold;">GetNumFeatures</span> - 获取特征点数量
- <span style="color:#1976d2;font-weight:bold;">GetNumValidFeatures</span> - 获取有效特征点数量（SIMD优化）
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - 添加单个特征（方式1）
  - coord `const Feature&`：坐标
  - size `float`：大小（默认0.0f）
  - angle `float`：角度（默认0.0f）
- <span style="color:#1976d2;font-weight:bold;">AddFeature</span> - 添加单个特征（方式2）
  - x `float`：x坐标
  - y `float`：y坐标
  - size `float`：大小（默认0.0f）
  - angle `float`：角度（默认0.0f）
- <span style="color:#1976d2;font-weight:bold;">AddFeaturesBlock</span> - 批量添加特征（SIMD优化）
  - coords_block `const Eigen::Ref<const Eigen::Matrix<double, 2, Eigen::Dynamic>>&`：坐标块
  - sizes_block `const float*`：大小数据指针（可选）
  - angles_block `const float*`：角度数据指针（可选）
- <span style="color:#1976d2;font-weight:bold;">ClearUnusedFeatures</span> - 清除未使用的特征点
- <span style="color:#1976d2;font-weight:bold;">ClearAllFeatures</span> - 清除所有特征点
- <span style="color:#1976d2;font-weight:bold;">ReserveFeatures</span> - 预留特征点容量
  - capacity `Size`：预留容量
- <span style="color:#1976d2;font-weight:bold;">ZeroFeatureCounters</span> - 清零特征计数器（SIMD优化）
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取特征点数量
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - 迭代器访问
- <span style="color:#1976d2;font-weight:bold;">HasDescriptors</span> - 检查是否存储了描述子（AOS或SOA）
- <span style="color:#1976d2;font-weight:bold;">HasDescriptorsSOA</span> - 检查是否存储了SOA描述子
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsPtr</span> - 获取旧版AOS描述子指针（const版本）
  - 返回 `const Ptr<Descs>&`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsPtr</span> - 获取旧版AOS描述子指针（非const版本）
  - 返回 `Ptr<Descs>&`
- <span style="color:#1976d2;font-weight:bold;">SetDescriptors</span> - 设置旧版AOS描述子
  - descs_ptr `const Ptr<Descs>&`：描述子指针
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOAPtr</span> - 获取SOA描述子指针（const版本）(推荐)
  - 返回 `const Ptr<DescriptorsSOA>&`
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOAPtr</span> - 获取SOA描述子指针（非const版本）(推荐)
  - 返回 `Ptr<DescriptorsSOA>&`
- <span style="color:#1976d2;font-weight:bold;">SetDescriptorsSOA</span> - 设置SOA描述子 (推荐)
  - descs_soa_ptr `const Ptr<DescriptorsSOA>&`：SOA描述子指针
- <span style="color:#1976d2;font-weight:bold;">ClearDescriptors</span> - 清除描述子并释放内存
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsRefCount</span> - 获取AOS描述子引用计数（调试用）
- <span style="color:#1976d2;font-weight:bold;">GetDescriptorsSOARefCount</span> - 获取SOA描述子引用计数（调试用）
- <span style="color:#1976d2;font-weight:bold;">GetFeature</span> - 获取特征代理对象（兼容性方法）
  - index `Size`：特征索引
  - 返回 `FeatureProxy`：特征代理对象

### `FeaturesInfo`
(features-info)=
所有图像的特征信息集合，增强的容器类，继承自std::vector<[`ImageFeatureInfo`](image-feature-info)>

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（非const版本）
  - index `Size`：图像索引
  - 返回 `ImageFeatureInfo*`：图像特征信息指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（const版本）
  - index `Size`：图像索引
  - 返回 `const ImageFeatureInfo*`：图像特征信息常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetImageFeaturePtr</span> - 安全获取图像特征信息指针（非const版本）
  - index `Size`：图像索引
  - 返回 `ImageFeatureInfo*`：图像特征信息指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetImageFeaturePtr</span> - 安全获取图像特征信息指针（const版本）
  - index `Size`：图像索引
  - 返回 `const ImageFeatureInfo*`：图像特征信息常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">AddImageFeatures</span> - 添加图像特征信息
  - image_path `const std::string &`：图像路径
  - is_used `bool`：是否使用标志（默认true）
- <span style="color:#1976d2;font-weight:bold;">GetNumValidImages</span> - 获取有效图像数量
- <span style="color:#1976d2;font-weight:bold;">ClearUnusedImages</span> - 清除未使用的图像
- <span style="color:#1976d2;font-weight:bold;">ClearAllUnusedFeatures</span> - 清除所有未使用的特征点
- <span style="color:#1976d2;font-weight:bold;">Print</span> - 打印特征信息详情
  - print_unused `bool`：是否打印未使用的特征（默认false）

### 特征相关类型定义
(feature-related-types)=
(features-info-ptr)=

**类型定义**:
| 类型              | 定义                                                 | 默认值  | 用途                   |
| ----------------- | ---------------------------------------------------- | ------- | ---------------------- |
| `FeaturesInfoPtr` | `std::shared_ptr<`[`FeaturesInfo`](features-info)`>` | nullptr | 特征信息的智能指针类型 |

---

## 匹配 (types/matches.hpp)

### `IdMatch`
(id-match)=
特征点匹配结构

**成员变量**:
| 名称        | 类型                | 默认值 | 用途                         |
| ----------- | ------------------- | ------ | ---------------------------- |
| `i`         | [`IndexT`](index-t) | 0      | 第一个特征的索引标识         |
| `j`         | [`IndexT`](index-t) | 0      | 第二个特征的索引标识         |
| `is_inlier` | `bool`              | false  | RANSAC内点标志，标识匹配质量 |

### 匹配集合类型定义
(matches-collection-types)=
(id-matches)=
(view-pair)=
(matches)=
(matches-ptr)=

**类型定义**:
| 类型         | 定义                                                               | 默认值  | 用途                                   |
| ------------ | ------------------------------------------------------------------ | ------- | -------------------------------------- |
| `IdMatches`  | `std::vector<`[`IdMatch`](id-match)`>`                             | 空向量  | 特征匹配集合，存储一对图像间的所有匹配 |
| `ViewPair`   | `std::pair<`[`IndexT`](index-t)`, `[`IndexT`](index-t)`>`          | (0,0)   | 视图索引对，标识两个视图的组合         |
| `Matches`    | `std::map<`[`ViewPair`](view-pair)`, `[`IdMatches`](id-matches)`>` | 空映射  | 所有视图对之间的匹配关系映射           |
| `MatchesPtr` | `std::shared_ptr<`[`Matches`](matches)`>`                          | nullptr | 匹配集合的智能指针类型                 |

### 视线向量类型定义
(bearing-vector-types)=
(bearing-vector)=
(bearing-vectors)=
(bearing-pairs)=

**类型定义**:
| 类型             | 定义                                     | 默认值 | 用途                                 |
| ---------------- | ---------------------------------------- | ------ | ------------------------------------ |
| `BearingVector`  | [`Vector3d`](vector3d)                   | 零向量 | 单位观测向量类型，从相机中心指向3D点 |
| `BearingVectors` | `Eigen::Matrix<double,3,Eigen::Dynamic>` | 零矩阵 | 3×N观测向量矩阵，存储多个观测向量    |
| `BearingPairs`   | `std::vector<Eigen::Matrix<double,6,1>>` | 空向量 | 匹配的视线向量对，用于相对位姿估计   |

### 视线向量工具函数
(bearing-vector-utils)=

**可用函数**:
- <span style="color:#1976d2;font-weight:bold;">PixelToBearingVector</span> - 像素坐标转单位观测向量
  - pixel_coord `const Vector2d &`：像素坐标
  - camera_model `const CameraModel &`：相机模型
  - 返回 `BearingVector`：单位观测向量
- <span style="color:#1976d2;font-weight:bold;">PixelToBearingVector</span> - 像素坐标转单位观测向量（重载版本）
  - pixel_coord `const Vector2d &`：像素坐标
  - fx `double`：x方向焦距
  - fy `double`：y方向焦距
  - cx `double`：x方向主点偏移
  - cy `double`：y方向主点偏移
  - 返回 `BearingVector`：单位观测向量
- <span style="color:#1976d2;font-weight:bold;">MatchesToBearingPairs</span> - 将匹配转换为视线向量对
  - matches `const IdMatches &`：特征匹配
  - features_info `const FeaturesInfo &`：所有视图的特征信息
  - camera_models `const CameraModels &`：所有视图的相机模型
  - view_pair `const ViewPair &`：视图对索引
  - bearing_pairs `BearingPairs &`：输出视线向量对
  - 返回 `bool`：是否成功
- <span style="color:#1976d2;font-weight:bold;">MatchesToBearingPairsInliersOnly</span> - 将内点匹配转换为视线向量对
  - matches `const IdMatches &`：特征匹配
  - features_info `const FeaturesInfo &`：所有视图的特征信息
  - camera_models `const CameraModels &`：所有视图的相机模型
  - view_pair `const ViewPair &`：视图对索引
  - bearing_pairs `BearingPairs &`：输出视线向量对（仅内点）
  - 返回 `bool`：是否成功

---

## 轨迹与观测 (types/tracks.hpp)

轨迹系统采用封装类设计，提供更好的数据安全性和访问控制。

### 核心特性与使用模式

**封装设计优势**:
- **私有成员变量**: 防止意外修改，提高数据安全性
- **公有访问接口**: 提供统一的数据访问和修改方式
- **内存对齐优化**: 使用Eigen aligned_allocator提高性能，包含EIGEN_MAKE_ALIGNED_OPERATOR_NEW宏
- **容器式接口**: 支持STL容器的使用模式

### 典型使用示例

#### 基本观测信息创建和操作
```cpp
using namespace PoSDK::types;

// 创建观测信息
ObsInfo obs1;  // 默认构造
ObsInfo obs2(0, 1, Vector2d(100.0, 200.0));  // 视图ID、特征ID、坐标

// 设置和获取属性
obs1.SetViewId(1);
obs1.SetFeatureId(5);  // 设置在FeaturesInfo[view_id]中的特征索引
obs1.SetObsId(123);
obs1.SetCoord(Vector2d(150.0, 250.0));
obs1.SetUsed(true);

// 获取属性
ViewId view_id = obs1.GetViewId();
IndexT feature_id = obs1.GetFeatureId();  // 获取特征ID
IndexT obs_id = obs1.GetObsId();
const Vector2d& coord = obs1.GetCoord();
bool is_used = obs1.IsUsed();

// 获取齐次坐标
Vector3d homo_coord = obs1.GetHomoCoord();
```

#### 轨迹信息管理
```cpp
// 创建轨迹信息
TrackInfo track_info;

// 添加观测
track_info.AddObservation(0, 1, Vector2d(100.0, 200.0));  // 视图ID、特征ID、坐标
track_info.AddObservation(obs1);  // 直接添加观测对象

// 访问观测
Size obs_count = track_info.GetObservationCount();
Size valid_count = track_info.GetValidObservationCount();

// 数组式访问（返回指针）
const ObsInfo* obs_ptr = track_info[0];
if (obs_ptr) {
    Vector2d coord = obs_ptr->GetCoord();
}

// 安全访问（引用访问，越界抛异常）
try {
    const ObsInfo& obs_ref = track_info.GetObservation(1);
    ViewId view_id = obs_ref.GetViewId();
} catch (const std::out_of_range& e) {
    // 处理越界访问
}

// 获取轨迹数据
const Track& track = track_info.GetTrack();
Track& mutable_track = track_info.GetTrack();

// 设置轨迹使用状态
track_info.SetUsed(true);
bool track_used = track_info.IsUsed();

// 设置观测状态
track_info.SetObservationUsed(0, false);
track_info.SetObservationFeatureId(0, 10);
```

#### 轨迹集合管理
```cpp
// 创建轨迹集合
Tracks tracks;

// 添加轨迹
std::vector<ObsInfo> observations = {obs1, obs2};
tracks.AddTrack(observations);
tracks.AddTrack(track_info);

// 直接添加（兼容性方法）
tracks.push_back(track_info);
tracks.emplace_back(observations, true);

// 访问轨迹
Size track_count = tracks.size();
Size valid_track_count = tracks.GetValidTrackCount();

// 数组式访问（返回指针）
const TrackInfo* track_ptr = tracks[0];
if (track_ptr) {
    Size obs_count = track_ptr->GetObservationCount();
}

// 安全访问（引用访问，越界抛异常）
try {
    const TrackInfo& track_ref = tracks.GetTrack(1);
    bool is_used = track_ref.IsUsed();
} catch (const std::out_of_range& e) {
    // 处理越界访问
}

// 快速观测访问：Tracks[点ID, 视图ID]
const ObsInfo* obs_ptr = tracks(1, 0);  // 点ID=1, 视图ID=0
if (obs_ptr) {
    Vector2d coord = obs_ptr->GetCoord();
}

// 迭代器访问
for (const auto& track_info : tracks) {
    if (track_info.IsUsed()) {
        for (const auto& obs : track_info.GetTrack()) {
            if (obs.IsUsed()) {
                Vector2d coord = obs.GetCoord();
                // 处理观测
            }
        }
    }
}

// 归一化处理
CameraModels camera_models;
// ... 设置相机模型 ...
bool normalized = tracks.NormalizeTracks(camera_models);
bool is_normalized = tracks.IsNormalized();
tracks.SetNormalized(true);
```

### `ObsInfo`
(obs-info)=
3D点的单个观测信息封装类，提供私有成员和公有访问接口

**私有成员变量**:
| 名称              | 类型                     | 默认值  | 用途                                    |
| ----------------- | ------------------------ | ------- | --------------------------------------- |
| `coord_`          | [`Vector2d`](vector2d)   | 零向量  | 当前2D坐标（可能是原始的或重投影的）    |
| `original_coord_` | `std::vector<double>`    | 空向量  | 原始图像坐标（如果未存储则为空）        |
| `view_id_`        | [`ViewId`](view-id)      | 0       | 图像ID(与ImagePaths中的索引一致)        |
| `feature_id_`     | [`IndexT`](index-t)      | 0       | 在FeaturesInfo[view_id]中的特征点ID索引 |
| `obs_id_`         | [`IndexT`](index-t)      | 0       | 观测ID                                  |
| `is_used_`        | `bool`                   | true    | 当前观测是否被使用                      |
| `color_rgb_`      | `std::array<uint8_t, 3>` | {0,0,0} | RGB颜色值(0-255)                        |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">ObsInfo</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">ObsInfo</span> - 带参数构造函数
  - vid `ViewId`：视图ID
  - fid `IndexT`：特征ID
  - c `const Vector2d &`：观测坐标
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - 获取观测坐标
  - 返回 `const Vector2d&`：观测坐标引用
- <span style="color:#1976d2;font-weight:bold;">SetCoord</span> - 设置观测坐标
  - coord `const Vector2d&`：观测坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetViewId</span> - 获取视图ID
  - 返回 `ViewId`：视图ID
- <span style="color:#1976d2;font-weight:bold;">SetViewId</span> - 设置视图ID
  - view_id `ViewId`：视图ID
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetFeatureId</span> - 获取特征ID
  - 返回 `IndexT`：特征ID(在FeaturesInfo[view_id]中的索引)
- <span style="color:#1976d2;font-weight:bold;">SetFeatureId</span> - 设置特征ID
  - feature_id `IndexT`：特征ID(在FeaturesInfo[view_id]中的索引)
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetObsId</span> - 获取观测ID
  - 返回 `IndexT`：观测ID
- <span style="color:#1976d2;font-weight:bold;">SetObsId</span> - 设置观测ID
  - obs_id `IndexT`：观测ID
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - 获取使用状态
  - 返回 `bool`：使用状态标志
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - 设置观测使用状态
  - used `bool`：是否使用标志
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetColorRGB</span> - 获取RGB颜色
  - 返回 `const std::array<uint8_t, 3>&`：RGB颜色数组引用
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - 设置RGB颜色（方式1）
  - r `uint8_t`：红色分量(0-255)
  - g `uint8_t`：绿色分量(0-255)
  - b `uint8_t`：蓝色分量(0-255)
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - 设置RGB颜色（方式2）
  - rgb `const std::array<uint8_t, 3>&`：RGB颜色数组
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">HasColor</span> - 检查是否有颜色信息
  - 返回 `bool`：是否有非零颜色值
- <span style="color:#1976d2;font-weight:bold;">HasOriginalCoord</span> - 检查是否存储了原始坐标
  - 返回 `bool`：是否存储了原始坐标
- <span style="color:#1976d2;font-weight:bold;">StoreOriginalCoord</span> - 存储当前坐标为原始坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ResetToOriginalCoord</span> - 将当前坐标重置为原始坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetOriginalCoord</span> - 获取原始坐标
  - 返回 `Vector2d`：原始坐标（如果未存储则返回当前坐标）
- <span style="color:#1976d2;font-weight:bold;">GetReprojectionError</span> - 计算重投影误差
  - 返回 `double`：当前坐标与原始坐标之间的欧氏距离（如果无原始数据返回-1.0）
- <span style="color:#1976d2;font-weight:bold;">GetCoord</span> - 获取观测坐标（兼容版本）
  - coord_out `Vector2d &`：输出坐标引用
  - 返回 `bool`：是否成功获取
- <span style="color:#1976d2;font-weight:bold;">GetHomoCoord</span> - 获取齐次坐标
  - 返回 `Vector3d`：齐次坐标

### `Track`
(track)=
3D点观测信息集合，类型别名：`std::vector<`[`ObsInfo`](obs-info)`, Eigen::aligned_allocator<`[`ObsInfo`](obs-info)`>>`

### `TrackInfo`
(track-info)=
轨迹信息封装类，提供私有成员和公有访问接口

**私有成员变量**:
| 名称       | 类型             | 默认值 | 用途             |
| ---------- | ---------------- | ------ | ---------------- |
| `track_`   | [`Track`](track) | 默认值 | 轨迹观测数据     |
| `is_used_` | `bool`           | true   | 该轨迹是否被使用 |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - 带参数构造函数（方式1）
  - t `const Track &`：轨迹数据
  - used `bool`：是否使用标志（默认true）
- <span style="color:#1976d2;font-weight:bold;">TrackInfo</span> - 带参数构造函数（方式2）
  - observations `const std::vector<ObsInfo> &`：观测数组
  - used `bool`：是否使用标志（默认true）
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - 获取轨迹数据（非const版本）
  - 返回 `Track&`：轨迹数据引用
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - 获取轨迹数据（const版本）
  - 返回 `const Track&`：轨迹数据常量引用
- <span style="color:#1976d2;font-weight:bold;">IsUsed</span> - 获取使用状态
  - 返回 `bool`：使用状态标志
- <span style="color:#1976d2;font-weight:bold;">SetUsed</span> - 设置轨迹使用状态
  - used `bool`：是否使用该轨迹
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（非const版本）
  - index `Size`：观测索引
  - 返回 `ObsInfo*`：观测指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（const版本）
  - index `Size`：观测索引
  - 返回 `const ObsInfo*`：观测常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - 获取观测信息（非const版本）
  - index `IndexT`：观测索引
  - 返回 `ObsInfo&`：观测引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - 获取观测信息（const版本）
  - index `IndexT`：观测索引
  - 返回 `const ObsInfo&`：观测常量引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - 添加观测（方式1）
  - view_id `ViewId`：视图ID
  - feature_id `IndexT`：特征ID(在FeaturesInfo[view_id]中的索引)
  - coord `const Vector2d &`：观测坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - 添加观测（方式2）
  - obs `const ObsInfo &`：观测信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetObservationCount</span> - 获取观测数量
  - 返回 `Size`：观测数量
- <span style="color:#1976d2;font-weight:bold;">GetValidObservationCount</span> - 获取有效观测数量
  - 返回 `Size`：有效观测数量
- <span style="color:#1976d2;font-weight:bold;">GetObservationCoord</span> - 获取指定观测的坐标
  - index `IndexT`：观测索引
  - coord_out `Vector2d &`：输出坐标引用
  - 返回 `bool`：是否成功获取
- <span style="color:#1976d2;font-weight:bold;">SetObservationUsed</span> - 设置指定观测的使用状态
  - index `IndexT`：观测索引
  - used `bool`：是否使用该观测
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetObservationFeatureId</span> - 设置指定观测的特征ID
  - index `IndexT`：观测索引
  - feature_id `IndexT`：特征ID(在FeaturesInfo[view_id]中的索引)
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ClearObservations</span> - 清空观测数据
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ReserveObservations</span> - 预留观测容量
  - capacity `Size`：预留容量
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取观测数量
  - 返回 `Size`：观测数量
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
  - 返回 `bool`：是否为空
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - 迭代器访问
  - 返回迭代器对象

### `Tracks`
(tracks)=
轨迹集合封装类，提供完整的轨迹管理接口

**私有成员变量**:
| 名称             | 类型                                                                                              | 默认值 | 用途             |
| ---------------- | ------------------------------------------------------------------------------------------------- | ------ | ---------------- |
| `tracks_`        | `std::vector<`[`TrackInfo`](track-info)`, Eigen::aligned_allocator<`[`TrackInfo`](track-info)`>>` | 空向量 | 轨迹信息向量     |
| `is_normalized_` | `bool`                                                                                            | false  | 是否已归一化标志 |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">IsNormalized</span> - 获取归一化状态
  - 返回 `bool`：归一化状态标志
- <span style="color:#1976d2;font-weight:bold;">SetNormalized</span> - 设置归一化状态
  - normalized `bool`：归一化标志
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取轨迹数量
  - 返回 `Size`：轨迹数量
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
  - 返回 `bool`：是否为空
- <span style="color:#1976d2;font-weight:bold;">clear</span> - 清空所有轨迹
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - 预留容量
  - n `Size`：预留大小
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（非const版本）
  - index `Size`：轨迹索引
  - 返回 `TrackInfo*`：轨迹指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（const版本）
  - index `Size`：轨迹索引
  - 返回 `const TrackInfo*`：轨迹常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - 获取轨迹（非const版本）
  - index `Size`：轨迹索引
  - 返回 `TrackInfo&`：轨迹引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">GetTrack</span> - 获取轨迹（const版本）
  - index `Size`：轨迹索引
  - 返回 `const TrackInfo&`：轨迹常量引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - 快速观测访问（非const版本）
  - pts_id `PtsId`：点ID
  - view_id `ViewId`：视图ID
  - 返回 `ObsInfo*`：观测指针，未找到时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - 快速观测访问（const版本）
  - pts_id `PtsId`：点ID
  - view_id `ViewId`：视图ID
  - 返回 `const ObsInfo*`：观测常量指针，未找到时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - 获取观测（非const版本）
  - pts_id `PtsId`：点ID
  - view_id `ViewId`：视图ID
  - 返回 `ObsInfo*`：观测指针，未找到时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetObservation</span> - 获取观测（const版本）
  - pts_id `PtsId`：点ID
  - view_id `ViewId`：视图ID
  - 返回 `const ObsInfo*`：观测常量指针，未找到时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - 迭代器访问
  - 返回迭代器对象
- <span style="color:#1976d2;font-weight:bold;">cbegin</span>, <span style="color:#1976d2;font-weight:bold;">cend</span> - 常量迭代器访问
  - 返回常量迭代器对象
- <span style="color:#1976d2;font-weight:bold;">AddTrack</span> - 添加新轨迹（方式1）
  - observations `const std::vector<ObsInfo> &`：观测信息数组
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddTrack</span> - 添加新轨迹（方式2）
  - track_info `const TrackInfo &`：轨迹信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">AddObservation</span> - 向指定轨迹添加观测
  - track_id `PtsId`：轨迹ID（即tracks向量中的索引）
  - obs `const ObsInfo &`：观测信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetTrackCount</span> - 获取轨迹数量
  - 返回 `Size`：轨迹数量
- <span style="color:#1976d2;font-weight:bold;">GetObservationCount</span> - 获取指定轨迹的观测数量
  - track_id `PtsId`：轨迹ID
  - 返回 `Size`：观测数量
- <span style="color:#1976d2;font-weight:bold;">GetObservationByTrackIndex</span> - 按轨迹索引获取观测
  - track_id `PtsId`：轨迹ID
  - index `IndexT`：观测索引
  - 返回 `const ObsInfo&`：观测引用
- <span style="color:#1976d2;font-weight:bold;">GetValidTrackCount</span> - 获取有效轨迹数量
  - 返回 `Size`：有效轨迹数量
- <span style="color:#1976d2;font-weight:bold;">NormalizeTracks</span> - 使用相机模型将整个轨迹集归一化
  - camera_models `const CameraModels &`：相机模型集合
  - 返回 `bool`：归一化是否成功
- <span style="color:#1976d2;font-weight:bold;">OriginalizeTracks</span> - 使用相机模型将归一化轨迹还原为原始像素坐标
  - camera_models `const CameraModels &`：相机模型集合
  - 返回 `bool`：还原是否成功
- <span style="color:#1976d2;font-weight:bold;">StoreOriginalCoords</span> - 为所有观测存储原始坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ResetToOriginalCoords</span> - 将所有观测重置为原始坐标
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">push_back</span> - 添加轨迹（兼容性方法）
  - track `const TrackInfo &`：轨迹信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">push_back</span> - 添加轨迹（移动版本）
  - track `TrackInfo &&`：轨迹信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - 就地构造轨迹
  - args `Args &&...`：构造参数
  - 返回 `void`

### 轨迹相关类型定义
(tracks-related-types)=
(tracks-ptr)=

**类型定义**:
| 类型        | 定义                                    | 默认值  | 用途                 |
| ----------- | --------------------------------------- | ------- | -------------------- |
| `TracksPtr` | `std::shared_ptr<`[`Tracks`](tracks)`>` | nullptr | 轨迹集合智能指针类型 |
---

## 相对位姿 (types/relative_poses.hpp)

相对位姿系统采用封装类设计，使用PIMPL模式实现，提供更好的数据安全性和二进制兼容性。

### 核心特性与使用模式

**封装设计优势**:
- **PIMPL模式**: 隐藏实现细节，提高二进制兼容性
- **候选解支持**: 支持多个候选位姿解的管理
- **私有成员变量**: 防止意外修改，提高数据安全性
- **公有访问接口**: 提供统一的数据访问和修改方式
- **内存对齐优化**: 使用Eigen aligned_allocator提高性能

### 典型使用示例

#### 基本相对旋转创建和操作
```cpp
using namespace PoSDK::types;

// 创建相对旋转
RelativeRotation rel_rot1;  // 默认构造
RelativeRotation rel_rot2(0, 1, Matrix3d::Identity(), 1.0f);  // 完整参数构造

// 设置和获取视图ID
rel_rot1.SetViewIdI(0);
rel_rot1.SetViewIdJ(1);
ViewId view_i = rel_rot1.GetViewIdI();
ViewId view_j = rel_rot1.GetViewIdJ();

// 设置和获取旋转矩阵
Matrix3d R = Matrix3d::Identity();
rel_rot1.SetRotation(R);
Matrix3d R_out = rel_rot1.GetRotation();

// 设置和获取权重
rel_rot1.SetWeight(0.8f);
float weight = rel_rot1.GetWeight();

// 候选解管理
rel_rot1.AddCandidateRotation(R);
rel_rot1.AddCandidateRotation(Matrix3d::Identity());
size_t num_candidates = rel_rot1.GetCandidateCount();
Matrix3d candidate_R = rel_rot1.GetCandidateRotation(0);
rel_rot1.SetPrimaryCandidateIndex(1);
size_t primary_idx = rel_rot1.GetPrimaryCandidateIndex();
rel_rot1.ClearCandidates();
```

#### 相对位姿管理
```cpp
// 创建相对位姿
RelativePose rel_pose1;  // 默认构造
RelativePose rel_pose2(0, 1, Matrix3d::Identity(), Vector3d::Zero(), 1.0f);

// 基本访问器
Matrix3d R = rel_pose1.GetRotation();
Vector3d t = rel_pose1.GetTranslation();
rel_pose1.SetRotation(R);
rel_pose1.SetTranslation(t);

// 候选解管理
rel_pose1.AddCandidate(Matrix3d::Identity(), Vector3d::Zero());
size_t num_candidates = rel_pose1.GetCandidateCount();
Matrix3d candidate_R = rel_pose1.GetCandidateRotation(0);
Vector3d candidate_t = rel_pose1.GetCandidateTranslation(0);

// 位姿操作
Matrix3d E = rel_pose1.GetEssentialMatrix();
double rot_err, trans_err;
bool success = rel_pose1.ComputeErrors(rel_pose2, rot_err, trans_err, false);
```

#### 相对位姿集合管理
```cpp
// 创建相对位姿集合
RelativePoses poses;

// 添加位姿
poses.AddPose(rel_pose1);  // 拷贝添加
poses.AddPose(std::move(rel_pose2));  // 移动添加
poses.EmplacePose(0, 1, Matrix3d::Identity(), Vector3d::Zero(), 1.0f);  // 原地构造

// 访问位姿
size_t num_poses = poses.size();
const RelativePose* pose_ptr = poses[0];  // 数组式访问，返回指针
if (pose_ptr) {
    Matrix3d R = pose_ptr->GetRotation();
}

// 安全访问
try {
    const RelativePose& pose_ref = poses.GetPose(1);  // 引用访问，越界抛异常
    Vector3d t = pose_ref.GetTranslation();
} catch (const std::out_of_range& e) {
    // 处理越界访问
}

// 迭代器访问
for (const auto& pose : poses) {
    ViewId i = pose.GetViewIdI();
    ViewId j = pose.GetViewIdJ();
    // 处理位姿
}

// 特定操作
std::vector<double> rot_errors, trans_errors;
size_t matched = poses.EvaluateAgainst(gt_poses, rot_errors, trans_errors);

Matrix3d R;
Vector3d t;
ViewPair vp(0, 1);
bool found = poses.GetRelativePose(vp, R, t);
```

### `RelativeRotation`
(relative-rotation)=
封装的相对旋转类，支持候选解管理

**私有成员（通过PIMPL实现）**:
| 名称                  | 类型                    | 默认值   | 用途                         |
| --------------------- | ----------------------- | -------- | ---------------------------- |
| view_id_i             | [`ViewId`](view-id)     | 0        | 源相机视图的索引             |
| view_id_j             | [`ViewId`](view-id)     | 0        | 目标相机视图的索引           |
| Rij                   | [`Matrix3d`](matrix3d)  | 单位矩阵 | 从视图i到视图j的相对旋转矩阵 |
| weight                | `float`                 | 1.0f     | 相对旋转的权重因子           |
| candidates            | `std::vector<Matrix3d>` | 空向量   | 候选旋转解集合               |
| primary_candidate_idx | `size_t`                | 0        | 主要候选解索引               |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">RelativeRotation</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">RelativeRotation</span> - 带参数构造函数
  - view_id_i `ViewId`：源视图索引
  - view_id_j `ViewId`：目标视图索引
  - Rij `const Matrix3d &`：相对旋转矩阵（默认单位矩阵）
  - weight `float`：权重系数（默认1.0f）
- <span style="color:#1976d2;font-weight:bold;">GetViewIdI</span> - 获取源视图ID
- <span style="color:#1976d2;font-weight:bold;">GetViewIdJ</span> - 获取目标视图ID
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - 获取旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">GetWeight</span> - 获取权重
- <span style="color:#1976d2;font-weight:bold;">SetViewIdI</span> - 设置源视图ID
  - view_id_i `ViewId`：源视图ID
- <span style="color:#1976d2;font-weight:bold;">SetViewIdJ</span> - 设置目标视图ID
  - view_id_j `ViewId`：目标视图ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - 设置旋转矩阵
  - Rij `const Matrix3d &`：旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">SetWeight</span> - 设置权重
  - weight `float`：权重值
- <span style="color:#1976d2;font-weight:bold;">GetCandidateCount</span> - 获取候选解数量
- <span style="color:#1976d2;font-weight:bold;">GetCandidateRotation</span> - 获取指定索引的候选旋转
  - index `size_t`：候选解索引
- <span style="color:#1976d2;font-weight:bold;">AddCandidateRotation</span> - 添加候选旋转解
  - Rij `const Matrix3d &`：候选旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">SetPrimaryCandidateIndex</span> - 设置主要候选解索引
  - index `size_t`：候选解索引
- <span style="color:#1976d2;font-weight:bold;">GetPrimaryCandidateIndex</span> - 获取主要候选解索引
- <span style="color:#1976d2;font-weight:bold;">ClearCandidates</span> - 清除所有候选解

### `RelativePose`
(relative-pose)=
封装的相对位姿类，支持候选解管理

**私有成员（通过PIMPL实现）**:
| 名称                  | 类型                    | 默认值   | 用途                         |
| --------------------- | ----------------------- | -------- | ---------------------------- |
| view_id_i             | [`ViewId`](view-id)     | 0        | 源相机视图的索引             |
| view_id_j             | [`ViewId`](view-id)     | 0        | 目标相机视图的索引           |
| Rij                   | [`Matrix3d`](matrix3d)  | 单位矩阵 | 从视图i到视图j的相对旋转矩阵 |
| tij                   | [`Vector3d`](vector3d)  | 零向量   | 从视图i到视图j的相对平移向量 |
| weight                | `float`                 | 1.0f     | 位姿估计的权重因子           |
| candidates_R          | `std::vector<Matrix3d>` | 空向量   | 候选旋转解集合               |
| candidates_t          | `std::vector<Vector3d>` | 空向量   | 候选平移解集合               |
| primary_candidate_idx | `size_t`                | 0        | 主要候选解索引               |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">RelativePose</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">RelativePose</span> - 带参数构造函数
  - view_id_i `ViewId`：源视图索引
  - view_id_j `ViewId`：目标视图索引
  - Rij `const Matrix3d &`：相对旋转矩阵（默认单位矩阵）
  - tij `const Vector3d &`：相对平移向量（默认零向量）
  - weight `float`：权重系数（默认1.0f）
- <span style="color:#1976d2;font-weight:bold;">GetViewIdI</span> - 获取源视图ID
- <span style="color:#1976d2;font-weight:bold;">GetViewIdJ</span> - 获取目标视图ID
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - 获取旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">GetTranslation</span> - 获取平移向量
- <span style="color:#1976d2;font-weight:bold;">GetWeight</span> - 获取权重
- <span style="color:#1976d2;font-weight:bold;">SetViewIdI</span> - 设置源视图ID
  - view_id_i `ViewId`：源视图ID
- <span style="color:#1976d2;font-weight:bold;">SetViewIdJ</span> - 设置目标视图ID
  - view_id_j `ViewId`：目标视图ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - 设置旋转矩阵
  - Rij `const Matrix3d &`：旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">SetTranslation</span> - 设置平移向量
  - tij `const Vector3d &`：平移向量
- <span style="color:#1976d2;font-weight:bold;">SetWeight</span> - 设置权重
  - weight `float`：权重值
- <span style="color:#1976d2;font-weight:bold;">GetCandidateCount</span> - 获取候选解数量
- <span style="color:#1976d2;font-weight:bold;">GetCandidateRotation</span> - 获取指定索引的候选旋转
  - index `size_t`：候选解索引
- <span style="color:#1976d2;font-weight:bold;">GetCandidateTranslation</span> - 获取指定索引的候选平移
  - index `size_t`：候选解索引
- <span style="color:#1976d2;font-weight:bold;">AddCandidate</span> - 添加候选解
  - Rij `const Matrix3d &`：候选旋转矩阵
  - tij `const Vector3d &`：候选平移向量
- <span style="color:#1976d2;font-weight:bold;">SetPrimaryCandidateIndex</span> - 设置主要候选解索引
  - index `size_t`：候选解索引
- <span style="color:#1976d2;font-weight:bold;">GetPrimaryCandidateIndex</span> - 获取主要候选解索引
- <span style="color:#1976d2;font-weight:bold;">ClearCandidates</span> - 清除所有候选解
- <span style="color:#1976d2;font-weight:bold;">GetEssentialMatrix</span> - 获取本质矩阵
  - 返回 `Matrix3d`：本质矩阵
- <span style="color:#1976d2;font-weight:bold;">ComputeErrors</span> - 计算与另一个相对位姿之间的误差
  - other `const RelativePose &`：另一个相对位姿
  - rotation_error `double &`：输出旋转误差（角度）
  - translation_error `double &`：输出平移方向误差（角度）
  - is_opengv_format `bool`：是否为OpenGV格式（默认false）
  - 返回 `bool`：是否成功计算

### `RelativeRotations`
(relative-rotations)=
相对旋转的容器类（封装类设计，私有成员变量+公有访问接口）

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取元素数量
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
- <span style="color:#1976d2;font-weight:bold;">clear</span> - 清空所有元素
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - 预留容量
  - n `size_t`：预留大小
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（非const版本）
  - index `size_t`：索引
  - 返回 `RelativeRotation*`：旋转指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（const版本）
  - index `size_t`：索引
  - 返回 `const RelativeRotation*`：旋转常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - 获取旋转（非const版本）
  - index `size_t`：索引
  - 返回 `RelativeRotation&`：旋转引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - 获取旋转（const版本）
  - index `size_t`：索引
  - 返回 `const RelativeRotation&`：旋转常量引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">AddRotation</span> - 添加旋转（拷贝版本）
  - rotation `const RelativeRotation &`：旋转对象
- <span style="color:#1976d2;font-weight:bold;">AddRotation</span> - 添加旋转（移动版本）
  - rotation `RelativeRotation &&`：旋转对象
- <span style="color:#1976d2;font-weight:bold;">EmplaceRotation</span> - 原地构造旋转
  - args `Args &&...`：构造参数
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - 迭代器访问
- <span style="color:#1976d2;font-weight:bold;">push_back</span>, <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - 兼容性方法

### `RelativePoses`
(relative-poses)=
相对位姿的容器类（封装类设计，私有成员变量+公有访问接口）

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取元素数量
- <span style="color:#1976d2;font-weight:bold;">empty</span> - 判断是否为空
- <span style="color:#1976d2;font-weight:bold;">clear</span> - 清空所有元素
- <span style="color:#1976d2;font-weight:bold;">reserve</span> - 预留容量
  - n `size_t`：预留大小
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（非const版本）
  - index `size_t`：索引
  - 返回 `RelativePose*`：位姿指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问（const版本）
  - index `size_t`：索引
  - 返回 `const RelativePose*`：位姿常量指针，越界时返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetPose</span> - 获取位姿（非const版本）
  - index `size_t`：索引
  - 返回 `RelativePose&`：位姿引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">GetPose</span> - 获取位姿（const版本）
  - index `size_t`：索引
  - 返回 `const RelativePose&`：位姿常量引用（越界时抛出异常）
- <span style="color:#1976d2;font-weight:bold;">AddPose</span> - 添加位姿（拷贝版本）
  - pose `const RelativePose &`：位姿对象
- <span style="color:#1976d2;font-weight:bold;">AddPose</span> - 添加位姿（移动版本）
  - pose `RelativePose &&`：位姿对象
- <span style="color:#1976d2;font-weight:bold;">EmplacePose</span> - 原地构造位姿
  - args `Args &&...`：构造参数
- <span style="color:#1976d2;font-weight:bold;">begin</span>, <span style="color:#1976d2;font-weight:bold;">end</span> - 迭代器访问
- <span style="color:#1976d2;font-weight:bold;">EvaluateAgainst</span> - 评估相对位姿精度
  - gt_poses `const RelativePoses &`：真值相对位姿
  - rotation_errors `std::vector<double> &`：输出旋转误差（角度）
  - translation_errors `std::vector<double> &`：输出平移方向误差（角度）
  - 返回 `size_t`：匹配的位姿对数量
- <span style="color:#1976d2;font-weight:bold;">GetRelativePose</span> - 根据ViewPair获取相对位姿
  - view_pair `const ViewPair &`：视图对(i,j)
  - R `Matrix3d &`：输出旋转矩阵
  - t `Vector3d &`：输出平移向量
  - 返回 `bool`：是否找到对应的位姿
- <span style="color:#1976d2;font-weight:bold;">push_back</span>, <span style="color:#1976d2;font-weight:bold;">emplace_back</span> - 兼容性方法

### 相对位姿相关类型定义
(relative-pose-related-types)=
(relative-rotations-ptr)=
(relative-poses-ptr)=

**类型定义**:
| 类型                   | 定义                                                           | 默认值  | 用途                       |
| ---------------------- | -------------------------------------------------------------- | ------- | -------------------------- |
| `RelativeRotationsPtr` | `std::shared_ptr<`[`RelativeRotations`](relative-rotations)`>` | nullptr | 相对旋转容器的智能指针类型 |
| `RelativePosesPtr`     | `std::shared_ptr<`[`RelativePoses`](relative-poses)`>`         | nullptr | 相对位姿容器的智能指针类型 |

### 工具函数
(relative-pose-utils)=

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">Global2RelativePoses</span> - 将全局位姿转换为相对位姿
  - global_poses `const GlobalPoses &`：输入全局位姿
  - relative_poses_output `RelativePoses &`：输出相对位姿
  - 返回 `bool`：转换是否成功

---

## 全局位姿 (types/global_poses.hpp)

### 全局旋转和平移类型
(global-rotation-translation-types)=
(global-rotations)=
(global-rotations-ptr)=
(global-translations)=
(global-translations-ptr)=

**类型定义**:
| 类型                    | 定义                                                                                        | 说明                 |
| ----------------------- | ------------------------------------------------------------------------------------------- | -------------------- |
| `GlobalRotations`       | `std::vector<`[`Matrix3d`](matrix3d)`, Eigen::aligned_allocator<`[`Matrix3d`](matrix3d)`>>` | 全局旋转矩阵序列     |
| `GlobalRotationsPtr`    | `std::shared_ptr<`[`GlobalRotations`](global-rotations)`>`                                  | 全局旋转序列智能指针 |
| `GlobalTranslations`    | `std::vector<`[`Vector3d`](vector3d)`, Eigen::aligned_allocator<`[`Vector3d`](vector3d)`>>` | 全局平移向量序列     |
| `GlobalTranslationsPtr` | `std::shared_ptr<`[`GlobalTranslations`](global-translations)`>`                            | 全局平移序列智能指针 |

### `PoseFormat`
(pose-format)=
(rw-tw)=
(rw-tc)=
(rc-tw)=
(rc-tc)=
位姿坐标系格式枚举，定义不同的相机位姿表示方式：

**格式定义**:
| 格式   | 公式                    | 说明                                       |
| ------ | ----------------------- | ------------------------------------------ |
| `RwTw` | `xc = Rw * (Xw - tw)`   | 世界到相机的旋转，相机在世界坐标系中的位置 |
| `RwTc` | `xc = Rw * Xw + tc`     | 世界到相机的旋转，相机坐标系中的平移向量   |
| `RcTw` | `xc = Rc^T * (Xw - tw)` | 相机到相机的旋转，相机在世界坐标系中的位置 |
| `RcTc` | `xc = Rc^T * Xw + tc`   | 相机到相机的旋转，相机坐标系中的平移向量   |

### `EstInfo`
(est-info)=
(view-state)=
(invalid-state)=
(valid-state)=
(estimated-state)=
(optimized-state)=
(filtered-state)=
视图估计状态信息封装类（封装类设计，私有成员变量+公有访问接口），管理视图估计状态和ID映射关系。

**视图状态枚举 (`ViewState`)**:
- `INVALID` - 无效或未初始化的视图
- `VALID` - 有效但未参与估计的视图
- `ESTIMATED` - 已完成估计的视图
- `OPTIMIZED` - 已完成优化的视图
- `FILTERED` - 被过滤的视图（如离群点）

**私有成员变量**:
| 名称             | 类型                                       | 默认值 | 用途                                       |
| ---------------- | ------------------------------------------ | ------ | ------------------------------------------ |
| `est2origin_id_` | `std::vector<`[`ViewId`](view-id)`>`       | 空向量 | 估计ID到原始ID的映射（仅包含已估计的视图） |
| `origin2est_id_` | `std::vector<`[`ViewId`](view-id)`>`       | 空向量 | 原始ID到估计ID的映射（所有视图）           |
| `view_states_`   | `std::vector<`[`ViewState`](view-state)`>` | 空向量 | 视图状态表（所有视图）                     |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">GetEst2OriginId</span> - 获取est2origin_id映射（const版本）
  - 返回 `const std::vector<ViewId>&`：est2origin_id向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetOrigin2EstId</span> - 获取origin2est_id映射（const版本）
  - 返回 `const std::vector<ViewId>&`：origin2est_id向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetViewStates</span> - 获取view_states表（const版本）
  - 返回 `const std::vector<ViewState>&`：view_states向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">Init</span> - 初始化EstInfo
  - num_views `size_t`：视图数量
- <span style="color:#1976d2;font-weight:bold;">AddEstimatedView</span> - 添加已估计视图
  - original_id `ViewId`：原始视图ID
- <span style="color:#1976d2;font-weight:bold;">SetViewState</span> - 设置视图状态
  - original_id `ViewId`：原始视图ID
  - state `ViewState`：视图状态
- <span style="color:#1976d2;font-weight:bold;">GetViewState</span> - 获取视图状态
  - original_id `ViewId`：原始视图ID
- <span style="color:#1976d2;font-weight:bold;">IsEstimated</span> - 检查视图是否已估计
  - original_id `ViewId`：原始视图ID
- <span style="color:#1976d2;font-weight:bold;">GetNumEstimatedViews</span> - 获取已估计视图数量
- <span style="color:#1976d2;font-weight:bold;">GetViewsInState</span> - 获取特定状态的所有视图ID
  - state `ViewState`：视图状态
- <span style="color:#1976d2;font-weight:bold;">BuildFromTracks</span> - 从Tracks构建EstInfo
  - tracks_ptr `TracksPtr`：轨迹集合智能指针
  - fixed_id `ViewId`：固定视图ID
- <span style="color:#1976d2;font-weight:bold;">CollectValidViews</span> - 收集有效的视图ID
  - tracks_ptr `TracksPtr`：轨迹集合智能指针

### `GlobalPoses`
(global-poses)=
全局位姿管理类（封装类设计，私有成员变量+公有访问接口），存储所有视图的全局位姿信息

**私有成员变量**:
| 名称            | 类型                                        | 默认值          | 用途                   |
| --------------- | ------------------------------------------- | --------------- | ---------------------- |
| `rotations_`    | [`GlobalRotations`](global-rotations)       | 空向量          | 所有视图的旋转矩阵数组 |
| `translations_` | [`GlobalTranslations`](global-translations) | 空向量          | 所有视图的平移向量数组 |
| `est_info_`     | [`EstInfo`](est-info)                       | 默认值          | 估计状态信息           |
| `pose_format_`  | [`PoseFormat`](pose-format)                 | [`RwTw`](rw-tw) | 位姿格式，默认为RwTw   |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">GetRotations</span> - 获取旋转矩阵集合（const版本）
  - 返回 `const GlobalRotations&`：旋转矩阵向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetRotations</span> - 获取旋转矩阵集合（非const版本）
  - 返回 `GlobalRotations&`：旋转矩阵向量的引用
- <span style="color:#1976d2;font-weight:bold;">GetTranslations</span> - 获取平移向量集合（const版本）
  - 返回 `const GlobalTranslations&`：平移向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetTranslations</span> - 获取平移向量集合（非const版本）
  - 返回 `GlobalTranslations&`：平移向量的引用
- <span style="color:#1976d2;font-weight:bold;">Init</span> - 初始化位姿数据
  - num_views `size_t`：视图数量
- <span style="color:#1976d2;font-weight:bold;">GetPoseFormat</span> - 获取当前位姿格式
  - 返回 `PoseFormat`：位姿格式枚举值
- <span style="color:#1976d2;font-weight:bold;">SetPoseFormat</span> - 设置位姿格式
  - format `PoseFormat`：位姿格式
- <span style="color:#1976d2;font-weight:bold;">GetRotation</span> - 获取视图的旋转矩阵(使用原始ID)
  - original_id `ViewId`：原始视图ID
- <span style="color:#1976d2;font-weight:bold;">GetTranslation</span> - 获取视图的平移向量(使用原始ID)
  - original_id `ViewId`：原始视图ID
- <span style="color:#1976d2;font-weight:bold;">GetRotationByEstId</span> - 获取视图的旋转矩阵(使用估计ID)
  - est_id `ViewId`：估计视图ID
- <span style="color:#1976d2;font-weight:bold;">GetTranslationByEstId</span> - 获取视图的平移向量(使用估计ID)
  - est_id `ViewId`：估计视图ID
- <span style="color:#1976d2;font-weight:bold;">SetRotation</span> - 设置视图的旋转矩阵
  - original_id `ViewId`：原始视图ID
  - rotation `const Matrix3d&`：旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">SetTranslation</span> - 设置视图的平移向量
  - original_id `ViewId`：原始视图ID
  - translation `const Vector3d&`：平移向量
- <span style="color:#1976d2;font-weight:bold;">SetRotationByEstId</span> - 使用估计ID设置旋转
  - est_id `ViewId`：估计视图ID
  - rotation `const Matrix3d&`：旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">SetTranslationByEstId</span> - 使用估计ID设置平移
  - est_id `ViewId`：估计视图ID
  - translation `const Vector3d&`：平移向量
- <span style="color:#1976d2;font-weight:bold;">Size</span> - 获取位姿数量
- <span style="color:#1976d2;font-weight:bold;">GetEstInfo</span> - 获取EstInfo引用
- <span style="color:#1976d2;font-weight:bold;">BuildEstInfoFromTracks</span> - 从Tracks构建EstInfo
  - tracks_ptr `const TracksPtr&`：轨迹集合智能指针
  - fixed_id `const ViewId`：固定视图ID（默认0）
- <span style="color:#1976d2;font-weight:bold;">ConvertPoseFormat</span> - 成员函数，用于在不同位姿格式之间转换当前对象的位姿数据
  - target_format `PoseFormat`：目标位姿格式
  - ref_id `ViewId`：参考视图ID（默认0）
  - fixed_id `ViewId`：固定视图ID（默认最大值）
  - 返回 `bool`：转换是否成功
- <span style="color:#1976d2;font-weight:bold;">RwTw_to_RwTc</span> - 成员函数，将当前对象的位姿从RwTw转换为RwTc格式
  - ref_id `ViewId`：参考视图ID（默认0）
  - fixed_id `ViewId`：固定视图ID（默认最大值）
  - 返回 `bool`：转换是否成功
- <span style="color:#1976d2;font-weight:bold;">RwTc_to_RwTw</span> - 成员函数，将当前对象的位姿从RwTc转换为RwTw格式
  - ref_id `ViewId`：参考视图ID（默认0）
  - fixed_id `ViewId`：固定视图ID（默认最大值）
  - 返回 `bool`：转换是否成功
- <span style="color:#1976d2;font-weight:bold;">EvaluateWithSimilarityTransform</span> - 使用相似变换评估位姿精度
  - gt_poses `const GlobalPoses &`：真值全局位姿
  - rotation_errors `std::vector<double> &`：输出旋转误差（角度）
  - translation_errors `std::vector<double> &`：输出平移误差
  - 返回 `bool`：评估是否成功
- <span style="color:#1976d2;font-weight:bold;">EvaluateRotations</span> - 评估旋转精度
  - gt_rotations `const GlobalRotations &`：真值旋转矩阵
  - rotation_errors `std::vector<double> &`：输出旋转误差（角度）
  - 返回 `bool`：评估是否成功
- <span style="color:#1976d2;font-weight:bold;">EvaluateRotationsSVD</span> - 使用SVD方法评估旋转精度
  - gt_rotations `const GlobalRotations &`：真值旋转矩阵
  - rotation_errors `std::vector<double> &`：输出旋转误差（角度）
  - 返回 `bool`：评估是否成功
- <span style="color:#1976d2;font-weight:bold;">NormalizeCameraPositions</span> - 归一化相机位置
  - center_before `Vector3d &`：归一化前的中心位置
  - center_after `Vector3d &`：归一化后的中心位置
  - 返回 `bool`：归一化是否成功
- <span style="color:#1976d2;font-weight:bold;">ComputeCameraPositionCenter</span> - 计算相机位置中心
  - 返回 `Vector3d`：相机位置中心向量

**使用示例**:
```cpp
using namespace PoSDK::types;

// 创建全局位姿对象
GlobalPoses global_poses;

// 初始化3个相机的位姿
global_poses.Init(3);

// 设置位姿格式
global_poses.SetPoseFormat(PoseFormat::RwTw);

// 设置第一个相机位姿（参考相机，通常为单位矩阵）
Matrix3d R0 = Matrix3d::Identity();
Vector3d t0 = Vector3d::Zero();
global_poses.SetRotation(0, R0);
global_poses.SetTranslation(0, t0);

// 设置第二个相机位姿
Matrix3d R1;
R1 << 0.9998, -0.0174, 0.0087,
      0.0175,  0.9998, -0.0052,
     -0.0087,  0.0052,  1.0000;
Vector3d t1(1.0, 0.0, 0.0);
global_poses.SetRotation(1, R1);
global_poses.SetTranslation(1, t1);

// 设置第三个相机位姿
Matrix3d R2;
R2 << 0.9994, -0.0349, 0.0000,
      0.0349,  0.9994, 0.0000,
      0.0000,  0.0000, 1.0000;
Vector3d t2(2.0, 0.0, 0.0);
global_poses.SetRotation(2, R2);
global_poses.SetTranslation(2, t2);

// 标记视图为已估计状态
global_poses.AddEstimatedView(0);
global_poses.AddEstimatedView(1);
global_poses.AddEstimatedView(2);

// 获取位姿信息
const Matrix3d& R_cam1 = global_poses.GetRotation(1);
const Vector3d& t_cam1 = global_poses.GetTranslation(1);

// 检查视图估计状态
bool is_estimated = global_poses.IsEstimated(1);
size_t num_estimated = global_poses.GetNumEstimatedViews();

// 获取所有已估计视图的ID
std::vector<ViewId> estimated_views = global_poses.GetViewsInState(EstInfo::ViewState::ESTIMATED);

// 位姿格式转换
global_poses.ConvertPoseFormat(PoseFormat::RwTc);

// 直接访问旋转和平移向量（用于批量操作）
GlobalRotations& rotations = global_poses.GetRotations();
GlobalTranslations& translations = global_poses.GetTranslations();

// 遍历所有位姿
for (size_t i = 0; i < global_poses.Size(); ++i) {
    if (global_poses.IsEstimated(i)) {
        const Matrix3d& R = global_poses.GetRotation(i);
        const Vector3d& t = global_poses.GetTranslation(i);
        // 处理位姿数据...
    }
}
```

---

## 3D点云类型 (types/world_3dpoints.hpp)

### 基础3D点类型定义
(basic-3d-point-types)=
(points3d)=
(points3d-ptr)=
(point3d)=

**类型定义**:
| 类型          | 定义                                        | 默认值  | 用途                                |
| ------------- | ------------------------------------------- | ------- | ----------------------------------- |
| `Points3d`    | `Eigen::Matrix<double, 3, Eigen::Dynamic>`  | 零矩阵  | 3D点矩阵，3×N矩阵，每列代表一个3D点 |
| `Points3dPtr` | `std::shared_ptr<`[`Points3d`](points3d)`>` | nullptr | 3D点矩阵的智能指针                  |
| `Point3d`     | [`Vector3d`](vector3d)                      | 零向量  | 单个3D点类型                        |

### `WorldPointInfo`
(world-point-info)=
世界坐标系下的3D点信息封装类，提供私有成员和公有访问接口

**私有成员变量**:
| 名称            | 类型                                  | 默认值 | 用途                              |
| --------------- | ------------------------------------- | ------ | --------------------------------- |
| `world_points_` | [`Points3d`](points3d)                | 零矩阵 | 世界坐标点集合(3×N矩阵)           |
| `ids_used_`     | `std::vector<bool>`                   | 空向量 | 点是否被使用的标志位数组          |
| `colors_rgb_`   | `std::vector<std::array<uint8_t, 3>>` | 空向量 | 每个点的RGB颜色（可选，默认为空） |

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">WorldPointInfo</span> - 默认构造函数
- <span style="color:#1976d2;font-weight:bold;">WorldPointInfo</span> - 构造函数，初始化指定数量的点
  - num_points `size_t`：点的数量
- <span style="color:#1976d2;font-weight:bold;">resize</span> - 调整大小
  - num_points `size_t`：新的点数量
- <span style="color:#1976d2;font-weight:bold;">size</span> - 获取点数量
- <span style="color:#1976d2;font-weight:bold;">setPoint</span> - 设置点坐标
  - index `size_t`：点索引
  - point `const Point3d &`：3D点坐标
- <span style="color:#1976d2;font-weight:bold;">getPoint</span> - 获取点坐标
  - index `size_t`：点索引
- <span style="color:#1976d2;font-weight:bold;">setUsed</span> - 设置点使用状态
  - index `size_t`：点索引
  - used `bool`：是否使用
- <span style="color:#1976d2;font-weight:bold;">isUsed</span> - 获取点使用状态
  - index `size_t`：点索引
- <span style="color:#1976d2;font-weight:bold;">getValidPointsCount</span> - 获取有效点数量
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问返回指针（const版本）
  - index `size_t`：点索引
  - 返回 `const Vector3d*`：点坐标指针，如果索引无效或点未使用则返回nullptr
- <span style="color:#1976d2;font-weight:bold;">operator[]</span> - 数组式访问返回指针（非const版本）
  - index `size_t`：点索引
  - 返回 `Vector3d*`：点坐标指针，如果索引无效或点未使用则返回nullptr
- <span style="color:#1976d2;font-weight:bold;">GetWorldPoints</span> - 获取世界坐标点矩阵（const版本）
  - 返回 `const Points3d&`：世界坐标点矩阵的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetWorldPoints</span> - 获取世界坐标点矩阵（非const版本）
  - 返回 `Points3d&`：世界坐标点矩阵的引用
- <span style="color:#1976d2;font-weight:bold;">GetIdsUsed</span> - 获取ids_used向量（const版本）
  - 返回 `const std::vector<bool>&`：ids_used向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetIdsUsed</span> - 获取ids_used向量（非const版本）
  - 返回 `std::vector<bool>&`：ids_used向量的引用
- <span style="color:#1976d2;font-weight:bold;">HasColors</span> - 检查是否存在颜色信息
  - 返回 `bool`：是否存在颜色信息
- <span style="color:#1976d2;font-weight:bold;">GetColorCount</span> - 获取颜色数量
  - 返回 `size_t`：颜色数量
- <span style="color:#1976d2;font-weight:bold;">GetColorRGB</span> - 获取指定点的颜色
  - index `size_t`：点索引
  - 返回 `const std::array<uint8_t, 3>&`：RGB颜色数组
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - 设置指定点的颜色（方式1）
  - index `size_t`：点索引
  - r `uint8_t`：红色分量(0-255)
  - g `uint8_t`：绿色分量(0-255)
  - b `uint8_t`：蓝色分量(0-255)
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">SetColorRGB</span> - 设置指定点的颜色（方式2）
  - index `size_t`：点索引
  - rgb `const std::array<uint8_t, 3>&`：RGB颜色数组
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">InitializeColors</span> - 初始化所有点的颜色
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">ClearColors</span> - 清除所有颜色信息
  - 返回 `void`
- <span style="color:#1976d2;font-weight:bold;">GetColorsRGB</span> - 获取colors_rgb向量（const版本）
  - 返回 `const std::vector<std::array<uint8_t, 3>>&`：colors_rgb向量的常量引用
- <span style="color:#1976d2;font-weight:bold;">GetColorsRGB</span> - 获取colors_rgb向量（非const版本）
  - 返回 `std::vector<std::array<uint8_t, 3>>&`：colors_rgb向量的引用
- <span style="color:#1976d2;font-weight:bold;">EvaluateWith3DPoints</span> - 评估3D点重建精度
  - gt_points `const WorldPointInfo &`：真值WorldPointInfo
  - position_errors `std::vector<double> &`：输出位置误差
  - 返回 `bool`：评估是否成功

**使用示例**:
```cpp
using namespace PoSDK::types;

// 方式1：默认构造后设置点
WorldPointInfo point_cloud;
point_cloud.resize(5);  // 初始化5个点

// 设置点坐标
point_cloud.setPoint(0, Point3d(1.0, 0.0, 0.0));
point_cloud.setPoint(1, Point3d(0.0, 1.0, 0.0));
point_cloud.setPoint(2, Point3d(0.0, 0.0, 1.0));
point_cloud.setPoint(3, Point3d(1.0, 1.0, 0.0));
point_cloud.setPoint(4, Point3d(1.0, 0.0, 1.0));

// 设置点的使用状态
point_cloud.setUsed(0, true);
point_cloud.setUsed(1, true);
point_cloud.setUsed(2, false);  // 第3个点标记为不使用
point_cloud.setUsed(3, true);
point_cloud.setUsed(4, true);

// 方式2：构造时指定数量
WorldPointInfo point_cloud2(10);  // 直接创建10个点（默认全部启用）

// 获取点信息
size_t total_points = point_cloud.size();              // 总点数：5
size_t valid_points = point_cloud.getValidPointsCount();  // 有效点数：4

// 获取和检查单个点
Point3d point0 = point_cloud.getPoint(0);
bool is_used = point_cloud.isUsed(2);  // false

// 使用operator[]便捷访问（推荐用于需要nullptr检查的场景）
// Use operator[] for convenient access (recommended when nullptr check is needed)
const Vector3d* pt0 = point_cloud[0];  // 返回指针，点存在且被使用
if (pt0) {
    std::cout << "点0坐标: " << pt0->transpose() << std::endl;
    double x = (*pt0)(0);  // 访问x坐标
}

const Vector3d* pt2 = point_cloud[2];  // 返回nullptr，因为点未被使用
if (!pt2) {
    std::cout << "点2未使用，返回nullptr" << std::endl;
}

const Vector3d* pt_invalid = point_cloud[100];  // 返回nullptr，索引越界
if (!pt_invalid) {
    std::cout << "索引越界，返回nullptr" << std::endl;
}

// 遍历所有有效点（使用operator[]）
for (size_t i = 0; i < point_cloud.size(); ++i) {
    const Vector3d* pt = point_cloud[i];
    if (pt) {
        // 处理有效点...
        double distance = pt->norm();
    }
}

// 遍历所有有效点（传统方法）
for (size_t i = 0; i < point_cloud.size(); ++i) {
    if (point_cloud.isUsed(i)) {
        Point3d point = point_cloud.getPoint(i);
        // 处理有效点...
    }
}

// 直接访问底层数据（用于批量操作）
const Points3d& points_matrix = point_cloud.GetWorldPoints();  // 3×N矩阵
const std::vector<bool>& usage_flags = point_cloud.GetIdsUsed();

// 批量设置点坐标（高效方式 - SIMD优化）
// Batch point setting (efficient way - SIMD optimized)
Points3d& points_ref = point_cloud.GetWorldPoints();
for (int i = 0; i < points_ref.cols(); ++i) {
    points_ref.col(i) = Point3d::Random();  // 设置随机坐标
}

// 评估重建精度（与真值比较）
WorldPointInfo gt_points(5);
// ... 设置真值点 ...

std::vector<double> position_errors;
bool success = point_cloud.EvaluateWith3DPoints(gt_points, position_errors);
if (success) {
    // 计算平均误差
    double avg_error = 0.0;
    for (double err : position_errors) {
        avg_error += err;
    }
    avg_error /= position_errors.size();
}
```

### 3D点云相关类型定义
(world-point-related-types)=
(world-point-info-ptr)=

**类型定义**:
| 类型                | 定义                                                      | 默认值  | 用途                     |
| ------------------- | --------------------------------------------------------- | ------- | ------------------------ |
| `WorldPointInfoPtr` | `std::shared_ptr<`[`WorldPointInfo`](world-point-info)`>` | nullptr | 世界坐标点信息的智能指针 |


---

## 相似变换 (types/similarity_transform.hpp)

### 基础类型定义
(similarity-transform-types)=
(cayley-params)=
(quaternion)=

**类型定义**:
| 类型           | 定义                   | 默认值 | 用途                  |
| -------------- | ---------------------- | ------ | --------------------- |
| `CayleyParams` | [`Vector3d`](vector3d) | 零向量 | Cayley参数（3维向量） |
| `Quaternion`   | `Eigen::Vector4d`      | 零向量 | 四元数（4维向量）     |

### `SimilarityTransformError`
(similarity-transform-error)=
相似变换优化的Ceres代价函数

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">SimilarityTransformError</span> - 构造函数
  - src_point `const Vector3d &`：源点
  - dst_point `const Vector3d &`：目标点
- <span style="color:#1976d2;font-weight:bold;">operator()</span> - 代价函数计算操作符
  - scale `const T *const`：尺度参数
  - rotation `const T *const`：旋转参数
  - translation `const T *const`：平移参数
  - residuals `T *`：残差输出
- <span style="color:#1976d2;font-weight:bold;">Create</span> - 工厂函数
  - src_point `const Vector3d &`：源点
  - dst_point `const Vector3d &`：目标点

### 相似变换模板函数
(similarity-transform-functions)=

**可用方法**:
- <span style="color:#1976d2;font-weight:bold;">ComputeSimilarityTransform</span> - 模板函数：计算两组3D点间的最优相似变换（使用Ceres优化）
  - src_points `const PointContainer &`：源点集合
  - dst_points `const PointContainer &`：目标点集合
  - src_points_transformed `PointContainer &`：变换后的源点集合
  - scale `double &`：输出尺度因子
  - rotation `Matrix3d &`：输出旋转矩阵
  - translation `Vector3d &`：输出平移向量
  - 返回 `bool`：是否成功
- <span style="color:#1976d2;font-weight:bold;">ComputeSimilarityTransform</span> - 计算两组位姿间的最优相似变换（为保持向后兼容性）
  - src_centers `const std::vector<Vector3d> &`：源位置向量
  - dst_centers `const std::vector<Vector3d> &`：目标位置向量
  - src_centers_transformed `std::vector<Vector3d> &`：变换后的源位置向量
  - scale `double &`：输出尺度因子
  - rotation `Matrix3d &`：输出旋转矩阵
  - translation `Vector3d &`：输出平移向量
  - 返回 `bool`：是否成功

### 数学工具函数
(similarity-transform-math-utils)=

**可用函数**:
- <span style="color:#1976d2;font-weight:bold;">CayleyToRotation</span> - 将Cayley参数转换为旋转矩阵
  - cayley `const CayleyParams &`：Cayley参数（3维向量）
  - 返回 `Matrix3d`：旋转矩阵
- <span style="color:#1976d2;font-weight:bold;">RotationToCayley</span> - 将旋转矩阵转换为Cayley参数
  - R `const Matrix3d &`：旋转矩阵
  - 返回 `CayleyParams`：Cayley参数（3维向量）
- <span style="color:#1976d2;font-weight:bold;">RelativePoseToParams</span> - 将相对位姿转换为参数向量（6自由度：3旋转+3平移）
  - pose `const RelativePose &`：相对位姿
  - 返回 `Eigen::Matrix<double, 6, 1>`：6维参数向量（前3维为Cayley参数，后3维为平移）
- <span style="color:#1976d2;font-weight:bold;">ParamsToRelativePose</span> - 将参数向量转换为相对位姿
  - params `const Eigen::Matrix<double, 6, 1> &`：6维参数向量（前3维为Cayley参数，后3维为平移）
  - 返回 `RelativePose`：相对位姿

---

