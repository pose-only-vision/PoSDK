# Common Utility Functions

## Configuration Parameter Access

The following utility functions are member functions of the `MethodPreset` class and can be called directly by its derived classes to safely obtain various types of parameter values from method configuration.

### GetOptionAsIndexT
- Function: Get unsigned integer value (IndexT/uint32_t) from method configuration.
- Calling Method: `this->GetOptionAsIndexT(key, default_value)` or directly call `GetOptionAsIndexT(key, default_value)` in member functions.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: IndexT - Default value returned when key does not exist or conversion fails, defaults to 0.
- Return Value: Parameter value of IndexT type.
- Usage Example:
  ```cpp
  // In MethodPreset derived class member function:
  // IndexT max_iterations = this->GetOptionAsIndexT("max_iterations", 100);
  IndexT max_iterations = GetOptionAsIndexT("max_iterations", 100);
  ```

### GetOptionAsInt
- Function: Get signed integer value (int) from method configuration.
- Calling Method: `this->GetOptionAsInt(key, default_value)` or `GetOptionAsInt(key, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: int - Default value returned when key does not exist or conversion fails, defaults to 0.
- Return Value: Parameter value of int type.
- Usage Example:
  ```cpp
  // int step_size = this->GetOptionAsInt("step_size", -1);
  int step_size = GetOptionAsInt("step_size", -1);
  ```

### GetOptionAsFloat
- Function: Get float value from method configuration.
- Calling Method: `this->GetOptionAsFloat(key, default_value)` or `GetOptionAsFloat(key, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: float - Default value returned when key does not exist or conversion fails, defaults to 0.0f.
- Return Value: Parameter value of float type.
- Usage Example:
  ```cpp
  // float threshold = this->GetOptionAsFloat("threshold", 0.5f);
  float threshold = GetOptionAsFloat("threshold", 0.5f);
  ```

### GetOptionAsDouble
- Function: Get double precision floating point value from method configuration.
- Calling Method: `this->GetOptionAsDouble(key, default_value)` or `GetOptionAsDouble(key, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: double - Default value returned when key does not exist or conversion fails, defaults to 0.0.
- Return Value: Parameter value of double type.
- Usage Example:
  ```cpp
  // double precision_value = this->GetOptionAsDouble("precision", 1e-6);
  double precision_value = GetOptionAsDouble("precision", 1e-6);
  ```

### GetOptionAsBool
- Function: Get bool value from method configuration.
- Calling Method: `this->GetOptionAsBool(key, default_value)` or `GetOptionAsBool(key, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: bool - Default value returned when key does not exist or conversion fails, defaults to false.
- Return Value: Parameter value of bool type.
- Note: "true", "yes", "1", "on" in configuration are recognized as true; "false", "no", "0", "off" are recognized as false.
- Usage Example:
  ```cpp
  // bool enable_feature = this->GetOptionAsBool("enable_feature", false);
  bool enable_feature = GetOptionAsBool("enable_feature", false);
  ```

### GetOptionAsString
- Function: Get string value from method configuration.
- Calling Method: `this->GetOptionAsString(key, default_value)` or `GetOptionAsString(key, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `default_value`: const std::string& - Default value returned when key does not exist, defaults to empty string.
- Return Value: Parameter value of std::string type.
- Usage Example:
  ```cpp
  // std::string mode = this->GetOptionAsString("algorithm_mode", "default");
  std::string mode = GetOptionAsString("algorithm_mode", "default");
  ```

### GetOptionAsPath
- Function: Get path value from method configuration with placeholder support. Supports `{exe_dir}`, `{root_dir}`, and generic `{key_name}` placeholders.
- Calling Method: `this->GetOptionAsPath(key, root_dir, default_value)` or `GetOptionAsPath(key, root_dir, default_value)`.
- Parameters:
  - `key`: const MethodParams& - Parameter key name.
  - `root_dir`: const std::string& - Root directory path for `{root_dir}` placeholder replacement. If empty, gets from `method_options_`. Defaults to empty string.
  - `default_value`: const std::string& - Default value returned when key does not exist, defaults to empty string.
- Return Value: Parameter value of std::string type with placeholders replaced.
- Supported Placeholders:
  - `{root_dir}`: Root directory path. Prioritizes the `root_dir` parameter, otherwise gets from `method_options_`.
  - `{exe_dir}`: Executable file directory.
  - `{key_name}`: Reference other key values in `method_options_` (note: referenced keys need to exist first).
- Usage Example:
  ```cpp
  // Get path with placeholders
  std::string output_path = GetOptionAsPath("output_dir", "/project/root");
  // If config contains: "output_dir = {root_dir}/results/{dataset_name}"
  // And method_options_ contains: "dataset_name = test_data"
  // Result: "/project/root/results/test_data"
  
  // Get path with exe_dir placeholder
  std::string config_path = GetOptionAsPath("config_file");
  // If config contains: "config_file = {exe_dir}/../configs/settings.ini"
  // Result: "/path/to/executable/../configs/settings.ini"
  ```

## Data Type Conversion and Access

### GetDataPtr<T> Function
- Function: Safely get and convert data pointer to specified type. This function can intelligently handle both regular `DataPtr` and `DataPackage` that encapsulates multiple data items.
- Template Parameters:
  - `T`: Target data structure type to convert to (e.g., `GlobalPoses`, `Tracks`).
- Parameters:
  - `data_ptr_or_package_ptr`: const Interface::DataPtr& - Smart pointer pointing to a single `DataIO` object or `DataPackage` object.
  - `data_name_in_package` (optional): const std::string& - If the first parameter is a `DataPackage`, this parameter specifies the name of the target data item in the package. Defaults to empty string.
- Return Value: `std::shared_ptr<T>`, pointing to data of type T. Returns `nullptr` if conversion fails or data item is not found.
- How It Works:
  1. Check if input `data_ptr_or_package_ptr` is `nullptr`.
  2. Determine if input points to `DataPackage` or regular `DataIO` object:
     - If `DataPackage`:
       - Requires `data_name_in_package` must be provided.
       - Extract target `DataIO` object from `DataPackage` by name.
     - If regular `DataIO` object:
       - `data_name_in_package` parameter (if provided) will be ignored.
       - Directly use this `DataIO` object.
  3. Call `GetData()` from target `DataIO` object to get raw `void*` pointer.
  4. `static_cast` `void*` pointer to `T*`.
  5. Use `std::shared_ptr`'s aliasing constructor to create a new `std::shared_ptr<T>` that points to internal `T` type data but shares ownership with the original `DataIO` object (or `DataIO` object in `DataPackage`). This ensures that as long as the returned `std::shared_ptr<T>` exists, the underlying data will not be released.
- Usage Example:
  ```cpp
  // Example 1: Get GlobalPoses from regular DataPtr
  Interface::DataPtr single_pose_data_ptr = some_plugin->Run(); // Assume returns single pose data
  auto poses = GetDataPtr<GlobalPoses>(single_pose_data_ptr);
  if (poses) {
      // Use poses
  }

  // Example 2: Get Tracks data from DataPackage
  Interface::DataPtr package_ptr = another_plugin->Run(); // Assume returns a DataPackage
  auto tracks = GetDataPtr<Tracks>(package_ptr, "output_tracks"); // "output_tracks" is the name defined in package
  if (tracks) {
      // Use tracks
  }
  ```

## String Conversion Utilities

### String2Float
- Function: Convert string to float type
- Parameters:
  - value: const std::string& - Input string
  - default_value: float - Default value returned when conversion fails, defaults to 0.0f
- Return Value: Converted float value
- Usage Scenario: Used for safe conversion of configuration file string values to floating point numbers
- Usage Example:
  ```cpp
  float threshold = String2Float("0.75", 0.5f); // Returns 0.75
  float invalid = String2Float("xyz", 0.5f);    // Returns 0.5
  ```

### String2Bool
- Function: Convert string to bool type
- Parameters:
  - str: const std::string& - Input string
  - default_value: bool - Default value returned when conversion fails, defaults to false
- Return Value: Converted bool value
- Note: "true", "yes", "1", "on" are converted to true; "false", "no", "0", "off" are converted to false
- Usage Example:
  ```cpp
  bool enabled = String2Bool("yes", false);    // Returns true
  bool disabled = String2Bool("off", true);    // Returns false
  bool invalid = String2Bool("maybe", false);  // Returns default value false
  ```

### Deprecated Functions

- **String2IndexT (Deprecated)**: This function has been removed from `types.hpp`, related functionality is replaced by `MethodPreset::GetOptionAsIndexT()`.

## Factory Advanced Functions

The following functions are available in all three factory classes (`FactoryBehavior`, `FactoryMethod`, `FactoryData`) unless otherwise noted. They are defined in `<po_core/factory.hpp>`.

### ManagePlugins
- Function: Manually manage plugin directory, supports dynamic re-specification of plugin loading paths.
- Signature: `static void ManagePlugins(const std::string &plugin_path, bool append = false)`
- Parameters:
  - `plugin_path`: const std::string& - New plugin directory path.
  - `append`: bool - When `true`, appends to existing plugin map without clearing loaded plugins; when `false`, clears existing map and reloads. Defaults to `false`.
- Return Value: None.
- Applicable Classes: `FactoryBehavior`, `FactoryMethod`, `FactoryData`.
- Usage Example:
  ```cpp
  // Clear and reload plugins from specified directory
  FactoryMethod::ManagePlugins("/path/to/new/plugins");

  // Append additional plugin directory (preserving existing plugins)
  FactoryMethod::ManagePlugins("/path/to/extra/plugins", true);

  // Also applicable to data and behavior factories
  FactoryData::ManagePlugins("/path/to/data/plugins");
  FactoryBehavior::ManagePlugins("/path/to/behavior/plugins", true);
  ```

### RemovePlugins
- Function: Remove loaded plugins from specified path.
- Signature: `static int RemovePlugins(const std::string &plugin_path)`
- Parameters:
  - `plugin_path`: const std::string& - Plugin directory path to remove.
- Return Value: `int` - Number of plugins successfully removed.
- Applicable Classes: `FactoryBehavior`, `FactoryMethod`, `FactoryData`.
- Usage Example:
  ```cpp
  int removed_count = FactoryMethod::RemovePlugins("/path/to/old/plugins");
  // removed_count is the actual number of plugins removed
  ```

### GetRegisteredPaths
- Function: Get list of all currently registered plugin paths.
- Signature: `static std::vector<std::string> GetRegisteredPaths()`
- Parameters: None.
- Return Value: `std::vector<std::string>` - List of registered plugin directory paths.
- Applicable Classes: `FactoryBehavior`, `FactoryMethod`, `FactoryData`.
- Usage Example:
  ```cpp
  auto paths = FactoryMethod::GetRegisteredPaths();
  for (const auto &path : paths) {
      std::cout << "Registered plugin path: " << path << std::endl;
  }
  ```

### FactoryMethod::Create\<TSample\> (Template-Based Creation Interface)
- Function: Create method instances based on template parameters, for methods that require specifying sample data types (e.g., `RANSACEstimator`, `GNCIRLSEstimator`, etc.).
- Signature: `template <typename TSample> static Interface::MethodPtr Create(const std::string &type)`
- Template Parameters:
  - `TSample`: Sample data type (e.g., `std::vector<Eigen::Vector2d>`).
- Parameters:
  - `type`: const std::string& - Method type name.
- Return Value: `Interface::MethodPtr` - Created method pointer, returns `nullptr` on failure.
- Applicable Classes: `FactoryMethod` only.
- Note: Using this interface requires including the `factory_template_impl.hpp` header file.
- Usage Example:
  ```cpp
  #include <po_core/factory_template_impl.hpp>

  // Create a RANSAC estimator with template parameters
  auto ransac = FactoryMethod::Create<std::vector<Eigen::Vector2d>>("ransac_estimator");
  if (ransac) {
      ransac->SetRequiredData(input_data);
      auto result = ransac->Build();
  }
  ```

## Global Utility Functions

The following are global utility functions under the `PoSDK` namespace.

### DisplayAllPluginVersions
- Function: Display version information for all registered plugins.
- Header: `<po_core/factory.hpp>`
- Signature: `void PoSDK::DisplayAllPluginVersions()`
- Parameters: None.
- Return Value: None.
- Usage Example:
  ```cpp
  #include <po_core/factory.hpp>

  int main() {
      // Load plugins
      FactoryMethod factory("/path/to/plugins");

      // Display all plugin version information
      PoSDK::DisplayAllPluginVersions();
      return 0;
  }
  ```

### DisplayCopyrightSummary
- Function: Display all collected copyright information lists and dependency graph. Suitable for calling at the end of program execution to summarize copyright information for all components used during runtime.
- Header: `<po_core/method_utils.hpp>`
- Signature: `void PoSDK::Interface::DisplayCopyrightSummary()`
- Parameters: None.
- Return Value: None.
- Usage Example:
  ```cpp
  #include <po_core/method_utils.hpp>

  int main() {
      // ... Execute your algorithms ...

      // Display copyright summary before program ends
      PoSDK::Interface::DisplayCopyrightSummary();
      return 0;
  }
  ```

### CreateAndConfigureSubMethod
- Function: Create and configure a sub-method, automatically inheriting parameter configurations and Profiler labels from the parent method. This is a generic utility function that simplifies the process of creating and configuring sub-methods within composite methods.
- Header: `<po_core/method_utils.hpp>`
- Signature: `Interface::MethodPresetProfilerPtr PoSDK::Interface::CreateAndConfigureSubMethod(const std::string &method_type, Interface::MethodPresetProfiler *parent_method = nullptr)`
- Parameters:
  - `method_type`: const std::string& - Method type name to create.
  - `parent_method`: Interface::MethodPresetProfiler* - Parent method instance pointer for parameter and Profiler label inheritance. Defaults to `nullptr` (no inheritance).
- Return Value: `Interface::MethodPresetProfilerPtr` - Created and configured method instance. Returns `nullptr` if creation fails or type conversion fails.
- Workflow:
  1. Creates a method instance of the specified type via `FactoryMethod::Create()`.
  2. Converts the instance to `MethodPresetProfiler` type.
  3. If a parent method is provided, automatically inherits its parameter configurations (via `PassingMethodOptions`) and Profiler labels.
- Usage Example:
  ```cpp
  #include <po_core/method_utils.hpp>

  // Create sub-method in a parent method's Build()
  auto sub_method = PoSDK::Interface::CreateAndConfigureSubMethod("method_LiGT", this);
  if (sub_method) {
      sub_method->SetRequiredData(tracks_data);
      auto result = sub_method->Build();
  }

  // Standalone creation without parent parameter inheritance
  auto standalone = PoSDK::Interface::CreateAndConfigureSubMethod("method_rotation_averaging");
  ```
