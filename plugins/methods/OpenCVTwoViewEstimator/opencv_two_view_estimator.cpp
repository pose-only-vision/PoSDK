/**
 * @file opencv_two_view_estimator.cpp
 * @brief OpenCV two-view estimator implementation | OpenCV双视图估计器实现
 * @details Implementation of two-view pose estimation using OpenCV library
 * 使用OpenCV库实现双视图位姿估计
 * @copyright Copyright (c) 2024 XXX Company | Copyright (c) 2024 XXX公司
 */

#include "opencv_two_view_estimator.hpp"
#include <opencv2/calib3d.hpp>
#include <limits>
#include <cmath>
#include <iomanip>
#include <po_core/po_logger.hpp>

namespace PluginMethods
{

    OpenCVTwoViewEstimator::OpenCVTwoViewEstimator()
    {
        // 注册所需数据类型
        required_package_["data_sample"] = nullptr;        // DataSample<IdMatches>
        required_package_["data_features"] = nullptr;      // FeaturesInfo
        required_package_["data_camera_models"] = nullptr; // CameraModels

        // 初始化默认配置路径
        InitializeDefaultConfigPath();
    }

    DataPtr OpenCVTwoViewEstimator::Run()
    {
        DisplayConfigInfo();
        // 1. Get input data | 1. 获取输入数据
        auto sample_ptr = CastToSample<IdMatches>(required_package_["data_sample"]);
        auto features_ptr = GetDataPtr<FeaturesInfo>(required_package_["data_features"]);
        auto cameras_ptr = GetDataPtr<CameraModels>(required_package_["data_camera_models"]);

        if (!sample_ptr || sample_ptr->empty() || !features_ptr || !cameras_ptr)
        {
            LOG_ERROR_ZH << "无效或空输入数据。";
            LOG_ERROR_EN << "Invalid or empty input data.";
            return nullptr;
        }

        // 2. 从method_options_获取视图对信息
        ViewPair view_pair(
            GetOptionAsIndexT("view_i", 0), // 源视图ID
            GetOptionAsIndexT("view_j", 1)  // 目标视图ID
        );

        // 3. 获取算法类型
        std::string algorithm_str = GetOptionAsString("algorithm", "findEssentialMat_ransac");
        OpenCVAlgorithm algorithm = CreateAlgorithmFromString(algorithm_str);

        std::string algo_msg = LanguageEnvironment::GetText(
            "OpenCV 双视图估计器 - 算法: " + algorithm_str,
            "OpenCV Two View Estimator - Algorithm: " + algorithm_str);
        LOG_DEBUG_ZH << "OpenCV 模型估计器: 调试输出已启用";
        LOG_DEBUG_EN << "OpenCV Model Estimator: Debug output is enabled";

        // 4. 检查匹配数据
        size_t total_matches = sample_ptr->size();
        size_t min_samples = GetMinimumSamplesForAlgorithm(algorithm);

        if (total_matches < min_samples)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "算法 " + algorithm_str + " 匹配不足: 获得 " + std::to_string(total_matches) + "，至少需要 " + std::to_string(min_samples),
                "Insufficient matches for algorithm " + algorithm_str + ": got " + std::to_string(total_matches) + ", need at least " + std::to_string(min_samples));
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;

