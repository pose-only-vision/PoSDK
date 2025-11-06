# 方法配置文件 (INI)

- **格式**: 标准 INI 文件格式。
  ```ini
  [section_name] ; 对应方法或行为的 GetType() 返回值
  key1 = value1
  key2 = value2 # 支持注释
  # 空行和注释行会被忽略
  @inherit = /path/to/base_config.ini ; 可选：继承指令
  ```
- **加载机制 (`MethodPreset::InitializeDefaultConfigPath`, `LoadMethodOptions`)**:
  1. **优先级**: 运行时设置 > 代码中 `method_options_` > 当前目录 `configs/methods/` > 构建目录 `configs/methods/` > 安装目录 `configs/methods/`。
  2. **自动加载**: `MethodPreset` 构造函数会尝试按上述优先级加载与 `GetType()` 同名的 `.ini` 文件。
  3. **继承**: 支持 `@inherit` 指令加载基础配置，当前配置会覆盖继承的配置。
  4. **特定配置加载**: `LoadMethodOptions` 可以传入第二个参数，加载 INI 文件中特定的节作为配置。
- **读取配置 (`GetOption*` in `types.hpp`)**:
  - 在插件代码中，使用 `method_options_` 成员变量访问配置。
  - 使用 `GetOptionAsString`, `GetOptionAsIndexT`, `GetOptionAsFloat`, `GetOptionAsBool` 辅助函数安全地读取配置值，并提供默认值。
- **写入配置 (`ConfigurationTools`)**:
  - 虽然 `MethodPreset` 主要用于读取，但 `ConfigurationTools` 类 (`inifile.hpp`) 提供了写入 INI 文件的功能 (`WriteItem`, `WriteFile`)。可以用于保存用户修改后的配置。 
 