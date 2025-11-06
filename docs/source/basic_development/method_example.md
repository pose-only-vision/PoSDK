# Plugin Development Quick Example

This document quickly introduces how to develop PoSDK plugins through simple examples. For detailed interface descriptions, please refer to [Data Plugin Interface Details](data_plugins.md) and [Method Plugin Interface Details](method_plugins.md).

---

## Plugin Registration System

All PoSDK plugins need to be registered through the `REGISTRATION_PLUGIN` macro, which is defined in `pomvg_plugin_register.hpp` (already included in `po_core.hpp`).

### Registration Macro Syntax

```cpp
REGISTRATION_PLUGIN(PluginClassName)
```

**Parameter Description**:
- **Parameter**: Plugin class name (e.g., `MyPlugin::MyMethod`, `MyPlugin::MyDataPlugin`)
- Plugin type string is automatically read from CMake `PLUGIN_NAME` macro

```{important}
** Automatic Implementation of `GetType()` Function**

- Macro **automatically implements** `GetType()` function
- Only **declare** `const std::string& GetType() const override;` in header file
- **Do not** manually implement `GetType()` in source file
- Plugin name is defined once in CMakeLists.txt, automatically used by the macro
```

**Notes**:
- Must be placed at the end of `.cpp` source file (cannot be placed in `.hpp` header file)
- Plugin name must be defined in CMakeLists.txt using `add_posdk_plugin(plugin_name ...)`
- Plugin type string is automatically read from CMake, ensuring consistency between CMake target, file name, and registration type

---

## Method Plugin Development Quick Example

Method plugins are used to implement specific algorithm logic. PoSDK provides three Method base classes to choose from:

### Base Class Selection Guide

| Base Class             | Applicable Scenarios                      | Required Override Functions | Main Features                           |
| ---------------------- | ----------------------------------------- | --------------------------- | --------------------------------------- |
| `Method`               | Simple algorithms, no config needed       | `Build()`, `GetType()`      | Most basic interface                    |
| `MethodPreset`         | Algorithms requiring config files         | `Run()`, `GetType()`        | Parameter management, input checking    |
| `MethodPresetProfiler` | Algorithms requiring performance analysis | `Run()`, `GetType()`        | Performance statistics, auto evaluation |

**Recommendation**: For research and comparison purposes, recommend using `MethodPresetProfiler`.

---

## Example 1: Simple Method Plugin

Most basic Method plugin, suitable for quick prototype validation.

### Code Example

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(my_simple_method
    PLUGIN_TYPE methods
    SOURCES my_simple_method.cpp
    HEADERS my_simple_method.hpp
)
```

**my_simple_method.hpp**:
```cpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

class MySimpleMethod : public Method {
public:
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    DataPtr Build(const DataPtr& material_ptr = nullptr) override;
};

} // namespace MyPlugin
```

**my_simple_method.cpp**:
```cpp
#include "my_simple_method.hpp"


