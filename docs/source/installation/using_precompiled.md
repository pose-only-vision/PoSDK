# Using Precompiled po_core_lib

This chapter describes how to import and use precompiled PoSDK po_core_lib in your project.

## Core Library Structure

```{note}
Precompiled po_core_lib contains the following directory structure (based on po_core project build output):
```

```text
po_core_lib/
├── include/                  # Header files directory
│   ├── po_core.hpp          # Main entry header file
│   └── po_core/             # Core library header files
│       ├── version.hpp      # Version information
│       ├── interfaces.hpp   # Core interface definitions
│       ├── types.hpp        # Core type definitions
│       ├── factory.hpp      # Factory class interfaces
│       ├── pb_dataio.hpp    # Protocol Buffer data IO
│       ├── evaluator.hpp    # Evaluation system
│       ├── method_utils.hpp # Method utility functions
│       ├── types/           # Modular type files
│       ├── logger/          # Logging system header files
│       └── profiler/        # Profiler header files
└── lib/                      # Library files directory
    ├── libpo_core.so/.dylib/.dll          # PoSDK core library
    ├── libpomvg_proto.so/.dylib/.dll      # Core dependency library
    ├── libpomvg_file_io.so/.dylib/.dll    # Core dependency library
    ├── libpomvg_factory.so/.dylib/.dll    # Core dependency library
    └── cmake/               # CMake configuration files
        └── po_core/         # po_core CMake configuration
            ├── po_core-config.cmake
            ├── po_core-config-version.cmake
            └── po_core-targets.cmake
```

## Core Components Description

### Library Files (lib/)

po_core_lib provides the following core library files:

```{note}
**libpo_core** is PoSDK's main library, aggregating all core functionality. Other libraries (libpomvg_proto, libpomvg_file_io, libpomvg_factory) are core dependency libraries, automatically linked by libpo_core. Users only need to link `PoSDK::po_core` and `PoSDK::pomvg_proto` to use full functionality.
```

### Header Files (include/)

The header file directory contains the following core modules:

