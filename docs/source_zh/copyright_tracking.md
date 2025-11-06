(copyright-tracking)=
# 版权跟踪功能 | Copyright Tracking

PoSDK 内置的**自动版权跟踪**，在运行时自动收集、管理和显示所有插件及算法的版权信息，保持知识产权的透明性和可追溯性。

## 工作原理 | How It Works

```{figure} _static/copyright.png
:alt: Copyright Tracking Workflow
:align: center
:width: 90%

版权跟踪功能示意图
```

如上图所示，当Method A依赖多个算法（a/b/c/d/e/f）时，系统会自动追踪调用链，收集版权信息（copyright_1至copyright_5），自动去重（如copyright_2被多个算法使用但只显示一次），并统一汇总展示。

## 功能特性 | Features

### 自动版权收集与显示

当应用程序运行时，系统会自动：

- **收集版权信息**：自动记录被调用的插件或方法的版权信息 

- **即时显示**：方法首次被使用时，显示其版权声明  

- **依赖追踪**：自动识别和记录方法之间的调用关系  

- **去重处理**：相同方法的版权信息只显示一次  

- **汇总报告**：运行结束时生成完整的版权列表和依赖关系图  

### 版权信息展示示例

#### 方法首次加载时的即时显示

```text
============= 插件版权信息 =============
方法类型: SIFT_FeatureExtractor
插件版本: 1.2.3
编译时间: 2025-01-15 10:30:45
版权标识: OpenCV
Copyright (c) 2000-2025, Intel Corporation, all rights reserved.
Copyright (c) 2009-2011, Willow Garage Inc., all rights reserved.
Third-party copyrights are property of their respective owners.
License: Apache License 2.0
=====================================
```

#### 流程结束时的版权汇总

```text
============= 版权信息列表 =============
[ Copyright.1 ]
版权标识: PoSDK
Copyright (c) 2025 PoSDK/PoCore author: Qi Cai.
Based on Pose-only Imaging Geometry Theory.
License: Proprietary
------------------------------

[ Copyright.2 ]
版权标识: OpenCV
Copyright (c) 2000-2025, Intel Corporation, all rights reserved.
Copyright (c) 2009-2011, Willow Garage Inc., all rights reserved.
License: Apache License 2.0
------------------------------

[ Copyright.3 ]
版权标识: Eigen
Copyright (c) Eigen Authors
License: MPL2 (Mozilla Public License 2.0)
------------------------------
==========================================

============= 依赖关系图 =============
[方法: FeatureMatchingPipeline | 版权标识: PoSDK]
     << OpenCV | Eigen

[方法: SIFT_Detector | 版权标识: OpenCV]
     << (无依赖)

[方法: BundleAdjustment | 版权标识: PoSDK]
     << Ceres | Eigen
==========================================
```

## 如何启用版权跟踪

PoSDK 提供了两种方式来启用插件的版权跟踪，取决于你的 Method 从哪个基类派生。

### 方式 1: 从 `MethodPreset` 派生（推荐）

**这是最简单的方式**。如果你的 Method 继承自 `MethodPreset` 或其派生类（如 `MethodPresetProfiler`），版权跟踪会**自动启用**。

你只需要：

1. **重载 `Copyright()` 函数**，返回你的版权信息
2. 其他什么都不用做！

```cpp
class MyMethod : public Interface::MethodPreset {
public:
    // 只需重载 Copyright() 函数
    Interface::CopyrightData Copyright() const override {
        return {
            "MyOrganization",
            "Copyright (c) 2025 MyOrganization\n"
            "License: MIT"
        };
    }
    
    // 其他方法实现...
};
```

`MethodPreset::Build()` 会自动处理版权跟踪的启动和停止。

### 方式 2: 从 `Method` 直接派生

如果你的 Method **直接继承自 `Method` 基类**，你需要**手动管理版权跟踪**：

1. **重载 `Copyright()` 函数**
2. 在 `Build()` 方法中调用 `StartCopyrightTracking()` 和 `StopCopyrightTracking()`

```cpp
class MyMethod : public Interface::Method {
public:
    // 1. 重载 Copyright() 函数
    Interface::CopyrightData Copyright() const override {
        return {
            "MyOrganization",
            "Copyright (c) 2025 MyOrganization\n"
            "License: MIT"
        };
    }
    
    // 2. 在 Build() 中手动管理版权跟踪
    Interface::DataPtr Build() override {
        // ⚠️ 重要：在开始处理前启动版权跟踪
        StartCopyrightTracking();
        
        try {
            // 你的算法实现...
            Interface::DataPtr result = ProcessData();
            
            // ⚠️ 重要：在返回前停止版权跟踪
            StopCopyrightTracking();
            return result;
        }
        catch (const std::exception& e) {
            // 异常处理中也要停止跟踪
            StopCopyrightTracking();
            throw;
        }
    }
    
    // 其他方法实现...
};
```

