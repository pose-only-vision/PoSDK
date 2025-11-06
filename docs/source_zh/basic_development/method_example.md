# 插件开发快速示例

本文档通过简单示例快速介绍如何开发PoSDK插件。详细的接口说明请参考 [数据插件接口详解](data_plugins.md) 和 [方法插件接口详解](method_plugins.md)。

---

## 插件注册系统

所有PoSDK插件都需要通过 `REGISTRATION_PLUGIN` 宏进行注册，该宏定义在 `pomvg_plugin_register.hpp` 中（已包含在 `po_core.hpp` 内）。

### 注册宏语法

```cpp
REGISTRATION_PLUGIN(PluginClassName, "plugin_type_string")
```

**参数说明**：
- **第一个参数**：插件的类名（如 `MyMethod`, `MyDataPlugin`）
- **第二个参数**：插件的类型字符串（如 `"my_method"`）

```{important}
** 自动实现 `GetType()` 函数**

- 宏会**自动实现** `GetType()` 函数
- 头文件中只需**声明** `const std::string& GetType() const override;`
- 源文件中**不需要**手动实现 `GetType()`

**宏的两种用法：**

| 用法   | 语法                                                  | 适用场景                    | 类型名      |
| ------ | ----------------------------------------------------- | --------------------------- | ----------- |
| 单参数 | `REGISTRATION_PLUGIN(MyClass)`                        | ⚠️ 仅 `PoSDKPlugin` 命名空间 | `"MyClass"` |
| 双参数 | `REGISTRATION_PLUGIN(Namespace::MyClass, "my_class")` | 任意命名空间                | 可自定义    |
```

**注意事项**：
- 必须放在 `.cpp` 源文件末尾（不能放在 `.hpp` 头文件中）
- 类型字符串在同类插件中必须唯一
- **单参数模式**：类必须定义在 `PoSDKPlugin` 命名空间中，宏会自动添加命名空间前缀
- **双参数模式**：支持任意命名空间，推荐使用

---

## Method插件开发快速示例

Method插件用于实现具体的算法逻辑。PoSDK提供了三种Method基类供选择：

### 基类选择指南

| 基类                   | 适用场景           | 必须重载函数           | 主要功能           |
| ---------------------- | ------------------ | ---------------------- | ------------------ |
| `Method`               | 简单算法，无需配置 | `Build()`, `GetType()` | 最基础接口         |
| `MethodPreset`         | 需要配置文件的算法 | `Run()`, `GetType()`   | 参数管理、输入检查 |
| `MethodPresetProfiler` | 需要性能分析的算法 | `Run()`, `GetType()`   | 性能统计、自动评估 |

**推荐**：对于研究和对比用途，建议使用 `MethodPresetProfiler`。

---

## 示例1：简单Method插件

最基础的Method插件，适合快速原型验证。

### 代码示例

**my_simple_method.hpp**:
```cpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

class MySimpleMethod : public Method {
public:
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    DataPtr Build(const DataPtr& material_ptr = nullptr) override;
};

} // namespace MyPlugin
```

**my_simple_method.cpp**:
```cpp
#include "my_simple_method.hpp"


namespace MyPlugin {

//  不需要手动实现 GetType()，宏会自动生成

DataPtr MySimpleMethod::Build(const DataPtr& material_ptr) {
    LOG_INFO_ZH << "执行简单算法";
    LOG_INFO_EN << "Running simple algorithm";

    // 实现算法逻辑...

    // 返回结果
    return FactoryData::Create("data_map_string");
}

} // namespace MyPlugin

//  插件注册 - GetType() 由宏自动实现（必须在.cpp文件末尾）
REGISTRATION_PLUGIN(MyPlugin::MySimpleMethod)
```

### 使用方式

```cpp
auto method = FactoryMethod::Create("my_simple_method");
auto result = method->Build();
```

---

## 示例2：带配置的Method插件

使用 `MethodPreset` 基类，支持配置文件管理和输入数据检查。

### 代码示例

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
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
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
    // 定义需要的输入数据类型
    required_package_["data_tracks"] = nullptr;
    required_package_["data_camera_models"] = nullptr;

    // 加载配置文件（configs/methods/my_preset_method.ini）
    // method_options_ 会自动从配置文件加载
    InitializeDefaultConfigPath();
    // 初始化日志目录
    InitializeLogDir();
}

//  不需要手动实现 GetType()，宏会自动生成

DataPtr MyPresetMethod::Run() {
    // 1. 获取输入数据（MethodPreset已自动检查类型）
    // 推荐使用GetRequiredDataPtr简化访问 | Recommended: use GetRequiredDataPtr for simplified access
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    auto cameras = GetRequiredDataPtr<CameraModels>("data_camera_models");

    if (!tracks || !cameras) {
        LOG_ERROR_ZH << "缺少必要的输入数据";
        LOG_ERROR_EN << "Missing required input data";
        return nullptr;
    }

    // 2. 获取配置参数（从配置文件或使用默认值）
    float threshold = GetOptionAsFloat("threshold", 0.5f);
    int max_iter = GetOptionAsInt("max_iterations", 100);

    LOG_INFO_ZH << "阈值=" << threshold << ", 最大迭代=" << max_iter;
    LOG_INFO_EN << "Threshold=" << threshold << ", Max iterations=" << max_iter;

    // 3. 实现核心算法
    // ... 算法实现 ...

    // 4. 返回结果
    return FactoryData::Create("data_map_string", "处理完成");
}

} // namespace MyPlugin

