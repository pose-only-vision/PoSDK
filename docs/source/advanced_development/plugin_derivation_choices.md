# Detailed Plugin Derivation Choices

## Data Plugins (Data Plugin)

-   **`Interface::DataIO`**:
    -   **Purpose**: Implement the most basic data container, or use when data does not need serialization/deserialization.
    -   **Advantages**: Simple and direct.
    -   **Disadvantages**: Need to manually implement `Save`/`Load`.
-   **`Interface::PbDataIO`**:
    -   **Purpose**: Scenarios requiring data serialization/deserialization via Protobuf (saving to files or network transmission).
    -   **Advantages**: Automatic file I/O handling; provides macros to simplify Protobuf field mapping.
    -   **Disadvantages**: Introduces Protobuf dependency; requires defining `.proto` files and configuring CMake code generation.
    -   **Key Implementation**: Users need to override `CreateProtoMessage`, `ToProto`, `FromProto`. Recommended to use the following helper macros (see `interfaces_preset.hpp` for details) to simplify implementation:
    ```text
        -   `PROTO_SET_BASIC` / `PROTO_GET_BASIC`: Basic types (int, float, bool, string).
        -   `PROTO_SET_VECTOR2F/D` / `PROTO_GET_VECTOR2F/D`: Eigen::Vector2f/d.
        -   `PROTO_SET_VECTOR3D` / `PROTO_GET_VECTOR3D`: Eigen::Vector3d.
        -   `PROTO_SET_MATRIX3D` / `PROTO_GET_MATRIX3D`: Eigen::Matrix3d.
        -   `PROTO_SET_ARRAY` / `PROTO_GET_ARRAY`: `std::vector<basic_type>`.
        -   `PROTO_SET_ENUM` / `PROTO_GET_ENUM`: Enum types.
    ```

-   **`Interface::DataMap<T>`**:
    -   **Template Parameter `T`**: Any default-constructible type.
    -   **Purpose**: Quickly wrap a **single** data object (such as `RelativePose`, `GlobalPoses`, `std::vector`, `std::map`, custom structs, etc.) as `DataPtr`, convenient for passing between methods or as method return values, avoiding creating complete data plugins for simple data structures.
    -   **Advantages**: Easy integration of existing data into PoSDK framework without declaring and compiling new Data classes.
    -   **Disadvantages**: Does not directly support serialization (`PbDataIO`); if persistence is needed, requires containing plugins (such as `DataPackage`) or external logic to handle.
    -   **Access**: Use `GetDataPtr<T>(data_map_ptr)` to get pointer to internal data.
    -   **Deep Copy (`CopyData`)**:
        -   `DataMap<T>` provides `CopyData()` method for creating deep copies of data.
        -   Deep copy effect depends on the copy constructor implementation of template parameter `T`. If type `T` implements deep copy, then `DataMap<T>::CopyData()` also performs deep copy; if type `T` only has shallow copy, then `CopyData()` result is also shallow copy.

    ```{note}
    For custom types containing pointer members or requiring deep copy, ensure the type correctly implements deep copy constructor.
    ```
    -   **Evaluation (`Evaluate`)**: 
        -   `DataMap<T>` provides virtual function implementation of `Evaluate(DataPtr gt_data)`.
        -   **Specialized Handling**: Currently specialized for `T = RelativePose`, can directly calculate rotation and translation errors, and add results (`rotation_error`, `translation_error`) to `EvaluatorStatus`, `eval_type` will be set to `"RelativePose"`.
        -   **User Extension**: For other `T` types, if evaluation functionality is needed, users need to:
            1.  **Inherit `DataMap<T>`**: Create a new class, e.g., `MyDataMap<MyType> : public DataMap<MyType>`.
            2.  **Override `Evaluate`**: Override `virtual EvaluatorStatus Evaluate(DataPtr gt_data) override` method in derived class.
            3.  In the overridden `Evaluate` method:
                -   Implement specific evaluation logic for `MyType`.
                -   Create `EvaluatorStatus` object.
                -   Use `eval_status.SetEvalType("YourEvalType");` to set evaluation type.
                -   Use `eval_status.AddResult("metric_name", value);` to add evaluation metrics and results.
                -   Return `eval_status`.
        -   If `Evaluate` is not implemented for specific type `T` or its derived class does not override the function, calling it will output error message informing users that this functionality needs to be implemented.
    -   **Example**: Return a `RelativePose` object
        ```cpp
        // In Method's Run() function
        RelativePose estimated_pose = CalculatePose(); // Assume we get a RelativePose object
        return std::make_shared<DataMap<RelativePose>>(estimated_pose);
        ```