namespace MyPlugin {

// Don't need to manually implement GetType(), macro automatically generates

DataPtr MySimpleMethod::Build(const DataPtr& material_ptr) {
    LOG_INFO_ZH << "执行简单算法";
    LOG_INFO_EN << "Running simple algorithm";

    // Implement algorithm logic...

    // Return result
    return std::make_shared<DataMap<std::string>>("算法执行完成", "data_map_string");
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "my_simple_method" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::MySimpleMethod)
```

### Usage

```cpp
auto method = FactoryMethod::Create("my_simple_method");
auto result = method->Build();
```

---

## Example 2: Method Plugin with Configuration

Using `MethodPreset` base class, supports configuration file management and input data checking.

### Code Example

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(my_preset_method
    PLUGIN_TYPE methods
    SOURCES my_preset_method.cpp
    HEADERS my_preset_method.hpp
)
```

**my_preset_method.hpp**:
```cpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

class MyPresetMethod : public MethodPreset {
public:
    MyPresetMethod();
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    DataPtr Run() override;
};

} // namespace MyPlugin
```

**my_preset_method.cpp**:
```cpp
#include "my_preset_method.hpp"

namespace MyPlugin {

MyPresetMethod::MyPresetMethod() {
    // Define required input data types
    required_package_["data_tracks"] = nullptr;
    required_package_["data_camera_models"] = nullptr;

    // Load configuration file (configs/methods/my_preset_method.ini)
    // method_options_ automatically loads from configuration file
    InitializeDefaultConfigPath();
    // Initialize log directory
    InitializeLogDir();
}

// Don't need to manually implement GetType(), macro automatically generates

DataPtr MyPresetMethod::Run() {
    // 1. Get input data (MethodPreset already automatically checks types)
    // Recommended: use GetRequiredDataPtr for simplified access
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");

    if (!tracks || !cameras) {
        LOG_ERROR_ZH << "缺少必要的输入数据";
        LOG_ERROR_EN << "Missing required input data";
        return nullptr;
    }

    // 2. Get configuration parameters (from config file or use defaults)
    float threshold = GetOptionAsFloat("threshold", 0.5f);
    int max_iter = GetOptionAsInt("max_iterations", 100);

    LOG_INFO_ZH << "阈值=" << threshold << ", 最大迭代=" << max_iter;
    LOG_INFO_EN << "Threshold=" << threshold << ", Max iterations=" << max_iter;

    // 3. Implement core algorithm
    // ... Algorithm implementation ...

    // 4. Return result
    return std::make_shared<DataMap<std::string>>("处理完成", "data_map_string");
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "my_preset_method" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::MyPresetMethod)
```

### Configuration File Example

**configs/methods/my_preset_method.ini**:
```ini
[my_preset_method]
threshold=0.8
max_iterations=200
output_path={root_dir}/results
```

### Usage

```cpp
// Create Method object
auto method = FactoryMethod::Create("my_preset_method");

// Prepare input data package
auto data_package = FactoryData::Create("data_package");
auto tracks_data = FactoryData::Create("data_tracks");
auto cameras_data = FactoryData::Create("data_camera_models");
// ... Populate data ...

data_package->Call("SetData", "data_tracks", &tracks_data);
data_package->Call("SetData", "data_camera_models", &cameras_data);

// Execute algorithm
auto result = method->Build(data_package);
```

---

## Example 3: Method Plugin with Performance Analysis

Using `MethodPresetProfiler` base class, automatically performs performance statistics and evaluation.

### Code Example

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(my_profiler_method
    PLUGIN_TYPE methods
    SOURCES my_profiler_method.cpp
    HEADERS my_profiler_method.hpp
)
```

**my_profiler_method.hpp**:
```cpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

class MyProfilerMethod : public MethodPresetProfiler {
public:
    MyProfilerMethod();
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    DataPtr Run() override;
};

} // namespace MyPlugin
```

**my_profiler_method.cpp**:
```cpp
#include "my_profiler_method.hpp"

