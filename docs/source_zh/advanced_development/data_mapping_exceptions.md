# 数据映射与异常处理

## 数据访问与类型安全 (`GetDataPtr`)

- **核心**: 使用 `Interface::GetDataPtr<T>(data_ptr)` 模板函数进行安全的数据类型转换。
- **优点**: 避免直接使用 `static_cast` 带来的类型风险；统一数据访问接口。
- **示例**:
  ```cpp
  // 在方法插件的 Run() 中
  auto tracks_data = required_package_["data_tracks"];
  auto tracks = Interface::GetDataPtr<Tracks>(tracks_data);
  if (!tracks) {//也可以不写，因为GetDataPtr会自动检查
      std::cerr << "错误：输入的 data_tracks 类型不正确或数据为空！" << std::endl;
      return nullptr;
  }
  // 现在可以安全使用 tracks 指针
  for (const auto& track_info : *tracks) {
      // ...
  }
  ```

## 输入数据提供方式 (`MethodPreset` 派生类)

对于继承自 `MethodPreset` 或其派生类（如 `MethodPresetProfiler`, `RobustEstimator`）的方法插件，有以下几种方式可以提供所需的输入数据：

1.  **通过 `Build` 方法传入**: 
    -   在调用 `method_ptr->Build(data_ptr)` 时，可以将输入数据 `data_ptr` 直接传入。
    -   **前提**: 必须在插件的构造函数中，通过 `required_package_["data_type"] = nullptr;` 预先声明该方法所需的输入数据类型 (`data_type`)。
    -   如果需要传入多个不同类型的数据，应将它们打包在一个 `DataPackage` 中，然后将 `DataPackage` 指针传给 `Build`。
    ```cpp
    // 插件构造函数中声明
    required_package_["data_tracks"] = nullptr;
    required_package_["data_camera_model"] = nullptr;
    
    // 调用时
    auto data_package = std::make_shared<DataPackage>();
    data_package->AddData(tracks_data_ptr);
    data_package->AddData(camera_model_data_ptr);
    auto result = method_ptr->Build(data_package);
    ```

2.  **通过 `SetRequiredData` 方法设置**: 
    -   在调用 `Build` 之前，可以单独调用 `method_ptr->SetRequiredData(data_ptr)` 来设置输入数据。
    -   **前提**: 同样需要在插件的构造函数中声明所需的数据类型。
    -   如果需要设置多个数据，需要多次调用 `SetRequiredData`。
    ```cpp
    // 插件构造函数中声明
    required_package_["data_matches"] = nullptr;
    
    // 调用时
    method_ptr->SetRequiredData(matches_data_ptr);
    auto result = method_ptr->Build(); // Build 时可以不传参数
    ```

3.  **通过配置文件指定路径加载**: 
    -   可以在方法的 `.ini` 配置文件中，使用与 `required_package_` 中声明的数据类型 **同名的键** 来指定一个文件路径。
    
    ```ini
    # 在 my_method.ini 中
    [my_method]
    data_tracks = /path/to/my_tracks.pb 
    ...
    ```
    -   `MethodPreset` 的 `Build` 方法在检查输入时，如果发现某个 `required_package_` 中的 `DataPtr` 仍为 `nullptr`，它会尝试在 `method_options_` 中查找同名键，并调用对应 `DataIO` 子类的 `Load()` 方法从该路径加载数据。
    -   **注意**: 这要求对应的数据插件正确实现了 `Load()` 方法。
    ```cpp
    // 插件构造函数中声明
    required_package_["data_tracks"] = nullptr;
    
    // 调用时 (假设配置文件已加载)
    auto result = method_ptr->Build(); // 会自动尝试加载配置文件中指定的 data_tracks
    ```

4.  **通过 `SetPriorInfo` 方法传入 (用于先验信息)**:
    -   **目的**: 向方法插件传递额外的"指导"信息，这些信息不是核心输入数据，但可能影响算法行为（例如，RANSAC 的初始模型、优化的初始位姿估计、带权重的样本等）。
    -   **函数签名**: `void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");`
        -   `data_ptr`: 包含先验信息的 `DataPtr`。
        -   `type` (可选): 一个字符串标识符，用于区分不同类型的先验信息。如果留空，则行为会有所不同（见下文）。
    -   **两种模式**:
        1.  **`type` 为空**: 此时 `data_ptr` **必须** 是一个 `DataPackagePtr`，支持一次性设置多种先验信息的情况。
        2.  **`type` 不为空**: 此时 `data_ptr` 可以是**任意** `DataPtr`。`SetPriorInfo` 会将这个 `data_ptr` 以 `type` 字符串为键，**添加或更新** 到插件内部的 `prior_info_` 成员中。这适用于设置单个或特定类型的先验信息。
    -   **使用**: 在调用 `Build()` 之前调用此方法设置先验信息。
    -   **插件内部访问**: 在插件的 `Run()` 方法或其他成员函数中，通过访问受保护的 `prior_info_` 成员变量（`std::unordered_map<std::string, DataPtr>`）来获取设置的先验数据，并使用 `GetDataPtr<T>()` 进行类型转换。
    ```cpp
    // 示例：设置初始位姿作为先验信息
    auto initial_pose_data = std::make_shared<DataMap<RelativePose>>(initial_pose);
    method_ptr->SetPriorInfo(initial_pose_data, "initial_guess");
    
    // 示例：一次性设置多个先验信息
    auto prior_package = std::make_shared<DataPackage>();
    prior_package->AddData("weights", weights_data);
    prior_package->AddData("mask", mask_data);
    method_ptr->SetPriorInfo(prior_package); // type 为空
    
    auto result = method_ptr->Build(main_input_data);
    ```
    -   **重置**: 调用 `ResetPriorInfo()` 可以清空所有已设置的先验信息。

## 异常处理

- **策略**: PoSDK 核心库倾向于使用返回值 (`nullptr` 或 `false`) 表示错误 + cerr显示错误信息，而不是抛出异常 `throw()` 导致程序终止，以避免跨插件边界的异常处理复杂性。
- **后续**：考虑给Method制定log和error等级与id，方便用户调试
- **插件开发建议**:
  - 在插件内部可以使用 C++ 标准异常处理 (`try-catch`) 来捕获和处理内部错误。
  - 在覆盖的接口方法（如 `Run`, `Build`, `Save`, `Load`）中，应捕获所有内部异常，并转换为 `nullptr` 或 `false` 返回值。
  - 使用 `std::cerr` 输出详细的错误信息，帮助用户定位问题。
  - 检查从工厂函数 (`FactoryData::Create`, `FactoryMethod::Create`) 或 `GetDataPtr` 返回的指针是否为 `nullptr`。 