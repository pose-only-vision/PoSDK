/**
 * @file opengv_model_estimator.cpp
 * @brief OpenGV relative pose estimator implementation | OpenGV相对位姿估计器实现
 * @copyright Copyright (c) 2024 XXX Company | Copyright (c) 2024 XXX公司
 */

#include "opengv_model_estimator.hpp"
#include <opengv/relative_pose/methods.hpp>
#include <opengv/triangulation/methods.hpp>
#include <limits>
#include <cmath>
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统

namespace PluginMethods
{

    OpenGVModelEstimator::OpenGVModelEstimator()
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

    DataPtr OpenGVModelEstimator::Run()
    {
        LOG_DEBUG_ZH << "OpenGV 模型估计器: 调试输出已启用";
        LOG_DEBUG_EN << "OpenGV Model Estimator: Debug output is enabled";
        // DisplayConfigInfo();

        std::string algorithm = GetOptionAsString("algorithm", "fivept_stewenius");
        std::string algo_msg = LanguageEnvironment::GetText(
            "OpenGV 模型估计器 - 来自选项的算法: " + algorithm,
            "OpenGV Model Estimator - Algorithm from options: " + algorithm);
        LOG_DEBUG_ZH << algo_msg;
        LOG_DEBUG_EN << algo_msg;

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

        // 3.1 检查匹配点对数量是否满足最低要求
        size_t total_matches = sample_ptr->size();

        if (total_matches < GetMinimumSamplesForAlgorithm(algorithm))
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "算法 " + algorithm + " 匹配不足: 获得 " + std::to_string(total_matches) + "，至少需要 " + std::to_string(GetMinimumSamplesForAlgorithm(algorithm)),
                "Insufficient matches for algorithm " + algorithm + ": got " + std::to_string(total_matches) + ", need at least " + std::to_string(GetMinimumSamplesForAlgorithm(algorithm)));
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
                "算法: " + algorithm + ", 总匹配: " + std::to_string(total_matches) + ", 最小要求: " + std::to_string(GetMinimumSamplesForAlgorithm(algorithm)),
                "Algorithm: " + algorithm + ", Total matches: " + std::to_string(total_matches) + ", Min required: " + std::to_string(GetMinimumSamplesForAlgorithm(algorithm)));
            LOG_DEBUG_ZH << match_msg;
            LOG_DEBUG_EN << match_msg;
        }

        // 4. 创建OpenGV适配器
        opengv::bearingVectors_t bearingVectors1, bearingVectors2;

        if (!Converter::OpenGVConverter::MatchesToBearingVectors(
                *sample_ptr, *features_ptr, *cameras_ptr, view_pair,
                bearingVectors1, bearingVectors2))
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "转换匹配到 bearing vectors 失败",
                "Failed to convert matches to bearing vectors");
            LOG_ERROR_ZH << err_msg;
            LOG_ERROR_EN << err_msg;
            return nullptr;
        }

        // 创建适配器
        opengv::relative_pose::CentralRelativeAdapter adapter(
            bearingVectors1,
            bearingVectors2);

        // 5. 分配先验信息
        AssignPriorInfo(adapter);

        // 6. Execute relative pose estimation with profiling | 6. 执行相对位姿估计并进行性能分析
        transformation_t transformation;
        std::vector<int> inliers;
        {
            PROFILER_START_AUTO(enable_profiling_);

            if (IsRansacAlgorithm(algorithm))
            {
                PROFILER_STAGE("ransac_estimation"); // Mark RANSAC stage | 标记RANSAC阶段
                // RANSAC方法
                transformation = EstimateRelativePoseRansac(adapter, inliers);

                // 注意: 质量验证功能已移至TwoViewEstimator统一管控
                // 这里只负责算法执行和内点标记，质量检查由上层框架处理

                // 更新内点/外点信息到IdMatches中
                if (sample_ptr && !sample_ptr->empty())
                {
                    // 先将所有匹配点标记为外点
                    for (auto &match : *sample_ptr)
                    {
                        match.is_inlier = false;
                    }

                    // 根据RANSAC内点结果更新
                    for (const auto &inlier_idx : inliers)
                    {
                        if (inlier_idx < sample_ptr->size())
                        {
                            (*sample_ptr)[inlier_idx].is_inlier = true;
                        }
                    }

                    if (SHOULD_LOG(DEBUG))
                    {
                        std::string ransac_msg = LanguageEnvironment::GetText(
                            "RANSAC 算法: " + algorithm,
                            "RANSAC algorithm: " + algorithm);
                        LOG_DEBUG_ZH << ransac_msg;
                        LOG_DEBUG_EN << ransac_msg;

                        std::string inlier_msg = LanguageEnvironment::GetText(
                            "总匹配: " + std::to_string(sample_ptr->size()) + ", 内点: " + std::to_string(inliers.size()) + " (" + std::to_string(100.0 * inliers.size() / sample_ptr->size()) + "%)",
                            "Total matches: " + std::to_string(sample_ptr->size()) + ", Inliers: " + std::to_string(inliers.size()) + " (" + std::to_string(100.0 * inliers.size() / sample_ptr->size()) + "%)");
                        LOG_DEBUG_ZH << inlier_msg;
                        LOG_DEBUG_EN << inlier_msg;

                        // 调试：验证内点标记是否正确设置
                        int marked_inliers = 0;
                        for (const auto &match : *sample_ptr)
                        {
                            if (match.is_inlier)
                                marked_inliers++;
                        }
                        std::string marked_msg = LanguageEnvironment::GetText(
                            "在 sample_ptr 中标记的内点: " + std::to_string(marked_inliers),
                            "Marked inliers in sample_ptr: " + std::to_string(marked_inliers));
                        LOG_DEBUG_ZH << marked_msg;
                        LOG_DEBUG_EN << marked_msg;
                    }
                }
            }
            else
            {
                PROFILER_STAGE("direct_estimation"); // Mark direct method stage | 标记直接方法阶段
                // 直接方法
                transformation = EstimateRelativePose(adapter);

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

            // 判断是否有有效输出
            if (transformation.block<3, 3>(0, 0).determinant() < 1e-6)
            {
                PROFILER_END(); // End profiling before return | 返回前结束性能分析
                std::string err_msg = LanguageEnvironment::GetText(
                    "无效变换: 行列式 < 1e-6",
                    "Invalid transformation: determinant < 1e-6");
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

                transformation = RefineModel(adapter, transformation, refine_method);

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

        // 8. opengv输出的是Rji, tji, 需要转换为Rij, tij
        // Matrix3d Rij = transformation.block<3, 3>(0, 0).transpose();
        // Vector3d tij = -Rij * transformation.block<3, 1>(0, 3);

        // 9. 创建并返回结果
        RelativePose relative_pose(
            view_pair.first,
            view_pair.second,
            transformation.block<3, 3>(0, 0),
            transformation.block<3, 1>(0, 3),
            1.0f);

        return std::make_shared<DataMap<RelativePose>>(relative_pose, "data_relative_pose");
    }

    transformation_t OpenGVModelEstimator::EstimateRelativePose(
        opengv::relative_pose::CentralRelativeAdapter &adapter)
    {
        // 获取算法类型
        auto algorithm = GetOptionAsString("algorithm", "fivept_stewenius");

        opengv::transformation_t best_transformation = opengv::transformation_t::Zero();

        try
        {
            if (algorithm == "twopt")
            {
                // 两点法估计平移 (需要先验R)
                if (prior_info_.find("R_prior") == prior_info_.end())
                {
                    std::string err_msg = LanguageEnvironment::GetText(
                        "twopt 算法需要先验旋转矩阵",
                        "Prior rotation matrix is required for twopt algorithm");
                    LOG_ERROR_ZH << err_msg;
                    LOG_ERROR_EN << err_msg;
                    return best_transformation;
                }
                best_transformation.block<3, 1>(0, 3) = opengv::relative_pose::twopt(adapter, false);
                best_transformation.block<3, 3>(0, 0) = adapter.getR12();
            }
            else if (algorithm == "twopt_rotationOnly")
            {
                // 两点法仅估计旋转
                best_transformation.block<3, 3>(0, 0) = opengv::relative_pose::twopt_rotationOnly(adapter);
                best_transformation.block<3, 1>(0, 3) = adapter.gett12();
            }
            else if (algorithm == "rotationOnly")
            {
                // 仅估计旋转 (使用Arun方法)
                best_transformation.block<3, 3>(0, 0) = opengv::relative_pose::rotationOnly(adapter);
                best_transformation.block<3, 1>(0, 3) = adapter.gett12();
            }
            else if (algorithm == "fivept_stewenius")
            {
                opengv::complexEssentials_t complex_essentials =
                    opengv::relative_pose::fivept_stewenius(adapter);

                if (!complex_essentials.empty())
                {
                    // 转换complex essentials到real essentials
                    opengv::essentials_t essentials;
                    for (const auto &E_complex : complex_essentials)
                    {
                        if (E_complex.imag().norm() < 1e-10)
                        {
                            opengv::essential_t E_real = E_complex.real();
                            essentials.push_back(E_real);
                        }
                    }

                    if (!essentials.empty())
                    {
                        best_transformation = GetBestTransformationFromEssentials(adapter, essentials);
                    }
                }
            }
            else if (algorithm == "fivept_nister")
            {
                opengv::essentials_t essentials = opengv::relative_pose::fivept_nister(adapter);
                if (!essentials.empty())
                {
                    best_transformation = GetBestTransformationFromEssentials(adapter, essentials);
                }
            }
            else if (algorithm == "fivept_kneip")
            {
                // Kneip 5点法需要5个点的索引
                std::vector<int> indices5;
                for (int i = 0; i < 5; i++)
                    indices5.push_back(i);

                // 使用这5个点计算rotations
                auto rotations = opengv::relative_pose::fivept_kneip(adapter, indices5);

                if (!rotations.empty())
                {
                    best_transformation = GetBestTransformationFromRotations(adapter, rotations);
                }
            }
            else if (algorithm == "sevenpt")
            {
                // 7点法
                opengv::essentials_t essentials = opengv::relative_pose::sevenpt(adapter);
                if (!essentials.empty())
                {
                    best_transformation = GetBestTransformationFromEssentials(adapter, essentials);
                }
            }
            else if (algorithm == "eightpt")
            {
                // 8点法
                opengv::essential_t essential = opengv::relative_pose::eightpt(adapter);
                opengv::essentials_t essentials;
                essentials.push_back(essential);
                best_transformation = GetBestTransformationFromEssentials(adapter, essentials);
            }
            else if (algorithm == "eigensolver")
            {
                // 特征值分解法 (仅估计旋转)
                best_transformation.block<3, 3>(0, 0) = opengv::relative_pose::eigensolver(adapter);
                best_transformation.block<3, 1>(0, 3) = adapter.gett12();
            }
            else if (algorithm == "rel_nonlin_central")
            {
                // 中心相对位姿非线性优化
                best_transformation = opengv::relative_pose::optimize_nonlinear(adapter);
            }
            else
            {
                std::string err_msg = LanguageEnvironment::GetText(
                    "未知算法: " + algorithm,
                    "Unknown algorithm: " + algorithm);
                LOG_ERROR_ZH << err_msg;
                LOG_ERROR_EN << err_msg;

                std::string default_msg = LanguageEnvironment::GetText(
                    "使用默认算法: fivept_stewenius",
                    "Using default algorithm: fivept_stewenius");
                LOG_DEBUG_ZH << default_msg;
                LOG_DEBUG_EN << default_msg;

                // 默认采用stewenius五点法
                opengv::complexEssentials_t complex_essentials =
                    opengv::relative_pose::fivept_stewenius(adapter);

                if (!complex_essentials.empty())
                {
                    // 转换complex essentials到real essentials
                    opengv::essentials_t essentials;
                    for (const auto &E_complex : complex_essentials)
                    {
                        if (E_complex.imag().norm() < 1e-10)
                        {
                            opengv::essential_t E_real = E_complex.real();
                            essentials.push_back(E_real);
                        }
                    }

                    if (!essentials.empty())
                    {
                        best_transformation = GetBestTransformationFromEssentials(adapter, essentials);
                    }
                }
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

        return best_transformation;
    }

    transformation_t OpenGVModelEstimator::EstimateRelativePoseRansac(
        opengv::relative_pose::CentralRelativeAdapter &adapter,
        std::vector<int> &inliers)
    {
        // 获取算法类型
        std::string algorithm = GetOptionAsString("algorithm", "fivept_stewenius_ransac");

        // 配置RANSAC参数
        double ransac_threshold = GetOptionAsFloat("ransac_threshold", 2.0 * (1.0 - cos(atan(sqrt(2.0) * 0.5 / 800.0))));
        int max_iterations = GetOptionAsIndexT("ransac_max_iterations", 50);

        opengv::transformation_t result_transformation = opengv::transformation_t::Zero();
        inliers.clear();

        try
        {
            if (algorithm == "rotationOnly_ransac")
            {
                // 仅旋转RANSAC
                typedef opengv::sac_problems::relative_pose::RotationOnlySacProblem rotRansac;
                std::shared_ptr<rotRansac> problem(new rotRansac(adapter));

                opengv::sac::Ransac<rotRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                // 构造变换矩阵 (仅旋转)
                result_transformation.block<3, 3>(0, 0) = ransac.model_coefficients_;
                result_transformation.block<3, 1>(0, 3) = adapter.gett12();
                inliers = ransac.inliers_;
            }
            else if (algorithm == "fivept_stewenius_ransac")
            {
                // Stewenius五点法RANSAC
                typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem relRansac;
                std::shared_ptr<relRansac> problem(new relRansac(adapter, relRansac::STEWENIUS));

                opengv::sac::Ransac<relRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                result_transformation = ransac.model_coefficients_;
                inliers = ransac.inliers_;
            }
            else if (algorithm == "fivept_nister_ransac")
            {
                // Nister五点法RANSAC
                typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem relRansac;
                std::shared_ptr<relRansac> problem(new relRansac(adapter, relRansac::NISTER));

                opengv::sac::Ransac<relRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                result_transformation = ransac.model_coefficients_;
                inliers = ransac.inliers_;
            }
            else if (algorithm == "sevenpt_ransac")
            {
                // 七点法RANSAC
                typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem relRansac;
                std::shared_ptr<relRansac> problem(new relRansac(adapter, relRansac::SEVENPT));

                opengv::sac::Ransac<relRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                result_transformation = ransac.model_coefficients_;
                inliers = ransac.inliers_;
            }
            else if (algorithm == "eightpt_ransac")
            {
                // 八点法RANSAC
                typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem relRansac;
                std::shared_ptr<relRansac> problem(new relRansac(adapter, relRansac::EIGHTPT));

                opengv::sac::Ransac<relRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                result_transformation = ransac.model_coefficients_;
                inliers = ransac.inliers_;
            }
            else if (algorithm == "eigensolver_ransac")
            {
                // 特征值分解法RANSAC
                typedef opengv::sac_problems::relative_pose::EigensolverSacProblem eigRansac;
                std::shared_ptr<eigRansac> problem(new eigRansac(adapter, 10));

                opengv::sac::Ransac<eigRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                // 构造变换矩阵
                result_transformation.block<3, 3>(0, 0) = ransac.model_coefficients_.rotation;
                result_transformation.block<3, 1>(0, 3) = ransac.model_coefficients_.translation;
                inliers = ransac.inliers_;
            }
            else
            {
                std::string warn_msg = LanguageEnvironment::GetText(
                    "未知 RANSAC 算法: " + algorithm + ", 使用默认 fivept_stewenius_ransac",
                    "Unknown RANSAC algorithm: " + algorithm + ", using default fivept_stewenius_ransac");
                LOG_WARNING_ZH << warn_msg;
                LOG_WARNING_EN << warn_msg;

                typedef opengv::sac_problems::relative_pose::CentralRelativePoseSacProblem relRansac;
                std::shared_ptr<relRansac> problem(new relRansac(adapter, relRansac::STEWENIUS));

                opengv::sac::Ransac<relRansac> ransac;
                ransac.sac_model_ = problem;
                ransac.threshold_ = ransac_threshold;
                ransac.max_iterations_ = max_iterations;
                ransac.computeModel();

                result_transformation = ransac.model_coefficients_;
                inliers = ransac.inliers_;
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

        return result_transformation;
    }

    // 辅助函数:从Essential矩阵集合中选择最佳变换
    opengv::transformation_t OpenGVModelEstimator::GetBestTransformationFromEssentials(
        opengv::relative_pose::CentralRelativeAdapter &adapter,
        const opengv::essentials_t &essentialMatrices)
    {

        // 初始化W矩阵
        Eigen::Matrix3d W = Eigen::Matrix3d::Zero();
        W(0, 1) = -1;
        W(1, 0) = 1;
        W(2, 2) = 1;

        double bestQuality = 1000000.0;
        int bestQualityIndex = -1;
        int bestQualitySubindex = -1;

        // 遍历所有本质矩阵
        for (size_t i = 0; i < essentialMatrices.size(); i++)
        {
            // SVD分解
            Eigen::MatrixXd tempEssential = essentialMatrices[i];
            Eigen::JacobiSVD<Eigen::MatrixXd> SVD(
                tempEssential,
                Eigen::ComputeFullV | Eigen::ComputeFullU);
            Eigen::VectorXd singularValues = SVD.singularValues();

            // 检查本质矩阵的有效性（保留警告但不跳过）
            if (singularValues[2] > 0.001)
            {
                std::string warn_msg = LanguageEnvironment::GetText(
                    "坏本质矩阵 (奇异值约束)",
                    "Bad essential matrix (singular value constraint)");
                LOG_WARNING_ZH << warn_msg;
                LOG_WARNING_EN << warn_msg;
            }
            if (singularValues[1] < 0.75 * singularValues[0])
            {
                std::string warn_msg = LanguageEnvironment::GetText(
                    "坏本质矩阵 (中间奇异值约束)",
                    "Bad essential matrix (middle singular value constraint)");
                LOG_WARNING_ZH << warn_msg;
                LOG_WARNING_EN << warn_msg;
            }

            // 保持尺度
            double scale = singularValues[0];

            // 获取可能的R和t
            rotation_t Ra = SVD.matrixU() * W * SVD.matrixV().transpose();
            rotation_t Rb = SVD.matrixU() * W.transpose() * SVD.matrixV().transpose();
            translation_t ta = scale * SVD.matrixU().col(2);
            translation_t tb = -ta;

            // 确保行列式为1
            if (Ra.determinant() < 0)
                Ra = -Ra;
            if (Rb.determinant() < 0)
                Rb = -Rb;

            // 构造四种可能的变换
            transformations_t transformations;
            transformation_t transformation;

            transformation.col(3) = ta;
            transformation.block<3, 3>(0, 0) = Ra;
            transformations.push_back(transformation);

            transformation.col(3) = ta;
            transformation.block<3, 3>(0, 0) = Rb;
            transformations.push_back(transformation);

            transformation.col(3) = tb;
            transformation.block<3, 3>(0, 0) = Ra;
            transformations.push_back(transformation);

            transformation.col(3) = tb;
            transformation.block<3, 3>(0, 0) = Rb;
            transformations.push_back(transformation);

            // 计算逆变换
            transformations_t inverseTransformations;
            for (size_t j = 0; j < 4; j++)
            {
                transformation_t inverseTransformation;
                inverseTransformation.block<3, 3>(0, 0) =
                    transformations[j].block<3, 3>(0, 0).transpose();
                inverseTransformation.col(3) =
                    -inverseTransformation.block<3, 3>(0, 0) * transformations[j].col(3);
                inverseTransformations.push_back(inverseTransformation);
            }

            // 评估四个解
            Eigen::Matrix<double, 4, 1> p_hom;
            p_hom[3] = 1.0;

            for (size_t j = 0; j < 4; j++)
            {
                adapter.setR12(transformations[j].block<3, 3>(0, 0));
                adapter.sett12(transformations[j].col(3));

                double quality = 0.0;

                for (size_t k = 0; k < adapter.getNumberCorrespondences(); k++)
                {
                    // 三角化点
                    p_hom.block<3, 1>(0, 0) = opengv::triangulation::triangulate2(adapter, k);

                    // 计算重投影
                    bearingVector_t reprojection1 = p_hom.block<3, 1>(0, 0);
                    bearingVector_t reprojection2 = inverseTransformations[j] * p_hom;

                    // 归一化重投影向量
                    reprojection1 = reprojection1 / reprojection1.norm();
                    reprojection2 = reprojection2 / reprojection2.norm();

                    // 获取原始特征向量
                    bearingVector_t f1 = adapter.getBearingVector1(k);
                    bearingVector_t f2 = adapter.getBearingVector2(k);

                    // 计算重投影误差
                    double reprojError1 = 1.0 - (f1.dot(reprojection1));
                    double reprojError2 = 1.0 - (f2.dot(reprojection2));
                    quality += reprojError1 + reprojError2;
                }

                if (quality < bestQuality)
                {
                    bestQuality = quality;
                    bestQualityIndex = i;
                    bestQualitySubindex = j;
                }
            }
        }

        // 如果没找到好的解，返回单位矩阵
        if (bestQualityIndex == -1)
        {
            return transformation_t::Identity();
        }

        // 重新计算最佳解
        Eigen::MatrixXd tempEssential = essentialMatrices[bestQualityIndex];
        Eigen::JacobiSVD<Eigen::MatrixXd> SVD(
            tempEssential,
            Eigen::ComputeFullV | Eigen::ComputeFullU);
        const double scale = SVD.singularValues()[0];

        translation_t translation;
        rotation_t rotation;

        // 根据最佳子索引选择正确的解
        switch (bestQualitySubindex)
        {
        case 0:
            translation = scale * SVD.matrixU().col(2);
            rotation = SVD.matrixU() * W * SVD.matrixV().transpose();
            break;
        case 1:
            translation = scale * SVD.matrixU().col(2);
            rotation = SVD.matrixU() * W.transpose() * SVD.matrixV().transpose();
            break;
        case 2:
            translation = -scale * SVD.matrixU().col(2);
            rotation = SVD.matrixU() * W * SVD.matrixV().transpose();
            break;
        case 3:
            translation = -scale * SVD.matrixU().col(2);
            rotation = SVD.matrixU() * W.transpose() * SVD.matrixV().transpose();
            break;
        default:
            return transformation_t::Identity();
        }

        // 确保行列式为1
        if (rotation.determinant() < 0)
            rotation = -rotation;

        transformation_t bestTransformation = transformation_t::Identity();
        bestTransformation.block<3, 3>(0, 0) = rotation;
        bestTransformation.col(3) = translation;

        return bestTransformation;
    }

    opengv::transformation_t OpenGVModelEstimator::GetBestTransformationFromRotations(
        opengv::relative_pose::CentralRelativeAdapter &adapter,
        const opengv::rotations_t &rotations)
    {

        opengv::transformation_t best_transformation = opengv::transformation_t::Zero();
        double best_quality = std::numeric_limits<double>::max();

        // 对每个可能的旋转矩阵
        for (const auto &R : rotations)
        {
            // 设置当前旋转
            adapter.setR12(R);

            // 使用twopt方法估计平移
            opengv::translation_t t = opengv::relative_pose::twopt(adapter, true);

            // 构造当前变换矩阵
            opengv::transformation_t transformation = opengv::transformation_t::Identity();
            transformation.block<3, 3>(0, 0) = R;
            transformation.block<3, 1>(0, 3) = t;

            // 构造逆变换矩阵
            opengv::transformation_t inverse_transformation = opengv::transformation_t::Identity();
            inverse_transformation.block<3, 3>(0, 0) = R.transpose();
            inverse_transformation.block<3, 1>(0, 3) = -R.transpose() * t;

            // 计算重投影质量
            double quality = 0.0;
            Eigen::Matrix<double, 4, 1> p_hom;
            p_hom[3] = 1.0;

            // 设置当前变换用于三角化
            adapter.setR12(R);
            adapter.sett12(t);

            // 对所有点进行评估
            for (size_t i = 0; i < adapter.getNumberCorrespondences(); i++)
            {
                // 三角化得到3D点
                p_hom.block<3, 1>(0, 0) = opengv::triangulation::triangulate2(adapter, i);

                // 计算在两个视角下的重投影向量
                opengv::bearingVector_t reprojection1 = p_hom.block<3, 1>(0, 0);
                opengv::bearingVector_t reprojection2 = inverse_transformation * p_hom;

                // 归一化重投影向量
                reprojection1 = reprojection1 / reprojection1.norm();
                reprojection2 = reprojection2 / reprojection2.norm();

                // 获取原始观测向量
                const opengv::bearingVector_t &f1 = adapter.getBearingVector1(i);
                const opengv::bearingVector_t &f2 = adapter.getBearingVector2(i);

                // 计算重投影误差 (1-cos(alpha))
                double reprojError1 = 1.0 - f1.dot(reprojection1);
                double reprojError2 = 1.0 - f2.dot(reprojection2);
                quality += reprojError1 + reprojError2;
            }

            // 更新最佳解
            if (quality < best_quality)
            {
                best_quality = quality;
                best_transformation = transformation;
            }
        }

        return best_transformation;
    }

    // 分配先验信息
    void OpenGVModelEstimator::AssignPriorInfo(
        opengv::relative_pose::CentralRelativeAdapter &adapter)
    {
        // 检查并设置先验信息
        if (prior_info_.size() > 0)
        {
            // 检查先验旋转矩阵
            auto it_R = prior_info_.find("R_prior");
            if (it_R != prior_info_.end())
            {
                auto R_ptr = GetDataPtr<opengv::rotation_t>(it_R->second);
                if (R_ptr)
                {
                    adapter.setR12(*R_ptr);
                    if (SHOULD_LOG(DEBUG))
                    {
                        std::string verbose_msg = LanguageEnvironment::GetText(
                            "使用先验旋转:\n",
                            "Using prior rotation:\n");
                        LOG_DEBUG_ZH << verbose_msg;
                        LOG_DEBUG_EN << verbose_msg;
                    }
                }
            }

            // 检查先验平移向量
            auto it_t = prior_info_.find("t_prior");
            if (it_t != prior_info_.end())
            {
                auto t_ptr = GetDataPtr<opengv::translation_t>(it_t->second);
                if (t_ptr)
                {
                    adapter.sett12(*t_ptr);
                    if (SHOULD_LOG(DEBUG))
                    {
                        std::string verbose_msg = LanguageEnvironment::GetText(
                            "使用先验平移:\n",
                            "Using prior translation:\n");
                        LOG_DEBUG_ZH << verbose_msg;
                        LOG_DEBUG_EN << verbose_msg;
                    }
                }
            }
        }
    }

    transformation_t OpenGVModelEstimator::RefineModel(
        opengv::relative_pose::CentralRelativeAdapter &adapter,
        const transformation_t &initial_transformation,
        RefineMethod refine_method)
    {
        try
        {
            // 设置初始值
            adapter.setR12(initial_transformation.block<3, 3>(0, 0));
            adapter.sett12(initial_transformation.block<3, 1>(0, 3));

            transformation_t refined_transformation = initial_transformation;

            // 检查是否是 RANSAC 模式，如果是则只使用内点进行优化
            std::string algorithm = GetOptionAsString("algorithm", "fivept_stewenius");
            std::vector<int> inlier_indices;

            if (IsRansacAlgorithm(algorithm))
            {
                // 获取内点索引
                auto sample_ptr = CastToSample<IdMatches>(required_package_["data_sample"]);
                if (sample_ptr)
                {
                    for (size_t i = 0; i < sample_ptr->size(); ++i)
                    {
                        if ((*sample_ptr)[i].is_inlier)
                        {
                            inlier_indices.push_back(static_cast<int>(i));
                        }
                    }

                    if (SHOULD_LOG(DEBUG))
                    {
                        std::string verbose_msg = LanguageEnvironment::GetText(
                            "使用 " + std::to_string(inlier_indices.size()) + " 个内点进行模型优化",
                            "Using " + std::to_string(inlier_indices.size()) + " inliers for model refinement");
                        LOG_DEBUG_ZH << verbose_msg;
                        LOG_DEBUG_EN << verbose_msg;
                    }
                }
            }

            switch (refine_method)
            {
            case RefineMethod::EIGENSOLVER:
            {
                if (SHOULD_LOG(DEBUG))
                {
                    std::string verbose_msg = LanguageEnvironment::GetText(
                        "使用特征值分解法优化旋转",
                        "Refining rotation using eigensolver");
                    LOG_DEBUG_ZH << verbose_msg;
                    LOG_DEBUG_EN << verbose_msg;
                }

                // 获取优化参数
                bool use_weights = GetOptionAsBool("use_weights", false);

                // 使用特征值分解法优化旋转
                rotation_t optimized_rotation;
                if (!inlier_indices.empty())
                {
                    optimized_rotation = opengv::relative_pose::eigensolver(adapter, inlier_indices, use_weights);
                }
                else
                {
                    optimized_rotation = opengv::relative_pose::eigensolver(adapter, use_weights);
                }

                refined_transformation.block<3, 3>(0, 0) = optimized_rotation;
                // 保持原有平移
                break;
            }

            case RefineMethod::NONLINEAR:
            {
                if (SHOULD_LOG(DEBUG))
                {
                    std::string verbose_msg = LanguageEnvironment::GetText(
                        "使用非线性优化方法优化完整位姿",
                        "Refining full pose using nonlinear optimization");
                    LOG_DEBUG_ZH << verbose_msg;
                    LOG_DEBUG_EN << verbose_msg;
                }

                // 使用非线性优化方法优化完整位姿
                if (!inlier_indices.empty())
                {
                    refined_transformation = opengv::relative_pose::optimize_nonlinear(adapter, inlier_indices);
                }
                else
                {
                    refined_transformation = opengv::relative_pose::optimize_nonlinear(adapter);
                }
                break;
            }

            case RefineMethod::ROTATION_ONLY:
            {
                if (SHOULD_LOG(DEBUG))
                {
                    std::string verbose_msg = LanguageEnvironment::GetText(
                        "仅优化旋转部分",
                        "Refining rotation only");
                    LOG_DEBUG_ZH << verbose_msg;
                    LOG_DEBUG_EN << verbose_msg;
                }

                // 使用Arun方法优化旋转
                rotation_t optimized_rotation;
                if (!inlier_indices.empty())
                {
                    optimized_rotation = opengv::relative_pose::rotationOnly(adapter, inlier_indices);
                }
                else
                {
                    optimized_rotation = opengv::relative_pose::rotationOnly(adapter);
                }

                refined_transformation.block<3, 3>(0, 0) = optimized_rotation;
                // 保持原有平移
                break;
            }

            case RefineMethod::NONE:
            default:
                // 不进行优化，直接返回初始变换
                break;
            }

            if (SHOULD_LOG(DEBUG))
            {
                std::string initial_msg = LanguageEnvironment::GetText(
                    "优化前变换矩阵:\n",
                    "Initial transformation:\n");
                LOG_DEBUG_ZH << initial_msg;
                LOG_DEBUG_EN << initial_msg;

                std::string refined_msg = LanguageEnvironment::GetText(
                    "优化后变换矩阵:\n",
                    "Refined transformation:\n");
                LOG_DEBUG_ZH << refined_msg;
                LOG_DEBUG_EN << refined_msg;

                // 计算优化前后的差异
                Matrix3d R_initial = initial_transformation.block<3, 3>(0, 0);
                Matrix3d R_refined = refined_transformation.block<3, 3>(0, 0);
                Vector3d t_initial = initial_transformation.block<3, 1>(0, 3);
                Vector3d t_refined = refined_transformation.block<3, 1>(0, 3);

                // 计算旋转差异（使用角轴表示）
                Matrix3d R_diff = R_refined * R_initial.transpose();
                double rotation_diff = Eigen::AngleAxisd(R_diff).angle() * 180.0 / M_PI;

                // 计算平移差异（欧几里得距离）
                double translation_diff = (t_refined - t_initial).norm();

                std::string rot_diff_msg = LanguageEnvironment::GetText(
                    "旋转差异: " + std::to_string(rotation_diff) + " 度",
                    "Rotation difference: " + std::to_string(rotation_diff) + " degrees");
                LOG_DEBUG_ZH << rot_diff_msg;
                LOG_DEBUG_EN << rot_diff_msg;

                std::string trans_diff_msg = LanguageEnvironment::GetText(
                    "平移差异: " + std::to_string(translation_diff),
                    "Translation difference: " + std::to_string(translation_diff));
                LOG_DEBUG_ZH << trans_diff_msg;
                LOG_DEBUG_EN << trans_diff_msg;

                if (!inlier_indices.empty())
                {
                    std::string inlier_refine_msg = LanguageEnvironment::GetText(
                        "使用了 " + std::to_string(inlier_indices.size()) + " 个内点进行优化",
                        "Used " + std::to_string(inlier_indices.size()) + " inliers for refinement");
                    LOG_DEBUG_ZH << inlier_refine_msg;
                    LOG_DEBUG_EN << inlier_refine_msg;
                }
            }

            return refined_transformation;
        }
        catch (const std::exception &e)
        {
            std::string err_msg = LanguageEnvironment::GetText(
                "模型优化失败: ",
                "Model refinement failed: ");
            LOG_ERROR_ZH << err_msg << e.what();
            LOG_ERROR_EN << err_msg << e.what();
            return initial_transformation; // 返回初始变换
        }
    }

    // 辅助函数：获取算法所需的最小样本数
    size_t OpenGVModelEstimator::GetMinimumSamplesForAlgorithm(const std::string &algorithm) const
    {
        // 参考OpenMVG和OpenGV的要求
        if (algorithm == "twopt" || algorithm == "twopt_rotationOnly")
        {
            return 2;
        }
        else if (algorithm == "rotationOnly")
        {
            return 3; // Arun方法至少需要3个点
        }
        else if (algorithm == "fivept_stewenius" || algorithm == "fivept_nister" ||
                 algorithm == "fivept_kneip" || algorithm == "fivept_stewenius_ransac" ||
                 algorithm == "fivept_nister_ransac")
        {
            return 5;
        }
        else if (algorithm == "sevenpt" || algorithm == "sevenpt_ransac")
        {
            return 7;
        }
        else if (algorithm == "eightpt" || algorithm == "eightpt_ransac")
        {
            return 8;
        }
        else if (algorithm == "eigensolver" || algorithm == "eigensolver_ransac")
        {
            return 5; // 特征值分解法通常需要至少5个点
        }
        else if (algorithm == "rotationOnly_ransac")
        {
            return 3;
        }
        else if (algorithm == "rel_nonlin_central")
        {
            return 5; // 非线性优化需要初始估计，通常需要5个点
        }
        else
        {
            // 默认返回5（五点法）
            return 5;
        }
    }

} // namespace PluginMethods

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::OpenGVModelEstimator)
