/**
 * @file barath_two_view_estimator.cpp
 * @brief Barath two-view estimator implementation | Barath双视图估计器实现
 * @details Implementation of two-view pose estimation using MAGSAC, MAGSAC++ and SupeRANSAC algorithms
 * 使用MAGSAC、MAGSAC++和SupeRANSAC算法实现双视图位姿估计
 * @copyright Copyright (c) 2024 XXX Company | Copyright (c) 2024 XXX公司
 */

#include "barath_two_view_estimator.hpp"
#include <opencv2/calib3d.hpp>
#include <limits>
#include <cmath>
#include <iomanip>
#include <random>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <opencv2/core/eigen.hpp>
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统

// MAGSAC and Graph-Cut RANSAC headers
#include "magsac.h"
#include "utils.h"
#include "estimators.h"
#include "samplers/uniform_sampler.h"
#include "samplers/prosac_sampler.h"
#include "samplers/progressive_napsac_sampler.h"
#include "samplers/importance_sampler.h"
#include "samplers/adaptive_reordering_sampler.h"
#include "estimators/solver_essential_matrix_bundle_adjustment.h"
#include "most_similar_inlier_selector.h"

// SupeRANSAC headers (if integrated)
#ifdef HAVE_SUPERANSAC
#include "superansac.h"
#include <Eigen/Dense>

// SuperRANSAC type definition
using DataMatrix = Eigen::MatrixXd;

// SuperRANSAC specific function declarations
extern std::tuple<Eigen::Matrix3d, std::vector<size_t>, double, size_t> estimateEssentialMatrix(
    const DataMatrix &kCorrespondences_,
    const Eigen::Matrix3d &kIntrinsicsSource_,
    const Eigen::Matrix3d &kIntrinsicsDestination_,
    const std::vector<double> &kPointProbabilities_,
    const std::vector<double> &kImageSizes_,
    superansac::RANSACSettings &settings_);
