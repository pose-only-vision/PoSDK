# 使用预编译的 po_core_lib

本章节将介绍如何在项目中导入和使用预编译的PoSDK po_core_lib 库。

## 核心库结构

```{note}
预编译的 po_core_lib 包含以下目录结构（基于 po_core 项目的构建输出）：
```

```text
po_core_lib/
├── include/                  # 头文件目录
│   ├── po_core.hpp          # 主入口头文件
│   └── po_core/             # 核心库头文件
│       ├── version.hpp      # 版本信息
│       ├── interfaces.hpp   # 核心接口定义
│       ├── types.hpp        # 核心类型定义
│       ├── factory.hpp      # 工厂类接口
│       ├── pb_dataio.hpp    # Protocol Buffer数据IO
│       ├── evaluator.hpp    # 评估系统
│       ├── method_utils.hpp # 方法工具函数
│       ├── types/           # 类型模块化文件
│       ├── logger/          # 日志系统头文件
│       └── profiler/        # 性能分析器头文件
└── lib/                      # 库文件目录
    ├── libpo_core.so/.dylib/.dll          # PoSDK核心库
    ├── libpomvg_proto.so/.dylib/.dll      # 核心依赖库
    ├── libpomvg_file_io.so/.dylib/.dll    # 核心依赖库
    ├── libpomvg_factory.so/.dylib/.dll    # 核心依赖库
    └── cmake/               # CMake配置文件
        └── po_core/         # po_core CMake配置
            ├── po_core-config.cmake
            ├── po_core-config-version.cmake
            └── po_core-targets.cmake
```

## 核心组件说明

### 库文件（lib/）

po_core_lib 提供以下核心库文件：

```{note}
**libpo_core** 是 PoSDK 的主库，聚合了所有核心功能。其他库（libpomvg_proto、libpomvg_file_io、libpomvg_factory）是核心依赖库，由 libpo_core 自动链接。用户只需链接 `PoSDK::po_core` 和 `PoSDK::pomvg_proto` 即可使用完整功能。
```

### 头文件（include/）

头文件目录包含以下核心模块：

