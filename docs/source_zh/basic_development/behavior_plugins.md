# 行为插件 (Behavior Plugin)

行为插件是 `Method` 的一种特殊形式，通常用于封装一系列方法调用，形成一个特定的功能模块或工作流。

## 接口介绍

```{important}
行为插件必须实现 `Method` 的必须接口:
- **`GetType()`**: 只需在头文件中**声明**，由`REGISTRATION_PLUGIN`宏自动实现
- `Build()` 或 `Run()`: 实现具体的算法逻辑，通常行为插件只需重载 `GetType()`，利用基类提供的 `Build()` 方法即可
```

行为插件通常还会重载以下**可选接口**：

- **`virtual const std::string& GetMaterialType() const override;`**
  - 返回该行为期望的主要输入数据类型。
- **`virtual const std::string& GetProductType() const override;`**
  - 返回该行为最终生成的产品数据类型。
- **`virtual void SetOptionsFromConfigFile(const std::string& path, const std::string& file_type) override;`**
  - 如果需要，可以自定义当前行为所有方法配置的加载逻辑。

## 可选派生方式

- **`Interface::Behavior`** (派生自 `Method`)
  - 提供行为插件的基础接口。
- **`Interface::BehaviorPreset`** (派生自 `Behavior`)
  - (似乎在当前代码中未完全实现或使用，但概念上存在) 可以提供类似 `MethodPreset` 的预设功能，如自动加载行为步骤、管理子方法配置等。

## 示例

```cpp
// my_behavior.hpp
#include <po_core.hpp>

namespace MyPlugin {
using namespace PoSDK;
using namespace Interface;

class MyBehavior : public BehaviorPreset { // 继承 BehaviorPreset

public:
    MyBehavior();
    // 只需声明，由 REGISTRATION_PLUGIN 宏自动实现
    const std::string& GetType() const override;
    // Run() 通常在 BehaviorPreset 中不需要重载，Build 会处理流程
    // DataPtr Run() override; 
    const std::string& GetMaterialType() const override;
    const std::string& GetProductType() const override; 
};
} // namespace MyPlugin


// my_behavior.cpp
#include "my_behavior.hpp"

namespace MyPlugin {

MyBehavior::MyBehavior() {
    // 定义行为包含的串行方法步骤
    sequential_methods_ = {
        "method_feature_extraction", 
        "method_matching", 
        "method_track_building"
    };
    

}

// 不需要手动实现 GetType()，宏会自动生成

const std::string& MyBehavior::GetMaterialType() const {
    static const std::string type = "data_images"; // 假设输入是图像
    return type;
}

const std::string& MyBehavior::GetProductType() const {
    static const std::string type = "data_tracks"; // 假设输出是轨迹
    return type;
}

// Build 方法由 BehaviorPreset 基类提供，它会按 sequential_methods_ 顺序执行方法
// DataPtr MyBehavior::Build(const DataPtr& material_ptr) { ... }

} // namespace MyPlugin

// 插件注册 - GetType() 由宏自动实现
REGISTRATION_PLUGIN(MyPlugin::MyBehavior, "my_behavior")
```

## 常用数据类型

行为插件的输入 (`MaterialType`) 和输出 (`ProductType`) 取决于其封装的工作流程。

## 行为参数配置

- **设置**：行为插件本身可以通过 `SetOptionsFromConfigFile` 加载配置（例如定义方法调用顺序）。
- **管理**：行为插件内部调用的每个方法，其配置通常由行为插件的 `method_options_`来管理和传递。
- **优先级**：行为插件的配置 > 方法插件的配置