# 插件构建容错机制
# Plugin Build Fault Tolerance Mechanism

## 概述 | Overview

PoSDK的插件系统现在支持容错构建，这意味着：
- 如果某个插件在CMake配置阶段出错，会跳过该插件并继续配置其他插件
- 如果某个插件在Make编译阶段出错，可以选择继续构建其他插件
- 构建结束后会生成详细的报告，显示哪些插件成功、哪些失败

The PoSDK plugin system now supports fault-tolerant building, which means:
- If a plugin fails during CMake configuration, it will be skipped and other plugins will continue
- If a plugin fails during Make compilation, you can choose to continue building other plugins
- A detailed report is generated at the end showing which plugins succeeded and which failed

## 使用方法 | Usage

### 方法1：使用make -k（推荐）
### Method 1: Using make -k (Recommended)

`-k`选项（keep-going）让make在遇到错误时继续构建其他目标：

The `-k` option (keep-going) tells make to continue building other targets when errors occur:

```bash
cd /path/to/PoSDK/build

# Configure with CMake (will show initial plugin discovery report)
cmake ..

# Build with keep-going flag (continue on errors)
make -k -j$(nproc)

# Or specify the number of parallel jobs
make -k -j8
```

**优点 | Advantages:**
- ✓ 简单易用，只需添加`-k`标志
- ✓ 自动跳过失败的插件
- ✓ 最大化构建成功的插件数量
- ✓ 适合CI/CD环境

**缺点 | Disadvantages:**
- ⚠ 如果一个插件是其他插件的依赖，依赖它的插件也会失败
- ⚠ 构建输出可能包含很多错误信息

### 方法2：将插件标记为OPTIONAL
### Method 2: Mark Plugins as OPTIONAL

在插件的`CMakeLists.txt`中使用`OPTIONAL`选项：

Use the `OPTIONAL` option in the plugin's `CMakeLists.txt`:

```cmake
# Example: plugins/methods/MyPlugin/CMakeLists.txt

add_posdk_plugin(my_plugin
    PLUGIN_TYPE methods
    OPTIONAL  # ← 添加此选项 | Add this option
    SOURCES
        my_plugin.cpp
    HEADERS
        my_plugin.hpp
    LINK_LIBRARIES
        ${OpenCV_LIBS}
)
```

**何时使用 | When to Use:**
- 实验性插件（Experimental plugins）
- 需要特殊依赖的插件（Plugins requiring special dependencies）
- 平台特定的插件（Platform-specific plugins）

### 方法3：完全自动化（推荐用于CI/CD）
### Method 3: Fully Automated (Recommended for CI/CD)

创建一个构建脚本：

Create a build script:

```bash
#!/bin/bash
# build_plugins_with_tolerance.sh

set +e  # Don't exit on error

BUILD_DIR="build"
LOG_FILE="plugin_build.log"

echo "Building PoSDK with fault-tolerant plugins..."

# Configure
cmake -B ${BUILD_DIR} -S . 2>&1 | tee ${LOG_FILE}

# Build with keep-going
make -C ${BUILD_DIR} -k -j$(nproc) 2>&1 | tee -a ${LOG_FILE}

# Extract build result
if grep -q "All plugins built successfully" ${LOG_FILE}; then
    echo "✓ All plugins built successfully!"
    exit 0
elif grep -q "Build completed with warnings" ${LOG_FILE}; then
    echo "⚠ Build completed but some plugins failed"
    echo "Check ${LOG_FILE} for details"
    exit 0  # Still return success if some plugins built
else
    echo "✗ Build failed completely"
    exit 1
fi
```

## 构建报告说明 | Build Report Explanation

构建结束后，会自动生成一个报告：

A report is automatically generated at the end of the build:

