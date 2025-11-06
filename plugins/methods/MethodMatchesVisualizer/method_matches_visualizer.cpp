/**
 * @file method_matches_visualizer.cpp
 * @brief Dual-view match visualization plugin implementation | 双视图匹配可视化插件实现
 */

#include "method_matches_visualizer.hpp"
#include <po_core/po_logger.hpp>
#include <iomanip>
#include <sstream>
#include <set>
#include <algorithm>

namespace PluginMethods
{

    MethodMatchesVisualizer::MethodMatchesVisualizer()
    {
        // Register required data types | 注册所需数据类型
        required_package_["data_matches"] = nullptr;  // Match data | 匹配数据
        required_package_["data_images"] = nullptr;   // Image path data | 图像路径数据
        required_package_["data_features"] = nullptr; // Feature point data | 特征点数据

        // Initialize default configuration path | 初始化默认配置路径
        InitializeDefaultConfigPath();

        LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 初始化完成";
        LOG_DEBUG_EN << "[MethodMatchesVisualizer] Initialization complete";
    }

    DataPtr MethodMatchesVisualizer::Run()
    {
        try
        {
            LOG_INFO_ZH << "[MethodMatchesVisualizer] === 开始匹配可视化处理 ===";
            LOG_INFO_EN << "[MethodMatchesVisualizer] === Starting match visualization processing ===";
            DisplayConfigInfo();

            // 1. Get input data | 1. 获取输入数据
            auto matches_data_ptr = required_package_["data_matches"];
            auto images_data_ptr = required_package_["data_images"];
            auto features_data_ptr = required_package_["data_features"];

            if (!matches_data_ptr || !images_data_ptr || !features_data_ptr)
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 缺少必需的输入数据";
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Missing required input data";
                return nullptr;
            }

            // 2. Convert data pointers | 2. 转换数据指针
            auto matches_ptr = GetDataPtr<Matches>(matches_data_ptr);
            auto image_paths_ptr = GetDataPtr<ImagePaths>(images_data_ptr);
            auto features_info_ptr = GetDataPtr<FeaturesInfo>(features_data_ptr);

            if (!matches_ptr || !image_paths_ptr || !features_info_ptr)
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 数据类型转换失败";
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Data type conversion failed";
                return nullptr;
            }

            if (matches_ptr->empty())
            {
                LOG_WARNING_ZH << "[MethodMatchesVisualizer] 匹配数据为空，没有可可视化的内容";
                LOG_WARNING_EN << "[MethodMatchesVisualizer] Match data is empty, no content to visualize";
                return matches_data_ptr;
            }

            // 3. Get configuration parameters | 3. 获取配置参数
            std::string export_folder = GetOptionAsString("export_folder", "storage/matches_visualization");
            bool enhance_outliers = GetOptionAsBool("enhance_outliers", false);
            bool batch_mode = GetOptionAsBool("batch_mode", true);
            bool save_empty_matches = GetOptionAsBool("save_empty_matches", false);
            size_t max_matches_per_image = GetOptionAsIndexT("max_matches_per_image", 1000);

            LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 输出文件夹: " << export_folder;
            LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 增强外点显示: " << (enhance_outliers ? "是" : "否");
            LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 批处理模式: " << (batch_mode ? "是" : "否");
            if (max_matches_per_image > 0)
            {
                LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 最大匹配数限制: " << max_matches_per_image;
            }
            LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 总共 " << matches_ptr->size() << " 个视图对需要处理";
            LOG_DEBUG_EN << "[MethodMatchesVisualizer] Output folder: " << export_folder;
            LOG_DEBUG_EN << "[MethodMatchesVisualizer] Enhance outliers display: " << (enhance_outliers ? "Yes" : "No");
            LOG_DEBUG_EN << "[MethodMatchesVisualizer] Batch mode: " << (batch_mode ? "Yes" : "No");
            if (max_matches_per_image > 0)
            {
                LOG_DEBUG_EN << "[MethodMatchesVisualizer] Maximum matches limit: " << max_matches_per_image;
            }
            LOG_DEBUG_EN << "[MethodMatchesVisualizer] Total " << matches_ptr->size() << " view pairs to process";

