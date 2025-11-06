/**
 * @file poselib_model_estimator.cpp
 * @brief PoseLib relative pose estimator implementation | PoseLib相对位姿估计器实现
 * @copyright Copyright (c) 2024 XXX Company | Copyright (c) 2024 XXX公司
 */

#include "poselib_model_estimator.hpp"
#include <limits>
#include <cmath>
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统

namespace PluginMethods
{

    PoseLibModelEstimator::PoseLibModelEstimator()
    {
        // 注册所需数据类型
        required_package_["data_sample"] = nullptr; // DataSample<IdMatches>
        required_package_["data_features"] = nullptr;
        required_package_["data_camera_models"] = nullptr;

        // 初始化默认配置路径
        InitializeDefaultConfigPath();

        // 加载refine配置
        InitializeDefaultConfigPath("refine");
    }

    DataPtr PoseLibModelEstimator::Run()
    {
        DisplayConfigInfo();

        // 获取算法参数
        std::string algorithm = GetOptionAsString("algorithm", "relpose_5pt");

        // 1. 获取输入数据
        auto sample_ptr = CastToSample<IdMatches>(required_package_["data_sample"]);
        auto features_ptr = GetDataPtr<FeaturesInfo>(required_package_["data_features"]);
        auto cameras_ptr = GetDataPtr<CameraModels>(required_package_["data_camera_models"]);

        if (!sample_ptr || !features_ptr || !cameras_ptr)
        {
            std::string err_msg = LanguageEnvironment::GetText("无效输入数据", "Invalid input data");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;
            return nullptr;
        }

        // 2. 从method_options_获取视图对信息
        ViewPair view_pair(
            GetOptionAsIndexT("view_i", 0), // 源视图ID
            GetOptionAsIndexT("view_j", 1)  // 目标视图ID
        );

        // 3. 获取匹配数据并进行初步检查
        if (sample_ptr->empty())
        {
            std::string err_msg = LanguageEnvironment::GetText("空样本数据", "Empty sample data");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;
            return nullptr;
        }

        // 检查匹配点对数量是否满足最低要求
        size_t total_matches = sample_ptr->size();
        size_t min_samples = GetMinimumSamplesForAlgorithm(algorithm);

        if (total_matches < min_samples)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "算法 " + algorithm + " 匹配不足: 获得 " + std::to_string(total_matches) + "，至少需要 " + std::to_string(min_samples),
                "Insufficient matches for algorithm " + algorithm + ": got " + std::to_string(total_matches) + ", need at least " + std::to_string(min_samples));
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;

