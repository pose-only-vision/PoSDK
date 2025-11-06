/**
 * @file img2matches_viewer_mode.cpp
 * @brief Image Feature Matching Visualization Mode Implementation | 图像特征匹配可视化模式实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "img2matches_pipeline.hpp"
#include <po_core/po_logger.hpp>
#include <po_core/types.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <common/image_viewer/image_viewer.hpp>
#include <filesystem>
#include <regex>
#include <algorithm>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace common;

    DataPtr Img2MatchesPipeline::RunViewerMode()
    {
        try
        {
            // 1. Get image data | 获取图像数据
            auto data_images_ptr = required_package_["data_images"];
            if (!data_images_ptr)
                throw std::runtime_error("No image data provided | 没有提供图像数据");

            auto image_paths_ptr = GetDataPtr<ImagePaths>(data_images_ptr);
            if (!image_paths_ptr || image_paths_ptr->empty())
                throw std::runtime_error("Empty image paths | 图像路径为空");

            // 2. Create a sorted image list consistent with fast mode | 创建与fast模式一致的排序图像列表
            std::vector<std::pair<std::string, std::string>> valid_image_pairs; // (filename_number, img_path) | (文件名数字, 图像路径)
            for (const auto &[img_path, is_valid] : *image_paths_ptr)
            {
                if (!is_valid)
                    continue;

                // Extract number from image filename for sorting (consistent with fast mode logic)
                // 从图像文件名提取数字用于排序（与fast模式逻辑一致）
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
                    LOG_ERROR_ZH << "无法从文件名中提取数字: " << filename;
                    LOG_ERROR_EN << "Cannot extract number from filename: " << filename;
                    continue;
                }
            }

            // Sort by filename number (consistent with fast mode) | 按文件名数字排序（与fast模式一致）
            std::sort(valid_image_pairs.begin(), valid_image_pairs.end());

            // 3. Parse the view pair to display, using sorted indices | 解析要显示的视图对，使用排序后的索引
            auto [i, j] = ParseViewPair();
            ValidateViewPairIndices(i, j, valid_image_pairs.size());

            // 4. Read the specified image pair (using sorted list) | 读取指定的图像对（使用排序后的列表）
            const std::string &path1 = valid_image_pairs[i].second;
            const std::string &path2 = valid_image_pairs[j].second;

            cv::Mat img1 = cv::imread(path1, cv::IMREAD_GRAYSCALE);
            cv::Mat img2 = cv::imread(path2, cv::IMREAD_GRAYSCALE);

            if (img1.empty() || img2.empty())
                throw std::runtime_error("Failed to load images | 无法加载图像");

            // 5. Detect keypoints (using processing logic consistent with fast mode)
            // 检测特征点（使用与fast模式一致的处理逻辑）
            LOG_INFO_ZH << "========== Viewer模式：开始特征提取+匹配 ==========";
            LOG_INFO_EN << "========== Viewer Mode: Starting Feature Extraction + Matching ==========";

            std::vector<cv::KeyPoint> keypoints1, keypoints2;
            cv::Mat descriptors1, descriptors2;

            // Detect features for img1 | 为img1检测特征
            if (params_.base.detector_type == "SIFT")
            {
                // SIFT-specific processing: apply first_octave image preprocessing
                // SIFT特有的处理：应用first_octave图像预处理
                cv::Mat processed_img1 = ApplyFirstOctaveProcessing(img1);
                DetectFeatures(processed_img1, keypoints1, descriptors1);

                // Adjust keypoint coordinates if image scaling was performed
                // 如果进行了图像缩放，需要调整特征点坐标
                if (params_.sift.first_octave != 0)
                {
                    AdjustKeypointsForScaling(keypoints1, params_.sift.first_octave);
                }

                // Ensure descriptor type is CV_32F (required by matcher)
                // 确保描述子类型为CV_32F（匹配器要求）
                if (!descriptors1.empty() && descriptors1.type() != CV_32F)
                {
                    LOG_DEBUG_ZH << "将描述子1从类型 " << descriptors1.type() << " 转换为 CV_32F 以确保兼容性";
                    LOG_DEBUG_EN << "Converting descriptors1 from type " << descriptors1.type() << " to CV_32F for compatibility";
                    descriptors1.convertTo(descriptors1, CV_32F);
                }

                // Apply RootSIFT normalization (if enabled) | 应用RootSIFT归一化（如果启用）
                if (params_.sift.root_sift && !descriptors1.empty())
                {
                    ApplyRootSIFTNormalization(descriptors1);
                }
            }
            else
            {
                DetectFeatures(img1, keypoints1, descriptors1);
            }

            // Detect features for img2 | 为img2检测特征
            if (params_.base.detector_type == "SIFT")
            {
                // SIFT-specific processing: apply first_octave image preprocessing
                // SIFT特有的处理：应用first_octave图像预处理
                cv::Mat processed_img2 = ApplyFirstOctaveProcessing(img2);
                DetectFeatures(processed_img2, keypoints2, descriptors2);

                // Adjust keypoint coordinates if image scaling was performed
                // 如果进行了图像缩放，需要调整特征点坐标
                if (params_.sift.first_octave != 0)
                {
                    AdjustKeypointsForScaling(keypoints2, params_.sift.first_octave);
                }

                // Ensure descriptor type is CV_32F (required by matcher)
                // 确保描述子类型为CV_32F（匹配器要求）
                if (!descriptors2.empty() && descriptors2.type() != CV_32F)
                {
                    LOG_DEBUG_ZH << "将描述子2从类型 " << descriptors2.type() << " 转换为 CV_32F 以确保兼容性";
                    LOG_DEBUG_EN << "Converting descriptors2 from type " << descriptors2.type() << " to CV_32F for compatibility";
                    descriptors2.convertTo(descriptors2, CV_32F);
                }

                // Apply RootSIFT normalization (if enabled) | 应用RootSIFT归一化（如果启用）
                if (params_.sift.root_sift && !descriptors2.empty())
                {
                    ApplyRootSIFTNormalization(descriptors2);
                }
            }
            else
            {
                DetectFeatures(img2, keypoints2, descriptors2);
            }

            // 6. Set up image viewer | 设置图像查看器
            auto &viewer = ImageViewer::Instance();
            ImageViewer::DisplayOptions options;
            options.match_color = cv::Scalar(0, 255, 255);
            options.line_thickness = 2;
            options.line_transparency = 0.5f; // Set default transparency | 设置默认透明度
            viewer.SetDisplayOptions(options);

            // 7. Create parameter adjustment interface | 创建参数调整界面
            std::string window_name = "Feature Matches";
            cv::namedWindow(window_name, cv::WINDOW_NORMAL);

            // Create callback data | 创建回调数据
            MatcherCallbackData cb_data{
                &img1, &img2,
                &keypoints1, &keypoints2,
                nullptr, // matches will be set later | matches将在后面设置
                &descriptors1, &descriptors2,
                this,
                &viewer // Pass viewer pointer | 传入viewer指针
            };

            LOG_INFO_ZH << "正在匹配视图对 (" << i << ", " << j << ")...";
            LOG_INFO_EN << "Matching view pair (" << i << ", " << j << ")...";
            std::vector<cv::DMatch> matches = MatchFeatures(descriptors1, descriptors2);
            LOG_INFO_ZH << "找到 " << matches.size() << " 个匹配点";
            LOG_INFO_EN << "Found " << matches.size() << " matches";
            cb_data.matches = &matches;

            // Create ratio threshold trackbar | 创建ratio阈值控制条
            cv::createTrackbar("Ratio Threshold", window_name, nullptr, 100, [](int pos, void *userdata)
                               {
                auto* data = static_cast<MatcherCallbackData*>(userdata);
                float ratio = pos / 100.0f;
                
                // Update plugin parameters | 更新插件参数
                data->plugin->params_.matching.ratio_thresh = ratio;
                
                // Recalculate matches | 重新计算匹配
                data->matches->clear();
                *(data->matches) = data->plugin->MatchFeatures(*(data->descriptors1), 
                                                             *(data->descriptors2));
                
                // Redisplay matching results | 重新显示匹配结果
                data->plugin->VisualizeMatches(*(data->img1), *(data->img2),
                                             *(data->keypoints1), *(data->keypoints2),
                                             *(data->matches), "Feature Matches"); }, &cb_data);

            // Create transparency trackbar | 创建透明度控制条
            cv::createTrackbar("Line Transparency", window_name, nullptr, 100, [](int pos, void *userdata)
                               {
                auto* data = static_cast<MatcherCallbackData*>(userdata);
                float transparency = pos / 100.0f;
                
                // Update display options | 更新显示选项
                ImageViewer::DisplayOptions options = data->viewer->GetDisplayOptions();
                options.line_transparency = transparency;
                data->viewer->SetDisplayOptions(options);
                
                // Redisplay matching results | 重新显示匹配结果
                data->plugin->VisualizeMatches(*(data->img1), *(data->img2),
                                             *(data->keypoints1), *(data->keypoints2),
                                             *(data->matches), "Feature Matches"); }, &cb_data);

            // Set initial values | 设置初始值
            float initial_ratio = params_.matching.ratio_thresh;
            cv::setTrackbarPos("Ratio Threshold", window_name, static_cast<int>(initial_ratio * 100));
            cv::setTrackbarPos("Line Transparency", window_name,
                               static_cast<int>(options.line_transparency * 100));

            // Initial display | 初始显示
            VisualizeMatches(img1, img2, keypoints1, keypoints2, matches, window_name);

            // 8. Wait for user confirmation of parameters | 等待用户确认参数
            LOG_INFO_ZH << "\n调整参数并按下：";
            LOG_INFO_ZH << "  'Enter' 将当前参数应用到所有图像";
            LOG_INFO_ZH << "  'Esc' 取消操作";
            LOG_INFO_EN << "\nAdjust parameters and press:";
            LOG_INFO_EN << "  'Enter' to apply current parameters to all images";
            LOG_INFO_EN << "  'Esc' to cancel";

            bool apply_to_all = false;
            while (true)
            {
                char key = cv::waitKey(100);
                if (key == 27)
                { // Esc
                    cv::destroyWindow(window_name);
                    return nullptr;
                }
                if (key == 13)
                { // Enter
                    apply_to_all = true;
                    break;
                }
            }

            cv::destroyWindow(window_name);

            // 9. If user confirms, process all images with current parameters
            // 如果用户确认，使用当前参数处理所有图像
            if (apply_to_all)
            {
                LOG_INFO_ZH << "将参数应用到所有图像中...";
                LOG_INFO_EN << "Applying parameters to all images...";

                // Update method options to reflect user-adjusted parameters
                // 更新方法选项以反映用户调整的参数
                auto updated_options = Img2MatchesParameterConverter::ToMethodOptions(params_);
                for (const auto &[key, value] : updated_options)
                {
                    method_options_[key] = value;
                }

                return RunFastMode(); // Run with adjusted parameters | 使用调整后的参数运行
            }

            return nullptr;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[Img2MatchesPipeline] RunViewerMode中发生错误: " << e.what();
            LOG_ERROR_EN << "[Img2MatchesPipeline] Error in RunViewerMode: " << e.what();
            return nullptr;
        }
    }

} // namespace PluginMethods
