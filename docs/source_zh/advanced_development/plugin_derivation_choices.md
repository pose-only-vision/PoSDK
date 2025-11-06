# 详细的插件派生选择

## 数据插件 (Data Plugin)

-   **`Interface::DataIO`**:
    -   **用途**: 实现最基础的数据容器，或当数据不需要序列化/反序列化时使用。
    -   **优点**: 简单直接。
    -   **缺点**: 需要手动实现 `Save`/`Load`。
-   **`Interface::PbDataIO`**:
    -   **用途**: 需要将数据通过 Protobuf 进行序列化/反序列化（保存到文件或网络传输）的场景。
    -   **优点**: 自动处理文件 I/O；提供宏简化 Protobuf 字段映射。
    -   **缺点**: 引入 Protobuf 依赖；需要定义 `.proto` 文件并配置 CMake 生成代码。
    -   **关键实现**: 用户需要重载 `CreateProtoMessage`, `ToProto`, `FromProto`。推荐使用如下辅助宏（详情见 `interfaces_preset.hpp` 中）来简化实现：
    ```text
        -   `PROTO_SET_BASIC` / `PROTO_GET_BASIC`: 基础类型 (int, float, bool, string)。
        -   `PROTO_SET_VECTOR2F/D` / `PROTO_GET_VECTOR2F/D`: Eigen::Vector2f/d。
        -   `PROTO_SET_VECTOR3D` / `PROTO_GET_VECTOR3D`: Eigen::Vector3d。
        -   `PROTO_SET_MATRIX3D` / `PROTO_GET_MATRIX3D`: Eigen::Matrix3d。
        -   `PROTO_SET_ARRAY` / `PROTO_GET_ARRAY`: `std::vector<基础类型>`。
        -   `PROTO_SET_ENUM` / `PROTO_GET_ENUM`: 枚举类型。
    ```

-   **`Interface::DataMap<T>`**:
    -   **模板参数 `T`**: 任意可默认构造的类型。
    -   **用途**: 快速包装 **单个** 数据对象（如 `RelativePose`, `GlobalPoses`, `std::vector`, `std::map`, 自定义结构体等）为 `DataPtr`，方便在方法间传递或作为方法返回值，避免为简单数据结构创建完整的数据插件。
    -   **优点**: 方便将现有数据集成到 PoSDK 框架，不用专门声明和编译新的 Data 数据类。
    -   **缺点**: 本身不直接支持序列化 (`PbDataIO`)，如果需要持久化，需要包含它的插件（如 `DataPackage`）或外部逻辑来处理。
    -   **访问**: 使用 `GetDataPtr<T>(data_map_ptr)` 获取内部数据的指针。
    -   **深拷贝 (`CopyData`)**:
        -   `DataMap<T>` 提供了 `CopyData()` 方法用于创建数据的深拷贝。
        -   深拷贝的效果取决于模板参数 `T` 类型的拷贝构造函数实现。如果 `T` 类型实现了深拷贝，则 `DataMap<T>::CopyData()` 也会进行深拷贝；如果 `T` 类型只有浅拷贝，则 `CopyData()` 的结果也是浅拷贝。

    ```{note}
    对于包含指针成员或需要深拷贝的自定义类型，请确保该类型正确实现了深拷贝构造函数。
    ```
    -   **评估 (`Evaluate`)**: 
        -   `DataMap<T>` 提供了 `Evaluate(DataPtr gt_data)` 的虚函数实现。
        -   **特化处理**: 目前已为 `T = RelativePose` 类型进行了特化，可以直接计算旋转和平移误差，并将结果 (`rotation_error`, `translation_error`) 添加到 `EvaluatorStatus` 中，`eval_type` 会被设置为 `"RelativePose"`。
        -   **用户扩展**: 对于其他 `T` 类型，如果需要评估功能，用户需要：
            1.  **继承 `DataMap<T>`**: 创建一个新的类，例如 `MyDataMap<MyType> : public DataMap<MyType>`。
            2.  **重载 `Evaluate`**: 在派生类中重载 `virtual EvaluatorStatus Evaluate(DataPtr gt_data) override` 方法。
            3.  在重载的 `Evaluate` 方法中：
                -   实现针对 `MyType` 的具体评估逻辑。
                -   创建 `EvaluatorStatus` 对象。
                -   使用 `eval_status.SetEvalType("YourEvalType");` 设置评估类型。
                -   使用 `eval_status.AddResult("metric_name", value);` 添加评估指标和结果。
                -   返回 `eval_status`。
        -   如果未对特定类型 `T` 实现 `Evaluate` 或其派生类未重载该函数，调用时会输出错误提示，告知用户需要实现该功能。
    -   **示例**: 返回一个 `RelativePose` 对象
        ```cpp
        // In Method's Run() function
        RelativePose estimated_pose = CalculatePose(); // 假设得到一个 RelativePose 对象
        return std::make_shared<DataMap<RelativePose>>(estimated_pose);
        ```