<!-- 
-   **`Interface::DataSample<T>`**:
    -   **Template Parameter `T`**: **Must** be `std::vector<ValueType>` type, `ValueType` is the type of sample element (e.g., `BearingPair`, `IdMatch`).
    -   **Purpose**: Dedicated sample data management for robust estimators (e.g., RANSAC, GNC-IRLS). Supports zero-copy (shared memory) random subset and indexed subset generation.
    -   **Advantages**: Efficient subset sampling, avoiding large data copying; provides STL-compatible interface (`begin`, `end`, `operator[]`, `size`, etc.), convenient for accessing sample data (including subsets).
    -   **Disadvantages**: Mainly designed for robust estimation flow, less general than `DataMap` or standard `DataIO`.
    -   **Key Interfaces**: See `interfaces_robust_estimator.hpp` for `DataSample` definition details
    ```cpp
        GetPopulationPtr(): Get population sample data pointer
        GetPopulationSize(): Get population sample data size
        GetRandomSubset(size): Get random subset
        GetSubset(indices): Get specified index subset
        GetInlierSubset(indices): Get inlier subset
        ...
    ```
-   **`Interface::DataCosts`**: (May be deprecated later, as `DataMap<double>` can replace it)
    -   **Purpose**: Store residual or cost value lists (`std::vector<double>`) calculated by cost evaluators (`MethodRelativeCost`, etc.).
    -   **Advantages**: Standardized cost storage container; provides `std::vector` interface, convenient for access and processing.
    -   **Usage Scenarios**: Typically used as **output** of cost evaluators (`CostEvaluator`), for inlier determination in robust estimators like RANSAC.
    -   **Interface**: Provides `push_back`, `operator[]`, `size`, `empty`, `begin`, `end` and other `std::vector<double>` compatible interfaces.
-->

-   **`Interface::DataPackage`**: 
    -   **Purpose**: Package multiple different `DataPtr` objects into a single `DataPtr`, mainly used for passing multiple input data to `Method::Build()` or `Behavior::Build()`. (Future: will support task-driven message response packages / splitting sub-packages for parallel algorithm acceleration with parallel and heterogeneous mechanism method base classes)
    -   **Advantages**: Simplifies interfaces for `Method` or `Behavior` requiring multiple inputs, only need to pass one `DataPackagePtr`.
    -   **Disadvantages**: Does not provide serialization itself; requires accessing internal data through type strings (keys).
    -   **Key Interfaces** (see `interfaces_preset.hpp` for details):
        -   **`AddData(const DataPtr& data_ptr)`**: Add data, using data's `GetType()` as key.
        -   **`AddData(const Package& package)`**: Merge another `DataPackage`.
        -  **`AddData(const DataType& alias_type, const DataPtr& data_ptr)`**: Add data using custom alias as key.
        -   **`GetData(const DataType& type)`**: Get internal `DataPtr` according to type string (key).
        -   **`GetPackage()`**: Get constant reference to current Package.
    -   **Example**: 
        ```cpp
        auto data_package = std::make_shared<DataPackage>();
        data_package->AddData(tracks_data); // Use "data_tracks" as key
        data_package->AddData("initial_poses", global_pose_data); // Use alias "initial_poses"
        
        // Get data inside method
        auto tracks = GetDataPtr<Tracks>(data_package->GetData("data_tracks"));
        auto poses = GetDataPtr<GlobalPoses>(data_package->GetData("initial_poses"));
        ```

## Method Plugins (Method Plugin)

