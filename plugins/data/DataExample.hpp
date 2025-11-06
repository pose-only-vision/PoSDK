#pragma once

// ==================== PoSDK Data Serialization Example | PoSDK 数据序列化示例 ======================
// This is a simplified example showing how to use PbDataIO for custom data types
// 这是一个简化的示例，展示如何为自定义数据类型使用 PbDataIO
//
// Note: po_core library already implements complete DataFeatures class internally
// 注意：po_core 库内部已经实现了完整的 DataFeatures 类
// This example is provided for reference and learning purposes
// 此示例仅供参考和学习使用
// ====================================================================

#include <po_core.hpp>
#include <filesystem>
#include <vector>
#include <string>

namespace PoSDKPlugin
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    // Example: Simple data storage class showing DataIO usage
    // 示例：展示 DataIO 用法的简单数据存储类
    //
    // For production use, consider:
    // 生产环境建议：
    // 1. Use DataPackage for heterogeneous data collections
    //    使用 DataPackage 存储异构数据集合
    // 2. Use po_core's built-in DataFeatures/DataMatches/etc for standard types
    //    使用 po_core 内置的 DataFeatures/DataMatches 等标准类型
    // 3. Implement custom PbDataIO only for truly custom data structures
    //    仅对真正自定义的数据结构实现 PbDataIO
    class DataExample : public DataIO
    {
    public:
        DataExample() = default;

        virtual ~DataExample() = default;

        virtual void *GetData() override
        {
            return static_cast<void *>(&example_data_);
        }

        // Implement pure virtual function
        // 实现纯虚函数
        const std::string &GetType() const override;

        // Simple save/load implementation (for demonstration)
        // 简单的保存/加载实现（用于演示）
        bool Save(const std::string &folder = "",
                  const std::string &filename = "",
                  const std::string &extension = ".txt") override
        {
            // For real implementation, use protobuf or other serialization
            // 实际实现中，使用 protobuf 或其他序列化方式
            LOG_INFO_ZH << "[DataExample] 保存示例数据（简化版）";
            LOG_INFO_EN << "[DataExample] Saving example data (simplified)";
            return true;
        }

        bool Load(const std::string &filepath = "",
                  const std::string &file_type = "txt") override
        {
            LOG_INFO_ZH << "[DataExample] 加载示例数据（简化版）";
            LOG_INFO_EN << "[DataExample] Loading example data (simplified)";
            return true;
        }

    public:
        // Example data member - replace with your own data structure
        // 示例数据成员 - 替换为你自己的数据结构
        std::vector<std::string> example_data_;
    };

    // ==================== Usage Example | 使用示例 ======================
    /*

    Example 1: Basic usage | 基本用法:

    auto example = std::make_shared<DataExample>();
    example->example_data_ = {"data1", "data2", "data3"};
    example->Save("output/path");
    example->Load("output/path");


    Example 2: Use DataPackage for complex data | 使用 DataPackage 存储复杂数据:

    auto package = std::make_shared<DataPackage>();

    // Add multiple data types
    // 添加多种数据类型
    auto features = std::make_shared<DataFeatures>();
    auto matches = std::make_shared<DataMatches>();
    auto example = std::make_shared<DataExample>();

    package->AddData(features);
    package->AddData(matches);
    package->AddData(example);

    // Save all at once
    // 一次性保存所有数据
    package->Save("project.pb");

    // Load all at once
    // 一次性加载所有数据
    package->Load("project.pb");


    Example 3: For features/matches, use built-in classes | 对于特征/匹配，使用内置类:

    #include <po_core.hpp>

    // Use built-in DataFeatures (complete implementation in po_core)
    // 使用内置的 DataFeatures（po_core 中完整实现）
    auto features = std::make_shared<PoSDK::DataFeatures>();

    // Add feature data
    // 添加特征数据
    auto& feature_info = features->GetFeaturesInfo();
    // ... populate feature data ...

    // Save with protobuf
    // 使用 protobuf 保存
    features->Save("features.pb");

    */

} // namespace PoSDKPlugin
