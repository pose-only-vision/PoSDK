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

##  数据类型转换与获取

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

### String2IndexT (已废弃)
- **注意**: 此函数已从 `types.hpp` 中移除，相关功能由 `MethodPreset::GetOptionAsIndexT()` 替代。

### String2Float (已废弃)
- **注意**: 此函数已从 `types.hpp` 中移除，相关功能由 `MethodPreset::GetOptionAsFloat()` 替代。

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