- **`Interface::Method`**:
  - **Purpose**: Implement standalone, simple algorithms that do not require complex configuration or input checking.
  - **Advantages**: Maximum flexibility, no additional framework constraints.
  - **Disadvantages**: Need to manually handle all input/output, configuration loading, and error checking.
- **`Interface::MethodPreset`**:
  - **Purpose**: **Recommended** for most method plugin development. Implement algorithms with standardized input/output, configuration management, and optional prior information.
  - **Advantages**: Simplified development process, provides structured framework; automatic INI configuration loading; automatic input data checking; supports prior information passing.
  - **Disadvantages**: Introduces some framework conventions (e.g., `Run()` vs `Build()`, `required_package_`).
- **`Interface::MethodPresetProfiler`**:
  - **Purpose**: Scenarios requiring method performance analysis (execution time, memory usage) and recording, **and integrates automated evaluation flow**.
  - **Advantages**: Automatic performance data collection and CSV export; cross-platform memory monitoring; **when `enable_evaluator=true`, automatically calls result data's `Evaluate` method and submits results to `EvaluatorManager`**.
  - **Disadvantages**: Adds slight performance overhead (can be disabled via `enable_profiling` configuration).
  - **Evaluation Flow (when `enable_evaluator=true`)**:
    1.  **Configuration**: Set `enable_evaluator = true` in method's `.ini` file.
    2.  **Provide Ground Truth**: Call `method_profiler_ptr->SetGTData(gt_data_ptr)` to set ground truth data (`gt_data_ptr`) to `MethodPresetProfiler`. Ground truth data will be stored in internal `prior_info_` map with key `"gt_data"`.
    3.  **Execute `Build`**: When calling `method_profiler_ptr->Build(input_data_ptr)`:
        -   `Build` method internally first calls `Run()` to execute core algorithm, obtaining result `result_data_ptr`.
        -   Then, `Build` automatically calls `CallEvaluator(result_data_ptr)`.
    4.  **`CallEvaluator` Internal Logic**:
        -   Search for ground truth data with key `"gt_data"` from `prior_info_`.
        -   Call `result_data_ptr->Evaluate(gt_data_ptr)`. This executes actual evaluation calculation (e.g., `DataMap<RelativePose>::Evaluate` or user-defined evaluation logic).
        -   `Evaluate` method returns an `EvaluatorStatus` object containing evaluation type (`eval_type`) and specific evaluation results (`eval_results`, a list of `metric_name` to `value`).
    5.  **Result Submission**: `CallEvaluator`'s internal `ProcessEvaluationResults` function processes `EvaluatorStatus`:
        -   It gets `eval_type` from `EvaluatorStatus`.
        -   It uses current method plugin's `GetType()` as `algorithm_name`.
        -   It uses `ProfileCommit` from method configuration (if exists) or "default_commit" as `eval_commit`.
        -   Iterates through `eval_status.eval_results`, for each metric, calls `EvaluatorManager::AddEvaluationResult(eval_type, algorithm_name, eval_commit, metric_name, value)` to add result to global evaluator.
- **`Interface::RobustEstimator<TSample>`**:
  - **Purpose**: Implement robust estimation algorithms based on RANSAC or GNC-IRLS, etc.
  - **Advantages**: Provides standardized robust estimation flow; decouples model estimation and cost evaluation.
  - **Disadvantages**: Only applicable to specific types of robust estimation problems; requires `DataSample<TSample>` input.

<!-- 
## Behavior Plugins (Behavior Plugin)

- **`Interface::Behavior`**:
  - **Purpose**: Encapsulate a series of method calls to form a complete functional flow or specific application scenario.
  - **Advantages**: Modular organization of complex flows; defines clear input (`MaterialType`) and output (`ProductType`).
  - **Disadvantages**: Need to manually manage creation, configuration, and data passing of internal methods.
- **`Interface::BehaviorPreset`**: (Currently only supports serialized method calls)
  - **Purpose**: Can provide higher-level behavior management, such as automatic method orchestration based on configuration, unified handling of sub-method configuration, etc. 

-->
