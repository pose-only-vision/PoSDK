# Using Precompiled po_core_lib

This chapter describes how to import and use precompiled PoMVG po_core_lib in your project.

## Core Library Structure

```{note}
Precompiled po_core_lib contains the following directory structure (based on po_core project build output):
```

```text
po_core_lib/
+-- include/                  # Header files directory
|   +-- po_core.hpp          # Main entry header file
|   -- po_core/             # Core library header files
|       +-- version.hpp                    # Version information
|       +-- interfaces.hpp                 # Core interface definitions
|       +-- interfaces_preset.hpp          # Preset method interfaces (MethodPreset, etc.)
|       +-- interfaces_preset_profiler.hpp # Preset method interfaces with profiling
|       +-- interfaces_robust_estimator.hpp # Robust estimator interfaces
|       +-- types.hpp                      # Core type definitions unified entry
|       +-- factory.hpp                    # Factory class interfaces
|       +-- factory_types_fwd.hpp          # Factory system forward declarations and type definitions
|       +-- data_package.hpp               # Data package management (multi-data composition)
|       +-- pb_dataio.hpp                  # Protocol Buffer data IO
|       +-- file_io.hpp                    # File IO utilities (BAL/SfM data read/write)
|       +-- g2o_io.hpp                     # G2O format file read/write
|       +-- evaluator.hpp                  # Evaluation system
|       +-- method_utils.hpp               # Method utility functions
|       +-- relative_residuals.hpp         # Relative pose residual computation
|       +-- inifile.hpp                    # INI configuration file parser
|       +-- posdk_register.hpp             # Plugin registration and dynamic loading
|       +-- executable_path.hpp            # Cross-platform executable path utility
|       +-- output_directory_manager.hpp   # Global output directory management
|       +-- po_logger.hpp                  # Logging system unified entry
|       +-- high_performance_logger.hpp    # High-performance synchronous logging core
|       +-- log_stream.hpp                 # Stream-style log builder
|       +-- bilingual_log_macros.hpp       # Bilingual logging macros
|       +-- language_environment.hpp       # Language environment management
|       +-- ProfilerInfo.hpp               # Profiler information structures
|       +-- ProfilerManager.hpp            # Profiler manager
|       +-- robust/                        # Robust estimation data module
|       |   +-- data_sample.hpp            # Sampling strategies and data sampling
|       |   -- data_cost.hpp              # Cost value data class
|       +-- types/                         # Modular type files
|       |   +-- base.hpp                   # Base type definitions
|       |   +-- camera_model.hpp           # Camera model types
|       |   +-- features.hpp               # Feature types unified entry
|       |   +-- global_poses.hpp           # Global pose types
|       |   +-- image.hpp                  # Image data types
|       |   +-- matches.hpp                # Feature matching types
|       |   +-- relative_poses.hpp         # Relative pose types
|       |   +-- similarity_transform.hpp   # Similarity transform types
|       |   +-- tracks.hpp                 # Feature track types
|       |   +-- world_3dpoints.hpp         # 3D point cloud types
|       |   -- features/                  # Feature submodule
|       |       +-- descriptors.hpp        # Feature descriptors
|       |       +-- feature_base.hpp       # Feature base class
|       |       +-- features_info.hpp      # Feature information
|       |       -- image_feature_info.hpp # Image feature information
-- lib/                      # Library files directory
    +-- libpo_core.so/.dylib/.dll          # PoSDK core library
    +-- libpomvg_proto.so/.dylib/.dll      # Core dependency library
    +-- libpomvg_file_io.so/.dylib/.dll    # Core dependency library
    +-- libpomvg_factory.so/.dylib/.dll    # Core dependency library
    -- cmake/               # CMake configuration files
        -- po_core/         # po_core CMake configuration
            +-- po_core-config.cmake
            +-- po_core-config-version.cmake
            -- po_core-targets.cmake
```

## Core Components Description

### Library Files (lib/)

po_core_lib provides the following core library files:

