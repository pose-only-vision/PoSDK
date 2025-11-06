(prior-and-gt)=
# Ground Truth and Evaluation

This document describes how to set ground truth data for algorithm evaluation in PoSDK method plugins and the core functionality of data package operations.

## Quick Navigation

### Core Concepts
- [Data Package Management Mechanism](#data-type-classification)
- [Ground Truth Evaluation System](#gt-evaluation-system)

### Function List

#### Required Package Operations
| Function Name             | Brief Description                                      | Details Link                     |
| ------------------------- | ------------------------------------------------------ | -------------------------------- |
| `SetRequiredData()`       | Set single required input data                         | [Details](#setrequireddata)      |
| `GetRequiredPackage()`    | Get required input data package reference              | [Details](#getrequiredpackage)   |
| `SetRequiredPackage()`    | Batch set required input data package                  | [Details](#setrequiredpackage)   |
| `GetRequiredDataTypes()`  | Get required input data type list                      | [Details](#getrequireddatatypes) |
| `GetRequiredDataPtr<T>()` | Simplified access to required input data (recommended) | [Details](#getrequireddataptr)   |

#### Output Package Operations
```{warning}
**Reserved Interface**

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```

| Function Name          | Brief Description               | Details Link                   | Status   |
| ---------------------- | ------------------------------- | ------------------------------ | -------- |
| `GetOutputPackage()`   | Get output data package pointer | [Details](#getoutputpackage)   | Reserved |
| `AddOutputDataPtr()`   | Add data to output package      | [Details](#addoutputdataptr)   | Reserved |
| `GetOutputDataTypes()` | Get output data type list       | [Details](#getoutputdatatypes) | Reserved |
| `ClearOutputPackage()` | Clear output data package       | [Details](#clearoutputpackage) | Reserved |

#### Ground Truth Operations
| Function Name | Brief Description     | Details Link          |
| ------------- | --------------------- | --------------------- |
| `SetGTData()` | Set ground truth data | [Details](#setgtdata) |
| `GetGTData()` | Get ground truth data | [Details](#getgtdata) |

#### Evaluator Operations
| Function Name             | Brief Description            | Details Link                      |
| ------------------------- | ---------------------------- | --------------------------------- |
| `SetEvaluatorAlgorithm()` | Set evaluator algorithm name | [Details](#setevaluatoralgorithm) |
| `GetEvaluatorAlgorithm()` | Get evaluator algorithm name | [Details](#getevaluatoralgorithm) |
| `CallEvaluator()`         | Manually call evaluator      | [Details](#callevaluator)         |

### Function Navigation
- [Required Package Functions](#required-package-functions)
- [Output Package Functions](#output-package-functions)
- [Ground Truth Functions](#gt-data-functions)
- [Evaluator Functions](#evaluator-functions)

### Application Examples
- [Basic Package Operations](#example-basic-package)
- [Ground Truth Evaluation Flow](#example-gt-evaluation)

---

(data-type-classification)=
## Data Package Management

PoSDK's `MethodPreset` class provides two core data management mechanisms:

### 1. Required Input Package (`required_package_`)
- Input data **required** by the algorithm
- Must be provided before `Build()` is called
- Missing data will cause the algorithm to fail
- Accessed via `required_package_["key"]`

### 2. Output Package (`output_package_`)
```{warning}
**Reserved Feature**

Output package functionality is a reserved interface and is not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.

Output package functionality is a reserved interface and is not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```
- **Result data** after algorithm execution (reserved)
- Results added via `AddOutputDataPtr()` (reserved)
- Can contain multiple types of output data (reserved)
- Retrieved via `GetOutputPackage()` (reserved)

(gt-evaluation-system)=
### Ground Truth Evaluation System

- **Ground Truth Data**: Used for accuracy evaluation and result verification
- Typically used during development and testing phases
- Does not affect actual algorithm execution
- Accessed via `SetGTData()`/`GetGTData()`
- Works with evaluators for automatic accuracy analysis

### Key Differences Comparison

| Feature               | Required Input Package              | Output Package (Reserved)                | Ground Truth Data       |
| --------------------- | ----------------------------------- | ---------------------------------------- | ----------------------- |
| **Direction**         | Input                               | Output                                   | Evaluation              |
| **Required**          | **Required**                        | Generated by algorithm (reserved)        | Optional                |
| **Purpose**           | Core input for algorithm            | Algorithm execution results (reserved)   | Accuracy evaluation     |
| **Impact if Missing** | Algorithm cannot run                | No output results (reserved)             | Only affects evaluation |
| **Typical Content**   | Features, tracks, camera parameters | Poses, reconstruction results (reserved) | Standard answers        |
| **Access Method**     | `GetRequiredDataPtr<T>()`           | `AddOutputDataPtr()` (reserved)          | `SetGTData()`           |
| **Setting Time**      | Set before execution                | Generated during execution (reserved)    | Set before execution    |
| **Current Status**    | ✓ Implemented                       | ⚠️ Reserved interface, not implemented    | ✓ Implemented           |

---

## Function Details

(required-package-functions)=
### Required Package Functions

(setrequireddata)=
#### `SetRequiredData()`
**Function Signature**:
```cpp
bool SetRequiredData(const DataPtr &data_ptr);
```

**Purpose**: Set a single required input data

**Parameters**:
- `data_ptr` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): Data pointer to set

**Return**: `bool` - Whether setting was successful

**Usage Example**:
```cpp
auto tracks_data = LoadTracks("tracks.bin");
method_ptr->SetRequiredData(tracks_data);
```

---

(getrequiredpackage)=
#### `GetRequiredPackage()`
**Function Signature**:
```cpp
const Package& GetRequiredPackage() const;  // const version
Package& GetRequiredPackage();              // non-const version
```

**Purpose**: Get reference to required input package

**Return**: [`Package`](../appendices/appendix_a_types.md#package) & - Reference to required package

**Usage Example**:
```cpp
const auto& req_pkg = method_ptr->GetRequiredPackage();
for (const auto& [key, data_ptr] : req_pkg) {
    std::cout << "Required data: " << key << std::endl;
}
```

---

(setrequiredpackage)=
#### `SetRequiredPackage()`
**Function Signature**:
```cpp
void SetRequiredPackage(const Package &package);
```

**Purpose**: Batch set required input package

**Parameters**:
- `package` [`Package`](../appendices/appendix_a_types.md#package): Package to set

**Usage Example**:
```cpp
Package input_package;
input_package["data_tracks"] = tracks_ptr;
input_package["data_camera_models"] = cameras_ptr;
method_ptr->SetRequiredPackage(input_package);
```

---

(getrequireddatatypes)=
#### `GetRequiredDataTypes()`
**Function Signature**:
```cpp
std::vector<std::string> GetRequiredDataTypes() const;
```

**Purpose**: Get list of required input data types

**Return**: `std::vector<std::string>` - List of required data type names

**Usage Example**:
```cpp
auto required_types = method_ptr->GetRequiredDataTypes();
for (const auto& type : required_types) {
    std::cout << "Required type: " << type << std::endl;
}
```

---

(getrequireddataptr)=
#### `GetRequiredDataPtr<T>()` (Recommended)
**Function Signature**:
```cpp
template <typename T>
std::shared_ptr<T> GetRequiredDataPtr(const std::string &data_name) const;
```

**Purpose**: Simplified access to required input data with automatic type conversion

**Parameters**:
- `data_name` `std::string`: Data name in required_package_

**Return**: `std::shared_ptr<T>` - Smart pointer to the data

**Usage Example**:
```cpp
// Usage in MethodPreset-derived class:
auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
auto features = GetRequiredDataPtr<FeaturesInfo>("data_features");

// Or through dynamic_pointer_cast:
auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method_ptr);
if (method_preset) {
    auto poses = method_preset->GetRequiredDataPtr<GlobalPoses>("data_global_poses");
}
```

```{tip}
**Recommended Usage**

`GetRequiredDataPtr<T>()` provides a more concise API, automatically handles Package lookup and type conversion, reducing code redundancy.

`GetRequiredDataPtr<T>()` provides a more concise API, automatically handles Package lookup and type conversion, reducing code redundancy.
```

---

(output-package-functions)=
### Output Package Functions

```{warning}
**Reserved Interface**

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.

The following output package operations are reserved interfaces and are not yet implemented in the current version. Algorithms should directly obtain result data through the return value of `Build()` or `Run()`.
```

(getoutputpackage)=
#### `GetOutputPackage()` ⚠️ Reserved Interface
**Function Signature**:
```cpp
DataPackagePtr GetOutputPackage() const;
```

**Purpose**: Get output data package pointer (reserved)

**Return**: [`DataPackagePtr`](../appendices/appendix_a_types.md#data-package-ptr) - Smart pointer to output package

**Current Status**: ⚠️ Reserved interface, not yet implemented

---

(addoutputdataptr)=
#### `AddOutputDataPtr()` ⚠️ Reserved Interface
**Function Signature**:
```cpp
void AddOutputDataPtr(const DataPtr &data_ptr, const std::string &data_name = "");
```

**Purpose**: Add data to output package (reserved)

**Parameters**:
- `data_ptr` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): Data pointer to add
- `data_name` `std::string`: Data name (optional, uses data_ptr type if empty)

**Current Status**: ⚠️ Reserved interface, not yet implemented

---

(getoutputdatatypes)=
#### `GetOutputDataTypes()` ⚠️ Reserved Interface
**Function Signature**:
```cpp
std::vector<std::string> GetOutputDataTypes() const;
```

**Purpose**: Get list of output data types (reserved)

**Return**: `std::vector<std::string>` - List of output data type names

**Current Status**: ⚠️ Reserved interface, not yet implemented

---

(clearoutputpackage)=
#### `ClearOutputPackage()` ⚠️ Reserved Interface
**Function Signature**:
```cpp
void ClearOutputPackage();
```

**Purpose**: Clear output data package (reserved)

**Current Status**: ⚠️ Reserved interface, not yet implemented

---

(gt-data-functions)=
### Ground Truth Functions

(setgtdata)=
#### `SetGTData()`
**Function Signature**:
```cpp
void SetGTData(DataPtr &gt_data);
```

**Purpose**: Set ground truth data for evaluation

**Parameters**:
- `gt_data` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): Ground truth data pointer

**Usage Example**:
```cpp
// Load ground truth data for evaluation
auto gt_poses = LoadGroundTruthPoses("ground_truth.txt");
method_ptr->SetGTData(gt_poses);

// Algorithm will automatically compare with ground truth during execution
auto result = method_ptr->Build(input_data);
```

---

(getgtdata)=
#### `GetGTData()`
**Function Signature**:
```cpp
DataPtr GetGTData() const;
```

**Purpose**: Get currently set ground truth data

**Return**: [`DataPtr`](../appendices/appendix_a_types.md#ptr-type) - Ground truth data pointer

**Usage Example**:
```cpp
auto gt_data = method_ptr->GetGTData();
if (gt_data) {
    std::cout << "Ground truth data is set" << std::endl;
}
```

---

(evaluator-functions)=
### Evaluator Functions

(setevaluatoralgorithm)=
#### `SetEvaluatorAlgorithm()`
**Function Signature**:
```cpp
virtual void SetEvaluatorAlgorithm(const std::string &algorithm_name);
```

**Purpose**: Set algorithm name identifier for evaluation

**Parameters**:
- `algorithm_name` `std::string`: Algorithm name (to distinguish different configurations)

**Usage Example**:
```cpp
// Used to identify different algorithm configurations in evaluation results
method_ptr->SetEvaluatorAlgorithm("PoSDK_v2");
```

---

(getevaluatoralgorithm)=
#### `GetEvaluatorAlgorithm()`
**Function Signature**:
```cpp
virtual std::string GetEvaluatorAlgorithm() const;
```

**Purpose**: Get algorithm name for evaluation

**Return**: `std::string` - Algorithm name (defaults to `GetType()` value)

---

(callevaluator)=
#### `CallEvaluator()`
**Function Signature**:
```cpp
bool CallEvaluator(const DataPtr &result_data);
```

**Purpose**: Manually call evaluator for accuracy evaluation

**Parameters**:
- `result_data` [`DataPtr`](../appendices/appendix_a_types.md#ptr-type): Result data to evaluate

**Return**: `bool` - Whether evaluation was successful

**Usage Example**:
```cpp
// Usually no need to call manually, Build() calls automatically
// Usually no need to call manually, Build() calls automatically
auto result = method_ptr->Run();
bool eval_success = method_ptr->CallEvaluator(result);
```

---

## Application Examples

(example-basic-package)=
### Example 1: Basic Package Operations

Demonstrates basic usage of required input package:

```cpp
class MyPoseEstimator : public MethodPresetProfiler {
public:
    MyPoseEstimator() {
        // Declare required input data
        required_package_["data_tracks"] = nullptr;
        required_package_["data_camera_models"] = nullptr;
    }

    DataPtr Run() override {
        // Get required input data
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");

        // Execute pose estimation algorithm
        auto result_poses = EstimatePoses(tracks, cameras);

        LOG_INFO_ZH << "位姿估计完成，输出" << result_poses->Size() << "个位姿";
        LOG_INFO_EN << "Pose estimation completed, output " << result_poses->Size() << " poses";

        // Directly return result data
        return result_poses;
    }
};

// Usage
auto estimator = FactoryMethod::Create("my_pose_estimator");

// Set required inputs
estimator->SetRequiredData(tracks_data);
estimator->SetRequiredData(camera_models);

// Execute algorithm, directly get return value as result
auto result = estimator->Build();
if (result) {
    auto poses = GetDataPtr<GlobalPoses>(result);
    // Use result data...
}
```

---

(example-gt-evaluation)=
### Example 2: Ground Truth Evaluation Flow

Demonstrates how to set ground truth data and perform automatic evaluation:

```cpp
class MyBundleAdjustment : public MethodPresetProfiler {
public:
    MyBundleAdjustment() {
        required_package_["data_tracks"] = nullptr;
        required_package_["data_camera_models"] = nullptr;
        required_package_["data_initial_poses"] = nullptr;
    }

    DataPtr Run() override {
        // Get input data
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");
        auto initial_poses = GetRequiredDataPtr<GlobalPoses>("data_initial_poses");

        // Execute Bundle Adjustment optimization
        auto optimized_poses = OptimizePoses(tracks, cameras, initial_poses);

        LOG_INFO_ZH << "Bundle Adjustment优化完成";
        LOG_INFO_EN << "Bundle Adjustment optimization completed";

        // Directly return result data
        return optimized_poses;
    }
};

// Complete evaluation flow
int main() {
    // 1. Create algorithm instance
    auto ba_method = FactoryMethod::Create("my_bundle_adjustment");

    // 2. Set algorithm name identifier
    ba_method->SetEvaluatorAlgorithm("PoSDK_BA_v2.1");

    // 3. Set required inputs
    auto tracks = LoadTracks("input_tracks.bin");
    auto cameras = LoadCameras("cameras.txt");
    auto initial_poses = LoadInitialPoses("initial.txt");

    ba_method->SetRequiredData(tracks);
    ba_method->SetRequiredData(cameras);
    ba_method->SetRequiredData(initial_poses);

    // 4. Set ground truth data for evaluation
    auto gt_poses = LoadGroundTruthPoses("ground_truth_poses.txt");
    auto gt_points = LoadGroundTruthPoints("ground_truth_points.txt");

    ba_method->SetGTData(gt_poses);  // Set pose ground truth

    // 5. Execute algorithm (will automatically call evaluator)
    auto result = ba_method->Build();

    // 6. View evaluation results (evaluator automatically outputs to console and files)
    LOG_INFO_ZH << "算法执行完成，评估结果已保存";
    LOG_INFO_EN << "Algorithm completed, evaluation results saved";

    // 7. Use returned result data
    if (result) {
        auto optimized_poses = GetDataPtr<GlobalPoses>(result);
        
        // Manually perform additional evaluation (optional)
        // Note: CallEvaluator parameter should be result data, evaluator automatically gets GT from GetGTData() for comparison
        // ba_method->CallEvaluator(result);
        
        // Use optimized poses...
    }

    return 0;
}
```

### Ground Truth Data Preparation Guide

```cpp
// Ground truth pose data preparation example
void PrepareGroundTruthData() {
    // 1. Create ground truth poses
    auto gt_poses = FactoryData::Create("global_poses");
    gt_poses->Init(5);  // 5 camera poses

    // Set reference camera pose (usually identity transformation)
    Matrix3d R_ref = Matrix3d::Identity();
    Vector3d t_ref = Vector3d::Zero();
    gt_poses->SetRotation(0, R_ref);
    gt_poses->SetTranslation(0, t_ref);

    // Set ground truth poses for other cameras
    for (int i = 1; i < 5; ++i) {
        Matrix3d R_gt = LoadRotationFromFile("gt_rotation_" + std::to_string(i) + ".txt");
        Vector3d t_gt = LoadTranslationFromFile("gt_translation_" + std::to_string(i) + ".txt");
        gt_poses->SetRotation(i, R_gt);
        gt_poses->SetTranslation(i, t_gt);
    }

    // 2. Create ground truth 3D points
    auto gt_points = FactoryData::Create("world_point_info");
    gt_points->resize(1000);  // 1000 3D points

    for (size_t i = 0; i < 1000; ++i) {
        Point3d gt_point = LoadPointFromFile("gt_point_" + std::to_string(i) + ".txt");
        gt_points->setPoint(i, gt_point);
        gt_points->setUsed(i, true);
    }

    // 3. Save ground truth data for algorithm use
    gt_poses->Save("ground_truth_poses.posdk");
    gt_points->Save("ground_truth_points.posdk");

    LOG_INFO_ZH << "真值数据准备完成：" << gt_poses->Size() << "个位姿，"
                << gt_points->getValidPointsCount() << "个3D点";
    LOG_INFO_EN << "Ground truth data prepared: " << gt_poses->Size() << " poses, "
                << gt_points->getValidPointsCount() << " 3D points";
}
```

---

## Best Practices

### 1. Clear Package Management

```cpp
// Recommended: Clearly distinguish input data, directly return results
class MyAlgorithm : public MethodPresetProfiler {
public:
    MyAlgorithm() {
        // Required input data declaration
        required_package_["data_tracks"] = nullptr;
        required_package_["data_cameras"] = nullptr;
    }

    DataPtr Run() override {
        // Get input data
        auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
        auto cameras = GetRequiredDataPtr<CameraModels>("data_cameras");

        // Algorithm processing
        auto result = ProcessData(tracks, cameras);

        // Directly return result data
        return result;
    }
};

// ❌ Not recommended: Confusing input/output management
// Don't dynamically add required_package_ items outside constructor
// ⚠️ Note: Current version does not support AddOutputDataPtr(), should directly return results
```

### 2. Complete Ground Truth Evaluation

```cpp
// Recommended: Complete evaluation flow setup
auto algorithm = FactoryMethod::Create("my_algorithm");

// Set algorithm identifier (for evaluation result distinction)
algorithm->SetEvaluatorAlgorithm("MyAlgo_v1.2");

// Set input data
algorithm->SetRequiredData(input_tracks);
algorithm->SetRequiredData(camera_models);

// Set ground truth data
auto gt_poses = LoadGroundTruth("gt_poses.txt");
algorithm->SetGTData(gt_poses);

// Execute and automatically evaluate
auto result = algorithm->Build();

// ❌ Not recommended: Ignore ground truth setup
// Missing SetGTData() will prevent accuracy evaluation
```

### 3. Error Handling and Validation

```cpp
// Recommended: Check data validity
DataPtr Run() override {
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    if (!tracks || tracks->size() == 0) {
        LOG_ERROR_ZH << "输入轨迹数据无效";
        LOG_ERROR_EN << "Invalid input tracks data";
        return nullptr;
    }

    auto result = ProcessTracks(tracks);

    // Validate output data
    if (!result || !IsValidResult(result)) {
        LOG_ERROR_ZH << "算法输出结果无效";
        LOG_ERROR_EN << "Algorithm output is invalid";
        return nullptr;
    }

    return result;
}

// ❌ Not recommended: Missing data validation
// Not checking input data validity may cause algorithm crash
```

### 4. Documentation and README Standards

The plugin's README should clearly state:

```markdown
## Data Interface

### Required Inputs
- `data_tracks`: Feature track data type - [`Tracks`](link-to-tracks-doc)
- `data_camera_models`: Camera parameters - [`CameraModels`](link-to-camera-doc)

### Output Data
- Algorithm returns result data through return value of `Build()` or `Run()`
- Return type: `DataPtr`, can get specific type via `GetDataPtr<T>()`
- ⚠️ Note: Current version does not support `AddOutputDataPtr()` and `GetOutputPackage()`, should directly use return value

### Ground Truth Support
- Supports pose ground truth evaluation (same format as output poses)
- Supports 3D point ground truth evaluation (same format as output point cloud)
- Use `SetGTData()` to set ground truth data

### Evaluation Metrics
- Pose rotation error (angle)
- Pose translation error (distance)
- 3D point reconstruction error (Euclidean distance)
```

---

## References

- [Method Plugin Development Guide](method_plugins)
- [Core Data Types](../appendices/appendix_a_types)
- [Data Package Management](data_plugins)
- [Evaluator System](../advanced_development/evaluator_manager)