- **po_core.hpp**: Main entry header file containing all necessary sub-headers
- **po_core/**: PoSDK core interface header files
  - `interfaces.hpp`: Core interface definitions (DataIO, Method, Behavior, etc.)
  - `types.hpp`: Core data types unified entry
  - `factory.hpp`: Factory class interfaces (FactoryData, FactoryMethod, FactoryBehavior)
  - `pb_dataio.hpp`: Protocol Buffer data IO base class
  - `evaluator.hpp`: Evaluation system interface
  - `method_utils.hpp`: Method utility functions (CreateAndConfigureSubMethod, etc.)
  - `types/`: Modular type definition subdirectory
  - `logger/`: Logging system header files
  - `profiler/`: Profiler header files



## Integration in CMake Projects

To use precompiled po_core_lib in a CMake project, add relevant configuration to CMakeLists.txt.

```{note}
**About Directory Structure**:
If you build the PoSDK development framework from source, the build generates an `output` directory with complete functionality. Precompiled `po_core_lib` is a streamlined version of `output`, containing only core libraries and header files.

For detailed PoSDK development framework post-build directory structure description, please refer to [Build Output Directory Structure](installation.md#posdk-build-output-structure).
```

### Method 1: Integrated Mode

Integrated mode copies all contents of po_core_lib to the project's output directory, allowing the application to run independently. (See root directory [`CMakeLists.txt:789`](../../CMakeLists.txt#L789))

### Method 2: Non-Integrated Mode

Non-integrated mode uses po_core_lib directly from its original location, suitable for development environments.

```cmake


# Specify po_core_lib path
set(po_core_folder "/path/to/po_core_lib")

# Add CMake configuration path
list(APPEND CMAKE_PREFIX_PATH "${po_core_folder}/lib/cmake")

# Find po_core package
find_package(po_core REQUIRED)

# Link libraries (same as integrated mode)
target_link_libraries(my_app PRIVATE
    PoSDK::po_core
    PoSDK::pomvg_proto
)
```

```{warning}
In non-integrated mode, runtime requires setting dynamic library search path:
- **Linux**: `export LD_LIBRARY_PATH=/path/to/po_core_lib/lib:$LD_LIBRARY_PATH`
- **macOS**: `export DYLD_LIBRARY_PATH=/path/to/po_core_lib/lib:$DYLD_LIBRARY_PATH`
- **Windows**: Add `po_core_lib\lib` to PATH environment variable
```

## Basic Usage Examples

### Minimal Example

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;

    // Create image path data
    auto images_data = FactoryData::Create("data_images");
    if (!images_data) {
        std::cerr << "Failed to create data_images" << std::endl;
        return 1;
    }

    // Create feature extraction method
    auto method = FactoryMethod::Create("method_example");
    if (!method) {
        std::cerr << "Failed to create method_example" << std::endl;
        return 1;
    }

    std::cout << "Successfully created data and method objects!" << std::endl;
    return 0;
}
```

### Complete Workflow Example

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;
    using namespace PoSDK::Interface;

    // 1. Create input data
    auto images_data = FactoryData::Create("data_images");

    // 2. Create method and configure
    auto method_ptr = FactoryMethod::Create("method_example");
    auto method = std::dynamic_pointer_cast<MethodPreset>(method_ptr);

    if (method) {
        // Set configuration file path
        method->SetConfigPath("configs/methods/method_example.ini");

        // Set ground truth data (for evaluation)
        auto gt_data = FactoryData::Create("data_ground_truth");
        method->SetGTData(gt_data);
    }

    // 3. Execute method (using Build interface)
    auto result_package = method_ptr->Build(images_data);

    return 0;
}
```

```{note}
PoSDK's Method interface provides multiple execution methods:
- `Build(DataPtr input)`: Basic execution interface, all Methods must implement
- MethodPreset derived classes provide additional interfaces:
  - `SetConfigPath()`: Set configuration file path
  - `SetGTData()`: Set ground truth data for evaluation

For detailed usage, please refer to: [Method Interface Documentation](basic_development/method_plugins.md)
```

## View Available Plugin Types

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;

    // Get available plugin types (only shows plugins, not built-in types)
    std::cout << "Available Data Plugin Types:" << std::endl;
    auto data_types = FactoryData::DispPluginTypes();
    for (const auto& type : data_types) {
        std::cout << "  - " << type << std::endl;
    }

    std::cout << "\nAvailable Method Plugin Types:" << std::endl;
    auto method_types = FactoryMethod::DispPluginTypes();
    for (const auto& type : method_types) {
        std::cout << "  - " << type << std::endl;
    }

    std::cout << "\nAvailable Behavior Plugin Types:" << std::endl;
    auto behavior_types = FactoryBehavior::DispPluginTypes();
    for (const auto& type : behavior_types) {
        std::cout << "  - " << type << std::endl;
    }

    return 0;
}
```

```{note}
`DispPluginTypes()` only displays types from plugins, not built-in types. This helps users understand available plugin functionality in the current system without needing to know internal implementation details.
```

## Compilation and Execution

### Compile Project

```bash
# Create build directory
mkdir build && cd build

# Configure CMake
cmake ..

# Compile
make -j$(nproc)
```

### Run Application

```bash
# In integrated mode, run directly
./output/bin/my_app

# In non-integrated mode, need to set library path (Linux)
export LD_LIBRARY_PATH=/path/to/po_core_lib/lib:$LD_LIBRARY_PATH
./output/bin/my_app
```

## Troubleshooting

### Common Issues

1. **Cannot find po_core package**
   ```
   CMake Error: Could not find a package configuration file provided by "po_core"
   ```

   **Solution**:
   - Check if `po_core_external_folder` path is correct
   - Verify `${po_core_external_folder}/lib/cmake/po_core/po_core-config.cmake` file exists

2. **Runtime cannot find dynamic library**
   ```
   error while loading shared libraries: libpo_core.so: cannot open shared object file
   ```

   **Solution**:
   - **Integrated Mode**: Ensure `output/lib/` under build directory contains all library files
   - **Non-integrated Mode**: Set correct dynamic library search path (LD_LIBRARY_PATH, etc.)
   - Use `ldd`(Linux) / `otool -L`(macOS) / `Dependency Walker`(Windows) to check dependencies

3. **Undefined symbol errors**
   ```
   undefined reference to `PoSDK::FactoryData::Create(std::string const&)'
   ```

   **Solution**:
   - Verify correct linking of `PoSDK::po_core` and `PoSDK::pomvg_proto`
   - Check if C++ standard is set to C++17 or higher

### Debugging Tips

1. **Check CMake Configuration**
   ```bash
   cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
   ```

2. **View Library Dependencies**
   ```bash
   # Linux
   ldd output/bin/my_app

   # macOS
   otool -L output/bin/my_app
   ```

3. **Enable Verbose Logging**
   ```cpp
   // Enable verbose log output in code
   PoSDK::Logger::SetLogLevel(PoSDK::Logger::LogLevel::DEBUG);
   ```