```{note}
**libpo_core** is PoMVG's main library, aggregating all core functionality. Other libraries (libpomvg_proto, libpomvg_file_io, libpomvg_factory) are core dependency libraries, automatically linked by libpo_core. Users only need to link `PoSDK::po_core` and `PoSDK::pomvg_proto` to use full functionality.
```

### Header Files (include/)

The header file directory contains the following core modules:

- **po_core.hpp**: Main entry header file containing all necessary sub-headers
- **po_core/**: PoMVG core interface header files
  - Interfaces & Methods:
    - `interfaces.hpp`: Core interface definitions (DataIO, Method, Behavior, etc.)
    - `interfaces_preset.hpp`: Preset method interfaces (MethodPreset, etc., providing config file, ground truth data, and other extended interfaces)
    - `interfaces_preset_profiler.hpp`: Preset method interfaces with profiling support (MethodPresetProfiler)
    - `interfaces_robust_estimator.hpp`: Robust estimator interfaces (RANSAC and other robust estimation frameworks)
    - `method_utils.hpp`: Method utility functions (CreateAndConfigureSubMethod, etc.)
  - Types & Data:
    - `types.hpp`: Core data types unified entry
    - `data_package.hpp`: Data package management (multi-data composition with type-safe access)
    - `relative_residuals.hpp`: Relative pose residual computation (coplanar residuals, Sampson residuals, etc.)
    - `types/`: Modular type definition subdirectory (base, camera_model, features, matches, poses, etc.)
    - `robust/`: Robust estimation data module
      - `data_sample.hpp`: Sampling strategies and data sampling (uniform random, spherical uniform, stratified, adaptive)
      - `data_cost.hpp`: Cost value data class (storing model evaluation cost values)
  - Factory & Registration:
    - `factory.hpp`: Factory class interfaces (FactoryData, FactoryMethod, FactoryBehavior)
    - `factory_types_fwd.hpp`: Factory system forward declarations and type definitions
    - `posdk_register.hpp`: Plugin registration and dynamic library loading
  - File IO:
    - `pb_dataio.hpp`: Protocol Buffer data IO base class
    - `file_io.hpp`: File IO utilities (BAL/SfM data format read/write, data type conversion)
    - `g2o_io.hpp`: G2O format file read/write (pose graphs, feature tracks, etc.)
    - `inifile.hpp`: INI configuration file parser (based on Boost.PropertyTree)
  - Evaluation:
    - `evaluator.hpp`: Evaluation system interface
  - Logging:
    - `po_logger.hpp`: Logging system unified entry (single header containing all logging functionality)
    - `high_performance_logger.hpp`: High-performance synchronous logging core (thread-safe, console and file output)
    - `log_stream.hpp`: Stream-style log builder (supports method chaining)
    - `bilingual_log_macros.hpp`: Bilingual logging macros (auto-switches between Chinese/English based on language environment)
    - `language_environment.hpp`: Global language environment management (Chinese/English switching)
  - Profiling:
    - `ProfilerInfo.hpp`: Profiler information structures (timing, memory, and other statistics)
    - `ProfilerManager.hpp`: High-performance global profiler manager (lock-free design, supports high concurrency)
  - Utilities:
    - `executable_path.hpp`: Cross-platform executable file path utility
    - `output_directory_manager.hpp`: Global output directory manager (centralized workflow output path management)
    - `version.hpp`: Version information



## Integration in CMake Projects

To use precompiled po_core_lib in a CMake project, add relevant configuration to CMakeLists.txt.

```{note}
**About Directory Structure**:
If you build the PoMVG development framework from source, the build generates an `output` directory with complete functionality. Precompiled `po_core_lib` is a streamlined version of `output`, containing only core libraries and header files.

For detailed PoMVG development framework post-build directory structure description, please refer to [Build Output Directory Structure](installation.md#posdk-build-output-structure).
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
PoMVG's Method interface provides multiple execution methods:
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

