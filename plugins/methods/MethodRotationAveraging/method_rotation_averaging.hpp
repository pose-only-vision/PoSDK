/**
 * @file method_rotation_averaging.hpp
 * @brief 旋转平均方法
 * @details 实现全局旋转估计,支持多种旋转平均算法
 *
 * @copyright Copyright (c) 2024 Qi Cai
 * Licensed under the Mozilla Public License Version 2.0
 */

#ifndef _METHOD_ROTATION_AVERAGING_HPP_
#define _METHOD_ROTATION_AVERAGING_HPP_

#include <po_core.hpp>
#include <filesystem>
#include <fstream>
#include <po_core/po_logger.hpp>
namespace PoSDK
{
    using namespace Interface;
    using namespace types;

    class MethodRotationAveraging : public Interface::MethodPresetProfiler
    {
    public:
        MethodRotationAveraging()
        {
            // 设置需要的输入数据包
            required_package_["data_relative_poses"] = nullptr; // 相对位姿数据

            // 初始化配置
            InitializeDefaultConfigPath();

            // 自动检测GraphOptim二进制文件目录并缓存
            graphoptim_bin_folder_ = DetectGraphOptimBinPath();
        }

        ~MethodRotationAveraging() override = default;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

        DataPtr Run() override;

    private:
        /**
         * @brief 使用GraphOptim进行旋转平均
         * @param relative_poses 相对位姿数据
         * @param global_poses 全局位姿数据
         * @return 是否成功
         */
        bool RunGraphOptim(
            const RelativePoses &relative_poses,
            GlobalPoses &global_poses);

        /**
         * @brief 从GraphOptim结果文件读取全局旋转
         * @param filename 结果文件名
         * @param global_poses 全局位姿数据
         * @return 是否成功
         */
        bool ImportFromGraphOptim(
            const std::string &filename,
            GlobalPoses &global_poses) const;

        /**
         * @brief 检查GraphOptim工具是否存在且可执行
         * @param tool_path GraphOptim工具路径
         * @return 是否可用
         */
        bool CheckGraphOptimTool(const std::string &tool_path) const;

        /**
         * @brief 自动检测GraphOptim二进制目录路径
         * @return GraphOptim bin目录路径，失败时返回空字符串
         */
        std::string DetectGraphOptimBinPath() const;

    private:
        // GraphOptim二进制文件目录
        std::string graphoptim_bin_folder_;
        // 临时文件路径
        std::string temp_g2o_path_;
        std::string temp_result_path_;
    };

} // namespace PoSDK

#endif // _METHOD_ROTATION_AVERAGING_HPP_