<!-- 
-   **`Interface::DataSample<T>`**:
    -   **模板参数 `T`**: **必须** 是 `std::vector<ValueType>` 类型，`ValueType` 是样本元素的类型 (如 `BearingPair`, `IdMatch`)。
    -   **用途**: 专用于鲁棒估计器 (如 RANSAC, GNC-IRLS) 的样本数据管理。支持零拷贝（共享内存）的随机子集和索引子集生成。
    -   **优点**: 高效的子集采样，避免大数据拷贝；提供 STL 兼容接口 (`begin`, `end`, `operator[]`, `size` 等)，方便访问样本数据（包括子集）。
    -   **缺点**: 主要设计用于鲁棒估计流程，通用性不如 `DataMap` 或标准 `DataIO`。
    -   **关键接口**:详细请见 `interfaces_robust_estimator.hpp` 中 `DataSample` 的定义
    ```cpp
        GetPopulationPtr(): 获取总体样本数据指针
        GetPopulationSize(): 获取总体样本数据大小
        GetRandomSubset(size): 获取随机子集
        GetSubset(indices): 获取指定索引子集
        GetInlierSubset(indices): 获取内点子集
        ...
    ```
-   **`Interface::DataCosts`**: (后续可能考虑弃用，因为用`DataMap<double>`可以替代)
    -   **用途**: 存储由代价评估器 (`MethodRelativeCost` 等) 计算出的残差或代价值列表 (`std::vector<double>`)。
    -   **优点**: 标准化的代价存储容器；提供 `std::vector` 接口，方便访问和处理。
    -   **使用场景**: 通常作为代价评估器 (`CostEvaluator`) 的**输出**，用于 RANSAC 等鲁棒估计器的内点判断。
    -   **接口**: 提供了 `push_back`, `operator[]`, `size`, `empty`, `begin`, `end` 等 `std::vector<double>` 兼容接口。
-->

-   **`Interface::DataPackage`**: 
    -   **用途**: 将多个不同的 `DataPtr` 对象打包成一个单一的 `DataPtr`，主要用于向 `Method::Build()` 或 `Behavior::Build()` 传递多个输入数据。（后续会配合开发并行和异构机制的method基类来支持发送任务驱动的消息响应包 /拆分子包并行加速算法运行的功能）
    -   **优点**: 简化了需要多输入的 `Method` 或 `Behavior` 的接口，只需传递一个 `DataPackagePtr`。
    -   **缺点**: 本身不提供序列化；需要通过类型字符串（键）来存取内部数据。
    -   **关键接口** (详见 `interfaces_preset.hpp`):
        -   **`AddData(const DataPtr& data_ptr)`**: 添加数据，使用数据的 `GetType()` 作为键。
        -   **`AddData(const Package& package)`**: 合并另一个 `DataPackage`。
        -  **`AddData(const DataType& alias_type, const DataPtr& data_ptr)`**: 使用自定义别名作为键添加数据。
        -   **`GetData(const DataType& type)`**: 根据类型字符串（键）获取内部的 `DataPtr`。
        -   **`GetPackage()`**: 获取当前Package的常量引用。
    -   **示例**: 
        ```cpp
        auto data_package = std::make_shared<DataPackage>();
        data_package->AddData(tracks_data); // 使用 "data_tracks" 作为键
        data_package->AddData("initial_poses", global_pose_data); // 使用别名 "initial_poses"
        
        // 在方法内部获取数据
        auto tracks = GetDataPtr<Tracks>(data_package->GetData("data_tracks"));
        auto poses = GetDataPtr<GlobalPoses>(data_package->GetData("initial_poses"));
        ```

