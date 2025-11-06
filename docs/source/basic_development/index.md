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

## Plugin Registration

All plugins need to use the `REGISTRATION_PLUGIN` macro for registration at the end of the corresponding `.cpp` source file.

```{important}
** Automatic Implementation of `GetType()` Function**

The `REGISTRATION_PLUGIN` macro **automatically implements** the `GetType()` function:
- Only **declare** `const std::string& GetType() const override;` in the header file
- **Do not** manually implement `GetType()` in the source file
- The macro automatically generates the implementation
```

### **Plugin Registration Method**

| Usage                    | Syntax                         | Plugin Name Source | Applicable Scenarios | Recommendation         |
| ------------------------ | ------------------------------ | ------------------ | -------------------- | ---------------------- |
| **Single-param + CMake** | `REGISTRATION_PLUGIN(MyClass)` | CMake auto-defined | Any namespace        | **Highly Recommended** |

```{tip}
**Recommended Method (Single Source of Truth):**

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

### **Example - Plugin Registration**

**CMakeLists.txt**:
```cmake
# Plugin name defined here (single definition point)
add_posdk_plugin(opencv_two_view_estimator
    PLUGIN_TYPE methods
    SOURCES opencv_two_view_estimator.cpp
    HEADERS opencv_two_view_estimator.hpp
    LINK_LIBRARIES
        # Note: PoSDK::po_core, PoSDK::pomvg_converter, PoSDK::pomvg_common, and Eigen3::Eigen
        # are automatically linked by add_posdk_plugin function, no need to specify them here.
        # 注意：PoSDK::po_core、PoSDK::pomvg_converter、PoSDK::pomvg_common 和 Eigen3::Eigen
        # 已由 add_posdk_plugin 函数自动链接，无需在此指定。
        ${OpenCV_LIBS}  # Only specify additional libraries not auto-linked
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
)
```

**C++ Header File (opencv_two_view_estimator.hpp)**:
```cpp
namespace PluginMethods {
    class OpenCVTwoViewEstimator : public MethodPresetProfiler {
    public:
        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string& GetType() const override;
        DataPtr Run() override;
        // ... (other methods) ...
    };
}
```

**C++ Source File (opencv_two_view_estimator.cpp)**:
```cpp
#include "opencv_two_view_estimator.hpp"

// ... (Other function implementations for OpenCVTwoViewEstimator class) ...

} // namespace PluginMethods

// ✅ Single-parameter mode - automatically reads from CMake PLUGIN_NAME
// Plugin type: "opencv_two_view_estimator" (from CMake)
// File name: posdk_plugin_opencv_two_view_estimator.{so|dylib|dll}
REGISTRATION_PLUGIN(PluginMethods::OpenCVTwoViewEstimator)
```

**Automatic Consistency**:
- CMake Target: `opencv_two_view_estimator`
- Plugin file name: `posdk_plugin_opencv_two_view_estimator.dylib`
- Registration type: `"opencv_two_view_estimator"` (auto-read from CMake)

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
PoSDK::FactoryData::ManagePlugins("/path/to/your/data/plugins");
PoSDK::FactoryMethod::ManagePlugins("/path/to/your/method/plugins");
PoSDK::FactoryBehavior::ManagePlugins("/path/to/your/behavior/plugins");
```

### Get Available Plugin Types

Use the following functions to display available plugin types in command line:

```cpp
// Only display types provided by plugins
PoSDK::FactoryData::DispPluginTypes();
PoSDK::FactoryMethod::DispPluginTypes();
PoSDK::FactoryBehavior::DispPluginTypes();
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


