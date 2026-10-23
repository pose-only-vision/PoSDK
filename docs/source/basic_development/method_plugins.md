# Method Plugin (Method Plugin)

Method plugins are used to implement specific algorithm logic and data processing workflows.

## Current Build and Loading Notes

Method plugins are expected to follow the current PoMVG build and runtime model:

- Build with `add_posdk_plugin(...)` from `plugins/cmake/plugin-config.cmake`
- Register with `REGISTRATION_PLUGIN(...)` in the `.cpp`
- Optional platform declarations: `SUPPORTED_SYSTEMS` and `SUPPORTED_ARCHES`
- Runtime build artifact pair:
  - `posdk_plugin_<name>.{dylib|so|dll}`
  - `posdk_plugin_<name>.posdk.json`
- Incompatible or invalid method plugin binaries are filtered out of `Modules` and runtime factory registration

---

## MethodPreset Common Operation Functions Quick Reference

The following list covers common operation functions of `MethodPreset` and its derived classes (`MethodPresetProfiler`). For detailed descriptions, see [corresponding section](#methodpreset-interface-details).

### Data Management | Data Management

| Function                                              | Brief Description                                         |
| ----------------------------------------------------- | --------------------------------------------------------- |
| [`SetRequiredData()`](#setrequireddata)               | Set single required input data                            |
| [`GetRequiredPackage()`](#getrequiredpackage)         | Get required input data package reference                 |
| [`GetInputTypes()`](#getinputtypes)                   | Get input data type list (recommended⭐)                   |
| [`GetRequiredDataPtr<T>()`](#getrequireddataptr)      | Simplified access to required input data (recommended⭐)   |
| [`GetInputData()`](#getinputdata)                     | Get input data by name                                    |
| [`GetInputDataAs<T>()`](#getinputdataas)              | Get typed input data by name (recommended⭐)               |
| [`GetOutputPackage()`](#getoutputpackage)             | Get output data package pointer                           |
| [`AddOutputDataPtr()`](#addoutputdataptr)             | Add data to output package                                |
| [`GetOutputTypes()`](#getoutputtypes)                 | Get output data type list (recommended⭐)                  |
| [`CreateOutputData()`](#createoutputdata)             | Create output data by name                                |
| [`CreateOutputDataAs<T>()`](#createoutputdataas)      | Create typed output data by name (recommended⭐)           |
| [`ClearOutputPackage()`](#clearoutputpackage)         | Clear output data package                                 |
| ~~[`GetRequiredDataTypes()`](#getrequireddatatypes)~~ | ⚠️ Deprecated, use `GetInputTypes()` instead              |
| ~~[`GetOutputDataTypes()`](#getoutputdatatypes)~~     | ⚠️ Deprecated, use `GetOutputTypes()` instead             |

### Configuration Management | Configuration Management

| Function                                    | Brief Description                    |
| ------------------------------------------- | ------------------------------------ |
| [`LoadMethodOptions()`](#loadmethodoptions) | Load method options from config file |
| [`SaveMethodOptions()`](#savemethodoptions) | Save method options to config file   |
| [`GetMethodOptions()`](#getmethodoptions)   | Get method options dictionary        |
| [`SetMethodOptions()`](#setmethodoptions)   | Batch set method options             |
| [`SetMethodOption()`](#setmethodoption)     | Set single method option             |

### Parameter Access | Parameter Access

| Function                                    | Brief Description                          |
| ------------------------------------------- | ------------------------------------------ |
| [`GetOptionAsIndexT()`](#getoptionasindext) | Get IndexT type parameter                  |
| [`GetOptionAsInt()`](#getoptionasint)       | Get int type parameter                     |
| [`GetOptionAsFloat()`](#getoptionasfloat)   | Get float type parameter                   |
| [`GetOptionAsDouble()`](#getoptionasdouble) | Get double type parameter                  |
| [`GetOptionAsBool()`](#getoptionasbool)     | Get bool type parameter                    |
| [`GetOptionAsString()`](#getoptionasstring) | Get string type parameter                  |
| [`GetOptionAsPath()`](#getoptionaspath)     | Get path parameter (supports placeholders) |

### Prior Information & Ground Truth | Prior Information & Ground Truth

| Function                              | Brief Description       |
| ------------------------------------- | ----------------------- |
| [`SetPriorInfo()`](#setpriorinfo)     | Set prior information   |
| [`ResetPriorInfo()`](#resetpriorinfo) | Reset prior information |
| [`SetGTData()`](#setgtdata)           | Set ground truth data   |
| [`GetGTData()`](#getgtdata)           | Get ground truth data   |

### Profiling & Evaluation (MethodPresetProfiler) | Profiling & Evaluation

| Function                                                          | Brief Description                              |
| ----------------------------------------------------------------- | ---------------------------------------------- |
| [`SetProfilerLabel()`](#setprofilerlabel)                         | Set profiler label (string)                    |
| [`SetProfilerLabels()`](#setprofilerlabels)                       | Set structured profiler labels (key-value)     |
| [`GetProfilerLabel()`](#getprofilerlabel)                         | Get profiler label string                      |
| [`GetProfilerLabels()`](#getprofilerlabels)                       | Get structured profiler labels                 |
| [`HasStructuredLabels()`](#hasstructuredlabels)                   | Check if structured labels are available       |
| [`SetEvaluatorAlgorithm()`](#setevaluatoralgorithm)               | Set algorithm name used in evaluator           |
| [`GetEvaluatorAlgorithm()`](#getevaluatoralgorithm)               | Get algorithm name used in evaluator           |
| [`ClearEvaluatorAlgorithm()`](#clearevaluatoralgorithm)           | Clear evaluator algorithm setting              |
| [`CallEvaluator()`](#callevaluator)                               | Call evaluator for evaluation                  |

```{seealso}
- [Prior Information and Ground Truth Setup Details](prior_and_gt.md)
- [Performance Analysis System Details](../advanced_development/profiler.md)
```

---

(methodpreset-interface-details)=
## Interface Introduction

### Required Override Interfaces

````{important}
Each method plugin must declare `GetType()` and implement the algorithm entrypoint required by its base class:

- **Method Name**: Only **declare** in header file
  ```cpp
  virtual const std::string& GetType() const override;
  // Returns plugin's unique type string identifier.
  // Automatically implemented by REGISTRATION_PLUGIN macro, no manual implementation needed
  // Example: `"method_sift"`, `"method_matches2tracks"`
  ```

- **Algorithm Main Entry**: Implement `Build()` when deriving directly from `Method`; implement `Run()` when deriving from `MethodPreset` or `MethodPresetProfiler`
  ```cpp
  virtual DataPtr Build(const DataPtr& material_ptr = nullptr) override;
  // Used to implement core algorithm:
  // (1) material_ptr is optional input data. If method needs multiple inputs, pack them in `DataPackage` and pass in.
  // (2) Return processing result (`DataPtr`), if processing fails or no result, return `nullptr`.
  ```

  Or

  ```cpp
  virtual DataPtr Run() override;
  // Used to implement core algorithm: Base class's `Build` method will call `Run` and handle input/output checking and performance analysis, etc.
  // (1) User needs to set input data types in constructor: required_package_["data_type"] = nullptr;
  // (2) Then can use GetRequiredDataPtr<T>("data_type") (recommended) or GetDataPtr<T>(required_package_["data_type"]) in Run function to get input data.
  ```


````

### Prior Information and Copyright Interfaces

- **Set algorithm's prior information:**
```cpp
void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");
// (1) `MethodPreset` sets prior information for algorithms requiring additional guidance information (such as initial poses, weights).
// (2) `data_ptr` is prior information data pointer, `type` is prior information type string.

void ResetPriorInfo();
// Reset prior information (clear all prior information)
```

```{seealso}
For detailed descriptions and usage examples of prior information, required input data, and ground truth data, please refer to [Prior Information and Ground Truth Setup](prior_and_gt.md).
```

```cpp
virtual CopyrightData Copyright() const;
```

`Copyright()` is a virtual function provided by the `Method` base class for plugin developers to declare copyright information. When copyright tracking is enabled, the `MethodPreset::Build()` execution path collects and manages this information through `CopyrightManager`. For details, see [Copyright Tracking Feature](../copyright_tracking.rst).

## Optional Derivation Methods

- **`Interface::Method`** (base class)
  - Provides most basic method plugin interface. Need to handle input checking, configuration loading, etc. yourself.
- **`Interface::MethodPreset`** (derived from `Method`)
  - Provides preset functional framework to simplify development:
    - **Automatic Input Checking**: Checks input data types based on `required_package_` member variable.
    - **Configuration Management**: Supports configuring `method_options_` through INI files.
    - **Prior Information**: Supports passing additional information through `SetPriorInfo`.
    - **Core Logic Separation**: Implement core algorithm in `Run()`, `Build()` handles flow control.
- **`Interface::MethodPresetProfiler`** (derived from `MethodPreset`)
  - Adds **performance analysis** functionality on top of `MethodPreset`.
  - Records selected metrics (time only by default) when profiling macros are enabled in `Run()`, and supports CSV export.
  - Can control whether to enable analysis through `enable_profiling` option.
- **`Interface::RobustEstimator`** (derived from `MethodPresetProfiler`)
  - Robust estimator base class, provides model estimation and cost evaluation framework.
  - Supports automatic initialization of model estimator and cost evaluator.
  - See [Robust Estimators](../advanced_development/robust_estimators.md) for details.

## Example

The example below supplies fallback values through `GetOptionAs...()` and reads configured values from INI.

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(my_method
    PLUGIN_TYPE methods
    SOURCES my_method.cpp
    HEADERS my_method.hpp
)
```

```cpp
// my_method.hpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types; // Introduce common types

class MyMethod : public MethodPresetProfiler { // Inherit Profiler for performance analysis
public:
    MyMethod();
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    DataPtr Run() override; // Implement core logic

    // Declare input types through required_package_; GetInputTypes() is not virtual.

    // Optional: Override Copyright() to declare copyright information
    // Interface::CopyrightData Copyright() const override;
};
} // namespace MyPlugin

// my_method.cpp
#include "my_method.hpp"

namespace MyPlugin {

MyMethod::MyMethod() {
    // Define required input data types
    // Setting to nullptr indicates this type is required but not yet provided
    required_package_["data_tracks"] = nullptr;
    required_package_["data_global_poses"] = nullptr;

    // Load default config file (if exists)
    InitializeDefaultConfigPath();
}

// Don't need to manually implement GetType(), macro automatically generates

DataPtr MyMethod::Run() {
    // 1. Get input data (MethodPreset has handled validation)
    // Recommended: use GetRequiredDataPtr for simplified access
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    auto poses = GetRequiredDataPtr<GlobalPoses>("data_global_poses");

    // Validate data
    if (!tracks || !poses) {
        std::cerr << "[" << GetType() << "] Error: Missing required input data." << std::endl;
        return nullptr;
    }

    // 2. Get configuration parameters
    // ✅ Correct: Provide default values via function parameters here
    float threshold = GetOptionAsFloat("threshold", 0.5f);      // Default value: 0.5f
    int max_iter = GetOptionAsIndexT("max_iterations", 100);    // Default value: 100

    std::cout << "[" << GetType() << "] Running with threshold=" << threshold
              << ", max_iterations=" << max_iter << std::endl;

    // 3. Implement core algorithm logic...
    // ... process tracks and poses ...

    // 4. Create and return result (e.g., return processed poses)
    // Note: If you don't want to modify input data, deep copy DataPtr first
    // auto result_poses_data = GetInputData("data_global_poses")->CopyData();
    // auto result_poses = GetDataPtr<GlobalPoses>(result_poses_data);
    // ... Modify result_poses ...
    // return result_poses_data;

    // Or create and return new data object
    auto result_data = std::make_shared<DataMap<std::string>>("Processing finished", "data_map_string");
    return result_data;
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "my_method" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::MyMethod)
```

````{warning}
**Parameter Default Value Setting in Constructor**

Values preloaded into `method_options_` are overridden by matching INI entries. Prefer defaults in `GetOptionAs...()` calls so missing or reset options still have fallback values.

**Correct Approach**:
- Directly provide default value parameter when calling `GetOptionAs...()` function
- For example: `0.5f` in `GetOptionAsFloat("threshold", 0.5f)` is the default value

**Not Recommended**:
```cpp
// Matching INI entries override these values
method_options_["threshold"] = "0.5";
method_options_["max_iterations"] = "100";
```



````

## Method Parameter Configuration

- **Set Default Values**:
  - **Recommended Method**: Use member functions provided by `MethodPreset` base class for accessing configuration parameters (such as `GetOptionAsFloat`, `GetOptionAsIndexT`, etc.) to access config values, and provide default value parameters when calling.
  - Example: `float threshold = GetOptionAsFloat("threshold", 0.5f);` where `0.5f` is the default value
- **Configuration File (.ini)**:
  - Create `.ini` file with the same name as `GetType()` return value in `configs/methods/` directory (e.g., `my_method.ini`).
  - File format is standard INI format, containing a section with the same name as method type `[my_method]`.
  - Define `key=value` pairs under the section.
  - `MethodPreset` automatically loads this file, values in config file will override default values.
  - For PoMVG GUI visual grouping, keep ordinary method parameters inside `[my_method]` and use `## Group | 分组` markers such as `## Input | 输入` or `## Algorithm | 算法`. Extra `[section]` blocks are runtime namespaces for sub-method/component configuration and are not automatically read by `GetOptionAs...()`.
- **Runtime Settings**:
  - Can batch set options through `SetMethodOptions(const MethodOptions& options)`.
  - Can set options individually through `SetMethodOption(const MethodParams& key, const ParamsValue& value)`.
  - Runtime settings have highest priority and will override config file and default values.
- **Get Configuration Values**:
  - In `Run()` or other member functions, directly call member functions provided by `MethodPreset` base class such as `GetOptionAsString("param_key", "default_val")`, `GetOptionAsIndexT("param_key", 0)`, `GetOptionAsFloat("param_key", 0.0f)`, `GetOptionAsBool("param_key", false)`, etc. to safely get configuration values. These functions automatically search in `method_options_`, if not found or type mismatch, return the provided default value.
- **Parameter Setting Priority**: Runtime settings > Configuration file > Default values

```{important}
**Correct Method for Setting Default Values**

**Configuration overrides**: Matching INI entries override preloaded `method_options_` values. `LoadMethodOptions()` does not clear an existing value when its key is absent from the INI.

**Recommended**: Provide fallback values through function parameters when calling `GetOptionAs...()`
- For example: `GetOptionAsFloat("threshold", 0.5f)`
- When both config file and runtime don't set this parameter, will use `0.5f` as default value
```


## Core Data Types

The core library provides various predefined data types that can be directly used in plugins or as reference (see **Core Data Type Detailed Description**: Please refer to [Appendix A: PoMVG Core Data Types](../appendices/appendix_a_types.md))

---

## MethodPreset Function Details

This section details each operation function of the `MethodPreset` class.

### Data Management Functions

(setrequireddata)=
#### `SetRequiredData()`

Set single required input data to `required_package_`.

**Function Signature**:
```cpp
bool SetRequiredData(const DataPtr &data_ptr);
```

**Parameters**:
- `data_ptr`: Data pointer to set

**Return Value**: Returns true on success, false on failure

**Usage Example**:
```cpp
auto tracks_data = FactoryData::Create("data_tracks");
// ... Populate tracks_data ...
method->SetRequiredData(tracks_data);
```

---

(getrequiredpackage)=
#### `GetRequiredPackage()`

Get reference to required input data package.

**Function Signature**:
```cpp
const Package& GetRequiredPackage() const;  // const version
Package& GetRequiredPackage();              // non-const version
```

**Return Value**: Reference to `required_package_`

**Usage Example**:
```cpp
const Package& pkg = method->GetRequiredPackage();
for (const auto& [key, value] : pkg) {
    std::cout << "Data type: " << key << std::endl;
}
```

---

(getrequireddatatypes)=
#### `GetRequiredDataTypes()` ⚠️ Deprecated

```{warning}
Use [`GetInputTypes()`](#getinputtypes) instead.
```

Get list of all required input data types.

**Function Signature**:
```cpp
[[deprecated("Use GetInputTypes() instead")]]
std::vector<std::string> GetRequiredDataTypes() const;
```

---

(getrequireddataptr)=
#### `GetRequiredDataPtr<T>()`  (Recommended)

**Simplified data access function**, directly get typed data pointer from `required_package_`.

**Function Signature**:
```cpp
template <typename T>
std::shared_ptr<T> GetRequiredDataPtr(const std::string &data_name) const;
```

**Parameters**:
- `data_name`: Key name of data in `required_package_`

**Return Value**: Smart pointer to type `T`, returns `nullptr` on failure

**Usage Example**:
```cpp
// Old way (not recommended)
auto tracks_old = GetDataPtr<Tracks>(required_package_["data_tracks"]);

// New way (recommended)
auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
auto poses = GetRequiredDataPtr<GlobalPoses>("data_global_poses");

if (!tracks || !poses) {
    LOG_ERROR_ZH << "缺少必要的输入数据";
    LOG_ERROR_EN << "Missing required input data";
    return nullptr;
}
```

```{tip}
**Why Recommend Using GetRequiredDataPtr?**
- More concise code, reduces redundancy
- Automatically handles lookup and type conversion
- Better error messages
```

---

(getoutputpackage)=
#### `GetOutputPackage()`

Get smart pointer to output data package.

**Function Signature**:
```cpp
DataPackagePtr GetOutputPackage() const;
```

**Return Value**: Smart pointer to output data package

**Usage Example**:
```cpp
auto output_pkg = method->GetOutputPackage();
if (output_pkg) {
    // Access output data...
    auto result = output_pkg->GetData("result_poses");
}
```

---

(addoutputdataptr)=
#### `AddOutputDataPtr()`

Add data to output data package.

**Function Signature**:
```cpp
void AddOutputDataPtr(const DataPtr &data_ptr, const std::string &data_name = "");
```

**Parameters**:
- `data_ptr`: Data pointer to add
- `data_name`: Data name (optional)

**Usage Example**:
```cpp
// Method 1: Specify data name
auto result_poses = FactoryData::Create("data_global_poses");
AddOutputDataPtr(result_poses, "optimized_poses");

// Method 2: Use data type as name
auto result_tracks = FactoryData::Create("data_tracks");
AddOutputDataPtr(result_tracks);  // Automatically uses "data_tracks" as name

// Method 3: Add DataPackage
auto sub_package = FactoryData::Create("data_package");
// ... Populate sub_package ...
AddOutputDataPtr(sub_package, "sub_results");
```

---

(getoutputdatatypes)=
#### `GetOutputDataTypes()` ⚠️ Deprecated

```{warning}
Use [`GetOutputTypes()`](#getoutputtypes) instead.
```

Get list of all data types in output data package.

**Function Signature**:
```cpp
[[deprecated("Use GetOutputTypes() instead")]]
std::vector<std::string> GetOutputDataTypes() const;
```

---

(getinputtypes)=
#### `GetInputTypes()` (Recommended)

Get input data type list. Replaces deprecated `GetRequiredDataTypes()`.

**Function Signature**:
```cpp
const std::vector<std::string>& GetInputTypes() const;
```

**Return Value**: Reference to string vector of input data types

**Usage Example**:
```cpp
const auto& types = method->GetInputTypes();
for (const auto& type : types) {
    std::cout << "Input type: " << type << std::endl;
}
```

---

(getoutputtypes)=
#### `GetOutputTypes()` (Recommended)

Get output data type list. Replaces deprecated `GetOutputDataTypes()`.

**Function Signature**:
```cpp
const std::vector<std::string>& GetOutputTypes() const;
```

**Return Value**: Reference to string vector of output data types

---

(getinputdata)=
#### `GetInputData()`

Get input data from `required_package_` by name.

**Function Signature**:
```cpp
DataPtr GetInputData(const std::string &dataName) const;
```

**Parameters**:
- `dataName`: Key name of data in `required_package_`

**Return Value**: Data pointer, returns `nullptr` if not found

---

(getinputdataas)=
#### `GetInputDataAs<T>()` (Recommended)

Get typed input data pointer by name. Equivalent to `GetDataPtr<T>(GetInputData(dataName))`.

**Function Signature**:
```cpp
template <typename T>
std::shared_ptr<T> GetInputDataAs(const std::string &dataName) const;
```

**Usage Example**:
```cpp
auto tracks = method->GetInputDataAs<Tracks>("data_tracks");
```

---

(createoutputdata)=
#### `CreateOutputData()`

Create output data by name via factory, optionally auto-add to output package.

**Function Signature**:
```cpp
DataPtr CreateOutputData(const std::string &dataName, bool addToPackage = true);
```

**Parameters**:
- `dataName`: Data type name
- `addToPackage`: Whether to automatically add to output data package (default `true`)

**Return Value**: Created data pointer

---

(createoutputdataas)=
#### `CreateOutputDataAs<T>()` (Recommended)

Create typed output data pointer by name.

**Function Signature**:
```cpp
template <typename T>
std::shared_ptr<T> CreateOutputDataAs(const std::string &dataName, bool addToPackage = true);
```

**Usage Example**:
```cpp
auto result_poses = method->CreateOutputDataAs<GlobalPoses>("data_global_poses");
```

---

(clearoutputpackage)=
#### `ClearOutputPackage()`

Clear all data in output data package.

**Function Signature**:
```cpp
void ClearOutputPackage();
```

**Usage Example**:
```cpp
// Clear previous output
method->ClearOutputPackage();

// Add new output data
method->AddOutputDataPtr(new_result, "updated_result");
```

---

### Configuration Management Functions

(loadmethodoptions)=
#### `LoadMethodOptions()`

Load method options from configuration file.

**Function Signature**:
```cpp
void LoadMethodOptions(const std::string &config_file, const std::string &specific_method = "");
```

**Parameters**:
- `config_file`: Configuration file path
- `specific_method`: Specific method name (optional)

---

(savemethodoptions)=
#### `SaveMethodOptions()`

Save method options to configuration file.

**Function Signature**:
```cpp
void SaveMethodOptions(const std::string &config_file);
```

---

(getmethodoptions)=
#### `GetMethodOptions()`

Get method options dictionary.

**Function Signature**:
```cpp
const MethodOptions& GetMethodOptions() const;
```

---

(setmethodoptions)=
#### `SetMethodOptions()`

Batch set method options.

**Function Signature**:
```cpp
void SetMethodOptions(const MethodOptions &options);
```

---

(setmethodoption)=
#### `SetMethodOption()`

Set single method option.

**Function Signature**:
```cpp
void SetMethodOption(const MethodParams &option_type, const ParamsValue &content);
```

---

### Parameter Access Functions

(getoptionasindext)=
#### `GetOptionAsIndexT()`

Get IndexT type parameter.

**Function Signature**:
```cpp
IndexT GetOptionAsIndexT(const MethodParams &key, IndexT default_value = 0) const;
```

---

(getoptionasint)=
#### `GetOptionAsInt()`

Get int type parameter.

**Function Signature**:
```cpp
int GetOptionAsInt(const MethodParams &key, int default_value = 0) const;
```

---

(getoptionasfloat)=
#### `GetOptionAsFloat()`

Get float type parameter.

**Function Signature**:
```cpp
float GetOptionAsFloat(const MethodParams &key, float default_value = 0.0f) const;
```

---

(getoptionasdouble)=
#### `GetOptionAsDouble()`

Get double type parameter.

**Function Signature**:
```cpp
double GetOptionAsDouble(const MethodParams &key, double default_value = 0.0) const;
```

---

(getoptionasbool)=
#### `GetOptionAsBool()`

Get bool type parameter.

**Function Signature**:
```cpp
bool GetOptionAsBool(const MethodParams &key, bool default_value = false) const;
```

---

(getoptionasstring)=
#### `GetOptionAsString()`

Get string type parameter.

**Function Signature**:
```cpp
std::string GetOptionAsString(const MethodParams &key, const std::string &default_value = "") const;
```

---

(getoptionaspath)=
#### `GetOptionAsPath()`

Get path parameter, supports placeholder replacement.

**Function Signature**:
```cpp
std::string GetOptionAsPath(const MethodParams &key, const std::string &root_dir = "", const std::string &default_value = "") const;
```

**Supported Placeholders**:
- `{root_dir}`: Root directory path
- `{exe_dir}`: Executable file directory
- `{key_name}`: Reference other configuration item values

**Usage Example**:
```cpp
// In config file: output_path={root_dir}/results
std::string output = GetOptionAsPath("output_path", "/home/user/project");
// Result: /home/user/project/results
```

---

### Prior Information and Ground Truth Functions

(setpriorinfo)=
#### `SetPriorInfo()`

Set prior information. For details, see [Prior Information and Ground Truth Setup](prior_and_gt.md).

**Function Signature**:
```cpp
void SetPriorInfo(const DataPtr &data_ptr, const std::string& type = "");
```

---

(resetpriorinfo)=
#### `ResetPriorInfo()`

Reset prior information.

**Function Signature**:
```cpp
void ResetPriorInfo();
```

---

(setgtdata)=
#### `SetGTData()`

Set ground truth data.

**Function Signature**:
```cpp
void SetGTData(DataPtr &gt_data);
```

---

(getgtdata)=
#### `GetGTData()`

Get ground truth data.

**Function Signature**:
```cpp
DataPtr GetGTData() const;
```

---

### Profiling & Evaluation Functions (MethodPresetProfiler)

(setprofilerlabel)=
#### `SetProfilerLabel()`

Set profiler label (string version).

**Function Signature**:
```cpp
void SetProfilerLabel(const std::string &profiler_label);
```

**Parameters**:
- `profiler_label`: Profiler label string

**Usage Example**:
```cpp
method->SetProfilerLabel("MyAlgorithm_v2");
```

---

(setprofilerlabels)=
#### `SetProfilerLabels()`

Set structured profiler labels (key-value pairs), used for custom headers in CSV export.

**Function Signature**:
```cpp
void SetProfilerLabels(const std::unordered_map<std::string, std::string> &structured_labels);
```

**Usage Example**:
```cpp
method->SetProfilerLabels({{"pipeline", "PoSDK"}, {"dataname", "test_data1"}});
```

---

(getprofilerlabel)=
#### `GetProfilerLabel()`

Get profiler label string. Defaults to `GetType()` value if not set.

**Function Signature**:
```cpp
std::string GetProfilerLabel() const;
```

---

(getprofilerlabels)=
#### `GetProfilerLabels()`

Get structured profiler labels.

**Function Signature**:
```cpp
const std::unordered_map<std::string, std::string>& GetProfilerLabels() const;
```

---

(hasstructuredlabels)=
#### `HasStructuredLabels()`

Check if structured labels are available.

**Function Signature**:
```cpp
bool HasStructuredLabels() const;
```

---

(setevaluatoralgorithm)=
#### `SetEvaluatorAlgorithm()`

Set algorithm name used in evaluator, used to distinguish different algorithm configurations. Defaults to `GetType()` value if not set.

**Function Signature**:
```cpp
void SetEvaluatorAlgorithm(const std::string &algorithm_name);
```

---

(getevaluatoralgorithm)=
#### `GetEvaluatorAlgorithm()`

Get algorithm name used in evaluator. Returns empty string if not set, caller should use `GetType()` as fallback if needed.

**Function Signature**:
```cpp
std::string GetEvaluatorAlgorithm() const;
```

---

(clearevaluatoralgorithm)=
#### `ClearEvaluatorAlgorithm()`

Clear evaluator algorithm setting, resets `algorithm_` to empty string. Call this before each test run to avoid cached algorithm name from previous runs affecting current run.

**Function Signature**:
```cpp
void ClearEvaluatorAlgorithm();
```

---

(callevaluator)=
#### `CallEvaluator()`

Call evaluator for evaluation. This function copies input data, checks ground truth data, and calls `DataIO::Evaluate()` for evaluation. Supports both single data objects and `DataPackage` cases.

**Function Signature**:
```cpp
bool CallEvaluator(const DataPtr &result_data);
```

**Parameters**:
- `result_data`: Result data to be evaluated

**Return Value**: Whether evaluator was successfully called

```{seealso}
- [Performance Analysis System Details](../advanced_development/profiler.md)
- [EvaluatorManager](../advanced_development/evaluator_manager.md)
```

## Pluggable two-view estimator contract

An estimator intended for selection inside `two_view_estimator` must not rely
on its plugin ID being known by the wrapper. Implement the optional capability
provider in addition to `MethodPresetProfiler`:

```cpp
class MyEstimator final
    : public PoSDK::Interface::MethodPresetProfiler,
      public PoSDK::Interface::StaticTwoViewEstimatorCapabilities<
          PoSDK::Interface::TwoViewEstimatorInputContract::IdMatchesWithFeaturesAndCameras,
          PoSDK::Interface::TwoViewEstimatorPoseConvention::PoSDK,
          PoSDK::Interface::TwoViewEstimatorInlierContract::MatchesInPlace,
          PoSDK::Interface::TwoViewEstimatorRefinementContract::Unsupported>
{
    // Run() and GetType() ...
};
```

Declare the same contract in the strict source manifest so the GUI can offer
the installed plugin without loading native code in the GUI process:

```json
"capabilities": {
  "two_view_estimator": {
    "contract_version": 1,
    "input_contract": "id_matches_features_cameras",
    "output_contract": "relative_pose",
    "pose_convention": "posdk",
    "inlier_contract": "matches_in_place",
    "refinement_contract": "unsupported"
  }
}
```

The manifest and runtime provider must agree. `BearingPairs` estimators use
`bearing_pairs + sample_best_inliers`; match-based estimators use
`id_matches_features_cameras` and normally `matches_in_place`. Pose convention
and refinement ownership are intrinsic contract properties, not user-tunable
options. A contract method must return `DataMap<RelativePose>` and must never
silently perform a second external refinement when it declares
`estimator_optional` or `estimator_always`.
