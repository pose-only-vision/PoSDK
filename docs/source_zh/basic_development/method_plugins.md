# 方法插件 (Method Plugin)

方法插件用于实现具体的算法逻辑和数据处理流程。

---

## MethodPreset 常用操作函数速查

以下列表涵盖`MethodPreset`及其派生类（`MethodPresetProfiler`）的常用操作函数。详细说明请参见[对应章节](#methodpreset-interface-details)。

### 数据管理 | Data Management

| 函数                                              | 功能简述                                  |
| ------------------------------------------------- | ----------------------------------------- |
| [`SetRequiredData()`](#setrequireddata)           | 设置单个必需输入数据                      |
| [`GetRequiredPackage()`](#getrequiredpackage)     | 获取必需输入数据包引用                    |
| [`GetInputTypes()`](#getinputtypes)               | 获取输入数据类型列表（推荐⭐）             |
| [`GetRequiredDataPtr<T>()`](#getrequireddataptr)  | 简化获取必需输入数据（推荐⭐）             |
| [`GetInputData()`](#getinputdata)                 | 按名称获取输入数据                        |
| [`GetInputDataAs<T>()`](#getinputdataas)          | 按名称获取类型化输入数据（推荐⭐）         |
| [`GetOutputPackage()`](#getoutputpackage)         | 获取输出数据包指针                        |
| [`AddOutputDataPtr()`](#addoutputdataptr)         | 添加数据到输出包                          |
| [`GetOutputTypes()`](#getoutputtypes)             | 获取输出数据类型列表（推荐⭐）             |
| [`CreateOutputData()`](#createoutputdata)         | 按名称创建输出数据                        |
| [`CreateOutputDataAs<T>()`](#createoutputdataas)  | 按名称创建类型化输出数据（推荐⭐）         |
| [`ClearOutputPackage()`](#clearoutputpackage)     | 清空输出数据包                            |
| ~~[`GetRequiredDataTypes()`](#getrequireddatatypes)~~ | ⚠️ 已废弃，请使用 `GetInputTypes()`   |
| ~~[`GetOutputDataTypes()`](#getoutputdatatypes)~~     | ⚠️ 已废弃，请使用 `GetOutputTypes()`  |

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

### 性能分析与评估（MethodPresetProfiler）| Profiling & Evaluation

| 函数                                                          | 功能简述                           |
| ------------------------------------------------------------- | ---------------------------------- |
| [`SetProfilerLabel()`](#setprofilerlabel)                     | 设置性能分析标签（字符串）         |
| [`SetProfilerLabels()`](#setprofilerlabels)                   | 设置结构化性能分析标签（键值对）   |
| [`GetProfilerLabel()`](#getprofilerlabel)                     | 获取性能分析标签字符串             |
| [`GetProfilerLabels()`](#getprofilerlabels)                   | 获取结构化性能分析标签             |
| [`HasStructuredLabels()`](#hasstructuredlabels)               | 检查是否有结构化标签               |
| [`SetEvaluatorAlgorithm()`](#setevaluatoralgorithm)           | 设置评估器中使用的算法名称         |
| [`GetEvaluatorAlgorithm()`](#getevaluatoralgorithm)           | 获取评估器中使用的算法名称         |
| [`ClearEvaluatorAlgorithm()`](#clearevaluatoralgorithm)       | 清空评估器算法设置                 |
| [`CallEvaluator()`](#callevaluator)                           | 调用评估器进行评估                 |

```{seealso}
- [先验信息与真值设置详细说明](prior_and_gt.md)
- [性能分析系统详细说明](../advanced_development/profiler.md)
```

---

(methodpreset-interface-details)=
## 接口介绍

### 必须重载接口

````{important}
每个方法插件需要声明 `GetType()`，并根据所选基类实现算法入口：

- **方法名称**：只需在头文件中**声明**
  ```cpp
  virtual const std::string& GetType() const override;
  // 返回插件的唯一类型字符串标识。
  // 由 REGISTRATION_PLUGIN 宏自动实现，不需要手动实现
  // 示例: `"method_sift"`, `"method_matches2tracks"`
  ```

- **算法主入口**：直接继承 `Method` 时实现 `Build()`；继承 `MethodPreset` 或 `MethodPresetProfiler` 时实现 `Run()`
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
  // （2）之后可以在Run函数中使用GetRequiredDataPtr<T>("data_type")（推荐）或GetDataPtr<T>(required_package_["data_type"])来获取输入数据。
  ```


````

### 先验信息与版权接口

- **设置算法的先验信息：**
```cpp
void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");
// （1）`MethodPreset` 设置先验信息，用于需要额外指导信息（如初始位姿、权重）的算法。
// （2）`data_ptr` 是先验信息数据指针，`type` 是先验信息类型字符串。

void ResetPriorInfo();
// 重置先验信息（清空所有先验信息）
```

```{seealso}
关于先验信息、必需输入数据、真值数据的详细说明和使用示例，请参阅 [先验信息与真值设置](prior_and_gt.md)。
```

```cpp
virtual CopyrightData Copyright() const;
```

`Copyright()` 是 `Method` 基类提供的虚函数，用于插件开发者声明版权信息。启用版权跟踪后，`MethodPreset::Build()` 的执行流程会通过 `CopyrightManager` 收集并管理版权数据，详细见[版权跟踪功能](../copyright_tracking.rst)。

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
  - 在 `Run()` 中启用性能分析宏后记录所选指标（默认仅时间），并可导出 CSV 报告。
  - 可以通过 `enable_profiling` 选项控制是否启用分析。
- **`Interface::RobustEstimator`** (派生自 `MethodPresetProfiler`)
  - 鲁棒估计器基类，提供模型估计和代价评估框架。
  - 支持自动初始化模型估计器和代价评估器。
  - 详见 [鲁棒估计器](../advanced_development/robust_estimators.md)。

## 示例

下面的示例通过 `GetOptionAs...()` 提供缺省值，并读取 INI 中的配置。

**CMakeLists.txt**:
```cmake
# 插件名称在这里定义（唯一定义处）
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
using namespace types; // 引入常用类型

class MyMethod : public MethodPresetProfiler { // 继承Profiler以获得性能分析
public:
    MyMethod();
    //  只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    DataPtr Run() override; // 实现核心逻辑

    // 输入类型通过 required_package_ 声明；GetInputTypes() 不是虚函数。

    // 可选: 重载Copyright()函数声明版权信息
    // Optional: Override Copyright() to declare copyright information
    // Interface::CopyrightData Copyright() const override;
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

    // 加载默认配置文件 (如果存在) | Load default config file (if exists)
    InitializeDefaultConfigPath();
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
    // Plugin registration 正确：在这里通过函数参数提供默认值
    // Plugin registration Correct: Provide default values via function parameters here
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
    // auto result_poses_data = GetInputData("data_global_poses")->CopyData();
    // auto result_poses = GetDataPtr<GlobalPoses>(result_poses_data);
    // ... 修改 result_poses ...
    // return result_poses_data;

    // 或者创建新的数据对象返回 | Or create and return new data object
    auto result_data = std::make_shared<DataMap<std::string>>("处理完成", "data_map_string");
    return result_data;
}

} // namespace MyPlugin

// Plugin registration 插件注册 - 自动从CMake读取PLUGIN_NAME
// 插件类型："my_method"（来自CMake）
REGISTRATION_PLUGIN(MyPlugin::MyMethod)
```

````{warning}
**构造函数中的参数默认值设置**

构造函数中预先写入 `method_options_` 的值会被 INI 中的同名项覆盖。推荐在 `GetOptionAs...()` 调用中提供缺省值，使配置项缺失或重置后仍有回退值。

**正确做法**：
- 在调用 `GetOptionAs...()` 函数时直接提供默认值参数
- 例如：`GetOptionAsFloat("threshold", 0.5f)` 中的 `0.5f` 就是默认值

**不推荐的做法**：
```cpp
// 同名 INI 项会覆盖以下值
method_options_["threshold"] = "0.5";
method_options_["max_iterations"] = "100";
```



````

## 方法参数配置

- **设置默认值**:
  - **推荐方式**：利用 `MethodPreset` 基类提供的获取配置参数的成员函数（如 `GetOptionAsFloat`, `GetOptionAsIndexT` 等）来访问配置值，并在调用时提供默认值参数。
  - 示例：`float threshold = GetOptionAsFloat("threshold", 0.5f);` 其中 `0.5f` 是默认值
- **配置文件 (.ini)**:
  - 在 `configs/methods/` 目录下创建与 `GetType()` 返回值同名的 `.ini` 文件 (例如 `my_method.ini`)。
  - 文件格式为标准的 INI 格式，包含一个与方法类型同名的节 `[my_method]`。
  - 在节下面定义 `key=value` 对。
  - `MethodPreset` 会自动加载此文件，配置文件中的值会覆盖默认值。
  - PoMVG GUI 的普通视觉分组请把参数继续放在 `[my_method]` 下，并使用 `## Group | 分组` 标记，例如 `## Input | 输入`、`## Algorithm | 算法`。额外的 `[section]` 是子 method/组件配置命名空间，不会被 `GetOptionAs...()` 自动读取。
- **运行时设置**:
  - 可以通过 `SetMethodOptions(const MethodOptions& options)` 批量设置选项。
  - 可以通过 `SetMethodOption(const MethodParams& key, const ParamsValue& value)` 单独设置选项。
  - 运行时设置的优先级最高，会覆盖配置文件和默认值。
- **获取配置值**:
  - 在 `Run()` 或其他成员函数中，直接调用 `MethodPreset` 基类提供的成员函数如 `GetOptionAsString("param_key", "default_val")`, `GetOptionAsIndexT("param_key", 0)`, `GetOptionAsFloat("param_key", 0.0f)`, `GetOptionAsBool("param_key", false)` 等来安全地获取配置值。这些函数会自动从 `method_options_` 中查找，如果未找到或类型不匹配，则返回提供的默认值。
- **参数设置的优先级**：运行时设置 > 配置文件 > 默认值

```{important}
**默认值的正确设置方式**

**配置覆盖**：预先写入 `method_options_` 的值可被同名 INI 项覆盖；INI 缺少该项时，`LoadMethodOptions()` 不会主动清除已有值。

**推荐**：在调用 `GetOptionAs...()` 时通过函数参数提供缺省值
- 例如：`GetOptionAsFloat("threshold", 0.5f)`
- 当配置文件和运行时都未设置该参数时，会使用 `0.5f` 作为默认值
```


## 核心数据类型

核心库提供了多种预定义的数据类型，可以直接在插件中使用或作为参考（详见 **核心数据类型详细说明**：请参阅 [附录A：PoMVG核心数据类型](../appendices/appendix_a_types.md)）

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
#### `GetRequiredDataTypes()` ⚠️ 已废弃

```{warning}
请使用 [`GetInputTypes()`](#getinputtypes) 替代。
```

获取所有必需输入数据的类型列表。

**函数签名**:
```cpp
[[deprecated("Use GetInputTypes() instead")]]
std::vector<std::string> GetRequiredDataTypes() const;
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
#### `GetOutputDataTypes()` ⚠️ 已废弃

```{warning}
请使用 [`GetOutputTypes()`](#getoutputtypes) 替代。
```

获取通过 `output_types_` 声明的输出类型列表。

**函数签名**:
```cpp
[[deprecated("Use GetOutputTypes() instead")]]
std::vector<std::string> GetOutputDataTypes() const;
```

---

(getinputtypes)=
#### `GetInputTypes()` （推荐）

获取输入数据类型列表。替代已废弃的 `GetRequiredDataTypes()`。

**函数签名**:
```cpp
const std::vector<std::string>& GetInputTypes() const;
```

**返回值**: 输入数据类型的字符串向量引用

**使用示例**:
```cpp
const auto& types = method->GetInputTypes();
for (const auto& type : types) {
    std::cout << "输入类型: " << type << std::endl;
}
```

---

(getoutputtypes)=
#### `GetOutputTypes()` （推荐）

获取输出数据类型列表。替代已废弃的 `GetOutputDataTypes()`。

**函数签名**:
```cpp
const std::vector<std::string>& GetOutputTypes() const;
```

**返回值**: 输出数据类型的字符串向量引用

---

(getinputdata)=
#### `GetInputData()`

按名称从 `required_package_` 获取输入数据。

**函数签名**:
```cpp
DataPtr GetInputData(const std::string &dataName) const;
```

**参数**:
- `dataName`: 数据在 `required_package_` 中的键名

**返回值**: 数据指针，未找到时返回 `nullptr`

---

(getinputdataas)=
#### `GetInputDataAs<T>()` （推荐）

按名称获取类型化输入数据指针。等价于 `GetDataPtr<T>(GetInputData(dataName))`。

**函数签名**:
```cpp
template <typename T>
std::shared_ptr<T> GetInputDataAs(const std::string &dataName) const;
```

**使用示例**:
```cpp
auto tracks = method->GetInputDataAs<Tracks>("data_tracks");
```

---

(createoutputdata)=
#### `CreateOutputData()`

按名称通过工厂创建输出数据，可选自动添加到输出包。

**函数签名**:
```cpp
DataPtr CreateOutputData(const std::string &dataName, bool addToPackage = true);
```

**参数**:
- `dataName`: 数据类型名称
- `addToPackage`: 是否自动添加到输出数据包（默认 `true`）

**返回值**: 创建的数据指针

---

(createoutputdataas)=
#### `CreateOutputDataAs<T>()` （推荐）

按名称创建类型化输出数据指针。

**函数签名**:
```cpp
template <typename T>
std::shared_ptr<T> CreateOutputDataAs(const std::string &dataName, bool addToPackage = true);
```

**使用示例**:
```cpp
auto result_poses = method->CreateOutputDataAs<GlobalPoses>("data_global_poses");
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

### 性能分析与评估函数（MethodPresetProfiler）

(setprofilerlabel)=
#### `SetProfilerLabel()`

设置性能分析标签（字符串版本）。

**函数签名**:
```cpp
void SetProfilerLabel(const std::string &profiler_label);
```

**参数**:
- `profiler_label`: 性能分析标签字符串

**使用示例**:
```cpp
method->SetProfilerLabel("MyAlgorithm_v2");
```

---

(setprofilerlabels)=
#### `SetProfilerLabels()`

设置结构化性能分析标签（键值对），用于CSV导出时使用自定义表头。

**函数签名**:
```cpp
void SetProfilerLabels(const std::unordered_map<std::string, std::string> &structured_labels);
```

**使用示例**:
```cpp
method->SetProfilerLabels({{"pipeline", "PoSDK"}, {"dataname", "test_data1"}});
```

---

(getprofilerlabel)=
#### `GetProfilerLabel()`

获取性能分析标签字符串。如果未设置，默认返回 `GetType()` 的值。

**函数签名**:
```cpp
std::string GetProfilerLabel() const;
```

---

(getprofilerlabels)=
#### `GetProfilerLabels()`

获取结构化性能分析标签。

**函数签名**:
```cpp
const std::unordered_map<std::string, std::string>& GetProfilerLabels() const;
```

---

(hasstructuredlabels)=
#### `HasStructuredLabels()`

检查是否设置了结构化标签。

**函数签名**:
```cpp
bool HasStructuredLabels() const;
```

---

(setevaluatoralgorithm)=
#### `SetEvaluatorAlgorithm()`

设置评估器中使用的算法名称，用于在评估器中区分不同的算法配置。如果不设置则默认使用 `GetType()` 的值。

**函数签名**:
```cpp
void SetEvaluatorAlgorithm(const std::string &algorithm_name);
```

---

(getevaluatoralgorithm)=
#### `GetEvaluatorAlgorithm()`

获取评估器中使用的算法名称。未设置时返回空字符串，调用方在需要时应使用 `GetType()` 作为备选。

**函数签名**:
```cpp
std::string GetEvaluatorAlgorithm() const;
```

---

(clearevaluatoralgorithm)=
#### `ClearEvaluatorAlgorithm()`

清空评估器算法设置，将 `algorithm_` 重置为空字符串。在每次测试运行前调用此方法，避免上一次运行设置的算法名缓存影响当前运行。

**函数签名**:
```cpp
void ClearEvaluatorAlgorithm();
```

---

(callevaluator)=
#### `CallEvaluator()`

调用评估器进行评估。该函数会拷贝输入数据，检查真值数据，并调用 `DataIO::Evaluate()` 函数进行评估。支持单个数据对象和 `DataPackage` 两种情况。

**函数签名**:
```cpp
bool CallEvaluator(const DataPtr &result_data);
```

**参数**:
- `result_data`: 待评估的结果数据

**返回值**: 是否成功调用评估

```{seealso}
- [性能分析系统详细说明](../advanced_development/profiler.md)
- [EvaluatorManager 精度评估器](../advanced_development/evaluator_manager.md)
```

## 可插拔双视图估计器契约

如果新 estimator 希望被 `two_view_estimator` 直接选择，不应要求 wrapper
预先知道它的插件 ID。插件除了继承 `MethodPresetProfiler`，还应实现可选的
能力接口：

```cpp
class MyEstimator final
    : public PoSDK::Interface::MethodPresetProfiler,
      public PoSDK::Interface::StaticTwoViewEstimatorCapabilities<
          PoSDK::Interface::TwoViewEstimatorInputContract::IdMatchesWithFeaturesAndCameras,
          PoSDK::Interface::TwoViewEstimatorPoseConvention::PoSDK,
          PoSDK::Interface::TwoViewEstimatorInlierContract::MatchesInPlace,
          PoSDK::Interface::TwoViewEstimatorRefinementContract::Unsupported>
{
    // Run() 与 GetType() ...
};
```

同时在严格 source manifest 中声明同一份契约。GUI 只读取静态 manifest，
不会为了生成下拉选项而在 GUI 进程内实例化原生插件：

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

manifest 与运行时 provider 必须一致。`BearingPairs` 类型应选择
`bearing_pairs + sample_best_inliers`；外部匹配类型应选择
`id_matches_features_cameras`，通常通过 `matches_in_place` 回传内点。
位姿约定和 refine 所有权属于不可变契约，不应作为普通用户参数切换。
契约 estimator 必须返回 `DataMap<RelativePose>`；声明
`estimator_optional` 或 `estimator_always` 后，wrapper 不会再重复执行外部
refine。
