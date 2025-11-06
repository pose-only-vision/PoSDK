# Data Plugin (Data Plugin)

Data plugins are responsible for data encapsulation, loading, saving, and access.

## Interface Introduction

### Required Override Interfaces

```{important}
Each data plugin must implement the following core interfaces, which are the foundation for the plugin to work properly:

- ** `virtual const std::string& GetType() const override;`**
  - Returns the plugin's unique type string identifier
  - Only **declare** in header file, automatically implemented by `REGISTRATION_PLUGIN` macro
  - **Example**: `"data_images"`, `"data_tracks"`
- **`virtual void* GetData() override;`**
  - Returns a `void*` pointer to the plugin's internal core data storage. Users perform type-safe conversion when using `GetDataPtr<T>`.
```

### Optional Override Interfaces

- **Read/Write Interfaces**:
```cpp
virtual bool Save(const std::string& folder = "", 
                const std::string& filename = "", 
                const std::string& extension = ".pb") override;
virtual bool Load(const std::string& filepath = "", 
                const std::string& file_type = "pb") override;
```
Users can override Save/Load methods to implement logic for saving data to files.
- **Callback Function Interface**: Implement custom callback function interfaces for external calls to specific plugin functionality.
```cpp
virtual void Call(std::string func, ...);
```
  Use `FUNC_INTERFACE_BEGIN`, `CALL`, `FUNC_INTERFACE_END` macros to define interfaces. For example:
```cpp
FUNC_INTERFACE_BEGIN(MyDataPlugin)
 // Corresponding function prototype void SetData(std::vector<double>& data, int id, const std::string& name)
CALL(void, SetData, REF(std::vector<double>), VAL(int), STR())
FUNC_INTERFACE_END()
```
Parameter type macros are provided in interfaces.hpp - simplifying parameter access
```cpp
#define REF(type) (*va_arg(argp, type*)) // Get pointer/reference type parameter
#define VAL(type) va_arg(argp, type) // Get basic type parameter
#define STR() std::string(va_arg(argp, const char*)) // Get string type parameter
```

- **`virtual DataPtr CopyData() const override;`**
  - Implement deep copy logic for data, output DataPtr has same data content as original DataPtr, addresses are independent.

## Optional Derivation Methods

- **Virtual Base Class**: **`Interface::DataIO`**
  - Provides most basic data plugin interface.
- **Serialization Class**: **`Interface::PbDataIO`**
  - Provides Protobuf-based serialization and deserialization support.
  - Derived classes need to implement `CreateProtoMessage`, `ToProto`, `FromProto`.
  - Provides `PROTO_SET_*` and `PROTO_GET_*` macro series to simplify serialization code.
  - Automatically handles file saving (`Save`) and loading (`Load`).
- **Mapping Class**: `DataMap<T>`
  - Provides data mapping functionality interface, users can use it as temporary DataPtr data type without writing separate data plugins each time.
  - Provides `GetDataPtr<T>` method to get mapped value.


## Example

```cpp
// my_data_plugin.hpp
#include <po_core.hpp>
#include <vector>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;

struct CustomData {
    int id;
    std::vector<double> values;
};

class MyDataPlugin : public DataIO {
private:
    CustomData data_;
public:
    // Only declare, automatically implemented by REGISTRATION_PLUGIN macro
    const std::string& GetType() const override;
    void* GetData() override {
        return static_cast<void*>(&data_);
    }
    // Optional: implement Save/Load/Call/CopyData
};
} // namespace MyPlugin

// my_data_plugin.cpp
#include "my_data_plugin.hpp"

// Plugin registration - GetType() automatically implemented by macro
REGISTRATION_PLUGIN(MyPlugin::MyDataPlugin, "my_custom_data")
```

## Data Configuration

Data plugins typically do not use independent `.ini` configuration files for parameter settings. Their configuration and data population are mainly done through member variables and the following methods:

1.  **`Load()` Method**: For data plugins that support loading from files (such as `DataTracks`, `DataGlobalPoses`, `PbDataIO` derived classes), data can be loaded by calling `Load(filepath, file_type)`.
    ```cpp
    auto tracks_data = FactoryData::Create("data_tracks");
    tracks_data->Load("/path/to/your/tracks.tracks", "tracks");
    ```

2.  **`Call()` Callback Interface**: Plugins can define their own member functions and expose them through the `Call()` interface, allowing external code to pass parameters or trigger specific operations.
    ```cpp
    // Assume MyDataPlugin has a SetValues(const std::vector<double>&) method
    auto my_data = FactoryData::Create("my_custom_data");
    std::vector<double> values = {1.0, 2.0, 3.0};
    // Note: Reference parameters need to take address
    my_data->Call("SetValues", &values);
    ```
    This requires the plugin to use `FUNC_INTERFACE_BEGIN`, `CALL`, `FUNC_INTERFACE_END` macros to define corresponding interfaces internally.

3.  **`SetStorageFolder()`** (Only for `PbDataIO` derived classes):
    -   For plugins inheriting from `PbDataIO` (supporting Protobuf serialization), you can use `SetStorageFolder(path)` method to set default file save and load directory.
    -   When calling `Save()` or `Load()` without specifying full path, plugin will use this default directory.
    ```cpp
    auto serializable_data = FactoryData::Create("my_serializable_data");
    auto pbdata_io = std::dynamic_pointer_cast<PbDataIO>(serializable_data);
    if (pbdata_io) {
        pbdata_io->SetStorageFolder("/path/to/default/storage"); // Set default storage path
        // If default storage path is set, Save can omit path, will save to default directory
        serializable_data->Save("", "my_data_file", ".pb"); 
    }
    ```

4.  **Direct Member Function Calls**: Users can directly call public member functions after creating instances to get core data for algorithm design.
    ```cpp
    auto camera_data = FactoryData::Create("data_camera_model");
    auto camera_models = GetDataPtr<CameraModels>(camera_data);
    if (camera_models) {
        CameraModel cam; 
        cam.SetCameraIntrinsics(/*...*/);
        camera_models->push_back(cam);
    }
    ```

## Core Data Types

The core library provides various predefined data types that can be directly used in plugins or as reference (see **Core Data Type Detailed Description**: Please refer to [Appendix A: PoSDK Core Data Types](../appendices/appendix_a_types.md))