#endif

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace Converter;
    using namespace types;
    using namespace Eigen;

    DataPtr BarathTwoViewEstimator::Run()
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

        // 2. Get view pair information from method_options_ | 2. 从method_options_获取视图对信息
        ViewPair view_pair(
            GetOptionAsIndexT("view_i", 0),
            GetOptionAsIndexT("view_j", 1));
        ViewId view_id1 = view_pair.first;
        ViewId view_id2 = view_pair.second;

        // 3. Determine the algorithm to use | 3. 确定要使用的算法
        const std::string algorithm_str = GetOptionAsString("algorithm", "magsac");
        Algorithm algorithm = CreateAlgorithmFromString(algorithm_str);

        // Get camera intrinsics - using overloaded operator[] for smart camera model access | 获取相机内参 - 使用重载的operator[]支持智能相机模型访问
        Matrix3d K1_eig;
        cv::Mat K1;
        try
        {
            (*cameras_ptr)[view_id1]->GetKMat(K1_eig);
            cv::eigen2cv(K1_eig, K1);
            // std::cout << "K1: " << K1_eig << std::endl;
        }
        catch (const std::out_of_range &e)
        {
            LOG_ERROR_ZH << "无法获取视图 " << view_id1 << " 的相机模型: ";
            LOG_ERROR_ZH << e.what();
            LOG_ERROR_EN << "Failed to get camera model for view " << view_id1 << ": ";
            LOG_ERROR_EN << e.what();
            return nullptr;
        }

        // Extract matching points | 提取匹配点
        IdMatches *matches_ptr = static_cast<IdMatches *>(sample_ptr->GetData());
        if (!matches_ptr)
        {
            LOG_ERROR_ZH << "从样本数据中获取匹配失败。";
            LOG_ERROR_EN << "Failed to get matches from sample data.";
            return nullptr;
        }
        const auto &id_matches = *matches_ptr;
        std::vector<cv::Point2f> points1, points2;
        points1.reserve(id_matches.size());
        points2.reserve(id_matches.size());

        for (const auto &match : id_matches)
        {
            const auto &p1 = (*features_ptr)[view_id1]->GetFeaturePoints()[match.i].GetCoord();
            const auto &p2 = (*features_ptr)[view_id2]->GetFeaturePoints()[match.j].GetCoord();
            points1.emplace_back(p1.x(), p1.y());
            points2.emplace_back(p2.x(), p2.y());
        }

        // Check if there are enough matching points | 检查是否有足够的匹配点
        if (points1.size() < GetMinimumSamples(algorithm))
        {
            LOG_WARNING_ZH << "由于匹配不足而跳过视图对 (" << view_id1 << ", " << view_id2 << "): " << points1.size();
            LOG_WARNING_EN << "Skipping view pair (" << view_id1 << ", " << view_id2 << ") due to insufficient matches: " << points1.size();
            return nullptr;
        }

        std::string algo_name = GetAlgorithmName(algorithm);
        LOG_INFO_ZH << "处理视图对 (" << view_id1 << ", " << view_id2 << ") 使用算法: " << algo_name;
        LOG_INFO_EN << "Processing view pair (" << view_id1 << ", " << view_id2 << ") with algorithm: " << algo_name;

        // 5. Estimate pose using selected algorithm - add timing statistics | 5. 使用选择的算法估计位姿 - 添加时间统计
        cv::Mat E, inliers_mask;

        // Execute core computation with profiling | 执行核心计算并进行性能分析
        bool success = false;
        {
            PROFILER_START_AUTO(enable_profiling_);
            success = EstimateEssentialMatrix(points1, points2, K1, algorithm, E, inliers_mask);
            PROFILER_END();

            // Print profiling statistics | 打印性能分析统计
            if (SHOULD_LOG(DEBUG))
            {
                PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
            }
        }

        if (!success)
        {
            LOG_ERROR_ZH << "对视图对 (" << view_id1 << ", " << view_id2 << ") 的本质矩阵估计失败。";
            LOG_ERROR_EN << "Essential matrix estimation failed for pair (" << view_id1 << ", " << view_id2 << ").";
            return nullptr;
        }

        // Update inlier flags in matching data | 更新匹配数据中的内点标志
        if (matches_ptr && !UpdateInlierFlags(*matches_ptr, inliers_mask))
        {
            LOG_WARNING_ZH << "无法为视图对 (" << view_id1 << ", " << view_id2 << ") 更新内点标志。";
            LOG_WARNING_EN << "Failed to update inlier flags for pair (" << view_id1 << ", " << view_id2 << ").";
        }

        // 6. Recover pose from essential matrix | 6. 从本质矩阵恢复位姿
        cv::Mat R, t;
        int inlier_count = cv::recoverPose(E, points1, points2, K1, R, t, inliers_mask);

        // 7. Save results - output pose in OpenGV format | 7. 保存结果 - 输出OpenGV格式的位姿
        RelativePose rel_pose;
        rel_pose.SetViewIdI(view_id1);
        rel_pose.SetViewIdJ(view_id2);

        Matrix3d R_opencv;
        Vector3d t_opencv;

        cv::cv2eigen(R, R_opencv);
        cv::cv2eigen(t, t_opencv);

        rel_pose.SetRotation(R_opencv.transpose());
        rel_pose.SetTranslation(-R_opencv.transpose() * t_opencv);
        rel_pose.SetWeight(static_cast<float>(inlier_count) / static_cast<float>(points1.size()));

        // Debug output: display coordinate conversion information | 调试输出：显示坐标转换信息
        LOG_DEBUG_ZH << "BarathTwoViewEstimator 坐标转换 for pair (" << view_id1 << ", " << view_id2 << "):";
        LOG_DEBUG_ZH << "OpenCV 格式 (xj = R*xi + t):";
        LOG_DEBUG_ZH << "R_opencv = " << std::endl
                     << R_opencv;
        LOG_DEBUG_ZH << "t_opencv = " << t_opencv.transpose();
        LOG_DEBUG_EN << "BarathTwoViewEstimator coordinate conversion for pair (" << view_id1 << ", " << view_id2 << "):";
        LOG_DEBUG_EN << "OpenCV format (xj = R*xi + t):";
        LOG_DEBUG_EN << "R_opencv = " << std::endl
                     << R_opencv;
        LOG_DEBUG_EN << "t_opencv = " << t_opencv.transpose();

        return std::make_shared<DataMap<RelativePose>>(rel_pose, "data_relative_pose");
    }

    BarathTwoViewEstimator::Algorithm BarathTwoViewEstimator::CreateAlgorithmFromString(const std::string &algorithm_str)
    {
        std::string lower_case_str = boost::to_lower_copy(algorithm_str);
        if (lower_case_str == "magsac")
        {
            return Algorithm::MAGSAC;
        }
        else if (lower_case_str == "magsac++")
        {
            return Algorithm::MAGSAC_PLUS_PLUS;
        }
        else if (lower_case_str == "superansac")
        {
            return Algorithm::SUPERANSAC;
        }
        LOG_WARNING_ZH << "未知算法 '" << algorithm_str << "', 默认使用 MAGSAC。";
        LOG_WARNING_EN << "Unknown algorithm '" << algorithm_str << "', defaulting to MAGSAC.";
        return Algorithm::MAGSAC;
    }

    std::string BarathTwoViewEstimator::GetAlgorithmName(Algorithm algorithm) const
    {
        switch (algorithm)
        {
        case Algorithm::MAGSAC:
            return LanguageEnvironment::GetText("MAGSAC", "MAGSAC");
        case Algorithm::MAGSAC_PLUS_PLUS:
            return LanguageEnvironment::GetText("MAGSAC++", "MAGSAC++");
        case Algorithm::SUPERANSAC:
            return LanguageEnvironment::GetText("SupeRANSAC", "SupeRANSAC");
        default:
            return LanguageEnvironment::GetText("未知", "Unknown");
        }
    }

    size_t BarathTwoViewEstimator::GetMinimumSamples(Algorithm algorithm) const
    {
        // For Essential Matrix estimation, 5 points are required.
        return 5;
    }

    bool BarathTwoViewEstimator::EstimateEssentialMatrix(
        const std::vector<cv::Point2f> &points1,
        const std::vector<cv::Point2f> &points2,
        const cv::Mat &K,
        Algorithm algorithm,
        cv::Mat &E,
        cv::Mat &inliers_mask)
    {
        double prob = GetOptionAsDouble("confidence", 0.99);           // Barath Python默认: 0.99
        double threshold = GetOptionAsDouble("ransac_threshold", 1.0); // Barath Python默认: sigma_th=1.0
        int max_iters = GetOptionAsIndexT("max_iterations", 1000);     // Barath Python默认: 1000
        int min_iters = GetOptionAsIndexT("min_iterations", 50);       // Barath Python默认: 50
        int partition_num = GetOptionAsIndexT("partition_num", 5);     // Barath默认: 5
        int core_number = GetOptionAsIndexT("core_number", 1);         // Barath默认: 1

        if (algorithm == Algorithm::MAGSAC || algorithm == Algorithm::SUPERANSAC)
        {
            return EstimateEssentialMatrixWithMAGSAC(points1, points2, K, algorithm, E, inliers_mask,
                                                     threshold, prob, min_iters, max_iters, partition_num, core_number);
        }
        else if (algorithm == Algorithm::SUPERANSAC)
        {
            // SupeRANSAC doesn't have a direct OpenCV flag. Using a standard robust one for now.
            E = cv::findEssentialMat(points1, points2, K, cv::RANSAC, prob, threshold, max_iters, inliers_mask);
            LOG_WARNING_ZH << "SupeRANSAC 尚未集成。使用 OpenCV 的 RANSAC 作为占位符。";
            LOG_WARNING_EN << "SupeRANSAC is not yet integrated. Using OpenCV's RANSAC as a placeholder.";
        }
        else
        {
            E = cv::findEssentialMat(points1, points2, K, cv::RANSAC, prob, threshold, max_iters, inliers_mask);
        }

        return !E.empty();
    }

    bool BarathTwoViewEstimator::EstimateEssentialMatrixWithMAGSAC(
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
        int core_number)
    {
        try
        {
            // 1. Data Conversion
            cv::Mat points(points1.size(), 4, CV_64F);
            for (size_t i = 0; i < points1.size(); ++i)
            {
                points.at<double>(i, 0) = points1[i].x;
                points.at<double>(i, 1) = points1[i].y;
                points.at<double>(i, 2) = points2[i].x;
                points.at<double>(i, 3) = points2[i].y;
            }

            Eigen::Matrix3d intrinsics_src, intrinsics_dst;
            cv::cv2eigen(K, intrinsics_src);
            intrinsics_dst = intrinsics_src; // Assuming same intrinsics

            // 2. Normalization
            const double &fx1 = intrinsics_src(0, 0);
            const double &fy1 = intrinsics_src(1, 1);
            const double &fx2 = intrinsics_dst(0, 0);
            const double &fy2 = intrinsics_dst(1, 1);

            const double threshold_normalizer = (fx1 + fx2 + fy1 + fy2) / 4.0;
            const double normalized_sigma_max = threshold / threshold_normalizer;

            cv::Mat normalized_points(points.size(), CV_64F);
            gcransac::utils::normalizeCorrespondences(points,
                                                      intrinsics_src,
                                                      intrinsics_dst,
                                                      normalized_points);

            // 3. Estimator and Model
            magsac::utils::DefaultEssentialMatrixEstimator estimator(intrinsics_src, intrinsics_dst);
            gcransac::EssentialMatrix model;

            // 4. MAGSAC object setup
            bool use_magsac_plus_plus = (algorithm == Algorithm::MAGSAC_PLUS_PLUS);
            MAGSAC<cv::Mat, magsac::utils::DefaultEssentialMatrixEstimator> magsac(
                use_magsac_plus_plus ? MAGSAC<cv::Mat, magsac::utils::DefaultEssentialMatrixEstimator>::MAGSAC_PLUS_PLUS : MAGSAC<cv::Mat, magsac::utils::DefaultEssentialMatrixEstimator>::MAGSAC_ORIGINAL);

            magsac.setMaximumThreshold(normalized_sigma_max);
            magsac.setCoreNumber(core_number); // 使用配置的核心数
            magsac.setPartitionNumber(partition_num);
            magsac.setIterationLimit(max_iters);
            magsac.setMinimumIterationNumber(min_iters);
            if (use_magsac_plus_plus)
            {
                magsac.setReferenceThreshold(magsac.getReferenceThreshold() / threshold_normalizer);
            }

            // 5. Sampler setup
            int sampler_id = GetOptionAsIndexT("sampler_id", 4); // Barath Python默认: 4 (AR-Sampler)
            std::unique_ptr<gcransac::sampler::Sampler<cv::Mat, size_t>> main_sampler;
            double imageWidth = GetOptionAsDouble("image_width", 640.0);
            double imageHeight = GetOptionAsDouble("image_height", 480.0);

            // P-NAPSAC parameters
            int pnapsac_layers = GetOptionAsIndexT("pnapsac_layers", 4);
            double pnapsac_blend_ratio = GetOptionAsDouble("pnapsac_blend_ratio", 0.5);

            // AR-Sampler parameters
            double ar_variance = GetOptionAsDouble("ar_variance", 0.1);

            if (sampler_id == 0) // Uniform sampler (均匀采样)
            {
                main_sampler = std::make_unique<gcransac::sampler::UniformSampler>(&points);
                LOG_INFO_ZH << "使用均匀采样器";
                LOG_INFO_EN << "Using Uniform sampler";
            }
            else if (sampler_id == 1) // PROSAC sampler (需要按质量排序的点)
            {
                main_sampler = std::make_unique<gcransac::sampler::ProsacSampler>(&points, estimator.sampleSize());
                LOG_INFO_ZH << "使用 PROSAC 采样器";
                LOG_INFO_EN << "Using PROSAC sampler";
            }
            else if (sampler_id == 2) // Progressive NAPSAC sampler (渐进式邻域感知采样)
            {
                // 根据配置生成网格层
                std::vector<size_t> grid_layers;
                for (int i = 0; i < pnapsac_layers; ++i)
                {
                    grid_layers.push_back(16 >> i); // {16, 8, 4, 2}
                }

                main_sampler = std::make_unique<gcransac::sampler::ProgressiveNapsacSampler<4>>(&points,
                                                                                                grid_layers,
                                                                                                estimator.sampleSize(),
                                                                                                std::vector<double>{imageWidth, imageHeight, imageWidth, imageHeight},
                                                                                                pnapsac_blend_ratio);
                LOG_INFO_ZH << "使用 P-NAPSAC 采样器，层数: " << pnapsac_layers << "，混合比率: " << pnapsac_blend_ratio;
                LOG_INFO_EN << "Using P-NAPSAC sampler with " << pnapsac_layers << " layers, blend ratio: " << pnapsac_blend_ratio;
            }
            else if (sampler_id == 3) // NG-RANSAC sampler (需要内点概率)
            {
                // 为NG-RANSAC生成简单的概率分布 (基于匹配顺序)
                std::vector<double> inlier_probabilities(points1.size());
                for (size_t i = 0; i < points1.size(); ++i)
                {
                    inlier_probabilities[i] = 1.0 - static_cast<double>(i) / points1.size();
                }

                main_sampler = std::make_unique<gcransac::sampler::ImportanceSampler>(&points,
                                                                                      inlier_probabilities,
                                                                                      estimator.sampleSize());

                if (!main_sampler->isInitialized())
                {
                    LOG_WARNING_ZH << "NG-RANSAC 采样器初始化失败，回退到均匀采样器";
                    LOG_WARNING_EN << "NG-RANSAC sampler initialization failed, falling back to Uniform sampler";
                    main_sampler = std::make_unique<gcransac::sampler::UniformSampler>(&points);
                }
                else
                {
                    LOG_INFO_ZH << "使用 NG-RANSAC 采样器与生成的概率";
                    LOG_INFO_EN << "Using NG-RANSAC sampler with generated probabilities";
                }
            }
            else if (sampler_id == 4) // AR-Sampler (自适应重排序采样)
            {
                // 为AR-Sampler生成概率分布并归一化
                std::vector<double> inlier_probabilities(points1.size());
                for (size_t i = 0; i < points1.size(); ++i)
                {
                    inlier_probabilities[i] = 1.0 - static_cast<double>(i) / points1.size();
                }

                // 归一化概率
                double max_prob = *std::max_element(inlier_probabilities.begin(), inlier_probabilities.end());
                if (max_prob > 0)
                {
                    for (auto &prob : inlier_probabilities)
                    {
                        prob /= max_prob;
                    }
                }

                main_sampler = std::make_unique<gcransac::sampler::AdaptiveReorderingSampler>(&points,
                                                                                              inlier_probabilities,
                                                                                              estimator.sampleSize(),
                                                                                              ar_variance);

                if (!main_sampler->isInitialized())
                {
                    LOG_WARNING_ZH << "AR-Sampler 初始化失败，回退到均匀采样器";
                    LOG_WARNING_EN << "AR-Sampler initialization failed, falling back to Uniform sampler";
                    main_sampler = std::make_unique<gcransac::sampler::UniformSampler>(&points);
                }
                else
                {
                    LOG_INFO_ZH << "使用 AR-Sampler，方差: " << ar_variance;
                    LOG_INFO_EN << "Using AR-Sampler with variance: " << ar_variance;
                }
            }
            else
            {
                LOG_WARNING_ZH << "不支持的 sampler_id: " << sampler_id << "。支持的采样器: 0 (Uniform), 1 (PROSAC), 2 (P-NAPSAC), 3 (NG-RANSAC), 4 (AR-Sampler)。默认使用均匀采样器。";
                LOG_WARNING_EN << "Unsupported sampler_id: " << sampler_id << ". Supported samplers: 0 (Uniform), 1 (PROSAC), 2 (P-NAPSAC), 3 (NG-RANSAC), 4 (AR-Sampler). Defaulting to Uniform sampler.";
                main_sampler = std::make_unique<gcransac::sampler::UniformSampler>(&points);
            }

            // 6. Run MAGSAC
            ModelScore score;
            int iterations_run = 0;
            bool success = magsac.run(normalized_points,
                                      confidence,
                                      estimator,
                                      *main_sampler.get(),
                                      model,
                                      iterations_run,
                                      score);

            if (!success)
            {
                LOG_ERROR_ZH << "MAGSAC 本质矩阵估计失败。";
                LOG_ERROR_EN << "MAGSAC essential matrix estimation failed.";
                return false;
            }

            // 7. Process results
            std::vector<size_t> inlier_indices;
            inlier_indices.reserve(points.rows);

            inliers_mask = cv::Mat::zeros(points1.size(), 1, CV_8U);
            int inlier_count = 0;
            for (size_t i = 0; i < points1.size(); ++i)
            {
                double residual = estimator.residual(normalized_points.row(i), model.descriptor);
                if (residual <= normalized_sigma_max)
                {
                    inliers_mask.at<uchar>(i) = 1;
                    inlier_indices.push_back(i);
                    inlier_count++;
                }
            }

            // 8. Bundle Adjustment (可配置)
            bool enable_ba = GetOptionAsBool("enable_bundle_adjustment", true);
            int ba_min_inliers = GetOptionAsIndexT("ba_min_inliers", 6);
            int ba_max_iterations = GetOptionAsIndexT("ba_max_iterations", 100);

            if (enable_ba && inlier_count >= ba_min_inliers)
            {
                LOG_INFO_ZH << "应用 Bundle Adjustment 优化，使用 " << inlier_count << " 个内点 (最小要求: " << ba_min_inliers << ")";
                LOG_INFO_EN << "Applying Bundle Adjustment optimization with " << inlier_count << " inliers (min required: " << ba_min_inliers << ")";

#ifdef HAVE_MAGSAC_BUNDLE_ADJUSTMENT
                try
                {
                    // 创建Bundle Adjustment求解器，设置最大迭代次数
                    gcransac::estimator::solver::EssentialMatrixBundleAdjustmentSolver bundleOptimizer(
                        pose_lib::BundleOptions::LossType::TRUNCATED, ba_max_iterations);

                    // 获取可修改的选项并设置收敛阈值
                    auto &bundle_options = bundleOptimizer.getMutableOptions();
                    bundle_options.max_iterations = ba_max_iterations;
                    // 注意：pose_lib可能没有直接的收敛阈值设置，这里我们只设置迭代次数

                    std::vector<gcransac::Model> models = {model};
                    std::vector<double> weights(inlier_indices.size(), 1.0);

                    bool ba_success = bundleOptimizer.estimateModel(
                        normalized_points,
                        &inlier_indices[0],
                        inlier_indices.size(),
                        models,
                        &weights[0]);

                    if (ba_success && !models.empty())
                    {
                        // 更新模型为BA优化后的结果
                        model.descriptor = models[0].descriptor;
                        LOG_INFO_ZH << "Bundle Adjustment 优化成功完成";
                        LOG_INFO_EN << "Bundle Adjustment optimization completed successfully";
                    }
                    else
                    {
                        LOG_WARNING_ZH << "Bundle Adjustment 优化失败，使用原始 MAGSAC 结果";
                        LOG_WARNING_EN << "Bundle Adjustment optimization failed, using original MAGSAC result";
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_WARNING_ZH << "Bundle Adjustment 异常: " << std::string(e.what()) << "，使用原始 MAGSAC 结果";
                    LOG_WARNING_EN << "Bundle Adjustment exception: " << std::string(e.what()) << ", using original MAGSAC result";
                }
#else
                LOG_WARNING_ZH << "请求了 Bundle Adjustment 但 MAGSAC 库不可用。请安装 MAGSAC 库或设置 enable_bundle_adjustment=false";
                LOG_WARNING_EN << "Bundle Adjustment requested but MAGSAC library not available. Please install MAGSAC library or set enable_bundle_adjustment=false";
#endif
            }
            else if (enable_ba && inlier_count < ba_min_inliers)
            {
                LOG_INFO_ZH << "跳过 Bundle Adjustment: 内点不足 (" << inlier_count << " < " << ba_min_inliers << ")";
                LOG_INFO_EN << "Skipping Bundle Adjustment: insufficient inliers (" << inlier_count << " < " << ba_min_inliers << ")";
            }
            else
            {
                LOG_INFO_ZH << "Bundle Adjustment 被配置禁用";
                LOG_INFO_EN << "Bundle Adjustment disabled by configuration";
            }

            // 9. Adaptive Inlier Selection (可选后处理)
            bool enable_adaptive = GetOptionAsBool("enable_adaptive_inlier_selection", false);
            if (enable_adaptive && inlier_count > 5)
            {
                double adaptive_max_threshold = GetOptionAsDouble("adaptive_max_threshold", 10.0);
                int adaptive_min_inliers = GetOptionAsIndexT("adaptive_min_inliers", 20);

                LOG_INFO_ZH << "应用自适应内点选择，max_threshold=" << adaptive_max_threshold << ", min_inliers=" << adaptive_min_inliers;
                LOG_INFO_EN << "Applying adaptive inlier selection with max_threshold=" << adaptive_max_threshold << ", min_inliers=" << adaptive_min_inliers;

                cv::Mat adaptive_inliers_mask;
                bool adaptive_success = ApplyAdaptiveInlierSelection(
                    normalized_points, model, normalized_sigma_max,
                    adaptive_max_threshold, adaptive_min_inliers,
                    adaptive_inliers_mask);

                if (adaptive_success)
                {
                    int adaptive_inlier_count = cv::countNonZero(adaptive_inliers_mask);
                    LOG_INFO_ZH << "自适应内点选择: " << adaptive_inlier_count << " 个内点 (原为 " << inlier_count << ")";
                    LOG_INFO_EN << "Adaptive inlier selection: " << adaptive_inlier_count << " inliers (was " << inlier_count << ")";

                    // 使用adaptive选择的结果
                    inliers_mask = adaptive_inliers_mask;
                    inlier_count = adaptive_inlier_count;
                }
                else
                {
                    LOG_WARNING_ZH << "自适应内点选择失败，使用原始 MAGSAC 结果";
                    LOG_WARNING_EN << "Adaptive inlier selection failed, using original MAGSAC results";
                }
            }

            // 10. Convert matrix to output
            E = cv::Mat(3, 3, CV_64F);
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    E.at<double>(i, j) = model.descriptor(i, j);
                }
            }

            LOG_INFO_ZH << "MAGSAC 结果: " << inlier_count << "/" << points1.size() << " 个内点。迭代次数: " << iterations_run;
            LOG_INFO_EN << "MAGSAC result: " << inlier_count << "/" << points1.size() << " inliers. Iterations: " << iterations_run;

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "MAGSAC 估计中的异常: ";
            LOG_ERROR_EN << "Exception in MAGSAC estimation: ";
            LOG_ERROR_ZH << e.what();
            LOG_ERROR_EN << e.what();
            return false;
        }
    }

    bool BarathTwoViewEstimator::UpdateInlierFlags(IdMatches &matches, const cv::Mat &inliers_mask)
    {
        if (inliers_mask.rows != static_cast<int>(matches.size()))
        {
            LOG_ERROR_ZH << "内点掩码大小不匹配: " << inliers_mask.rows << " vs " << matches.size();
            LOG_ERROR_EN << "Inlier mask size mismatch: " << inliers_mask.rows << " vs " << matches.size();
            return false;
        }

        for (size_t i = 0; i < matches.size(); ++i)
        {
            matches[i].is_inlier = (inliers_mask.at<uchar>(i) > 0);
        }

        return true;
    }

    bool BarathTwoViewEstimator::ApplyAdaptiveInlierSelection(
        const cv::Mat &normalized_points,
        const gcransac::EssentialMatrix &model,
        double normalized_sigma_max,
        double adaptive_max_threshold,
        int adaptive_min_inliers,
        cv::Mat &adaptive_inliers_mask)
    {
        try
        {
            // 使用MostSimilarInlierSelector进行自适应内点选择
            MostSimilarInlierSelector<magsac::utils::DefaultEssentialMatrixEstimator> inlierSelector(
                std::max(magsac::utils::DefaultEssentialMatrixEstimator::sampleSize() + 1,
                         static_cast<size_t>(adaptive_min_inliers)),
                adaptive_max_threshold);

            // 创建Essential Matrix估计器 (使用单位矩阵，因为点已经归一化)
            magsac::utils::DefaultEssentialMatrixEstimator essentialEstimator(
                Eigen::Matrix3d::Identity(),
                Eigen::Matrix3d::Identity());

            std::vector<size_t> selectedInliers;
            double bestThreshold;

            // 执行自适应内点选择
            inlierSelector.selectInliers(normalized_points,
                                         essentialEstimator,
                                         model,
                                         selectedInliers,
                                         bestThreshold);

            // 转换结果为掩码格式
            adaptive_inliers_mask = cv::Mat::zeros(normalized_points.rows, 1, CV_8U);
            for (const auto &inlierIdx : selectedInliers)
            {
                if (inlierIdx < static_cast<size_t>(normalized_points.rows))
                {
                    adaptive_inliers_mask.at<uchar>(inlierIdx) = 1;
                }
            }

            LOG_INFO_ZH << "自适应内点选择: 找到 " << selectedInliers.size() << " 个内点，最佳阈值: " << bestThreshold;
            LOG_INFO_EN << "Adaptive inlier selection: found " << selectedInliers.size() << " inliers with best threshold: " << bestThreshold;

            return selectedInliers.size() >= static_cast<size_t>(adaptive_min_inliers);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "自适应内点选择中的异常: ";
            LOG_ERROR_EN << "Exception in adaptive inlier selection: ";
            LOG_ERROR_ZH << e.what();
            LOG_ERROR_EN << e.what();
            return false;
        }
    }

} // namespace PluginMethods

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::BarathTwoViewEstimator)