#### ⚠️ 重要警告

如果从 `Method` 直接派生但**没有调用** `StartCopyrightTracking()` 和 `StopCopyrightTracking()`：

- ✗ **你的插件不会被版权管理器跟踪**
- ✗ **版权信息将会丢失**
- ✗ **依赖关系图中不会显示你的插件**
- ✗ **责任自负**：这可能导致版权合规问题

### 版权跟踪函数说明

```cpp
// 在 Method 基类中提供的函数
protected:
    /**
     * @brief 启动版权跟踪
     * @details 将当前方法添加到版权管理器的跟踪栈中
     *          在 Build() 开始时调用
     */
    void StartCopyrightTracking();
    
    /**
     * @brief 停止版权跟踪
     * @details 从版权管理器的跟踪栈中移除当前方法
     *          在 Build() 结束时调用（包括异常情况）
     */
    void StopCopyrightTracking();
```

### 选择建议

| 派生方式                  | 版权跟踪   | 推荐场景                   |
| ------------------------- | ---------- | -------------------------- |
| `MethodPreset` 或其派生类 | 自动启用   | ✅ **推荐**：大多数插件开发 |
| `Method` 直接派生         | 需手动管理 | 仅在有特殊需求时使用       |

**总结**：除非有特殊原因，否则应该从 `MethodPreset` 派生，这样版权跟踪会自动处理。

### 参考实现

可以参考以下文件了解实现细节：

- `/po_core/src/internal/interfaces.hpp` - `Method` 基类定义
- `/po_core/src/internal/interfaces.cpp` - 版权跟踪函数实现
- `/po_core/src/internal/interfaces_preset.hpp` - `MethodPreset` 自动版权跟踪实现

---

## 如何在插件中定义版权信息

插件开发者需要在方法类中**重载 `Copyright()` 函数**：

### 基本用法

```cpp
// 在你的Method派生类中 | In your Method-derived class
class MyFeatureExtractor : public Interface::Method {
public:
    // 重载Copyright函数，返回版权信息 | Override Copyright() to return copyright info
    Interface::CopyrightData Copyright() const override {
        return {
            "YourOrganization",  // 版权标识符 | Copyright identifier

            // 详细版权信息 | Detailed copyright information
            "Copyright (c) 2025 YourOrganization, All rights reserved.\n"
            "License: MIT License\n"
            "Author: Your Name\n"
            "Email: your.email@example.com"
        };
    }

    // 其他方法实现...
    // Other method implementations...
};
```

### 默认行为

如果不重载此函数，默认实现会返回：
```cpp
return std::make_pair("Unknown", "");
```

这意味着该插件将被归类为**无版权标识类型**。

### 版权信息编写规范

推荐在版权信息中包含以下内容：

1. **版权声明** - 版权所有者和年份
2. **许可证类型** - 使用的开源许可证（如MIT、Apache 2.0、GPL等）
3. **作者信息** - 作者姓名和联系方式
4. **引用信息** - 如果插件根据学术论文编制，请包含引用格式（可选）
5. **专利声明** - 涉及专利保护的算法（可选）

### 完整示例

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "MyResearchLab",  // 简洁的标识符，用于依赖图显示

        // 多行详细信息
        "Copyright (c) 2025 MyResearchLab, University of Example\n"
        "License: BSD 3-Clause License\n"
        "\n"
        "Author: Dr. Example Researcher\n"
        "Email: researcher@example.edu\n"
        "\n"
        "Citation:\n"
        "  E. Researcher et al., \"Novel Algorithm for Feature Extraction,\"\n"
        "  IEEE Conference on Computer Vision, 2025.\n"
        "\n"
        "Patent: US 10,123,456 (Method and System for...)"
    };
}
```

(third-party-copyright-important)=
### 使用第三方库时的版权声明 ⚠️ 重要

**当前版本限制**：系统目前**仅支持每个Method返回一个版权信息对**（一个标识符 + 一个信息字符串）。

如果插件使用了**多个第三方库**，需要在**同一个版权信息字符串中**列出所有依赖库的版权信息：

#### 推荐格式（多依赖库版权模板）

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "YourOrganization",  // 主版权标识符（使用你的组织名）

        // ===== 主版权信息 =====
        "Copyright (c) 2025 YourOrganization Ltd.\n"
        "License: MIT License\n"
        "Author: Your Name\n"
        "\n"
        // ===== 第三方库版权声明 =====
        "This plugin uses the following third-party libraries:\n"
        "\n"
        "[1] OpenCV 4.x\n"
        "    Copyright (c) 2000-2025, Intel Corporation\n"
        "    Copyright (c) 2009-2011, Willow Garage Inc.\n"
        "    License: Apache License 2.0\n"
        "    URL: https://opencv.org\n"
        "\n"
        "[2] Eigen 3.x\n"
        "    Copyright (c) Eigen Authors\n"
        "    License: MPL2 (Mozilla Public License 2.0)\n"
        "    URL: https://eigen.tuxfamily.org\n"
        "\n"
        "[3] FLANN 1.9.x\n"
        "    Copyright (c) 2008-2011, Marius Muja and David G. Lowe\n"
        "    License: BSD License\n"
        "    URL: https://github.com/mariusmuja/flann"
    };
}
```

