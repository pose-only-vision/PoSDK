/**
 * @file barath_two_view_estimator.hpp
 * @brief Barath双视图估计器
 * @details 使用MAGSAC、MAGSAC++和SupeRANSAC算法实现双视图位姿估计
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <po_core/po_logger.hpp>
// MAGSAC/SupeRANSAC headers
#include <magsac.h>
#include <estimators.h>
#include <magsac_utils.h>
#include <settings.h>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace Converter;
    using namespace types;

    class BarathTwoViewEstimator : public MethodPresetProfiler
    {
    public:
        BarathTwoViewEstimator()
        {
            // 设置需要的输入数据包
            required_package_["data_sample"] = nullptr;
            required_package_["data_features"] = nullptr;
            required_package_["data_camera_models"] = nullptr;

            // 加载配置
            InitializeDefaultConfigPath();
        }

        ~BarathTwoViewEstimator() override = default;

        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        enum class Algorithm
        {
            MAGSAC,
            MAGSAC_PLUS_PLUS,
            SUPERANSAC
        };

        Algorithm CreateAlgorithmFromString(const std::string &algorithm_str);

        std::string GetAlgorithmName(Algorithm algorithm) const;
        size_t GetMinimumSamples(Algorithm algorithm) const;

        bool EstimateEssentialMatrix(
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            const cv::Mat &K,
            Algorithm algorithm,
            cv::Mat &E,
            cv::Mat &inliers_mask);

        // 使用MAGSAC/MAGSAC++算法估计本质矩阵
        bool EstimateEssentialMatrixWithMAGSAC(
            const std::vector<cv::Point2f> &points1,
            const std::vector<cv::Point2f> &points2,
            const cv::Mat &K,
            Algorithm algorithm,
            cv::Mat &E,
            cv::Mat &inliers_mask,
            double threshold,
            double confidence,
            int min_iters,
            int max_iters,
            int partition_num,
            int core_number);

        // 更新匹配数据中的内点标志
        bool UpdateInlierFlags(IdMatches &matches, const cv::Mat &inliers_mask);

        // 自适应内点选择 (Adaptive Inlier Selection)
        bool ApplyAdaptiveInlierSelection(
            const cv::Mat &normalized_points,
            const gcransac::EssentialMatrix &model,
            double normalized_sigma_max,
            double adaptive_max_threshold,
            int adaptive_min_inliers,
            cv::Mat &adaptive_inliers_mask);
    };

} // namespace PluginMethods