REGISTRATION_PLUGIN(MyPlugin::MyPresetMethod, "my_preset_method")
```

### 配置文件示例

**configs/methods/my_preset_method.ini**:
```ini
[my_preset_method]
threshold=0.8
max_iterations=200
output_path={root_dir}/results
```

### 使用方式

```cpp
// 创建Method对象
auto method = FactoryMethod::Create("my_preset_method");

// 准备输入数据包
auto data_package = FactoryData::Create("data_package");
auto tracks_data = FactoryData::Create("data_tracks");
auto cameras_data = FactoryData::Create("data_camera_models");
// ... 填充数据 ...

data_package->Call("SetData", "data_tracks", &tracks_data);
data_package->Call("SetData", "data_camera_models", &cameras_data);

// 执行算法
auto result = method->Build(data_package);
```

---

## 示例3：带性能分析的Method插件

使用 `MethodPresetProfiler` 基类，自动进行性能统计和评估。

### 代码示例

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
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
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
    // 定义需要的输入数据类型
    required_package_["data_tracks"] = nullptr;

    // 加载配置文件（configs/methods/my_profiler_method.ini）
    // method_options_ 会自动从配置文件加载
    InitializeDefaultConfigPath();
    InitializeLogDir();
}

//  不需要手动实现 GetType()，宏会自动生成

DataPtr MyProfilerMethod::Run() {
    // 性能分析：步骤1
    {
        PROFILER_SCOPED("预处理", GetType());
        // 预处理代码...
    }

    // 性能分析：步骤2
    {
        PROFILER_SCOPED("核心算法", GetType());
        // 核心算法代码...
    }

    // 性能分析：步骤3
    {
        PROFILER_SCOPED("后处理", GetType());
        // 后处理代码...
    }

    return FactoryData::Create("data_map_string");
}

} // namespace MyPlugin

REGISTRATION_PLUGIN(MyPlugin::MyProfilerMethod)
```

### 配置文件示例

**configs/methods/my_profiler_method.ini**:
```ini
[my_profiler_method]
threshold=0.7
enable_profiling=true
enable_evaluation=true
```

### 使用方式

```cpp
auto method = FactoryMethod::Create("my_profiler_method");
auto data_package = FactoryData::Create("data_package");
// ... 准备数据 ...

auto result = method->Build(data_package);

// 性能统计会自动导出到CSV文件
```

---

## DataIO插件开发快速示例

DataIO插件用于封装各种数据类型。最简单的方式是继承 `DataIO` 基类。

### 代码示例

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
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    void* GetData() override;

    // 可选：实现 Save/Load
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

//  不需要手动实现 GetType()，宏会自动生成

void* MyDataPlugin::GetData() {
    return static_cast<void*>(&data_);
}

bool MyDataPlugin::Save(const std::string& folder,
                        const std::string& filename,
                        const std::string& extension) {
    // 实现保存逻辑...
    return true;
}

bool MyDataPlugin::Load(const std::string& filepath,
                        const std::string& file_type) {
    // 实现加载逻辑...
    return true;
}

} // namespace MyPlugin

//  插件注册 - GetType() 由宏自动实现
REGISTRATION_PLUGIN(MyPlugin::MyDataPlugin, "my_custom_data")
```

### 使用方式

```cpp
// 创建数据对象
auto data = FactoryData::Create("my_custom_data");

// 获取内部数据指针
auto custom_data = GetDataPtr<CustomData>(data);
if (custom_data) {
    custom_data->id = 1;
    custom_data->name = "test";
    custom_data->values = {1.0, 2.0, 3.0};
}

// 保存数据
data->Save("/path/to/folder", "my_data");

// 加载数据
auto loaded_data = FactoryData::Create("my_custom_data");
loaded_data->Load("/path/to/folder/my_data.dat");
```

---

## 示例4：支持Protobuf序列化的DataIO插件

使用 `PbDataIO` 基类可以轻松实现数据的protobuf序列化和持久化。这是PoSDK推荐的数据插件实现方式，适合需要跨平台存储和版本兼容的场景。

```{seealso}
完整的PbDataIO开发指南、序列化宏使用说明和高级功能，请参阅 [序列化存储与读取](../advanced_development/serialization.md)。
```

### 代码示例

**data_example.hpp**:
```cpp
#include <po_core.hpp>
#include <proto/my_example.pb.h>  // Protobuf生成的头文件

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types;

// Internal data structure | 内部数据结构
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
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    void* GetData() override;
    DataPtr CopyData() const override;

