/**
 * @file TwoViewEstimator.hpp
 * @brief 双视图位姿估计器
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opengv.hpp>
#include <opengv/relative_pose/methods.hpp>
#include <po_core/po_logger.hpp>
namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;
    class TwoViewEstimator : public Interface::MethodPresetProfiler
    {
    public:
        TwoViewEstimator();
        ~TwoViewEstimator() = default;

        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

        // 实现 Call 接口
    private:
        /**
         * @brief 从优化器的DataSample中同步内点信息到IdMatches
         * @param matches 匹配点引用，将被修改
         * @param optimizer_method 优化器方法实例
         * @param sample_data 包含内点信息的DataSample
         */
        void UpdateInlierFlagsFromOptimizer(
            IdMatches &matches,
            Interface::MethodPresetPtr optimizer_method,
            std::shared_ptr<DataSample<BearingPairs>> sample_data);

        /**
         * @brief 将算法内部的相对位姿转换为PoSDK标准格式
         * @details 算法内部使用OpenGV约定: xi = R * xj + t
         *          转换为PoSDK标准约定: xj = R * xi + t
         * @param pose_result 输入的相对位姿（OpenGV内部格式）
         * @return 转换后的相对位姿（PoSDK标准格式）
         * @note 转换公式: R_posdk = R_opengv^T, t_posdk = -R_opengv^T * t_opengv
         */
        RelativePose ToPoSDKRelativePoseFormat(const RelativePose &pose_result);

        /**
         * @brief 统一的质量验证函数
         * @param inlier_count 内点数量
         * @param total_matches 总匹配数量
         * @param estimator_name 估计器名称（用于日志输出）
         * @return 是否通过质量验证
         */
        bool ValidateEstimationQuality(size_t inlier_count, size_t total_matches, const std::string &estimator_name) const;

        /**
         * @brief 为当前method设置对应view_pair的GT相对位姿数据
         * @param view_pair 当前处理的视图对
         * @details 从prior_info_中获取GT相对位姿数据，提取当前view_pair对应的位姿，并设置给method
         */
        void SetCurrentViewPairGTData(const ViewPair &view_pair);

        /**
         * @brief 为指定method设置对应view_pair的GT相对位姿数据（线程安全版本）
         * @param view_pair 当前处理的视图对
         * @param method 指定的方法实例
         * @details 从prior_info_中获取GT相对位姿数据，提取当前view_pair对应的位姿，并设置给指定的method
         */
        void SetCurrentViewPairGTDataForMethod(const ViewPair &view_pair, Interface::MethodPresetPtr method);

        /**
         * @brief 使用PoSDK TwoViewOptimizer对位姿进行精细优化
         * @param initial_pose 初始位姿估计
         * @param bearing_pairs 对应的bearing pairs数据
         * @param view_pair 视图对信息
         * @param matches 匹配数据（用于更新内点标记和质量检查）
         * @return 优化后的位姿，如果优化失败则返回nullptr
         */
        std::shared_ptr<RelativePose> ApplyPoSDKRefinement(
            const RelativePose &initial_pose,
            const BearingPairs &bearing_pairs,
            const ViewPair &view_pair,
            IdMatches &matches);

        /**
         * @brief 显示进度条
         * @param current 当前进度
         * @param total 总数
         * @param task_name 任务名称
         * @param bar_width 进度条宽度（默认50）
         */
        void ShowProgressBar(size_t current, size_t total, const std::string &task_name, int bar_width = 50);

        Interface::MethodPresetPtr current_method_; ///< 当前使用的方法实例
    };

} // namespace PluginMethods
