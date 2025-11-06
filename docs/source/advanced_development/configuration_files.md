# Method Configuration Files (INI)

- **Format**: Standard INI file format.
  ```ini
  [section_name] ; Corresponds to the GetType() return value of the method or behavior
  key1 = value1
  key2 = value2 # Comments supported
  # Empty lines and comment lines are ignored
  @inherit = /path/to/base_config.ini ; Optional: inheritance directive
  ```
- **Loading Mechanism (`MethodPreset::InitializeDefaultConfigPath`, `LoadMethodOptions`)**:
  1. **Priority**: Runtime settings > `method_options_` in code > current directory `configs/methods/` > build directory `configs/methods/` > installation directory `configs/methods/`.
  2. **Auto-loading**: `MethodPreset` constructor attempts to load `.ini` files with the same name as `GetType()` in the above priority order.
  3. **Inheritance**: Supports `@inherit` directive to load base configuration, current configuration overrides inherited configuration.
  4. **Specific Configuration Loading**: `LoadMethodOptions` can accept a second parameter to load a specific section from the INI file as configuration.
- **Reading Configuration (`GetOption*` in `types.hpp`)**:
  - In plugin code, use the `method_options_` member variable to access configuration.
  - Use helper functions `GetOptionAsString`, `GetOptionAsIndexT`, `GetOptionAsFloat`, `GetOptionAsBool` to safely read configuration values with default values.
- **Writing Configuration (`ConfigurationTools`)**:
  - Although `MethodPreset` is mainly for reading, the `ConfigurationTools` class (`inifile.hpp`) provides functionality to write INI files (`WriteItem`, `WriteFile`). Can be used to save user-modified configurations. 
