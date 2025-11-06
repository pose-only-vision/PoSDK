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
