/**
 * @file img2matches_fast_mode.cpp
 * @brief Image feature matching fast mode implementation | 图像特征匹配快速模式实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "img2matches_pipeline.hpp"
#include <po_core/po_logger.hpp>
#include <po_core/types.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <chrono>

namespace PluginMethods
{
    using namespace PoSDK;

    DataPtr Img2MatchesPipeline::RunFastMode()
    {

        try
        {
            // 1. Get input image paths | 获取输入图像路径
            DataPtr data_images_ptr = required_package_["data_images"];
            if (!data_images_ptr)
            {
                LOG_ERROR_ZH << "没有输入图像数据！" << std::endl;
                LOG_ERROR_EN << "No input images data!" << std::endl;
                return nullptr;
            }

            // 2. Get image path list | 获取图像路径列表
            ImagePathsPtr image_paths_ptr = GetDataPtr<ImagePaths>(data_images_ptr);
            if (!image_paths_ptr || image_paths_ptr->empty())
            {
                LOG_ERROR_ZH << "图像路径为空！" << std::endl;
                LOG_ERROR_EN << "Empty image paths!" << std::endl;
                return nullptr;
            }

            // 3. Try to get existing feature data, create new if not available | 尝试获取已有的特征数据，如果没有则创建新的
            DataPtr features_data_ptr = nullptr;
            if (required_package_.find("data_features") != required_package_.end() &&
                required_package_["data_features"] != nullptr)
            {
                features_data_ptr = required_package_["data_features"];
                LOG_DEBUG_ZH << "使用已有的特征数据" << std::endl;
                LOG_DEBUG_EN << "Using existing features data" << std::endl;
            }
            else
            {
                features_data_ptr = FactoryData::Create("data_features");
                LOG_DEBUG_ZH << "创建新的特征数据" << std::endl;
                LOG_DEBUG_EN << "Creating new features data" << std::endl;
            }

            // 4. Create matching result data | 创建匹配结果数据
            DataPtr matches_data_ptr = FactoryData::Create("data_matches");
            if (!features_data_ptr || !matches_data_ptr)
            {
                LOG_ERROR_ZH << "创建数据容器失败！" << std::endl;
                LOG_ERROR_EN << "Failed to create data containers!" << std::endl;
                return nullptr;
            }

            FeaturesInfoPtr features_info_ptr = GetDataPtr<FeaturesInfo>(features_data_ptr);
            MatchesPtr matches_ptr = GetDataPtr<Matches>(matches_data_ptr);

            // 5. Process feature extraction and matching | 处理特征提取和匹配
            std::vector<std::vector<cv::KeyPoint>> all_keypoints;
            std::vector<cv::Mat> all_descriptors;
            std::vector<IndexT> all_view_ids;
            std::vector<std::string> all_image_paths;

            // 内存优化：只有LightGlue才需要缓存图像数据，SIFT+FLANN不需要
            std::vector<cv::Mat> all_images;                // Image data needed by LightGlue | LightGlue需要的图像数据
            std::vector<cv::Mat> *all_images_ptr = nullptr; // 默认不缓存图像

            // Check if there is existing feature data | 检查是否有已存在的特征数据
            bool has_existing_features = features_info_ptr && !features_info_ptr->empty();

            // 内存优化：根据匹配器类型决定是否缓存图像数据
            if (params_.matching.matcher_type == MatcherType::LIGHTGLUE)
            {
                all_images_ptr = &all_images;
                LOG_INFO_ZH << "使用LightGlue匹配器，启用图像缓存";
                LOG_INFO_EN << "Using LightGlue matcher, enabling image caching";
            }
            else
            {
                LOG_INFO_ZH << "使用传统匹配器 (SIFT+FLANN)，禁用图像缓存以节省内存";
                LOG_INFO_EN << "Using traditional matcher (SIFT+FLANN), disabling image caching to save memory";

                // 估算内存节省量
                size_t estimated_image_count = image_paths_ptr ? image_paths_ptr->size() : 0;
                size_t estimated_memory_saved = estimated_image_count * 2 * 1024 * 1024; // 假设每张图像2MB
                LOG_INFO_ZH << "预计节省内存: " << (estimated_memory_saved / 1024.0 / 1024.0) << " MB (基于 " << estimated_image_count << " 张图像)";
                LOG_INFO_EN << "Estimated memory saved: " << (estimated_memory_saved / 1024.0 / 1024.0) << " MB (based on " << estimated_image_count << " images)";
            }

            // 5. Feature processing (core computation starts) | 特征处理（核心计算开始）
            LOG_INFO_ZH << "========== 开始特征提取+匹配流程 ==========";
            LOG_INFO_EN << "========== Starting Feature Extraction + Matching ==========";

            // Get profiling configuration | 获取性能分析配置
            std::string metrics_config = GetOptionAsString("metrics_config", "time");

            // Start profiling for the entire RunFastMode function | 开始对整个RunFastMode函数进行性能分析
            {

                POSDK_START(enable_profiling_, metrics_config);
                PROFILER_STAGE("Feature Extraction");
                if (has_existing_features)
                {
                    ProcessExistingFeatures(features_info_ptr, all_keypoints, all_descriptors,
                                            all_view_ids, all_image_paths, all_images_ptr);
                }
                else
                {
                    ExtractNewFeatures(image_paths_ptr, features_info_ptr, all_keypoints,
                                       all_descriptors, all_view_ids, all_image_paths, all_images_ptr);
                }
                PROFILER_END();
                // Print profiling statistics | 打印性能分析统计
                if (SHOULD_LOG(DEBUG))
                {
                    PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
                }
            }
            LOG_INFO_ZH << "========== 特征提取完成，开始匹配阶段 ==========";
            LOG_INFO_EN << "========== Feature Extraction Complete, Starting Matching ==========";

            // 6. Perform pairwise matching (core computation step) | 执行全对匹配（核心计算步骤）
            size_t successful_pairs = 0;

            // 根据num_threads选择单线程或多线程版本
            {

                POSDK_START(enable_profiling_, metrics_config);
                PROFILER_STAGE("Matching");
                if (params_.base.num_threads > 1)
                {
                    LOG_INFO_ZH << "使用多线程匹配版本 (num_threads=" << params_.base.num_threads << ")";
                    LOG_INFO_EN << "Using multi-threaded matching version (num_threads=" << params_.base.num_threads << ")";
                    successful_pairs = PerformPairwiseMatchingMultiThreads(all_descriptors, all_view_ids, matches_ptr, &all_keypoints, all_images_ptr);
                }
                else
                {
                    LOG_INFO_ZH << "使用单线程匹配版本 (num_threads=" << params_.base.num_threads << ")";
                    LOG_INFO_EN << "Using single-threaded matching version (num_threads=" << params_.base.num_threads << ")";
                    successful_pairs = PerformPairwiseMatching(all_descriptors, all_view_ids, matches_ptr, &all_keypoints, all_images_ptr);
                }
                PROFILER_END();
                // Print profiling statistics | 打印性能分析统计
                if (SHOULD_LOG(DEBUG))
                {
                    PROFILER_PRINT_STATS(enable_profiling_); // 当前会话统计
                }
            }

            // 7. Export results | 导出结果
            ExportResults(features_data_ptr, matches_data_ptr);

            LOG_INFO_ZH << "快速模式成功完成，成功匹配对数: " << successful_pairs << std::endl;
            LOG_INFO_EN << "Fast mode completed successfully with " << successful_pairs << " successful matches" << std::endl;

            // 8. Create data package containing feature and matching data | 创建包含特征和匹配数据的数据包
            auto data_package = std::make_shared<DataPackage>();
            data_package->AddData("data_features", features_data_ptr);
            data_package->AddData("data_matches", matches_data_ptr);

            LOG_INFO_ZH << "========== 匹配完成 ==========";
            LOG_INFO_EN << "========== Matching Complete ==========";

            return std::static_pointer_cast<DataIO>(data_package);
        }
        catch (const std::exception &e)
        {
            // Note: PROFILER_END will be called automatically when _profiler_session_ goes out of scope | 注意：当_profiler_session_离开作用域时会自动调用PROFILER_END
            LOG_ERROR_ZH << "RunFastMode中发生错误: " << e.what() << std::endl;
            LOG_ERROR_EN << "Error in RunFastMode: " << e.what() << std::endl;
            return nullptr;
        }
    }

} // namespace PluginMethods
