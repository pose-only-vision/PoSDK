/**
 * @file opengv_model_estimator.hpp
 * @brief OpenGV相对位姿估计器
 * @details 使用OpenGV库实现相对位姿估计,支持多种算法
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once
#include <po_core/po_logger.hpp>
#include <po_core.hpp>
#include <common/converter/converter_opengv.hpp>
#include <opengv/relative_pose/methods.hpp>
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <opengv/sac/Ransac.hpp>
#include <opengv/sac_problems/relative_pose/CentralRelativePoseSacProblem.hpp>
using Vec = Eigen::VectorXd;
using Matrix3x3Arr = std::vector<Eigen::Matrix3d>;
using Matrix3x3 = Eigen::Matrix3d;
using Mat3 = Eigen::Matrix3d;
using Vec3 = Eigen::Vector3d;
using IndexArr = std::vector<uint32_t>;

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace PoSDK::Interface;
    using namespace PoSDK::types;
    using namespace opengv;

    class RotationAveragingChatterjee : public MethodPresetProfiler
    {
    public:
        RotationAveragingChatterjee()
        {
            // 设置需要的输入数据包
            required_package_["data_relative_poses"] = nullptr; // 相对位姿数据
            required_package_["data_global_poses"] = nullptr;   // 全局位姿数据(输出)

            // 初始化配置
            InitializeDefaultConfigPath();
        };

        ~RotationAveragingChatterjee() override = default;

        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        /**
         * @brief Compute an initial estimation of global rotation (chain rotations along a MST).
         *
         * @param[in] RelRs Relative weighted rotation matrices
         * @param[out] Rs output global rotation matrices
         * @param[in] nMainViewID Id of the image considered as Identity (unit rotation)
         */
        void InitRotationsMST(
            const RelativeRotations &RelRs,
            Matrix3x3Arr &Rs,
            const uint32_t nMainViewID);

        /**
         * @brief Compute an initial estimation of global rotation and refines them under the L1 norm, [1].
         *
         * @param[in] RelRs Relative weighted rotation matrices
         * @param[out] Rs output global rotation matrices
         * @param[in] nMainViewID Id of the image considered as Identity (unit rotation)
         * @param[in] threshold (optional) threshold
         * @param[out] vec_inliers rotation labelled as inliers or outliers
         */
        bool GlobalRotationsRobust(
            const RelativeRotations &RelRs,
            Matrix3x3Arr &Rs,
            const uint32_t nMainViewID,
            float threshold = 0.f,
            std::vector<bool> *vec_inliers = nullptr);

        inline static double D2R(double degree)
        {
            return degree * M_PI / 180.0;
        }

        /**
         * @brief Implementation of Iteratively Reweighted Least Squares (IRLS) [1].
         *
         * @param[in] RelRs Relative weighted rotation matrices
         * @param[out] Rs output global rotation matrices
         * @param[in] nMainViewID Id of the image considered as Identity (unit rotation)
         * @param[in] sigma factor
         */
        bool RefineRotationsAvgL1IRLS(
            const RelativeRotations &RelRs,
            Matrix3x3Arr &Rs,
            const uint32_t nMainViewID,
            const double sigma = D2R(5));

        /**
         * @brief Sort relative rotation as inlier, outlier rotations.
         *
         * @param[in] RelRs Relative weighted rotation matrices
         * @param[out] Rs output global rotation matrices
         * @param[in] threshold used to label rotations as inlier, or outlier (if 0, threshold is computed with the X84 law)
         * @param[in] vec_inliers inlier, outlier labels
         */
        unsigned int FilterRelativeRotations(
            const RelativeRotations &RelRs,
            const Matrix3x3Arr &Rs,
            float threshold = 0.f,
            std::vector<bool> *vec_inliers = nullptr);
    };
} // namespace PluginMethods
