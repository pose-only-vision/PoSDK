# Method Plugin (Method Plugin)

Method plugins are used to implement specific algorithm logic and data processing workflows.

---

## MethodPreset Common Operation Functions Quick Reference

The following list covers common operation functions of `MethodPreset` and its derived classes (`MethodPresetProfiler`). For detailed descriptions, see [corresponding section](#methodpreset-interface-details).

### Data Management | Data Management

| Function                                          | Brief Description                                       |
| ------------------------------------------------- | ------------------------------------------------------- |
| [`SetRequiredData()`](#setrequireddata)           | Set single required input data                          |
| [`GetRequiredPackage()`](#getrequiredpackage)     | Get required input data package reference               |
| [`GetRequiredDataTypes()`](#getrequireddatatypes) | Get required input data type list                       |
| [`GetRequiredDataPtr<T>()`](#getrequireddataptr)  | Simplified access to required input data (recommended⭐) |
| [`GetOutputPackage()`](#getoutputpackage)         | Get output data package pointer                         |
| [`AddOutputDataPtr()`](#addoutputdataptr)         | Add data to output package                              |
| [`GetOutputDataTypes()`](#getoutputdatatypes)     | Get output data type list                               |
| [`ClearOutputPackage()`](#clearoutputpackage)     | Clear output data package                               |

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

### Profiling (MethodPresetProfiler) | Profiling

| Function                                  | Brief Description             |
| ----------------------------------------- | ----------------------------- |
| [`EnableProfiling()`](#enableprofiling)   | Enable performance analysis   |
| [`DisableProfiling()`](#disableprofiling) | Disable performance analysis  |
| [`GetProfilerInfo()`](#getprofilerinfo)   | Get performance analysis info |

```{seealso}
- [Prior Information and Ground Truth Setup Details](prior_and_gt.md)
- [Performance Analysis System Details](../advanced_development/profiler.md)
```

---

(methodpreset-interface-details)=
## Interface Introduction

### Required Override Interfaces

```{important}
Each method plugin must implement one of the following core interfaces:

- ** Method Name**: Only **declare** in header file
  ```cpp
  virtual const std::string& GetType() const override;
  // Returns plugin's unique type string identifier.
  // Automatically implemented by REGISTRATION_PLUGIN macro, no manual implementation needed
  // Example: `"method_sift"`, `"method_matches2tracks"`
  ```

- **Algorithm Main Entry**: Choose between Build and Run functions, recommend using Run function
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


### Optional Override Interfaces (for `MethodPreset` and its derived classes)

- **Set algorithm's prior information:**
```cpp
virtual void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");
// (1) `MethodPreset` sets prior information for algorithms requiring additional guidance information (such as initial poses, weights).
// (2) `data_ptr` is prior information data pointer, `type` is prior information type string.

virtual void ResetPriorInfo();
// Reset prior information (clear all prior information)
```

```{seealso}
For detailed descriptions and usage examples of prior information, required input data, and ground truth data, please refer to [Prior Information and Ground Truth Setup](prior-and-gt).
```

```cpp
virtual CopyrightData Copyright() const;
```

`Copyright()` is a virtual function provided by the `Method` base class for plugin developers to declare copyright information. The system automatically calls this function when the method is created, and `CopyrightManager` collects and manages copyright data. For details, see [Copyright Tracking Feature](copyright-tracking).

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
  - Automatically records execution time, memory usage, and can export CSV reports.
  - Can control whether to enable analysis through `enable_profiling` option.
- **`Interface::RobustEstimator<TSample>`** (derived from `MethodPresetProfiler`)
  - (Functionality to be expanded)

## Example

The `method_options_` assignments in the example code below are only for demonstrating the existence of configuration items. Actual default values should be provided when calling `GetOptionAs...()`.

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

    // Optional: If inheriting MethodPreset, can override GetInputTypes
    // const std::vector<std::string>& GetInputTypes() const override;

    // Optional: Override Copyright() to declare copyright information
    Interface::CopyrightData Copyright() const override;
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

    // ⚠️ Note: The following code only declares config items, NOT effective default values
    // Actual default values should be provided when calling GetOptionAs...()
    method_options_["threshold"] = "0.5";          // Only declares config item
    method_options_["max_iterations"] = "100";     // Only declares config item

    // Load default config file (if exists)
    InitializeDefaultConfigPath();
    // Initialize log directory
    InitializeLogDir();
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
    // auto result_poses_data = poses->CopyData(); // Assume DataGlobalPoses implements CopyData
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

```{warning}
**Parameter Default Value Setting in Constructor**

**Cannot** use `method_options_["..."] = "..."` in constructor to provide default values. Values set this way will be overwritten when configuration file is loaded.

**Correct Approach**:
- Directly provide default value parameter when calling `GetOptionAs...()` function
- For example: `0.5f` in `GetOptionAsFloat("threshold", 0.5f)` is the default value

**Incorrect Approach**:
```cpp
// ❌ Error: Don't set default values like this in constructor
method_options_["threshold"] = "0.5";
method_options_["max_iterations"] = "100";
```



## Method Parameter Configuration

- **Set Default Values**:
  - **Recommended Method**: Use member functions provided by `MethodPreset` base class for accessing configuration parameters (such as `GetOptionAsFloat`, `GetOptionAsIndexT`, etc.) to access config values, and provide default value parameters when calling.
  - Example: `float threshold = GetOptionAsFloat("threshold", 0.5f);` where `0.5f` is the default value
- **Configuration File (.ini)**:
  - Create `.ini` file with the same name as `GetType()` return value in `configs/methods/` directory (e.g., `my_method.ini`).
  - File format is standard INI format, containing a section with the same name as method type `[my_method]`.
  - Define `key=value` pairs under the section.
  - `MethodPreset` automatically loads this file, values in config file will override default values.
- **Runtime Settings**:
  - Can batch set options through `SetMethodOptions(const MethodOptions& options)`.
  - Can set options individually through `SetMethodOption(const MethodParams& key, const ParamsValue& value)`.
  - Runtime settings have highest priority and will override config file and default values.
- **Get Configuration Values**:
  - In `Run()` or other member functions, directly call member functions provided by `MethodPreset` base class such as `GetOptionAsString("param_key", "default_val")`, `GetOptionAsIndexT("param_key", 0)`, `GetOptionAsFloat("param_key", 0.0f)`, `GetOptionAsBool("param_key", false)`, etc. to safely get configuration values. These functions automatically search in `method_options_`, if not found or type mismatch, return the provided default value.
- **Parameter Setting Priority**: Runtime settings > Configuration file > Default values

```{important}
**Correct Method for Setting Default Values**

**❌ Incorrect**: Use `method_options_["param"] = "value"` in constructor to set default values
- Values set this way will be overwritten when configuration file is loaded
- If config file doesn't have this parameter, the parameter will become empty string or undefined

**✅ Correct**: Provide default values through function parameters when calling `GetOptionAs...()`
- For example: `GetOptionAsFloat("threshold", 0.5f)`
- When both config file and runtime don't set this parameter, will use `0.5f` as default value
```


## Core Data Types

The core library provides various predefined data types that can be directly used in plugins or as reference (see **Core Data Type Detailed Description**: Please refer to [Appendix A: PoSDK Core Data Types](../appendices/appendix_a_types.md))

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
#### `GetRequiredDataTypes()`

Get list of all required input data types.

**Function Signature**:
```cpp
std::vector<std::string> GetRequiredDataTypes() const;
```

**Return Value**: String vector of required data types

**Usage Example**:
```cpp
auto types = method->GetRequiredDataTypes();
for (const auto& type : types) {
    std::cout << "Required type: " << type << std::endl;
}
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
#### `GetOutputDataTypes()`

Get list of all data types in output data package.

**Function Signature**:
```cpp
std::vector<std::string> GetOutputDataTypes() const;
```

**Return Value**: String vector of output data types

**Usage Example**:
```cpp
auto output_types = method->GetOutputDataTypes();
for (const auto& type : output_types) {
    std::cout << "Output type: " << type << std::endl;
}
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

### Profiling Functions (MethodPresetProfiler)

(enableprofiling)=
#### `EnableProfiling()`

Enable performance analysis. For details, see [Performance Analysis System](../advanced_development/profiler.md).

---

(disableprofiling)=
#### `DisableProfiling()`

Disable performance analysis.

---

(getprofilerinfo)=
#### `GetProfilerInfo()`

Get performance analysis information.
