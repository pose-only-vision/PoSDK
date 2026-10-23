# 常用工具函数

## 配置参数获取

以下工具函数是 `MethodPreset` 类的成员函数，可被其派生类直接调用，用于从方法配置中安全地获取各种类型的参数值。

### GetOptionAsIndexT
- 功能: 从方法配置获取无符号整数值(IndexT/uint32_t)。
- 调用方式: `this->GetOptionAsIndexT(key, default_value)` 或在成员函数中直接调用 `GetOptionAsIndexT(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: IndexT - 默认值，当键不存在或转换失败时返回，默认为0。
- 返回值: IndexT类型的参数值。
- 使用示例:
  ```cpp
  // 在 MethodPreset 派生类的成员函数中：
  // IndexT max_iterations = this->GetOptionAsIndexT("max_iterations", 100);
  IndexT max_iterations = GetOptionAsIndexT("max_iterations", 100);
  ```

### GetOptionAsInt
- 功能: 从方法配置获取有符号整数值(int)。
- 调用方式: `this->GetOptionAsInt(key, default_value)` 或 `GetOptionAsInt(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: int - 默认值，当键不存在或转换失败时返回，默认为0。
- 返回值: int类型的参数值。
- 使用示例:
  ```cpp
  // int step_size = this->GetOptionAsInt("step_size", -1);
  int step_size = GetOptionAsInt("step_size", -1);
  ```

### GetOptionAsFloat
- 功能: 从方法配置获取浮点数值。
- 调用方式: `this->GetOptionAsFloat(key, default_value)` 或 `GetOptionAsFloat(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: float - 默认值，当键不存在或转换失败时返回，默认为0.0f。
- 返回值: float类型的参数值。
- 使用示例:
  ```cpp
  // float threshold = this->GetOptionAsFloat("threshold", 0.5f);
  float threshold = GetOptionAsFloat("threshold", 0.5f);
  ```

### GetOptionAsDouble
- 功能: 从方法配置获取双精度浮点数值。
- 调用方式: `this->GetOptionAsDouble(key, default_value)` 或 `GetOptionAsDouble(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: double - 默认值，当键不存在或转换失败时返回，默认为0.0。
- 返回值: double类型的参数值。
- 使用示例:
  ```cpp
  // double precision_value = this->GetOptionAsDouble("precision", 1e-6);
  double precision_value = GetOptionAsDouble("precision", 1e-6);
  ```

### GetOptionAsBool
- 功能: 从方法配置获取布尔值。
- 调用方式: `this->GetOptionAsBool(key, default_value)` 或 `GetOptionAsBool(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: bool - 默认值，当键不存在或转换失败时返回，默认为false。
- 返回值: bool类型的参数值。
- 注意: 配置中的"true","yes","1","on"会被识别为true；"false","no","0","off"会被识别为false。
- 使用示例:
  ```cpp
  // bool enable_feature = this->GetOptionAsBool("enable_feature", false);
  bool enable_feature = GetOptionAsBool("enable_feature", false);
  ```

### GetOptionAsString
- 功能: 从方法配置获取字符串值。
- 调用方式: `this->GetOptionAsString(key, default_value)` 或 `GetOptionAsString(key, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `default_value`: const std::string& - 默认值，当键不存在时返回，默认为空字符串。
- 返回值: std::string类型的参数值。
- 使用示例:
  ```cpp
  // std::string mode = this->GetOptionAsString("algorithm_mode", "default");
  std::string mode = GetOptionAsString("algorithm_mode", "default");
  ```

### GetOptionAsPath
- 功能: 从方法配置获取路径值，支持占位符替换。支持 `{exe_dir}`、`{root_dir}` 和通用 `{key_name}` 占位符。
- 调用方式: `this->GetOptionAsPath(key, root_dir, default_value)` 或 `GetOptionAsPath(key, root_dir, default_value)`。
- 参数:
  - `key`: const MethodParams& - 参数键名。
  - `root_dir`: const std::string& - 根目录路径，用于 `{root_dir}` 占位符替换。如果为空，则从 `method_options_` 中获取。默认为空字符串。
  - `default_value`: const std::string& - 默认值，当键不存在时返回，默认为空字符串。
- 返回值: std::string类型的参数值，占位符已被替换。
- 支持的占位符:
  - `{root_dir}`: 根目录路径。优先使用 `root_dir` 参数，否则从 `method_options_` 中获取。
  - `{exe_dir}`: 可执行文件所在目录。
  - `{key_name}`: 引用 `method_options_` 中其他键值（注意：被引用的键值需要先存在）。
- 使用示例:
  ```cpp
  // 获取带占位符的路径
  std::string output_path = GetOptionAsPath("output_dir", "/project/root");
  // 如果配置包含: "output_dir = {root_dir}/results/{dataset_name}"
  // 且 method_options_ 包含: "dataset_name = test_data"
  // 结果: "/project/root/results/test_data"
  
  // 获取带 exe_dir 占位符的路径
  std::string config_path = GetOptionAsPath("config_file");
  // 如果配置包含: "config_file = {exe_dir}/../configs/settings.ini"
  // 结果: "/path/to/executable/../configs/settings.ini"
  ```

## 数据类型转换与获取

### GetDataPtr<T>函数
- 功能: 安全地获取并转换数据指针到指定类型。该函数能够智能处理普通的 `DataPtr` 和封装多个数据项的 `DataPackage`。
- 模板参数:
  - `T`: 要转换到的目标数据结构类型 (例如 `GlobalPoses`, `Tracks`)。
- 参数:
  - `data_ptr_or_package_ptr`: const Interface::DataPtr& - 指向单个 `DataIO` 对象或 `DataPackage` 对象的智能指针。
  - `data_name_in_package` (可选): const std::string& - 如果第一个参数是 `DataPackage`，则此参数指定包内目标数据项的名称。默认为空字符串。
- 返回值: `std::shared_ptr<T>`，指向类型T的数据。如果转换失败或数据项未找到，则返回 `nullptr`。
- 工作原理:
  1. 检查输入 `data_ptr_or_package_ptr` 是否为 `nullptr`。
  2. 判断输入是指向 `DataPackage` 还是普通的 `DataIO` 对象：
     - 如果是 `DataPackage`：
       - 要求必须提供 `data_name_in_package`。
       - 从 `DataPackage` 中根据名称提取目标 `DataIO` 对象。
     - 如果是普通的 `DataIO` 对象：
       - `data_name_in_package` 参数（如果提供）将被忽略。
       - 直接使用此 `DataIO` 对象。
  3. 从目标 `DataIO` 对象中调用 `GetData()` 获取原始 `void*` 指针。
  4. 将 `void*` 指针 `static_cast` 为 `T*`。
  5. 使用 `std::shared_ptr` 的别名构造函数 (aliasing constructor)，创建一个新的 `std::shared_ptr<T>`，它指向内部的 `T` 类型数据，但与原始的 `DataIO` 对象（或 `DataPackage` 中的 `DataIO` 对象）共享所有权。这确保了只要返回的 `std::shared_ptr<T>` 存在，底层数据就不会被释放。
- 使用示例:
  ```cpp
  // 示例1: 从普通的 DataPtr 获取 GlobalPoses
  Interface::DataPtr single_pose_data_ptr = some_plugin->Run(); // 假设返回单个位姿数据
  auto poses = GetDataPtr<GlobalPoses>(single_pose_data_ptr);
  if (poses) {
      // 使用 poses
  }

  // 示例2: 从 DataPackage 中获取 Tracks 数据
  Interface::DataPtr package_ptr = another_plugin->Run(); // 假设返回一个 DataPackage
  auto tracks = GetDataPtr<Tracks>(package_ptr, "output_tracks"); // "output_tracks" 是在包中定义的名称
  if (tracks) {
      // 使用 tracks
  }
  ```

## 字符串转换工具函数

### String2Float
- 功能: 将字符串转换为float类型
- 参数:
  - value: const std::string& - 输入字符串
  - default_value: float - 转换失败时返回的默认值，默认为0.0f
- 返回值: 转换后的float值
- 使用场景: 用于配置文件字符串值到浮点数的安全转换
- 使用示例:
  ```cpp
  float threshold = String2Float("0.75", 0.5f); // 返回0.75
  float invalid = String2Float("xyz", 0.5f);    // 返回0.5
  ```

### String2Bool
- 功能: 将字符串转换为bool类型
- 参数:
  - str: const std::string& - 输入字符串
  - default_value: bool - 转换失败时返回的默认值，默认为false
- 返回值: 转换后的bool值
- 注意: "true","yes","1","on"会被转换为true；"false","no","0","off"会被转换为false
- 使用示例:
  ```cpp
  bool enabled = String2Bool("yes", false);    // 返回true
  bool disabled = String2Bool("off", true);    // 返回false
  bool invalid = String2Bool("maybe", false);  // 返回默认值false
  ```

### 已废弃的函数

- **String2IndexT (已废弃)**: 此函数已从 `types.hpp` 中移除，相关功能由 `MethodPreset::GetOptionAsIndexT()` 替代。

## 工厂高级函数

以下函数在 `FactoryBehavior`、`FactoryMethod`、`FactoryData` 三个工厂类中均可用（除特别说明外），定义在 `<po_core/factory.hpp>` 中。

### ManagePlugins
- 功能: 手动管理插件目录，支持动态重新指定插件加载路径。
- 签名: `static void ManagePlugins(const std::string &plugin_path, bool append = false)`
- 参数:
  - `plugin_path`: const std::string& - 新的插件目录路径。
  - `append`: bool - 为 `true` 时追加到现有插件映射，不清空已加载的插件；为 `false` 时清空现有映射后重新加载。默认为 `false`。
- 返回值: 无。
- 适用类: `FactoryBehavior`、`FactoryMethod`、`FactoryData`。
- 使用示例:
  ```cpp
  // 清空并重新加载指定目录的插件
  FactoryMethod::ManagePlugins("/path/to/new/plugins");

  // 追加加载额外的插件目录（保留已有插件）
  FactoryMethod::ManagePlugins("/path/to/extra/plugins", true);

  // 对数据工厂和行为工厂同样适用
  FactoryData::ManagePlugins("/path/to/data/plugins");
  FactoryBehavior::ManagePlugins("/path/to/behavior/plugins", true);
  ```

### RemovePlugins
- 功能: 移除指定路径下已加载的插件。
- 签名: `static int RemovePlugins(const std::string &plugin_path)`
- 参数:
  - `plugin_path`: const std::string& - 要移除的插件目录路径。
- 返回值: `int` - 成功移除的插件数量。
- 适用类: `FactoryBehavior`、`FactoryMethod`、`FactoryData`。
- 使用示例:
  ```cpp
  int removed_count = FactoryMethod::RemovePlugins("/path/to/old/plugins");
  // removed_count 为实际移除的插件数量
  ```

### GetRegisteredPaths
- 功能: 获取当前已注册的所有插件路径列表。
- 签名: `static std::vector<std::string> GetRegisteredPaths()`
- 参数: 无。
- 返回值: `std::vector<std::string>` - 已注册的插件目录路径列表。
- 适用类: `FactoryBehavior`、`FactoryMethod`、`FactoryData`。
- 使用示例:
  ```cpp
  auto paths = FactoryMethod::GetRegisteredPaths();
  for (const auto &path : paths) {
      std::cout << "Registered plugin path: " << path << std::endl;
  }
  ```

### FactoryMethod::Create\<TSample\>（模板化创建接口）
- 功能: 基于模板参数创建方法实例，用于需要指定样本数据类型的方法（如 `RANSACEstimator`、`GNCIRLSEstimator` 等）。
- 签名: `template <typename TSample> static Interface::MethodPtr Create(const std::string &type)`
- 模板参数:
  - `TSample`: 样本数据类型（如 `std::vector<Eigen::Vector2d>`）。
- 参数:
  - `type`: const std::string& - 方法类型名称。
- 返回值: `Interface::MethodPtr` - 创建的方法指针，失败返回 `nullptr`。
- 适用类: 仅 `FactoryMethod`。
- 注意: 使用此接口需要包含 `factory_template_impl.hpp` 头文件。
- 使用示例:
  ```cpp
  #include <po_core/factory_template_impl.hpp>

  // 创建带模板参数的 RANSAC 估计器
  auto ransac = FactoryMethod::Create<std::vector<Eigen::Vector2d>>("ransac_estimator");
  if (ransac) {
      ransac->SetRequiredData(input_data);
      auto result = ransac->Build();
  }
  ```

## 全局工具函数

以下是 `PoSDK` 命名空间下的全局工具函数。

### DisplayAllPluginVersions
- 功能: 显示所有已注册插件的版本信息。
- 头文件: `<po_core/factory.hpp>`
- 签名: `void PoSDK::DisplayAllPluginVersions()`
- 参数: 无。
- 返回值: 无。
- 使用示例:
  ```cpp
  #include <po_core/factory.hpp>

  int main() {
      // 加载插件
      FactoryMethod factory("/path/to/plugins");

      // 显示所有插件版本信息
      PoSDK::DisplayAllPluginVersions();
      return 0;
  }
  ```

### DisplayCopyrightSummary
- 功能: 显示所有已收集的版权信息列表和依赖关系图。适合在程序结束时调用，汇总展示运行过程中使用的所有组件的版权信息。
- 头文件: `<po_core/method_utils.hpp>`
- 签名: `void PoSDK::Interface::DisplayCopyrightSummary()`
- 参数: 无。
- 返回值: 无。
- 使用示例:
  ```cpp
  #include <po_core/method_utils.hpp>

  int main() {
      // ... 执行你的算法 ...

      // 程序结束前显示版权信息汇总
      PoSDK::Interface::DisplayCopyrightSummary();
      return 0;
  }
  ```

### CreateAndConfigureSubMethod
- 功能: 创建并配置子方法，自动从父方法继承参数配置和 Profiler 标签。这是一个通用工具函数，简化了在复合方法中创建和配置子方法的流程。
- 头文件: `<po_core/method_utils.hpp>`
- 签名: `Interface::MethodPresetProfilerPtr PoSDK::Interface::CreateAndConfigureSubMethod(const std::string &method_type, Interface::MethodPresetProfiler *parent_method = nullptr)`
- 参数:
  - `method_type`: const std::string& - 要创建的方法类型名称。
  - `parent_method`: Interface::MethodPresetProfiler* - 父方法实例指针，用于参数和 Profiler 标签继承。默认为 `nullptr`（不继承）。
- 返回值: `Interface::MethodPresetProfilerPtr` - 创建并配置好的方法实例。如果创建失败或类型转换失败，返回 `nullptr`。
- 工作流程:
  1. 通过 `FactoryMethod::Create()` 创建指定类型的方法实例。
  2. 将实例转换为 `MethodPresetProfiler` 类型。
  3. 如果提供了父方法，自动继承其参数配置（通过 `PassingMethodOptions`）和 Profiler 标签。
- 使用示例:
  ```cpp
  #include <po_core/method_utils.hpp>

  // 在某个父方法的 Build() 中创建子方法
  auto sub_method = PoSDK::Interface::CreateAndConfigureSubMethod("method_LiGT", this);
  if (sub_method) {
      sub_method->SetRequiredData(tracks_data);
      auto result = sub_method->Build();
  }

  // 不继承父方法参数的独立创建
  auto standalone = PoSDK::Interface::CreateAndConfigureSubMethod("method_rotation_averaging");
  ```