#### 简化格式（适用于依赖库较少的情况）

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "MyCompany",

        "Copyright (c) 2025 MyCompany Ltd. | License: Commercial\n"
        "\n"
        "Third-party dependencies:\n"
        "- OpenCV (Apache 2.0): Copyright (c) OpenCV contributors\n"
        "- Eigen (MPL2): Copyright (c) Eigen Authors\n"
        "- Ceres (BSD): Copyright (c) Ceres Solver Team"
    };
}
```

#### ⚠️ 注意事项

1. **版权标识符选择**：
   - 使用**你的组织名**作为主版权标识符（第一个参数）
   - 不要使用第三方库名称（如 "OpenCV"），因为那会误导依赖关系图

2. **完整性要求**：
   - **必须列出所有直接使用的第三方库**
   - 包含库的版权声明、许可证类型
   - 建议包含库的官方URL

3. **格式清晰**：
   - 使用分隔线或编号区分不同的库
   - 保持格式整洁，便于阅读

4. **许可证兼容性**：
   - 确保所使用的许可证与依赖库的许可证兼容
   - 例如：GPL库不能用于MIT或商业许可的插件中

## 应用场景

### 学术研究

-  自动生成引用列表，方便论文写作
-  追踪使用的算法来源和版本
-  确保合规的学术引用

### 商业应用

-  版权合规审计，满足法律要求
-  开源许可证管理和兼容性检查
-  第三方库使用情况追踪

### 开源项目

-  贡献者归属和信用展示
-  许可证兼容性自动检查
-  社区透明度和信任建设

## 最佳实践建议

### ✅ 推荐做法

1. **简洁的标识符**：使用组织名或项目名作为版权标识符（如 "OpenCV"、"PoSDK"）
2. **完整的信息**：包含版权声明、许可证类型、作者信息
3. **第三方声明**：明确列出使用的第三方库及其许可证
4. **学术引用**：如果基于论文，提供标准引用格式

### ❌ 避免做法

1. **过长的文本**：避免在版权信息中包含大段代码或文档
2. **隐藏版权**：不要返回空字符串或 "Unknown"
3. **错误信息**：确保版权信息真实准确
4. **遗漏许可证**：必须明确说明使用的许可证类型

## 常见问题

### Q: 如果我不想显示版权信息怎么办？

A: **所有插件都必须提供版权信息**。这是PoSDK的设计原则，用于保护所有开发者的知识产权。如果插件是开源的，可以选择MIT、Apache等宽松许可证。

### Q: 版权信息会影响性能吗？

A: 不会。版权信息仅在方法**首次创建**时显示一次，后续调用不会重复显示或影响性能。

### Q: 可以修改其他人的版权信息吗？

A: **不可以**。每个插件的版权信息由其源代码中的 `Copyright()` 函数定义，只有插件作者才能修改。编译后的插件版权信息无法被外部更改。

### Q: 我的插件使用了多个第三方库，都需要列出来吗？

A: **是的，必须列出所有直接使用的第三方库及其许可证**。这是开源合规的最佳实践，也是法律法规要求。

**注意**：当前系统仅支持返回一个版权信息对，因此你需要在同一个字符串中列出所有依赖库。请参考上面的"[使用第三方库时的版权声明](third-party-copyright-important)"部分的模板。

### Q: 为什么不能为每个第三方库返回独立的版权信息？

A: 这是当前版本的技术限制。`Copyright()` 函数只能返回一个 `std::pair<std::string, std::string>`（版权标识符 + 版权信息）。

**未来改进计划**：我们考虑在未来版本中支持返回多个版权信息对，届时每个依赖库可以有独立的版权标识符和信息。

**当前解决方案**：在版权信息字符串中使用清晰的格式（如编号、分隔线）区分不同的库，确保信息完整且易读。

### Q: 如果我忘记列出某个第三方库的版权信息会怎样？

A: 这可能导致：
- **法律风险**：违反开源许可证要求（如Apache 2.0、GPL等都要求保留版权声明）
- **合规问题**：商业应用可能无法通过版权审计
- **社区信任**：降低开源社区对你的插件的信任度

**建议**：在开发过程中维护一个依赖库清单，编写版权信息时逐一核对。

---

**版权跟踪让PoSDK生态中的每一个贡献都被记录和尊重，保护开发者的知识产权，同时确保使用者了解所用算法的来源和许可要求。**
