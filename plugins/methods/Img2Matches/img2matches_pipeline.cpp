/**
 * @file img2matches_pipeline.cpp
 * @brief 图像特征匹配处理流水线实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "img2matches_pipeline.hpp"
#include "FASTCASCADEHASHINGL2.hpp"
#include "LightGlueMatcher.hpp"
#include <po_core/types.hpp>
#include <po_core/po_logger.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <common/image_viewer/image_viewer.hpp>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <mutex>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace common;

    Img2MatchesPipeline::Img2MatchesPipeline()
    {
        // Add debug output | 添加调试输出
        LOG_DEBUG_ZH << "初始化 Img2MatchesPipeline..." << std::endl;
        LOG_DEBUG_EN << "Initializing Img2MatchesPipeline..." << std::endl;

        // Set required input data packages | 设置需要的输入数据包
        required_package_["data_features"] = nullptr;

        // Initialize default configuration path | 初始化默认配置
        InitializeDefaultConfigPath();
    }

    DataPtr Img2MatchesPipeline::Run()
    {
        try
        {
            // 1. Load configuration at runtime | 1. 在运行时加载配置
            LoadConfigurationAtRuntime();

            // 2. Validate parameters | 2. 验证参数
            if (!params_.Validate(this))
            {
                LOG_ERROR_ZH << "参数验证失败" << std::endl;
                LOG_ERROR_EN << "Parameter validation failed" << std::endl;
                return nullptr;
            }
            DisplayConfigInfo();
            // 3. Display parameter summary | 3. 显示参数摘要
            LOG_DEBUG_ZH << "显示参数摘要" << std::endl;
            LOG_DEBUG_EN << "Displaying parameter summary" << std::endl;
            params_.PrintSummary(this);

            // 4. Choose execution method based on run mode | 4. 根据运行模式选择执行方式
            switch (params_.base.run_mode)
            {
            case RunMode::Viewer:
                return RunViewerMode();
            case RunMode::Fast:
            default:
                return RunFastMode();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[Img2MatchesPipeline] 运行时错误: " << e.what() << std::endl;
            LOG_ERROR_EN << "[Img2MatchesPipeline] Error in Run: " << e.what() << std::endl;
            return nullptr;
        }
        catch (...)
        {
            LOG_ERROR_ZH << "[Img2MatchesPipeline] 未知错误" << std::endl;
            LOG_ERROR_EN << "[Img2MatchesPipeline] Unknown error in Run" << std::endl;
            return nullptr;
        }
    }

    std::vector<cv::DMatch> Img2MatchesPipeline::MatchFeatures(
        const cv::Mat &descriptors1, const cv::Mat &descriptors2)
    {
        std::vector<cv::DMatch> matches;
        if (descriptors1.empty() || descriptors2.empty())
        {
            LOG_ERROR_ZH << "描述子为空!" << std::endl;
            LOG_ERROR_EN << "Empty descriptors!" << std::endl;
            return matches;
        }

        // Add debug information: Check descriptor type and size | 添加调试信息：检查描述子类型和尺寸
        LOG_DEBUG_ZH << "描述子1: " << descriptors1.rows << "x" << descriptors1.cols
                     << " 类型=" << descriptors1.type() << " (CV_32F=" << CV_32F << ")" << std::endl;
        LOG_DEBUG_EN << "Descriptor1: " << descriptors1.rows << "x" << descriptors1.cols
                     << " type=" << descriptors1.type() << " (CV_32F=" << CV_32F << ")" << std::endl;
        LOG_DEBUG_ZH << "描述子2: " << descriptors2.rows << "x" << descriptors2.cols
                     << " 类型=" << descriptors2.type() << " (CV_32F=" << CV_32F << ")" << std::endl;
        LOG_DEBUG_EN << "Descriptor2: " << descriptors2.rows << "x" << descriptors2.cols
                     << " type=" << descriptors2.type() << " (CV_32F=" << CV_32F << ")" << std::endl;

        try
        {
            // Choose the appropriate matcher based on descriptor type | 根据描述子类型选择合适的匹配器
            cv::Ptr<cv::DescriptorMatcher> matcher;

            switch (params_.matching.matcher_type)
            {
            case MatcherType::LIGHTGLUE:
            {
                LOG_WARNING_ZH << "LightGlue 匹配需要图像数据，在此上下文中不可用。" << std::endl;
                LOG_WARNING_ZH << "回退到 FASTCASCADEHASHINGL2 匹配器以确保兼容性。" << std::endl;
                LOG_WARNING_EN << "LightGlue matching requires image data, which is not available in this context." << std::endl;
                LOG_WARNING_EN << "Falling back to FASTCASCADEHASHINGL2 matcher for compatibility." << std::endl;

                // Fallback to FASTCASCADEHASHINGL2 matcher | 降级到 FASTCASCADEHASHINGL2 匹配器
                if (FastCascadeHashingL2Matcher::IsCompatible(descriptors1) &&
                    FastCascadeHashingL2Matcher::IsCompatible(descriptors2))
                {
                    bool success = FastCascadeHashingL2Matcher::Match(
                        descriptors1, descriptors2, matches,
                        params_.matching.ratio_thresh, params_.matching.cross_check);

                    if (success)
                    {
                        LOG_DEBUG_ZH << "回退匹配成功，找到 " << matches.size() << " 个匹配项" << std::endl;
                        LOG_DEBUG_EN << "Fallback matching successful, found " << matches.size() << " matches" << std::endl;
                        return matches;
                    }
                }

                // If still fails, proceed to default brute force matcher | 如果还是失败，继续到默认的暴力匹配器
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                break;
            }
            case MatcherType::FASTCASCADEHASHINGL2:
            {
                // Check if descriptor type is compatible with FastCascadeHashingL2 | 检查描述子类型是否兼容 FastCascadeHashingL2
                if (!FastCascadeHashingL2Matcher::IsCompatible(descriptors1) ||
                    !FastCascadeHashingL2Matcher::IsCompatible(descriptors2))
                {
                    LOG_ERROR_ZH << "FASTCASCADEHASHINGL2 匹配器需要 CV_32F 描述子。得到类型: "
                                 << descriptors1.type() << " 和 " << descriptors2.type() << std::endl;
                    LOG_ERROR_ZH << "回退到 BruteForce 匹配器" << std::endl;
                    LOG_ERROR_EN << "FASTCASCADEHASHINGL2 matcher requires CV_32F descriptors. Got types: "
                                 << descriptors1.type() << " and " << descriptors2.type() << std::endl;
                    LOG_ERROR_EN << "Falling back to BruteForce matcher" << std::endl;
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                }
                else
                {
                    // Use FastCascadeHashingL2 matcher for static matching | 使用 FastCascadeHashingL2 匹配器进行静态匹配
                    LOG_DEBUG_ZH << "使用 FASTCASCADEHASHINGL2 匹配器，比例=" << params_.matching.ratio_thresh << std::endl;
                    LOG_DEBUG_EN << "Using FASTCASCADEHASHINGL2 matcher with ratio=" << params_.matching.ratio_thresh << std::endl;

                    bool success = FastCascadeHashingL2Matcher::Match(
                        descriptors1, descriptors2, matches,
                        params_.matching.ratio_thresh, params_.matching.cross_check);

                    if (!success)
                    {
                        LOG_ERROR_ZH << "FastCascadeHashingL2 匹配失败，回退到 BruteForce" << std::endl;
                        LOG_ERROR_EN << "FastCascadeHashingL2 matching failed, falling back to BruteForce" << std::endl;
                        matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                    }
                    else
                    {
                        // Return matching results directly, no further processing needed | 直接返回匹配结果，无需进一步处理
                        LOG_DEBUG_ZH << "FastCascadeHashingL2 匹配成功，找到 " << matches.size() << " 个匹配项" << std::endl;
                        LOG_DEBUG_EN << "FastCascadeHashingL2 matching successful, found " << matches.size() << " matches" << std::endl;
                        return matches;
                    }
                }
                break;
            }
            case MatcherType::FLANN:
            {
                // Check if descriptor type is compatible with FLANN | 检查描述子类型是否兼容 FLANN
                if (descriptors1.type() != CV_32F || descriptors2.type() != CV_32F)
                {
                    LOG_ERROR_ZH << "FLANN 匹配器需要 CV_32F 描述子。得到类型: "
                                 << descriptors1.type() << " 和 " << descriptors2.type() << std::endl;
                    LOG_ERROR_ZH << "回退到 BruteForce 匹配器" << std::endl;
                    LOG_ERROR_EN << "FLANN matcher requires CV_32F descriptors. Got types: "
                                 << descriptors1.type() << " and " << descriptors2.type() << std::endl;
                    LOG_ERROR_EN << "Falling back to BruteForce matcher" << std::endl;
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                }
                else
                {
                    if (params_.flann.use_advanced_control)
                    {
                        // Create matcher with advanced FLANN parameter control | 使用高级 FLANN 参数控制创建匹配器
                        LOG_INFO_ZH << "使用高级 FLANN 参数控制" << std::endl;
                        LOG_INFO_EN << "Using advanced FLANN parameter control" << std::endl;
                        matcher = CreateFLANNMatcher();
                    }
                    else
                    {
                        // Create matcher with OpenCV default FLANN parameters | 使用 OpenCV 默认 FLANN 参数创建匹配器
                        LOG_INFO_ZH << "使用 OpenCV 默认 FLANN 参数" << std::endl;
                        LOG_INFO_EN << "Using OpenCV default FLANN parameters" << std::endl;
                        matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
                    }
                }
                break;
            }
            case MatcherType::BF_HAMMING:
            {
                // Verify if descriptor type is suitable for Hamming distance matching | 验证描述子类型是否适合汉明距离匹配
                if (descriptors1.type() == CV_8U && descriptors2.type() == CV_8U)
                {
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE_HAMMING);
                    LOG_DEBUG_ZH << "对二进制描述子 (CV_8U) 使用 BF_HAMMING 匹配器" << std::endl;
                    LOG_DEBUG_EN << "Using BF_HAMMING matcher for binary descriptors (CV_8U)" << std::endl;
                }
                else
                {
                    LOG_ERROR_ZH << "BF_HAMMING 匹配器需要 CV_8U 描述子。得到类型: "
                                 << descriptors1.type() << " 和 " << descriptors2.type() << std::endl;
                    LOG_ERROR_ZH << "回退到 BruteForce 匹配器" << std::endl;
                    LOG_ERROR_EN << "BF_HAMMING matcher requires CV_8U descriptors. Got types: "
                                 << descriptors1.type() << " and " << descriptors2.type() << std::endl;
                    LOG_ERROR_EN << "Falling back to BruteForce matcher" << std::endl;
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                }
                break;
            }
            case MatcherType::BF_NORM_L1:
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE_L1);
                break;
            case MatcherType::BF:
            default:
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                break;
            }

            // Perform feature matching | 执行特征匹配
            LOG_DEBUG_ZH << "匹配参数: cross_check=" << params_.matching.cross_check
                         << ", ratio_thresh=" << params_.matching.ratio_thresh
                         << ", max_matches=" << params_.matching.max_matches << std::endl;
            LOG_DEBUG_EN << "Matching with: cross_check=" << params_.matching.cross_check
                         << ", ratio_thresh=" << params_.matching.ratio_thresh
                         << ", max_matches=" << params_.matching.max_matches << std::endl;

            if (params_.matching.cross_check)
            {
                matcher->match(descriptors1, descriptors2, matches);
                LOG_DEBUG_ZH << "交叉检查匹配找到 " << matches.size() << " 个匹配项" << std::endl;
                LOG_DEBUG_EN << "Cross-check matching found " << matches.size() << " matches" << std::endl;
            }
            else
            {
                std::vector<std::vector<cv::DMatch>> knn_matches;
                matcher->knnMatch(descriptors1, descriptors2, knn_matches, 2);
                LOG_DEBUG_ZH << "KNN 匹配找到 " << knn_matches.size() << " 个候选对" << std::endl;
                LOG_DEBUG_EN << "KNN matching found " << knn_matches.size() << " candidate pairs" << std::endl;

                // Apply ratio test | 应用比率测试
                size_t passed_ratio_test = 0;
                for (const auto &knn_match : knn_matches)
                {
                    if (knn_match.size() >= 2 &&
                        knn_match[0].distance < params_.matching.ratio_thresh * knn_match[1].distance)
                    {
                        matches.push_back(knn_match[0]);
                        passed_ratio_test++;
                    }
                }
                LOG_DEBUG_ZH << "比率测试通过: " << passed_ratio_test << " 个匹配项" << std::endl;
                LOG_DEBUG_EN << "Ratio test passed: " << passed_ratio_test << " matches" << std::endl;
            }

            // Limit the number of matches | 限制匹配数量
            if (params_.matching.max_matches > 0 && matches.size() > params_.matching.max_matches)
            {
                std::sort(matches.begin(), matches.end());
                matches.resize(params_.matching.max_matches);
                LOG_DEBUG_ZH << "限制匹配项到 " << params_.matching.max_matches << std::endl;
                LOG_DEBUG_EN << "Limited matches to " << params_.matching.max_matches << std::endl;
            }

            LOG_DEBUG_ZH << "最终匹配数量: " << matches.size() << std::endl;
            LOG_DEBUG_EN << "Final match count: " << matches.size() << std::endl;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "MatchFeatures 中出错: " << e.what() << std::endl;
            LOG_ERROR_EN << "Error in MatchFeatures: " << e.what() << std::endl;
        }

        return matches;
    }

    std::vector<cv::DMatch> Img2MatchesPipeline::MatchFeaturesThreadSafe(
        const cv::Mat &descriptors1, const cv::Mat &descriptors2,
        IndexT view_id1, IndexT view_id2)
    {
        std::vector<cv::DMatch> matches;
        if (descriptors1.empty() || descriptors2.empty())
        {
            return matches;
        }

        // 为每个view pair设置确定性的随机种子，确保结果可重复
        // 使用更复杂的种子计算，避免简单的线性组合可能导致的冲突
        uint32_t deterministic_seed = 12345;
        deterministic_seed ^= (view_id1 << 16) | view_id2;         // 位运算组合，减少冲突
        deterministic_seed ^= (view_id1 * 7919 + view_id2 * 7927); // 使用质数进一步混合
        cv::setRNGSeed(deterministic_seed);

        try
        {
            // 为每个线程创建独立的匹配器实例，确保线程安全
            cv::Ptr<cv::DescriptorMatcher> matcher;

            switch (params_.matching.matcher_type)
            {
            case MatcherType::FLANN:
            {
                // 检查描述子类型兼容性
                if (descriptors1.type() != CV_32F || descriptors2.type() != CV_32F)
                {
                    LOG_ERROR_ZH << "FLANN 匹配器需要 CV_32F 描述子。得到类型: "
                                 << descriptors1.type() << " 和 " << descriptors2.type();
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                }
                else
                {
                    if (params_.flann.use_advanced_control)
                    {
                        // 创建具有高级参数控制的FLANN匹配器
                        // 为了确保线程安全和确定性，在每次匹配前重新设置种子
                        cv::setRNGSeed(deterministic_seed);
                        matcher = CreateFLANNMatcher();
                    }
                    else
                    {
                        // 使用OpenCV默认FLANN参数，同样设置确定性种子
                        cv::setRNGSeed(deterministic_seed);
                        matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
                    }
                }
                break;
            }
            case MatcherType::FASTCASCADEHASHINGL2:
            {
                // 使用FastCascadeHashingL2进行静态匹配
                if (FastCascadeHashingL2Matcher::IsCompatible(descriptors1) &&
                    FastCascadeHashingL2Matcher::IsCompatible(descriptors2))
                {
                    bool success = FastCascadeHashingL2Matcher::Match(
                        descriptors1, descriptors2, matches,
                        params_.matching.ratio_thresh, params_.matching.cross_check);
                    if (success)
                    {
                        return matches;
                    }
                }
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                break;
            }
            case MatcherType::BF_HAMMING:
            {
                if (descriptors1.type() == CV_8U && descriptors2.type() == CV_8U)
                {
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE_HAMMING);
                }
                else
                {
                    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                }
                break;
            }
            case MatcherType::BF_NORM_L1:
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE_L1);
                break;
            case MatcherType::BF:
            default:
                matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
                break;
            }

            // 执行匹配
            if (params_.matching.cross_check)
            {
                matcher->match(descriptors1, descriptors2, matches);
            }
            else
            {
                std::vector<std::vector<cv::DMatch>> knn_matches;
                matcher->knnMatch(descriptors1, descriptors2, knn_matches, 2);

                // 应用比率测试
                for (const auto &knn_match : knn_matches)
                {
                    if (knn_match.size() >= 2 &&
                        knn_match[0].distance < params_.matching.ratio_thresh * knn_match[1].distance)
                    {
                        matches.push_back(knn_match[0]);
                    }
                }
            }

            // 限制匹配数量
            if (params_.matching.max_matches > 0 && matches.size() > params_.matching.max_matches)
            {
                std::sort(matches.begin(), matches.end());
                matches.resize(params_.matching.max_matches);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "MatchFeaturesThreadSafe 中出错: " << e.what();
            LOG_ERROR_EN << "Error in MatchFeaturesThreadSafe: " << e.what();
            matches.clear();
        }

        return matches;
    }

    std::vector<cv::DMatch> Img2MatchesPipeline::MatchFeaturesWithLightGlue(
        const cv::Mat &img1, const cv::Mat &img2,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const cv::Mat &descriptors1, const cv::Mat &descriptors2)
    {
        std::vector<cv::DMatch> matches;

        try
        {
            LOG_DEBUG_ZH << "使用 LightGlue 深度学习匹配器" << std::endl;
            LOG_DEBUG_EN << "Using LightGlue deep learning matcher" << std::endl;

            // Check input validity | 检查输入有效性
            if (img1.empty() || img2.empty() || keypoints1.empty() || keypoints2.empty() ||
                descriptors1.empty() || descriptors2.empty())
            {
                LOG_ERROR_ZH << "LightGlue 匹配的输入数据无效" << std::endl;
                LOG_ERROR_EN << "Invalid input data for LightGlue matching" << std::endl;
                return matches;
            }

            // Perform matching with LightGlue | 使用 LightGlue 进行匹配
            bool success = LightGlueMatcher::Match(
                params_.lightglue,
                img1, img2,
                keypoints1, keypoints2,
                descriptors1, descriptors2,
                matches);

            if (success)
            {
                LOG_DEBUG_ZH << "LightGlue 匹配成功，找到 " << matches.size() << " 个匹配项" << std::endl;
                LOG_DEBUG_EN << "LightGlue matching successful, found " << matches.size() << " matches" << std::endl;

                // Limit the number of matches if a limit is set | 限制匹配数量（如果设置了限制）
                if (params_.matching.max_matches > 0 && matches.size() > params_.matching.max_matches)
                {
                    std::sort(matches.begin(), matches.end());
                    matches.resize(params_.matching.max_matches);
                    LOG_DEBUG_ZH << "限制匹配项到 " << params_.matching.max_matches << std::endl;
                    LOG_DEBUG_EN << "Limited matches to " << params_.matching.max_matches << std::endl;
                }
            }
            else
            {
                LOG_ERROR_ZH << "LightGlue 匹配失败，回退到 FASTCASCADEHASHINGL2" << std::endl;
                LOG_ERROR_EN << "LightGlue matching failed, falling back to FASTCASCADEHASHINGL2" << std::endl;

                // Fallback to traditional matcher | 降级到传统匹配器
                matches = MatchFeatures(descriptors1, descriptors2);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "LightGlue 匹配中出现异常: " << e.what() << std::endl;
            LOG_ERROR_EN << "Exception in LightGlue matching: " << e.what() << std::endl;

            // Fallback to traditional matcher | 降级到传统匹配器
            matches = MatchFeatures(descriptors1, descriptors2);
        }

        return matches;
    }

    void Img2MatchesPipeline::VisualizeMatches(
        const cv::Mat &img1, const cv::Mat &img2,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const std::vector<cv::DMatch> &matches,
        const std::string &window_name)
    {
        // Show matches in image viewer | 在图像查看器中显示匹配结果
        auto &viewer = ImageViewer::Instance();
        viewer.ShowMatches(img1, img2, keypoints1, keypoints2, matches, window_name);
    }

    std::pair<size_t, size_t> Img2MatchesPipeline::ParseViewPair()
    {
        // Parse view pair indices from parameters | 从参数中解析视图对索引
        return {params_.visualization.show_view_pair_i, params_.visualization.show_view_pair_j};
    }

    void Img2MatchesPipeline::ValidateViewPairIndices(size_t i, size_t j, size_t max_size)
    {
        // Validate view pair indices | 验证视图对索引
        if (i >= max_size || j >= max_size)
        {
            throw std::runtime_error("View pair indices out of range | 视图对索引超出范围");
        }
        if (i == j)
        {
            throw std::runtime_error("View pair indices must be different | 视图对索引必须不同");
        }
    }

    void Img2MatchesPipeline::ProcessExistingFeatures(
        FeaturesInfoPtr features_info_ptr,
        std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
        std::vector<cv::Mat> &all_descriptors,
        std::vector<IndexT> &all_view_ids,
        std::vector<std::string> &all_image_paths,
        std::vector<cv::Mat> *all_images)
    {
        // Using existing features, re-extracting descriptors for matching
        // 使用已有特征，但重新提取描述子用于匹配
        LOG_INFO_ZH << "使用已有特征，重新提取描述子用于匹配";
        LOG_INFO_EN << "Using existing features, re-extracting descriptors for matching";

        // Progress tracking setup | 进度跟踪设置
        const size_t total_views = features_info_ptr->size();
        size_t processed_views = 0;
        size_t last_progress_milestone = 0; // Track last shown progress milestone | 跟踪上次显示的进度里程碑

        // FeaturesInfo is continuous, iterate directly | FeaturesInfo是连续的，直接遍历即可
        for (IndexT view_id = 0; view_id < features_info_ptr->size(); ++view_id)
        {
            const auto &image_feature = (*features_info_ptr)[view_id];

            // Check if feature information is valid | 检查特征信息是否有效
            if (!image_feature || image_feature->GetFeaturePoints().empty() || image_feature->GetImagePath().empty())
            {
                LOG_WARNING_ZH << "视图ID " << view_id << " 的特征为空";
                LOG_WARNING_ZH << "Empty features for view_id " << view_id;
                continue;
            }

            // Read image | 读取图像
            cv::Mat img = cv::imread(image_feature->GetImagePath(), cv::IMREAD_GRAYSCALE);
            if (img.empty())
            {
                LOG_WARNING_ZH << "无法加载图像: " << image_feature->GetImagePath();
                LOG_WARNING_ZH << "Failed to load image: " << image_feature->GetImagePath();
                continue;
            }

            // 内存优化：只有LightGlue才需要缓存图像数据，SIFT+FLANN不需要
            if (params_.matching.matcher_type == MatcherType::LIGHTGLUE && all_images != nullptr)
            {
                all_images->push_back(img.clone());
                LOG_DEBUG_ZH << "缓存图像数据用于LightGlue匹配 (视图ID: " << view_id << ")";
                LOG_DEBUG_EN << "Caching image data for LightGlue matching (view_id: " << view_id << ")";
            }
            else if (all_images != nullptr)
            {
                LOG_DEBUG_ZH << "跳过图像缓存以节省内存 (视图ID: " << view_id << ", 匹配器: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "其他") << ")";
                LOG_DEBUG_EN << "Skipping image caching to save memory (view_id: " << view_id << ", matcher: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "other") << ")";
            }

            // Apply first_octave image preprocessing | 应用first_octave图像预处理
            cv::Mat processed_img = ApplyFirstOctaveProcessing(img);

            // Restore keypoints from existing feature information | 从已有特征信息中恢复特征点
            std::vector<cv::KeyPoint> keypoints;
            OpenCVConverter::FeaturesInfo2CVFeatures(*image_feature, keypoints);

            // Adjust keypoint coordinates if image scaling was applied | 如果进行了图像缩放，需要调整特征点坐标
            if (params_.sift.first_octave != 0)
            {
                // For ProcessExistingFeatures, coordinate adjustment direction is opposite to ExtractNewFeatures
                // 对于ProcessExistingFeatures，坐标调整方向与ExtractNewFeatures相反
                // Because keypoint coordinates are based on the original image, need to adjust to the scaled image coordinate system
                // 因为特征点坐标是基于原图的，需要调整到缩放后图像的坐标系
                float scale_factor = 1.0f;
                switch (params_.sift.first_octave)
                {
                case -1:
                    // Image upsampled 2x, keypoint coordinates also need to be scaled up 2x
                    // 图像上采样2倍，特征点坐标也要放大2倍
                    scale_factor = 2.0f;
                    break;
                case 1:
                    // Image downsampled 0.5x, keypoint coordinates also need to be scaled down 0.5x
                    // 图像下采样0.5倍，特征点坐标也要缩小0.5倍
                    scale_factor = 0.5f;
                    break;
                default:
                    // first_octave = 0, no adjustment needed | first_octave = 0，无需调整
                    break;
                }

                if (scale_factor != 1.0f)
                {
                    LOG_DEBUG_ZH << "调整 " << keypoints.size() << " 个已有特征点的缩放因子: " << scale_factor << " (first_octave=" << params_.sift.first_octave << ")";
                    LOG_DEBUG_EN << "Adjusting " << keypoints.size() << " existing keypoints with scale factor: " << scale_factor << " (first_octave=" << params_.sift.first_octave << ")";

                    for (auto &kp : keypoints)
                    {
                        kp.pt.x *= scale_factor;
                        kp.pt.y *= scale_factor;
                        kp.size *= scale_factor;
                    }
                }
            }

            // Recompute descriptors (using the same keypoint positions) | 重新计算描述子（使用相同的特征点位置）
            cv::Mat descriptors;

            // Process differently based on detector type | 根据检测器类型进行不同的处理
            if (params_.base.detector_type == "SIFT")
            {
                // Create SIFT detector to recompute descriptors | 创建SIFT检测器用于重新计算描述子
                cv::Ptr<cv::SIFT> sift_detector = cv::SIFT::create(
                    params_.sift.nfeatures,             // Number of features | 特征点数量
                    params_.sift.nOctaveLayers,         // Number of octave layers | octave层数
                    params_.sift.contrastThreshold,     // Contrast threshold | 对比度阈值
                    params_.sift.edgeThreshold,         // Edge threshold | 边缘阈值
                    params_.sift.sigma,                 // Gaussian blur sigma | 高斯模糊系数
                    params_.sift.enable_precise_upscale // Enable precise upscaling | 是否启用精确上采样
                );

                // Compute descriptors only, do not redetect keypoints (using preprocessed image)
                // 只计算描述子，不重新检测特征点（使用预处理后的图像）
                sift_detector->compute(processed_img, keypoints, descriptors);

                // Ensure descriptor type is CV_32F (required by matcher) | 确保描述子类型为CV_32F（匹配器要求）
                if (!descriptors.empty() && descriptors.type() != CV_32F)
                {
                    LOG_DEBUG_ZH << "将描述子从类型 " << descriptors.type() << " 转换为 CV_32F 以确保兼容性";
                    LOG_DEBUG_EN << "Converting descriptors from type " << descriptors.type() << " to CV_32F for compatibility";
                    descriptors.convertTo(descriptors, CV_32F);
                }

                // Apply RootSIFT normalization (if enabled) | 应用RootSIFT归一化（如果启用）
                if (params_.sift.root_sift && !descriptors.empty())
                {
                    ApplyRootSIFTNormalization(descriptors);
                }
            }
            else
            {
                LOG_ERROR_ZH << "不支持的检测器类型用于描述子重新计算: " << params_.base.detector_type;
                LOG_ERROR_EN << "Unsupported detector type for descriptor recomputation: " << params_.base.detector_type;
                continue;
            }

            // Add debug information | 添加调试信息
            LOG_DEBUG_ZH << "为视图ID " << view_id << " 重新计算描述子: " << descriptors.rows << "x" << descriptors.cols << " 类型=" << descriptors.type() << " (CV_32F=" << CV_32F << ")";
            LOG_DEBUG_EN << "Recomputed descriptors for view_id " << view_id << ": " << descriptors.rows << "x" << descriptors.cols << " type=" << descriptors.type() << " (CV_32F=" << CV_32F << ")";

            all_keypoints.push_back(keypoints);
            all_descriptors.push_back(descriptors);
            all_view_ids.push_back(view_id); // Use continuous view_id | 使用连续的view_id
            all_image_paths.push_back(image_feature->GetImagePath());

            LOG_DEBUG_ZH << "处理视图ID " << view_id << "，包含 " << keypoints.size() << " 个特征";
            LOG_DEBUG_EN << "Processed view_id " << view_id << " with " << keypoints.size() << " features";

            // Update progress | 更新进度
            processed_views++;
            // Show progress at 20% intervals | 按20%间隔显示进度
            size_t current_milestone = (processed_views * 5) / total_views; // 0-5 represents 0%, 20%, 40%, 60%, 80%, 100%
            if (current_milestone > last_progress_milestone || processed_views == total_views)
            {
                ShowProgressBar(processed_views, total_views, "Feature Extraction:");
                last_progress_milestone = current_milestone;
            }
        }
    }

    void Img2MatchesPipeline::ExtractNewFeatures(
        ImagePathsPtr image_paths_ptr,
        FeaturesInfoPtr features_info_ptr,
        std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
        std::vector<cv::Mat> &all_descriptors,
        std::vector<IndexT> &all_view_ids,
        std::vector<std::string> &all_image_paths,
        std::vector<cv::Mat> *all_images)
    {
        // No existing features, extracting new features and descriptors
        // 没有已有特征，提取新的特征和描述子
        LOG_INFO_ZH << "没有已有特征，提取新的特征和描述子";
        LOG_INFO_EN << "No existing features, extracting new features and descriptors";

        // Consistent processing logic with feature extraction plugin | 与特征提取插件保持一致的处理逻辑
        std::vector<std::pair<std::string, std::string>> valid_image_pairs; // (filename_number, img_path) | (文件名数字, 图像路径)
        for (const auto &[img_path, is_valid] : *image_paths_ptr)
        {
            if (!is_valid)
                continue;

            // Extract number from image filename for sorting | 从图像文件名提取数字用于排序
            std::string filename = std::filesystem::path(img_path).stem().string();
            std::regex number_regex("^(\\d+)");
            std::smatch match;
            if (std::regex_search(filename, match, number_regex))
            {
                std::string filename_number = match[1].str();
                // Pad with zeros to ensure correct sorting | 补零确保正确排序
                std::string padded_number = std::string(8 - filename_number.length(), '0') + filename_number;
                valid_image_pairs.emplace_back(padded_number, img_path);
            }
            else
            {
                LOG_ERROR_ZH << "无法从文件名提取数字: " << filename;
                LOG_ERROR_EN << "Cannot extract number from filename: " << filename;
                continue;
            }
        }

        // Sort by filename number | 按文件名数字排序
        std::sort(valid_image_pairs.begin(), valid_image_pairs.end());

        // Create continuous view_id mapping | 创建连续的view_id映射
        features_info_ptr->clear();
        features_info_ptr->resize(valid_image_pairs.size());

        // Progress tracking setup | 进度跟踪设置
        const size_t total_views = valid_image_pairs.size();
        std::atomic<size_t> processed_views(0);
        size_t last_progress_milestone = 0; // Track last shown progress milestone | 跟踪上次显示的进度里程碑
        std::mutex progress_mutex;          // Mutex for thread-safe progress reporting | 进度报告的线程安全互斥锁

        // Prepare thread-safe containers for results | 准备线程安全的结果容器
        all_keypoints.resize(valid_image_pairs.size());
        all_descriptors.resize(valid_image_pairs.size());
        all_view_ids.resize(valid_image_pairs.size());
        all_image_paths.resize(valid_image_pairs.size());
        if (all_images != nullptr)
        {
            all_images->resize(valid_image_pairs.size());
        }

        // Configure OpenMP thread count | 配置OpenMP线程数
        int num_threads = params_.base.num_threads;
        if (num_threads <= 0)
        {
            num_threads = 1; // Fallback to single thread | 回退到单线程
        }

#ifdef USE_OPENMP
        // Set OpenMP thread count | 设置OpenMP线程数
        omp_set_num_threads(num_threads);
        LOG_INFO_ZH << "使用OpenMP多线程特征提取，线程数: " << num_threads;
        LOG_INFO_EN << "Using OpenMP multi-threaded feature extraction, threads: " << num_threads;
#else
        LOG_INFO_ZH << "OpenMP未启用，使用单线程特征提取";
        LOG_INFO_EN << "OpenMP not enabled, using single-threaded feature extraction";
        num_threads = 1;
#endif

        // Re-extract all features and descriptors, using continuous view_id | 重新提取所有特征和描述子，使用连续的view_id
#ifdef USE_OPENMP
#pragma omp parallel for schedule(dynamic) shared(valid_image_pairs, all_keypoints, all_descriptors, all_view_ids, all_image_paths, all_images, features_info_ptr, processed_views, progress_mutex, last_progress_milestone, total_views)
#endif
        for (IndexT view_id = 0; view_id < static_cast<IndexT>(valid_image_pairs.size()); ++view_id)
        {
            const std::string &img_path = valid_image_pairs[view_id].second;

            // Read image | 读取图像
            cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
            if (img.empty())
                continue;

            // Read color image for extracting RGB values at feature points | 读取彩色图像以提取特征点处的RGB值
            cv::Mat img_color = cv::imread(img_path, cv::IMREAD_COLOR);
            bool has_color_image = !img_color.empty();

            // 内存优化：只有LightGlue才需要缓存图像数据，SIFT+FLANN不需要
            if (params_.matching.matcher_type == MatcherType::LIGHTGLUE && all_images != nullptr)
            {
                (*all_images)[view_id] = img.clone();
                LOG_DEBUG_ZH << "缓存图像数据用于LightGlue匹配 (视图ID: " << view_id << ")";
                LOG_DEBUG_EN << "Caching image data for LightGlue matching (view_id: " << view_id << ")";
            }
            else if (all_images != nullptr)
            {
                LOG_DEBUG_ZH << "跳过图像缓存以节省内存 (视图ID: " << view_id << ", 匹配器: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "其他") << ")";
                LOG_DEBUG_EN << "Skipping image caching to save memory (view_id: " << view_id << ", matcher: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "other") << ")";
            }

            // Detect keypoints and descriptors | 检测特征点和描述子
            std::vector<cv::KeyPoint> keypoints;
            cv::Mat descriptors;

            // Process differently based on detector type | 根据检测器类型进行不同的处理
            if (params_.base.detector_type == "SIFT")
            {
                // SIFT-specific processing: apply first_octave image preprocessing
                // SIFT特有的处理：应用first_octave图像预处理
                cv::Mat processed_img = ApplyFirstOctaveProcessing(img);
                DetectFeatures(processed_img, keypoints, descriptors);

                // Adjust keypoint coordinates if image scaling was applied | 如果进行了图像缩放，需要调整特征点坐标
                if (params_.sift.first_octave != 0)
                {
                    AdjustKeypointsForScaling(keypoints, params_.sift.first_octave);
                }

                // Ensure descriptor type is CV_32F (required by matcher) | 确保描述子类型为CV_32F（匹配器要求）
                if (!descriptors.empty() && descriptors.type() != CV_32F)
                {
                    LOG_DEBUG_ZH << "将描述子从类型 " << descriptors.type() << " 转换为 CV_32F 以确保兼容性";
                    LOG_DEBUG_EN << "Converting descriptors from type " << descriptors.type() << " to CV_32F for compatibility";
                    descriptors.convertTo(descriptors, CV_32F);
                }

                // Apply RootSIFT normalization (if enabled) | 应用RootSIFT归一化（如果启用）
                if (params_.sift.root_sift && !descriptors.empty())
                {
                    ApplyRootSIFTNormalization(descriptors);
                }
            }
            else
            {
                // Other detectors (like ORB, etc.) detect directly on the original image
                // 其他检测器（如ORB等）直接使用原图进行检测
                DetectFeatures(img, keypoints, descriptors);

                // Determine correct descriptor type based on detector type | 根据检测器类型确定正确的描述子类型
                if (params_.base.detector_type == "SIFT" || params_.base.detector_type == "KAZE")
                {
                    // Floating-point descriptors: ensure type is CV_32F | 浮点描述子：确保类型为CV_32F
                    if (!descriptors.empty() && descriptors.type() != CV_32F)
                    {
                        LOG_DEBUG_ZH << "将 " << params_.base.detector_type << " 描述子从类型 " << descriptors.type() << " 转换为 CV_32F 以确保兼容性";
                        LOG_DEBUG_EN << "Converting " << params_.base.detector_type << " descriptors from type " << descriptors.type() << " to CV_32F for compatibility";
                        descriptors.convertTo(descriptors, CV_32F);
                    }
                }
                else if (params_.base.detector_type == "ORB" || params_.base.detector_type == "BRISK" || params_.base.detector_type == "AKAZE")
                {
                    // Binary descriptors: ensure type is CV_8U | 二进制描述子：确保类型为CV_8U
                    if (!descriptors.empty() && descriptors.type() != CV_8U)
                    {
                        LOG_DEBUG_ZH << "将 " << params_.base.detector_type << " 描述子从类型 " << descriptors.type() << " 转换为 CV_8U 以确保二进制兼容性";
                        LOG_DEBUG_EN << "Converting " << params_.base.detector_type << " descriptors from type " << descriptors.type() << " to CV_8U for binary compatibility";
                        descriptors.convertTo(descriptors, CV_8U);
                    }
                    else if (!descriptors.empty())
                    {
                        LOG_DEBUG_ZH << params_.base.detector_type << " 描述子已经是正确的 CV_8U 格式";
                        LOG_DEBUG_EN << params_.base.detector_type << " descriptors already in correct CV_8U format";
                    }
                }
                else
                {
                    LOG_DEBUG_ZH << "未知检测器类型 " << params_.base.detector_type << "，保留原始描述子类型 " << descriptors.type();
                    LOG_DEBUG_EN << "Unknown detector type " << params_.base.detector_type << ", keeping original descriptor type " << descriptors.type();
                }
            }

            // Save keypoints and corresponding continuous view_id | 保存特征点和对应的连续view_id
            all_keypoints[view_id] = std::move(keypoints);
            all_descriptors[view_id] = descriptors.clone(); // Clone to ensure thread safety | 克隆以确保线程安全
            all_view_ids[view_id] = view_id;                // Use continuous view_id | 使用连续的view_id
            all_image_paths[view_id] = img_path;

            // Create image feature information and store at continuous view_id position
            // 创建图像特征信息并存储到连续的view_id位置
            ImageFeatureInfo image_feature;
            image_feature.SetImagePath(img_path);

            image_feature.ReserveFeatures(all_keypoints[view_id].size());

            // Prepare color data if color image is available | 如果有彩色图像，准备颜色数据
            std::vector<std::array<uint8_t, 3>> colors;
            if (has_color_image)
            {
                colors.reserve(all_keypoints[view_id].size());
            }

            for (const auto &kp : all_keypoints[view_id])
            {
                Feature coord(kp.pt.x, kp.pt.y);
                image_feature.AddFeature(coord, kp.size, kp.angle);

                // Extract color from the color image at keypoint position | 从彩色图像中提取关键点位置的颜色
                if (has_color_image)
                {
                    int x = static_cast<int>(std::round(kp.pt.x));
                    int y = static_cast<int>(std::round(kp.pt.y));

                    // Boundary check | 边界检查
                    if (x >= 0 && x < img_color.cols && y >= 0 && y < img_color.rows)
                    {
                        cv::Vec3b bgr = img_color.at<cv::Vec3b>(y, x);
                        // OpenCV uses BGR format, convert to RGB | OpenCV使用BGR格式，转换为RGB
                        colors.push_back({bgr[2], bgr[1], bgr[0]});
                    }
                    else
                    {
                        // Out of bounds, use black color | 超出边界，使用黑色
                        colors.push_back({0, 0, 0});
                    }
                }
            }

            // Set colors to FeaturePoints if available | 如果有颜色数据，设置到FeaturePoints
            if (has_color_image && !colors.empty())
            {
                auto &feature_points = image_feature.GetFeaturePoints();
                feature_points.GetColorsRGBRef().assign(colors.begin(), colors.end());

                LOG_DEBUG_ZH << "已为视图 " << view_id << " 的 " << colors.size() << " 个特征点提取颜色信息";
                LOG_DEBUG_EN << "Extracted color information for " << colors.size() << " features in view " << view_id;
            }

            // Store feature information using continuous view_id as index | 使用连续的view_id作为索引存储特征信息
            // 使用原地构造避免指针赋值问题 | Use in-place construction to avoid pointer assignment issues
            if (view_id < features_info_ptr->size())
            {
                *(*features_info_ptr)[view_id] = std::move(image_feature);
            }

            LOG_DEBUG_ZH << "提取视图ID " << view_id << "，包含 " << all_keypoints[view_id].size() << " 个特征";
            LOG_DEBUG_EN << "Extracted view_id " << view_id << " with " << all_keypoints[view_id].size() << " features";

            // Update progress with thread safety | 线程安全地更新进度
            size_t current_processed = processed_views.fetch_add(1) + 1;

            // Thread-safe progress reporting | 线程安全的进度报告
            {
                std::lock_guard<std::mutex> lock(progress_mutex);
                size_t current_milestone = (current_processed * 5) / total_views; // 0-5 represents 0%, 20%, 40%, 60%, 80%, 100%
                if (current_milestone > last_progress_milestone || current_processed == total_views)
                {
                    ShowProgressBar(current_processed, total_views, "Feature Extraction:");
                    last_progress_milestone = current_milestone;
                }
            }
        }

        // 关键修复：重新组织数据以匹配旧版本的push_back行为
        // 确保数据访问索引与旧版本完全一致
        std::vector<std::vector<cv::KeyPoint>> final_keypoints;
        std::vector<cv::Mat> final_descriptors;
        std::vector<IndexT> final_view_ids;
        std::vector<std::string> final_image_paths;

        // 按view_id顺序重新添加数据，模拟旧版本的push_back行为
        for (IndexT view_id = 0; view_id < valid_image_pairs.size(); ++view_id)
        {
            final_keypoints.push_back(std::move(all_keypoints[view_id]));
            final_descriptors.push_back(all_descriptors[view_id].clone());
            final_view_ids.push_back(all_view_ids[view_id]);
            final_image_paths.push_back(all_image_paths[view_id]);
        }

        // 替换原始容器，确保后续访问与旧版本一致
        all_keypoints = std::move(final_keypoints);
        all_descriptors = std::move(final_descriptors);
        all_view_ids = std::move(final_view_ids);
        all_image_paths = std::move(final_image_paths);

        // 内存优化：只有LightGlue才需要重新组织图像缓存
        if (all_images != nullptr && !all_images->empty() && params_.matching.matcher_type == MatcherType::LIGHTGLUE)
        {
            std::vector<cv::Mat> final_images;
            for (IndexT view_id = 0; view_id < valid_image_pairs.size(); ++view_id)
            {
                if (view_id < all_images->size())
                {
                    final_images.push_back((*all_images)[view_id].clone());
                }
            }
            *all_images = std::move(final_images);
            LOG_DEBUG_ZH << "重新组织LightGlue图像缓存，共 " << all_images->size() << " 张图像";
            LOG_DEBUG_EN << "Reorganized LightGlue image cache with " << all_images->size() << " images";
        }
        else if (all_images != nullptr)
        {
            LOG_DEBUG_ZH << "跳过图像缓存重新组织以节省内存 (匹配器: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "其他") << ")";
            LOG_DEBUG_EN << "Skipping image cache reorganization to save memory (matcher: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : "other") << ")";
        }
    }

    size_t Img2MatchesPipeline::PerformPairwiseMatching(
        const std::vector<cv::Mat> &all_descriptors,
        const std::vector<IndexT> &all_view_ids,
        MatchesPtr matches_ptr,
        const std::vector<std::vector<cv::KeyPoint>> *all_keypoints,
        const std::vector<cv::Mat> *all_images)
    {
        // Process matching between all image pairs | 处理所有图像对之间的匹配
        LOG_INFO_ZH << "开始对 " << all_view_ids.size() << " 个视图进行成对匹配";
        LOG_INFO_EN << "Starting pairwise matching for " << all_view_ids.size() << " views";

        size_t total_pairs = 0;
        size_t successful_pairs = 0;

        for (size_t i = 0; i < all_view_ids.size(); ++i)
        {
            for (size_t j = i + 1; j < all_view_ids.size(); ++j)
            {
                total_pairs++;

                LOG_DEBUG_ZH << "匹配视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";
                LOG_DEBUG_EN << "Matching view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";

                // Select matching method based on matcher type | 根据匹配器类型选择匹配方法
                std::vector<cv::DMatch> matches;
                if (params_.matching.matcher_type == MatcherType::LIGHTGLUE &&
                    all_keypoints != nullptr && all_images != nullptr &&
                    !all_keypoints->empty() && !all_images->empty() &&
                    i < all_keypoints->size() && j < all_keypoints->size() &&
                    i < all_images->size() && j < all_images->size())
                {
                    // Use LightGlue deep learning matcher | 使用LightGlue深度学习匹配器
                    matches = MatchFeaturesWithLightGlue(
                        (*all_images)[i], (*all_images)[j],
                        (*all_keypoints)[i], (*all_keypoints)[j],
                        all_descriptors[i], all_descriptors[j]);
                }
                else
                {
                    // Use traditional matcher (SIFT+FLANN) | 使用传统匹配器 (SIFT+FLANN)
                    // 内存优化：传统匹配器不需要图像数据，只使用描述子
                    matches = MatchFeatures(all_descriptors[i], all_descriptors[j]);
                }

                if (!matches.empty())
                {
                    successful_pairs++;
                    LOG_DEBUG_ZH << "视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ") 找到 " << matches.size() << " 个匹配";
                    LOG_DEBUG_EN << "Found " << matches.size() << " matches for view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";

                    // Convert and save matching results using correct view_id | 转换并保存匹配结果，使用正确的view_id
                    OpenCVConverter::CVDMatch2Matches(matches,
                                                      all_view_ids[i], all_view_ids[j], matches_ptr);
                }
                else
                {
                    LOG_DEBUG_ZH << "视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ") 未找到匹配";
                    LOG_DEBUG_EN << "No matches found for view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";
                }
            }
        }

        LOG_INFO_ZH << "匹配完成: " << successful_pairs << "/" << total_pairs << " 对视图有匹配结果";
        LOG_INFO_EN << "Matching completed: " << successful_pairs << "/" << total_pairs << " pairs have matches";

        return successful_pairs;
    }

    size_t Img2MatchesPipeline::PerformPairwiseMatchingMultiThreads(
        const std::vector<cv::Mat> &all_descriptors,
        const std::vector<IndexT> &all_view_ids,
        MatchesPtr matches_ptr,
        const std::vector<std::vector<cv::KeyPoint>> *all_keypoints,
        const std::vector<cv::Mat> *all_images)
    {
        // Process matching between all image pairs | 处理所有图像对之间的匹配
        LOG_INFO_ZH << "开始多线程对 " << all_view_ids.size() << " 个视图进行成对匹配";
        LOG_INFO_EN << "Starting multi-threaded pairwise matching for " << all_view_ids.size() << " views";

        // Calculate total number of pairs | 计算总的匹配对数
        const size_t num_views = all_view_ids.size();
        const size_t total_pairs_count = (num_views * (num_views - 1)) / 2;

        // Generate all image pairs for parallel processing | 生成所有图像对用于并行处理
        std::vector<std::pair<size_t, size_t>> image_pairs;
        image_pairs.reserve(total_pairs_count);
        for (size_t i = 0; i < num_views; ++i)
        {
            for (size_t j = i + 1; j < num_views; ++j)
            {
                image_pairs.emplace_back(i, j);
            }
        }

        // Thread-safe progress tracking | 线程安全的进度跟踪
        std::atomic<size_t> processed_pairs(0);
        std::atomic<size_t> successful_pairs(0);
        size_t last_progress_milestone = 0;
        std::mutex progress_mutex; // Mutex for thread-safe progress reporting | 进度报告的线程安全互斥锁
        std::mutex matches_mutex;  // Mutex for thread-safe matches writing | 匹配结果写入的线程安全互斥锁
        std::mutex flann_mutex;    // Mutex for FLANN matcher creation to ensure determinism | FLANN匹配器创建的互斥锁，确保确定性

        // Configure OpenMP thread count | 配置OpenMP线程数
        int num_threads = params_.base.num_threads;
        if (num_threads <= 0)
        {
            num_threads = 1; // Fallback to single thread | 回退到单线程
        }

#ifdef USE_OPENMP
        // Set OpenMP thread count | 设置OpenMP线程数
        omp_set_num_threads(num_threads);
        LOG_INFO_ZH << "使用OpenMP多线程特征匹配，线程数: " << num_threads;
        LOG_INFO_EN << "Using OpenMP multi-threaded feature matching, threads: " << num_threads;
#else
        LOG_INFO_ZH << "OpenMP未启用，使用单线程特征匹配";
        LOG_INFO_EN << "OpenMP not enabled, using single-threaded feature matching";
        num_threads = 1;
#endif

        // 显示匹配器参数配置用于调试
        LOG_INFO_ZH << "多线程匹配器参数配置:";
        LOG_INFO_EN << "Multi-threaded matcher parameters:";
        LOG_INFO_ZH << "  matcher_type: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : params_.matching.matcher_type == MatcherType::BF      ? "BF"
                                                                                                          : params_.matching.matcher_type == MatcherType::LIGHTGLUE ? "LIGHTGLUE"
                                                                                                                                                                    : "OTHER");
        LOG_INFO_EN << "  matcher_type: " << (params_.matching.matcher_type == MatcherType::FLANN ? "FLANN" : params_.matching.matcher_type == MatcherType::BF      ? "BF"
                                                                                                          : params_.matching.matcher_type == MatcherType::LIGHTGLUE ? "LIGHTGLUE"
                                                                                                                                                                    : "OTHER");
        LOG_INFO_ZH << "  ratio_thresh: " << params_.matching.ratio_thresh;
        LOG_INFO_EN << "  ratio_thresh: " << params_.matching.ratio_thresh;
        LOG_INFO_ZH << "  cross_check: " << (params_.matching.cross_check ? "true" : "false");
        LOG_INFO_EN << "  cross_check: " << (params_.matching.cross_check ? "true" : "false");

        if (params_.matching.matcher_type == MatcherType::FLANN)
        {
            LOG_INFO_ZH << "  FLANN参数: use_advanced_control=" << (params_.flann.use_advanced_control ? "true" : "false");
            LOG_INFO_EN << "  FLANN params: use_advanced_control=" << (params_.flann.use_advanced_control ? "true" : "false");
            if (params_.flann.use_advanced_control)
            {
                LOG_INFO_ZH << "    trees=" << params_.flann.trees << ", checks=" << params_.flann.checks;
                LOG_INFO_EN << "    trees=" << params_.flann.trees << ", checks=" << params_.flann.checks;
            }
        }

        // 为确保FLANN匹配器的确定性结果，使用静态调度而非动态调度
        // 静态调度确保每次运行时任务分配给线程的顺序完全一致
        LOG_INFO_ZH << "使用静态调度确保多线程匹配的确定性结果";
        LOG_INFO_EN << "Using static scheduling for deterministic multi-threaded matching results";

        // Parallel matching of all image pairs | 并行匹配所有图像对
#ifdef USE_OPENMP
#pragma omp parallel for schedule(static) shared(image_pairs, all_descriptors, all_view_ids, all_keypoints, all_images, matches_ptr, processed_pairs, successful_pairs, progress_mutex, matches_mutex, flann_mutex, last_progress_milestone, total_pairs_count)
#endif
        for (size_t pair_idx = 0; pair_idx < image_pairs.size(); ++pair_idx)
        {
            const auto &[i, j] = image_pairs[pair_idx];

            LOG_DEBUG_ZH << "多线程匹配视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ") - 特征数量: "
                         << all_descriptors[i].rows << "x" << all_descriptors[j].rows;
            LOG_DEBUG_EN << "Multi-thread matching view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ") - feature counts: "
                         << all_descriptors[i].rows << "x" << all_descriptors[j].rows;

            // 为每个线程设置相同的随机种子，确保FLANN结果一致性
            if (params_.matching.matcher_type == MatcherType::FLANN)
            {
                // 使用与MatchFeaturesThreadSafe相同的种子计算方法
                uint32_t thread_seed = 12345;
                thread_seed ^= (all_view_ids[i] << 16) | all_view_ids[j];
                thread_seed ^= (all_view_ids[i] * 7919 + all_view_ids[j] * 7927);
                cv::setRNGSeed(thread_seed);
            }

            // Select matching method based on matcher type | 根据匹配器类型选择匹配方法
            std::vector<cv::DMatch> matches;
            if (params_.matching.matcher_type == MatcherType::LIGHTGLUE &&
                all_keypoints != nullptr && all_images != nullptr &&
                !all_keypoints->empty() && !all_images->empty() &&
                i < all_keypoints->size() && j < all_keypoints->size() &&
                i < all_images->size() && j < all_images->size())
            {
                // Use LightGlue deep learning matcher | 使用LightGlue深度学习匹配器
                matches = MatchFeaturesWithLightGlue(
                    (*all_images)[i], (*all_images)[j],
                    (*all_keypoints)[i], (*all_keypoints)[j],
                    all_descriptors[i], all_descriptors[j]);
            }
            else
            {
                // Use traditional matcher (SIFT+FLANN) | 使用传统匹配器 (SIFT+FLANN)
                // 内存优化：传统匹配器不需要图像数据，只使用描述子
                // 为确保多线程结果一致性，使用线程安全的匹配方法
                matches = MatchFeaturesThreadSafe(all_descriptors[i], all_descriptors[j], all_view_ids[i], all_view_ids[j]);
            }

            // Thread-safe result processing | 线程安全的结果处理
            if (!matches.empty())
            {
                successful_pairs.fetch_add(1);
                LOG_DEBUG_ZH << "多线程匹配成功 - 视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ") 找到 " << matches.size() << " 个匹配";
                LOG_DEBUG_EN << "Multi-thread matching success - Found " << matches.size() << " matches for view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";

                // Thread-safe conversion and saving of matching results | 线程安全的匹配结果转换和保存
                {
                    std::lock_guard<std::mutex> lock(matches_mutex);
                    OpenCVConverter::CVDMatch2Matches(matches,
                                                      all_view_ids[i], all_view_ids[j], matches_ptr);
                }
            }
            else
            {
                LOG_DEBUG_ZH << "多线程匹配失败 - 视图对 (" << all_view_ids[i] << ", " << all_view_ids[j] << ") 未找到匹配";
                LOG_DEBUG_EN << "Multi-thread matching failed - No matches found for view pair (" << all_view_ids[i] << ", " << all_view_ids[j] << ")";
            }

            // Update progress with thread safety | 线程安全地更新进度
            size_t current_processed = processed_pairs.fetch_add(1) + 1;
            size_t current_successful = successful_pairs.load();

            // Thread-safe progress reporting | 线程安全的进度报告
            {
                std::lock_guard<std::mutex> lock(progress_mutex);
                size_t current_milestone = (current_processed * 5) / total_pairs_count; // 0-5 represents 0%, 20%, 40%, 60%, 80%, 100%
                if (current_milestone > last_progress_milestone || current_processed == total_pairs_count)
                {
                    std::string task_name = "Multi-thread Matching (successful: " + std::to_string(current_successful) + "):";
                    ShowProgressBar(current_processed, total_pairs_count, task_name);
                    last_progress_milestone = current_milestone;
                }
            }
        }

        size_t final_successful_pairs = successful_pairs.load();
        LOG_INFO_ZH << "多线程匹配完成: " << final_successful_pairs << "/" << total_pairs_count << " 对视图有匹配结果";
        LOG_INFO_EN << "Multi-threaded matching completed: " << final_successful_pairs << "/" << total_pairs_count << " pairs have matches";

        return final_successful_pairs;
    }

    void Img2MatchesPipeline::LoadConfigurationAtRuntime()
    {
        LOG_DEBUG_ZH << "运行时加载配置...";
        LOG_DEBUG_EN << "Loading configuration at runtime...";

        // === Correct parameter flow | 正确的参数流程 ===
        // 1. Initialize configuration file path, load main config and specific method config to method_options_ and specific_methods_config_
        //    初始化配置文件路径，加载主配置和特定方法配置到method_options_和specific_methods_config_
        InitializeDefaultConfigPath(); // This will load [method_img2matches] section to method_options_ | 这会加载 [method_img2matches] 部分到 method_options_

        // 2. Load specific feature detector configuration to specific_methods_config_ based on detector_type
        //    根据detector_type加载特定的特征检测器配置到specific_methods_config_
        const std::string &detector_type = GetOptionAsString("detector_type", "SIFT");
        if (!detector_type.empty())
        {
            InitializeDefaultConfigPath(detector_type); // Load to specific_methods_config_[detector_type] | 加载到 specific_methods_config_[detector_type]
            LOG_DEBUG_ZH << "已加载检测器配置: " << detector_type;
            LOG_DEBUG_EN << "Loaded detector configuration: " << detector_type;
        }

        // 3. Load specific matcher configuration to specific_methods_config_ based on matcher_type
        //    根据matcher_type加载特定的匹配器配置到specific_methods_config_
        const std::string &matcher_type = GetOptionAsString("matcher_type", "FLANN");
        if (!matcher_type.empty() && matcher_type != detector_type)
        {
            InitializeDefaultConfigPath(matcher_type); // Load to specific_methods_config_[matcher_type] | 加载到 specific_methods_config_[matcher_type]
            LOG_DEBUG_ZH << "已加载匹配器配置: " << matcher_type;
            LOG_DEBUG_EN << "Loaded matcher configuration: " << matcher_type;
        }

        // 4. Load parameters from method_options_ and specific_methods_config_ to structured config params_
        //    从method_options_和specific_methods_config_加载参数到结构化配置params_
        params_.LoadFromConfig(this);

        // 5. Synchronize SIFT parameters to parent class method_options_ for use in parent's DetectFeatures method
        //    将SIFT参数同步到父类method_options_，以便父类的DetectFeatures方法使用
        if (params_.base.detector_type == "SIFT")
        {
            // Synchronize SIFT parameters to method_options_ | 同步SIFT参数到method_options_
            method_options_["nfeatures"] = std::to_string(params_.sift.nfeatures);
            method_options_["nOctaveLayers"] = std::to_string(params_.sift.nOctaveLayers);
            method_options_["contrastThreshold"] = std::to_string(params_.sift.contrastThreshold);
            method_options_["edgeThreshold"] = std::to_string(params_.sift.edgeThreshold);
            method_options_["sigma"] = std::to_string(params_.sift.sigma);
            method_options_["enable_precise_upscale"] = params_.sift.enable_precise_upscale ? "true" : "false";

            // Synchronize OpenMVG extension parameters | 同步OpenMVG扩展参数
            method_options_["first_octave"] = std::to_string(params_.sift.first_octave);
            method_options_["num_octaves"] = std::to_string(params_.sift.num_octaves);
            method_options_["root_sift"] = params_.sift.root_sift ? "true" : "false";
            method_options_["preset"] = Img2MatchesParameterConverter::SIFTPresetToString(params_.sift.preset);

            LOG_DEBUG_ZH << "SIFT参数已同步到父类";
            LOG_DEBUG_EN << "SIFT parameters synchronized to parent class";
        }

        // 6. Synchronize ORB parameters to parent class method_options_ for use in parent's DetectFeatures method
        //    将ORB参数同步到父类method_options_，以便父类的DetectFeatures方法使用
        if (params_.base.detector_type == "ORB")
        {
            // Synchronize ORB parameters to method_options_ | 同步ORB参数到method_options_
            method_options_["orb_nfeatures"] = std::to_string(params_.orb.nfeatures);
            method_options_["orb_scaleFactor"] = std::to_string(params_.orb.scaleFactor);
            method_options_["orb_nlevels"] = std::to_string(params_.orb.nlevels);
            method_options_["orb_edgeThreshold"] = std::to_string(params_.orb.edgeThreshold);
            method_options_["orb_firstLevel"] = std::to_string(params_.orb.firstLevel);
            method_options_["orb_WTA_K"] = std::to_string(params_.orb.WTA_K);
            method_options_["orb_patchSize"] = std::to_string(params_.orb.patchSize);
            method_options_["orb_fastThreshold"] = std::to_string(params_.orb.fastThreshold);

            LOG_DEBUG_ZH << "ORB参数已同步到父类";
            LOG_DEBUG_ZH << "  特征数量: " << params_.orb.nfeatures;
            LOG_DEBUG_ZH << "  尺度因子: " << params_.orb.scaleFactor;
            LOG_DEBUG_ZH << "  金字塔层级: " << params_.orb.nlevels;
            LOG_DEBUG_ZH << "  边缘阈值: " << params_.orb.edgeThreshold;
            LOG_DEBUG_EN << "ORB parameters synchronized to parent class";
            LOG_DEBUG_EN << "  nfeatures: " << params_.orb.nfeatures;
            LOG_DEBUG_EN << "  scaleFactor: " << params_.orb.scaleFactor;
            LOG_DEBUG_EN << "  nlevels: " << params_.orb.nlevels;
            LOG_DEBUG_EN << "  edgeThreshold: " << params_.orb.edgeThreshold;
        }

        // 8. Synchronize SuperPoint parameters to parent class method_options_ for use in parent's DetectFeatures method
        //    将SuperPoint参数同步到父类method_options_，以便父类的DetectFeatures方法使用
        if (params_.base.detector_type == "SUPERPOINT")
        {
            // Synchronize SuperPoint parameters to method_options_ | 同步SuperPoint参数到method_options_
            method_options_["max_keypoints"] = std::to_string(params_.superpoint.max_keypoints);
            method_options_["detection_threshold"] = std::to_string(params_.superpoint.detection_threshold);
            method_options_["nms_radius"] = std::to_string(params_.superpoint.nms_radius);
            method_options_["remove_borders"] = std::to_string(params_.superpoint.remove_borders);
            method_options_["python_executable"] = params_.superpoint.python_executable;

            LOG_DEBUG_ZH << "SuperPoint参数已同步到父类";
            LOG_DEBUG_ZH << "  最大关键点数: " << params_.superpoint.max_keypoints;
            LOG_DEBUG_ZH << "  检测阈值: " << params_.superpoint.detection_threshold;
            LOG_DEBUG_ZH << "  Python执行路径: " << params_.superpoint.python_executable;
            LOG_DEBUG_EN << "SuperPoint parameters synchronized to parent class";
            LOG_DEBUG_EN << "  max_keypoints: " << params_.superpoint.max_keypoints;
            LOG_DEBUG_EN << "  detection_threshold: " << params_.superpoint.detection_threshold;
            LOG_DEBUG_EN << "  python_executable: " << params_.superpoint.python_executable;
        }

        LOG_DEBUG_ZH << "配置加载成功";
        LOG_DEBUG_EN << "Configuration loaded successfully";
    }

    void Img2MatchesPipeline::ExportResults(DataPtr features_data_ptr, DataPtr matches_data_ptr)
    {
        // Export results | 导出结果
        if (params_.feature_export.export_features)
        {
            if (!params_.feature_export.export_fea_path.empty())
            {
                features_data_ptr->Save(params_.feature_export.export_fea_path, "features_all");
            }
        }

        if (params_.matches_export.export_matches)
        {
            if (!params_.matches_export.export_match_path.empty())
            {
                matches_data_ptr->Save(params_.matches_export.export_match_path, "matches_all");
            }
        }
    }

    void Img2MatchesPipeline::ApplyRootSIFTNormalization(cv::Mat &descriptors)
    {
        if (descriptors.empty() || descriptors.type() != CV_32F)
        {
            LOG_WARNING_ZH << "RootSIFT归一化需要CV_32F描述子";
            LOG_WARNING_EN << "RootSIFT normalization requires CV_32F descriptors";
            return;
        }

        LOG_DEBUG_ZH << "对 " << descriptors.rows << " 个描述子应用RootSIFT归一化";
        LOG_DEBUG_EN << "Applying RootSIFT normalization to " << descriptors.rows << " descriptors";

        for (int i = 0; i < descriptors.rows; ++i)
        {
            cv::Mat descriptor = descriptors.row(i);

            // 1. L1 normalization | L1归一化
            float l1_norm = cv::sum(cv::abs(descriptor))[0];
            if (l1_norm > 1e-12f)
            {
                descriptor /= l1_norm;
            }

            // 2. Square root (key step of RootSIFT) | 开平方根（RootSIFT的关键步骤）
            cv::sqrt(descriptor, descriptor);

            // 3. L2 normalization | L2归一化
            float l2_norm = cv::norm(descriptor, cv::NORM_L2);
            if (l2_norm > 1e-12f)
            {
                descriptor /= l2_norm;
            }
        }

        LOG_DEBUG_ZH << "RootSIFT归一化完成";
        LOG_DEBUG_EN << "RootSIFT normalization completed";
    }

    cv::Mat Img2MatchesPipeline::ApplyFirstOctaveProcessing(const cv::Mat &img)
    {
        cv::Mat processed_img;

        switch (params_.sift.first_octave)
        {
        case -1:
            // Upsampling: Double the image size to detect more detailed features
            // 上采样：图像尺寸翻倍，用于检测更多细节特征
            cv::resize(img, processed_img, cv::Size(), 2.0, 2.0, cv::INTER_CUBIC);
            LOG_DEBUG_ZH << "应用上采样 (first_octave=-1): " << img.size() << " -> " << processed_img.size();
            LOG_DEBUG_EN << "Applied upsampling (first_octave=-1): " << img.size() << " -> " << processed_img.size();
            break;

        case 1:
            // Downsampling: Halve the image size to reduce computation
            // 下采样：图像尺寸减半，减少计算量
            cv::resize(img, processed_img, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
            LOG_DEBUG_ZH << "应用下采样 (first_octave=1): " << img.size() << " -> " << processed_img.size();
            LOG_DEBUG_EN << "Applied downsampling (first_octave=1): " << img.size() << " -> " << processed_img.size();
            break;

        case 0:
        default:
            // Use original image | 使用原始图像
            processed_img = img.clone();
            break;
        }

        return processed_img;
    }

    void Img2MatchesPipeline::AdjustKeypointsForScaling(std::vector<cv::KeyPoint> &keypoints, int first_octave)
    {
        float scale_factor = 1.0f;

        switch (first_octave)
        {
        case -1:
            // Upsampling: Keypoint coordinates need to be scaled down to original image size
            // 上采样：特征点坐标需要缩小到原图尺寸
            scale_factor = 0.5f;
            break;
        case 1:
            // Downsampling: Keypoint coordinates need to be scaled up to original image size
            // 下采样：特征点坐标需要放大到原图尺寸
            scale_factor = 2.0f;
            break;
        case 0:
        default:
            // No adjustment needed | 无需调整
            return;
        }

        LOG_DEBUG_ZH << "调整 " << keypoints.size() << " 个关键点，缩放因子: " << scale_factor;
        LOG_DEBUG_EN << "Adjusting " << keypoints.size() << " keypoints with scale factor: " << scale_factor;

        for (auto &kp : keypoints)
        {
            kp.pt.x *= scale_factor;
            kp.pt.y *= scale_factor;
            kp.size *= scale_factor; // Keypoint size also needs to be adjusted accordingly | 特征点尺寸也需要相应调整
        }
    }

    cv::Ptr<cv::DescriptorMatcher> Img2MatchesPipeline::CreateFLANNMatcher()
    {
        // Creating FLANN matcher with configured parameters | 使用配置参数创建FLANN匹配器
        LOG_DEBUG_ZH << "使用配置参数创建FLANN匹配器...";
        LOG_DEBUG_EN << "Creating FLANN matcher with configured parameters...";

        // Ensure the correct algorithm is applied | 确保应用了正确的算法选择
        auto &flann_params = params_.flann;
        flann_params.AutoSelectAlgorithm(params_.base.detector_type);

        // Create IndexParams - control index building | 创建IndexParams - 控制索引构建
        cv::Ptr<cv::flann::IndexParams> indexParams;

        switch (flann_params.algorithm)
        {
        case FLANNAlgorithm::KDTREE:
        {
            // Using KDTree algorithm | 使用KDTree算法
            LOG_DEBUG_ZH << "使用KDTree算法，树的数量为 " << flann_params.trees;
            LOG_DEBUG_EN << "Using KDTree algorithm with " << flann_params.trees << " trees";
            auto kdtreeParams = cv::makePtr<cv::flann::KDTreeIndexParams>(flann_params.trees);
            indexParams = kdtreeParams;
            break;
        }
        case FLANNAlgorithm::LSH:
        {
            // Using LSH algorithm | 使用LSH算法
            LOG_DEBUG_ZH << "使用LSH算法，table_number=" << flann_params.table_number
                         << "，key_size=" << flann_params.key_size
                         << "，multi_probe_level=" << flann_params.multi_probe_level;
            LOG_DEBUG_EN << "Using LSH algorithm with table_number=" << flann_params.table_number
                         << ", key_size=" << flann_params.key_size
                         << ", multi_probe_level=" << flann_params.multi_probe_level;
            auto lshParams = cv::makePtr<cv::flann::LshIndexParams>(
                flann_params.table_number, flann_params.key_size, flann_params.multi_probe_level);
            indexParams = lshParams;
            break;
        }
        case FLANNAlgorithm::KMEANS:
        {
            // Using KMeans algorithm | 使用KMeans算法
            LOG_DEBUG_ZH << "使用KMeans算法，branching=" << flann_params.branching
                         << "，iterations=" << flann_params.iterations;
            LOG_DEBUG_EN << "Using KMeans algorithm with branching=" << flann_params.branching
                         << ", iterations=" << flann_params.iterations;

            // Convert centers_init enum to OpenCV parameter | 转换centers_init枚举到OpenCV参数
            cvflann::flann_centers_init_t centers_init_cv;
            switch (flann_params.centers_init)
            {
            case FLANNCentersInit::CENTERS_RANDOM:
                centers_init_cv = cvflann::FLANN_CENTERS_RANDOM;
                break;
            case FLANNCentersInit::CENTERS_GONZALES:
                centers_init_cv = cvflann::FLANN_CENTERS_GONZALES;
                break;
            case FLANNCentersInit::CENTERS_KMEANSPP:
                centers_init_cv = cvflann::FLANN_CENTERS_KMEANSPP;
                break;
            default:
                centers_init_cv = cvflann::FLANN_CENTERS_RANDOM;
                break;
            }

            auto kmeansParams = cv::makePtr<cv::flann::KMeansIndexParams>(
                flann_params.branching, flann_params.iterations, centers_init_cv, 0.2f);
            indexParams = kmeansParams;
            break;
        }
        case FLANNAlgorithm::COMPOSITE:
        {
            // Using Composite algorithm | 使用Composite算法
            LOG_DEBUG_ZH << "使用Composite算法";
            LOG_DEBUG_EN << "Using Composite algorithm";
            auto compositeParams = cv::makePtr<cv::flann::CompositeIndexParams>();
            indexParams = compositeParams;
            break;
        }
        case FLANNAlgorithm::LINEAR:
        {
            // Using Linear algorithm (brute force search) | 使用Linear算法（暴力搜索）
            LOG_DEBUG_ZH << "使用Linear算法（暴力搜索）";
            LOG_DEBUG_EN << "Using Linear algorithm (brute force search)";
            auto linearParams = cv::makePtr<cv::flann::LinearIndexParams>();
            indexParams = linearParams;
            break;
        }
        case FLANNAlgorithm::AUTO:
        default:
        {
            // Using AutoTuned algorithm | 使用AutoTuned算法
            LOG_DEBUG_ZH << "使用AutoTuned算法";
            LOG_DEBUG_EN << "Using AutoTuned algorithm";
            auto autoParams = cv::makePtr<cv::flann::AutotunedIndexParams>(
                0.8f,  // target_precision
                0.01f, // build_weight
                0.01f, // memory_weight
                0.1f   // sample_fraction
            );
            indexParams = autoParams;
            break;
        }
        }

        // Create SearchParams - control search behavior | 创建SearchParams - 控制搜索行为
        auto searchParams = cv::makePtr<cv::flann::SearchParams>(
            flann_params.checks,       // checks: number of search checks | checks: 搜索检查次数
            flann_params.eps,          // eps: search precision | eps: 搜索精度
            flann_params.sorted,       // sorted: whether to sort results | sorted: 是否排序结果
            flann_params.max_neighbors // max_neighbors: maximum number of neighbors | max_neighbors: 最大邻居数
        );

        // Log FLANN search parameters | 记录FLANN搜索参数
        LOG_DEBUG_ZH << "FLANN搜索参数：checks=" << flann_params.checks
                     << "，eps=" << flann_params.eps
                     << "，sorted=" << (flann_params.sorted ? "true" : "false")
                     << "，max_neighbors=" << flann_params.max_neighbors;
        LOG_DEBUG_EN << "FLANN search parameters: checks=" << flann_params.checks
                     << ", eps=" << flann_params.eps
                     << ", sorted=" << (flann_params.sorted ? "true" : "false")
                     << ", max_neighbors=" << flann_params.max_neighbors;

        // Create FlannBasedMatcher | 创建FlannBasedMatcher
        auto matcher = cv::makePtr<cv::FlannBasedMatcher>(indexParams, searchParams);

        // Log successful creation of FLANN matcher | 记录FLANN匹配器创建成功
        LOG_DEBUG_ZH << "FLANN匹配器创建成功，使用的算法为 "
                     << Img2MatchesParameterConverter::FLANNAlgorithmToString(flann_params.algorithm);
        LOG_DEBUG_EN << "FLANN matcher created successfully with "
                     << Img2MatchesParameterConverter::FLANNAlgorithmToString(flann_params.algorithm)
                     << " algorithm";

        return matcher;
    }

    void Img2MatchesPipeline::ShowProgressBar(size_t current, size_t total, const std::string &task_name, int bar_width)
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

// 注册插件
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
// Use single-parameter mode, automatically read PLUGIN_NAME from CMake (single source of truth)
REGISTRATION_PLUGIN(PluginMethods::Img2MatchesPipeline)
