# 方法插件 (Method Plugin)

方法插件用于实现具体的算法逻辑和数据处理流程。

---

## MethodPreset 常用操作函数速查

以下列表涵盖`MethodPreset`及其派生类（`MethodPresetProfiler`）的常用操作函数。详细说明请参见[对应章节](#methodpreset-interface-details)。

### 数据管理 | Data Management

| 函数                                              | 功能简述                      |
| ------------------------------------------------- | ----------------------------- |
| [`SetRequiredData()`](#setrequireddata)           | 设置单个必需输入数据          |
| [`GetRequiredPackage()`](#getrequiredpackage)     | 获取必需输入数据包引用        |
| [`GetRequiredDataTypes()`](#getrequireddatatypes) | 获取必需输入数据类型列表      |
| [`GetRequiredDataPtr<T>()`](#getrequireddataptr)  | 简化获取必需输入数据（推荐⭐） |
| [`GetOutputPackage()`](#getoutputpackage)         | 获取输出数据包指针            |
| [`AddOutputDataPtr()`](#addoutputdataptr)         | 添加数据到输出包              |
| [`GetOutputDataTypes()`](#getoutputdatatypes)     | 获取输出数据类型列表          |
| [`ClearOutputPackage()`](#clearoutputpackage)     | 清空输出数据包                |

### 配置管理 | Configuration Management

| 函数                                        | 功能简述               |
| ------------------------------------------- | ---------------------- |
| [`LoadMethodOptions()`](#loadmethodoptions) | 从配置文件加载方法选项 |
| [`SaveMethodOptions()`](#savemethodoptions) | 保存方法选项到配置文件 |
| [`GetMethodOptions()`](#getmethodoptions)   | 获取方法选项字典       |
| [`SetMethodOptions()`](#setmethodoptions)   | 批量设置方法选项       |
| [`SetMethodOption()`](#setmethodoption)     | 设置单个方法选项       |

### 参数访问 | Parameter Access

| 函数                                        | 功能简述                   |
| ------------------------------------------- | -------------------------- |
| [`GetOptionAsIndexT()`](#getoptionasindext) | 获取IndexT类型参数         |
| [`GetOptionAsInt()`](#getoptionasint)       | 获取int类型参数            |
| [`GetOptionAsFloat()`](#getoptionasfloat)   | 获取float类型参数          |
| [`GetOptionAsDouble()`](#getoptionasdouble) | 获取double类型参数         |
| [`GetOptionAsBool()`](#getoptionasbool)     | 获取bool类型参数           |
| [`GetOptionAsString()`](#getoptionasstring) | 获取string类型参数         |
| [`GetOptionAsPath()`](#getoptionaspath)     | 获取路径参数（支持占位符） |

### 先验信息与真值 | Prior Information & Ground Truth

| 函数                                  | 功能简述     |
| ------------------------------------- | ------------ |
| [`SetPriorInfo()`](#setpriorinfo)     | 设置先验信息 |
| [`ResetPriorInfo()`](#resetpriorinfo) | 重置先验信息 |
| [`SetGTData()`](#setgtdata)           | 设置真值数据 |
| [`GetGTData()`](#getgtdata)           | 获取真值数据 |

### 性能分析（MethodPresetProfiler）| Profiling

| 函数                                      | 功能简述         |
| ----------------------------------------- | ---------------- |
| [`EnableProfiling()`](#enableprofiling)   | 启用性能分析     |
| [`DisableProfiling()`](#disableprofiling) | 禁用性能分析     |
| [`GetProfilerInfo()`](#getprofilerinfo)   | 获取性能分析信息 |

```{seealso}
- [先验信息与真值设置详细说明](prior_and_gt.md)
- [性能分析系统详细说明](../advanced_development/profiler.md)
```

---

(methodpreset-interface-details)=
## 接口介绍

### 必须重载接口

```{important}
每个方法插件必须实现以下核心接口之一：

- ** 方法名称**：只需在头文件中**声明**
  ```cpp
  virtual const std::string& GetType() const override;
  // 返回插件的唯一类型字符串标识。
  // 由 REGISTRATION_PLUGIN 宏自动实现，不需要手动实现
  // 示例: `"method_sift"`, `"method_matches2tracks"`
  ```

- **算法主入口**：Build和Run函数二选一，推荐使用Run函数
  ```cpp
  virtual DataPtr Build(const DataPtr& material_ptr = nullptr) override;
  // 用于实现核心算法：
  // （1）material_ptr 是可选的输入数据。如果方法需要多个输入，将它们打包在 `DataPackage` 中传入。
  // （2）返回处理结果 (`DataPtr`)，如果处理失败或无结果，返回 `nullptr`。
  ```

  或者

  ```cpp
  virtual DataPtr Run() override;
  // 用于实现核心算法：基类的`Build` 方法会负责调用 `Run` 并处理输入/输出检查和性能分析等。
  // （1）用户需要在构造函数中设置输入数据类型：required_package_["data_type"] = nullptr;
  // （2）之后可以在Run函数中使用GetDataPtr<T>(required_package_["data_type"])来获取输入数据。
  ```


### 可选重载接口 (针对 `MethodPreset` 及其派生类)

- **设置算法的先验信息：**
```cpp
virtual void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");
// （1）`MethodPreset` 设置先验信息，用于需要额外指导信息（如初始位姿、权重）的算法。
// （2）`data_ptr` 是先验信息数据指针，`type` 是先验信息类型字符串。

virtual void ResetPriorInfo();
// 重置先验信息（清空所有先验信息）
```

```{seealso}
关于先验信息、必需输入数据、真值数据的详细说明和使用示例，请参阅 [先验信息与真值设置](prior-and-gt)。
```

```cpp
virtual CopyrightData Copyright() const;
```

`Copyright()` 是 `Method` 基类提供的虚函数，用于插件开发者声明版权信息。系统会在方法创建时自动调用此函数，由 `CopyrightManager` 收集并管理版权数据，详细见[版权跟踪功能](copyright-tracking)。

## 可选派生方式

- **`Interface::Method`** (基类)
  - 提供最基本的方法插件接口。需要自己处理输入检查、配置加载等。
- **`Interface::MethodPreset`** (派生自 `Method`)
  - 提供了预设的功能框架，简化开发：
    - **自动输入检查**: 基于 `required_package_` 成员变量检查输入数据类型。
    - **配置管理**: 支持通过 INI 文件配置 `method_options_`。
    - **先验信息**: 支持通过 `SetPriorInfo` 传递额外信息。
    - **核心逻辑分离**: 将核心算法实现在 `Run()` 中，`Build()` 负责流程控制。
- **`Interface::MethodPresetProfiler`** (派生自 `MethodPreset`)
  - 在 `MethodPreset` 基础上增加了**性能分析**功能。
  - 自动记录执行时间、内存使用，并可导出 CSV 报告。
  - 可以通过 `enable_profiling` 选项控制是否启用分析。
- **`Interface::RobustEstimator<TSample>`** (派生自 `MethodPresetProfiler`)
  - （功能待拓展）

## 示例

下面的示例代码中的 `method_options_` 赋值仅用于演示配置项的存在，实际默认值应在 `GetOptionAs...()` 调用时提供。


```cpp
// my_method.hpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;
using namespace types; // 引入常用类型

class MyMethod : public MethodPresetProfiler { // 继承Profiler以获得性能分析
public:
    MyMethod();
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    DataPtr Run() override; // 实现核心逻辑

    // 可选: 如果继承MethodPreset，可以重写GetInputTypes
    // const std::vector<std::string>& GetInputTypes() const override;

    // 可选: 重载Copyright()函数声明版权信息
    // Optional: Override Copyright() to declare copyright information
    Interface::CopyrightData Copyright() const override;
};
} // namespace MyPlugin

// my_method.cpp
#include "my_method.hpp"

namespace MyPlugin {

MyMethod::MyMethod() {
    // 定义需要的输入数据类型 | Define required input data types
    // 设置为nullptr表示需要这种类型的输入，但尚未提供
    // Setting to nullptr indicates this type is required but not yet provided
    required_package_["data_tracks"] = nullptr;
    required_package_["data_global_poses"] = nullptr;

    // ⚠️ 注意：以下代码仅用于声明配置项，不能作为有效的默认值
    // ⚠️ Note: The following code only declares config items, NOT effective default values
    // 实际默认值应在GetOptionAs...()调用时提供
    // Actual default values should be provided when calling GetOptionAs...()
    method_options_["threshold"] = "0.5";          // 仅声明配置项 | Only declares config item
    method_options_["max_iterations"] = "100";     // 仅声明配置项 | Only declares config item

    // 加载默认配置文件 (如果存在) | Load default config file (if exists)
    InitializeDefaultConfigPath();
    // 初始化日志目录 | Initialize log directory
    InitializeLogDir();
}

//  不需要手动实现 GetType()，宏会自动生成

DataPtr MyMethod::Run() {
    // 1. 获取输入数据 (MethodPreset已处理检查)
    // Get input data (MethodPreset has handled validation)
    // 推荐使用GetRequiredDataPtr简化访问 | Recommended: use GetRequiredDataPtr for simplified access
    auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
    auto poses = GetRequiredDataPtr<GlobalPoses>("data_global_poses");

    // 检查数据有效性 | Validate data
    if (!tracks || !poses) {
        std::cerr << "[" << GetType() << "] Error: Missing required input data." << std::endl;
        return nullptr;
    }

    // 2. 获取配置参数 | Get configuration parameters
    // ✅ 正确：在这里通过函数参数提供默认值
    // ✅ Correct: Provide default values via function parameters here
    float threshold = GetOptionAsFloat("threshold", 0.5f);      // 默认值: 0.5f
    int max_iter = GetOptionAsIndexT("max_iterations", 100);    // 默认值: 100

    std::cout << "[" << GetType() << "] Running with threshold=" << threshold
              << ", max_iterations=" << max_iter << std::endl;

    // 3. 实现核心算法逻辑... | Implement core algorithm logic...
    // ... process tracks and poses ...

    // 4. 创建并返回结果 (例如，返回处理后的位姿)
    // Create and return result (e.g., return processed poses)
    // 注意: 如果不希望修改输入数据，可以先深拷贝DataPtr
    // Note: If you don't want to modify input data, deep copy DataPtr first
    // auto result_poses_data = poses->CopyData(); // 假设DataGlobalPoses实现了CopyData
    // auto result_poses = GetDataPtr<GlobalPoses>(result_poses_data);
    // ... 修改 result_poses ...
    // return result_poses_data;

    // 或者创建新的数据对象返回 | Or create and return new data object
    auto result_data = std::make_shared<DataMap<std::string>>("Processing finished");
    return result_data;
}

} // namespace MyPlugin

// 注册插件
REGISTRATION_PLUGIN(MyPlugin::MyMethod, "my_method")
```

```{warning}
**构造函数中的参数默认值设置**

在构造函数中**不能**使用 `method_options_["..."] = "..."` 的方式来提供默认值。这种方式设置的值会被配置文件加载时覆盖。

**正确做法**：
- 在调用 `GetOptionAs...()` 函数时直接提供默认值参数
- 例如：`GetOptionAsFloat("threshold", 0.5f)` 中的 `0.5f` 就是默认值

**错误做法**：
```cpp
// ❌ 错误：不要在构造函数中这样设置默认值
method_options_["threshold"] = "0.5";
method_options_["max_iterations"] = "100";
```



## 方法参数配置

- **设置默认值**:
  - **推荐方式**：利用 `MethodPreset` 基类提供的获取配置参数的成员函数（如 `GetOptionAsFloat`, `GetOptionAsIndexT` 等）来访问配置值，并在调用时提供默认值参数。
  - 示例：`float threshold = GetOptionAsFloat("threshold", 0.5f);` 其中 `0.5f` 是默认值
- **配置文件 (.ini)**:
  - 在 `configs/methods/` 目录下创建与 `GetType()` 返回值同名的 `.ini` 文件 (例如 `my_method.ini`)。
  - 文件格式为标准的 INI 格式，包含一个与方法类型同名的节 `[my_method]`。
  - 在节下面定义 `key=value` 对。
  - `MethodPreset` 会自动加载此文件，配置文件中的值会覆盖默认值。
- **运行时设置**:
  - 可以通过 `SetMethodOptions(const MethodOptions& options)` 批量设置选项。
  - 可以通过 `SetMethodOption(const MethodParams& key, const ParamsValue& value)` 单独设置选项。
  - 运行时设置的优先级最高，会覆盖配置文件和默认值。
- **获取配置值**:
  - 在 `Run()` 或其他成员函数中，直接调用 `MethodPreset` 基类提供的成员函数如 `GetOptionAsString("param_key", "default_val")`, `GetOptionAsIndexT("param_key", 0)`, `GetOptionAsFloat("param_key", 0.0f)`, `GetOptionAsBool("param_key", false)` 等来安全地获取配置值。这些函数会自动从 `method_options_` 中查找，如果未找到或类型不匹配，则返回提供的默认值。
- **参数设置的优先级**：运行时设置 > 配置文件 > 默认值

```{important}
**默认值的正确设置方式**

**❌ 错误**：在构造函数中使用 `method_options_["param"] = "value"` 来设置默认值
- 这种方式设置的值会在配置文件加载时被覆盖
- 如果配置文件中没有该参数，则该参数会变成空字符串或未定义

**✅ 正确**：在调用 `GetOptionAs...()` 时通过函数参数提供默认值
- 例如：`GetOptionAsFloat("threshold", 0.5f)`
- 当配置文件和运行时都未设置该参数时，会使用 `0.5f` 作为默认值
```


## 核心数据类型

核心库提供了多种预定义的数据类型，可以直接在插件中使用或作为参考（详见 **核心数据类型详细说明**：请参阅 [附录A：PoSDK核心数据类型](../appendices/appendix_a_types.md)）

---

## MethodPreset 函数详细说明

本节详细说明`MethodPreset`类的各个操作函数。

### 数据管理函数

(setrequireddata)=
#### `SetRequiredData()`

设置单个必需输入数据到`required_package_`。

**函数签名**:
```cpp
bool SetRequiredData(const DataPtr &data_ptr);
```

**参数**:
- `data_ptr`: 要设置的数据指针

**返回值**: 设置成功返回true，失败返回false

**使用示例**:
```cpp
auto tracks_data = FactoryData::Create("data_tracks");
// ... 填充tracks_data ...
method->SetRequiredData(tracks_data);
```

---

(getrequiredpackage)=
#### `GetRequiredPackage()`

获取必需输入数据包的引用。

**函数签名**:
```cpp
const Package& GetRequiredPackage() const;  // const版本
Package& GetRequiredPackage();              // 非const版本
```

**返回值**: `required_package_`的引用

**使用示例**:
```cpp
const Package& pkg = method->GetRequiredPackage();
for (const auto& [key, value] : pkg) {
    std::cout << "数据类型: " << key << std::endl;
}
```

---

(getrequireddatatypes)=
#### `GetRequiredDataTypes()`

获取所有必需输入数据的类型列表。

**函数签名**:
```cpp
std::vector<std::string> GetRequiredDataTypes() const;
```

**返回值**: 必需数据类型的字符串向量

**使用示例**:
```cpp
auto types = method->GetRequiredDataTypes();
for (const auto& type : types) {
    std::cout << "需要类型: " << type << std::endl;
}
```

---

(getrequireddataptr)=
#### `GetRequiredDataPtr<T>()`  （推荐）

**简化的数据访问函数**,直接从`required_package_`获取类型化数据指针。

**函数签名**:
```cpp
template <typename T>
std::shared_ptr<T> GetRequiredDataPtr(const std::string &data_name) const;
```

**参数**:
- `data_name`: 数据在`required_package_`中的键名

**返回值**: 指向类型`T`的智能指针，失败时返回`nullptr`

**使用示例**:
```cpp
// 旧方式（不推荐）
auto tracks_old = GetDataPtr<Tracks>(required_package_["data_tracks"]);

// 新方式（推荐）
auto tracks = GetRequiredDataPtr<Tracks>("data_tracks");
auto poses = GetRequiredDataPtr<GlobalPoses>("data_global_poses");

if (!tracks || !poses) {
    LOG_ERROR_ZH << "缺少必要的输入数据";
    LOG_ERROR_EN << "Missing required input data";
    return nullptr;
}
```

```{tip}
**为什么推荐使用GetRequiredDataPtr?**
- 代码更简洁，减少冗余
- 自动处理查找和类型转换
- 更好的错误提示
```

---

(getoutputpackage)=
#### `GetOutputPackage()`

获取输出数据包的智能指针。

**函数签名**:
```cpp
DataPackagePtr GetOutputPackage() const;
```

**返回值**: 指向输出数据包的智能指针

**使用示例**:
```cpp
auto output_pkg = method->GetOutputPackage();
if (output_pkg) {
    // 访问输出数据...
    auto result = output_pkg->GetData("result_poses");
}
```

---

(addoutputdataptr)=
#### `AddOutputDataPtr()`

添加数据到输出数据包。

**函数签名**:
```cpp
void AddOutputDataPtr(const DataPtr &data_ptr, const std::string &data_name = "");
```

**参数**:
- `data_ptr`: 要添加的数据指针
- `data_name`: 数据名称（可选）

**使用示例**:
```cpp
// 方式1: 指定数据名称
auto result_poses = FactoryData::Create("data_global_poses");
AddOutputDataPtr(result_poses, "optimized_poses");

// 方式2: 使用数据类型作为名称
auto result_tracks = FactoryData::Create("data_tracks");
AddOutputDataPtr(result_tracks);  // 自动使用"data_tracks"作为名称

// 方式3: 添加DataPackage
auto sub_package = FactoryData::Create("data_package");
// ... 填充sub_package ...
AddOutputDataPtr(sub_package, "sub_results");
```

---

(getoutputdatatypes)=
#### `GetOutputDataTypes()`

获取输出数据包中所有数据的类型列表。

**函数签名**:
```cpp
std::vector<std::string> GetOutputDataTypes() const;
```

**返回值**: 输出数据类型的字符串向量

**使用示例**:
```cpp
auto output_types = method->GetOutputDataTypes();
for (const auto& type : output_types) {
    std::cout << "输出类型: " << type << std::endl;
}
```

---

(clearoutputpackage)=
#### `ClearOutputPackage()`

清空输出数据包中的所有数据。

**函数签名**:
```cpp
void ClearOutputPackage();
```

**使用示例**:
```cpp
// 清空之前的输出
method->ClearOutputPackage();

// 添加新的输出数据
method->AddOutputDataPtr(new_result, "updated_result");
```

---

### 配置管理函数

(loadmethodoptions)=
#### `LoadMethodOptions()`

从配置文件加载方法选项。

**函数签名**:
```cpp
void LoadMethodOptions(const std::string &config_file, const std::string &specific_method = "");
```

**参数**:
- `config_file`: 配置文件路径
- `specific_method`: 特定方法名称（可选）

---

(savemethodoptions)=
#### `SaveMethodOptions()`

保存方法选项到配置文件。

**函数签名**:
```cpp
void SaveMethodOptions(const std::string &config_file);
```

---

(getmethodoptions)=
#### `GetMethodOptions()`

获取方法选项字典。

**函数签名**:
```cpp
const MethodOptions& GetMethodOptions() const;
```

---

(setmethodoptions)=
#### `SetMethodOptions()`

批量设置方法选项。

**函数签名**:
```cpp
void SetMethodOptions(const MethodOptions &options);
```

---

(setmethodoption)=
#### `SetMethodOption()`

设置单个方法选项。

**函数签名**:
```cpp
void SetMethodOption(const MethodParams &option_type, const ParamsValue &content);
```

---

### 参数访问函数

(getoptionasindext)=
#### `GetOptionAsIndexT()`

获取IndexT类型参数。

**函数签名**:
```cpp
IndexT GetOptionAsIndexT(const MethodParams &key, IndexT default_value = 0) const;
```

---

(getoptionasint)=
#### `GetOptionAsInt()`

获取int类型参数。

**函数签名**:
```cpp
int GetOptionAsInt(const MethodParams &key, int default_value = 0) const;
```

---

(getoptionasfloat)=
#### `GetOptionAsFloat()`

获取float类型参数。

**函数签名**:
```cpp
float GetOptionAsFloat(const MethodParams &key, float default_value = 0.0f) const;
```

---

(getoptionasdouble)=
#### `GetOptionAsDouble()`

获取double类型参数。

**函数签名**:
```cpp
double GetOptionAsDouble(const MethodParams &key, double default_value = 0.0) const;
```

---

(getoptionasbool)=
#### `GetOptionAsBool()`

获取bool类型参数。

**函数签名**:
```cpp
bool GetOptionAsBool(const MethodParams &key, bool default_value = false) const;
```

---

(getoptionasstring)=
#### `GetOptionAsString()`

获取string类型参数。

**函数签名**:
```cpp
std::string GetOptionAsString(const MethodParams &key, const std::string &default_value = "") const;
```

---

(getoptionaspath)=
#### `GetOptionAsPath()`

获取路径参数，支持占位符替换。

**函数签名**:
```cpp
std::string GetOptionAsPath(const MethodParams &key, const std::string &root_dir = "", const std::string &default_value = "") const;
```

**支持的占位符**:
- `{root_dir}`: 根目录路径
- `{exe_dir}`: 可执行文件目录
- `{key_name}`: 引用其他配置项的值

**使用示例**:
```cpp
// 配置文件中: output_path={root_dir}/results
std::string output = GetOptionAsPath("output_path", "/home/user/project");
// 结果: /home/user/project/results
```

---

### 先验信息与真值函数

(setpriorinfo)=
#### `SetPriorInfo()`

设置先验信息。详见[先验信息与真值设置](prior_and_gt.md)。

**函数签名**:
```cpp
void SetPriorInfo(const DataPtr &data_ptr, const std::string &type = "");
```

---

(resetpriorinfo)=
#### `ResetPriorInfo()`

重置先验信息。

**函数签名**:
```cpp
void ResetPriorInfo();
```

---

(setgtdata)=
#### `SetGTData()`

设置真值数据。

**函数签名**:
```cpp
void SetGTData(DataPtr &gt_data);
```

---

(getgtdata)=
#### `GetGTData()`

获取真值数据。

**函数签名**:
```cpp
DataPtr GetGTData() const;
```

---

### 性能分析函数（MethodPresetProfiler）

(enableprofiling)=
#### `EnableProfiling()`

启用性能分析。详见[性能分析系统](../advanced_development/profiler.md)。

---

(disableprofiling)=
#### `DisableProfiling()`

禁用性能分析。

---

(getprofilerinfo)=
#### `GetProfilerInfo()`

获取性能分析信息。