```
╔════════════════════════════════════════════════════════════════╗
║          Plugin Build Report | 插件构建报告                    ║
╚════════════════════════════════════════════════════════════════╝

Total plugins discovered: 15 | 发现的插件总数: 15
  ✓ Success: 12 | 成功: 12
  ⊘ Skipped: 2 | 跳过: 2
  ✗ Failed:  1 | 失败: 1

✓ Successfully Built Plugins | 成功构建的插件:
    ✓ method_img2features
    ✓ method_img2matches
    ✓ opencv_two_view_estimator
    ... (9 more)

⊘ Skipped Plugins | 跳过的插件:
    ⊘ experimental_plugin
      Reason | 原因: CMakeLists.txt not readable
    ⊘ windows_only_plugin
      Reason | 原因: Platform not supported

✗ Failed Plugins | 构建失败的插件:
    ✗ broken_plugin
      Reason | 原因: Missing dependency libXYZ

⚠ Note: Some plugins failed to build, but the build will continue.
⚠ 注意：某些插件构建失败，但构建将继续进行。

╔════════════════════════════════════════════════════════════════╗
║  ⚠ Build completed with warnings | 构建完成但有警告         ║
╚════════════════════════════════════════════════════════════════╝
```

### 报告解读 | Report Interpretation

**✓ Success (成功)**
- 插件已成功配置并编译
- 可以正常使用

**⊘ Skipped (跳过)**
- 插件在配置阶段被跳过
- 常见原因：
  - CMakeLists.txt不存在或不可读
  - 平台不支持
  - 依赖项未满足（在配置阶段检测到）

**✗ Failed (失败)**
- 插件配置成功但编译失败
- 常见原因：
  - 编译错误（语法错误、类型错误等）
  - 链接错误（缺少库文件）
  - 依赖项问题（在编译阶段才发现）

## CMake阶段 vs Make阶段的容错
## Fault Tolerance: CMake Phase vs Make Phase

### CMake配置阶段 | CMake Configuration Phase

**自动处理的情况 | Automatically Handled:**
- CMakeLists.txt不存在
- CMakeLists.txt不可读
- 基本的语法错误

**示例 | Example:**
```cmake
# plugins/methods/BadPlugin/CMakeLists.txt (有语法错误)
add_posdk_plugin(bad_plugin
    PLUGIN_TYPE methods
    SOURCES
        # 缺少源文件
    # 缺少右括号 - 语法错误
```

**结果 | Result:**
```
⊘ Plugin BadPlugin skipped: CMakeLists.txt syntax error
Other plugins continue to configure normally
```

### Make编译阶段 | Make Compilation Phase

**需要使用 `-k` 标志 | Requires `-k` Flag:**

```bash
make -k -j8  # Keep going even if some plugins fail
```

**示例 | Example:**
```cpp
// BadPlugin.cpp (编译错误)
#include "non_existent_header.hpp"  // 文件不存在

void BadFunction() {
    undeclared_function();  // 未声明的函数
}
```

**结果（使用 -k）| Result (with -k):**
```
[ 10%] Building CXX object plugins/methods/BadPlugin/...
BadPlugin.cpp:1:10: fatal error: non_existent_header.hpp: No such file or directory
[ 20%] Building CXX object plugins/methods/GoodPlugin/...
[ 30%] Linking CXX shared library GoodPlugin.so
✓ GoodPlugin built successfully
✗ BadPlugin failed to compile
```

**结果（不使用 -k）| Result (without -k):**
```
[ 10%] Building CXX object plugins/methods/BadPlugin/...
BadPlugin.cpp:1:10: fatal error: non_existent_header.hpp: No such file or directory
make[2]: *** [BadPlugin] Error 1
make[1]: *** [plugins/methods/BadPlugin] Error 2
make: *** [all] Error 2
Build stopped!  ← 构建停止，其他插件不会被构建
```

## 最佳实践 | Best Practices

### 1. 开发阶段 | Development Phase

**不使用 `-k`（推荐）**
- 立即发现并修复错误
- 避免忽略重要问题

```bash
cmake ..
make -j8  # No -k flag
```

### 2. 持续集成 | Continuous Integration

**使用 `-k`（推荐）**
- 构建尽可能多的插件
- 生成完整的测试报告

```yaml
# .github/workflows/build.yml
- name: Build with fault tolerance
  run: |
    cmake -B build
    make -C build -k -j$(nproc)
    
- name: Check build report
  run: |
    # Parse build report to determine success
    if grep -q "All plugins built successfully" build_log.txt; then
      exit 0
    elif [ $(grep -o "Success: [0-9]*" build_log.txt | cut -d' ' -f2) -gt 10 ]; then
      echo "Acceptable: More than 10 plugins succeeded"
      exit 0
    else
      echo "Too many failures"
      exit 1
    fi
```

