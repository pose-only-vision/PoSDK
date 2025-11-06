# Data Mapping and Exception Handling

## Data Access and Type Safety (`GetDataPtr`)

- **Core**: Use the `Interface::GetDataPtr<T>(data_ptr)` template function for safe data type conversion.
- **Advantages**: Avoids type risks from direct `static_cast` usage; provides unified data access interface.
- **Example**:
  ```cpp
  // In method plugin's Run()
  auto tracks_data = required_package_["data_tracks"];
  auto tracks = Interface::GetDataPtr<Tracks>(tracks_data);
  if (!tracks) {// Can also omit, as GetDataPtr automatically checks
      std::cerr << "Error: Input data_tracks type is incorrect or data is empty!" << std::endl;
      return nullptr;
  }
  // Now can safely use tracks pointer
  for (const auto& track_info : *tracks) {
      // ...
  }
  ```

## Input Data Provision Methods (`MethodPreset` Derived Classes)

For method plugins inheriting from `MethodPreset` or its derived classes (such as `MethodPresetProfiler`, `RobustEstimator`), there are several ways to provide required input data:

1.  **Pass via `Build` Method**: 
    -   When calling `method_ptr->Build(data_ptr)`, input data `data_ptr` can be passed directly.
    -   **Prerequisite**: Must pre-declare the required input data types (`data_type`) for the method in the plugin constructor via `required_package_["data_type"] = nullptr;`.
    -   If multiple different types of data need to be passed, they should be packaged in a `DataPackage`, then pass the `DataPackage` pointer to `Build`.
    ```cpp
    // Declaration in plugin constructor
    required_package_["data_tracks"] = nullptr;
    required_package_["data_camera_model"] = nullptr;
    
    // When calling
    auto data_package = std::make_shared<DataPackage>();
    data_package->AddData(tracks_data_ptr);
    data_package->AddData(camera_model_data_ptr);
    auto result = method_ptr->Build(data_package);
    ```

2.  **Set via `SetRequiredData` Method**: 
    -   Before calling `Build`, can separately call `method_ptr->SetRequiredData(data_ptr)` to set input data.
    -   **Prerequisite**: Also need to declare required data types in the plugin constructor.
    -   If multiple data need to be set, need to call `SetRequiredData` multiple times.
    ```cpp
    // Declaration in plugin constructor
    required_package_["data_matches"] = nullptr;
    
    // When calling
    method_ptr->SetRequiredData(matches_data_ptr);
    auto result = method_ptr->Build(); // Build can be called without parameters
    ```

3.  **Load via Configuration File Path**: 
    -   In the method's `.ini` configuration file, can use a **key with the same name** as the data type declared in `required_package_` to specify a file path.
    
    ```ini
    # In my_method.ini
    [my_method]
    data_tracks = /path/to/my_tracks.pb 
    ...
    ```
    -   When `MethodPreset`'s `Build` method checks inputs, if it finds a `DataPtr` in `required_package_` still `nullptr`, it will try to find the same-named key in `method_options_` and call the corresponding `DataIO` subclass's `Load()` method to load data from that path.
    -   **Note**: This requires the corresponding data plugin to correctly implement the `Load()` method.
    ```cpp
    // Declaration in plugin constructor
    required_package_["data_tracks"] = nullptr;
    
    // When calling (assuming configuration file is loaded)
    auto result = method_ptr->Build(); // Will automatically try to load data_tracks specified in config file
    ```

4.  **Pass via `SetPriorInfo` Method (for Prior Information)**:
    -   **Purpose**: Pass additional "guidance" information to method plugins, which are not core input data but may affect algorithm behavior (e.g., RANSAC initial model, optimization initial pose estimate, weighted samples, etc.).
    -   **Function Signature**: `void SetPriorInfo(const DataPtr& data_ptr, const std::string& type = "");`
        -   `data_ptr`: `DataPtr` containing prior information.
        -   `type` (optional): A string identifier to distinguish different types of prior information. If left empty, behavior differs (see below).
    -   **Two Modes**:
        1.  **`type` is empty**: In this case, `data_ptr` **must** be a `DataPackagePtr`, supporting setting multiple types of prior information at once.
        2.  **`type` is not empty**: In this case, `data_ptr` can be **any** `DataPtr`. `SetPriorInfo` will add or update this `data_ptr` to the plugin's internal `prior_info_` member using the `type` string as the key. This is suitable for setting single or specific types of prior information.
    -   **Usage**: Call this method before calling `Build()` to set prior information.
    -   **Internal Plugin Access**: In the plugin's `Run()` method or other member functions, access the set prior data through the protected `prior_info_` member variable (`std::unordered_map<std::string, DataPtr>`), and use `GetDataPtr<T>()` for type conversion.
    ```cpp
    // Example: Set initial pose as prior information
    auto initial_pose_data = std::make_shared<DataMap<RelativePose>>(initial_pose);
    method_ptr->SetPriorInfo(initial_pose_data, "initial_guess");
    
    // Example: Set multiple prior information at once
    auto prior_package = std::make_shared<DataPackage>();
    prior_package->AddData("weights", weights_data);
    prior_package->AddData("mask", mask_data);
    method_ptr->SetPriorInfo(prior_package); // type is empty
    
    auto result = method_ptr->Build(main_input_data);
    ```
    -   **Reset**: Call `ResetPriorInfo()` to clear all set prior information.

## Exception Handling

- **Strategy**: PoSDK core library tends to use return values (`nullptr` or `false`) to indicate errors + cerr to display error messages, rather than throwing exceptions `throw()` that terminate the program, to avoid exception handling complexity across plugin boundaries.
- **Future**: Consider assigning log and error levels and IDs to Methods for easier user debugging
- **Plugin Development Recommendations**:
  - Inside plugins, can use C++ standard exception handling (`try-catch`) to catch and handle internal errors.
  - In overridden interface methods (such as `Run`, `Build`, `Save`, `Load`), should catch all internal exceptions and convert them to `nullptr` or `false` return values.
  - Use `std::cerr` to output detailed error information to help users locate issues.
  - Check if pointers returned from factory functions (`FactoryData::Create`, `FactoryMethod::Create`) or `GetDataPtr` are `nullptr`.
