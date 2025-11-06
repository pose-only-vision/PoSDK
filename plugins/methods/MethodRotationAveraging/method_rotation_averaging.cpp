/**
 * @file method_rotation_averaging.cpp
 * @brief Rotation averaging method implementation | 旋转平均方法实现
 */

#include "method_rotation_averaging.hpp"
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统
#include <cstdlib>
#include <sstream>
#include <chrono>
#include <Eigen/Geometry> // For Eigen::Quaterniond

namespace PoSDK
{

    DataPtr MethodRotationAveraging::Run()
    {
        try
        {
            DisplayConfigInfo();

            // Get input data | 获取输入数据
            auto relative_poses_ptr = GetDataPtr<RelativePoses>(required_package_["data_relative_poses"]);
            if (!relative_poses_ptr)
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 无相对位姿数据" << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] No relative poses data" << std::endl;
                return nullptr;
            }

            // Create global pose data as output | 创建全局位姿数据作为输出
            auto global_poses_data = FactoryData::Create("data_global_poses");
            if (!global_poses_data)
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 创建全局位姿数据失败" << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] Failed to create global poses data" << std::endl;
                return nullptr;
            }
            auto global_poses_ptr = GetDataPtr<GlobalPoses>(global_poses_data);
            if (!global_poses_ptr)
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 获取GlobalPoses指针失败" << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] Failed to get GlobalPoses pointer" << std::endl;
                return nullptr;
            }

            // Set temporary file paths | 设置临时文件路径
            std::string temp_dir = GetOptionAsString("temp_dir", "./temp");
            std::filesystem::create_directories(temp_dir);

            temp_g2o_path_ = temp_dir + "/" + GetOptionAsString("g2o_filename", "relative_poses.g2o");
            temp_result_path_ = temp_dir + "/" + GetOptionAsString("estimator_output_g2o", "optimized_poses.g2o");

            // Check GraphOptim tool path | 检查GraphOptim工具路径
            if (graphoptim_bin_folder_.empty())
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 未找到GraphOptim二进制目录" << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] GraphOptim binary directory not found" << std::endl;
                return nullptr;
            }

            std::string rotation_estimator = GetOptionAsString("rotation_estimator", "GraphOptim");

            bool success = false;
            if (rotation_estimator == "GraphOptim")
            {
                success = RunGraphOptim(*relative_poses_ptr, *global_poses_ptr);
            }
            else if (rotation_estimator == "Chatterjee")
            {
                // Create Chatterjee rotation averaging method instance via factory | 通过工厂创建Chatterjee旋转平均方法实例
                auto chatterjee_method = std::dynamic_pointer_cast<Interface::MethodPreset>(
                    FactoryMethod::Create("method_rotation_averaging_Chatterjee"));
                if (!chatterjee_method)
                {
                    LOG_ERROR_ZH << "[MethodRotationAveraging] 创建method_rotation_averaging_Chatterjee失败" << std::endl;
                    LOG_ERROR_EN << "[MethodRotationAveraging] Failed to create method_rotation_averaging_Chatterjee" << std::endl;
                    return nullptr;
                }
                // Chatterjee method may also need to set some options if it has a configuration file | Chatterjee方法可能也需要设置一些选项，如果它有配置文件的话
                // chatterjee_method->SetMethodOptions(...);

                // Execute Chatterjee method | 执行Chatterjee方法
                auto chatterjee_output_data = chatterjee_method->Build(required_package_["data_relative_poses"]);
                if (!chatterjee_output_data)
                {
                    LOG_ERROR_ZH << "[MethodRotationAveraging] method_rotation_averaging_Chatterjee失败" << std::endl;
                    LOG_ERROR_EN << "[MethodRotationAveraging] method_rotation_averaging_Chatterjee failed" << std::endl;
                    return nullptr;
                }
                // Get result from Chatterjee method (assuming it is also GlobalPoses) | 获取Chatterjee方法的结果 (假设它也是GlobalPoses)
                auto chatterjee_global_poses_ptr = GetDataPtr<GlobalPoses>(chatterjee_output_data);
                if (!chatterjee_global_poses_ptr)
                {
                    LOG_ERROR_ZH << "[MethodRotationAveraging] 从Chatterjee输出获取GlobalPoses失败" << std::endl;
                    LOG_ERROR_EN << "[MethodRotationAveraging] Failed to get GlobalPoses from Chatterjee output" << std::endl;
                    return nullptr;
                }
                *global_poses_ptr = *chatterjee_global_poses_ptr; // Copy result to main output | 将结果复制到主输出
                success = true;
            }
            else
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 未知旋转估计器: " << rotation_estimator << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] Unknown rotation estimator: " << rotation_estimator << std::endl;
                return nullptr;
            }

            if (!success)
            {
                LOG_ERROR_ZH << "[MethodRotationAveraging] 估计全局旋转失败" << std::endl;
                LOG_ERROR_EN << "[MethodRotationAveraging] Failed to estimate global rotations" << std::endl;
                return nullptr;
            }

            return global_poses_data;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodRotationAveraging] 错误: " << e.what() << std::endl;
            LOG_ERROR_EN << "[MethodRotationAveraging] Error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    bool MethodRotationAveraging::RunGraphOptim(
        const RelativePoses &relative_poses,
        GlobalPoses &global_poses)
    {

        // 1. Export relative poses to g2o file using PoSDK::file::SaveToG2O | 1. 导出相对位姿到g2o文件 using PoSDK::file::SaveToG2O
        if (!file::SaveToG2O(temp_g2o_path_, relative_poses)) // Use PoSDK::file::SaveToG2O
        {
            LOG_ERROR_ZH << "[MethodRotationAveraging] 使用file::SaveToG2O导出g2o文件失败: " << temp_g2o_path_ << std::endl;
            LOG_ERROR_EN << "[MethodRotationAveraging] Failed to export g2o file using file::SaveToG2O: " << temp_g2o_path_ << std::endl;
            return false;
        }

        // 2. Build GraphOptim command | 2. 构建GraphOptim命令
        std::string rotation_estimator_path = graphoptim_bin_folder_ + "/rotation_estimator";

        if (!CheckGraphOptimTool(rotation_estimator_path))
        {
            LOG_ERROR_ZH << "[MethodRotationAveraging] 未找到或不可执行GraphOptim工具" << std::endl;
            LOG_ERROR_EN << "[MethodRotationAveraging] GraphOptim tool not found or not executable" << std::endl;
            return false;
        }

        // Build command line - pass input g2o and output g2o filenames | 构建命令行 - 传递输入g2o和输出g2o文件名
        std::stringstream cmd;
        cmd << rotation_estimator_path
            << " --g2o_filename=" << temp_g2o_path_
            << " --output_g2o_filename=" << temp_result_path_;

        LOG_DEBUG_ZH << "[MethodRotationAveraging] 执行命令: " << cmd.str() << std::endl;
        LOG_DEBUG_EN << "[MethodRotationAveraging] Executing command: " << cmd.str() << std::endl;

        // 3. Execute GraphOptim with profiling | 3. 执行GraphOptim并进行性能分析
        int ret = 0;
        {
            PROFILER_START_AUTO(enable_profiling_);
            POSDK_SYSTEM(cmd.str().c_str()); // PROFILER_SYSTEM
            PROFILER_END();

            // Print profiling statistics | 打印性能分析统计
            if (SHOULD_LOG(DEBUG))
            {
                PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
            }
        }

        if (ret != 0)
        {
            LOG_ERROR_ZH << "[MethodRotationAveraging] GraphOptim执行失败" << std::endl;
            LOG_ERROR_EN << "[MethodRotationAveraging] GraphOptim execution failed" << std::endl;
            return false;
        }

        // 4. Read results | 4. 读取结果
        if (!file::LoadFromG2O(temp_result_path_, global_poses))
        {
            LOG_ERROR_ZH << "[MethodRotationAveraging] 从G2O文件导入结果失败: " << temp_result_path_ << std::endl;
            LOG_ERROR_EN << "[MethodRotationAveraging] Failed to import results from G2O file: " << temp_result_path_ << std::endl;
            return false;
        }

        // 5. Clean up temporary files | 5. 清理临时文件
        LOG_DEBUG_ZH << "[MethodRotationAveraging] 为检查保留临时文件: " << temp_g2o_path_ << " 和 " << temp_result_path_ << std::endl;
        LOG_DEBUG_EN << "[MethodRotationAveraging] Preserving temporary files for inspection: " << temp_g2o_path_ << " and " << temp_result_path_ << std::endl;

        // Always clean up temporary files, but log details | 总是清理临时文件，但用详细日志记录
        std::filesystem::remove(temp_g2o_path_);
        std::filesystem::remove(temp_result_path_);
        LOG_DEBUG_ZH << "[MethodRotationAveraging] 已清理临时文件." << std::endl;
        LOG_DEBUG_EN << "[MethodRotationAveraging] Cleaned up temporary files." << std::endl;

        return true;
    }

    std::string MethodRotationAveraging::DetectGraphOptimBinPath() const
    {
        // Try multiple possible GraphOptim build paths | 尝试多个可能的GraphOptim构建路径
        std::vector<std::string> candidate_paths = {
            // Prioritize dependency under project source directory (recommended) | 优先尝试项目源码目录下的依赖（推荐方式）
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/GraphOptim/build_scripted/bin",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/GraphOptim/bin",
            // Try upper level directory dependency (compatible with old structure) | 尝试上一级目录的依赖（兼容旧结构）
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/GraphOptim/build_scripted/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/GraphOptim/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/GraphOptim/build/bin",
            // Relative path alternatives (runtime paths) | 相对路径备选（运行时路径）
            "../../dependencies/GraphOptim/build_scripted/bin",
            "../../dependencies/GraphOptim/bin",
            "../dependencies/GraphOptim/build_scripted/bin",
            "../dependencies/GraphOptim/bin",
            "/Users/caiqi/Documents/PoMVG/GraphOptim/bin",
            // Path specified in configuration (manual configuration) | 配置文件指定的路径（手动配置）
            GetOptionAsString("GraphOptim_bin", "")};

        for (const auto &path : candidate_paths)
        {
            if (path.empty())
                continue;

            std::string rotation_estimator_path = path + "/rotation_estimator";
            if (CheckGraphOptimTool(rotation_estimator_path))
            {
                LOG_DEBUG_ZH << "[MethodRotationAveraging] 在 " << path << " 找到GraphOptim" << std::endl;
                LOG_DEBUG_EN << "[MethodRotationAveraging] Found GraphOptim at: " << path << std::endl;
                return path;
            }
        }

        // If not found, try system PATH | 如果都找不到，尝试系统PATH
        if (CheckGraphOptimTool("rotation_estimator"))
        {
            LOG_DEBUG_ZH << "[MethodRotationAveraging] 在系统PATH中找到GraphOptim" << std::endl;
            LOG_DEBUG_EN << "[MethodRotationAveraging] Found GraphOptim in system PATH" << std::endl;
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "[MethodRotationAveraging] 在任何候选路径中未找到GraphOptim rotation_estimator" << std::endl;
        LOG_ERROR_EN << "[MethodRotationAveraging] GraphOptim rotation_estimator not found in any candidate paths" << std::endl;
        return "";
    }

    bool MethodRotationAveraging::CheckGraphOptimTool(const std::string &tool_path) const
    {
        // Check if file exists | 检查文件是否存在
        if (!std::filesystem::exists(tool_path))
        {
            return false;
        }

// Check if executable | 检查是否可执行
#ifdef _WIN32
        // Windows platform | Windows平台
        std::string check_cmd = "where \"" + tool_path + "\" > nul 2>&1";
#else
        // Linux/Unix platform | Linux/Unix平台
        std::string check_cmd = "which \"" + tool_path + "\" > /dev/null 2>&1";
#endif

        return std::system(check_cmd.c_str()) == 0;
    }

} // namespace PoSDK

// Register plugin | 注册插件
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PoSDK::MethodRotationAveraging)
