# 模块基础开发

本部分介绍 PoSDK 插件开发的基础知识，包括核心接口、插件注册、派生选择、配置方法，并详细介绍内置的数据和方法插件。

```{note}
**当前PoSDK主要支持以下插件开发**：
- **DataIO插件**：用于封装各种数据类型
- **Method插件**：用于实现具体的算法逻辑

**Behavior插件** 暂时不对用户开放开发。
```

## 快速开始

```{tip}
**新手推荐阅读顺序**：
1. 首先阅读 [插件开发快速示例](method_example.md)，了解插件开发的基本流程
2. 然后查看 [数据插件接口详解](data_plugins.md) 和 [方法插件接口详解](method_plugins.md)，深入理解接口设计
3. 最后参考 [内置类型列表](#内置类型参考)，了解可用的数据类型和方法
```

## 插件注册宏

所有插件都需要使用 `REGISTRATION_PLUGIN` 宏在对应的 `.cpp` 源文件末尾进行注册。

```{important}
** 自动实现 `GetType()` 函数**

`REGISTRATION_PLUGIN` 宏会**自动实现** `GetType()` 函数：
- 头文件中只需**声明** `const std::string& GetType() const override;`
- **不需要**在源文件中手动实现 `GetType()`
- 宏会自动生成实现
```

### **三种注册方式**

| 用法             | 语法                                   | 插件名来源    | 适用场景                  | 推荐度       |
| ---------------- | -------------------------------------- | ------------- | ------------------------- | ------------ |
| **单参数+CMake** | `REGISTRATION_PLUGIN(MyClass)`         | CMake自动定义 | 任意命名空间              | **强烈推荐** |
| 单参数（旧版）   | `REGISTRATION_PLUGIN(MyClass)`         | 使用类名      | ⚠️ 仅`PoSDKPlugin`命名空间 | ⚠️ 不推荐     |
| 双参数           | `REGISTRATION_PLUGIN(MyClass, "type")` | 手动指定      | 需覆盖CMake定义时         | ⚠️ 向后兼容   |

```{tip}
**推荐方式1（单一信息源）：**

**CMakeLists.txt** 中定义插件名称：
```cmake
add_posdk_plugin(my_method  # ← 插件名称只在这里定义一次
    PLUGIN_TYPE methods
    SOURCES my_method.cpp
)
```

**C++ 代码**中使用单参数模式（自动读取）：
```cpp
REGISTRATION_PLUGIN(MyNamespace::MyMethod)  // ← 自动使用CMake定义的名称
```

**优势**：
- **单一信息源**：插件名称只需在CMake中定义一次
- **自动一致**：文件名、注册名、CMake target自动保持一致
- **避免错误**：无需手动同步，降低维护成本
```

```{note} 
**重要说明**：
- `REGISTRATION_PLUGIN` **必须** 放置在 **源文件 (.cpp)** 中，**不能** 放置在头文件 (.hpp) 中
- 确保注册的类型字符串在当前插件目录中是唯一的
- 推荐使用**单参数+CMake自动定义**模式，实现单一信息源管理
```

### **示例1 - 单参数+CMake自动定义（推荐）**

**CMakeLists.txt**:
```cmake
# 插件名称在这里定义（唯一定义处）
add_posdk_plugin(method_img2matches
    PLUGIN_TYPE methods
    SOURCES img2matches_pipeline.cpp
)
```

**C++ 头文件 (img2matches_pipeline.hpp)**:
```cpp
namespace PluginMethods {
    class Img2MatchesPipeline : public Method {
    public:
        //  只需声明GetType，不需要实现
        const std::string& GetType() const override;
        DataPtr Build(const DataPtr& material_ptr = nullptr) override;
    };
}
```

**C++ 源文件 (img2matches_pipeline.cpp)**:
```cpp
#include "img2matches_pipeline.hpp"

// ... (Img2MatchesPipeline 类的其他函数实现) ...

// 单参数模式 - 自动从CMake读取PLUGIN_NAME
// 插件类型："method_img2matches"（来自CMake）
// 文件名：posdk_plugin_method_img2matches.{so|dylib|dll}
REGISTRATION_PLUGIN(PluginMethods::Img2MatchesPipeline)
```

**自动一致性**：
- CMake Target: `method_img2matches`
- 插件文件名: `posdk_plugin_method_img2matches.dylib`
- 注册类型: `"method_img2matches"`（自动读取）

---

### **示例2 - 双参数模式（向后兼容）**

```cpp
// 手动指定插件类型（会覆盖CMake定义）
REGISTRATION_PLUGIN(MyNamespace::MyMethod, "custom_method_name")
```

```{warning}
双参数模式会**覆盖**CMake自动定义的`PLUGIN_NAME`，可能导致：
- 文件名与注册名不一致（降级注册）
- 需要在两处维护插件名称
- 增加维护成本和出错风险

**仅在必须覆盖CMake定义时使用！**
```

## 插件目录管理

PoSDK 使用工厂模式来创建和管理插件。系统会从特定目录加载动态库（.so/.dll/.dylib）作为插件。每种插件类型都有对应的默认目录：

```{important}
默认的插件目录结构（相对于可执行文件或库）：
- 数据插件: `../plugins/data/`
- 方法插件: `../plugins/methods/`
```

## 插件加载机制

PoSDK 使用懒加载机制，只有在第一次调用 `Create` 方法时才会加载插件。加载过程如下：

1. 检查对应类型的插件是否已加载
2. 如果未加载，尝试从默认插件目录加载所有匹配的动态库
3. 用户请求创建对象时，优先从内置工厂创建，如果没有匹配类型，则尝试从插件创建

```{tip}
如果您遇到插件加载问题，请检查:
1. 插件目录是否存在且有读取权限
2. 插件文件是否与当前系统兼容（Linux .so, Windows .dll, macOS .dylib）
3. 插件是否正确注册了类型
4. 插件的依赖库是否都已正确加载
```

### 手动指定插件目录

您可以通过工厂类的静态方法 `ManagePlugins` 来手动指定插件目录：

```cpp
PoMVG::FactoryData::ManagePlugins("/path/to/your/data/plugins");
PoMVG::FactoryMethod::ManagePlugins("/path/to/your/method/plugins");
PoMVG::FactoryBehavior::ManagePlugins("/path/to/your/behavior/plugins");
```

### 获取可用插件类型

使用以下函数在命令行显示可用的插件类型：

```cpp
// 仅显示插件提供的类型
PoMVG::FactoryData::DispPluginTypes();
PoMVG::FactoryMethod::DispPluginTypes();
PoMVG::FactoryBehavior::DispPluginTypes();
```

## 插件开发指南

本节提供完整的插件开发教程和接口规范。

```{toctree}
:maxdepth: 1
:caption: 插件开发

method_example
data_plugins
method_plugins
prior_and_gt
``` 

## 内置类型参考

本节提供了PoSDK核心库中预置的数据和方法插件列表及其简要说明。

```{toctree}
:maxdepth: 1
:caption: 内置类型

data_types
plugin_list
```


