/**
 * @file poselib_model_estimator.hpp
 * @brief PoseLib相对位姿估计器
 * @details 使用PoseLib库实现相对位姿估计,支持多种算法
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opengv.hpp>
#include <boost/algorithm/string.hpp>

// PoseLib头文件
#include <PoseLib/poselib.h>
#include <po_core/po_logger.hpp>
namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    class PoseLibModelEstimator : public MethodPresetProfiler
    {
    public:
        /**
         * @brief 模型优化方法枚举
         */
        enum class RefineMethod
        {
            NONE,          ///< 不进行优化
            BUNDLE_ADJUST, ///< 使用Bundle Adjustment优化
            NONLINEAR      ///< 使用非线性优化方法
        };

        PoseLibModelEstimator();
        ~PoseLibModelEstimator() override = default;

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
            if (boost::algorithm::iequals(refine_str, "bundle_adjust"))
                return RefineMethod::BUNDLE_ADJUST;
            if (boost::algorithm::iequals(refine_str, "nonlinear"))
                return RefineMethod::NONLINEAR;
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

        /**
         * @brief 直接估计相对位姿
         * @param points1 第一视图的2D点
         * @param points2 第二视图的2D点
         * @param camera1 第一视图的相机参数
         * @param camera2 第二视图的相机参数
         * @return 相对位姿结果
         */
        poselib::CameraPoseVector EstimateRelativePose(
            const std::vector<poselib::Point2D> &points1,
            const std::vector<poselib::Point2D> &points2,
            const poselib::Camera &camera1,
            const poselib::Camera &camera2);

        /**
         * @brief RANSAC鲁棒估计相对位姿
         * @param points1 第一视图的2D点
         * @param points2 第二视图的2D点
         * @param camera1 第一视图的相机参数
         * @param camera2 第二视图的相机参数
         * @param inliers 输出内点索引
         * @return 相对位姿结果
         */
        poselib::CameraPose EstimateRelativePoseRansac(
            const std::vector<poselib::Point2D> &points1,
            const std::vector<poselib::Point2D> &points2,
            const poselib::Camera &camera1,
            const poselib::Camera &camera2,
            std::vector<char> &inliers);

        /**
         * @brief 优化模型
         * @param points1 第一视图的2D点
         * @param points2 第二视图的2D点
         * @param camera1 第一视图的相机参数
         * @param camera2 第二视图的相机参数
         * @param initial_pose 初始位姿
         * @param refine_method 优化方法
         * @return 优化后的位姿
         */
        poselib::CameraPose RefineModel(
            const std::vector<poselib::Point2D> &points1,
            const std::vector<poselib::Point2D> &points2,
            const poselib::Camera &camera1,
            const poselib::Camera &camera2,
            const poselib::CameraPose &initial_pose,
            RefineMethod refine_method);

        /**
         * @brief 从PoseLib的CameraPose转换为PoSDK的RelativePose
         * @param pose PoseLib的位姿
         * @param view_i 源视图ID
         * @param view_j 目标视图ID
         * @return PoSDK的相对位姿
         */
        RelativePose ConvertPoseLibToOpenGV(
            const poselib::CameraPose &pose,
            IndexT view_i,
            IndexT view_j);

        /**
         * @brief 从IdMatches转换为PoseLib所需的bearing vectors
         * @param sample_ptr 匹配数据
         * @param features_ptr 特征点数据
         * @param cameras_ptr 相机模型数据
         * @param view_pair 视图对
         * @param x1 输出第一视图的bearing vectors
         * @param x2 输出第二视图的bearing vectors
         * @return 是否转换成功
         */
        bool ConvertToPoseLibBearingVectors(
            const std::shared_ptr<DataSample<IdMatches>> &sample_ptr,
            const std::shared_ptr<FeaturesInfo> &features_ptr,
            const std::shared_ptr<CameraModels> &cameras_ptr,
            const ViewPair &view_pair,
            std::vector<Eigen::Vector3d> &x1,
            std::vector<Eigen::Vector3d> &x2);

        /**
         * @brief 从IdMatches转换为PoseLib所需的2D点对和相机参数
         * @param sample_ptr 匹配数据
         * @param features_ptr 特征点数据
         * @param cameras_ptr 相机模型数据
         * @param view_pair 视图对
         * @param points1 输出第一视图的2D点
         * @param points2 输出第二视图的2D点
         * @param camera1 输出第一视图的相机参数
         * @param camera2 输出第二视图的相机参数
         * @return 是否转换成功
         */
        bool ConvertToPoseLibPoints(
            const std::shared_ptr<DataSample<IdMatches>> &sample_ptr,
            const std::shared_ptr<FeaturesInfo> &features_ptr,
            const std::shared_ptr<CameraModels> &cameras_ptr,
            const ViewPair &view_pair,
            std::vector<poselib::Point2D> &points1,
            std::vector<poselib::Point2D> &points2,
            poselib::Camera &camera1,
            poselib::Camera &camera2);

        /**
         * @brief 获取算法所需的最小样本数
         * @param algorithm 算法名称
         * @return 最小样本数
         */
        size_t GetMinimumSamplesForAlgorithm(const std::string &algorithm) const;

        /**
         * @brief 创建PoseLib求解器配置
         * @return 求解器配置
         */
        poselib::RansacOptions CreateRansacOptions() const;

        /**
         * @brief 检查位姿的有效性
         * @param pose 位姿
         * @return 是否有效
         */
        bool IsPoseValid(const poselib::CameraPose &pose) const;
    };

} // namespace PluginMethods