### 3. 发布构建 | Release Build

**严格模式（不使用 `-k`）**
- 确保所有插件都正常工作
- 不允许有失败的插件

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8  # No -k - fail if any plugin fails
```

## 调试失败的插件 | Debugging Failed Plugins

### 1. 查看详细错误信息 | View Detailed Errors

```bash
# Rebuild only the failed plugin with verbose output
make VERBOSE=1 failed_plugin_name

# Or use CMake's verbose flag
cmake --build build --target failed_plugin_name -- VERBOSE=1
```

### 2. 单独构建失败的插件 | Build Failed Plugin Separately

```bash
# Navigate to plugin directory
cd build/plugins/methods/FailedPlugin

# Rebuild with verbose output
make VERBOSE=1
```

### 3. 检查依赖项 | Check Dependencies

```bash
# Check if required libraries are available
ldd build/plugins/methods/FailedPlugin/posdk_plugin_failed.so

# On macOS:
otool -L build/plugins/methods/FailedPlugin/posdk_plugin_failed.dylib
```

## 常见问题 | Common Issues

### Q1: 报告显示插件成功，但实际上没有.so文件
### Q1: Report shows plugin success, but no .so file exists

**原因 | Cause:** 
插件在CMake配置阶段成功，但在Make编译阶段失败

The plugin succeeded in CMake configuration but failed during Make compilation

**解决方案 | Solution:**
```bash
# Check if plugin was actually compiled
ls -la build/plugins/methods/*/posdk_plugin_*.so

# Rebuild with verbose output
make VERBOSE=1 -k
```

### Q2: 所有插件都被标记为"跳过"
### Q2: All plugins marked as "skipped"

**原因 | Cause:**
可能是CMakeLists.txt路径问题

Likely a CMakeLists.txt path issue

**解决方案 | Solution:**
```bash
# Check plugin directory structure
find plugins -name CMakeLists.txt

# Verify auto-discovery is working
cmake -B build -S . | grep "Discovered plugin"
```

### Q3: 如何强制重新构建失败的插件？
### Q3: How to force rebuild of failed plugins?

```bash
# Clean only failed plugin
rm -rf build/plugins/methods/FailedPlugin

# Or clean all plugins
make clean

# Rebuild
make -k -j8
```

## 性能考虑 | Performance Considerations

### 并行构建 | Parallel Building

```bash
# Use all CPU cores
make -k -j$(nproc)

# Limit to 8 cores (avoid system overload)
make -k -j8

# Sequential (slowest, but easier to read errors)
make -k -j1
```

### 增量构建 | Incremental Building

```bash
# Only rebuild changed plugins
make -k

# Force rebuild all
make clean && make -k -j8
```

## 集成到现有工作流 | Integration with Existing Workflow

### 修改 PoSDK 构建脚本 | Modify PoSDK Build Script

如果你使用 `install.sh` 或类似脚本，可以修改为：

If you use `install.sh` or similar, modify it to:

```bash
# In install.sh or similar build script

echo "Building PoSDK with fault-tolerant plugins..."

# Configure
cmake -B build

# Build with keep-going flag
if make -C build -k -j$(nproc); then
    echo "✓ Build completed successfully"
else
    echo "⚠ Build completed with some plugin failures"
    echo "Check the report above for details"
    # Don't exit with error - some plugins succeeded
fi

# Installation can proceed with successfully built plugins
make -C build install
```

## 总结 | Summary

插件容错构建机制提供了：

The plugin fault-tolerance build mechanism provides:

1. **灵活性 | Flexibility**
   - 可选地跳过有问题的插件
   - 继续构建其他插件

2. **可见性 | Visibility**
   - 详细的构建报告
   - 清晰的成功/失败状态

3. **可靠性 | Reliability**
   - 即使某些插件失败，核心系统仍可构建
   - 适合CI/CD环境

4. **可维护性 | Maintainability**
   - 容易识别问题插件
   - 便于逐个修复

**推荐配置 | Recommended Configuration:**
- 开发环境：不使用`-k`，立即修复错误
- CI环境：使用`-k`，最大化构建覆盖
- 发布版本：不使用`-k`，确保所有插件正常

---

*Last Updated: 2025-10-29*
*Version: 1.0*

