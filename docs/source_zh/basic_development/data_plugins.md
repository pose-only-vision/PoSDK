# 数据插件 (Data Plugin)

数据插件负责数据的封装、加载、保存和访问。

## 接口介绍

### 必须重载接口

```{important}
每个数据插件必须实现以下核心接口，这些接口是插件能够正常工作的基础：

- ** `virtual const std::string& GetType() const override;`**
  - 返回插件的唯一类型字符串标识
  - 只需在头文件中**声明**，由`REGISTRATION_PLUGIN`宏自动实现
  - **示例**: `"data_images"`, `"data_tracks"`
- **`virtual void* GetData() override;`**
  - 返回指向插件内部核心数据存储的 `void*` 指针。用户在使用 `GetDataPtr<T>` 时进行类型智能转换。
```

### 可选重载接口

- **读写接口**：
```cpp
virtual bool Save(const std::string& folder = "", 
                const std::string& filename = "", 
                const std::string& extension = ".pb") override;
virtual bool Load(const std::string& filepath = "", 
                const std::string& file_type = "pb") override;
```
用户可以重载Save/Load方法，实现将数据保存到文件的逻辑。
- **回调函数接口**：实现自定义的回调函数接口，用于外部调用插件的特定功能。
```cpp
virtual void Call(std::string func, ...);
```
  使用 `FUNC_INTERFACE_BEGIN`, `CALL`, `FUNC_INTERFACE_END` 宏来定义接口。例如：
```cpp
FUNC_INTERFACE_BEGIN(MyDataPlugin)
 // 对应函数原型 void SetData(std::vector<double>& data, int id, const std::string& name)
CALL(void, SetData, REF(std::vector<double>), VAL(int), STR())
FUNC_INTERFACE_END()
```
在 interfaces.hpp 中提供了参数类型宏 - 简化参数获取
```cpp
#define REF(type) (*va_arg(argp, type*)) // 获取指针/引用类型参数
#define VAL(type) va_arg(argp, type) // 获取基本类型参数
#define STR() std::string(va_arg(argp, const char*)) // 获取字符串类型参数
```

- **`virtual DataPtr CopyData() const override;`**
  - 实现数据的深拷贝逻辑，输出的DataPtr与原本DataPtr指向数据内容一样，地址无关。

## 可选派生方式

- **虚基类**：**`Interface::DataIO`**
  - 提供最基本的数据插件接口。
- **序列化类**：**`Interface::PbDataIO`**
  - 提供基于 Protobuf 的序列化和反序列化支持。
  - 派生类需要实现 `CreateProtoMessage`, `ToProto`, `FromProto`。
  - 提供了 `PROTO_SET_*` 和 `PROTO_GET_*` 系列宏简化序列化代码。
  - 自动处理文件保存 (`Save`) 和加载 (`Load`)。
- **映射类**：`DataMap<T>`
  - 提供数据映射功能的接口，用户可以作为临时的DataPtr数据类型使用，不用每次都要写单独的数据插件。
  - 提供 `GetDataPtr<T>` 方法获取映射的值。


## 示例

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
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    void* GetData() override {
        return static_cast<void*>(&data_);
    }
    // 可选：实现 Save/Load/Call/CopyData
};
} // namespace MyPlugin

// my_data_plugin.cpp
#include "my_data_plugin.hpp"

// 插件注册 - GetType() 由宏自动实现
REGISTRATION_PLUGIN(MyPlugin::MyDataPlugin, "my_custom_data")
```

## 数据配置

数据插件通常不使用独立的 `.ini` 配置文件进行参数设置。其配置和数据填充主要通过成员变量和以下方式进行：

1.  **`Load()` 方法**: 对于支持从文件加载的数据插件（如 `DataTracks`, `DataGlobalPoses`, `PbDataIO` 派生类），可以通过调用 `Load(filepath, file_type)` 来加载指定路径和类型的数据。
    ```cpp
    auto tracks_data = FactoryData::Create("data_tracks");
    tracks_data->Load("/path/to/your/tracks.tracks", "tracks");
    ```

2.  **`Call()` 回调接口**: 插件可以定义自己的成员函数，并通过 `Call()` 接口暴露出来，允许外部代码传递参数或触发特定操作。
    ```cpp
    // 假设 MyDataPlugin 有一个 SetValues(const std::vector<double>&) 方法
    auto my_data = FactoryData::Create("my_custom_data");
    std::vector<double> values = {1.0, 2.0, 3.0};
    // 注意：传递引用参数需要取地址
    my_data->Call("SetValues", &values);
    ```
    这需要插件内部使用 `FUNC_INTERFACE_BEGIN`, `CALL`, `FUNC_INTERFACE_END` 宏定义好对应的接口。

3.  **`SetStorageFolder()`** (仅限 `PbDataIO` 派生类):
    -   对于继承自 `PbDataIO` 的插件（支持 Protobuf 序列化），可以使用 `SetStorageFolder(path)` 方法设置默认的文件保存和加载目录。
    -   当调用 `Save()` 或 `Load()` 时不指定完整路径，插件会使用这个默认目录。
    ```cpp
    auto serializable_data = FactoryData::Create("my_serializable_data");
    auto pbdata_io = std::dynamic_pointer_cast<PbDataIO>(serializable_data);
    if (pbdata_io) {
        pbdata_io->SetStorageFolder("/path/to/default/storage"); // 设置默认存储路径
        // 如果设置了默认存储路径，则Save可以省略路径，会保存到默认目录
        serializable_data->Save("", "my_data_file", ".pb"); 
    }
    ```

4.  **直接成员函数调用**: 用户可以在创建实例后直接调用其公有成员函数来获取核心数据来进行算法设计。
    ```cpp
    auto camera_data = FactoryData::Create("data_camera_model");
    auto camera_models = GetDataPtr<CameraModels>(camera_data);
    if (camera_models) {
        CameraModel cam; 
        cam.SetCameraIntrinsics(/*...*/);
        camera_models->push_back(cam);
    }
    ```

## 核心数据类型

核心库提供了多种预定义的数据类型，可以直接在插件中使用或作为参考（详见 **核心数据类型详细说明**：请参阅 [附录A：PoSDK核心数据类型](../appendices/appendix_a_types.md)）