            // 将所有匹配标记为外点
            for (auto &match : *sample_ptr)
            {
                match.is_inlier = false;
            }
            return nullptr;
        }

        std::string match_msg = LanguageEnvironment::GetText(
            "算法: " + algorithm_str + ", 总匹配: " + std::to_string(total_matches) + ", 最小要求: " + std::to_string(min_samples),
            "Algorithm: " + algorithm_str + ", Total matches: " + std::to_string(total_matches) + ", Min required: " + std::to_string(min_samples));
        LOG_DEBUG_ZH << "OpenCV 模型估计器: 调试输出已启用";
        LOG_DEBUG_EN << "OpenCV Model Estimator: Debug output is enabled";

        // 5. 转换匹配数据为OpenCV点集
        std::vector<cv::Point2f> points1, points2;
        if (!Converter::OpenCVConverter::MatchesDataPtr2CVPoints(sample_ptr, *features_ptr, *cameras_ptr,
                                                                 view_pair, points2, points1))
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "转换匹配到 OpenCV 点失败",
                "Failed to convert matches to OpenCV points");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;
            return nullptr;
        }

        // 6. 执行算法估计
        cv::Mat R, t;
        cv::Mat inliers_mask;
        bool success = false;

        try
        {
            // 基础矩阵算法：7点 8点 鲁棒估计（RANSAC、LMEDS、RHO）
            if (algorithm >= OpenCVAlgorithm::FUNDAMENTAL_7POINT &&
                algorithm <= OpenCVAlgorithm::FUNDAMENTAL_RHO)
            {
                // 基础矩阵算法
                cv::Mat fundamental_matrix = EstimateFundamentalMatrix(
                    points1, points2, algorithm, inliers_mask);

                if (!fundamental_matrix.empty())
                {
                    // 获取相机内参
                    const CameraModel *camera1 = (*cameras_ptr)[view_pair.first];
                    const CameraModel *camera2 = (*cameras_ptr)[view_pair.second];

                    if (camera1 && camera2)
                    {
                        cv::Mat K1, K2, dist1, dist2;
                        if (Converter::OpenCVConverter::CameraModel2CVCalibration(
                                *camera1, K1, dist1) &&
                            Converter::OpenCVConverter::CameraModel2CVCalibration(
                                *camera2, K2, dist2))
                        {
                            success = RecoverPoseFromFundamental(
                                fundamental_matrix, points1, points2, K1, K2, R, t, inliers_mask);
                        }
                    }
                }
            }
            else if (algorithm >= OpenCVAlgorithm::ESSENTIAL_RANSAC &&
                     algorithm <= OpenCVAlgorithm::ESSENTIAL_USAC_MAGSAC)
            {
                // 本质矩阵算法
                const CameraModel *camera = (*cameras_ptr)[view_pair.first];
                if (camera)
                {
                    cv::Mat camera_matrix, dist_coeffs;
                    if (Converter::OpenCVConverter::CameraModel2CVCalibration(
                            *camera, camera_matrix, dist_coeffs))
                    {
                        cv::Mat essential_matrix = EstimateEssentialMatrix(
                            points1, points2, camera_matrix, algorithm, inliers_mask);

                        if (!essential_matrix.empty())
                        {
                            success = RecoverPoseFromEssential(
                                essential_matrix, points1, points2, camera_matrix, R, t, inliers_mask);
                        }
                    }
                }
            }
            else if (algorithm >= OpenCVAlgorithm::HOMOGRAPHY_RANSAC &&
                     algorithm <= OpenCVAlgorithm::HOMOGRAPHY_RHO)
            {
                // 单应性矩阵算法 - 仅用于平面场景，这里主要作为演示
                cv::Mat homography = EstimateHomography(points1, points2, algorithm, inliers_mask);

                if (!homography.empty())
                {
                    std::string warn_msg = LanguageEnvironment::GetText(
                        "单应性估计完成，但从单应性恢复位姿尚未实现",
                        "Homography estimation completed, but pose recovery from homography not implemented");
                    LOG_WARNING_ZH << warn_msg;
                    LOG_WARNING_EN << warn_msg;
                    // 这里可以实现从单应性矩阵恢复位姿的方法
                    // 但需要额外的约束信息（如平面法向量等）
                    success = false;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "OpenCV 算法执行错误: ",
                "Error in OpenCV algorithm execution: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            success = false;
        }

        // 7. 检查估计结果
        if (!success || R.empty() || t.empty())
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "位姿估计失败",
                "Failed to estimate pose");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;

            // 将所有匹配标记为外点
            if (auto *matches_ptr = static_cast<IdMatches *>(sample_ptr->GetData()))
            {
                for (auto &match : *matches_ptr)
                {
                    match.is_inlier = false;
                }
            }
            return nullptr;
        }

        // 8. 更新内点标记
        if (IsRobustAlgorithm(algorithm) && !inliers_mask.empty())
        {
            // 从DataPtr获取匹配数据
            auto *matches_ptr = static_cast<IdMatches *>(sample_ptr->GetData());
            if (matches_ptr)
            {
                UpdateInlierFlags(*matches_ptr, inliers_mask);

                // 注意: 质量验证功能已移至TwoViewEstimator统一管控
                // 这里只负责算法执行和内点标记，质量检查由上层框架处理
            }

            if (log_level_ >= PO_LOG_NORMAL)
            {
                int inlier_count = cv::countNonZero(inliers_mask);
                std::string ransac_msg = LanguageEnvironment::GetText(
                    "RANSAC 算法: " + algorithm_str,
                    "RANSAC algorithm: " + algorithm_str);
                LOG_INFO_ZH << ransac_msg;
                LOG_INFO_EN << ransac_msg;

                std::string inlier_msg = LanguageEnvironment::GetText(
                    "总匹配: " + std::to_string(total_matches) + ", 内点: " + std::to_string(inlier_count) + " (" + std::to_string(100.0 * inlier_count / total_matches) + "%)",
                    "Total matches: " + std::to_string(total_matches) + ", Inliers: " + std::to_string(inlier_count) + " (" + std::to_string(100.0 * inlier_count / total_matches) + "%)");
                LOG_INFO_ZH << inlier_msg;
                LOG_INFO_EN << inlier_msg;
            }
        }

        // 9. 转换结果格式
        Matrix3d Rij;
        Vector3d tij;

        // OpenCV输出的是从第一帧到第二帧的变换
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                Rij(i, j) = R.at<double>(i, j);
            }
            tij(i) = t.at<double>(i, 0);
        }

        // 10. 创建并返回结果
        RelativePose relative_pose(
            view_pair.first,
            view_pair.second,
            Rij,
            tij,
            1.0f);

        return std::make_shared<DataMap<RelativePose>>(relative_pose, "data_relative_pose");
    }

    OpenCVTwoViewEstimator::OpenCVAlgorithm OpenCVTwoViewEstimator::CreateAlgorithmFromString(
        const std::string &algorithm_str) const
    {
        // 基础矩阵算法 - 支持两种命名格式
        if (boost::algorithm::iequals(algorithm_str, "findFundamentalMat_7point") ||
            boost::algorithm::iequals(algorithm_str, "FUNDAMENTAL_7POINT"))
            return OpenCVAlgorithm::FUNDAMENTAL_7POINT;
        if (boost::algorithm::iequals(algorithm_str, "findFundamentalMat_8point") ||
            boost::algorithm::iequals(algorithm_str, "FUNDAMENTAL_8POINT"))
            return OpenCVAlgorithm::FUNDAMENTAL_8POINT;
        if (boost::algorithm::iequals(algorithm_str, "findFundamentalMat_ransac") ||
            boost::algorithm::iequals(algorithm_str, "FUNDAMENTAL_RANSAC"))
            return OpenCVAlgorithm::FUNDAMENTAL_RANSAC;
        if (boost::algorithm::iequals(algorithm_str, "findFundamentalMat_lmeds") ||
            boost::algorithm::iequals(algorithm_str, "FUNDAMENTAL_LMEDS"))
            return OpenCVAlgorithm::FUNDAMENTAL_LMEDS;
        if (boost::algorithm::iequals(algorithm_str, "findFundamentalMat_rho") ||
            boost::algorithm::iequals(algorithm_str, "FUNDAMENTAL_RHO"))
            return OpenCVAlgorithm::FUNDAMENTAL_RHO;

        // 本质矩阵算法 - 支持两种命名格式
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_ransac") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_RANSAC"))
            return OpenCVAlgorithm::ESSENTIAL_RANSAC;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_lmeds") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_LMEDS"))
            return OpenCVAlgorithm::ESSENTIAL_LMEDS;

        // USAC算法 - 支持两种命名格式
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_default") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_DEFAULT"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_DEFAULT;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_parallel") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_PARALLEL"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_PARALLEL;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_fm_8pts") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_FM_8PTS"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_FM_8PTS;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_fast") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_FAST"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_FAST;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_accurate") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_ACCURATE"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_ACCURATE;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_prosac") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_PROSAC"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_PROSAC;
        if (boost::algorithm::iequals(algorithm_str, "findEssentialMat_usac_magsac") ||
            boost::algorithm::iequals(algorithm_str, "ESSENTIAL_USAC_MAGSAC"))
            return OpenCVAlgorithm::ESSENTIAL_USAC_MAGSAC;

        // 单应性算法 - 支持两种命名格式
        if (boost::algorithm::iequals(algorithm_str, "findHomography_ransac") ||
            boost::algorithm::iequals(algorithm_str, "HOMOGRAPHY_RANSAC"))
            return OpenCVAlgorithm::HOMOGRAPHY_RANSAC;
        if (boost::algorithm::iequals(algorithm_str, "findHomography_lmeds") ||
            boost::algorithm::iequals(algorithm_str, "HOMOGRAPHY_LMEDS"))
            return OpenCVAlgorithm::HOMOGRAPHY_LMEDS;
        if (boost::algorithm::iequals(algorithm_str, "findHomography_rho") ||
            boost::algorithm::iequals(algorithm_str, "HOMOGRAPHY_RHO"))
            return OpenCVAlgorithm::HOMOGRAPHY_RHO;

        // 默认算法
        std::string warn_msg = LanguageEnvironment::GetText(
            "未知算法: " + algorithm_str + ", 使用默认 findEssentialMat_ransac",
            "Unknown algorithm: " + algorithm_str + ", using default findEssentialMat_ransac");
        LOG_WARNING_ZH << warn_msg;
        LOG_WARNING_EN << warn_msg;
        return OpenCVAlgorithm::ESSENTIAL_RANSAC;
    }

    bool OpenCVTwoViewEstimator::IsRobustAlgorithm(OpenCVAlgorithm algorithm) const
    {
        switch (algorithm)
        {
        case OpenCVAlgorithm::FUNDAMENTAL_RANSAC:
        case OpenCVAlgorithm::FUNDAMENTAL_LMEDS:
        case OpenCVAlgorithm::FUNDAMENTAL_RHO:
        case OpenCVAlgorithm::ESSENTIAL_RANSAC:
        case OpenCVAlgorithm::ESSENTIAL_LMEDS:
        case OpenCVAlgorithm::ESSENTIAL_USAC_DEFAULT:
        case OpenCVAlgorithm::ESSENTIAL_USAC_PARALLEL:
        case OpenCVAlgorithm::ESSENTIAL_USAC_FM_8PTS:
        case OpenCVAlgorithm::ESSENTIAL_USAC_FAST:
        case OpenCVAlgorithm::ESSENTIAL_USAC_ACCURATE:
        case OpenCVAlgorithm::ESSENTIAL_USAC_PROSAC:
        case OpenCVAlgorithm::ESSENTIAL_USAC_MAGSAC:
        case OpenCVAlgorithm::HOMOGRAPHY_RANSAC:
        case OpenCVAlgorithm::HOMOGRAPHY_LMEDS:
        case OpenCVAlgorithm::HOMOGRAPHY_RHO:
            return true;
        default:
            return false;
        }
    }

    size_t OpenCVTwoViewEstimator::GetMinimumSamplesForAlgorithm(OpenCVAlgorithm algorithm) const
    {
        switch (algorithm)
        {
        case OpenCVAlgorithm::FUNDAMENTAL_7POINT:
            return 7;
        case OpenCVAlgorithm::FUNDAMENTAL_8POINT:
        case OpenCVAlgorithm::FUNDAMENTAL_RANSAC:
        case OpenCVAlgorithm::FUNDAMENTAL_LMEDS:
        case OpenCVAlgorithm::FUNDAMENTAL_RHO:
        case OpenCVAlgorithm::ESSENTIAL_USAC_FM_8PTS:
            return 8;
        case OpenCVAlgorithm::ESSENTIAL_RANSAC:
        case OpenCVAlgorithm::ESSENTIAL_LMEDS:
        case OpenCVAlgorithm::ESSENTIAL_USAC_DEFAULT:
        case OpenCVAlgorithm::ESSENTIAL_USAC_PARALLEL:
        case OpenCVAlgorithm::ESSENTIAL_USAC_FAST:
        case OpenCVAlgorithm::ESSENTIAL_USAC_ACCURATE:
        case OpenCVAlgorithm::ESSENTIAL_USAC_PROSAC:
        case OpenCVAlgorithm::ESSENTIAL_USAC_MAGSAC:
            return 5;
        case OpenCVAlgorithm::HOMOGRAPHY_RANSAC:
        case OpenCVAlgorithm::HOMOGRAPHY_LMEDS:
        case OpenCVAlgorithm::HOMOGRAPHY_RHO:
            return 4;
        default:
            return 5;
        }
    }

    cv::Mat OpenCVTwoViewEstimator::EstimateFundamentalMatrix(
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        OpenCVAlgorithm algorithm,
        cv::Mat &inliers_mask)
    {
        int method = GetOpenCVMethodFlag(algorithm);
        double ransac_threshold = GetOptionAsFloat("ransac_threshold", 1.0);
        double confidence = GetOptionAsFloat("confidence", 0.99);
        int max_iterations = GetOptionAsIndexT("max_iterations", 2000);

        cv::Mat fundamental_matrix;

        try
        {
            if (algorithm == OpenCVAlgorithm::FUNDAMENTAL_7POINT ||
                algorithm == OpenCVAlgorithm::FUNDAMENTAL_8POINT)
            {
                // 直接方法，不需要鲁棒估计参数
                fundamental_matrix = cv::findFundamentalMat(points1, points2, method);
            }
            else
            {
                // 鲁棒方法
                fundamental_matrix = cv::findFundamentalMat(
                    points1, points2, method, ransac_threshold, confidence, max_iterations, inliers_mask);
            }
        }
        catch (const cv::Exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "OpenCV findFundamentalMat 错误: ",
                "OpenCV findFundamentalMat error: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
        }

        return fundamental_matrix;
    }

    cv::Mat OpenCVTwoViewEstimator::EstimateEssentialMatrix(
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        const cv::Mat &camera_matrix,
        OpenCVAlgorithm algorithm,
        cv::Mat &inliers_mask)
    {
        int method = GetOpenCVMethodFlag(algorithm);
        double ransac_threshold = GetOptionAsFloat("ransac_threshold", 1.0);
        double confidence = GetOptionAsFloat("confidence", 0.99);
        int max_iterations = GetOptionAsIndexT("max_iterations", 2000);

        cv::Mat essential_matrix;

        try
        {
            // 鲁棒方法 - 修复参数顺序：method, prob, threshold, maxIters, mask
            essential_matrix = cv::findEssentialMat(
                points1, points2, camera_matrix, method, confidence, ransac_threshold, max_iterations, inliers_mask);
        }
        catch (const cv::Exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "OpenCV findEssentialMat 错误: ",
                "OpenCV findEssentialMat error: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
        }

        return essential_matrix;
    }

    cv::Mat OpenCVTwoViewEstimator::EstimateHomography(
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        OpenCVAlgorithm algorithm,
        cv::Mat &inliers_mask)
    {
        int method = GetOpenCVMethodFlag(algorithm);
        double ransac_threshold = GetOptionAsFloat("ransac_threshold", 3.0); // 单应性通常用较大阈值
        double confidence = GetOptionAsFloat("confidence", 0.99);
        int max_iterations = GetOptionAsIndexT("max_iterations", 2000);

        cv::Mat homography;

        try
        {
            // 修复参数顺序：method, ransacReprojThreshold, mask, maxIters, confidence
            homography = cv::findHomography(
                points1, points2, method, ransac_threshold, inliers_mask, max_iterations, confidence);
        }
        catch (const cv::Exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "OpenCV findHomography 错误: ",
                "OpenCV findHomography error: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
        }

        return homography;
    }

    bool OpenCVTwoViewEstimator::RecoverPoseFromEssential(
        const cv::Mat &essential_matrix,
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        const cv::Mat &camera_matrix,
        cv::Mat &R,
        cv::Mat &t,
        cv::Mat &inliers_mask)
    {
        try
        {
            int inliers_count = cv::recoverPose(
                essential_matrix, points1, points2, camera_matrix, R, t, inliers_mask);

            if (log_level_ >= PO_LOG_VERBOSE)
            {
                std::string verbose_msg = LanguageEnvironment::GetText(
                    "recoverPose 内点: " + std::to_string(inliers_count) + " / " + std::to_string(points1.size()),
                    "recoverPose inliers: " + std::to_string(inliers_count) + " / " + std::to_string(points1.size()));
                LOG_DEBUG_ZH << verbose_msg;
                LOG_DEBUG_EN << verbose_msg;
            }

            return inliers_count > 0 && !R.empty() && !t.empty();
        }
        catch (const cv::Exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "OpenCV recoverPose 错误: ",
                "OpenCV recoverPose error: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            return false;
        }
    }

    bool OpenCVTwoViewEstimator::RecoverPoseFromFundamental(
        const cv::Mat &fundamental_matrix,
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        const cv::Mat &camera_matrix1,
        const cv::Mat &camera_matrix2,
        cv::Mat &R,
        cv::Mat &t,
        cv::Mat &inliers_mask)
    {
        try
        {
            // 从基础矩阵恢复本质矩阵
            cv::Mat essential_matrix = camera_matrix2.t() * fundamental_matrix * camera_matrix1;

            // 使用第一个相机的内参进行位姿恢复
            return RecoverPoseFromEssential(essential_matrix, points1, points2, camera_matrix1, R, t, inliers_mask);
        }
        catch (const cv::Exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "从基础矩阵恢复位姿错误: ",
                "Error recovering pose from fundamental matrix: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            return false;
        }
    }

    void OpenCVTwoViewEstimator::UpdateInlierFlags(
        IdMatches &matches,
        const cv::Mat &inliers_mask)
    {
        if (inliers_mask.empty())
            return;

        // 先将所有匹配标记为外点
        for (auto &match : matches)
        {
            match.is_inlier = false;
        }

        // 根据掩码更新内点标记
        for (int i = 0; i < inliers_mask.rows && i < matches.size(); ++i)
        {
            if (inliers_mask.at<uchar>(i, 0) > 0)
            {
                matches[i].is_inlier = true;
            }
        }
    }

    int OpenCVTwoViewEstimator::GetOpenCVMethodFlag(OpenCVAlgorithm algorithm) const
    {
        switch (algorithm)
        {
        // 基础矩阵方法
        case OpenCVAlgorithm::FUNDAMENTAL_7POINT:
            return cv::FM_7POINT;
        case OpenCVAlgorithm::FUNDAMENTAL_8POINT:
            return cv::FM_8POINT;
        case OpenCVAlgorithm::FUNDAMENTAL_RANSAC:
            return cv::FM_RANSAC;
        case OpenCVAlgorithm::FUNDAMENTAL_LMEDS:
            return cv::FM_LMEDS;

        // 本质矩阵和USAC方法
        case OpenCVAlgorithm::ESSENTIAL_RANSAC:
            return cv::RANSAC;
        case OpenCVAlgorithm::ESSENTIAL_LMEDS:
            return cv::LMEDS;
        case OpenCVAlgorithm::ESSENTIAL_USAC_DEFAULT:
            return cv::USAC_DEFAULT;
        case OpenCVAlgorithm::ESSENTIAL_USAC_PARALLEL:
            return cv::USAC_PARALLEL;
        case OpenCVAlgorithm::ESSENTIAL_USAC_FM_8PTS:
            // 注意：USAC_FM_8PTS专用于基础矩阵，对本质矩阵使用USAC_DEFAULT
            return cv::USAC_DEFAULT;
        case OpenCVAlgorithm::ESSENTIAL_USAC_FAST:
            return cv::USAC_FAST;
        case OpenCVAlgorithm::ESSENTIAL_USAC_ACCURATE:
            return cv::USAC_ACCURATE;
        case OpenCVAlgorithm::ESSENTIAL_USAC_PROSAC:
            return cv::USAC_PROSAC;
        case OpenCVAlgorithm::ESSENTIAL_USAC_MAGSAC:
            return cv::USAC_MAGSAC;

        // 单应性方法
        case OpenCVAlgorithm::HOMOGRAPHY_RANSAC:
            return cv::RANSAC;
        case OpenCVAlgorithm::HOMOGRAPHY_LMEDS:
            return cv::LMEDS;

        // RHO方法（基础矩阵和单应性共用）
        case OpenCVAlgorithm::FUNDAMENTAL_RHO:
        case OpenCVAlgorithm::HOMOGRAPHY_RHO:
            return cv::RHO;

        default:
            return cv::RANSAC;
        }
    }

} // namespace PluginMethods

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::OpenCVTwoViewEstimator)