protected:
    // PbDataIO必须实现的三个protobuf接口
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

    // 设置默认存储目录
    auto storage_folder = std::filesystem::current_path() / "storage" / "examples";
    std::filesystem::create_directories(storage_folder);
    SetStorageFolder(storage_folder);
}

//  不需要手动实现 GetType()，宏会自动生成

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

// ========== Protobuf序列化实现 ==========

std::unique_ptr<google::protobuf::Message> DataExample::CreateProtoMessage() const {
    return std::make_unique<proto::ExampleData>();
}

std::unique_ptr<google::protobuf::Message> DataExample::ToProto() const {
    auto proto_msg = std::make_unique<proto::ExampleData>();
    if (!data_ptr_) return proto_msg;

    // 使用宏简化序列化
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

    // 使用宏简化反序列化
    PROTO_GET_BASIC(*proto_msg, id, data_ptr_->id);
    PROTO_GET_BASIC(*proto_msg, name, data_ptr_->name);
    PROTO_GET_ARRAY(*proto_msg, values, data_ptr_->values);

    return true;
}

} // namespace MyPlugin

//  插件注册 - GetType() 由宏自动实现
REGISTRATION_PLUGIN(MyPlugin::DataExample, "data_example")
```

### 使用方式

```cpp
// 创建数据对象
auto data = FactoryData::Create("data_example");

// 填充数据
auto example_data = GetDataPtr<ExampleDataStruct>(data);
if (example_data) {
    example_data->id = 123;
    example_data->name = "test_example";
    example_data->values = {1.0, 2.0, 3.0, 4.0, 5.0};
}

// 保存数据（自动序列化为protobuf格式）
data->Save("/path/to/folder", "my_example");

// 加载数据（自动反序列化）
auto loaded_data = FactoryData::Create("data_example");
loaded_data->Load("/path/to/folder/my_example.pb");
```

### PbDataIO核心优势

1. **自动文件I/O**: `Save()`和`Load()`自动处理文件读写和序列化
2. **跨平台兼容**: Protobuf保证数据在不同平台间可移植
3. **便利宏支持**: 提供`PROTO_SET_*`和`PROTO_GET_*`宏简化序列化代码
4. **路径记忆**: 保存后可直接调用`Load()`无需指定路径

```{tip}
**何时使用PbDataIO?**
- 需要将数据保存到磁盘并在不同会话间共享
- 需要跨平台数据交换（Windows/Linux/macOS）
- 数据结构复杂，包含Eigen类型、数组等
- 需要版本兼容性（protobuf支持向后兼容）

详细的protobuf序列化宏列表、`.proto`文件定义、高级功能等，请参阅 [序列化存储与读取](../advanced_development/serialization.md)。
```

---

## 关键要点总结

### Method插件开发

1. **必须重载**：
   - ** `GetType()`**：只需在头文件中**声明**，由`REGISTRATION_PLUGIN`宏自动实现
   - `Build()` 或 `Run()`：实现算法逻辑

2. **基类选择**：
   - 简单算法 → `Method`
   - 需要配置 → `MethodPreset`（推荐）
   - 需要性能分析 → `MethodPresetProfiler`（推荐用于研究）

3. **配置管理**：
   - 在构造函数中调用 `InitializeDefaultConfigPath()` 加载配置文件
   - 配置文件路径：`configs/methods/<method_type>.ini`
   - 使用 `GetOptionAsXXX(key, default_value)` 获取参数，参数不存在时返回默认值
   - `method_options_` 会自动从 `.ini` 文件加载，无需手动设置

### DataIO插件开发

1. **必须重载**：
   - ** `GetType()`**：只需在头文件中**声明**，由`REGISTRATION_PLUGIN`宏自动实现
   - `GetData()`：返回内部数据指针

2. **可选重载**：
   - `Save()`/`Load()`：数据持久化
   - `CopyData()`：深拷贝

3. **数据访问**：
   - 使用 `GetDataPtr<T>(data_ptr)` 获取类型化指针

4. **推荐使用PbDataIO** (支持Protobuf序列化)：
   - 继承 `PbDataIO` 基类
   - 实现三个protobuf接口：`CreateProtoMessage()`, `ToProto()`, `FromProto()`
   - 使用序列化宏简化代码（`PROTO_SET_*`, `PROTO_GET_*`）
   - 自动处理文件I/O和跨平台兼容性
   - 详见 [序列化存储与读取](../advanced_development/serialization.md)

### 插件注册

```cpp
//  必须在.cpp文件末尾 - 自动实现GetType()
REGISTRATION_PLUGIN(PluginClassName, "type_string")
```



---

## 下一步

-  深入学习：[数据插件接口详解](data_plugins.md)
-  深入学习：[方法插件接口详解](method_plugins.md)
-  查看可用类型：[核心数据类型](../appendices/appendix_a_types.md)
-  性能分析工具：[性能分析系统](../advanced_development/profiler.md)
-  评估系统：[评估系统](../advanced_development/evaluator_manager.md)
