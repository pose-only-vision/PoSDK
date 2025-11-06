(prior-and-gt)=
# 真值设置与评估 | Ground Truth and Evaluation

本文档介绍如何在PoSDK方法插件中设置真值数据（Ground Truth）进行算法评估，以及数据包操作的核心功能。

## 快速导航 | Quick Navigation

### 核心概念
- [数据包管理机制](#data-type-classification)
- [真值评估系统](#gt-evaluation-system)

### 操作函数列表 | Function List

#### 必需输入数据包操作 | Required Package Operations
| 函数名                    | 功能简述                     | 详细链接                      |
| ------------------------- | ---------------------------- | ----------------------------- |
| `SetRequiredData()`       | 设置单个必需输入数据         | [详情](#setrequireddata)      |
| `GetRequiredPackage()`    | 获取必需输入数据包引用       | [详情](#getrequiredpackage)   |
| `SetRequiredPackage()`    | 批量设置必需输入数据包       | [详情](#setrequiredpackage)   |
| `GetRequiredDataTypes()`  | 获取必需输入数据类型列表     | [详情](#getrequireddatatypes) |
| `GetRequiredDataPtr<T>()` | 简化获取必需输入数据（推荐） | [详情](#getrequireddataptr)   |

#### 输出数据包操作 | Output Package Operations
```{warning}
**预留接口 | Reserved Interface**

以下输出数据包操作函数为预留接口，当前版本尚未实现。算法应直接通过`Build()`或`Run()`的返回值获取结果数据。

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```

| 函数名                 | 功能简述             | 详细链接                    | 状态     |
| ---------------------- | -------------------- | --------------------------- | -------- |
| `GetOutputPackage()`   | 获取输出数据包指针   | [详情](#getoutputpackage)   | 预留接口 |
| `AddOutputDataPtr()`   | 添加数据到输出包     | [详情](#addoutputdataptr)   | 预留接口 |
| `GetOutputDataTypes()` | 获取输出数据类型列表 | [详情](#getoutputdatatypes) | 预留接口 |
| `ClearOutputPackage()` | 清空输出数据包       | [详情](#clearoutputpackage) | 预留接口 |

#### 真值数据操作 | Ground Truth Operations
| 函数名        | 功能简述     | 详情链接           |
| ------------- | ------------ | ------------------ |
| `SetGTData()` | 设置真值数据 | [详情](#setgtdata) |
| `GetGTData()` | 获取真值数据 | [详情](#getgtdata) |

#### 评估器操作 | Evaluator Operations
| 函数名                    | 功能简述         | 详情链接                       |
| ------------------------- | ---------------- | ------------------------------ |
| `SetEvaluatorAlgorithm()` | 设置评估算法名称 | [详情](#setevaluatoralgorithm) |
| `GetEvaluatorAlgorithm()` | 获取评估算法名称 | [详情](#getevaluatoralgorithm) |
| `CallEvaluator()`         | 手动调用评估器   | [详情](#callevaluator)         |

### 操作函数导航
- [必需输入数据包操作](#required-package-functions)
- [输出数据包操作](#output-package-functions)
- [真值数据操作](#gt-data-functions)
- [评估器相关函数](#evaluator-functions)

### 应用示例
- [基础数据包操作](#example-basic-package)
- [真值评估流程](#example-gt-evaluation)

---

(data-type-classification)=
## 数据包管理机制 | Data Package Management

PoSDK的`MethodPreset`类提供了两种核心数据管理机制：

### 1. 必需输入数据包 (`required_package_`)
- 算法**必须**的输入数据
- 在`Build()`调用前必须提供
- 如果缺失会导致算法无法执行
- 通过`required_package_["key"]`访问

### 2. 输出数据包 (`output_package_`)
```{warning}
**预留功能 | Reserved Feature**

输出数据包功能为预留接口，当前版本尚未实现。算法应直接通过`Build()`或`Run()`的返回值获取结果数据。

Output package functionality is a reserved interface and is not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```
- 算法执行后的**结果数据**（预留）
- 通过`AddOutputDataPtr()`添加结果（预留）
- 可包含多种类型的输出数据（预留）
- 通过`GetOutputPackage()`获取（预留）

(gt-evaluation-system)=
### 真值评估系统 | Ground Truth Evaluation System

- **真值数据 (Ground Truth)**: 用于精度评估和结果验证
- 通常在开发和测试阶段使用
- 不影响算法的实际执行
- 通过`SetGTData()`/`GetGTData()`访问
- 配合评估器进行自动精度分析（当`enable_evaluator=true`时）

### 关键区别对比 | Key Differences

| 特性         | 必需输入数据包            | 输出数据包（预留）           | 真值数据      |
| ------------ | ------------------------- | ---------------------------- | ------------- |
| **方向**     | 输入                      | 输出                         | 评估          |
| **是否必需** | **必需**                  | 算法产生（预留）             | 可选          |
| **用途**     | 算法的核心输入            | 算法的执行结果（预留）       | 精度评估      |
| **缺失影响** | 算法无法运行              | 无输出结果（预留）           | 仅影响评估    |
| **典型内容** | 特征、轨迹、相机参数      | 位姿、重建结果（预留）       | 标准答案      |
| **访问方式** | `GetRequiredDataPtr<T>()` | `AddOutputDataPtr()`（预留） | `SetGTData()` |
| **设置时机** | 执行前设置                | 执行过程中产生（预留）       | 执行前设置    |
| **当前状态** | ✓ 已实现                  | ⚠️ 预留接口，未实现           | ✓ 已实现      |

---

## 操作函数详解 | Function Details

(required-package-functions)=
### 必需输入数据包操作函数 | Required Package Functions

(setrequireddata)=
#### `SetRequiredData()`
**函数签名 | Function Signature**:
```cpp
bool SetRequiredData(const DataPtr &data_ptr);
```

**用途 | Purpose**: 设置单个必需输入数据 | Set a single required input data

**参数 | Parameters**:
- `data_ptr` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): 要设置的数据指针 | Data pointer to set

**返回值 | Return**: `bool` - 是否设置成功 | Whether setting was successful

**使用示例 | Usage Example**:
```cpp
auto tracks_data = LoadTracks("tracks.bin");
method_ptr->SetRequiredData(tracks_data);
```

---

(getrequiredpackage)=
#### `GetRequiredPackage()`
**函数签名 | Function Signature**:
```cpp
const Package& GetRequiredPackage() const;  // const版本
Package& GetRequiredPackage();              // 非const版本
```

**用途 | Purpose**: 获取必需输入数据包引用 | Get reference to required input package

**返回值 | Return**: [`Package`](../appendices/appendix_a_types.md#package) & - 必需输入数据包的引用 | Reference to required package

**使用示例 | Usage Example**:
```cpp
const auto& req_pkg = method_ptr->GetRequiredPackage();
for (const auto& [key, data_ptr] : req_pkg) {
    std::cout << "Required data: " << key << std::endl;
}
```

---

(setrequiredpackage)=
#### `SetRequiredPackage()`
**函数签名 | Function Signature**:
```cpp
void SetRequiredPackage(const Package &package);
```

**用途 | Purpose**: 批量设置必需输入数据包 | Batch set required input package

**参数 | Parameters**:
- `package` [`Package`](../appendices/appendix_a_types.md#package): 要设置的数据包 | Package to set

**使用示例 | Usage Example**:
```cpp
Package input_package;
input_package["data_tracks"] = tracks_ptr;
input_package["data_camera_models"] = cameras_ptr;
method_ptr->SetRequiredPackage(input_package);
```

---

(getrequireddatatypes)=
#### `GetRequiredDataTypes()`
**函数签名 | Function Signature**:
```cpp
std::vector<std::string> GetRequiredDataTypes() const;
```

**用途 | Purpose**: 获取必需输入数据的类型列表 | Get list of required input data types

**返回值 | Return**: `std::vector<std::string>` - 必需输入数据类型名称列表 | List of required data type names

**使用示例 | Usage Example**:
```cpp
auto required_types = method_ptr->GetRequiredDataTypes();
for (const auto& type : required_types) {
    std::cout << "Required type: " << type << std::endl;
}
```

---

(getrequireddataptr)=
#### `GetRequiredDataPtr<T>()`  推荐使用 | Recommended
**函数签名 | Function Signature**:
```cpp
template <typename T>
std::shared_ptr<T> GetRequiredDataPtr(const std::string &data_name) const;
```

**用途 | Purpose**: 简化获取必需输入数据，自动类型转换 | Simplified access to required input data with automatic type conversion

**参数 | Parameters**:
- `data_name` `std::string`: required_package_中的数据名称 | Data name in required_package_

**返回值 | Return**: `std::shared_ptr<T>` - 指向数据的智能指针 | Smart pointer to the data

**使用示例 | Usage Example**:
```cpp
// 在MethodPreset派生类中使用 | Usage in MethodPreset-derived class:
auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
auto features = GetRequiredDataPtr<FeaturesInfo>("data_features");

// 或者通过dynamic_pointer_cast | Or through dynamic_pointer_cast:
auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method_ptr);
if (method_preset) {
    auto poses = method_preset->GetRequiredDataPtr<GlobalPoses>("data_global_poses");
}
```

```{tip}
**推荐使用方式 | Recommended Usage**

`GetRequiredDataPtr<T>()` 提供了更简洁的API，自动处理Package查找和类型转换，减少代码冗余。

`GetRequiredDataPtr<T>()` provides a more concise API, automatically handles Package lookup and type conversion, reducing code redundancy.
```

---

(output-package-functions)=
### 输出数据包操作函数 | Output Package Functions

```{warning}
**预留接口 | Reserved Interface**

以下输出数据包操作函数为预留接口，当前版本尚未实现。算法应直接通过`Build()`或`Run()`的返回值获取结果数据。

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```

(getoutputpackage)=
#### `GetOutputPackage()` ⚠️ 预留接口
**函数签名 | Function Signature**:
```cpp
DataPackagePtr GetOutputPackage() const;
```

**用途 | Purpose**: 获取输出数据包指针（预留）| Get output data package pointer (reserved)

**返回值 | Return**: [`DataPackagePtr`](../appendices/appendix_a_types.md#data-package-ptr) - 输出数据包的智能指针 | Smart pointer to output package

**当前状态 | Current Status**: ⚠️ 预留接口，尚未实现 | Reserved interface, not yet implemented

---

(addoutputdataptr)=
#### `AddOutputDataPtr()` ⚠️ 预留接口
**函数签名 | Function Signature**:
```cpp
void AddOutputDataPtr(const DataPtr &data_ptr, const std::string &data_name = "");
```

**用途 | Purpose**: 添加数据到输出包（预留）| Add data to output package (reserved)

**参数 | Parameters**:
- `data_ptr` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): 要添加的数据指针 | Data pointer to add
- `data_name` `std::string`: 数据名称（可选，如果为空则使用data_ptr的类型名）| Data name (optional, uses data_ptr type if empty)

**当前状态 | Current Status**: ⚠️ 预留接口，尚未实现 | Reserved interface, not yet implemented

---

(getoutputdatatypes)=
#### `GetOutputDataTypes()` ⚠️ 预留接口
**函数签名 | Function Signature**:
```cpp
std::vector<std::string> GetOutputDataTypes() const;
```

**用途 | Purpose**: 获取输出数据的类型列表（预留）| Get list of output data types (reserved)

**返回值 | Return**: `std::vector<std::string>` - 输出数据类型名称列表 | List of output data type names

**当前状态 | Current Status**: ⚠️ 预留接口，尚未实现 | Reserved interface, not yet implemented

---

(clearoutputpackage)=
#### `ClearOutputPackage()` ⚠️ 预留接口
**函数签名 | Function Signature**:
```cpp
void ClearOutputPackage();
```

**用途 | Purpose**: 清空输出数据包（预留）| Clear output data package (reserved)

**当前状态 | Current Status**: ⚠️ 预留接口，尚未实现 | Reserved interface, not yet implemented

---

(gt-data-functions)=
### 真值数据操作函数 | Ground Truth Functions

(setgtdata)=
#### `SetGTData()`
**函数签名 | Function Signature**:
```cpp
void SetGTData(DataPtr &gt_data);
```

**用途 | Purpose**: 设置用于评估的真值数据 | Set ground truth data for evaluation

**参数 | Parameters**:
- `gt_data` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): 真值数据指针 | Ground truth data pointer

**使用示例 | Usage Example**:
```cpp
// 加载真值数据用于评估
auto gt_poses = LoadGroundTruthPoses("ground_truth.txt");
method_ptr->SetGTData(gt_poses);

// 如果enable_evaluator=true，算法会自动与真值比较
auto result = method_ptr->Build(input_data);
```

---

(getgtdata)=
#### `GetGTData()`
**函数签名 | Function Signature**:
```cpp
DataPtr GetGTData() const;
```

**用途 | Purpose**: 获取当前设置的真值数据 | Get currently set ground truth data

**返回值 | Return**: [`DataPtr`](../appendices/appendix_a_types.md#ptr-type) - 真值数据指针 | Ground truth data pointer

**使用示例 | Usage Example**:
```cpp
auto gt_data = method_ptr->GetGTData();
if (gt_data) {
    std::cout << "Ground truth data is set" << std::endl;
}
```

---

(evaluator-functions)=
### 评估器相关函数 | Evaluator Functions

(setevaluatoralgorithm)=
#### `SetEvaluatorAlgorithm()`
**函数签名 | Function Signature**:
```cpp
virtual void SetEvaluatorAlgorithm(const std::string &algorithm_name);
```

**用途 | Purpose**: 设置评估算法名称标识 | Set algorithm name identifier for evaluation

**参数 | Parameters**:
- `algorithm_name` `std::string`: 算法名称（用于区分不同算法配置）| Algorithm name (to distinguish different configurations)

**使用示例 | Usage Example**:
```cpp
// 用于在评估结果中标识不同的算法配置
method_ptr->SetEvaluatorAlgorithm("PoSDK_v2");
```

---

(getevaluatoralgorithm)=
#### `GetEvaluatorAlgorithm()`
**函数签名 | Function Signature**:
```cpp
virtual std::string GetEvaluatorAlgorithm() const;
```

**用途 | Purpose**: 获取评估算法名称 | Get algorithm name for evaluation

**返回值 | Return**: `std::string` - 算法名称（默认返回`GetType()`的值）| Algorithm name (defaults to `GetType()` value)

---

(callevaluator)=
#### `CallEvaluator()`
**函数签名 | Function Signature**:
```cpp
bool CallEvaluator(const DataPtr &result_data);
```

**用途 | Purpose**: 手动调用评估器进行精度评估 | Manually call evaluator for accuracy evaluation

**参数 | Parameters**:
- `result_data` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): 待评估的结果数据 | Result data to evaluate

**返回值 | Return**: `bool` - 评估是否成功 | Whether evaluation was successful

**使用示例 | Usage Example**:
```cpp
// 通常不需要手动调用，当enable_evaluator=true时，Build()会自动调用
// Usually no need to call manually, Build() calls automatically when enable_evaluator=true
auto result = method_ptr->Run();
bool eval_success = method_ptr->CallEvaluator(result);
```

```{note}
**自动评估 | Automatic Evaluation**

`CallEvaluator()` 会在以下条件下由 `MethodPresetProfiler::Build()` 自动调用：
`CallEvaluator()` is automatically called by `MethodPresetProfiler::Build()` when:
- `enable_evaluator` 选项设置为 `true`（默认：`false`）
- `enable_evaluator` option is set to `true` (default: `false`)
- 结果数据不为空
- Result data is not null
- 已通过 `SetGTData()` 设置真值数据
- Ground truth data is set via `SetGTData()`

如果 `enable_evaluator` 为 `false`，需要在 `Build()` 或 `Run()` 后手动调用 `CallEvaluator()`。
If `enable_evaluator` is `false`, you need to manually call `CallEvaluator()` after `Build()` or `Run()`.
```

---

## 应用示例 | Application Examples

(example-basic-package)=
### 示例1：基础数据包操作 | Example 1: Basic Package Operations

演示必需输入数据包的基本使用：

```cpp
class MyPoseEstimator : public MethodPresetProfiler {
public:
    MyPoseEstimator() {
        // 声明必需输入数据
        required_package_["data_tracks"] = nullptr;
        required_package_["data_camera_models"] = nullptr;
    }

    DataPtr Run() override {
        // 获取必需输入数据
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");

        // 执行位姿估计算法
        auto result_poses = EstimatePoses(tracks, cameras);

        LOG_INFO_ZH << "位姿估计完成，输出" << result_poses->Size() << "个位姿";
        LOG_INFO_EN << "Pose estimation completed, output " << result_poses->Size() << " poses";

        // 直接返回结果数据
        return result_poses;
    }
};

// 使用方式
auto estimator = FactoryMethod::Create("my_pose_estimator");

// 设置必需输入
estimator->SetRequiredData(tracks_data);
estimator->SetRequiredData(camera_models);

// 执行算法，直接获取返回值作为结果
auto result = estimator->Build();
if (result) {
    auto poses = GetDataPtr<GlobalPoses>(result);
    // 使用结果数据...
}
```

---

(example-gt-evaluation)=
### 示例2：真值评估流程 | Example 2: Ground Truth Evaluation

演示如何设置真值数据并进行自动评估：

```cpp
class MyBundleAdjustment : public MethodPresetProfiler {
public:
    MyBundleAdjustment() {
        required_package_["data_tracks"] = nullptr;
        required_package_["data_camera_models"] = nullptr;
        required_package_["data_initial_poses"] = nullptr;
    }

    DataPtr Run() override {
        // 获取输入数据
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");
        auto initial_poses = GetRequiredDataPtr<GlobalPoses>("data_initial_poses");

        // 执行Bundle Adjustment优化
        auto optimized_poses = OptimizePoses(tracks, cameras, initial_poses);

        LOG_INFO_ZH << "Bundle Adjustment优化完成";
        LOG_INFO_EN << "Bundle Adjustment optimization completed";

        // 直接返回结果数据
        return optimized_poses;
    }
};

// 完整的评估流程
int main() {
    // 1. 创建算法实例
    auto ba_method = FactoryMethod::Create("my_bundle_adjustment");

    // 2. 设置算法名称标识
    ba_method->SetEvaluatorAlgorithm("PoSDK_BA_v2.1");

    // 3. 设置必需输入
    auto tracks = LoadTracks("input_tracks.bin");
    auto cameras = LoadCameras("cameras.txt");
    auto initial_poses = LoadInitialPoses("initial.txt");

    ba_method->SetRequiredData(tracks);
    ba_method->SetRequiredData(cameras);
    ba_method->SetRequiredData(initial_poses);

    // 4. 设置真值数据用于评估
    auto gt_poses = LoadGroundTruthPoses("ground_truth_poses.txt");
    auto gt_points = LoadGroundTruthPoints("ground_truth_points.txt");

    ba_method->SetGTData(gt_poses);  // 设置位姿真值

    // 5. 启用自动评估（可选，默认为false）
    ba_method->SetMethodOption("enable_evaluator", "true");

    // 6. 执行算法（如果enable_evaluator=true，会自动调用评估器）
    auto result = ba_method->Build();

    // 7. 查看评估结果（评估器会自动输出到控制台和文件）
    LOG_INFO_ZH << "算法执行完成，评估结果已保存";
    LOG_INFO_EN << "Algorithm completed, evaluation results saved";

    // 8. 使用返回的结果数据
    if (result) {
        auto optimized_poses = GetDataPtr<GlobalPoses>(result);
        
        // 手动进行额外评估（可选）
        // 注意：CallEvaluator的参数应该是结果数据，评估器会自动从GetGTData()获取真值进行比较
        // ba_method->CallEvaluator(result);
        
        // 使用优化后的位姿...
    }

    return 0;
}
```

### 真值数据准备指南 | Ground Truth Data Preparation

```cpp
// 真值位姿数据准备示例
void PrepareGroundTruthData() {
    // 1. 创建真值位姿
    auto gt_poses = FactoryData::Create("global_poses");
    gt_poses->Init(5);  // 5个相机位姿

    // 设置参考相机位姿（通常为单位变换）
    Matrix3d R_ref = Matrix3d::Identity();
    Vector3d t_ref = Vector3d::Zero();
    gt_poses->SetRotation(0, R_ref);
    gt_poses->SetTranslation(0, t_ref);

    // 设置其他相机的真值位姿
    for (int i = 1; i < 5; ++i) {
        Matrix3d R_gt = LoadRotationFromFile("gt_rotation_" + std::to_string(i) + ".txt");
        Vector3d t_gt = LoadTranslationFromFile("gt_translation_" + std::to_string(i) + ".txt");
        gt_poses->SetRotation(i, R_gt);
        gt_poses->SetTranslation(i, t_gt);
    }

    // 2. 创建真值3D点
    auto gt_points = FactoryData::Create("world_point_info");
    gt_points->resize(1000);  // 1000个3D点

    for (size_t i = 0; i < 1000; ++i) {
        Point3d gt_point = LoadPointFromFile("gt_point_" + std::to_string(i) + ".txt");
        gt_points->setPoint(i, gt_point);
        gt_points->setUsed(i, true);
    }

    // 3. 保存真值数据供算法使用
    gt_poses->Save("ground_truth_poses.posdk");
    gt_points->Save("ground_truth_points.posdk");

    LOG_INFO_ZH << "真值数据准备完成：" << gt_poses->Size() << "个位姿，"
                << gt_points->getValidPointsCount() << "个3D点";
    LOG_INFO_EN << "Ground truth data prepared: " << gt_poses->Size() << " poses, "
                << gt_points->getValidPointsCount() << " 3D points";
}
```

---

## 最佳实践 | Best Practices

### 1. 清晰的数据包管理

```cpp
// 推荐：明确区分输入数据，直接返回结果
class MyAlgorithm : public MethodPresetProfiler {
public:
    MyAlgorithm() {
        // 必需输入数据声明
        required_package_["data_tracks"] = nullptr;
        required_package_["data_cameras"] = nullptr;
    }

    DataPtr Run() override {
        // 获取输入数据
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_cameras");

        // 算法处理
        auto result = ProcessData(tracks, cameras);

        // 直接返回结果数据
        return result;
    }
};

// ❌ 不推荐：混淆输入输出管理
// 不要在构造函数外动态添加required_package_项目
// ⚠️ 注意：当前版本不支持AddOutputDataPtr()，应直接返回结果
```

### 2. 完善的真值评估

```cpp
// 推荐：完整的评估流程设置
auto algorithm = FactoryMethod::Create("my_algorithm");

// 设置算法标识（用于评估结果区分）
algorithm->SetEvaluatorAlgorithm("MyAlgo_v1.2");

// 设置输入数据
algorithm->SetRequiredData(input_tracks);
algorithm->SetRequiredData(camera_models);

// 设置真值数据
auto gt_poses = LoadGroundTruth("gt_poses.txt");
algorithm->SetGTData(gt_poses);

// 启用自动评估（可选）
algorithm->SetMethodOption("enable_evaluator", "true");

// 执行并自动评估
auto result = algorithm->Build();

// ❌ 不推荐：忽略真值设置
// 缺少SetGTData()会导致无法进行精度评估
```

### 3. 错误处理和验证

```cpp
// 推荐：检查数据有效性
DataPtr Run() override {
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    if (!tracks || tracks->size() == 0) {
        LOG_ERROR_ZH << "输入轨迹数据无效";
        LOG_ERROR_EN << "Invalid input tracks data";
        return nullptr;
    }

    auto result = ProcessTracks(tracks);

    // 验证输出数据
    if (!result || !IsValidResult(result)) {
        LOG_ERROR_ZH << "算法输出结果无效";
        LOG_ERROR_EN << "Algorithm output is invalid";
        return nullptr;
    }

    return result;
}

// ❌ 不推荐：缺少数据验证
// 不检查输入数据有效性可能导致算法崩溃
```

### 4. 文档和README规范

在插件的README中应明确说明：

```markdown
## 数据接口 | Data Interface

### 必需输入 | Required Inputs
- `data_tracks`: 特征轨迹数据类型 - [`Tracks`](link-to-tracks-doc)
- `data_camera_models`: 相机参数 - [`CameraModels`](link-to-camera-doc)

### 输出数据 | Output Data
- 算法通过`Build()`或`Run()`的返回值返回结果数据
- 返回值类型：`DataPtr`，可通过`GetDataPtr<T>()`获取具体类型
- ⚠️ 注意：当前版本不支持`AddOutputDataPtr()`和`GetOutputPackage()`，应直接使用返回值

### 真值数据支持 | Ground Truth Support
- 支持位姿真值评估（与输出位姿相同格式）
- 支持3D点真值评估（与输出点云相同格式）
- 使用`SetGTData()`设置真值数据

### 评估指标 | Evaluation Metrics
- 位姿旋转误差（角度）
- 位姿平移误差（距离）
- 3D点重建误差（欧氏距离）
```

---

## 参考资料 | References

- [方法插件开发指南](method_plugins)
- [核心数据类型](../appendices/appendix_a_types)
- [数据包管理](data_plugins)
- [评估器系统](../advanced_development/evaluator_manager)
