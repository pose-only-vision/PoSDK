/**
 * @file img2features_pipeline.cpp
 * @brief Image feature extraction pipeline implementation | 图像特征提取流水线实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "img2features_pipeline.hpp"
#include <po_core/po_logger.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <po_core/types.hpp>
#include <common/image_viewer/image_viewer.hpp>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace PoSDK::types;
    using namespace PoSDK::Interface;

    DataPtr Img2FeaturesPipeline::Run()
    {
        // Load general configuration first | 先加载通用配置
        InitializeDefaultConfigPath(); // This loads [method_img2features] section | 这会加载 [method_img2features] 部分

        // Load specific configuration based on detector_type | 根据detector_type加载特定配置
        const std::string &detector_type = method_options_["detector_type"];
        InitializeDefaultConfigPath(detector_type);

        // Use wrapped display function | 使用封装的显示函数
        DisplayConfigInfo();

        // Choose different implementation based on run mode | 根据运行模式选择不同的实现
        if (method_options_["run_mode"] == "viewer")
        {
            return RunWithImageViewer();
        }
        else
        {
            return RunFast();
        }

        return nullptr;
    }

    cv::Ptr<cv::Feature2D> Img2FeaturesPipeline::CreateDetector()
    {
        const std::string &detector_type = method_options_["detector_type"];

        if (detector_type == "KAZE")
        {
            return cv::KAZE::create(
                false,                                      // extended | 是否扩展描述子
                false,                                      // upright | 是否保持直立
                GetOptionAsFloat("kaze_threshold", 0.001f), // threshold | 阈值
                GetOptionAsIndexT("kaze_nOctaves", 4),      // number of octaves | 八度数量
                GetOptionAsIndexT("kaze_nOctaveLayers", 4), // number of octave layers | 八度层数
                cv::KAZE::DIFF_PM_G2                        // diffusivity | 扩散性
            );
        }
        else if (detector_type == "FAST")
        {
            return cv::FastFeatureDetector::create(
                GetOptionAsIndexT("fast_threshold", 10),          // threshold | 阈值
                GetOptionAsBool("fast_nonmaxSuppression", true)); // non-max suppression | 非最大值抑制
        }
        else if (detector_type == "AGAST")
        {
            cv::AgastFeatureDetector::DetectorType type = cv::AgastFeatureDetector::OAST_9_16;
            const std::string &agast_type = method_options_["agast_type"];

            if (agast_type == "AGAST_5_8")
            {
                type = cv::AgastFeatureDetector::AGAST_5_8;
            }
            else if (agast_type == "AGAST_7_12d")
            {
                type = cv::AgastFeatureDetector::AGAST_7_12d;
            }
            else if (agast_type == "AGAST_7_12s")
            {
                type = cv::AgastFeatureDetector::AGAST_7_12s;
            }

            return cv::AgastFeatureDetector::create(
                GetOptionAsIndexT("agast_threshold", 10),         // threshold | 阈值
                GetOptionAsBool("agast_nonmaxSuppression", true), // non-max suppression | 非最大值抑制
                type);
        }
        else if (detector_type == "SIFT")
        {
            return cv::SIFT::create(
                GetOptionAsIndexT("nfeatures", 0),             // number of features | 特征数量
                GetOptionAsIndexT("nOctaveLayers", 3),         // number of octave layers | 八度层数
                GetOptionAsFloat("contrastThreshold", 0.015f), // contrast threshold | 对比度阈值
                GetOptionAsFloat("edgeThreshold", 10.0f),      // edge threshold | 边缘阈值
                GetOptionAsFloat("sigma", 1.6f));              // sigma | 标准差
        }
        else if (detector_type == "ORB")
        {
            return cv::ORB::create(
                GetOptionAsIndexT("orb_nfeatures", 1000),    // number of features | 特征数量
                GetOptionAsFloat("orb_scaleFactor", 1.2f),   // scale factor | 缩放因子
                GetOptionAsIndexT("orb_nlevels", 8),         // number of levels | 层数
                GetOptionAsIndexT("orb_edgeThreshold", 31),  // edge threshold | 边缘阈值
                GetOptionAsIndexT("orb_firstLevel", 0),      // first level | 第一层
                GetOptionAsIndexT("orb_WTA_K", 2),           // WTA_K parameter | WTA_K参数
                cv::ORB::HARRIS_SCORE,                       // scoreType | 评分类型
                GetOptionAsIndexT("orb_patchSize", 31),      // patch size | 补丁大小
                GetOptionAsIndexT("orb_fastThreshold", 20)); // FAST threshold | FAST阈值
        }
        else if (detector_type == "AKAZE")
        {
            return cv::AKAZE::create(
                cv::AKAZE::DESCRIPTOR_MLDB,                  // descriptor_type | 描述子类型
                0,                                           // descriptor_size | 描述子大小
                3,                                           // descriptor_channels | 描述子通道数
                GetOptionAsFloat("akaze_threshold", 0.001f), // threshold | 阈值
                GetOptionAsIndexT("akaze_nOctaves", 4),      // number of octaves | 八度数量
                GetOptionAsIndexT("akaze_nOctaveLayers", 4), // number of octave layers | 八度层数
                cv::KAZE::DIFF_PM_G2                         // diffusivity | 扩散性
            );
        }
        else if (detector_type == "BRISK")
        {
            return cv::BRISK::create(
                GetOptionAsIndexT("brisk_thresh", 30),         // threshold | 阈值
                GetOptionAsIndexT("brisk_octaves", 3),         // number of octaves | 八度数量
                GetOptionAsFloat("brisk_patternScale", 1.0f)); // pattern scale | 模式缩放
        }
        else if (detector_type == "SUPERPOINT")
        {
            // SuperPoint uses Python script for feature extraction | SuperPoint使用Python脚本进行特征提取
            // Return nullptr here, handled by dedicated SuperPoint strategy | 这里返回nullptr，由专门的SuperPoint策略处理
            return nullptr;
        }

        // If no matching detector type, default to SIFT | 如果没有匹配的检测器类型，默认使用SIFT
        LOG_WARNING_ZH << "[Img2FeaturesPipeline] 警告: 未知检测器类型 '" << detector_type << "', 使用SIFT作为默认";
        LOG_WARNING_EN << "[Img2FeaturesPipeline] Warning: Unknown detector type '" << detector_type << "', using SIFT as default";

        return cv::SIFT::create(
            GetOptionAsIndexT("nfeatures", 0),             // number of features | 特征数量
            GetOptionAsIndexT("nOctaveLayers", 3),         // number of octave layers | 八度层数
            GetOptionAsFloat("contrastThreshold", 0.015f), // contrast threshold | 对比度阈值
            GetOptionAsFloat("edgeThreshold", 10.0f),      // edge threshold | 边缘阈值
            GetOptionAsFloat("sigma", 1.6f));              // sigma | 标准差
    }

    void Img2FeaturesPipeline::DetectFeatures(cv::Mat &image,
                                              std::vector<cv::KeyPoint> &keypoints,
                                              cv::Mat &descriptors)
    {
        try
        {
            auto detector = CreateDetector();
            const std::string &detector_type = method_options_["detector_type"];

            // SuperPoint doesn't need OpenCV detector, other types do | SuperPoint不需要OpenCV detector，其他类型需要
            if (!detector && detector_type != "SUPERPOINT")
            {
                throw std::runtime_error("Failed to create detector");
            }

            // Clear output containers | 清空输出容器
            keypoints.clear();
            descriptors.release();

            // Ensure image is grayscale | 确保图像是灰度图
            cv::Mat working_image;
            if (image.type() != CV_8UC1)
            {
                cv::cvtColor(image, working_image, cv::COLOR_BGR2GRAY);
            }
            else
            {
                working_image = image.clone();
            }

            // Get and use corresponding detection strategy | 获取并使用对应的检测策略
            auto strategy = GetDetectorStrategy(detector_type);
            strategy->Process(working_image, keypoints, descriptors, detector);

            // Result check and debug information | 结果检查和调试信息
            if (keypoints.empty())
            {
                LOG_WARNING_ZH << "警告: 未检测到关键点，检测器类型: " << detector_type;
                LOG_WARNING_EN << "Warning: No keypoints detected for detector type: " << detector_type;
            }
            if (descriptors.empty())
            {
                LOG_WARNING_ZH << "警告: 未计算描述子，检测器类型: " << detector_type;
                LOG_WARNING_EN << "Warning: No descriptors computed for detector type: " << detector_type;
            }
            else
            {
                LOG_DEBUG_ZH << "DetectFeatures - " << detector_type << ": " << keypoints.size() << " 个关键点, " << descriptors.rows << "x" << descriptors.cols << " 描述子, 类型=" << descriptors.type() << " (CV_32F=" << CV_32F << ")";
                LOG_DEBUG_EN << "DetectFeatures - " << detector_type << ": " << keypoints.size() << " keypoints, " << descriptors.rows << "x" << descriptors.cols << " descriptors, type=" << descriptors.type() << " (CV_32F=" << CV_32F << ")";
            }
        }
        catch (const cv::Exception &e)
        {
            LOG_ERROR_ZH << "DetectFeatures中OpenCV错误: " << e.what();
            LOG_ERROR_EN << "OpenCV error in DetectFeatures: " << e.what();
            keypoints.clear();
            descriptors.release();
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "DetectFeatures中错误: " << e.what();
            LOG_ERROR_EN << "Error in DetectFeatures: " << e.what();
            keypoints.clear();
            descriptors.release();
        }
    }

    DataPtr Img2FeaturesPipeline::RunFast()
    {
        // Get input image paths | 获取输入图像路径
        DataPtr data_images_ptr = required_package_["data_images"];
        if (!data_images_ptr)
        {
            LOG_ERROR_ZH << "没有输入图像数据!";
            LOG_ERROR_EN << "No input images data!";
            return nullptr;
        }
        // Get image path list | 获取图像路径列表
        ImagePathsPtr image_paths_ptr = GetDataPtr<ImagePaths>(data_images_ptr);
        if (!image_paths_ptr || image_paths_ptr->empty())
        {
            LOG_ERROR_ZH << "空图像路径!";
            LOG_ERROR_EN << "Empty image paths!";
            return nullptr;
        }

        // Create feature data output object | 创建特征数据输出对象
        DataPtr output_dataptr = FactoryData::Create("data_features");
        if (!output_dataptr)
        {
            LOG_ERROR_ZH << "未能创建输出数据容器!";
            LOG_ERROR_EN << "Failed to create output data container!";
            return nullptr;
        }

        // Get feature data pointer | 获取特征数据指针
        FeaturesInfoPtr features_info_ptr = GetDataPtr<FeaturesInfo>(output_dataptr);

        // First collect all valid images and sort by filename | 首先收集所有有效图像并按文件名排序
        std::vector<std::pair<std::string, std::string>> valid_image_pairs; // (filename_number, img_path)
        for (const auto &[img_path, is_valid] : *image_paths_ptr)
        {
            if (!is_valid)
            {
                LOG_WARNING_ZH << "跳过无效图像: " << img_path;
                LOG_WARNING_EN << "Skipping invalid image: " << img_path;
                continue;
            }

            // Extract number from image filename for sorting | 从图像文件名提取数字用于排序
            std::string filename = std::filesystem::path(img_path).stem().string();
            std::regex number_regex("^(\\d+)");
            std::smatch match;
            if (std::regex_search(filename, match, number_regex))
            {
                std::string filename_number = match[1].str();
                // Pad with zeros to ensure correct sorting (e.g.: 0001, 0003, 0005) | 补零确保正确排序（例如：0001, 0003, 0005）
                std::string padded_number = std::string(8 - filename_number.length(), '0') + filename_number;
                valid_image_pairs.emplace_back(padded_number, img_path);
            }
            else
            {
                LOG_WARNING_ZH << "无法从文件名提取数字: " << filename;
                LOG_WARNING_EN << "Cannot extract number from filename: " << filename;
                continue;
            }
        }

        // Sort by filename number | 按文件名数字排序
        std::sort(valid_image_pairs.begin(), valid_image_pairs.end());

        // Create continuous view_id mapping (0, 1, 2, 3...) | 创建连续的view_id映射（0, 1, 2, 3...）
        features_info_ptr->clear();
        features_info_ptr->resize(valid_image_pairs.size());

        // Process each image using continuous view_id | 处理每张图像，使用连续的view_id
        for (IndexT view_id = 0; view_id < valid_image_pairs.size(); ++view_id)
        {
            const std::string &img_path = valid_image_pairs[view_id].second;

            // Read image | 读取图像
            cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
            if (img.empty())
            {
                LOG_ERROR_ZH << "加载图像失败: " << img_path;
                LOG_ERROR_EN << "Failed to load image: " << img_path;
                continue;
            }

            // Read color image for extracting RGB values at feature points | 读取彩色图像以提取特征点处的RGB值
            cv::Mat img_color = cv::imread(img_path, cv::IMREAD_COLOR);
            bool has_color_image = !img_color.empty();

            // Use shared feature detection function | 使用共用的特征检测函数
            std::vector<cv::KeyPoint> keypoints;
            cv::Mat descriptors;
            DetectFeatures(img, keypoints, descriptors);

            // Create image feature information | 创建图像特征信息
            ImageFeatureInfo image_feature;
            image_feature.SetImagePath(img_path);
            image_feature.ReserveFeatures(keypoints.size());

            // Prepare color data if color image is available | 如果有彩色图像，准备颜色数据
            std::vector<std::array<uint8_t, 3>> colors;
            if (has_color_image)
            {
                colors.reserve(keypoints.size());
            }

            // Convert feature points | 转换特征点
            for (const auto &kp : keypoints)
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

            // Output processing information | 输出处理信息
            LOG_DEBUG_ZH << "处理图像 " << img_path << " -> view_id " << view_id << " 提取 " << keypoints.size() << " 个特征";
            LOG_DEBUG_EN << "Processed image " << img_path << " -> view_id " << view_id << " with " << keypoints.size() << " features";
        }

        // Add DataFeatures output (if export_features is true) | 增加DataFeatures的输出（如果export_features为true）
        if (method_options_["export_features"] == "ON")
        {
            // Save output_dataptr | 保存output_dataptr
            std::filesystem::path export_path = method_options_["export_fea_path"];
            export_path /= "features_" + method_options_["detector_type"] + ".pb";
            output_dataptr->Save(export_path.string());
        }

        return output_dataptr;
    }

    DataPtr Img2FeaturesPipeline::RunWithImageViewer()
    {
        // Get input image paths | 获取输入图像路径
        DataPtr data_images_ptr = required_package_["data_images"];
        if (!data_images_ptr)
        {
            LOG_ERROR_ZH << "没有输入图像数据!";
            LOG_ERROR_EN << "No input images data!";
            return nullptr;
        }

        // Get image path list | 获取图像路径列表
        ImagePathsPtr image_paths_ptr = GetDataPtr<ImagePaths>(data_images_ptr);
        if (!image_paths_ptr || image_paths_ptr->empty())
        {
            LOG_ERROR_ZH << "没有可用图像!";
            LOG_ERROR_EN << "No images available!";
            return nullptr;
        }

        // Create feature data output object | 创建特征数据输出对象
        auto output_dataptr = FactoryData::Create("data_features");
        if (!output_dataptr)
        {
            LOG_ERROR_ZH << "未能创建输出数据容器!";
            LOG_ERROR_EN << "Failed to create output data container!";
            return nullptr;
        }

        // Get feature data pointer | 获取特征数据指针
        FeaturesInfoPtr features_info_ptr = GetDataPtr<FeaturesInfo>(output_dataptr);

        // Select and load first image | 选择并加载第一张图片
        if (image_paths_ptr->empty())
        {
            LOG_ERROR_ZH << "没有可用图像!";
            LOG_ERROR_EN << "No images available!";
            return nullptr;
        }
        const auto &[image_path, image_id] = image_paths_ptr->front();
        cv::Mat current_image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (current_image.empty())
        {
            LOG_ERROR_ZH << "加载图像失败: " << image_path;
            LOG_ERROR_EN << "Failed to load image: " << image_path;
            return nullptr;
        }

        // Image viewer configuration | 图像查看器配置
        auto &viewer = common::ImageViewer::Instance();
        common::ImageViewer::DisplayOptions options;
        // Keypoint display color (green) | 关键点显示颜色（绿色）
        options.keypoint_color = cv::Scalar(0, 255, 0);
        // Keypoint display size | 关键点显示大小
        options.keypoint_size = 2;
        // Show orientation | 显示方向
        options.show_orientation = true;
        // Show scale | 显示尺度
        options.show_scale = true;
        // Scale factor | 缩放因子
        options.scale_factor = 0.5f;
        // Auto wait | 自动等待
        options.auto_wait = false;
        viewer.SetDisplayOptions(options);

        // Initialize detector and data storage | 初始化检测器和数据存储
        cv::Ptr<cv::SIFT> detector;
        std::vector<cv::KeyPoint> current_keypoints;
        cv::Mat current_descriptors;

        // Feature detection and display update function | 特征检测和显示更新函数
        struct CallbackData
        {
            cv::Mat *image;
            std::vector<cv::KeyPoint> *keypoints;
            cv::Mat *descriptors;
            Img2FeaturesPipeline *plugin;
            std::function<void(CallbackData *)> update_func;
        };

        auto updateFeatures = [](CallbackData *data)
        {
            try
            {
                // Use shared feature detection function | 使用共用的特征检测函数
                data->plugin->DetectFeatures(*(data->image),
                                             *(data->keypoints),
                                             *(data->descriptors));

                // Update display | 更新显示
                std::string window_name = "Image Features";
                common::ImageViewer::ShowImage(*(data->image), *(data->keypoints), window_name);

                // Create detector parameter control bars in main window | 在主窗口中创建检测器参数控制条
                static bool first_time = true;
                if (first_time)
                {
                    const std::string &detector_type = data->plugin->method_options_["detector_type"];

                    // Ensure lifetime of all callback data | 确保所有回调数据的生命周期
                    static CallbackData *static_data = data;

                    if (detector_type == "SIFT")
                    {
                        // SIFT parameter control bars | SIFT 参数控制条
                        try
                        {
                            // Use GetOptionAs series functions to get initial values | 使用GetOptionAs系列函数获取初始值
                            int nfeatures = static_data->plugin->GetOptionAsIndexT("nfeatures", 0);
                            int octave_layers = static_data->plugin->GetOptionAsIndexT("nOctaveLayers", 3);
                            // Scale to fit trackbar | 缩放以适应trackbar
                            float contrast = static_data->plugin->GetOptionAsFloat("contrastThreshold", 0.015f) * 100;
                            float edge = static_data->plugin->GetOptionAsFloat("edgeThreshold", 10.0f);
                            // Scale to fit trackbar | 缩放以适应trackbar
                            float sigma = static_data->plugin->GetOptionAsFloat("sigma", 1.6f) * 10;

                            // SIFT parameter control bars | SIFT参数控制条
                            cv::createTrackbar("SIFT Features", window_name, NULL, 5000, [](int pos, void *userdata)
                                               {
                                auto* data = static_cast<CallbackData*>(userdata);
                                data->plugin->method_options_["nfeatures"] = std::to_string(pos);
                                data->update_func(data); }, static_data);

                            cv::createTrackbar("SIFT Contrast", window_name, NULL, 100, [](int pos, void *userdata)
                                               {
                                auto* data = static_cast<CallbackData*>(userdata);
                                data->plugin->method_options_["contrastThreshold"] = 
                                    std::to_string(pos/100.0f);
                                data->update_func(data); }, static_data);

                            // Set initial values | 设置初始值
                            cv::setTrackbarPos("SIFT Features", window_name, nfeatures);
                            cv::setTrackbarPos("SIFT Contrast", window_name, static_cast<int>(contrast));
                        }
                        catch (const std::exception &e)
                        {
                            LOG_ERROR_ZH << "设置SIFT控制失败: " << e.what();
                            LOG_ERROR_EN << "Error setting up SIFT controls: " << e.what();
                        }
                    }

                    first_time = false;
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "updateFeatures中错误: " << e.what();
                LOG_ERROR_EN << "Error in updateFeatures: " << e.what();
            }
        };

        // Create callback data | 创建回调数据
        CallbackData cb_data{
            &current_image,
            &current_keypoints,
            &current_descriptors,
            this,
            updateFeatures};

        // Initial detection | 初始检测
        updateFeatures(&cb_data);

        // Main loop: wait for user interaction | 主循环：等待用户交互
        while (true)
        {
            char key = cv::waitKey(100);
            if (key == 27)
            {
                // ESC: confirm parameters and exit | ESC: 确认参数并退出
                std::string window_name = "Image Features";
                // Save final parameters | 保存最终参数
                method_options_["nfeatures"] = std::to_string(cv::getTrackbarPos("SIFT Features", window_name));
                method_options_["contrastThreshold"] = std::to_string(cv::getTrackbarPos("SIFT Contrast", window_name) / 100.0f);
                break;
            }
        }

        // Clean up display resources | 清理显示资源
        cv::destroyAllWindows();

        // Process all images using final parameters | 使用最终参数处理所有图像
        OpenCVConverter::CVFeatures2FeaturesInfo(
            current_keypoints, features_info_ptr,
            image_paths_ptr->front().first);

        return output_dataptr;
    }

    void Img2FeaturesPipeline::SuperPointDetectorStrategy::Process(
        const cv::Mat &image,
        std::vector<cv::KeyPoint> &keypoints,
        cv::Mat &descriptors,
        cv::Ptr<cv::Feature2D> detector)
    {
        // SuperPoint feature extraction implemented via Python script | SuperPoint特征提取通过Python脚本实现
        if (!RunSuperPointExtraction(image, keypoints, descriptors))
        {
            LOG_WARNING_ZH << "[SuperPointStrategy] SuperPoint提取失败，降级到SIFT";
            LOG_WARNING_EN << "[SuperPointStrategy] SuperPoint extraction failed, falling back to SIFT";

            auto sift_detector = cv::SIFT::create(
                plugin_->GetOptionAsIndexT("nfeatures", 0),
                plugin_->GetOptionAsIndexT("nOctaveLayers", 3),
                plugin_->GetOptionAsFloat("contrastThreshold", 0.015f),
                plugin_->GetOptionAsFloat("edgeThreshold", 10.0f),
                plugin_->GetOptionAsFloat("sigma", 1.6f));

            sift_detector->detectAndCompute(image, cv::Mat(), keypoints, descriptors);
        }
    }

    bool Img2FeaturesPipeline::SuperPointDetectorStrategy::RunSuperPointExtraction(
        const cv::Mat &image,
        std::vector<cv::KeyPoint> &keypoints,
        cv::Mat &descriptors)
    {
        try
        {
            // 1. Create temporary directory and files | 1. 创建临时目录和文件
            std::string temp_dir = CreateTempDirectory();
            if (temp_dir.empty())
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] 未能创建临时目录";
                LOG_ERROR_EN << "[SuperPointStrategy] Failed to create temp directory";
                return false;
            }

            std::string temp_id = std::to_string(std::rand());
            std::string img_path = temp_dir + "/input_" + temp_id + ".png";
            std::string output_path = temp_dir + "/features_" + temp_id + ".txt";

            // 2. Save image | 2. 保存图像
            if (!cv::imwrite(img_path, image))
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] 保存图像失败";
                LOG_ERROR_EN << "[SuperPointStrategy] Failed to save image";
                return false;
            }

            // 3. Check and configure Python environment | 3. 检查并配置Python环境
            std::string python_exe = CheckAndSetupPythonEnvironment();
            if (python_exe.empty())
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] Python环境设置失败";
                LOG_ERROR_EN << "[SuperPointStrategy] Python environment setup failed";
                return false;
            }

            std::string script_path = FindSuperPointScript();
            if (script_path.empty())
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] 未找到SuperPoint脚本";
                LOG_ERROR_EN << "[SuperPointStrategy] SuperPoint script not found";
                return false;
            }

            std::ostringstream cmd;
            cmd << python_exe << " \"" << script_path << "\""
                << " --image \"" << img_path << "\""
                << " --output \"" << output_path << "\""
                << " --max_keypoints " << plugin_->GetOptionAsIndexT("max_keypoints", 2048)
                << " --detection_threshold " << plugin_->GetOptionAsFloat("detection_threshold", 0.0005f)
                << " --nms_radius " << plugin_->GetOptionAsIndexT("nms_radius", 4)
                << " 2>&1";

            LOG_DEBUG_ZH << "[SuperPointStrategy] 执行: " << cmd.str();
            LOG_DEBUG_EN << "[SuperPointStrategy] Executing: " << cmd.str();

            // 4. Execute Python script | 4. 执行Python脚本
            int result = std::system(cmd.str().c_str());
            if (result != 0)
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] Python脚本执行失败，代码: " << result;
                LOG_ERROR_EN << "[SuperPointStrategy] Python script failed with code: " << result;
                CleanupTempFiles({img_path, output_path});
                return false;
            }

            // 5. Load feature results | 5. 加载特征结果
            if (!LoadSuperPointFeatures(output_path, keypoints, descriptors))
            {
                LOG_ERROR_ZH << "[SuperPointStrategy] 加载SuperPoint特征失败";
                LOG_ERROR_EN << "[SuperPointStrategy] Failed to load SuperPoint features";
                CleanupTempFiles({img_path, output_path});
                return false;
            }

            // 6. Clean up temporary files | 6. 清理临时文件
            CleanupTempFiles({img_path, output_path});

            LOG_INFO_ZH << "[SuperPointStrategy] SuperPoint提取成功，找到 " << keypoints.size() << " 个关键点";
            LOG_INFO_EN << "[SuperPointStrategy] SuperPoint extraction successful, found " << keypoints.size() << " keypoints";
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[SuperPointStrategy] 异常: " << e.what();
            LOG_ERROR_EN << "[SuperPointStrategy] Exception: " << e.what();
            return false;
        }
    }

    std::string Img2FeaturesPipeline::SuperPointDetectorStrategy::FindSuperPointScript()
    {
        // Search for script location based on plugin-config.cmake installation logic | 根据plugin-config.cmake的安装逻辑查找脚本位置
        std::vector<std::string> possible_paths = {
            // 1. Build output directory's methods directory (plugin-config.cmake installation location)
            "plugins/methods/method_img2features_plugin_superpoint.py",
            "output/plugins/methods/method_img2features_plugin_superpoint.py",
            "../output/plugins/methods/method_img2features_plugin_superpoint.py",
            "../../output/plugins/methods/method_img2features_plugin_superpoint.py",

            // 2. Plugin source code directory
            "src/plugins/methods/Img2Features/method_img2features_plugin_superpoint.py",
            "../plugins/methods/Img2Features/method_img2features_plugin_superpoint.py",
            "../../plugins/methods/Img2Features/method_img2features_plugin_superpoint.py",

            // 3. Alternative locations (backward compatibility)
            "src/plugins/methods/method_img2features_plugin_superpoint.py",
            "../plugins/methods/method_img2features_plugin_superpoint.py"};

        for (const std::string &path : possible_paths)
        {
            if (std::filesystem::exists(path))
            {
                LOG_DEBUG_ZH << "[SuperPointStrategy] 在以下位置找到SuperPoint脚本: " << path;
                LOG_DEBUG_EN << "[SuperPointStrategy] Found SuperPoint script at: " << path;
                return std::filesystem::absolute(path).string();
            }
        }

        LOG_ERROR_ZH << "[SuperPointStrategy] 错误: 在标准位置未找到SuperPoint脚本";
        LOG_ERROR_EN << "[SuperPointStrategy] ERROR: SuperPoint script not found in standard locations";
        return "";
    }

    std::string Img2FeaturesPipeline::SuperPointDetectorStrategy::CreateTempDirectory()
    {
        try
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);

            std::string temp_name = "superpoint_temp_" + std::to_string(dis(gen));
            std::filesystem::path temp_path = std::filesystem::temp_directory_path() / temp_name;

            if (std::filesystem::create_directories(temp_path))
            {
                return temp_path.string();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[SuperPointStrategy] 创建临时目录失败: " << e.what();
            LOG_ERROR_EN << "[SuperPointStrategy] Exception creating temp directory: " << e.what();
        }

        return "";
    }

    bool Img2FeaturesPipeline::SuperPointDetectorStrategy::LoadSuperPointFeatures(
        const std::string &features_path,
        std::vector<cv::KeyPoint> &keypoints,
        cv::Mat &descriptors)
    {
        try
        {
            std::ifstream file(features_path);
            if (!file.is_open())
            {
                return false;
            }

            keypoints.clear();
            std::vector<std::vector<float>> desc_list;
            std::string line;

            // 读取第一行：特征点数量
            if (!std::getline(file, line))
            {
                return false;
            }

            int num_features = std::stoi(line);
            if (num_features <= 0)
            {
                return false;
            }

            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                float x, y, size, angle, score;

                if (iss >> x >> y >> size >> angle >> score)
                {
                    // 创建特征点
                    cv::KeyPoint kp(cv::Point2f(x, y), size, angle, score);
                    keypoints.push_back(kp);

                    // 读取描述子（256维）
                    std::vector<float> desc(256);
                    for (int i = 0; i < 256; ++i)
                    {
                        float val;
                        if (iss >> val)
                        {
                            desc[i] = val;
                        }
                        else
                        {
                            desc[i] = 0.0f;
                        }
                    }
                    desc_list.push_back(desc);
                }
            }

            // 转换描述子为OpenCV Mat格式
            if (!desc_list.empty())
            {
                descriptors = cv::Mat(desc_list.size(), 256, CV_32F);
                for (size_t i = 0; i < desc_list.size(); ++i)
                {
                    for (int j = 0; j < 256; ++j)
                    {
                        descriptors.at<float>(i, j) = desc_list[i][j];
                    }
                }
            }

            file.close();
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[SuperPointStrategy] 加载特征失败: " << e.what();
            LOG_ERROR_EN << "[SuperPointStrategy] Exception loading features: " << e.what();
            return false;
        }
    }

    void Img2FeaturesPipeline::SuperPointDetectorStrategy::CleanupTempFiles(
        const std::vector<std::string> &file_paths)
    {
        for (const std::string &path : file_paths)
        {
            try
            {
                if (std::filesystem::exists(path))
                {
                    std::filesystem::remove(path);
                }
            }
            catch (const std::exception &e)
            {
                LOG_WARNING_ZH << "[SuperPointStrategy] 清理文件失败 " << path << ": " << e.what();
                LOG_WARNING_EN << "[SuperPointStrategy] WARNING: Failed to cleanup file " << path << ": " << e.what();
            }
        }
    }

    std::string Img2FeaturesPipeline::SuperPointDetectorStrategy::CheckAndSetupPythonEnvironment()
    {
        // 1. First try user-configured Python interpreter | 1. 首先尝试用户配置的Python解释器
        std::string python_exe = plugin_->GetOptionAsString("python_executable", "python3");

        // 2. Check available Python environments (sorted by priority) | 2. 检查可用的Python环境（按优先级排序）
        std::vector<std::string> env_paths = {
            // Local LightGlue environment path (highest priority, newly created dedicated environment)
            "/Users/caiqi/Documents/PoMVG/src/plugins/methods/Img2Features/conda_env/bin/python",
            "./conda_env/bin/python",
            "../Img2Features/conda_env/bin/python",
            "../../Img2Features/conda_env/bin/python",

            // Drawer environment path (secondary priority, general environment)
            "/Users/caiqi/Documents/PoMVG/po_core/drawer/conda_env/bin/python",
            "../../../po_core/drawer/conda_env/bin/python",
            "../../po_core/drawer/conda_env/bin/python",
            "../po_core/drawer/conda_env/bin/python",

            // System Python (lowest priority)
            "python3",
            "python"};

        for (const std::string &env_path : env_paths)
        {
            // Check if Python interpreter exists
            if (std::filesystem::exists(env_path) || env_path == "python3" || env_path == "python")
            {
                // Test if necessary packages are included
                std::string test_cmd = "\"" + env_path + "\" -c \"import torch, numpy, cv2; print('OK')\" 2>/dev/null";
                int result = std::system(test_cmd.c_str());

                if (result == 0)
                {
                    LOG_DEBUG_ZH << "[SuperPointStrategy] 找到合适的Python环境: " << env_path;
                    LOG_DEBUG_EN << "[SuperPointStrategy] Found suitable Python environment: " << env_path;
                    return env_path;
                }
                else
                {
                    LOG_DEBUG_ZH << "[SuperPointStrategy] 环境 " << env_path << " 缺少依赖";
                    LOG_DEBUG_EN << "[SuperPointStrategy] Environment " << env_path << " missing dependencies";
                }
            }
            else
            {
                LOG_DEBUG_ZH << "[SuperPointStrategy] Python解释器未找到: " << env_path;
                LOG_DEBUG_EN << "[SuperPointStrategy] Python interpreter not found: " << env_path;
            }
        }

        // 3. If no suitable environment found, try running environment configuration script | 3. 如果没有找到合适的环境，尝试运行环境配置脚本
        LOG_DEBUG_ZH << "[SuperPointStrategy] 未找到合适的Python环境，尝试配置...";
        LOG_DEBUG_EN << "[SuperPointStrategy] No suitable Python environment found, attempting to configure...";

        std::vector<std::string> config_scripts = {
            "./configure_lightglue_env.sh",
            "../Img2Features/configure_lightglue_env.sh",
            "../../../po_core/drawer/configure_drawer_env.sh"};

        for (const std::string &script_path : config_scripts)
        {
            if (std::filesystem::exists(script_path))
            {
                LOG_DEBUG_ZH << "[SuperPointStrategy] 运行环境配置脚本: " << script_path;
                LOG_DEBUG_EN << "[SuperPointStrategy] Running environment config script: " << script_path;

                std::string config_cmd = "bash \"" + script_path + "\" 2>&1";
                int result = std::system(config_cmd.c_str());

                if (result == 0)
                {
                    LOG_DEBUG_ZH << "[SuperPointStrategy] 环境配置成功";
                    LOG_DEBUG_EN << "[SuperPointStrategy] Environment configuration successful";

                    // Re-check environment
                    for (const std::string &env_path : env_paths)
                    {
                        if (std::filesystem::exists(env_path) || env_path == "python3" || env_path == "python")
                        {
                            std::string test_cmd = env_path + " -c \"import torch, numpy, cv2; print('OK')\" 2>/dev/null";
                            if (std::system(test_cmd.c_str()) == 0)
                            {
                                LOG_DEBUG_ZH << "[SuperPointStrategy] 环境已配置，使用: " << env_path;
                                LOG_DEBUG_EN << "[SuperPointStrategy] Environment configured, using: " << env_path;
                                return env_path;
                            }
                        }
                    }
                }
                else
                {
                    LOG_DEBUG_ZH << "[SuperPointStrategy] 环境配置失败，代码: " << result;
                    LOG_DEBUG_EN << "[SuperPointStrategy] Environment configuration failed with code: " << result;
                }

                break; // Only try the first found config script
            }
        }

        // 4. Finally try default Python (possibly user manually installed dependencies) | 4. 最后尝试默认Python（可能用户手动安装了依赖）
        LOG_DEBUG_ZH << "[SuperPointStrategy] 降级到默认Python: " << python_exe;
        LOG_DEBUG_EN << "[SuperPointStrategy] Falling back to default Python: " << python_exe;
        return python_exe;
    }

} // namespace PluginMethods

// Register plugin | 注册插件
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::Img2FeaturesPipeline)
