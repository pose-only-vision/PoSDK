/**
 * @file opengv_model_estimator.hpp
 * @brief OpenGV相对位姿估计器
 * @details 使用OpenGV库实现相对位姿估计,支持多种算法
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opengv.hpp>
#include <opengv/relative_pose/methods.hpp>
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/sac/Ransac.hpp>
#include <opengv/sac_problems/relative_pose/CentralRelativePoseSacProblem.hpp>
#include <opengv/sac_problems/relative_pose/RotationOnlySacProblem.hpp>
#include <opengv/sac_problems/relative_pose/EigensolverSacProblem.hpp>
#include <boost/algorithm/string.hpp>
#include <po_core/po_logger.hpp>
namespace PluginMethods
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace types;
    using namespace opengv;

    class OpenGVModelEstimator : public MethodPresetProfiler
    {
    public:
        /**
         * @brief 模型优化方法枚举
         */
        enum class RefineMethod
        {
            NONE,         ///< 不进行优化
            EIGENSOLVER,  ///< 使用特征值分解法优化旋转
            NONLINEAR,    ///< 使用非线性优化方法优化完整位姿
            ROTATION_ONLY ///< 仅优化旋转部分
        };

        OpenGVModelEstimator();
        ~OpenGVModelEstimator() override = default;

        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        /**
         * @brief 从字符串创建优化方法枚举
         * @param refine_str 优化方法字符串
         * @return RefineMethod 优化方法枚举
         */
        RefineMethod CreateRefineMethodFromString(const std::string &refine_str) const
        {
            if (boost::algorithm::iequals(refine_str, "eigensolver"))
                return RefineMethod::EIGENSOLVER;
            if (boost::algorithm::iequals(refine_str, "nonlinear"))
                return RefineMethod::NONLINEAR;
            if (boost::algorithm::iequals(refine_str, "rotationOnly"))
                return RefineMethod::ROTATION_ONLY;
            // 默认不优化
            return RefineMethod::NONE;
        }

        /**
         * @brief 判断算法是否为RANSAC方法
         * @param algorithm 算法名称
         * @return true如果是RANSAC方法，false否则
         */
        bool IsRansacAlgorithm(const std::string &algorithm) const
        {
            return algorithm.find("_ransac") != std::string::npos;
        }

        // 估计相对位姿
        transformation_t EstimateRelativePose(
            opengv::relative_pose::CentralRelativeAdapter &adapter);

        // RANSAC估计相对位姿
        transformation_t EstimateRelativePoseRansac(
            opengv::relative_pose::CentralRelativeAdapter &adapter,
            std::vector<int> &inliers);

        /**
         * @brief 优化模型
         * @param adapter OpenGV适配器
         * @param initial_transformation 初始变换矩阵
         * @param refine_method 优化方法
         * @return 优化后的变换矩阵
         */
        transformation_t RefineModel(
            opengv::relative_pose::CentralRelativeAdapter &adapter,
            const transformation_t &initial_transformation,
            RefineMethod refine_method);

        opengv::transformation_t GetBestTransformationFromRotations(
            opengv::relative_pose::CentralRelativeAdapter &adapter,
            const opengv::rotations_t &rotations);

        // 从本质矩阵集合中获取最优变换
        opengv::transformation_t GetBestTransformationFromEssentials(
            opengv::relative_pose::CentralRelativeAdapter &adapter,
            const opengv::essentials_t &essentialMatrices);

        // 分配先验信息
        void AssignPriorInfo(
            opengv::relative_pose::CentralRelativeAdapter &adapter);

        /**
         * @brief 获取算法所需的最小样本数
         * @param algorithm 算法名称
         * @return 最小样本数
         */
        size_t GetMinimumSamplesForAlgorithm(const std::string &algorithm) const;
    };

} // namespace PluginMethods