namespace MyPlugin {

MyProfilerMethod::MyProfilerMethod() {
    // Define required input data types
    required_package_["data_tracks"] = nullptr;

    // Load configuration file (configs/methods/my_profiler_method.ini)
    // method_options_ automatically loads from configuration file
    InitializeDefaultConfigPath();
    InitializeLogDir();
}

// Don't need to manually implement GetType(), macro automatically generates

DataPtr MyProfilerMethod::Run() {
    // ✅ Performance analysis: Start session (automatically uses method labels)
    POSDK_START(true);  // Default: TIME only to save overhead
    
    // Performance analysis: Step 1 - Add stage checkpoint
    PROFILER_STAGE("preprocessing");
        // Preprocessing code...

    // Performance analysis: Step 2 - Add stage checkpoint
    PROFILER_STAGE("core_algorithm");
        // Core algorithm code...

    // Performance analysis: Step 3 - Add stage checkpoint
    PROFILER_STAGE("postprocessing");
        // Post-processing code...
    
    // ✅ Performance analysis: End session (automatically submits data)
    PROFILER_END();

    return std::make_shared<DataMap<std::string>>("Processing finished", "data_map_string");
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "my_profiler_method" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::MyProfilerMethod)
```

**Performance Profiling Macro Notes**:
- `POSDK_START(true)`: Start profiling session, automatically uses method's configured labels, defaults to TIME only (saves overhead)
- `PROFILER_STAGE("stage_name")`: Add stage checkpoint, automatically calculates interval performance metrics
- `PROFILER_END()`: End session and automatically submit data to ProfilerManager
- To collect more metrics, use `POSDK_START(true, "time|memory|cpu")`

### Configuration File Example

**configs/methods/my_profiler_method.ini**:
```ini
[my_profiler_method]
threshold=0.7
enable_profiling=true
enable_evaluation=true
```

### Usage

```cpp
auto method = FactoryMethod::Create("my_profiler_method");
auto data_package = FactoryData::Create("data_package");
// ... Prepare data ...

auto result = method->Build(data_package);

// Performance statistics automatically exported to CSV file
```

---

## DataIO Plugin Development Quick Example

DataIO plugins are used to encapsulate various data types. The simplest way is to inherit from `DataIO` base class.

### Code Example

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(my_custom_data
    PLUGIN_TYPE data
    SOURCES my_data_plugin.cpp
    HEADERS my_data_plugin.hpp
)
```

**my_data_plugin.hpp**:
```cpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;

struct CustomData {
    int id;
    std::string name;
    std::vector<double> values;
};

class MyDataPlugin : public DataIO {
private:
    CustomData data_;

public:
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    void* GetData() override;

    // Optional: implement Save/Load
    bool Save(const std::string& folder = "",
              const std::string& filename = "",
              const std::string& extension = ".dat") override;
    bool Load(const std::string& filepath = "",
              const std::string& file_type = "dat") override;
};

} // namespace MyPlugin
```

**my_data_plugin.cpp**:
```cpp
#include "my_data_plugin.hpp"

namespace MyPlugin {

// Don't need to manually implement GetType(), macro automatically generates

void* MyDataPlugin::GetData() {
    return static_cast<void*>(&data_);
}

bool MyDataPlugin::Save(const std::string& folder,
                        const std::string& filename,
                        const std::string& extension) {
    // Implement save logic...
    return true;
}

bool MyDataPlugin::Load(const std::string& filepath,
                        const std::string& file_type) {
    // Implement load logic...
    return true;
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "my_custom_data" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::MyDataPlugin)
```

### Usage

```cpp
// Create data object
auto data = FactoryData::Create("my_custom_data");

// Get internal data pointer
auto custom_data = GetDataPtr<CustomData>(data);
if (custom_data) {
    custom_data->id = 1;
    custom_data->name = "test";
    custom_data->values = {1.0, 2.0, 3.0};
}

// Save data
data->Save("/path/to/folder", "my_data");

// Load data
auto loaded_data = FactoryData::Create("my_custom_data");
loaded_data->Load("/path/to/folder/my_data.dat");
```

---

## Example 4: DataIO Plugin Supporting Protobuf Serialization

Using `PbDataIO` base class can easily implement protobuf serialization and persistence of data. This is PoSDK's recommended data plugin implementation method, suitable for scenarios requiring cross-platform storage and version compatibility.

```{seealso}
For complete PbDataIO development guide, serialization macro usage instructions, and advanced features, please refer to [Serialization Storage and Reading](../advanced_development/serialization.md).
```

### Code Example

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(data_example
    PLUGIN_TYPE data
    SOURCES data_example.cpp
    HEADERS data_example.hpp
)
```

**data_example.hpp**:
```cpp
#include <po_core.hpp>
#include <proto/my_example.pb.h>  // Protobuf generated header file

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

// Internal data structure
struct ExampleDataStruct {
    int id = 0;
    std::string name;
    std::vector<double> values;
};

class DataExample : public PbDataIO {
private:
    std::shared_ptr<ExampleDataStruct> data_ptr_;

public:
    DataExample();
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    void* GetData() override;
    DataPtr CopyData() const override;

protected:
    // Three protobuf interfaces that PbDataIO must implement
    std::unique_ptr<google::protobuf::Message> CreateProtoMessage() const override;
    std::unique_ptr<google::protobuf::Message> ToProto() const override;
    bool FromProto(const google::protobuf::Message& message) override;
};

} // namespace MyPlugin
```

**data_example.cpp**:
```cpp
#include "data_example.hpp"

namespace MyPlugin {

DataExample::DataExample() {
    data_ptr_ = std::make_shared<ExampleDataStruct>();

    // Set default storage directory
    auto storage_folder = std::filesystem::current_path() / "storage" / "examples";
    std::filesystem::create_directories(storage_folder);
    SetStorageFolder(storage_folder);
}

// Don't need to manually implement GetType(), macro automatically generates

void* DataExample::GetData() {
    return static_cast<void*>(data_ptr_.get());
}

DataPtr DataExample::CopyData() const {
    auto cloned = std::make_shared<DataExample>();
    auto cloned_struct = static_cast<ExampleDataStruct*>(cloned->GetData());
    if (cloned_struct && data_ptr_) {
        *cloned_struct = *data_ptr_;
    }
    cloned->SetStorageFolder(GetStorageFolder());
    return cloned;
}

// ========== Protobuf Serialization Implementation ==========

std::unique_ptr<google::protobuf::Message> DataExample::CreateProtoMessage() const {
    return std::make_unique<proto::ExampleData>();
}

std::unique_ptr<google::protobuf::Message> DataExample::ToProto() const {
    auto proto_msg = std::make_unique<proto::ExampleData>();
    if (!data_ptr_) return proto_msg;

    // Use macros to simplify serialization
    PROTO_SET_BASIC(proto_msg, id, data_ptr_->id);
    PROTO_SET_BASIC(proto_msg, name, data_ptr_->name);
    PROTO_SET_ARRAY(proto_msg, values, data_ptr_->values);

    return proto_msg;
}

bool DataExample::FromProto(const google::protobuf::Message& message) {
    const auto* proto_msg = dynamic_cast<const proto::ExampleData*>(&message);
    if (!proto_msg) return false;

    if (!data_ptr_) {
        data_ptr_ = std::make_shared<ExampleDataStruct>();
    }

    // Use macros to simplify deserialization
    PROTO_GET_BASIC(*proto_msg, id, data_ptr_->id);
    PROTO_GET_BASIC(*proto_msg, name, data_ptr_->name);
    PROTO_GET_ARRAY(*proto_msg, values, data_ptr_->values);

    return true;
}

} // namespace MyPlugin

