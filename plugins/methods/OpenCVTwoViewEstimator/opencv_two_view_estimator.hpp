/**
 * @file opencv_two_view_estimator.hpp
 * @brief OpenCV two-view estimator | OpenCV双视图估计器
 * @details Implementation of two-view pose estimation using OpenCV library, supporting multiple algorithms
 * 使用OpenCV库实现双视图位姿估计,支持多种算法
 * @copyright Copyright (c) 2024 XXX Company | Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <boost/algorithm/string.hpp>
#include <po_core/interfaces_robust_estimator.hpp>
#include <po_core/po_logger.hpp>

namespace PluginMethods
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    class OpenCVTwoViewEstimator : public MethodPresetProfiler
    {
    public:
        /**
         * @brief OpenCV algorithm type enumeration | OpenCV算法类型枚举
         */
        enum class OpenCVAlgorithm
        {
            // Fundamental matrix algorithms | 基础矩阵算法
            FUNDAMENTAL_7POINT,
            FUNDAMENTAL_8POINT,
            FUNDAMENTAL_RANSAC,
            FUNDAMENTAL_LMEDS,
            FUNDAMENTAL_RHO,

            // Essential matrix algorithms | 本质矩阵算法
            ESSENTIAL_RANSAC,
            ESSENTIAL_LMEDS,

            // USAC advanced algorithms | USAC高级算法
            ESSENTIAL_USAC_DEFAULT,
            ESSENTIAL_USAC_PARALLEL,
            ESSENTIAL_USAC_FM_8PTS,
            ESSENTIAL_USAC_FAST,
            ESSENTIAL_USAC_ACCURATE,
            ESSENTIAL_USAC_PROSAC,
            ESSENTIAL_USAC_MAGSAC,

            // Homography matrix algorithms | 单应性矩阵算法
            HOMOGRAPHY_RANSAC,
            HOMOGRAPHY_LMEDS,
            HOMOGRAPHY_RHO
        };

        OpenCVTwoViewEstimator();
        ~OpenCVTwoViewEstimator() override = default;

        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        /**
         * @brief Create algorithm enum from string | 从字符串创建算法枚举
         * @param algorithm_str Algorithm string | 算法字符串
         * @return OpenCVAlgorithm Algorithm enum | 算法枚举
         */
        OpenCVAlgorithm CreateAlgorithmFromString(const std::string &algorithm_str) const;

        /**
         * @brief Determine if algorithm is robust estimation method | 判断算法是否为鲁棒估计方法
         * @param algorithm Algorithm enum | 算法枚举
         * @return true if robust method, false otherwise | true如果是鲁棒方法，false否则
         */
        bool IsRobustAlgorithm(OpenCVAlgorithm algorithm) const;

        /**
         * @brief Get minimum number of samples required for algorithm | 获取算法所需的最小样本数
         * @param algorithm Algorithm enum | 算法枚举
         * @return Minimum number of samples | 最小样本数
         */
        size_t GetMinimumSamplesForAlgorithm(OpenCVAlgorithm algorithm) const;

        /**
         * @brief Estimate fundamental matrix | 估计基础矩阵
         * @param points1 First view point set | 第一视图点集
         * @param points2 Second view point set | 第二视图点集
         * @param algorithm Algorithm type | 算法类型
         * @param inliers_mask Output inlier mask | 输出内点掩码
         * @return Fundamental matrix | 基础矩阵
         */
        cv::Mat EstimateFundamentalMatrix(
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            OpenCVAlgorithm algorithm,
            cv::Mat &inliers_mask);

        /**
         * @brief Estimate essential matrix | 估计本质矩阵
         * @param points1 First view point set | 第一视图点集
         * @param points2 Second view point set | 第二视图点集
         * @param camera_matrix Camera intrinsic matrix | 相机内参矩阵
         * @param algorithm Algorithm type | 算法类型
         * @param inliers_mask Output inlier mask | 输出内点掩码
         * @return Essential matrix | 本质矩阵
         */
        cv::Mat EstimateEssentialMatrix(
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            const cv::Mat &camera_matrix,
            OpenCVAlgorithm algorithm,
            cv::Mat &inliers_mask);

        /**
         * @brief Estimate homography matrix | 估计单应性矩阵
         * @param points1 First view point set | 第一视图点集
         * @param points2 Second view point set | 第二视图点集
         * @param algorithm Algorithm type | 算法类型
         * @param inliers_mask Output inlier mask | 输出内点掩码
         * @return Homography matrix | 单应性矩阵
         */
        cv::Mat EstimateHomography(
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            OpenCVAlgorithm algorithm,
            cv::Mat &inliers_mask);

        /**
         * @brief Recover pose from essential matrix | 从本质矩阵恢复位姿
         * @param essential_matrix Essential matrix | 本质矩阵
         * @param points1 First view point set | 第一视图点集
         * @param points2 Second view point set | 第二视图点集
         * @param camera_matrix Camera intrinsic matrix | 相机内参矩阵
         * @param R Output rotation matrix | 输出旋转矩阵
         * @param t Output translation vector | 输出平移向量
         * @param inliers_mask Inlier mask | 内点掩码
         * @return true if successful, false otherwise | 恢复是否成功
         */
        bool RecoverPoseFromEssential(
            const cv::Mat &essential_matrix,
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            const cv::Mat &camera_matrix,
            cv::Mat &R,
            cv::Mat &t,
            cv::Mat &inliers_mask);

        /**
         * @brief Recover pose from fundamental matrix | 从基础矩阵恢复位姿
         * @param fundamental_matrix Fundamental matrix | 基础矩阵
         * @param points1 First view point set | 第一视图点集
         * @param points2 Second view point set | 第二视图点集
         * @param camera_matrix1 First view camera intrinsic matrix | 第一视图相机内参
         * @param camera_matrix2 Second view camera intrinsic matrix | 第二视图相机内参
         * @param R Output rotation matrix | 输出旋转矩阵
         * @param t Output translation vector | 输出平移向量
         * @param inliers_mask Inlier mask | 内点掩码
         * @return true if successful, false otherwise | 恢复是否成功
         */
        bool RecoverPoseFromFundamental(
            const cv::Mat &fundamental_matrix,
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            const cv::Mat &camera_matrix1,
            const cv::Mat &camera_matrix2,
            cv::Mat &R,
            cv::Mat &t,
            cv::Mat &inliers_mask);

        /**
         * @brief Update inlier flags for matched samples | 更新匹配样本的内点标记
         * @param matches Matches data reference | 匹配数据引用
         * @param inliers_mask Inlier mask | 内点掩码
         */
        void UpdateInlierFlags(
            IdMatches &matches,
            const cv::Mat &inliers_mask);

        /**
         * @brief Get OpenCV method flag | 获取OpenCV算法对应的标志位
         * @param algorithm Algorithm type | 算法类型
         * @return OpenCV method flag | OpenCV算法标志位
         */
        int GetOpenCVMethodFlag(OpenCVAlgorithm algorithm) const;
    };

} // namespace PluginMethods
