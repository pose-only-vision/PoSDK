/**
 * @file TwoViewEstimator.cpp
 * @brief Two-view pose estimator implementation | 双视图位姿估计器实现
 */

#include "TwoViewEstimator.hpp"
#include <boost/algorithm/string.hpp>
#include <po_core.hpp>
#include <iomanip>
#include <sstream>
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统
#include <atomic>
#include <mutex>
#include <numeric>
#include <algorithm>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    TwoViewEstimator::TwoViewEstimator()
    {
        // 注册所需数据类型
        required_package_["data_matches"] = nullptr;
        required_package_["data_features"] = nullptr;
        required_package_["data_camera_models"] = nullptr;

        // 初始化默认配置
        InitializeDefaultConfigPath();
    }

    DataPtr TwoViewEstimator::Run()
    {
        // Start profiling for the entire TwoViewEstimator::Run function | 开始对整个TwoViewEstimator::Run函数进行性能分析
        PROFILER_START_AUTO(enable_profiling_);

        DisplayConfigInfo();

        // ======== 显示启动信息 ========
        std::string estimator = GetOptionAsString("estimator", "opencv_two_view_estimator");
        bool enable_refine = GetOptionAsBool("enable_refine", false);
        std::string algorithm = GetOptionAsString("algorithm", "");

        LOG_INFO_ZH << "========================================";
        LOG_INFO_EN << "========================================";
        LOG_INFO_ZH << "  TwoViewEstimator 启动配置";
        LOG_INFO_EN << "  TwoViewEstimator Launch Configuration";
        LOG_INFO_ZH << "========================================";
        LOG_INFO_EN << "========================================";
        LOG_INFO_ZH << "  估计器算法: " << estimator;
        LOG_INFO_EN << "  Estimator algorithm: " << estimator;

        if (!algorithm.empty())
        {
            LOG_INFO_ZH << "  子算法: " << algorithm;
            LOG_INFO_EN << "  Sub-algorithm: " << algorithm;
        }

        LOG_INFO_ZH << "  精细优化: " << (enable_refine ? "✓ 启用" : "✗ 禁用");
        LOG_INFO_EN << "  Refinement: " << (enable_refine ? "✓ Enabled" : "✗ Disabled");

        if (enable_refine)
        {
            bool supports_posdk_refine = (boost::iequals(estimator, "opencv_two_view_estimator") ||
                                          boost::iequals(estimator, "barath_two_view_estimator") ||
                                          boost::iequals(estimator, "opengv_model_estimator"));

            if (supports_posdk_refine)
            {
                LOG_INFO_ZH << "    └─ 使用 PoSDK TwoViewOptimizer (Eigen-LM + Cauchy Loss)";
                LOG_INFO_EN << "    └─ Using PoSDK TwoViewOptimizer (Eigen-LM + Cauchy Loss)";
            }
            else if (boost::iequals(estimator, "poselib_model_estimator"))
            {
                LOG_INFO_ZH << "    └─ 使用 PoseLib 内部优化 (Bundle Adjustment)";
                LOG_INFO_EN << "    └─ Using PoseLib internal refinement (Bundle Adjustment)";
            }
        }
        LOG_INFO_ALL << "----------------------------------------";

        // 1. 获取输入数据
        auto matches_ptr = GetDataPtr<Matches>(required_package_["data_matches"]);
        auto features_ptr = GetDataPtr<FeaturesInfo>(required_package_["data_features"]);
        auto cameras_ptr = GetDataPtr<CameraModels>(required_package_["data_camera_models"]);

        if (!matches_ptr || !features_ptr || !cameras_ptr)
        {
            PROFILER_END(); // End profiling before return | 返回前结束性能分析
            LOG_ERROR_ZH << "无效输入数据";
            LOG_ERROR_EN << "Invalid input data";
            return nullptr;
        }

        // 2. 创建结果容器
        auto data_relative_poses = FactoryData::Create("data_relative_poses");
        auto poses = GetDataPtr<RelativePoses>(data_relative_poses);
        if (!poses)
        {
            PROFILER_END(); // End profiling before return | 返回前结束性能分析
            LOG_ERROR_ZH << "Failed to create relative poses container";
            LOG_ERROR_EN << "Failed to create relative poses container";
            return nullptr;
        }

        // 3. 创建方法
        auto method = std::dynamic_pointer_cast<Interface::MethodPreset>(FactoryMethod::Create(estimator.c_str()));
        if (!method)
        {
            PROFILER_END(); // End profiling before return | 返回前结束性能分析
            LOG_ERROR_ZH << "Failed to create method: " << estimator;
            LOG_ERROR_EN << "Failed to create method: " << estimator;
            return nullptr;
        }
        current_method_ = method; // 保存当前方法实例
        LOG_DEBUG_ZH << "Method created successfully: " << estimator;
        LOG_DEBUG_EN << "Method created successfully: " << estimator;

        // 4. 为支持评估器的方法设置算法名称

        // 构造完整的算法名称：estimator + algorithm + enable_refine
        std::string full_algorithm_name = estimator;
        if (!algorithm.empty())
        {
            full_algorithm_name += "_" + algorithm;
        }
        if (enable_refine)
        {
            full_algorithm_name += "_refine";
        }

        SetEvaluatorAlgorithm(full_algorithm_name);
        method->SetEvaluatorAlgorithm(full_algorithm_name);
        LOG_DEBUG_ZH << "Set evaluator algorithm name: " << full_algorithm_name;
        LOG_DEBUG_EN << "Set evaluator algorithm name: " << full_algorithm_name;

        // 统计变量
        size_t total_view_pairs = matches_ptr->size();
        size_t processed_pairs = 0;
        size_t successful_pairs = 0;
        size_t empty_matches = 0;
        size_t invalid_view_ids = 0;
        size_t insufficient_inliers = 0;
        size_t insufficient_pairs = 0; // 新增：匹配对数量不足的统计
        size_t conversion_failures = 0;
        size_t method_failures = 0;
        size_t invalid_poses = 0;

        // 获取最小匹配对数量要求
        int min_num_required_pairs = GetOptionAsIndexT("min_num_required_pairs", 50);
        LOG_DEBUG_ZH << "[TwoViewEstimator] Minimum required pairs: " << min_num_required_pairs;
        LOG_DEBUG_EN << "[TwoViewEstimator] Minimum required pairs: " << min_num_required_pairs;

        // 5. 收集所有视图对用于并行处理
        std::vector<std::pair<ViewPair, IdMatches *>> view_pair_list;
        view_pair_list.reserve(matches_ptr->size());
        for (auto &[view_pair, matches] : *matches_ptr)
        {
            view_pair_list.emplace_back(view_pair, &matches);
        }

        // 线程安全的统计变量
        std::atomic<size_t> atomic_processed_pairs(0);
        std::atomic<size_t> atomic_successful_pairs(0);
        std::atomic<size_t> atomic_empty_matches(0);
        std::atomic<size_t> atomic_invalid_view_ids(0);
        std::atomic<size_t> atomic_insufficient_inliers(0);
        std::atomic<size_t> atomic_insufficient_pairs(0);
        std::atomic<size_t> atomic_conversion_failures(0);
        std::atomic<size_t> atomic_method_failures(0);
        std::atomic<size_t> atomic_invalid_poses(0);

        // 线程安全的结果容器
        std::vector<RelativePose> thread_safe_poses;
        thread_safe_poses.reserve(total_view_pairs);
        std::mutex poses_mutex;

        // 进度跟踪变量
        size_t last_progress_milestone = 0;
        std::mutex progress_mutex;

        // 配置多线程
        int num_threads = GetOptionAsIndexT("num_threads", 4);
        if (num_threads <= 0)
        {
            num_threads = 1;
        }

        LOG_INFO_ALL << "----------------------------------------";
        LOG_INFO_ZH << "  多线程配置:";
        LOG_INFO_EN << "  Multi-threading Configuration:";
#ifdef USE_OPENMP
        omp_set_num_threads(num_threads);
        LOG_INFO_ZH << "    └─ OpenMP 已启用，线程数: " << num_threads;
        LOG_INFO_EN << "    └─ OpenMP enabled, threads: " << num_threads;
        LOG_INFO_ZH << "    └─ 并行粒度: View Pair 级别";
        LOG_INFO_EN << "    └─ Parallelism: View Pair level";
#else
        LOG_INFO_ZH << "    └─ OpenMP 未启用，使用单线程";
        LOG_INFO_EN << "    └─ OpenMP not enabled, single-threaded";
        num_threads = 1;
#endif
        LOG_INFO_ALL << "----------------------------------------";

        // 并行处理所有视图对
#ifdef USE_OPENMP
#pragma omp parallel for schedule(dynamic) shared(view_pair_list, features_ptr, cameras_ptr, estimator, algorithm, enable_refine, min_num_required_pairs, full_algorithm_name, atomic_processed_pairs, atomic_successful_pairs, atomic_empty_matches, atomic_invalid_view_ids, atomic_insufficient_inliers, atomic_insufficient_pairs, atomic_conversion_failures, atomic_method_failures, atomic_invalid_poses, thread_safe_poses, poses_mutex, last_progress_milestone, progress_mutex, total_view_pairs)
#endif
        for (size_t pair_idx = 0; pair_idx < view_pair_list.size(); ++pair_idx)
        {
            const auto &[view_pair, matches_ptr_local] = view_pair_list[pair_idx];
            IdMatches &matches = *matches_ptr_local;

            // 为每个线程创建独立的方法实例
            auto thread_method = std::dynamic_pointer_cast<Interface::MethodPreset>(FactoryMethod::Create(estimator.c_str()));
            if (!thread_method)
            {
                LOG_ERROR_ZH << "线程中创建方法失败: " << estimator;
                LOG_ERROR_EN << "Failed to create method in thread: " << estimator;
                atomic_method_failures.fetch_add(1);
                continue;
            }

            // 设置评估器算法名称（线程安全）
            thread_method->SetEvaluatorAlgorithm(full_algorithm_name);

            // 递增处理计数器
            size_t current_processed = atomic_processed_pairs.fetch_add(1) + 1;

            if (SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "处理视图对 (" << view_pair.first << "," << view_pair.second << "): " << current_processed << "/" << total_view_pairs;
                LOG_DEBUG_EN << "Processing view pair (" << view_pair.first << "," << view_pair.second << "): " << current_processed << "/" << total_view_pairs;
            }

            // 统计处理前的匹配数量
            size_t initial_matches_count = matches.size();
            size_t initial_inliers_count = 0;
            for (const auto &match : matches)
            {
                if (match.is_inlier)
                    initial_inliers_count++;
            }

            // 预先验证视图对和匹配数据
            if (matches.empty())
            {
                LOG_WARNING_ZH << "Warning: Empty matches for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Warning: Empty matches for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                atomic_empty_matches.fetch_add(1);
                continue;
            }

            // 检查匹配对数量是否满足最小要求
            if (static_cast<int>(matches.size()) < min_num_required_pairs)
            {
                LOG_WARNING_ZH << "Warning: Insufficient match pairs (" << matches.size()
                               << " < " << min_num_required_pairs << ") for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Warning: Insufficient match pairs (" << matches.size()
                               << " < " << min_num_required_pairs << ") for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";

                // 将所有匹配对的is_inlier标志设置为false
                for (auto &match : matches)
                {
                    match.is_inlier = false;
                }

                atomic_insufficient_pairs.fetch_add(1);
                continue;
            }

            // 显示处理前的匹配统计
            if (SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "视图对 (" << view_pair.first << "," << view_pair.second
                             << ") - Initial matches: " << initial_matches_count
                             << " (inliers: " << initial_inliers_count << ")";
                LOG_DEBUG_EN << "View pair (" << view_pair.first << "," << view_pair.second
                             << ") - Initial matches: " << initial_matches_count
                             << " (inliers: " << initial_inliers_count << ")";
            }

            // 验证view_id是否在有效范围内
            if (view_pair.first >= features_ptr->size() || view_pair.second >= features_ptr->size())
            {
                LOG_ERROR_ZH << "Invalid view_pair (" << view_pair.first << "," << view_pair.second
                             << ") - exceeds features size " << features_ptr->size();
                LOG_ERROR_EN << "Invalid view_pair (" << view_pair.first << "," << view_pair.second
                             << ") - exceeds features size " << features_ptr->size();
                atomic_invalid_view_ids.fetch_add(1);
                continue;
            }

            // 转换为射线向量
            BearingPairs bearing_pairs;
            if (!types::MatchesToBearingPairs(matches, *features_ptr, *cameras_ptr, view_pair, bearing_pairs))
            {
                LOG_WARNING_ZH << "Failed to convert matches to bearing pairs for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Failed to convert matches to bearing pairs for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                atomic_conversion_failures.fetch_add(1);
                continue;
            }

            // 根据算法类型准备数据（统一处理所有estimator）
            // 1. 创建共享的matches数据（避免拷贝）
            auto matches_shared = std::shared_ptr<IdMatches>(matches_ptr, &matches);
            auto matches_data = std::make_shared<DataSample<IdMatches>>(matches_shared);

            // 2. 设置通用方法选项
            MethodOptions options;
            options["view_i"] = std::to_string(view_pair.first);
            options["view_j"] = std::to_string(view_pair.second);

            // 3. 传递算法参数（如果指定）
            std::string algorithm = GetOptionAsString("algorithm", "");
            if (!algorithm.empty())
            {
                options["algorithm"] = algorithm;
            }

            // 4. PoseLib特殊处理：传递统一精细优化参数
            if (boost::iequals(estimator, "poselib_model_estimator"))
            {
                bool enable_refine = GetOptionAsBool("enable_refine", false);
                if (enable_refine)
                {
                    options["refine_model"] = "nonlinear";
                    if (SHOULD_LOG(DEBUG))
                    {
                        LOG_DEBUG_ZH << "启用PoseLib内部精细优化 (refine_model=nonlinear) for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                        LOG_DEBUG_EN << "Enabling PoseLib internal refinement (refine_model=nonlinear) for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                    }
                }
            }

            // 5. 设置方法选项和输入数据（统一流程）
            thread_method->SetMethodOptions(options);
            thread_method->SetRequiredData(matches_data);
            thread_method->SetRequiredData(required_package_["data_features"]);
            thread_method->SetRequiredData(required_package_["data_camera_models"]);

            // 设置GTdata(如果先验数据有的话) - 使用thread_method
            SetCurrentViewPairGTDataForMethod(view_pair, thread_method);

            // 执行位姿估计
            auto result = thread_method->Build();

            if (!result)
            {
                // 算法执行失败：清除所有内点标志
                for (auto &match : matches)
                {
                    match.is_inlier = false;
                }

                LOG_WARNING_ZH << "Method Build() failed for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Method Build() failed for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                atomic_method_failures.fetch_add(1);
                continue;
            }

            // 处理内点信息并进行统一的质量验证
            // OpenGV、OpenCV、Barath和PoseLib估计器会自动更新匹配数据中的内点标志

            // 统计内点数量（对所有算法统一处理）
            size_t final_inlier_count = 0;
            for (const auto &match : matches)
            {
                if (match.is_inlier)
                    final_inlier_count++;
            }

            // 显示处理后的匹配统计（算法执行后）
            LOG_DEBUG_ZH << "视图对 (" << view_pair.first << "," << view_pair.second
                         << ") - After estimation: " << matches.size() << " matches, "
                         << final_inlier_count << " inliers ("
                         << std::fixed << std::setprecision(1)
                         << (100.0 * final_inlier_count / matches.size()) << "%)";
            LOG_DEBUG_EN << "View pair (" << view_pair.first << "," << view_pair.second
                         << ") - After estimation: " << matches.size() << " matches, "
                         << final_inlier_count << " inliers ("
                         << std::fixed << std::setprecision(1)
                         << (100.0 * final_inlier_count / matches.size()) << "%)";

            // 统一的质量验证处理（对所有算法统一管控）
            bool quality_validation_passed = false;
            bool enable_quality_validation = GetOptionAsBool("enable_quality_validation", true);

            if (enable_quality_validation)
            {
                // 使用统一的质量验证函数
                quality_validation_passed = ValidateEstimationQuality(final_inlier_count, matches.size(), estimator);

                if (!quality_validation_passed)
                {
                    // 质量验证失败：清除所有内点标志
                    for (auto &match : matches)
                    {
                        match.is_inlier = false;
                    }

                    if (log_level_ >= 2)
                    {
                        LOG_WARNING_ZH << "Quality validation failed for view pair ("
                                       << view_pair.first << "," << view_pair.second << ")";
                        LOG_WARNING_EN << "Quality validation failed for view pair ("
                                       << view_pair.first << "," << view_pair.second << ")";
                    }
                    atomic_insufficient_inliers.fetch_add(1);
                    continue; // 跳过该视图对，不保存pose结果
                }
            }
            else
            {
                // 如果不启用质量验证，仅进行基本的内点数量检查
                if (final_inlier_count < 6) // 双视图估计至少需要8个内点
                {
                    // 基本验证失败：清除所有内点标志
                    for (auto &match : matches)
                    {
                        match.is_inlier = false;
                    }

                    if (log_level_ >= 2)
                    {
                        LOG_WARNING_ZH << "Warning: Insufficient final inliers (" << final_inlier_count
                                       << ") for view pair (" << view_pair.first << "," << view_pair.second << ")";
                        LOG_WARNING_EN << "Warning: Insufficient final inliers (" << final_inlier_count
                                       << ") for view pair (" << view_pair.first << "," << view_pair.second << ")";
                    }
                    atomic_insufficient_inliers.fetch_add(1);
                    continue; // 跳过该视图对，不保存pose结果
                }
                quality_validation_passed = true; // 基本检查通过
            }

            LOG_DEBUG_ZH << "Final inlier count: " << final_inlier_count
                         << "/" << matches.size() << " for view pair ("
                         << view_pair.first << "," << view_pair.second << ")";
            LOG_DEBUG_EN << "Final inlier count: " << final_inlier_count
                         << "/" << matches.size() << " for view pair ("
                         << view_pair.first << "," << view_pair.second << ")";

            // 获取估计结果并验证有效性
            auto pose_result = GetDataPtr<RelativePose>(result);
            if (!pose_result)
            {
                // 结果提取失败：清除所有内点标志
                for (auto &match : matches)
                {
                    match.is_inlier = false;
                }
                if (log_level_ >= 2)
                {
                    LOG_WARNING_ZH << "Failed to extract RelativePose from result for view pair ("
                                   << view_pair.first << "," << view_pair.second << ")";
                    LOG_WARNING_EN << "Failed to extract RelativePose from result for view pair ("
                                   << view_pair.first << "," << view_pair.second << ")";
                }
                atomic_method_failures.fetch_add(1);
                continue;
            }

            // 统一精细优化：根据估计器类型选择合适的精细优化方法
            bool enable_refine = GetOptionAsBool("enable_refine", false);
            if (enable_refine)
            {
                // 检查是否为支持PoSDK精细优化的方法
                bool supports_posdk_refine = (boost::iequals(estimator, "opencv_two_view_estimator") ||
                                              boost::iequals(estimator, "barath_two_view_estimator") ||
                                              boost::iequals(estimator, "opengv_model_estimator"));

                if (supports_posdk_refine)
                {
                    // 从matches转换bearing_pairs，只使用内点
                    BearingPairs refinement_bearing_pairs;
                    if (!types::MatchesToBearingPairsInliersOnly(matches, *features_ptr, *cameras_ptr, view_pair, refinement_bearing_pairs))
                    {
                        if (SHOULD_LOG(DEBUG))
                        {
                            LOG_DEBUG_ZH << "Failed to convert inlier matches to bearing pairs for refinement, skipping for view pair ("
                                         << view_pair.first << "," << view_pair.second << ")";
                            LOG_DEBUG_EN << "Failed to convert inlier matches to bearing pairs for refinement, skipping for view pair ("
                                         << view_pair.first << "," << view_pair.second << ")";
                        }
                        refinement_bearing_pairs.clear(); // 确保为空，跳过精细优化
                    }

                    if (!refinement_bearing_pairs.empty())
                    {
                        // 统计精细优化前的状态
                        size_t pre_refinement_total_matches = matches.size();
                        size_t pre_refinement_inliers = 0;
                        for (const auto &match : matches)
                        {
                            if (match.is_inlier)
                                pre_refinement_inliers++;
                        }

                        LOG_DEBUG_ZH << "[PoSDK Refinement] 精细优化前统计 - 视图对 (" << view_pair.first << "," << view_pair.second << "):";
                        LOG_DEBUG_ZH << "  总匹配数: " << pre_refinement_total_matches;
                        LOG_DEBUG_ZH << "  内点数: " << pre_refinement_inliers << " ("
                                     << std::fixed << std::setprecision(1) << (100.0 * pre_refinement_inliers / pre_refinement_total_matches) << "%)";
                        LOG_DEBUG_ZH << "  用于优化的bearing_pairs数: " << refinement_bearing_pairs.size();

                        LOG_DEBUG_EN << "[PoSDK Refinement] Pre-refinement statistics - View pair (" << view_pair.first << "," << view_pair.second << "):";
                        LOG_DEBUG_EN << "  Total matches: " << pre_refinement_total_matches;
                        LOG_DEBUG_EN << "  Inliers: " << pre_refinement_inliers << " ("
                                     << std::fixed << std::setprecision(1) << (100.0 * pre_refinement_inliers / pre_refinement_total_matches) << "%)";
                        LOG_DEBUG_EN << "  Bearing pairs for optimization: " << refinement_bearing_pairs.size();

                        auto optimized_pose_result = ApplyPoSDKRefinement(*pose_result, refinement_bearing_pairs, view_pair, matches);
                        if (optimized_pose_result)
                        {
                            // 统计精细优化后的状态
                            size_t post_refinement_total_matches = matches.size();
                            size_t post_refinement_inliers = 0;
                            for (const auto &match : matches)
                            {
                                if (match.is_inlier)
                                    post_refinement_inliers++;
                            }

                            LOG_DEBUG_ZH << "[PoSDK Refinement] 精细优化后统计 - 视图对 (" << view_pair.first << "," << view_pair.second << "):";
                            LOG_DEBUG_ZH << "  总匹配数: " << post_refinement_total_matches;
                            LOG_DEBUG_ZH << "  内点数: " << post_refinement_inliers << " ("
                                         << std::fixed << std::setprecision(1) << (100.0 * post_refinement_inliers / post_refinement_total_matches) << "%)";
                            LOG_DEBUG_ZH << "  内点变化: " << static_cast<int>(post_refinement_inliers) - static_cast<int>(pre_refinement_inliers);

                            LOG_DEBUG_EN << "[PoSDK Refinement] Post-refinement statistics - View pair (" << view_pair.first << "," << view_pair.second << "):";
                            LOG_DEBUG_EN << "  Total matches: " << post_refinement_total_matches;
                            LOG_DEBUG_EN << "  Inliers: " << post_refinement_inliers << " ("
                                         << std::fixed << std::setprecision(1) << (100.0 * post_refinement_inliers / post_refinement_total_matches) << "%)";
                            LOG_DEBUG_EN << "  Inlier change: " << static_cast<int>(post_refinement_inliers) - static_cast<int>(pre_refinement_inliers);

                            // 使用优化后的位姿替换原始估计
                            pose_result = optimized_pose_result;

                            if (SHOULD_LOG(DEBUG))
                            {
                                LOG_DEBUG_ZH << "PoSDK refinement applied successfully for " << estimator
                                             << " view pair (" << view_pair.first << "," << view_pair.second << ")";
                                LOG_DEBUG_EN << "PoSDK refinement applied successfully for " << estimator
                                             << " view pair (" << view_pair.first << "," << view_pair.second << ")";
                            }
                        }
                        else
                        {
                            // 精细优化失败意味着整个view pair的估计都有问题
                            // 清除所有内点标志并跳过这个view pair
                            for (auto &match : matches)
                            {
                                match.is_inlier = false;
                            }

                            LOG_WARNING_ZH << "[PoSDK Refinement] 精细优化失败，拒绝整个view pair ("
                                           << view_pair.first << "," << view_pair.second << ") - 初始估计也不可信";
                            LOG_WARNING_EN << "[PoSDK Refinement] Refinement failed, rejecting entire view pair ("
                                           << view_pair.first << "," << view_pair.second << ") - initial estimate also unreliable";

                            atomic_method_failures.fetch_add(1);
                            continue; // 跳过后续处理，不添加pose结果
                        }
                    }
                }
                else if (boost::iequals(estimator, "poselib_model_estimator"))
                {
                    // poselib_model_estimator的精细优化已在上面通过refine_model参数处理
                    if (SHOULD_LOG(DEBUG))
                    {
                        LOG_DEBUG_ZH << "PoseLib internal refinement enabled for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                        LOG_DEBUG_EN << "PoseLib internal refinement enabled for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                    }
                }
                else
                {
                    // 其他估计器不支持精细优化，记录警告信息
                    if (SHOULD_LOG(DEBUG))
                    {
                        LOG_DEBUG_ZH << "Refinement not supported for estimator " << estimator
                                     << ", ignoring enable_refine=true for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                        LOG_DEBUG_EN << "Refinement not supported for estimator " << estimator
                                     << ", ignoring enable_refine=true for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                    }
                }
            }

            // 验证位姿数据的有效性
            bool pose_valid = true;

            // 检查旋转矩阵是否有效
            double det = pose_result->GetRotation().determinant();
            if (std::abs(det - 1.0) > 0.1) // 旋转矩阵行列式应该接近1
            {
                LOG_WARNING_ZH << "Warning: Invalid rotation matrix determinant " << det
                               << " for view pair (" << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Warning: Invalid rotation matrix determinant " << det
                               << " for view pair (" << view_pair.first << "," << view_pair.second << ")";
                pose_valid = false;
            }

            // 检查旋转矩阵和平移向量是否包含NaN或Inf
            if (!pose_result->GetRotation().allFinite() || !pose_result->GetTranslation().allFinite())
            {
                LOG_WARNING_ZH << "Warning: Non-finite values in pose for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Warning: Non-finite values in pose for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                pose_valid = false;
            }

            // 检查平移向量是否为零向量（可能的估计失败）
            if (pose_result->GetTranslation().norm() < 1e-12)
            {
                LOG_WARNING_ZH << "Warning: Zero translation vector for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Warning: Zero translation vector for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                // 零平移可能是有效的（纯旋转），所以只警告不拒绝
            }

            if (pose_valid)
            {
                // 将算法内部格式转换为PoSDK标准格式
                RelativePose converted_pose = ToPoSDKRelativePoseFormat(*pose_result);

                // 线程安全地添加结果
                {
                    std::lock_guard<std::mutex> lock(poses_mutex);
                    thread_safe_poses.push_back(converted_pose);
                }
                atomic_successful_pairs.fetch_add(1);

                // 打印估计结果（显示转换后的PoSDK标准格式, 10位小数）
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "Successfully estimated relative pose: ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "Successfully estimated relative pose: ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_ZH << "PoSDK format - Rotation: " << std::endl
                                 << std::fixed << std::setprecision(10) << converted_pose.GetRotation() << std::endl;
                    LOG_DEBUG_EN << "PoSDK format - Rotation: " << std::endl
                                 << std::fixed << std::setprecision(10) << converted_pose.GetRotation() << std::endl;

                    LOG_DEBUG_ZH << "PoSDK format - Translation: " << std::endl
                                 << std::fixed << std::setprecision(10) << converted_pose.GetTranslation().transpose() << std::endl;
                    LOG_DEBUG_EN << "PoSDK format - Translation: " << std::endl
                                 << std::fixed << std::setprecision(10) << converted_pose.GetTranslation().transpose() << std::endl;

                    // 可选：同时显示原始算法输出（调试用）
                    if (SHOULD_LOG(DEBUG))
                    {
                        LOG_DEBUG_ZH << "Original algorithm format - Rotation: " << std::endl
                                     << pose_result->GetRotation() << std::endl;
                        LOG_DEBUG_EN << "Original algorithm format - Rotation: " << std::endl
                                     << pose_result->GetRotation() << std::endl;
                        LOG_DEBUG_ZH << "Original algorithm format - Translation: " << std::endl
                                     << pose_result->GetTranslation().transpose() << std::endl;
                        LOG_DEBUG_EN << "Original algorithm format - Translation: " << std::endl
                                     << pose_result->GetTranslation().transpose() << std::endl;
                    }
                }

                // 显示最新的评估结果（如果启用了评估器）
                if (GetOptionAsBool("enable_evaluator"))
                {
                    // 构造完整的算法名称，与上面SetEvaluatorAlgorithm中的逻辑一致
                    std::string display_algorithm = estimator;
                    if (!algorithm.empty())
                    {
                        display_algorithm += "_" + algorithm;
                    }
                    bool enable_refine = GetOptionAsBool("enable_refine", false);
                    if (enable_refine)
                    {
                        display_algorithm += "_refine";
                    }

                    // 显示最新评估结果，包含view_pairs和匹配数量信息
                    std::string view_pair_info = "(" + std::to_string(view_pair.first) + "," + std::to_string(view_pair.second) + ")";
                    // EvaluatorManager::PrintLatestEvaluationResults("RelativePose", display_algorithm, "view_pairs|match_num");
                }
            }
            else
            {
                // 位姿验证失败：清除所有内点标志
                for (auto &match : matches)
                {
                    match.is_inlier = false;
                }

                LOG_WARNING_ZH << "Rejected invalid pose for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                LOG_WARNING_EN << "Rejected invalid pose for view pair ("
                               << view_pair.first << "," << view_pair.second << ")";
                atomic_invalid_poses.fetch_add(1);
            }

            // 更新进度条（线程安全）
            {
                std::lock_guard<std::mutex> lock(progress_mutex);
                size_t current_milestone = (atomic_processed_pairs.load() * 5) / total_view_pairs;
                if (current_milestone > last_progress_milestone || atomic_processed_pairs.load() == total_view_pairs)
                {
                    std::string task_name = "(successful: " + std::to_string(atomic_successful_pairs.load()) + "):";
                    ShowProgressBar(atomic_processed_pairs.load(), total_view_pairs, task_name);
                    last_progress_milestone = current_milestone;
                }
            }
        }

        // 将原子变量的值赋给最终统计变量
        processed_pairs = atomic_processed_pairs.load();
        successful_pairs = atomic_successful_pairs.load();
        empty_matches = atomic_empty_matches.load();
        invalid_view_ids = atomic_invalid_view_ids.load();
        insufficient_inliers = atomic_insufficient_inliers.load();
        insufficient_pairs = atomic_insufficient_pairs.load();
        conversion_failures = atomic_conversion_failures.load();
        method_failures = atomic_method_failures.load();
        invalid_poses = atomic_invalid_poses.load();

        // 将多线程结果复制到最终的poses容器
        for (const auto &pose : thread_safe_poses)
        {
            poses->push_back(pose);
        }

        // 输出详细的处理统计信息
        LOG_DEBUG_ZH << "[TwoViewEstimator] Processing summary:";
        LOG_DEBUG_EN << "[TwoViewEstimator] Processing summary:";
        LOG_DEBUG_ZH << "  Total view pairs: " << total_view_pairs;
        LOG_DEBUG_EN << "  Total view pairs: " << total_view_pairs;
        LOG_DEBUG_ZH << "  Processed pairs: " << processed_pairs;
        LOG_DEBUG_EN << "  Processed pairs: " << processed_pairs;
        LOG_DEBUG_ZH << "  Successful estimations: " << successful_pairs;
        LOG_DEBUG_EN << "  Successful estimations: " << successful_pairs;

        if (total_view_pairs > 0)
        {
            double success_rate = (double)successful_pairs / total_view_pairs * 100.0;
            LOG_DEBUG_ZH << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%";
            LOG_DEBUG_EN << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%";
        }

        // 详细的失败原因统计
        if (successful_pairs < total_view_pairs)
        {
            LOG_DEBUG_ZH << "[TwoViewEstimator] Failure breakdown:";
            LOG_DEBUG_EN << "[TwoViewEstimator] Failure breakdown:";
            if (empty_matches > 0)
                LOG_DEBUG_ZH << "  Empty matches: " << empty_matches;
            LOG_DEBUG_EN << "  Empty matches: " << empty_matches;
            if (invalid_view_ids > 0)
                LOG_DEBUG_ZH << "  Invalid view IDs: " << invalid_view_ids;
            LOG_DEBUG_EN << "  Invalid view IDs: " << invalid_view_ids;
            if (insufficient_inliers > 0)
                LOG_DEBUG_ZH << "  Insufficient inliers: " << insufficient_inliers;
            LOG_DEBUG_EN << "  Insufficient inliers: " << insufficient_inliers;
            if (insufficient_pairs > 0)
                LOG_DEBUG_ZH << "  Insufficient pairs: " << insufficient_pairs;
            LOG_DEBUG_EN << "  Insufficient pairs: " << insufficient_pairs;
            if (conversion_failures > 0)
                LOG_DEBUG_ZH << "  Conversion failures: " << conversion_failures;
            LOG_DEBUG_EN << "  Conversion failures: " << conversion_failures;
            if (method_failures > 0)
                LOG_DEBUG_ZH << "  Method execution failures: " << method_failures;
            LOG_DEBUG_EN << "  Method execution failures: " << method_failures;
            if (invalid_poses > 0)
                LOG_DEBUG_ZH << "  Invalid poses rejected: " << invalid_poses;
            LOG_DEBUG_EN << "  Invalid poses rejected: " << invalid_poses;
        }

        // 5.5. 添加成功率评估结果到EvaluatorManager
        if (total_view_pairs > 0 && GetOptionAsBool("enable_evaluator"))
        {
            double success_ratio = static_cast<double>(successful_pairs) / static_cast<double>(total_view_pairs);

            // 获取评估提交信息（如果有的话）
            std::string eval_commit = GetOptionAsString("ProfileCommit");

            // 添加成功率评估结果（使用统一的算法名称）
            bool add_result_success = Interface::EvaluatorManager::AddEvaluationResult(
                "RelativePoses",                  // 评估类型
                full_algorithm_name,              // 算法名称（与SetEvaluatorAlgorithm一致）
                eval_commit,                      // 评估提交信息
                "SuccessfulRatio",                // 指标名称
                success_ratio,                    // 成功率值（0.0-1.0）
                "Success rate of pose estimation" // 备注信息
            );

            if (add_result_success && SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] Added SuccessfulRatio evaluation result: "
                             << std::fixed << std::setprecision(3) << success_ratio
                             << " for algorithm " << full_algorithm_name;
                LOG_DEBUG_EN << "[TwoViewEstimator] Added SuccessfulRatio evaluation result: "
                             << std::fixed << std::setprecision(3) << success_ratio
                             << " for algorithm " << full_algorithm_name;
            }
            else if (!add_result_success)
            {
                LOG_WARNING_ZH << "[TwoViewEstimator] Failed to add SuccessfulRatio evaluation result";
                LOG_WARNING_EN << "[TwoViewEstimator] Failed to add SuccessfulRatio evaluation result";
            }
        }

        if (poses->empty())
        {
            PROFILER_END(); // End profiling before return | 返回前结束性能分析
            LOG_WARNING_ZH << "[TwoViewEstimator] Critical Error: Failed to estimate any valid relative poses!";
            LOG_WARNING_EN << "[TwoViewEstimator] Critical Error: Failed to estimate any valid relative poses!";
            LOG_WARNING_ZH << "Possible causes:";
            LOG_WARNING_EN << "Possible causes:";
            if (total_view_pairs == 0)
            {
                LOG_WARNING_ZH << "  - No view pairs in input matches";
                LOG_WARNING_EN << "  - No view pairs in input matches";
            }
            else if (insufficient_inliers + invalid_poses > total_view_pairs * 0.5)
            {
                LOG_WARNING_ZH << "  - Poor data quality: too many outliers or invalid matches";
                LOG_WARNING_EN << "  - Poor data quality: too many outliers or invalid matches";
            }
            else if (insufficient_pairs > total_view_pairs * 0.5)
            {
                LOG_WARNING_ZH << "  - Insufficient match pairs: most pairs have < " << min_num_required_pairs << " matches";
                LOG_WARNING_EN << "  - Insufficient match pairs: most pairs have < " << min_num_required_pairs << " matches";
            }
            else if (method_failures > total_view_pairs * 0.5)
            {
                LOG_WARNING_ZH << "  - Algorithm failures: check " << estimator << " configuration";
                LOG_WARNING_EN << "  - Algorithm failures: check " << estimator << " configuration";
            }
            else if (invalid_view_ids > 0)
            {
                LOG_WARNING_ZH << "  - Data inconsistency: view IDs don't match features data";
                LOG_WARNING_EN << "  - Data inconsistency: view IDs don't match features data";
            }
            else
            {
                LOG_WARNING_ZH << "  - Mixed failures: check input data quality and algorithm parameters";
                LOG_WARNING_EN << "  - Mixed failures: check input data quality and algorithm parameters";
            }
            return nullptr;
        }

        LOG_DEBUG_ZH << "[TwoViewEstimator] Successfully estimated " << poses->size()
                     << " relative poses using " << estimator;
        LOG_DEBUG_EN << "[TwoViewEstimator] Successfully estimated " << poses->size()
                     << " relative poses using " << estimator;

        // 6. 创建输出数据包，包含相对位姿和修改后的匹配数据
        DataPackagePtr data_package_ptr = std::make_shared<DataPackage>();

        // 添加相对位姿数据
        data_package_ptr->AddData("data_relative_poses", data_relative_poses);

        // 添加修改后的匹配数据（包含更新的is_inlier信息）
        data_package_ptr->AddData("data_matches", required_package_["data_matches"]);

        LOG_DEBUG_ZH << "[TwoViewEstimator] Output package contains:";
        LOG_DEBUG_EN << "[TwoViewEstimator] Output package contains:";
        LOG_DEBUG_ZH << "  - data_relative_poses: " << poses->size() << " poses";
        LOG_DEBUG_EN << "  - data_relative_poses: " << poses->size() << " poses";
        LOG_DEBUG_ZH << "  - data_matches: " << matches_ptr->size() << " view pairs (with updated inlier flags)";
        LOG_DEBUG_EN << "  - data_matches: " << matches_ptr->size() << " view pairs (with updated inlier flags)";

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();

        // Print profiling statistics | 打印性能分析统计
        if (SHOULD_LOG(DEBUG))
        {
            PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
        }

        return data_package_ptr;
    }

    RelativePose TwoViewEstimator::ToPoSDKRelativePoseFormat(const RelativePose &pose_result)
    {
        // 创建转换后的相对位姿
        RelativePose converted_pose = pose_result;

        // 应用坐标转换：从OpenGV内部约定转换为PoSDK标准约定
        // OpenGV内部约定: xi = R * xj + t
        // PoSDK标准约定: xj = R * xi + t
        // 转换公式: R_posdk = R_opengv^T, t_posdk = -R_opengv^T * t_opengv

        Matrix3d R_opengv = pose_result.GetRotation();
        Vector3d t_opengv = pose_result.GetTranslation();

        // 执行坐标转换
        converted_pose.SetRotation(R_opengv.transpose());                // R^T
        converted_pose.SetTranslation(-R_opengv.transpose() * t_opengv); // -R^T * t

        // 保持其他属性不变
        converted_pose.SetViewIdI(pose_result.GetViewIdI());
        converted_pose.SetViewIdJ(pose_result.GetViewIdJ());
        converted_pose.SetWeight(pose_result.GetWeight());

        return converted_pose;
    }

    bool TwoViewEstimator::ValidateEstimationQuality(size_t inlier_count, size_t total_matches, const std::string &estimator_name) const
    {
        // 获取质量控制参数
        size_t min_geometric_inliers = GetOptionAsIndexT("min_geometric_inliers", 50);
        double min_inlier_ratio = GetOptionAsFloat("min_inlier_ratio", 0.25);

        // 1. 检查几何内点数量（参考OpenMVG的50个内点要求）
        if (inlier_count < min_geometric_inliers)
        {
            if (log_level_ >= 2)
            {
                LOG_WARNING_ZH << "[" << estimator_name << "] Quality validation failed: insufficient geometric inliers ("
                               << inlier_count << " < " << min_geometric_inliers << ")";
                LOG_WARNING_EN << "[" << estimator_name << "] Quality validation failed: insufficient geometric inliers ("
                               << inlier_count << " < " << min_geometric_inliers << ")";
            }
            return false;
        }

        // 2. 检查内点比例
        double inlier_ratio = static_cast<double>(inlier_count) / static_cast<double>(total_matches);
        if (inlier_ratio < min_inlier_ratio)
        {
            if (log_level_ >= 2)
            {
                LOG_WARNING_ZH << "[" << estimator_name << "] Quality validation failed: low inlier ratio ("
                               << std::fixed << std::setprecision(3) << inlier_ratio
                               << " < " << min_inlier_ratio << ")";
                LOG_WARNING_EN << "[" << estimator_name << "] Quality validation failed: low inlier ratio ("
                               << std::fixed << std::setprecision(3) << inlier_ratio
                               << " < " << min_inlier_ratio << ")";
            }
            return false;
        }

        // 3. 基本的内点数量检查（双视图估计至少需要8个内点）
        if (inlier_count < 6)
        {
            if (log_level_ >= 2)
            {
                LOG_WARNING_ZH << "[" << estimator_name << "] Quality validation failed: insufficient inliers for pose estimation ("
                               << inlier_count << " < 8)";
                LOG_WARNING_EN << "[" << estimator_name << "] Quality validation failed: insufficient inliers for pose estimation ("
                               << inlier_count << " < 8)";
            }
            return false;
        }

        // 4. 所有检查通过
        LOG_DEBUG_ZH << "[" << estimator_name << "] Quality validation passed: "
                     << inlier_count << " inliers ("
                     << std::fixed << std::setprecision(1) << (inlier_ratio * 100.0) << "%)";
        LOG_DEBUG_EN << "[" << estimator_name << "] Quality validation passed: "
                     << inlier_count << " inliers ("
                     << std::fixed << std::setprecision(1) << (inlier_ratio * 100.0) << "%)";

        return true;
    }

    void TwoViewEstimator::SetCurrentViewPairGTData(const ViewPair &view_pair)
    {
        try
        {
            // 检查是否有GT数据
            auto gt_data_it = prior_info_.find("gt_data");
            if (gt_data_it == prior_info_.end() || !gt_data_it->second)
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] No GT data found in prior_info_";
                LOG_DEBUG_EN << "[TwoViewEstimator] No GT data found in prior_info_";
                return;
            }

            DataPtr gt_data = gt_data_it->second;

            // 尝试转换为RelativePoses数据
            auto gt_poses_ptr = GetDataPtr<RelativePoses>(gt_data);
            if (!gt_poses_ptr)
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] GT data is not RelativePoses type: " << gt_data->GetType();
                LOG_DEBUG_EN << "[TwoViewEstimator] GT data is not RelativePoses type: " << gt_data->GetType();
                return;
            }

            // 使用新实现的GetRelativePose函数获取当前view_pair的GT位姿
            Matrix3d R_gt;
            Vector3d t_gt;
            if (gt_poses_ptr->GetRelativePose(view_pair, R_gt, t_gt))
            {
                // 创建单个RelativePose作为GT数据
                RelativePose current_gt_pose(view_pair.first, view_pair.second, R_gt.transpose(), -R_gt.transpose() * t_gt);

                // 包装为DataMap
                auto current_gt_pose_datamap = std::make_shared<DataMap<RelativePose>>(current_gt_pose, "data_relative_pose");
                DataPtr current_gt_pose_data = std::static_pointer_cast<DataIO>(current_gt_pose_datamap);

                // 为method设置GT数据
                if (current_method_)
                {
                    auto method_cast = std::dynamic_pointer_cast<MethodPresetProfiler>(current_method_);
                    if (method_cast)
                    {
                        method_cast->SetGTData(current_gt_pose_data);
                        LOG_DEBUG_ZH << "[TwoViewEstimator] Set GT pose for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                        LOG_DEBUG_EN << "[TwoViewEstimator] Set GT pose for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                    }
                    else
                    {
                        LOG_DEBUG_ZH << "[TwoViewEstimator] Method does not support GT data setting";
                        LOG_DEBUG_EN << "[TwoViewEstimator] Method does not support GT data setting";
                    }
                }
                else
                {
                    LOG_DEBUG_ZH << "[TwoViewEstimator] current_method_ is null";
                    LOG_DEBUG_EN << "[TwoViewEstimator] current_method_ is null";
                }
            }
            else
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] No GT pose found for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
                LOG_DEBUG_EN << "[TwoViewEstimator] No GT pose found for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[TwoViewEstimator] Error setting GT data for view pair ("
                      << view_pair.first << "," << view_pair.second << "): " << e.what();
            LOG_ERROR_ZH << "[TwoViewEstimator] Error setting GT data for view pair ("
                         << view_pair.first << "," << view_pair.second << "): " << e.what();
            LOG_ERROR_EN << "[TwoViewEstimator] Error setting GT data for view pair ("
                         << view_pair.first << "," << view_pair.second << "): " << e.what();
        }
    }

    void TwoViewEstimator::SetCurrentViewPairGTDataForMethod(const ViewPair &view_pair, Interface::MethodPresetPtr method)
    {
        try
        {
            // 检查是否有GT数据
            auto gt_data_it = prior_info_.find("gt_data");
            if (gt_data_it == prior_info_.end() || !gt_data_it->second)
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] No GT data found in prior_info_";
                LOG_DEBUG_EN << "[TwoViewEstimator] No GT data found in prior_info_";
                return;
            }

            DataPtr gt_data = gt_data_it->second;

            // 尝试转换为RelativePoses数据
            auto gt_poses_ptr = GetDataPtr<RelativePoses>(gt_data);
            if (!gt_poses_ptr)
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] GT data is not RelativePoses type: " << gt_data->GetType();
                LOG_DEBUG_EN << "[TwoViewEstimator] GT data is not RelativePoses type: " << gt_data->GetType();
                return;
            }

            // 使用新实现的GetRelativePose函数获取当前view_pair的GT位姿
            Matrix3d R_gt;
            Vector3d t_gt;
            if (gt_poses_ptr->GetRelativePose(view_pair, R_gt, t_gt))
            {
                // 创建单个RelativePose作为GT数据
                RelativePose current_gt_pose(view_pair.first, view_pair.second, R_gt.transpose(), -R_gt.transpose() * t_gt);

                // 包装为DataMap
                auto current_gt_pose_datamap = std::make_shared<DataMap<RelativePose>>(current_gt_pose, "data_relative_pose");
                DataPtr current_gt_pose_data = std::static_pointer_cast<DataIO>(current_gt_pose_datamap);

                // 为指定method设置GT数据
                if (method)
                {
                    auto method_cast = std::dynamic_pointer_cast<MethodPresetProfiler>(method);
                    if (method_cast)
                    {
                        method_cast->SetGTData(current_gt_pose_data);
                        LOG_DEBUG_ZH << "[TwoViewEstimator] Set GT pose for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                        LOG_DEBUG_EN << "[TwoViewEstimator] Set GT pose for view pair ("
                                     << view_pair.first << "," << view_pair.second << ")";
                    }
                    else
                    {
                        LOG_DEBUG_ZH << "[TwoViewEstimator] Method cannot be cast to MethodPresetProfiler for GT data setting";
                        LOG_DEBUG_EN << "[TwoViewEstimator] Method cannot be cast to MethodPresetProfiler for GT data setting";
                    }
                }
            }
            else
            {
                LOG_DEBUG_ZH << "[TwoViewEstimator] No GT pose found for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
                LOG_DEBUG_EN << "[TwoViewEstimator] No GT pose found for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[TwoViewEstimator] Error setting GT data for view pair ("
                      << view_pair.first << "," << view_pair.second << "): " << e.what();
            LOG_ERROR_ZH << "[TwoViewEstimator] Error setting GT data for view pair ("
                         << view_pair.first << "," << view_pair.second << "): " << e.what();
            LOG_ERROR_EN << "[TwoViewEstimator] Error setting GT data for view pair ("
                         << view_pair.first << "," << view_pair.second << "): " << e.what();
        }
    }

    std::shared_ptr<RelativePose> TwoViewEstimator::ApplyPoSDKRefinement(
        const RelativePose &initial_pose,
        const BearingPairs &bearing_pairs,
        const ViewPair &view_pair,
        IdMatches &matches)
    {
        try
        {
            // 1. 创建MethodTwoViewOptimizer实例
            auto optimizer_method = std::dynamic_pointer_cast<MethodPreset>(
                FactoryMethod::Create("method_TwoViewOptimizer"));

            if (!optimizer_method)
            {
                LOG_ERROR_ZH << "[PoSDK Refinement] Failed to create method_TwoViewOptimizer";
                LOG_ERROR_EN << "[PoSDK Refinement] Failed to create method_TwoViewOptimizer";
                return nullptr;
            }

            // 2. 准备输入数据
            // 创建bearing pairs数据
            auto sample_data = std::make_shared<DataSample<BearingPairs>>(bearing_pairs);

            // 创建初始位姿数据
            auto initial_pose_data = std::make_shared<DataMap<RelativePose>>(initial_pose, "data_relative_pose");

            // 创建输入数据包
            auto optimizer_package = std::make_shared<DataPackage>();
            (*optimizer_package)["data_sample"] = sample_data;
            (*optimizer_package)["data_relative_pose"] = initial_pose_data;

            // 3. 设置优化器参数（使用推荐配置）
            MethodOptions optimizer_options;

            // 基本配置
            optimizer_options["view_i"] = std::to_string(view_pair.first);
            optimizer_options["view_j"] = std::to_string(view_pair.second);

            // 优化器配置（使用推荐的ppo_opengv残差函数）
            optimizer_options["optimizer_type"] = "eigen_lm";
            optimizer_options["residual_type"] = "ppo_opengv";
            optimizer_options["loss_type"] = "cauchy";

            // 损失函数阈值（使用推荐配置）
            optimizer_options["huber_threshold_explicit"] = "0.0016"; // Huber阈值：0.0016
            optimizer_options["cauchy_threshold_explicit"] = "0.008"; // Cauchy阈值：0.008

            optimizer_method->SetMethodOptions(optimizer_options);
            optimizer_method->SetRequiredData(optimizer_package);

            if (SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "[PoSDK Refinement] Starting optimization for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
                LOG_DEBUG_EN << "[PoSDK Refinement] Starting optimization for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
                LOG_DEBUG_ZH << "[PoSDK Refinement] Using " << optimizer_options["residual_type"] << " + " << optimizer_options["loss_type"] << " loss with thresholds [" << optimizer_options["huber_threshold_explicit"] << ", " << optimizer_options["cauchy_threshold_explicit"] << "]";
                LOG_DEBUG_EN << "[PoSDK Refinement] Using " << optimizer_options["residual_type"] << " + " << optimizer_options["loss_type"] << " loss with thresholds [" << optimizer_options["huber_threshold_explicit"] << ", " << optimizer_options["cauchy_threshold_explicit"] << "]";
            }

            SetCurrentViewPairGTDataForMethod(view_pair, optimizer_method);
            // 4. 执行优化
            auto result = optimizer_method->Build();
            if (!result)
            {
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "[PoSDK Refinement] Optimization failed for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "[PoSDK Refinement] Optimization failed for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                }
                return nullptr;
            }

            // 5. 提取优化结果
            auto optimized_pose_ptr = GetDataPtr<RelativePose>(result);
            if (!optimized_pose_ptr)
            {
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "[PoSDK Refinement] Failed to extract optimized pose for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "[PoSDK Refinement] Failed to extract optimized pose for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                }
                return nullptr;
            }

            // 6. 验证优化结果的有效性
            if (!optimized_pose_ptr->GetRotation().allFinite() || !optimized_pose_ptr->GetTranslation().allFinite())
            {
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "[PoSDK Refinement] Optimized pose contains non-finite values for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "[PoSDK Refinement] Optimized pose contains non-finite values for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                }
                return nullptr;
            }

            // 检查旋转矩阵的有效性
            double det = optimized_pose_ptr->GetRotation().determinant();
            if (std::abs(det - 1.0) > 0.1)
            {
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "[PoSDK Refinement] Invalid rotation matrix determinant " << det
                                 << " for view pair (" << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "[PoSDK Refinement] Invalid rotation matrix determinant " << det
                                 << " for view pair (" << view_pair.first << "," << view_pair.second << ")";
                }
                return nullptr;
            }

            if (SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "[PoSDK Refinement] Optimization completed successfully for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";
                LOG_DEBUG_EN << "[PoSDK Refinement] Optimization completed successfully for view pair ("
                             << view_pair.first << "," << view_pair.second << ")";

                // 计算优化前后的位姿差异（可选的调试信息）
                Matrix3d R_diff = initial_pose.GetRotation().transpose() * optimized_pose_ptr->GetRotation();
                Vector3d t_diff = optimized_pose_ptr->GetTranslation() - initial_pose.GetTranslation();
                double rotation_diff_deg = std::acos(std::min(1.0, std::max(-1.0, (R_diff.trace() - 1.0) / 2.0))) * 180.0 / M_PI;
                double translation_diff = t_diff.norm();

                LOG_DEBUG_ZH << "[PoSDK Refinement] Refinement impact: rotation_diff="
                             << std::fixed << std::setprecision(6) << rotation_diff_deg
                             << "°, translation_diff=" << translation_diff;
                LOG_DEBUG_EN << "[PoSDK Refinement] Refinement impact: rotation_diff="
                             << std::fixed << std::setprecision(6) << rotation_diff_deg
                             << "°, translation_diff=" << translation_diff;
            }

            // 7. 从MethodTwoViewOptimizer的DataSample中同步内点信息到IdMatches
            // MethodTwoViewOptimizer::Run() 已经通过UpdateInliersAfterOptimization更新了sample_data的best_inliers
            // 现在需要将这些内点信息同步回IdMatches
            UpdateInlierFlagsFromOptimizer(matches, optimizer_method, sample_data);

            // 8. 质量检查：验证优化后的结果质量
            size_t final_inlier_count = 0;
            for (const auto &match : matches)
            {
                if (match.is_inlier)
                    final_inlier_count++;
            }

            // 使用统一的质量验证
            if (!ValidateEstimationQuality(final_inlier_count, matches.size(), "PoSDK_Refinement"))
            {
                if (SHOULD_LOG(DEBUG))
                {
                    LOG_DEBUG_ZH << "[PoSDK Refinement] Quality validation failed after refinement for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                    LOG_DEBUG_EN << "[PoSDK Refinement] Quality validation failed after refinement for view pair ("
                                 << view_pair.first << "," << view_pair.second << ")";
                }
                // 质量验证失败，清除所有内点标志
                for (auto &match : matches)
                {
                    match.is_inlier = false;
                }
                return nullptr; // 返回null表示精细优化失败
            }

            if (SHOULD_LOG(DEBUG))
            {
                LOG_DEBUG_ZH << "[PoSDK Refinement] Final inliers after refinement: " << final_inlier_count
                             << "/" << matches.size() << " ("
                             << std::fixed << std::setprecision(1) << (100.0 * final_inlier_count / matches.size()) << "%)";
                LOG_DEBUG_EN << "[PoSDK Refinement] Final inliers after refinement: " << final_inlier_count
                             << "/" << matches.size() << " ("
                             << std::fixed << std::setprecision(1) << (100.0 * final_inlier_count / matches.size()) << "%)";
            }

            // 返回优化后的位姿（创建shared_ptr）
            return std::make_shared<RelativePose>(*optimized_pose_ptr);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[PoSDK Refinement] Exception during optimization: " << e.what();
            LOG_ERROR_EN << "[PoSDK Refinement] Exception during optimization: " << e.what();
            return nullptr;
        }
    }

    void TwoViewEstimator::UpdateInlierFlagsFromOptimizer(
        IdMatches &matches,
        Interface::MethodPresetPtr optimizer_method,
        std::shared_ptr<DataSample<BearingPairs>> sample_data)
    {
        try
        {
            // 统计更新前的状态
            size_t pre_sync_inliers = 0;
            for (const auto &match : matches)
            {
                if (match.is_inlier)
                    pre_sync_inliers++;
            }

            LOG_DEBUG_ZH << "[UpdateInlierFlagsFromOptimizer] 同步前统计:";
            LOG_DEBUG_ZH << "  IdMatches内点数: " << pre_sync_inliers << "/" << matches.size();

            LOG_DEBUG_EN << "[UpdateInlierFlagsFromOptimizer] Pre-sync statistics:";
            LOG_DEBUG_EN << "  IdMatches inliers: " << pre_sync_inliers << "/" << matches.size();

            // 检查sample_data是否有内点信息
            if (!sample_data || !sample_data->HasBestInliers())
            {
                LOG_WARNING_ZH << "[UpdateInlierFlagsFromOptimizer] DataSample没有内点信息";
                LOG_WARNING_EN << "[UpdateInlierFlagsFromOptimizer] DataSample has no inlier information";
                return;
            }

            // 获取优化器更新后的内点索引
            auto best_inliers = sample_data->GetBestInliers();
            if (!best_inliers || best_inliers->empty())
            {
                LOG_WARNING_ZH << "[UpdateInlierFlagsFromOptimizer] 优化器的内点列表为空";
                LOG_WARNING_EN << "[UpdateInlierFlagsFromOptimizer] Optimizer's inlier list is empty";
                return;
            }

            LOG_DEBUG_ZH << "[UpdateInlierFlagsFromOptimizer] DataSample内点信息:";
            LOG_DEBUG_ZH << "  DataSample总数: " << sample_data->size();
            LOG_DEBUG_ZH << "  DataSample内点数: " << best_inliers->size();

            LOG_DEBUG_EN << "[UpdateInlierFlagsFromOptimizer] DataSample inlier information:";
            LOG_DEBUG_EN << "  DataSample total: " << sample_data->size();
            LOG_DEBUG_EN << "  DataSample inliers: " << best_inliers->size();

            // 重要：这里需要理解数据对应关系
            // sample_data 是从 bearing_pairs 创建的，而 bearing_pairs 是从 matches 的内点转换来的
            // 所以 sample_data 的索引对应的是原始 matches 中的内点索引

            // 首先保存调用前的内点索引映射（在清除标志之前）
            std::vector<size_t> original_inlier_indices;
            for (size_t i = 0; i < matches.size(); ++i)
            {
                if (matches[i].is_inlier) // 保存调用前的内点状态
                {
                    original_inlier_indices.push_back(i);
                }
            }

            // 然后将所有匹配标记为外点
            for (auto &match : matches)
            {
                match.is_inlier = false;
            }

            LOG_DEBUG_ZH << "[UpdateInlierFlagsFromOptimizer] 索引映射:";
            LOG_DEBUG_ZH << "  原始内点索引数: " << original_inlier_indices.size();

            LOG_DEBUG_EN << "[UpdateInlierFlagsFromOptimizer] Index mapping:";
            LOG_DEBUG_EN << "  Original inlier indices count: " << original_inlier_indices.size();

            // 根据优化器的内点结果更新IdMatches
            size_t updated_inliers = 0;
            for (size_t sample_inlier_idx : *best_inliers)
            {
                if (sample_inlier_idx < original_inlier_indices.size())
                {
                    size_t original_match_idx = original_inlier_indices[sample_inlier_idx];
                    if (original_match_idx < matches.size())
                    {
                        matches[original_match_idx].is_inlier = true;
                        updated_inliers++;
                    }
                }
            }

            LOG_DEBUG_ZH << "[UpdateInlierFlagsFromOptimizer] 同步后统计:";
            LOG_DEBUG_ZH << "  更新的内点数: " << updated_inliers;
            LOG_DEBUG_ZH << "  内点变化: " << static_cast<int>(updated_inliers) - static_cast<int>(pre_sync_inliers);

            LOG_DEBUG_EN << "[UpdateInlierFlagsFromOptimizer] Post-sync statistics:";
            LOG_DEBUG_EN << "  Updated inliers: " << updated_inliers;
            LOG_DEBUG_EN << "  Inlier change: " << static_cast<int>(updated_inliers) - static_cast<int>(pre_sync_inliers);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[UpdateInlierFlagsFromOptimizer] 异常: " << e.what();
            LOG_ERROR_EN << "[UpdateInlierFlagsFromOptimizer] Exception: " << e.what();
        }
    }

    void TwoViewEstimator::ShowProgressBar(size_t current, size_t total, const std::string &task_name, int bar_width)
    {
        if (total == 0)
            return;

        double progress = static_cast<double>(current) / total;
        int filled_width = static_cast<int>(progress * bar_width);

        // Create progress bar string | 创建进度条字符串
        std::string bar = "[";
        for (int i = 0; i < bar_width; ++i)
        {
            if (i < filled_width)
                bar += "█";
            else if (i == filled_width && progress > 0)
                bar += "▌";
            else
                bar += " ";
        }
        bar += "]";

        // Calculate percentage | 计算百分比
        double percentage = progress * 100.0;

        // Format output | 格式化输出
        std::ostringstream oss_zh, oss_en;
        oss_zh << task_name << " " << bar << " "
               << std::fixed << std::setprecision(1) << percentage << "% "
               << "(" << current << "/" << total << ")";
        oss_en << task_name << " " << bar << " "
               << std::fixed << std::setprecision(1) << percentage << "% "
               << "(" << current << "/" << total << ")";

        LOG_INFO_ZH << oss_zh.str();
        LOG_INFO_EN << oss_en.str();
    }

} // namespace PluginMethods

REGISTRATION_PLUGIN(PluginMethods::TwoViewEstimator, "TwoViewEstimator")