## 方法插件 (Method Plugin)

- **`Interface::Method`**:
  - **用途**: 实现独立的、不需要复杂配置或输入检查的简单算法。
  - **优点**: 最大灵活性，无额外框架约束。
  - **缺点**: 需要手动处理所有输入/输出、配置加载和错误检查。
- **`Interface::MethodPreset`**:
  - **用途**: **推荐**用于大多数方法插件开发。实现具有标准化输入/输出、配置管理和可选先验信息的算法。
  - **优点**: 简化开发流程，提供结构化框架；自动 INI 配置加载；自动输入数据检查；支持先验信息传递。
  - **缺点**: 引入少量框架约定（如 `Run()` vs `Build()`, `required_package_`）。
- **`Interface::MethodPresetProfiler`**:
  - **用途**: 需要对方法性能（执行时间、内存占用）进行分析和记录的场景，**并集成了自动化的评估流程**。
  - **优点**: 自动性能数据采集和 CSV 导出；跨平台内存监控；**当 `enable_evaluator=true` 时，自动调用结果数据的 `Evaluate` 方法并将结果提交给 `EvaluatorManager`**。
  - **缺点**: 增加少量性能开销（可通过 `enable_profiling` 配置禁用）。
  - **评估流程 (当 `enable_evaluator=true`)**:
    1.  **配置**: 在方法的 `.ini` 文件中设置 `enable_evaluator = true`。
    2.  **提供真值**: 调用 `method_profiler_ptr->SetGTData(gt_data_ptr)` 将真值数据 (`gt_data_ptr`) 设置给 `MethodPresetProfiler`。真值数据会被存储在内部的 `prior_info_` 映射中，键为 `"gt_data"`。
    3.  **执行 `Build`**: 当调用 `method_profiler_ptr->Build(input_data_ptr)` 时：
        -   `Build` 方法内部会先调用 `Run()` 执行核心算法，得到结果 `result_data_ptr`。
        -   之后，`Build` 会自动调用 `CallEvaluator(result_data_ptr)`。
    4.  **`CallEvaluator` 内部逻辑**:
        -   从 `prior_info_` 中查找键为 `"gt_data"` 的真值数据。
        -   调用 `result_data_ptr->Evaluate(gt_data_ptr)`。这会执行实际的评估计算（例如，`DataMap<RelativePose>::Evaluate` 或用户自定义的评估逻辑）。
        -   `Evaluate` 方法返回一个 `EvaluatorStatus` 对象，其中包含评估类型 (`eval_type`) 和具体的评估结果 (`eval_results`，一个 `metric_name` 到 `value` 的列表)。
    5.  **结果提交**: `CallEvaluator` 内部的 `ProcessEvaluationResults` 函数会处理 `EvaluatorStatus`：
        -   它从 `EvaluatorStatus` 中获取 `eval_type`。
        -   它使用当前方法插件的 `GetType()` 作为 `algorithm_name`。
        -   它使用方法配置中的 `ProfileCommit` (如果存在) 或 "default_commit" 作为 `eval_commit`。
        -   遍历 `eval_status.eval_results`，对每一个指标，调用 `EvaluatorManager::AddEvaluationResult(eval_type, algorithm_name, eval_commit, metric_name, value)` 将结果添加到全局评估器中。
- **`Interface::RobustEstimator<TSample>`**:
  - **用途**: 实现基于 RANSAC 或 GNC-IRLS 等鲁棒估计算法。
  - **优点**: 提供标准化的鲁棒估计流程；将模型估计和代价评估解耦。
  - **缺点**: 仅适用于特定类型的鲁棒估计问题；需要配合 `DataSample<TSample>` 输入。

<!-- 
## 行为插件 (Behavior Plugin)

- **`Interface::Behavior`**:
  - **用途**: 封装一系列方法调用，形成一个完整的功能流或特定应用场景。
  - **优点**: 模块化组织复杂流程；定义清晰的输入（`MaterialType`）和输出（`ProductType`）。
  - **缺点**: 需要手动管理内部方法的创建、配置和数据传递。
- **`Interface::BehaviorPreset`**: (当前仅支持序列化方法调用)
  - **用途**: 可以提供更高级的行为管理，如基于配置自动编排方法、统一处理子方法配置等。 

-->