            // 4. Create output folder | 4. 创建输出文件夹
            std::filesystem::path output_path(export_folder);
            if (!CreateOutputFolder(output_path))
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 无法创建输出文件夹: " << export_folder;
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Unable to create output folder: " << export_folder;
                return nullptr;
            }

            // 5. Process view pairs one by one | 5. 逐个处理视图对
            size_t processed_count = 0;
            size_t success_count = 0;
            size_t total_count = matches_ptr->size();

            // 5.1 Process batch mode or specific view pair mode | 5.1 处理批处理模式或指定视图对模式
            if (!batch_mode)
            {
                // Single view pair mode | 单视图对模式
                ViewId specific_view_i = GetOptionAsIndexT("specific_view_i", 0);
                ViewId specific_view_j = GetOptionAsIndexT("specific_view_j", 1);
                ViewPair specific_pair(specific_view_i, specific_view_j);

                auto it = matches_ptr->find(specific_pair);
                if (it != matches_ptr->end())
                {
                    const auto &[view_pair, id_matches] = *it;
                    size_t total_matches, inlier_count, outlier_count;
                    StatisticsMatches(id_matches, total_matches, inlier_count, outlier_count);

                    if (total_matches > 0 || save_empty_matches)
                    {
                        bool success = DrawMatchesForViewPair(
                            view_pair, id_matches, *image_paths_ptr,
                            *features_info_ptr, output_path);
                        if (success)
                            success_count++;
                    }
                    processed_count = 1;
                    total_count = 1;
                }
                else
                {
                    LOG_ERROR_ZH << "[MethodMatchesVisualizer] 未找到指定的视图对 (" << specific_view_i << "," << specific_view_j << ")";
                    LOG_ERROR_EN << "[MethodMatchesVisualizer] Specified view pair not found (" << specific_view_i << "," << specific_view_j << ")";
                }
            }
            else
            {
                // Batch mode | 批处理模式
                for (const auto &[view_pair, id_matches] : *matches_ptr)
                {
                    processed_count++;

                    if (log_level_ >= PO_LOG_VERBOSE)
                    {
                        PrintProgress(processed_count, total_count, view_pair);
                    }

                    // Count match information | 统计匹配信息
                    size_t total_matches, inlier_count, outlier_count;
                    StatisticsMatches(id_matches, total_matches, inlier_count, outlier_count);

                    if (total_matches == 0 && !save_empty_matches)
                    {
                        LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 视图对 (" << view_pair.first << "," << view_pair.second << ") 没有匹配点，跳过";
                        LOG_DEBUG_EN << "[MethodMatchesVisualizer] View pair (" << view_pair.first << "," << view_pair.second << ") has no match points, skipping";
                        continue;
                    }

                    // Draw match image | 绘制匹配图
                    bool success = DrawMatchesForViewPair(
                        view_pair, id_matches, *image_paths_ptr,
                        *features_info_ptr, output_path);

                    if (success)
                    {
                        success_count++;
                        LOG_INFO_ZH << "[MethodMatchesVisualizer] 视图对 (" << view_pair.first << "," << view_pair.second << ") 处理成功 - "
                                    << "总匹配: " << total_matches
                                    << ", 内点: " << inlier_count
                                    << ", 外点: " << outlier_count;
                        LOG_INFO_EN << "[MethodMatchesVisualizer] View pair (" << view_pair.first << "," << view_pair.second << ") processed successfully - "
                                    << "Total matches: " << total_matches
                                    << ", Inliers: " << inlier_count
                                    << ", Outliers: " << outlier_count;
                    }
                    else
                    {
                        LOG_ERROR_ZH << "[MethodMatchesVisualizer] 视图对 (" << view_pair.first << "," << view_pair.second << ") 处理失败";
                        LOG_ERROR_EN << "[MethodMatchesVisualizer] View pair (" << view_pair.first << "," << view_pair.second << ") processing failed";
                    }
                }
            }

            // 6. Output processing result statistics | 6. 输出处理结果统计
            LOG_INFO_ZH << "[MethodMatchesVisualizer] === 处理完成 ===";
            LOG_INFO_ZH << "[MethodMatchesVisualizer] 总处理数量: " << processed_count;
            LOG_INFO_ZH << "[MethodMatchesVisualizer] 成功数量: " << success_count;
            LOG_INFO_ZH << "[MethodMatchesVisualizer] 失败数量: " << (processed_count - success_count);
            LOG_INFO_ZH << "[MethodMatchesVisualizer] 输出路径: " << output_path;
            LOG_INFO_EN << "[MethodMatchesVisualizer] === Processing complete ===";
            LOG_INFO_EN << "[MethodMatchesVisualizer] Total processed: " << processed_count;
            LOG_INFO_EN << "[MethodMatchesVisualizer] Successful: " << success_count;
            LOG_INFO_EN << "[MethodMatchesVisualizer] Failed: " << (processed_count - success_count);
            LOG_INFO_EN << "[MethodMatchesVisualizer] Output path: " << output_path;

            return matches_data_ptr; // Return original match data | 返回原始匹配数据
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodMatchesVisualizer] 匹配可视化过程中发生异常: " << e.what();
            LOG_ERROR_EN << "[MethodMatchesVisualizer] Exception during match visualization: " << e.what();
            return nullptr;
        }
    }

    bool MethodMatchesVisualizer::DrawMatchesForViewPair(
        const ViewPair &view_pair,
        const IdMatches &matches,
        const ImagePaths &image_paths,
        const FeaturesInfo &features_info,
        const std::filesystem::path &output_folder)
    {
        try
        {
            ViewId view_i = view_pair.first;
            ViewId view_j = view_pair.second;

            // Validate view ID validity | 验证视图ID有效性
            if (!ValidateViewIds(view_i, view_j, image_paths.size()))
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 无效的视图ID: (" << view_i << "," << view_j << ")";
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Invalid view IDs: (" << view_i << "," << view_j << ")";
                return false;
            }

            // Read images | 读取图像
            cv::Mat img1 = cv::imread(image_paths[view_i].first, cv::IMREAD_COLOR);
            cv::Mat img2 = cv::imread(image_paths[view_j].first, cv::IMREAD_COLOR);

            if (img1.empty() || img2.empty())
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 无法读取图像文件";
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Unable to read image files";
                return false;
            }

            // Extract feature points | 提取特征点
            std::vector<cv::KeyPoint> keypoints1, keypoints2;
            if (!ExtractKeyPointsFromFeatures(features_info, view_i, keypoints1) ||
                !ExtractKeyPointsFromFeatures(features_info, view_j, keypoints2))
            {
                LOG_ERROR_ZH << "[MethodMatchesVisualizer] 提取特征点失败";
                LOG_ERROR_EN << "[MethodMatchesVisualizer] Failed to extract feature points";
                return false;
            }

            // Convert match data | 转换匹配数据
            std::vector<cv::DMatch> cv_matches;
            size_t match_count = ConvertIdMatchesToCVMatches(matches, cv_matches);

            if (match_count == 0 && !GetOptionAsBool("save_empty_matches", false))
            {
                LOG_WARNING_ZH << "[MethodMatchesVisualizer] 没有有效的匹配点";
                LOG_WARNING_EN << "[MethodMatchesVisualizer] No valid match points";
                return false;
            }

            // Declare inlier flag vector | 声明内点标志向量
            std::vector<bool> inlier_flags;

            // Apply distributed match point selection algorithm | 应用分布式匹配点选择算法
            size_t max_matches = GetOptionAsIndexT("max_matches_per_image", 1000);
            bool enable_distributed_selection = GetOptionAsBool("enable_distributed_selection", true);

            if (max_matches > 0 && cv_matches.size() > max_matches)
            {
                if (enable_distributed_selection)
                {
                    // Use distributed selection algorithm | 使用分布式选择算法
                    std::vector<cv::DMatch> selected_matches;
                    std::vector<bool> selected_inlier_flags;
                    SelectDistributedMatches(cv_matches, matches, keypoints1, keypoints2,
                                             img1.size(), img2.size(), max_matches,
                                             selected_matches, selected_inlier_flags);
                    cv_matches = selected_matches;
                    inlier_flags = selected_inlier_flags;
                }
                else
                {
                    // Simple truncation | 简单截断
                    cv_matches.resize(max_matches);
                }

                LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 匹配数量限制为: " << max_matches
                             << " (使用" << (enable_distributed_selection ? "分布式" : "简单")
                             << "选择算法)";
                LOG_DEBUG_EN << "[MethodMatchesVisualizer] Match count limited to: " << max_matches
                             << " (using " << (enable_distributed_selection ? "distributed" : "simple")
                             << " selection algorithm)";
            }
            else
            {
                // Extract inlier flags (original logic) | 提取内点标志（原有逻辑）
                size_t actual_matches = std::min(cv_matches.size(), matches.size());
                inlier_flags.reserve(actual_matches);
                for (size_t i = 0; i < actual_matches; ++i)
                {
                    inlier_flags.push_back(matches[i].is_inlier);
                }
            }

            // Generate output file path | 生成输出文件路径
            std::string filename = GenerateOutputFileName(view_i, view_j);
            std::filesystem::path output_path = output_folder / filename;

            // Get enhance_outliers parameter | 获取enhance_outliers参数
            bool enhance_outliers = GetOptionAsBool("enhance_outliers", false);

            // Draw and save match image | 绘制并保存匹配图
            return DrawAndSaveMatches(img1, img2, keypoints1, keypoints2,
                                      cv_matches, inlier_flags, output_path, enhance_outliers);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodMatchesVisualizer] 绘制视图对匹配时发生异常: " << e.what();
            LOG_ERROR_EN << "[MethodMatchesVisualizer] Exception when drawing view pair matches: " << e.what();
            return false;
        }
    }

    size_t MethodMatchesVisualizer::ConvertIdMatchesToCVMatches(
        const IdMatches &matches,
        std::vector<cv::DMatch> &cv_matches,
        bool enhance_outliers)
    {
        cv_matches.clear();
        cv_matches.reserve(matches.size());

        for (size_t idx = 0; idx < matches.size(); ++idx)
        {
            const auto &match = matches[idx];

            cv::DMatch cv_match;
            cv_match.queryIdx = static_cast<int>(match.i);
            cv_match.trainIdx = static_cast<int>(match.j);
            cv_match.distance = 0.0f; // Distance information not important in visualization | 距离信息在可视化中不重要
            cv_match.imgIdx = 0;      // Single image match | 单图像匹配

            cv_matches.push_back(cv_match);
        }

        return cv_matches.size();
    }

    bool MethodMatchesVisualizer::DrawAndSaveMatches(
        const cv::Mat &img1,
        const cv::Mat &img2,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const std::vector<cv::DMatch> &matches,
        const std::vector<bool> &inlier_flags,
        const std::filesystem::path &output_path,
        bool enhance_outliers)
    {
        try
        {
            // Create side-by-side image | 创建并排图像
            cv::Mat output_img;
            cv::hconcat(img1, img2, output_img);

            // Get drawing parameters | 获取绘制参数
            int keypoint_radius = GetOptionAsIndexT("keypoint_radius", 8);
            int line_thickness = GetOptionAsIndexT("line_thickness", 2);
            double line_alpha = GetOptionAsFloat("line_alpha", 0.6);
            double keypoint_alpha = GetOptionAsFloat("keypoint_alpha", 0.9);
            bool enable_color_diversity = GetOptionAsBool("enable_color_diversity", true);
            std::string color_mode = GetOptionAsString("color_mode", "rainbow");

            // Create transparency overlay image | 创建透明度叠加图像
            cv::Mat overlay = output_img.clone();

            // Draw all matches | 绘制所有匹配
            for (size_t i = 0; i < matches.size(); ++i)
            {
                const cv::DMatch &match = matches[i];

                if (match.queryIdx >= keypoints1.size() || match.trainIdx >= keypoints2.size())
                    continue;

                const cv::KeyPoint &kp1 = keypoints1[match.queryIdx];
                const cv::KeyPoint &kp2 = keypoints2[match.trainIdx];

                // Calculate point position in second image (add offset) | 计算第二张图像中的点位置（加上偏移量）
                cv::Point2f pt1(kp1.pt.x, kp1.pt.y);
                cv::Point2f pt2(kp2.pt.x + img1.cols, kp2.pt.y);

                cv::Scalar line_color, point_color;

                if (enhance_outliers && inlier_flags.size() == matches.size())
                {
                    // Set color based on inlier/outlier | 根据内外点设置颜色
                    bool is_inlier = inlier_flags[i];
                    if (is_inlier)
                    {
                        // Inliers use diversified color mode | 内点使用多样化颜色模式
                        if (enable_color_diversity)
                        {
                            line_color = point_color = GenerateDistinctColor(i, matches.size(), color_mode);
                        }
                        else
                        {
                            // Inliers use default color | 内点使用默认颜色
                            line_color = cv::Scalar(255, 255, 0);  // Cyan line | 青色线条
                            point_color = cv::Scalar(0, 255, 255); // Yellow keypoint | 黄色关键点
                        }
                    }
                    else
                    {
                        // Outliers always use red to highlight | 外点始终使用红色突出显示
                        line_color = point_color = cv::Scalar(0, 0, 255); // Red | 红色
                    }
                }
                else
                {
                    // Normal drawing mode | 普通绘制模式
                    if (enable_color_diversity)
                    {
                        // Assign different color for each match | 为每个匹配分配不同颜色
                        line_color = point_color = GenerateDistinctColor(i, matches.size(), color_mode);
                    }
                    else
                    {
                        // Use default color | 使用默认颜色
                        line_color = cv::Scalar(255, 255, 0);  // Cyan line | 青色线条
                        point_color = cv::Scalar(0, 255, 255); // Yellow keypoint | 黄色关键点
                    }
                }

                // Draw match line | 绘制匹配线
                cv::line(overlay, pt1, pt2, line_color, line_thickness);

                // Draw keypoints | 绘制关键点
                cv::circle(overlay, pt1, keypoint_radius, point_color, -1);
                cv::circle(overlay, pt2, keypoint_radius, point_color, -1);
            }

            // Apply transparency blending | 应用透明度混合
            cv::addWeighted(output_img, 1.0 - line_alpha, overlay, line_alpha, 0.0, output_img);

            // Add text information | 添加文字信息
            bool show_statistics = GetOptionAsBool("show_statistics", true);
            if (!show_statistics)
            {
                // Save image | 保存图像
                bool success = cv::imwrite(output_path.string(), output_img);

                if (success)
                {
                    LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 匹配图像已保存: " << output_path;
                    LOG_DEBUG_EN << "[MethodMatchesVisualizer] Match image saved: " << output_path;
                }
                return success;
            }

            int font_face = cv::FONT_HERSHEY_SIMPLEX;
            double font_scale = GetOptionAsFloat("font_scale", 0.7);
            int thickness = GetOptionAsIndexT("line_thickness", 2);
            cv::Scalar text_color(255, 255, 255); // White text | 白色文字

            // Statistics information | 统计信息
            size_t total_matches = matches.size();
            size_t inlier_count = 0;
            for (bool flag : inlier_flags)
            {
                if (flag)
                    inlier_count++;
            }
            size_t outlier_count = total_matches - inlier_count;

            // Draw statistics on image | 在图像上绘制统计信息
            std::stringstream ss;
            ss << "Total: " << total_matches << ", Inliers: " << inlier_count
               << ", Outliers: " << outlier_count;
            std::string info_text = ss.str();

            cv::Size text_size = cv::getTextSize(info_text, font_face, font_scale, thickness, nullptr);
            cv::Point text_position(10, text_size.height + 10);

            // Draw text background | 绘制文字背景
            cv::rectangle(output_img,
                          cv::Point(text_position.x - 5, text_position.y - text_size.height - 5),
                          cv::Point(text_position.x + text_size.width + 5, text_position.y + 5),
                          cv::Scalar(0, 0, 0), cv::FILLED);

            // Draw text | 绘制文字
            cv::putText(output_img, info_text, text_position, font_face, font_scale, text_color, thickness);

            // Save image | 保存图像
            bool success = cv::imwrite(output_path.string(), output_img);

            if (success)
            {
                LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 匹配图像已保存: " << output_path;
                LOG_DEBUG_EN << "[MethodMatchesVisualizer] Match image saved: " << output_path;
            }

            return success;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodMatchesVisualizer] 绘制和保存匹配图像时发生异常: " << e.what();
            LOG_ERROR_EN << "[MethodMatchesVisualizer] Exception when drawing and saving match image: " << e.what();
            return false;
        }
    }

    bool MethodMatchesVisualizer::ExtractKeyPointsFromFeatures(
        const FeaturesInfo &features_info,
        ViewId view_id,
        std::vector<cv::KeyPoint> &keypoints)
    {
        keypoints.clear();

        if (view_id >= features_info.size())
        {
            LOG_ERROR_ZH << "[MethodMatchesVisualizer] 视图ID超出特征信息范围: " << view_id;
            LOG_ERROR_EN << "[MethodMatchesVisualizer] View ID out of feature info range: " << view_id;
            return false;
        }

        const auto *image_features = features_info[view_id];
        const auto &feature_points = image_features->GetFeaturePoints();
        const size_t num_features = feature_points.size();
        keypoints.reserve(num_features);

        // 🚀 Performance optimization: Use batch SOA access (zero-copy, SIMD-friendly)
        // 🚀 性能优化：使用批量SOA访问（零拷贝，SIMD友好）
        const auto &coords = feature_points.GetCoordsRef(); // 2×N Eigen matrix | 2×N Eigen矩阵
        const auto &sizes = feature_points.GetSizesRef();   // std::vector<float>
        const auto &angles = feature_points.GetAnglesRef(); // std::vector<float>

        for (size_t i = 0; i < num_features; ++i)
        {
            cv::KeyPoint kp;
            // Direct access to contiguous memory | 直接访问连续内存
            kp.pt.x = static_cast<float>(coords(0, i));
            kp.pt.y = static_cast<float>(coords(1, i));
            kp.size = sizes[i];
            kp.angle = angles[i];
            kp.response = 1.0f; // Default response value | 默认响应值
            kp.octave = 0;      // Default pyramid level | 默认金字塔层级
            kp.class_id = -1;   // Default class ID | 默认类别ID

            keypoints.push_back(kp);
        }

        return true;
    }

    bool MethodMatchesVisualizer::ValidateViewIds(ViewId view_i, ViewId view_j, size_t max_views)
    {
        if (view_i >= max_views || view_j >= max_views)
        {
            return false;
        }
        if (view_i == view_j)
        {
            return false;
        }
        return true;
    }

    bool MethodMatchesVisualizer::CreateOutputFolder(const std::filesystem::path &output_folder)
    {
        try
        {
            if (!std::filesystem::exists(output_folder))
            {
                std::filesystem::create_directories(output_folder);
                LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 创建输出文件夹: " << output_folder;
                LOG_DEBUG_EN << "[MethodMatchesVisualizer] Created output folder: " << output_folder;
            }
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodMatchesVisualizer] 创建输出文件夹失败: " << e.what();
            LOG_ERROR_EN << "[MethodMatchesVisualizer] Failed to create output folder: " << e.what();
            return false;
        }
    }

    std::string MethodMatchesVisualizer::GenerateOutputFileName(ViewId view_i, ViewId view_j)
    {
        std::stringstream ss;
        ss << "view_pairs(" << view_i << "," << view_j << ").png";
        return ss.str();
    }

    void MethodMatchesVisualizer::StatisticsMatches(
        const IdMatches &matches,
        size_t &total_matches,
        size_t &inlier_count,
        size_t &outlier_count)
    {
        total_matches = matches.size();
        inlier_count = 0;
        outlier_count = 0;

        for (const auto &match : matches)
        {
            if (match.is_inlier)
            {
                inlier_count++;
            }
            else
            {
                outlier_count++;
            }
        }
    }

    void MethodMatchesVisualizer::PrintProgress(size_t current, size_t total, const ViewPair &view_pair)
    {
        double progress = static_cast<double>(current) / static_cast<double>(total) * 100.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1)
           << "处理进度: " << progress << "% (" << current << "/" << total
           << ") - 视图对 (" << view_pair.first << "," << view_pair.second << ")";
        LOG_DEBUG_ZH << ss.str();
        ss.str("");
        ss << std::fixed << std::setprecision(1)
           << "Processing progress: " << progress << "% (" << current << "/" << total
           << ") - View pair (" << view_pair.first << "," << view_pair.second << ")";
        LOG_DEBUG_EN << ss.str();
    }

    void MethodMatchesVisualizer::SelectDistributedMatches(
        const std::vector<cv::DMatch> &all_matches,
        const IdMatches &matches,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const cv::Size &img1_size,
        const cv::Size &img2_size,
        size_t target_count,
        std::vector<cv::DMatch> &selected_matches,
        std::vector<bool> &selected_inlier_flags)
    {
        selected_matches.clear();
        selected_inlier_flags.clear();

        if (all_matches.empty() || target_count == 0)
            return;

        // If number of matches is already less than target, return all | 如果匹配数量已经少于目标数量，直接返回所有匹配
        if (all_matches.size() <= target_count)
        {
            selected_matches = all_matches;
            selected_inlier_flags.reserve(all_matches.size());
            for (size_t i = 0; i < all_matches.size(); ++i)
            {
                selected_inlier_flags.push_back(i < matches.size() ? matches[i].is_inlier : true);
            }
            return;
        }

        // Create grid distribution strategy | 创建网格分布策略
        const int grid_cols = 8; // Number of grid columns | 网格列数
        const int grid_rows = 6; // Number of grid rows | 网格行数
        const int total_grids = grid_cols * grid_rows;

        // Size of each grid | 每个网格的尺寸
        int grid_width1 = img1_size.width / grid_cols;
        int grid_height1 = img1_size.height / grid_rows;
        int grid_width2 = img2_size.width / grid_cols;
        int grid_height2 = img2_size.height / grid_rows;

        // Create match candidate list for each grid | 为每个网格创建匹配候选列表
        std::vector<std::vector<size_t>> grid_matches(total_grids);

        // Group matches by grid | 将匹配按网格分组
        for (size_t i = 0; i < all_matches.size(); ++i)
        {
            const cv::DMatch &match = all_matches[i];

            if (match.queryIdx >= keypoints1.size() || match.trainIdx >= keypoints2.size())
                continue;

            const cv::KeyPoint &kp1 = keypoints1[match.queryIdx];
            const cv::KeyPoint &kp2 = keypoints2[match.trainIdx];

            // Calculate position in grid | 计算在网格中的位置
            int grid_x1 = std::min(static_cast<int>(kp1.pt.x) / grid_width1, grid_cols - 1);
            int grid_y1 = std::min(static_cast<int>(kp1.pt.y) / grid_height1, grid_rows - 1);
            int grid_x2 = std::min(static_cast<int>(kp2.pt.x) / grid_width2, grid_cols - 1);
            int grid_y2 = std::min(static_cast<int>(kp2.pt.y) / grid_height2, grid_rows - 1);

            // Assign match to intersection of two image grids (simple strategy: use first image's grid) | 将匹配分配到两个图像的网格交集（简单策略：使用第一张图像的网格）
            int grid_index = grid_y1 * grid_cols + grid_x1;
            grid_matches[grid_index].push_back(i);
        }

        // Calculate number of matches to select per grid | 计算每个网格应该选择的匹配数量
        int matches_per_grid = target_count / total_grids;
        int remaining_matches = target_count % total_grids;

        // Select matches from each grid | 从每个网格中选择匹配
        std::vector<size_t> selected_indices;
        selected_indices.reserve(target_count);

        for (int grid_idx = 0; grid_idx < total_grids; ++grid_idx)
        {
            const auto &matches_in_grid = grid_matches[grid_idx];
            if (matches_in_grid.empty())
                continue;

            // Number to select for current grid | 当前网格要选择的数量
            int current_grid_count = matches_per_grid;
            if (remaining_matches > 0)
            {
                current_grid_count++;
                remaining_matches--;
            }

            // If number of matches in this grid is less than required, select all | 如果该网格的匹配数少于要求数量，全部选择
            if (matches_in_grid.size() <= static_cast<size_t>(current_grid_count))
            {
                selected_indices.insert(selected_indices.end(),
                                        matches_in_grid.begin(), matches_in_grid.end());
            }
            else
            {
                // Uniform interval selection | 均匀间隔选择
                for (int i = 0; i < current_grid_count; ++i)
                {
                    size_t idx = i * matches_in_grid.size() / current_grid_count;
                    selected_indices.push_back(matches_in_grid[idx]);
                }
            }
        }

        // If selected count is insufficient, supplement from remaining matches | 如果选择的数量不足，从剩余匹配中补充
        if (selected_indices.size() < target_count)
        {
            std::set<size_t> selected_set(selected_indices.begin(), selected_indices.end());

            for (size_t i = 0; i < all_matches.size() && selected_indices.size() < target_count; ++i)
            {
                if (selected_set.find(i) == selected_set.end())
                {
                    selected_indices.push_back(i);
                }
            }
        }

        // Sort selected matches by inlier priority | 按照内点优先级排序选择的匹配
        std::sort(selected_indices.begin(), selected_indices.end(),
                  [&matches](size_t a, size_t b)
                  {
                      bool a_inlier = (a < matches.size()) ? matches[a].is_inlier : false;
                      bool b_inlier = (b < matches.size()) ? matches[b].is_inlier : false;
                      return a_inlier > b_inlier; // Inliers first | 内点优先
                  });

        // If selected count exceeds target, truncate to target count | 如果选择数量超过目标，截断到目标数量
        if (selected_indices.size() > target_count)
        {
            selected_indices.resize(target_count);
        }

        // Build final result | 构建最终结果
        selected_matches.reserve(selected_indices.size());
        selected_inlier_flags.reserve(selected_indices.size());

        for (size_t idx : selected_indices)
        {
            selected_matches.push_back(all_matches[idx]);
            selected_inlier_flags.push_back(idx < matches.size() ? matches[idx].is_inlier : true);
        }

        size_t inlier_count = std::count(selected_inlier_flags.begin(), selected_inlier_flags.end(), true);
        LOG_DEBUG_ZH << "[MethodMatchesVisualizer] 分布式选择完成: " << selected_matches.size() << " 个匹配 "
                     << "(内点: " << inlier_count << ", 外点: " << (selected_matches.size() - inlier_count) << ")";
        LOG_DEBUG_EN << "[MethodMatchesVisualizer] Distributed selection complete: " << selected_matches.size() << " matches "
                     << "(Inliers: " << inlier_count << ", Outliers: " << (selected_matches.size() - inlier_count) << ")";
    }

    cv::Scalar MethodMatchesVisualizer::GenerateDistinctColor(size_t index, size_t total_count, const std::string &mode)
    {
        if (total_count == 0)
            return cv::Scalar(255, 255, 255);

        float hue_step = 360.0f / std::max(static_cast<size_t>(1), total_count);
        float hue = fmod(index * hue_step, 360.0f);

        if (mode == "rainbow")
        {
            // Rainbow spectrum, fixed saturation and brightness | 彩虹色谱，饱和度和亮度固定
            float saturation = 0.8f;
            float brightness = 0.9f;
            return HSVtoBGR(hue, saturation, brightness);
        }
        else if (mode == "hsv")
        {
            // HSV color space uniform distribution | HSV颜色空间均匀分布
            float saturation = 0.7f + 0.3f * (index % 3) / 3.0f;
            float brightness = 0.8f + 0.2f * ((index / 3) % 2);
            return HSVtoBGR(hue, saturation, brightness);
        }
        else if (mode == "category")
        {
            // Predefined category colors | 预定义类别颜色
            static const std::vector<cv::Scalar> category_colors = {
                cv::Scalar(255, 0, 0),   // Red | 红色
                cv::Scalar(0, 255, 0),   // Green | 绿色
                cv::Scalar(0, 0, 255),   // Blue | 蓝色
                cv::Scalar(255, 255, 0), // Cyan | 青色
                cv::Scalar(255, 0, 255), // Magenta | 洋红
                cv::Scalar(0, 255, 255), // Yellow | 黄色
                cv::Scalar(128, 0, 255), // Purple | 紫色
                cv::Scalar(255, 128, 0), // Orange | 橙色
                cv::Scalar(0, 128, 255), // Light blue | 浅蓝
                cv::Scalar(255, 0, 128), // Pink | 粉红
                cv::Scalar(128, 255, 0), // Light green | 浅绿
                cv::Scalar(0, 255, 128)  // Teal | 青绿
            };
            return category_colors[index % category_colors.size()];
        }
        else
        {
            // Default random mode | 默认random模式
            std::srand(index * 7919);       // Use prime seed | 使用质数种子
            int r = std::rand() % 200 + 55; // Avoid too dark colors | 避免太暗的颜色
            int g = std::rand() % 200 + 55;
            int b = std::rand() % 200 + 55;
            return cv::Scalar(b, g, r); // BGR format | BGR格式
        }
    }

    cv::Scalar MethodMatchesVisualizer::GenerateWarmColor(size_t index, size_t total_count)
    {
        if (total_count == 0)
            return cv::Scalar(0, 165, 255); // Orange | 橙色

        // Warm color series: red to yellow (0-60 degrees) | 暖色系：红色到黄色 (0-60度)
        float hue_range = 60.0f;
        float hue = (index * hue_range / std::max(static_cast<size_t>(1), total_count));
        float saturation = 0.8f + 0.2f * (index % 2);       // 0.8-1.0
        float brightness = 0.8f + 0.2f * ((index / 2) % 2); // 0.8-1.0

        return HSVtoBGR(hue, saturation, brightness);
    }

    cv::Scalar MethodMatchesVisualizer::GenerateCoolColor(size_t index, size_t total_count)
    {
        if (total_count == 0)
            return cv::Scalar(255, 0, 0); // Blue | 蓝色

        // Cool color series: cyan to blue (180-240 degrees) | 冷色系：青色到蓝色 (180-240度)
        float hue_start = 180.0f;
        float hue_range = 60.0f;
        float hue = hue_start + (index * hue_range / std::max(static_cast<size_t>(1), total_count));
        float saturation = 0.7f + 0.3f * (index % 2);       // 0.7-1.0
        float brightness = 0.7f + 0.3f * ((index / 2) % 2); // 0.7-1.0

        return HSVtoBGR(hue, saturation, brightness);
    }

    cv::Scalar MethodMatchesVisualizer::HSVtoBGR(float hue, float saturation, float brightness)
    {
        // Convert HSV to BGR | 将HSV转换为BGR
        cv::Mat hsv(1, 1, CV_32FC3, cv::Scalar(hue, saturation * 255, brightness * 255));
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);

        cv::Vec3f bgr_pixel = bgr.at<cv::Vec3f>(0, 0);
        return cv::Scalar(bgr_pixel[0], bgr_pixel[1], bgr_pixel[2]);
    }

} // namespace PluginMethods

// Register plugin | 注册插件
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::MethodMatchesVisualizer)