// ✅ Plugin registration - automatically reads plugin name from CMake PLUGIN_NAME
// Plugin type: "data_example" (from CMake)
REGISTRATION_PLUGIN(MyPlugin::DataExample)
```

### Usage

```cpp
// Create data object
auto data = FactoryData::Create("data_example");

// Populate data
auto example_data = GetDataPtr<ExampleDataStruct>(data);
if (example_data) {
    example_data->id = 123;
    example_data->name = "test_example";
    example_data->values = {1.0, 2.0, 3.0, 4.0, 5.0};
}

// Save data (automatically serialized to protobuf format)
data->Save("/path/to/folder", "my_example");

// Load data (automatically deserialized)
auto loaded_data = FactoryData::Create("data_example");
loaded_data->Load("/path/to/folder/my_example.pb");
```

### PbDataIO Core Advantages

1. **Automatic File I/O**: `Save()` and `Load()` automatically handle file read/write and serialization
2. **Cross-platform Compatibility**: Protobuf ensures data portability across different platforms
3. **Convenient Macro Support**: Provides `PROTO_SET_*` and `PROTO_GET_*` macros to simplify serialization code
4. **Path Memory**: After saving, can directly call `Load()` without specifying path

```{tip}
**When to Use PbDataIO?**
- Need to save data to disk and share between different sessions
- Need cross-platform data exchange (Windows/Linux/macOS)
- Data structure is complex, containing Eigen types, arrays, etc.
- Need version compatibility (protobuf supports backward compatibility)

For detailed protobuf serialization macro list, `.proto` file definitions, advanced features, etc., please refer to [Serialization Storage and Reading](../advanced_development/serialization.md).
```

---

## Key Points Summary

### Method Plugin Development

1. **Must Override**:
   - ** `GetType()`**: Only **declare** in header file, automatically implemented by `REGISTRATION_PLUGIN` macro
   - `Build()` or `Run()`: Implement algorithm logic

2. **Base Class Selection**:
   - Simple algorithm → `Method`
   - Need configuration → `MethodPreset` (recommended)
   - Need performance analysis → `MethodPresetProfiler` (recommended for research)

3. **Configuration Management**:
   - Call `InitializeDefaultConfigPath()` in constructor to load configuration file
   - Configuration file path: `configs/methods/<method_type>.ini`
   - Use `GetOptionAsXXX(key, default_value)` to get parameters, returns default value when parameter doesn't exist
   - `method_options_` automatically loads from `.ini` file, no manual setup needed

### DataIO Plugin Development

1. **Must Override**:
   - ** `GetType()`**: Only **declare** in header file, automatically implemented by `REGISTRATION_PLUGIN` macro
   - `GetData()`: Return internal data pointer

2. **Optional Override**:
   - `Save()`/`Load()`: Data persistence
   - `CopyData()`: Deep copy

3. **Data Access**:
   - Use `GetDataPtr<T>(data_ptr)` to get typed pointer

4. **Recommend Using PbDataIO** (supports Protobuf serialization):
   - Inherit from `PbDataIO` base class
   - Implement three protobuf interfaces: `CreateProtoMessage()`, `ToProto()`, `FromProto()`
   - Use serialization macros to simplify code (`PROTO_SET_*`, `PROTO_GET_*`)
   - Automatically handles file I/O and cross-platform compatibility
   - See [Serialization Storage and Reading](../advanced_development/serialization.md) for details

### Plugin Registration

**CMakeLists.txt**:
```cmake
add_posdk_plugin(plugin_name  # ← Plugin name defined here
    PLUGIN_TYPE methods  # or "data"
    SOURCES plugin.cpp
    HEADERS plugin.hpp
)
```

**C++ code**:
```cpp
// Must be at end of .cpp file - automatically implements GetType()
// Plugin name automatically read from CMake PLUGIN_NAME macro
REGISTRATION_PLUGIN(MyPlugin::PluginClassName)
```



---

## Next Steps

- Deep Learning: [Data Plugin Interface Details](data_plugins.md)
- Deep Learning: [Method Plugin Interface Details](method_plugins.md)
- View Available Types: [Core Data Types](../appendices/appendix_a_types.md)
- Performance Analysis Tools: [Performance Analysis System](../advanced_development/profiler.md)
- Evaluation System: [Evaluation System](../advanced_development/evaluator_manager.md)
