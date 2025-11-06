# add_posdk_plugin 使用说明

## 函数签名

```cmake
add_posdk_plugin(PLUGIN_NAME
    [PLUGIN_TYPE <type>]
    [SOURCES <file1.cpp> <file2.cpp> ...]
    [HEADERS <file1.hpp> <file2.hpp> ...]
    [LINK_LIBRARIES <lib1> <lib2> ...]
    [INCLUDE_DIRS <dir1> <dir2> ...]
    [COMPILE_DEFINITIONS <def1> <def2> ...]
)
```

## 参数说明

### 必需参数
- `PLUGIN_NAME`: 插件名称

### 可选参数
- `PLUGIN_TYPE`: 插件类型（默认：`methods`）
- `SOURCES`: 源文件列表（默认：自动发现当前目录下所有`.cpp`文件）
- `HEADERS`: 头文件列表（默认：自动发现当前目录下所有`.hpp`文件）
- `LINK_LIBRARIES`: 额外链接库
- `INCLUDE_DIRS`: 额外包含目录
- `COMPILE_DEFINITIONS`: 编译定义

## 自动发现机制

✨ **新特性**：如果不指定`SOURCES`和`HEADERS`，函数会自动扫描`CMAKE_CURRENT_SOURCE_DIR`：
- 自动发现所有`.cpp`文件作为源文件
- 自动发现所有`.hpp`文件作为头文件
- **仅扫描当前目录**，不包括子目录

## 使用示例

### 示例1：最简单的插件（推荐）

假设插件目录结构：
```
my_plugin/
├── CMakeLists.txt
├── my_plugin.cpp
└── my_plugin.hpp
```

CMakeLists.txt：
```cmake
add_posdk_plugin(my_plugin)
```

✅ 自动发现`my_plugin.cpp`和`my_plugin.hpp`

### 示例2：带额外依赖的插件

```cmake
add_posdk_plugin(my_plugin
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        PoSDK::pomvg_converter
    COMPILE_DEFINITIONS
        USE_CUSTOM_FEATURE=1
)
```

### 示例3：多文件插件（自动发现）

假设插件目录结构：
```
complex_plugin/
├── CMakeLists.txt
├── main.cpp
├── helper.cpp
├── main.hpp
└── helper.hpp
```

CMakeLists.txt：
```cmake
add_posdk_plugin(complex_plugin)
```

✅ 自动发现所有`.cpp`和`.hpp`文件

### 示例4：显式指定文件（精确控制）

如果需要排除某些文件或指定特定文件顺序：
```cmake
add_posdk_plugin(complex_plugin
    SOURCES main.cpp helper.cpp
    HEADERS main.hpp helper.hpp
    LINK_LIBRARIES ${OpenCV_LIBS}
)
```

### 示例5：完整配置示例

```cmake
add_posdk_plugin(colmap_pipeline
    PLUGIN_TYPE methods
    SOURCES colmap_pipeline.cpp
    HEADERS colmap_pipeline.hpp
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        PoSDK::pomvg_converter
    INCLUDE_DIRS
        ${COLMAP_INCLUDE_DIRS}
    COMPILE_DEFINITIONS
        USE_COLMAP_PIPELINE=1
        $<$<CONFIG:Debug>:_DEBUG>
)
```

## 自动配置功能

函数会自动处理以下内容：

✅ **包含目录**：
- 自动包含`CMAKE_CURRENT_SOURCE_DIR`
- 自动包含`OUTPUT_INCLUDE_DIR`
- 自动包含`OpenCV_INCLUDE_DIRS`
- 自动包含`EIGEN3_INCLUDE_DIRS`

✅ **链接库**：
- 自动链接`PoSDK::pomvg_common`
- 自动链接`PoSDK::pomvg_converter`
- 自动链接`PoSDK::po_core`
- 自动链接`Eigen3::Eigen`

✅ **安全特性**：
- 自动启用强制符号绑定（防止假实现）
- Linux: `--no-undefined`
- macOS: `-undefined,error`

✅ **文件处理**：
- 自动处理配置文件（`.ini`）
- 自动处理Python文件（`.py`）
- 自动安装插件库和头文件

## 最佳实践

1. **常规插件**：使用自动发现，无需指定`SOURCES`和`HEADERS`
   ```cmake
   add_posdk_plugin(my_plugin)
   ```

2. **需要额外依赖**：只添加必要的参数
   ```cmake
   add_posdk_plugin(my_plugin
       LINK_LIBRARIES ${EXTRA_LIBS}
   )
   ```

3. **需要精确控制**：显式指定所有文件
   ```cmake
   add_posdk_plugin(my_plugin
       SOURCES file1.cpp file2.cpp
       HEADERS file1.hpp file2.hpp
   )
   ```

## 注意事项

⚠️ **文件扫描限制**：
- 仅扫描`CMAKE_CURRENT_SOURCE_DIR`（当前目录）
- 不递归扫描子目录
- 如果插件源文件在子目录中，必须显式指定

⚠️ **文件命名**：
- 建议插件目录名、插件名、主源文件名保持一致
- 例如：`my_plugin/my_plugin.cpp`

⚠️ **多插件目录**：
- 一个目录只建议配置一个插件
- 如需多个插件，建议分别创建子目录

## 迁移指南

### 旧代码（需要修改）
```cmake
add_posdk_plugin(my_plugin
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}  # ❌ 不再需要
    SOURCES my_plugin.cpp                 # ✅ 可选
    HEADERS my_plugin.hpp                 # ✅ 可选
)
```

### 新代码（推荐）
```cmake
add_posdk_plugin(my_plugin)  # ✨ 更简洁
```

或保留显式配置：
```cmake
add_posdk_plugin(my_plugin
    SOURCES my_plugin.cpp
    HEADERS my_plugin.hpp
)
```