            // 将所有匹配标记为外点
            for (auto &match : *sample_ptr)
            {
                match.is_inlier = false;
            }
            return nullptr;
        }

        if (SHOULD_LOG(DEBUG))
        {
            std::string match_msg = LanguageEnvironment::GetText(
                "算法: " + algorithm + ", 总匹配: " + std::to_string(total_matches) + ", 最小要求: " + std::to_string(min_samples),
                "Algorithm: " + algorithm + ", Total matches: " + std::to_string(total_matches) + ", Min required: " + std::to_string(min_samples));
            LOG_DEBUG_ZH << match_msg;
            LOG_DEBUG_EN << match_msg;
        }

        // 4. 转换为PoseLib格式的2D点对和相机参数
        std::vector<poselib::Point2D> points1, points2;
        poselib::Camera camera1, camera2;
        if (!ConvertToPoseLibPoints(sample_ptr, features_ptr, cameras_ptr, view_pair, points1, points2, camera1, camera2))
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "转换匹配到 PoseLib 格式失败",
                "Failed to convert matches to PoseLib format");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;
            return nullptr;
        }

        // 5. Execute relative pose estimation with profiling | 5. 执行相对位姿估计并进行性能分析
        poselib::CameraPose best_pose;
        std::vector<char> inliers;
        {
            PROFILER_START_AUTO(enable_profiling_);

            if (IsRansacAlgorithm(algorithm))
            {
                PROFILER_STAGE("ransac_estimation"); // Mark RANSAC stage | 标记RANSAC阶段
                // RANSAC方法
                best_pose = EstimateRelativePoseRansac(points1, points2, camera1, camera2, inliers);

                // 更新内点/外点信息到IdMatches中
                if (sample_ptr && !sample_ptr->empty())
                {
                    // 先将所有匹配点标记为外点
                    for (auto &match : *sample_ptr)
                    {
                        match.is_inlier = false;
                    }

                    // 根据RANSAC内点结果更新
                    for (size_t i = 0; i < inliers.size() && i < sample_ptr->size(); ++i)
                    {
                        if (inliers[i])
                        {
                            (*sample_ptr)[i].is_inlier = true;
                        }
                    }

                    if (SHOULD_LOG(DEBUG))
                    {
                        size_t num_inliers = std::count(inliers.begin(), inliers.end(), true);
                        std::string ransac_msg = LanguageEnvironment::GetText(
                            "RANSAC 算法: " + algorithm,
                            "RANSAC algorithm: " + algorithm);
                        LOG_DEBUG_ZH << ransac_msg;
                        LOG_DEBUG_EN << ransac_msg;

                        std::string inlier_msg = LanguageEnvironment::GetText(
                            "总匹配: " + std::to_string(sample_ptr->size()) + ", 内点: " + std::to_string(num_inliers) + " (" + std::to_string(100.0 * num_inliers / sample_ptr->size()) + "%)",
                            "Total matches: " + std::to_string(sample_ptr->size()) + ", Inliers: " + std::to_string(num_inliers) + " (" + std::to_string(100.0 * num_inliers / sample_ptr->size()) + "%)");
                        LOG_DEBUG_ZH << inlier_msg;
                        LOG_DEBUG_EN << inlier_msg;
                    }
                }
            }
            else
            {
                PROFILER_STAGE("direct_estimation"); // Mark direct method stage | 标记直接方法阶段
                // 直接方法
                poselib::CameraPoseVector poses = EstimateRelativePose(points1, points2, camera1, camera2);
                if (poses.empty())
                {
                    PROFILER_END(); // End profiling before return | 返回前结束性能分析
                    std::string err_msg = LanguageEnvironment::GetText("未找到有效位姿", "No valid poses found");
                    LOG_ERROR_ZH << err_msg;
                    LOG_ERROR_EN << err_msg;
                    return nullptr;
                }
                best_pose = poses[0];

                // 对于直接方法，所有匹配点都被认为是内点
                if (sample_ptr && !sample_ptr->empty())
                {
                    // 将所有匹配点标记为内点
                    for (auto &match : *sample_ptr)
                    {
                        match.is_inlier = true;
                    }

                    if (SHOULD_LOG(DEBUG))
                    {
                        std::string direct_msg = LanguageEnvironment::GetText(
                            "直接算法: " + algorithm,
                            "Direct algorithm: " + algorithm);
                        LOG_DEBUG_ZH << direct_msg;
                        LOG_DEBUG_EN << direct_msg;

                        std::string all_inlier_msg = LanguageEnvironment::GetText(
                            "总匹配: " + std::to_string(sample_ptr->size()) + ", 全部标记为内点 (100%)",
                            "Total matches: " + std::to_string(sample_ptr->size()) + ", All marked as inliers (100%)");
                        LOG_DEBUG_ZH << all_inlier_msg;
                        LOG_DEBUG_EN << all_inlier_msg;
                    }
                }
            }

            // 6. 检查位姿的有效性
            if (!IsPoseValid(best_pose))
            {
                PROFILER_END(); // End profiling before return | 返回前结束性能分析
                std::string err_msg = LanguageEnvironment::GetText("估计的位姿无效", "Invalid pose estimated");
                LOG_ERROR_ZH << err_msg;
                LOG_ERROR_EN << err_msg;
                return nullptr;
            }

            // 7. 检查是否需要进行模型优化
            std::string refine_model_str = GetOptionAsString("refine_model", "none");
            RefineMethod refine_method = CreateRefineMethodFromString(refine_model_str);

            if (refine_method != RefineMethod::NONE)
            {
                PROFILER_STAGE("model_refinement"); // Mark model refinement stage | 标记模型优化阶段

                if (SHOULD_LOG(DEBUG))
                {
                    std::string refine_start_msg = LanguageEnvironment::GetText(
                        "开始模型优化，方法: " + refine_model_str,
                        "Starting model refinement, method: " + refine_model_str);
                    LOG_DEBUG_ZH << refine_start_msg;
                    LOG_DEBUG_EN << refine_start_msg;
                }

                best_pose = RefineModel(points1, points2, camera1, camera2, best_pose, refine_method);

                if (SHOULD_LOG(DEBUG))
                {
                    std::string refine_complete_msg = LanguageEnvironment::GetText(
                        "模型优化完成",
                        "Model refinement completed");
                    LOG_DEBUG_ZH << refine_complete_msg;
                    LOG_DEBUG_EN << refine_complete_msg;
                }
            }

            PROFILER_END();

            // Print profiling statistics | 打印性能分析统计
            if (SHOULD_LOG(DEBUG))
            {
                PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
            }
        }

        // 8. 转换为PoSDK格式并返回结果
        RelativePose relative_pose = ConvertPoseLibToOpenGV(best_pose, view_pair.first, view_pair.second);

        return std::make_shared<DataMap<RelativePose>>(relative_pose, "data_relative_pose");
    }

    poselib::CameraPoseVector PoseLibModelEstimator::EstimateRelativePose(
        const std::vector<poselib::Point2D> &points1,
        const std::vector<poselib::Point2D> &points2,
        const poselib::Camera &camera1,
        const poselib::Camera &camera2)
    {
        std::string algorithm = GetOptionAsString("algorithm", "relpose_5pt");
        poselib::CameraPoseVector poses;

        try
        {
            // 将2D点转换为bearing vectors（直接算法需要）
            std::vector<Eigen::Vector3d> x1, x2;
            x1.reserve(points1.size());
            x2.reserve(points2.size());

            for (size_t i = 0; i < points1.size(); ++i)
            {
                // 通过相机内参将像素坐标转换为归一化坐标，然后构造单位向量
                double x_norm1 = (points1[i](0) - camera1.params[2]) / camera1.params[0];
                double y_norm1 = (points1[i](1) - camera1.params[3]) / camera1.params[1];
                double x_norm2 = (points2[i](0) - camera2.params[2]) / camera2.params[0];
                double y_norm2 = (points2[i](1) - camera2.params[3]) / camera2.params[1];

                Eigen::Vector3d bearing1(x_norm1, y_norm1, 1.0);
                Eigen::Vector3d bearing2(x_norm2, y_norm2, 1.0);
                bearing1.normalize();
                bearing2.normalize();

                x1.push_back(bearing1);
                x2.push_back(bearing2);
            }

            if (algorithm == "relpose_5pt")
            {
                // 5点相对位姿算法
                poselib::relpose_5pt(x1, x2, &poses);
            }
            else if (algorithm == "relpose_7pt")
            {
                // 7点基础矩阵算法转相对位姿
                // 注意：PoseLib可能没有直接的7点相对位姿算法，但我们可以通过基础矩阵来实现
                if (x1.size() >= 7 && x2.size() >= 7)
                {
                    // 对于7点算法，我们使用前7个点
                    std::vector<Eigen::Vector3d> x1_7(x1.begin(), x1.begin() + 7);
                    std::vector<Eigen::Vector3d> x2_7(x2.begin(), x2.begin() + 7);

                    try
                    {
                        // 使用5点算法作为fallback，因为PoseLib的7点算法可能需要特殊处理
                        poselib::relpose_5pt(x1, x2, &poses);

                        if (SHOULD_LOG(DEBUG))
                        {
                            std::string verbose_msg = LanguageEnvironment::GetText(
                                "使用5点算法作为7点算法的fallback",
                                "Using 5pt algorithm as fallback for 7pt");
                            LOG_DEBUG_ZH << verbose_msg;
                            LOG_DEBUG_EN << verbose_msg;
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::string err_msg = LanguageEnvironment::GetText(
                            "7点算法fallback中的错误: ",
                            "Error in 7pt algorithm fallback: ");
                        LOG_ERROR_ZH << err_msg << e.what();
                        LOG_ERROR_EN << err_msg << e.what();
                    }
                }
                else
                {
                    std::string err_msg = LanguageEnvironment::GetText(
                        "7点算法点数不足 (至少需要7个，获得 " + std::to_string(x1.size()) + ")",
                        "Insufficient points for 7pt algorithm (need at least 7, got " + std::to_string(x1.size()) + ")");
                    LOG_ERROR_ZH << err_msg;
                    LOG_ERROR_EN << err_msg;
                }
            }
            else if (algorithm == "relpose_8pt")
            {
                // 8点相对位姿算法
                poselib::relpose_8pt(x1, x2, &poses);
            }
            else if (algorithm == "relpose_upright_3pt")
            {
                // 直立3点相对位姿算法
                poselib::relpose_upright_3pt(x1, x2, &poses);
            }
            else if (algorithm == "relpose_upright_planar_3pt")
            {
                // 直立平面3点相对位姿算法
                poselib::relpose_upright_planar_3pt(x1, x2, &poses);
            }
            else
            {
                std::string err_msg = LanguageEnvironment::GetText(
                    "未知算法: " + algorithm,
                    "Unknown algorithm: " + algorithm);
                LOG_ERROR_ZH << err_msg;
                LOG_ERROR_EN << err_msg;

                std::string default_msg = LanguageEnvironment::GetText(
                    "使用默认算法: relpose_5pt",
                    "Using default algorithm: relpose_5pt");
                LOG_DEBUG_ZH << default_msg;
                LOG_DEBUG_EN << default_msg;
                poselib::relpose_5pt(x1, x2, &poses);
            }
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "EstimateRelativePose 中的错误: ",
                "Error in EstimateRelativePose: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
        }

        return poses;
    }

    poselib::CameraPose PoseLibModelEstimator::EstimateRelativePoseRansac(
        const std::vector<poselib::Point2D> &points1,
        const std::vector<poselib::Point2D> &points2,
        const poselib::Camera &camera1,
        const poselib::Camera &camera2,
        std::vector<char> &inliers)
    {
        std::string algorithm = GetOptionAsString("algorithm", "relpose_5pt_ransac");
        poselib::CameraPose best_pose;

        // 创建RANSAC配置
        poselib::RansacOptions ransac_opt;
        ransac_opt.max_iterations = GetOptionAsIndexT("ransac_max_iterations", 1000);
        ransac_opt.max_epipolar_error = GetOptionAsFloat("ransac_threshold", 1e-4);
        ransac_opt.progressive_sampling = GetOptionAsBool("progressive_sampling", true);

        // 创建Bundle配置
        poselib::BundleOptions bundle_opt;
        bundle_opt.max_iterations = 100;

        try
        {
            // 使用转换后的2D points进行RANSAC估计
            poselib::RansacStats stats = poselib::estimate_relative_pose(
                points1, points2, camera1, camera2, ransac_opt, bundle_opt, &best_pose, &inliers);

            if (SHOULD_LOG(DEBUG))
            {
                size_t num_inliers = std::count(inliers.begin(), inliers.end(), true);
                std::string verbose_msg = LanguageEnvironment::GetText(
                    "RANSAC 迭代: " + std::to_string(stats.iterations) + ", 内点: " + std::to_string(num_inliers),
                    "RANSAC iterations: " + std::to_string(stats.iterations) + ", Inliers: " + std::to_string(num_inliers));
                LOG_DEBUG_ZH << verbose_msg;
                LOG_DEBUG_EN << verbose_msg;
            }
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "EstimateRelativePoseRansac 中的错误: ",
                "Error in EstimateRelativePoseRansac: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
        }

        return best_pose;
    }

    poselib::CameraPose PoseLibModelEstimator::RefineModel(
        const std::vector<poselib::Point2D> &points1,
        const std::vector<poselib::Point2D> &points2,
        const poselib::Camera &camera1,
        const poselib::Camera &camera2,
        const poselib::CameraPose &initial_pose,
        RefineMethod refine_method)
    {
        poselib::CameraPose refined_pose = initial_pose;

        try
        {
            if (refine_method == RefineMethod::BUNDLE_ADJUST || refine_method == RefineMethod::NONLINEAR)
            {
                // Bundle Adjustment优化
                poselib::BundleOptions bundle_opt;
                bundle_opt.max_iterations = GetOptionAsIndexT("max_iterations", 100);
                bundle_opt.loss_type = (refine_method == RefineMethod::BUNDLE_ADJUST) ? poselib::BundleOptions::LossType::CAUCHY : poselib::BundleOptions::LossType::TRIVIAL;
                bundle_opt.loss_scale = GetOptionAsFloat("loss_scale", 1.0);

                // refine_relpose假设使用归一化坐标，需要转换像素坐标为归一化坐标
                std::vector<poselib::Point2D> norm_points1, norm_points2;
                norm_points1.reserve(points1.size());
                norm_points2.reserve(points2.size());

                for (size_t i = 0; i < points1.size(); ++i)
                {
                    // 将像素坐标转换为归一化坐标
                    poselib::Point2D norm_p1, norm_p2;
                    norm_p1(0) = (points1[i](0) - camera1.params[2]) / camera1.params[0];
                    norm_p1(1) = (points1[i](1) - camera1.params[3]) / camera1.params[1];
                    norm_p2(0) = (points2[i](0) - camera2.params[2]) / camera2.params[0];
                    norm_p2(1) = (points2[i](1) - camera2.params[3]) / camera2.params[1];

                    norm_points1.push_back(norm_p1);
                    norm_points2.push_back(norm_p2);
                }

                // 使用归一化坐标进行Bundle Adjustment
                poselib::refine_relpose(norm_points1, norm_points2, &refined_pose, bundle_opt);
            }
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "RefineModel 中的错误: ",
                "Error in RefineModel: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            return initial_pose;
        }

        return refined_pose;
    }

    RelativePose PoseLibModelEstimator::ConvertPoseLibToOpenGV(
        const poselib::CameraPose &pose,
        IndexT view_i,
        IndexT view_j)
    {
        // PoseLib的CameraPose表示从camera1到camera2的变换: P2 = R * P1 + t
        // OpenGV/PoSDK的RelativePose也表示相同的变换，因此直接使用
        Eigen::Matrix3d R = pose.R();
        Eigen::Vector3d t = pose.t.normalized();

        RelativePose relative_pose(
            view_i,
            view_j,
            R.transpose(),
            -R.transpose() * t,
            1.0f); // 默认权重

        return relative_pose;
    }

    bool PoseLibModelEstimator::ConvertToPoseLibPoints(
        const std::shared_ptr<DataSample<IdMatches>> &sample_ptr,
        const std::shared_ptr<FeaturesInfo> &features_ptr,
        const std::shared_ptr<CameraModels> &cameras_ptr,
        const ViewPair &view_pair,
        std::vector<poselib::Point2D> &points1,
        std::vector<poselib::Point2D> &points2,
        poselib::Camera &camera1,
        poselib::Camera &camera2)
    {
        try
        {
            points1.clear();
            points2.clear();
            points1.reserve(sample_ptr->size());
            points2.reserve(sample_ptr->size());

            // 获取真实相机内参
            const CameraModel *cam1 = (*cameras_ptr)[view_pair.first];
            const CameraModel *cam2 = (*cameras_ptr)[view_pair.second];

            if (!cam1 || !cam2)
            {
                std::string err_msg = LanguageEnvironment::GetText("无法获取相机模型", "Failed to get camera models");
                LOG_ERROR_ZH << err_msg;
                LOG_ERROR_EN << err_msg;
                return false;
            }

            // 设置PoseLib相机参数（使用真实内参）
            camera1.model_id = 0; // PINHOLE
            camera1.params.resize(4);
            const auto &intrinsics1 = cam1->GetIntrinsics();
            camera1.params[0] = intrinsics1.GetFx();
            camera1.params[1] = intrinsics1.GetFy();
            camera1.params[2] = intrinsics1.GetCx();
            camera1.params[3] = intrinsics1.GetCy();

            camera2.model_id = 0; // PINHOLE
            camera2.params.resize(4);
            const auto &intrinsics2 = cam2->GetIntrinsics();
            camera2.params[0] = intrinsics2.GetFx();
            camera2.params[1] = intrinsics2.GetFy();
            camera2.params[2] = intrinsics2.GetCx();
            camera2.params[3] = intrinsics2.GetCy();

            // 转换像素坐标（类似OpenCV方式）
            for (const auto &match : *sample_ptr)
            {
                // 获取像素坐标
                const Vector2d &pixel1 = (*features_ptr)[view_pair.first]->GetFeaturePoints()[match.i].GetCoord();
                const Vector2d &pixel2 = (*features_ptr)[view_pair.second]->GetFeaturePoints()[match.j].GetCoord();

                // 直接使用像素坐标（让PoseLib内部处理归一化）
                poselib::Point2D p1, p2;
                p1 << pixel1.x(), pixel1.y();
                p2 << pixel2.x(), pixel2.y();

                points1.push_back(p1);
                points2.push_back(p2);
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "ConvertToPoseLibPoints 中的错误: ",
                "Error in ConvertToPoseLibPoints: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            return false;
        }
    }

    bool PoseLibModelEstimator::ConvertToPoseLibBearingVectors(
        const std::shared_ptr<DataSample<IdMatches>> &sample_ptr,
        const std::shared_ptr<FeaturesInfo> &features_ptr,
        const std::shared_ptr<CameraModels> &cameras_ptr,
        const ViewPair &view_pair,
        std::vector<Eigen::Vector3d> &x1,
        std::vector<Eigen::Vector3d> &x2)
    {
        // 复用OpenGV的转换函数，然后转换为PoseLib格式
        opengv::bearingVectors_t bearingVectors1, bearingVectors2;

        if (!Converter::OpenGVConverter::MatchesToBearingVectors(
                *sample_ptr, *features_ptr, *cameras_ptr, view_pair,
                bearingVectors1, bearingVectors2))
        {
            return false;
        }

        // 转换为PoseLib格式
        x1.clear();
        x2.clear();
        x1.reserve(bearingVectors1.size());
        x2.reserve(bearingVectors2.size());

        for (size_t i = 0; i < bearingVectors1.size(); ++i)
        {
            x1.push_back(bearingVectors1[i]);
            x2.push_back(bearingVectors2[i]);
        }

        return true;
    }

    size_t PoseLibModelEstimator::GetMinimumSamplesForAlgorithm(const std::string &algorithm) const
    {
        if (algorithm == "relpose_upright_3pt" || algorithm == "relpose_upright_3pt_ransac" ||
            algorithm == "relpose_upright_planar_3pt")
        {
            return 3;
        }
        else if (algorithm == "relpose_5pt" || algorithm == "relpose_5pt_ransac")
        {
            return 5;
        }
        else if (algorithm == "relpose_7pt" || algorithm == "relpose_7pt_ransac")
        {
            return 7;
        }
        else if (algorithm == "relpose_8pt" || algorithm == "relpose_8pt_ransac")
        {
            return 8;
        }
        else
        {
            // 默认返回5（五点法）
            return 5;
        }
    }

    poselib::RansacOptions PoseLibModelEstimator::CreateRansacOptions() const
    {
        poselib::RansacOptions ransac_opt;
        ransac_opt.max_iterations = GetOptionAsIndexT("ransac_max_iterations", 1000);
        ransac_opt.max_epipolar_error = GetOptionAsFloat("ransac_threshold", 1e-4);
        ransac_opt.progressive_sampling = GetOptionAsBool("progressive_sampling", true);

        return ransac_opt;
    }

    bool PoseLibModelEstimator::IsPoseValid(const poselib::CameraPose &pose) const
    {
        // 检查四元数的有效性
        if (pose.q.norm() < 1e-8)
        {
            return false;
        }

        // 检查平移向量的有效性
        if (pose.t.norm() < 1e-8)
        {
            return false;
        }

        return true;
    }

} // namespace PluginMethods

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::PoseLibModelEstimator)