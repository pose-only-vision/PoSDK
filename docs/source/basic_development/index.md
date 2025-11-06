# Basic Plugin Development

This section introduces the fundamentals of PoSDK plugin development, including core interfaces, plugin registration, derivation choices, configuration methods, and detailed descriptions of built-in data and method plugins.

```{note}
**PoSDK currently mainly supports the following plugin development**:
- **DataIO Plugins**: For encapsulating various data types
- **Method Plugins**: For implementing specific algorithm logic

**Behavior Plugins** are temporarily not open for user development.
```

## Quick Start

```{tip}
**Recommended reading order for beginners**:
1. First read [Plugin Development Quick Example](method_example.md) to understand the basic plugin development workflow
2. Then check [Data Plugin Interface Details](data_plugins.md) and [Method Plugin Interface Details](method_plugins.md) to gain deeper understanding of interface design
3. Finally refer to [Built-in Type Reference](#built-in-type-reference) to learn about available data types and methods
```

## Plugin Registration Macro

All plugins need to use the `REGISTRATION_PLUGIN` macro for registration at the end of the corresponding `.cpp` source file.

```{important}
** Automatic Implementation of `GetType()` Function**

The `REGISTRATION_PLUGIN` macro **automatically implements** the `GetType()` function:
- Only **declare** `const std::string& GetType() const override;` in the header file
- **Do not** manually implement `GetType()` in the source file
- The macro automatically generates the implementation
```

### **Three Registration Methods**

| Usage                    | Syntax                                 | Plugin Name Source | Applicable Scenarios                   | Recommendation         |
| ------------------------ | -------------------------------------- | ------------------ | -------------------------------------- | ---------------------- |
| **Single-param + CMake** | `REGISTRATION_PLUGIN(MyClass)`         | CMake auto-defined | Any namespace                          | **Highly Recommended** |
| Single-param (legacy)    | `REGISTRATION_PLUGIN(MyClass)`         | Uses class name    | ⚠️ Only `PoSDKPlugin` namespace         | ⚠️ Not Recommended      |
| Dual-param               | `REGISTRATION_PLUGIN(MyClass, "type")` | Manually specified | When need to override CMake definition | ⚠️ Backward Compatible  |

```{tip}
**Recommended Method 1 (Single Source of Truth):**

Define plugin name in **CMakeLists.txt**:
```cmake
add_posdk_plugin(my_method  # ← Plugin name defined only once here
    PLUGIN_TYPE methods
    SOURCES my_method.cpp
)
```

Use single-parameter mode in **C++ code** (auto-read):
```cpp
REGISTRATION_PLUGIN(MyNamespace::MyMethod)  // ← Automatically uses CMake-defined name
```

**Advantages**:
- **Single Source of Truth**: Plugin name only needs to be defined once in CMake
- **Automatic Consistency**: File name, registration name, and CMake target automatically consistent
- **Error Prevention**: No manual synchronization needed, reduces maintenance cost
```

```{note} 
**Important Notes**:
- `REGISTRATION_PLUGIN` **must** be placed in **source file (.cpp)**, **cannot** be placed in header file (.hpp)
- Ensure registered type string is unique within current plugin directory
- Recommended to use **single-parameter + CMake auto-definition** mode for single source of truth management
```

### **Example 1 - Single-parameter + CMake Auto-definition (Recommended)**

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(method_img2matches
    PLUGIN_TYPE methods
    SOURCES img2matches_pipeline.cpp
)
```

**C++ Header File (img2matches_pipeline.hpp)**:
```cpp
namespace PluginMethods {
    class Img2MatchesPipeline : public Method {
    public:
        // Only declare GetType, no implementation needed
        const std::string& GetType() const override;
        DataPtr Build(const DataPtr& material_ptr = nullptr) override;
    };
}
```

**C++ Source File (img2matches_pipeline.cpp)**:
```cpp
#include "img2matches_pipeline.hpp"

// ... (Other function implementations for Img2MatchesPipeline class) ...

// Single-parameter mode - automatically reads from CMake PLUGIN_NAME
// Plugin type: "method_img2matches" (from CMake)
// File name: posdk_plugin_method_img2matches.{so|dylib|dll}
REGISTRATION_PLUGIN(PluginMethods::Img2MatchesPipeline)
```

**Automatic Consistency**:
- CMake Target: `method_img2matches`
- Plugin file name: `posdk_plugin_method_img2matches.dylib`
- Registration type: `"method_img2matches"` (auto-read)

---

### **Example 2 - Dual-parameter Mode (Backward Compatible)**

```cpp
// Manually specify plugin type (will override CMake definition)
REGISTRATION_PLUGIN(MyNamespace::MyMethod, "custom_method_name")
```

```{warning}
Dual-parameter mode will **override** CMake auto-defined `PLUGIN_NAME`, which may cause:
- File name and registration name inconsistency (downgrade registration)
- Need to maintain plugin name in two places
- Increases maintenance cost and error risk

**Use only when must override CMake definition!**
```

## Plugin Directory Management

PoSDK uses factory pattern to create and manage plugins. The system loads dynamic libraries (.so/.dll/.dylib) as plugins from specific directories. Each plugin type has a corresponding default directory:

```{important}
Default plugin directory structure (relative to executable or library):
- Data plugins: `../plugins/data/`
- Method plugins: `../plugins/methods/`
```

## Plugin Loading Mechanism

PoSDK uses lazy loading mechanism, plugins are only loaded when `Create` method is called for the first time. Loading process is as follows:

1. Check if plugin of corresponding type has been loaded
2. If not loaded, attempt to load all matching dynamic libraries from default plugin directory
3. When user requests object creation, prioritize creation from built-in factory; if no matching type, attempt to create from plugin

```{tip}
If you encounter plugin loading issues, please check:
1. Whether plugin directory exists and has read permissions
2. Whether plugin files are compatible with current system (Linux .so, Windows .dll, macOS .dylib)
3. Whether plugin correctly registered the type
4. Whether all plugin dependency libraries are correctly loaded
```

### Manually Specify Plugin Directory

You can manually specify plugin directory through factory class static method `ManagePlugins`:

```cpp
PoMVG::FactoryData::ManagePlugins("/path/to/your/data/plugins");
PoMVG::FactoryMethod::ManagePlugins("/path/to/your/method/plugins");
PoMVG::FactoryBehavior::ManagePlugins("/path/to/your/behavior/plugins");
```

### Get Available Plugin Types

Use the following functions to display available plugin types in command line:

```cpp
// Only display types provided by plugins
PoMVG::FactoryData::DispPluginTypes();
PoMVG::FactoryMethod::DispPluginTypes();
PoMVG::FactoryBehavior::DispPluginTypes();
```

## Plugin Development Guide

This section provides complete plugin development tutorials and interface specifications.

```{toctree}
:maxdepth: 1
:caption: Plugin Development

method_example
data_plugins
method_plugins
prior_and_gt
``` 

(built-in-type-reference)=
## Built-in Type Reference

This section provides a list of preset data and method plugins in PoSDK core library along with brief descriptions.

```{toctree}
:maxdepth: 1
:caption: Built-in Types

data_types
plugin_list
```