- **po_core.hpp**: 主入口头文件，包含所有必要的子头文件
- **po_core/**: PoSDK核心接口头文件
  - `interfaces.hpp`: 核心接口定义（DataIO、Method、Behavior等）
  - `types.hpp`: 核心数据类型统一入口
  - `factory.hpp`: 工厂类接口（FactoryData、FactoryMethod、FactoryBehavior）
  - `pb_dataio.hpp`: Protocol Buffer数据IO基类
  - `evaluator.hpp`: 评估系统接口
  - `method_utils.hpp`: 方法工具函数（CreateAndConfigureSubMethod等）
  - `types/`: 模块化类型定义子目录
  - `logger/`: 日志系统头文件
  - `profiler/`: 性能分析器头文件



## 在 CMake 项目中集成 

要在CMake 项目中使用预编译的 po_core_lib，需要在 CMakeLists.txt 中添加相关配置。

```{note}
**关于目录结构**：
如果你从源码构建 PoSDK 开发框架，构建后会生成包含完整功能的 `output` 目录。预编译的 `po_core_lib` 是 `output` 的精简版本，仅包含核心库和头文件。

详细的 PoSDK 开发框架构建后目录结构说明，请参考 [构建后的目录结构](installation.md#posdk-build-output-structure)。
```

### 方式1：集成模式

集成模式会将 po_core_lib 的所有内容复制到项目的输出目录，使应用程序可以独立运行。（见根目录 [`CMakeLists.txt:789`](../../CMakeLists.txt#L789)）

### 方式2：普通模式

非集成模式直接从原始位置使用 po_core_lib，适合开发环境。

```cmake


# 指定 po_core_lib 路径
set(po_core_folder "/path/to/po_core_lib")

# 添加 CMake 配置路径
list(APPEND CMAKE_PREFIX_PATH "${po_core_folder}/lib/cmake")

# 查找 po_core 包
find_package(po_core REQUIRED)

# 链接库（与集成模式相同）
target_link_libraries(my_app PRIVATE
    PoSDK::po_core
    PoSDK::pomvg_proto
)
```

```{warning}
非集成模式下，运行时需要设置动态库搜索路径：
- **Linux**: `export LD_LIBRARY_PATH=/path/to/po_core_lib/lib:$LD_LIBRARY_PATH`
- **macOS**: `export DYLD_LIBRARY_PATH=/path/to/po_core_lib/lib:$DYLD_LIBRARY_PATH`
- **Windows**: 将 `po_core_lib\lib` 添加到 PATH 环境变量
```

## 基本使用示例

### 最小示例

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;

    // 创建图像路径数据
    auto images_data = FactoryData::Create("data_images");
    if (!images_data) {
        std::cerr << "Failed to create data_images" << std::endl;
        return 1;
    }

    // 创建特征提取方法
    auto method = FactoryMethod::Create("method_example");
    if (!method) {
        std::cerr << "Failed to create method_example" << std::endl;
        return 1;
    }

    std::cout << "Successfully created data and method objects!" << std::endl;
    return 0;
}
```

### 完整工作流示例

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;
    using namespace PoSDK::Interface;

    // 1. 创建输入数据
    auto images_data = FactoryData::Create("data_images");

    // 2. 创建方法并配置
    auto method_ptr = FactoryMethod::Create("method_example");
    auto method = std::dynamic_pointer_cast<MethodPreset>(method_ptr);

    if (method) {
        // 设置配置文件路径
        method->SetConfigPath("configs/methods/method_example.ini");

        // 设置真值数据（用于评估）
        auto gt_data = FactoryData::Create("data_ground_truth");
        method->SetGTData(gt_data);
    }

    // 3. 执行方法（使用Build接口）
    auto result_package = method_ptr->Build(images_data);

    return 0;
}
```

```{note}
PoSDK的Method接口提供多种执行方式：
- `Build(DataPtr input)`: 基本执行接口，所有Method都必须实现
- MethodPreset派生类提供额外接口：
  - `SetConfigPath()`: 设置配置文件路径
  - `SetGTData()`: 设置真值数据用于评估

详细使用方法请参考：[Method接口文档](basic_development/method_plugins.md)
```

## 查看可用插件类型

```cpp
#include <po_core.hpp>
#include <iostream>

int main() {
    using namespace PoSDK;

    // 获取可用的插件类型（仅显示插件，不显示内置类型）
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
`DispPluginTypes()` 仅显示来自插件的类型，不包含内置类型。这有助于用户了解当前系统中可用的插件功能，而无需了解系统内部实现细节。
```

## 编译和运行

### 编译项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake ..

# 编译
make -j$(nproc)
```

### 运行应用程序

```bash
# 集成模式下，直接运行
./output/bin/my_app

# 非集成模式下，需要设置库路径（Linux）
export LD_LIBRARY_PATH=/path/to/po_core_lib/lib:$LD_LIBRARY_PATH
./output/bin/my_app
```

## 故障排除

### 常见问题

1. **找不到 po_core 包**
   ```
   CMake Error: Could not find a package configuration file provided by "po_core"
   ```

   **解决方法**：
   - 检查 `po_core_external_folder` 路径是否正确
   - 确认 `${po_core_external_folder}/lib/cmake/po_core/po_core-config.cmake` 文件存在

2. **运行时找不到动态库**
   ```
   error while loading shared libraries: libpo_core.so: cannot open shared object file
   ```

   **解决方法**：
   - **集成模式**：确保构建目录下 `output/lib/` 包含所有库文件
   - **普通模式**：设置正确的动态库搜索路径（LD_LIBRARY_PATH等）
   - 使用 `ldd`(Linux) / `otool -L`(macOS) / `Dependency Walker`(Windows) 检查依赖

3. **符号未定义错误**
   ```
   undefined reference to `PoSDK::FactoryData::Create(std::string const&)'
   ```

   **解决方法**：
   - 确认正确链接了 `PoSDK::po_core` 和 `PoSDK::pomvg_proto`
   - 检查 C++ 标准是否设置为 C++17 或更高

### 调试技巧

1. **检查 CMake 配置**
   ```bash
   cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
   ```

2. **查看库依赖关系**
   ```bash
   # Linux
   ldd output/bin/my_app

   # macOS
   otool -L output/bin/my_app
   ```

3. **启用详细日志**
   ```cpp
   // 在代码中启用详细日志输出
   PoSDK::Logger::SetLogLevel(PoSDK::Logger::LogLevel::DEBUG);
   ```

