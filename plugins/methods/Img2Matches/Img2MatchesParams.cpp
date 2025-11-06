/**
 * @file Img2MatchesParams.cpp
 * @brief 图像特征匹配插件参数配置系统实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "Img2MatchesParams.hpp"
#include <algorithm>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <po_core/po_logger.hpp>

namespace PluginMethods
{
    // ==================== Img2MatchesParameters Implementation | Img2MatchesParameters 实现 ====================

    void Img2MatchesParameters::LoadFromConfig(Interface::MethodPreset *config_loader)
    {
        // === Load basic parameters from method_options_ | 从method_options_加载基础参数 ===
        base.profile_commit = config_loader->GetOptionAsString("ProfileCommit", "Image feature matching");
        base.enable_profiling = config_loader->GetOptionAsBool("enable_profiling", false);
        base.enable_evaluator = config_loader->GetOptionAsBool("enable_evaluator", false);
        base.log_level = static_cast<int>(config_loader->GetOptionAsIndexT("log_level", 2));

        // Run mode | 运行模式
        std::string run_mode_str = config_loader->GetOptionAsString("run_mode", "fast");
        base.run_mode = Img2MatchesParameterConverter::StringToRunMode(run_mode_str);

        // Data types mode | 数据类型模式
        std::string data_types_mode_str = config_loader->GetOptionAsString("data_types", "full");
        base.data_types_mode = Img2MatchesParameterConverter::StringToDataTypesMode(data_types_mode_str);

        // Feature detector type | 特征检测器类型
        base.detector_type = config_loader->GetOptionAsString("detector_type", "SIFT");

        // Multi-threading configuration | 多线程配置
        base.num_threads = static_cast<int>(config_loader->GetOptionAsIndexT("num_threads", 4));

        // === Load SIFT parameters from specific_methods_config_ | 从specific_methods_config_加载SIFT参数 ===
        if (base.detector_type == "SIFT")
        {
            // Get SIFT specific method configuration | 获取SIFT特定方法配置
            const auto &sift_config = config_loader->GetSpecificMethodConfig("SIFT");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_sift_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = sift_config.find(key);
                return (it != sift_config.end()) ? it->second : default_val;
            };

            // Preset configuration | 预设配置
            std::string preset_str = get_sift_option("preset", "CUSTOM");
            sift.preset = Img2MatchesParameterConverter::StringToSIFTPreset(preset_str);

            // OpenCV style parameters | OpenCV风格参数
            sift.nfeatures = std::stoi(get_sift_option("nfeatures", "0"));
            sift.nOctaveLayers = std::stoi(get_sift_option("nOctaveLayers", "3"));
            sift.contrastThreshold = std::stod(get_sift_option("contrastThreshold", "0.04"));
            sift.edgeThreshold = std::stod(get_sift_option("edgeThreshold", "10.0"));
            sift.sigma = std::stod(get_sift_option("sigma", "1.6"));
            sift.enable_precise_upscale = (get_sift_option("enable_precise_upscale", "false") == "true");

            // OpenMVG extended parameters | OpenMVG扩展参数
            sift.first_octave = std::stoi(get_sift_option("first_octave", "0"));
            sift.num_octaves = std::stoi(get_sift_option("num_octaves", "6"));
            sift.root_sift = (get_sift_option("root_sift", "true") == "true");

            // Apply preset configuration (if not CUSTOM) | 应用预设配置（如果不是CUSTOM）
            if (sift.preset != SIFTPreset::CUSTOM)
            {
                sift.ApplyPreset();
            }
        }

        // === Load ORB parameters from specific_methods_config_ | 从specific_methods_config_加载ORB参数 ===
        if (base.detector_type == "ORB")
        {
            // Get ORB specific method configuration | 获取ORB特定方法配置
            const auto &orb_config = config_loader->GetSpecificMethodConfig("ORB");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_orb_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = orb_config.find(key);
                return (it != orb_config.end()) ? it->second : default_val;
            };

            // Load ORB parameters | 加载ORB参数
            orb.nfeatures = std::stoi(get_orb_option("nfeatures", "1000"));
            orb.scaleFactor = std::stod(get_orb_option("scaleFactor", "1.2"));
            orb.nlevels = std::stoi(get_orb_option("nlevels", "8"));
            orb.edgeThreshold = std::stoi(get_orb_option("edgeThreshold", "31"));
            orb.firstLevel = std::stoi(get_orb_option("firstLevel", "0"));
            orb.WTA_K = std::stoi(get_orb_option("WTA_K", "2"));
            orb.patchSize = std::stoi(get_orb_option("patchSize", "31"));
            orb.fastThreshold = std::stoi(get_orb_option("fastThreshold", "20"));
        }

        // === Load SURF parameters from specific_methods_config_ | 从specific_methods_config_加载SURF参数 ===
        if (base.detector_type == "SURF")
        {
            // Get SURF specific method configuration | 获取SURF特定方法配置
            const auto &surf_config = config_loader->GetSpecificMethodConfig("SURF");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_surf_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = surf_config.find(key);
                return (it != surf_config.end()) ? it->second : default_val;
            };

            surf.hessianThreshold = std::stod(get_surf_option("hessianThreshold", "100.0"));
            surf.nOctaves = std::stoi(get_surf_option("nOctaves", "4"));
            surf.nOctaveLayers = std::stoi(get_surf_option("nOctaveLayers", "3"));
            surf.extended = (get_surf_option("extended", "false") == "true");
            surf.upright = (get_surf_option("upright", "false") == "true");
        }

        // === Load SuperPoint parameters from specific_methods_config_ | 从specific_methods_config_加载SuperPoint参数 ===
        if (base.detector_type == "SUPERPOINT")
        {
            // Get SuperPoint specific method configuration | 获取SuperPoint特定方法配置
            const auto &superpoint_config = config_loader->GetSpecificMethodConfig("SUPERPOINT");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_superpoint_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = superpoint_config.find(key);
                return (it != superpoint_config.end()) ? it->second : default_val;
            };

            superpoint.max_keypoints = std::stoi(get_superpoint_option("max_keypoints", "2048"));
            superpoint.detection_threshold = std::stod(get_superpoint_option("detection_threshold", "0.0005"));
            superpoint.nms_radius = std::stoi(get_superpoint_option("nms_radius", "4"));
            superpoint.remove_borders = std::stoi(get_superpoint_option("remove_borders", "4"));
            superpoint.python_executable = get_superpoint_option("python_executable", "python3");
        }

        // === Load export and matching parameters from method_options_ | 从method_options_加载导出和匹配参数 ===
        feature_export.export_features = config_loader->GetOptionAsBool("export_features", false);
        feature_export.export_fea_path = config_loader->GetOptionAsPath("export_fea_path", "", "storage/features");

        matches_export.export_matches = config_loader->GetOptionAsBool("export_matches", false);
        matches_export.export_match_path = config_loader->GetOptionAsPath("export_match_path", "", "storage/matches");

        // Matcher parameters | 匹配器参数
        std::string matcher_type_str = config_loader->GetOptionAsString("matcher_type", "FASTCASCADEHASHINGL2");
        matching.matcher_type = Img2MatchesParameterConverter::StringToMatcherType(matcher_type_str);
        matching.cross_check = config_loader->GetOptionAsBool("cross_check", false);
        matching.ratio_thresh = static_cast<float>(config_loader->GetOptionAsDouble("ratio_thresh", 0.8));
        matching.max_matches = config_loader->GetOptionAsIndexT("max_matches", 0);

        // === Load FLANN parameters from specific_methods_config_ | 从specific_methods_config_加载FLANN参数 ===
        if (matching.matcher_type == MatcherType::FLANN)
        {
            // Get FLANN specific method configuration | 获取FLANN特定方法配置
            const auto &flann_config = config_loader->GetSpecificMethodConfig("FLANN");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_flann_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = flann_config.find(key);
                return (it != flann_config.end()) ? it->second : default_val;
            };

            // Control mode switch | 控制方式开关
            flann.use_advanced_control = (get_flann_option("use_advanced_control", "true") == "true");

            // Algorithm type | 算法类型
            std::string algorithm_str = get_flann_option("algorithm", "AUTO");
            flann.algorithm = Img2MatchesParameterConverter::StringToFLANNAlgorithm(algorithm_str);

            // KDTree algorithm parameters | KDTree算法参数
            flann.trees = std::stoi(get_flann_option("trees", "8"));

            // LSH algorithm parameters | LSH算法参数
            flann.table_number = std::stoi(get_flann_option("table_number", "12"));
            flann.key_size = std::stoi(get_flann_option("key_size", "20"));
            flann.multi_probe_level = std::stoi(get_flann_option("multi_probe_level", "2"));

            // KMeans algorithm parameters | KMeans算法参数
            flann.branching = std::stoi(get_flann_option("branching", "32"));
            flann.iterations = std::stoi(get_flann_option("iterations", "11"));
            std::string centers_init_str = get_flann_option("centers_init", "CENTERS_RANDOM");
            flann.centers_init = Img2MatchesParameterConverter::StringToFLANNCentersInit(centers_init_str);

            // Search parameters | 搜索参数
            flann.checks = std::stoi(get_flann_option("checks", "100"));
            flann.eps = std::stod(get_flann_option("eps", "0.0"));
            flann.sorted = (get_flann_option("sorted", "true") == "true");
            flann.max_neighbors = std::stoi(get_flann_option("max_neighbors", "-1"));

            // Preset configuration | 预设配置
            std::string preset_str = get_flann_option("preset", "BALANCED");
            flann.preset = Img2MatchesParameterConverter::StringToFLANNPreset(preset_str);

            // Apply preset configuration (if not CUSTOM) | 应用预设配置（如果不是CUSTOM）
            if (flann.preset != FLANNPreset::CUSTOM)
            {
                flann.ApplyPreset();
            }

            // Automatically select algorithm based on descriptor type | 根据描述子类型自动选择算法
            flann.AutoSelectAlgorithm(base.detector_type);
        }

        // === Load LightGlue parameters from specific_methods_config_ | 从specific_methods_config_加载LightGlue参数 ===
        if (matching.matcher_type == MatcherType::LIGHTGLUE)
        {
            // Get LightGlue specific method configuration | 获取LightGlue特定方法配置
            const auto &lightglue_config = config_loader->GetSpecificMethodConfig("LIGHTGLUE");

            // Helper function: Get parameter from specific method configuration | 辅助函数：从特定方法配置中获取参数
            auto get_lightglue_option = [&](const std::string &key, const std::string &default_val) -> std::string
            {
                auto it = lightglue_config.find(key);
                return (it != lightglue_config.end()) ? it->second : default_val;
            };

            // Basic configuration | 基础配置
            std::string feature_type_str = get_lightglue_option("feature_type", "SUPERPOINT");
            lightglue.feature_type = Img2MatchesParameterConverter::StringToLightGlueFeatureType(feature_type_str);
            lightglue.max_num_keypoints = std::stoi(get_lightglue_option("max_num_keypoints", "2048"));
            lightglue.depth_confidence = std::stof(get_lightglue_option("depth_confidence", "0.95"));
            lightglue.width_confidence = std::stof(get_lightglue_option("width_confidence", "0.99"));
            lightglue.filter_threshold = std::stof(get_lightglue_option("filter_threshold", "0.1"));

            // Performance optimization | 性能优化
            lightglue.flash_attention = (get_lightglue_option("flash_attention", "true") == "true");
            lightglue.mixed_precision = (get_lightglue_option("mixed_precision", "false") == "true");
            lightglue.compile_model = (get_lightglue_option("compile_model", "false") == "true");

            // Environment configuration | 环境配置
            lightglue.python_executable = get_lightglue_option("python_executable", "python3");
            lightglue.script_path = get_lightglue_option("script_path", "");
        }

        // Visualization parameters | 可视化参数
        visualization.show_view_pair_i = config_loader->GetOptionAsIndexT("show_view_pair_i", 0);
        visualization.show_view_pair_j = config_loader->GetOptionAsIndexT("show_view_pair_j", 1);
    }

    bool Img2MatchesParameters::Validate(Interface::MethodPreset *method_ptr) const
    {
        // Validate base parameters | 验证基础参数
        if (base.num_threads < 1 || base.num_threads > 64)
        {
            if (method_ptr)
            {
                LOG_ERROR_ZH << "[PoSDK | method_img2matches] 错误 >>> num_threads必须在[1,64]范围内，当前值: " << base.num_threads;
                LOG_ERROR_EN << "[PoSDK | method_img2matches] ERROR >>> num_threads must be in range [1,64], current value: " << base.num_threads;
            }
            else
            {
                LOG_ERROR_ZH << "[Img2Matches] 错误 >>> num_threads必须在[1,64]范围内，当前值: " << base.num_threads;
                LOG_ERROR_EN << "[Img2Matches] ERROR >>> num_threads must be in range [1,64], current value: " << base.num_threads;
            }
            return false;
        }

        // Validate matching parameters | 验证匹配参数
        if (matching.ratio_thresh <= 0.0f || matching.ratio_thresh >= 1.0f)
        {
            if (method_ptr)
            {
                LOG_ERROR_ZH << "[PoSDK | method_img2matches] 错误 >>> ratio_thresh必须在(0,1)范围内，当前值: " << matching.ratio_thresh;
                LOG_ERROR_EN << "[PoSDK | method_img2matches] ERROR >>> ratio_thresh must be in range (0,1), current value: " << matching.ratio_thresh;
            }
            else
            {
                LOG_ERROR_ZH << "[Img2Matches] 错误 >>> ratio_thresh必须在(0,1)范围内，当前值: " << matching.ratio_thresh;
                LOG_ERROR_EN << "[Img2Matches] ERROR >>> ratio_thresh must be in range (0,1), current value: " << matching.ratio_thresh;
            }
            std::cerr << std::endl;
            return false;
        }

        // Validate visualization parameters | 验证可视化参数
        if (visualization.show_view_pair_i == visualization.show_view_pair_j)
        {
            if (method_ptr)
            {
                LOG_ERROR_ZH << "[PoSDK | method_img2matches] 错误 >>> show_view_pair_i和show_view_pair_j不能相同";
                LOG_ERROR_EN << "[PoSDK | method_img2matches] ERROR >>> show_view_pair_i and show_view_pair_j cannot be the same";
            }
            else
            {
                LOG_ERROR_ZH << "[Img2Matches] 错误 >>> show_view_pair_i和show_view_pair_j不能相同";
                LOG_ERROR_EN << "[Img2Matches] ERROR >>> show_view_pair_i and show_view_pair_j cannot be the same";
            }
            std::cerr << std::endl;
            return false;
        }

        return true;
    }

    // Print summary of Img2Matches parameters | 打印Img2Matches参数摘要
    void Img2MatchesParameters::PrintSummary(Interface::MethodPreset *method_ptr) const
    {
        LOG_DEBUG_ZH << "\n=== Img2Matches Plugin 参数摘要 ===\n";
        LOG_DEBUG_ZH << "基础配置:\n";
        LOG_DEBUG_ZH << "  profile_commit: " << base.profile_commit << "\n";
        LOG_DEBUG_ZH << "  enable_profiling: " << (base.enable_profiling ? "true" : "false") << "\n";
        LOG_DEBUG_ZH << "  enable_evaluator: " << (base.enable_evaluator ? "true" : "false") << "\n";
        LOG_DEBUG_ZH << "  log_level: " << base.log_level << "\n";
        LOG_DEBUG_ZH << "  run_mode: " << Img2MatchesParameterConverter::RunModeToString(base.run_mode) << "\n";
        LOG_DEBUG_ZH << "  data_types: " << Img2MatchesParameterConverter::DataTypesModeToString(base.data_types_mode) << " (数据类型模式)\n";
        LOG_DEBUG_ZH << "  detector_type: " << base.detector_type << "\n";
        LOG_DEBUG_ZH << "  num_threads: " << base.num_threads << " (多线程特征提取)\n";
        LOG_DEBUG_EN << "\n=== Img2Matches Plugin Parameter Summary ===\n";
        LOG_DEBUG_EN << "Basic Configuration:\n";
        LOG_DEBUG_EN << "  profile_commit: " << base.profile_commit << "\n";
        LOG_DEBUG_EN << "  enable_profiling: " << (base.enable_profiling ? "true" : "false") << "\n";
        LOG_DEBUG_EN << "  enable_evaluator: " << (base.enable_evaluator ? "true" : "false") << "\n";
        LOG_DEBUG_EN << "  log_level: " << base.log_level << "\n";
        LOG_DEBUG_EN << "  run_mode: " << Img2MatchesParameterConverter::RunModeToString(base.run_mode) << "\n";
        LOG_DEBUG_EN << "  data_types: " << Img2MatchesParameterConverter::DataTypesModeToString(base.data_types_mode) << " (data types mode)\n";
        LOG_DEBUG_EN << "  detector_type: " << base.detector_type << "\n";
        LOG_DEBUG_EN << "  num_threads: " << base.num_threads << " (multi-threaded feature extraction)\n";

        // Output SIFT detector parameters (only when using SIFT) | 输出SIFT特征检测器参数（仅当使用SIFT时）
        if (base.detector_type == "SIFT")
        {
            LOG_DEBUG_ZH << "SIFT特征检测器配置:\n";
            LOG_DEBUG_ZH << "  preset: " << Img2MatchesParameterConverter::SIFTPresetToString(sift.preset) << "\n";
            LOG_DEBUG_ZH << "  nfeatures: " << sift.nfeatures << " (0=不限制)\n";
            LOG_DEBUG_ZH << "  nOctaveLayers: " << sift.nOctaveLayers << "\n";
            LOG_DEBUG_ZH << "  contrastThreshold: " << sift.contrastThreshold << "\n";
            LOG_DEBUG_ZH << "  edgeThreshold: " << sift.edgeThreshold << "\n";
            LOG_DEBUG_ZH << "  sigma: " << sift.sigma << "\n";
            LOG_DEBUG_ZH << "  enable_precise_upscale: " << (sift.enable_precise_upscale ? "true" : "false") << "\n";
            LOG_DEBUG_ZH << "  first_octave: " << sift.first_octave << " (-1=上采样, 0=原图, 1=下采样)\n";
            LOG_DEBUG_ZH << "  num_octaves: " << sift.num_octaves << "\n";
            LOG_DEBUG_ZH << "  root_sift: " << (sift.root_sift ? "true" : "false") << "\n";
            LOG_DEBUG_EN << "SIFT Detector Configuration:\n";
            LOG_DEBUG_EN << "  preset: " << Img2MatchesParameterConverter::SIFTPresetToString(sift.preset) << "\n";
            LOG_DEBUG_EN << "  nfeatures: " << sift.nfeatures << " (0=no limit)\n";
            LOG_DEBUG_EN << "  nOctaveLayers: " << sift.nOctaveLayers << "\n";
            LOG_DEBUG_EN << "  contrastThreshold: " << sift.contrastThreshold << "\n";
            LOG_DEBUG_EN << "  edgeThreshold: " << sift.edgeThreshold << "\n";
            LOG_DEBUG_EN << "  sigma: " << sift.sigma << "\n";
            LOG_DEBUG_EN << "  enable_precise_upscale: " << (sift.enable_precise_upscale ? "true" : "false") << "\n";
            LOG_DEBUG_EN << "  first_octave: " << sift.first_octave << " (-1=upsample, 0=original, 1=downsample)\n";
            LOG_DEBUG_EN << "  num_octaves: " << sift.num_octaves << "\n";
            LOG_DEBUG_EN << "  root_sift: " << (sift.root_sift ? "true" : "false") << "\n";
        }

        // Output ORB detector parameters (only when using ORB) | 输出ORB特征检测器参数（仅当使用ORB时）
        if (base.detector_type == "ORB")
        {
            LOG_DEBUG_ZH << "ORB特征检测器配置:\n";
            LOG_DEBUG_ZH << "  nfeatures: " << orb.nfeatures << " (0=不限制)\n";
            LOG_DEBUG_ZH << "  scaleFactor: " << orb.scaleFactor << " (金字塔比例因子)\n";
            LOG_DEBUG_ZH << "  nlevels: " << orb.nlevels << " (金字塔层数)\n";
            LOG_DEBUG_ZH << "  edgeThreshold: " << orb.edgeThreshold << " (边缘阈值)\n";
            LOG_DEBUG_ZH << "  firstLevel: " << orb.firstLevel << " (第一层级)\n";
            LOG_DEBUG_ZH << "  WTA_K: " << orb.WTA_K << " (BRIEF描述子点对数)\n";
            LOG_DEBUG_ZH << "  patchSize: " << orb.patchSize << " (特征点周围区域大小)\n";
            LOG_DEBUG_ZH << "  fastThreshold: " << orb.fastThreshold << " (FAST检测器阈值)\n";
            LOG_DEBUG_ZH << "  scoreType: HARRIS_SCORE (固定)\n";
            LOG_DEBUG_ZH << "  描述子维度: 32 (ORB固定32维二进制)\n";
            LOG_DEBUG_EN << "ORB Detector Configuration:\n";
            LOG_DEBUG_EN << "  nfeatures: " << orb.nfeatures << " (0=no limit)\n";
            LOG_DEBUG_EN << "  scaleFactor: " << orb.scaleFactor << " (pyramid scale factor)\n";
            LOG_DEBUG_EN << "  nlevels: " << orb.nlevels << " (pyramid levels)\n";
            LOG_DEBUG_EN << "  edgeThreshold: " << orb.edgeThreshold << " (edge threshold)\n";
            LOG_DEBUG_EN << "  firstLevel: " << orb.firstLevel << " (first level)\n";
            LOG_DEBUG_EN << "  WTA_K: " << orb.WTA_K << " (number of points for BRIEF descriptor)\n";
            LOG_DEBUG_EN << "  patchSize: " << orb.patchSize << " (size of area around feature point)\n";
            LOG_DEBUG_EN << "  fastThreshold: " << orb.fastThreshold << " (FAST detector threshold)\n";
            LOG_DEBUG_EN << "  scoreType: HARRIS_SCORE (fixed)\n";
            LOG_DEBUG_EN << "  descriptor dimension: 32 (ORB fixed 32-bit binary)\n";
        }

        // Output SURF detector parameters (only when using SURF) | 输出SURF特征检测器参数（仅当使用SURF时）
        if (base.detector_type == "SURF")
        {
            LOG_DEBUG_ZH << "SURF特征检测器配置:\n";
            LOG_DEBUG_ZH << "  hessianThreshold: " << surf.hessianThreshold << " (越大特征点越少)\n";
            LOG_DEBUG_ZH << "  nOctaves: " << surf.nOctaves << "\n";
            LOG_DEBUG_ZH << "  nOctaveLayers: " << surf.nOctaveLayers << "\n";
            LOG_DEBUG_ZH << "  extended: " << (surf.extended ? "true (128维)" : "false (64维)") << "\n";
            LOG_DEBUG_ZH << "  upright: " << (surf.upright ? "true (无旋转不变性, 更快)" : "false (旋转不变性)") << "\n";

            LOG_DEBUG_ZH << "  描述子维度: " << surf.GetDescriptorSize() << "\n";
            LOG_DEBUG_ZH << "  旋转不变性: " << (surf.IsRotationInvariant() ? "支持" : "不支持") << "\n";
            LOG_DEBUG_EN << "SURF Detector Configuration:\n";
            LOG_DEBUG_EN << "  hessianThreshold: " << surf.hessianThreshold << " (higher value, fewer keypoints)\n";
            LOG_DEBUG_EN << "  nOctaves: " << surf.nOctaves << "\n";
            LOG_DEBUG_EN << "  nOctaveLayers: " << surf.nOctaveLayers << "\n";
            LOG_DEBUG_EN << "  extended: " << (surf.extended ? "true (128 dims)" : "false (64 dims)") << "\n";
            LOG_DEBUG_EN << "  upright: " << (surf.upright ? "true (no rotation invariance, faster)" : "false (rotation invariance)") << "\n";
            LOG_DEBUG_EN << "  descriptor dimension: " << surf.GetDescriptorSize() << "\n";
            LOG_DEBUG_EN << "  rotation invariance: " << (surf.IsRotationInvariant() ? "supported" : "not supported") << "\n";
        }

        // Output SuperPoint detector parameters (only when using SuperPoint) | 输出SuperPoint特征检测器参数（仅当使用SuperPoint时）
        if (base.detector_type == "SUPERPOINT")
        {
            LOG_DEBUG_ZH << "SuperPoint深度学习特征检测器配置:\n";
            LOG_DEBUG_ZH << "  max_keypoints: " << superpoint.max_keypoints << " (最大特征点数量)\n";
            LOG_DEBUG_ZH << "  detection_threshold: " << superpoint.detection_threshold << " (检测阈值)\n";
            LOG_DEBUG_ZH << "  nms_radius: " << superpoint.nms_radius << " (非极大值抑制半径)\n";
            LOG_DEBUG_ZH << "  remove_borders: " << superpoint.remove_borders << " (移除边界像素数)\n";
            LOG_DEBUG_ZH << "  python_executable: " << superpoint.python_executable << "\n";
            LOG_DEBUG_ZH << "  描述子维度: 256 (SuperPoint固定256维)\n";
            LOG_DEBUG_EN << "SuperPoint Deep Learning Detector Configuration:\n";
            LOG_DEBUG_EN << "  max_keypoints: " << superpoint.max_keypoints << " (maximum number of keypoints)\n";
            LOG_DEBUG_EN << "  detection_threshold: " << superpoint.detection_threshold << " (detection threshold)\n";
            LOG_DEBUG_EN << "  nms_radius: " << superpoint.nms_radius << " (non-maximum suppression radius)\n";
            LOG_DEBUG_EN << "  remove_borders: " << superpoint.remove_borders << " (border pixels to remove)\n";
            LOG_DEBUG_EN << "  python_executable: " << superpoint.python_executable << "\n";
            LOG_DEBUG_EN << "  descriptor dimension: 256 (SuperPoint fixed 256 dims)\n";
        }

        LOG_DEBUG_ZH << "匹配配置:\n";
        LOG_DEBUG_ZH << "  matcher_type: " << Img2MatchesParameterConverter::MatcherTypeToString(matching.matcher_type) << "\n";
        LOG_DEBUG_ZH << "  cross_check: " << (matching.cross_check ? "true" : "false") << "\n";
        LOG_DEBUG_ZH << "  ratio_thresh: " << matching.ratio_thresh << "\n";
        LOG_DEBUG_ZH << "  max_matches: " << matching.max_matches << "\n";
        LOG_DEBUG_EN << "Matching Configuration:\n";
        LOG_DEBUG_EN << "  matcher_type: " << Img2MatchesParameterConverter::MatcherTypeToString(matching.matcher_type) << "\n";
        LOG_DEBUG_EN << "  cross_check: " << (matching.cross_check ? "true" : "false") << "\n";
        LOG_DEBUG_EN << "  ratio_thresh: " << matching.ratio_thresh << "\n";
        LOG_DEBUG_EN << "  max_matches: " << matching.max_matches << "\n";

        // Output FLANN matcher parameters (only when using FLANN) | 输出FLANN匹配器参数（仅当使用FLANN时）
        if (matching.matcher_type == MatcherType::FLANN)
        {
            LOG_DEBUG_ZH << "FLANN匹配器配置:\n";
            LOG_DEBUG_ZH << "  control_mode: " << (flann.use_advanced_control ? "Advanced Control (高级控制)" : "OpenCV Default (默认方式)") << "\n";
            LOG_DEBUG_EN << "FLANN Matcher Configuration:\n";
            LOG_DEBUG_EN << "  control_mode: " << (flann.use_advanced_control ? "Advanced Control" : "OpenCV Default") << "\n";
            if (flann.use_advanced_control)
            {
                LOG_DEBUG_ZH << "  preset: " << Img2MatchesParameterConverter::FLANNPresetToString(flann.preset) << "\n";
                LOG_DEBUG_ZH << "  algorithm: " << Img2MatchesParameterConverter::FLANNAlgorithmToString(flann.algorithm) << " (自动根据描述子类型选择)\n";
                LOG_DEBUG_ZH << "  === KDTree参数(SIFT/SURF) ===\n";
                LOG_DEBUG_ZH << "  trees: " << flann.trees << " (KDTree数量，值越大精度越高)\n";
                LOG_DEBUG_ZH << "  === LSH参数(ORB/BRIEF) ===\n";
                LOG_DEBUG_ZH << "  table_number: " << flann.table_number << " (哈希表数量)\n";
                LOG_DEBUG_ZH << "  key_size: " << flann.key_size << " (哈希键长度)\n";
                LOG_DEBUG_ZH << "  multi_probe_level: " << flann.multi_probe_level << " (多探测级别)\n";
                LOG_DEBUG_ZH << "  === 搜索参数 ===\n";
                LOG_DEBUG_ZH << "  checks: " << flann.checks << " (搜索检查次数，值越大精度越高)\n";
                LOG_DEBUG_ZH << "  eps: " << flann.eps << " (搜索精度)\n";
                LOG_DEBUG_ZH << "  sorted: " << (flann.sorted ? "true" : "false") << " (结果排序)\n";
                LOG_DEBUG_EN << "  preset: " << Img2MatchesParameterConverter::FLANNPresetToString(flann.preset) << "\n";
                LOG_DEBUG_EN << "  algorithm: " << Img2MatchesParameterConverter::FLANNAlgorithmToString(flann.algorithm) << " (automatically selected based on descriptor type)\n";
                LOG_DEBUG_EN << "  === KDTree Parameters (SIFT/SURF) ===\n";
                LOG_DEBUG_EN << "  trees: " << flann.trees << " (number of KDTrees, higher value for better precision)\n";
                LOG_DEBUG_EN << "  === LSH Parameters (ORB/BRIEF) ===\n";
                LOG_DEBUG_EN << "  table_number: " << flann.table_number << " (number of hash tables)\n";
                LOG_DEBUG_EN << "  key_size: " << flann.key_size << " (hash key length)\n";
                LOG_DEBUG_EN << "  multi_probe_level: " << flann.multi_probe_level << " (multi-probe level)\n";
                LOG_DEBUG_EN << "  === Search Parameters ===\n";
                LOG_DEBUG_EN << "  checks: " << flann.checks << " (number of search checks, higher value for better precision)\n";
                LOG_DEBUG_EN << "  eps: " << flann.eps << " (search precision)\n";
                LOG_DEBUG_EN << "  sorted: " << (flann.sorted ? "true" : "false") << " (result sorting)\n";
            }
            else
            {
                LOG_DEBUG_ZH << "  使用OpenCV默认FLANN参数 (兼容模式，速度优先)\n";
                LOG_DEBUG_EN << "  Using OpenCV default FLANN parameters (compatibility mode, speed priority)\n";
            }
        }

        // Output LightGlue matcher parameters (only when using LightGlue) | 输出LightGlue匹配器参数（仅当使用LightGlue时）
        if (matching.matcher_type == MatcherType::LIGHTGLUE)
        {
            LOG_DEBUG_ZH << "LightGlue深度学习匹配器配置:\n";
            LOG_DEBUG_ZH << "  feature_type: " << Img2MatchesParameterConverter::LightGlueFeatureTypeToString(lightglue.feature_type) << "\n";
            LOG_DEBUG_ZH << "  max_num_keypoints: " << lightglue.max_num_keypoints << " (最大特征点数量)\n";
            LOG_DEBUG_ZH << "  depth_confidence: " << lightglue.depth_confidence << " (深度置信度)\n";
            LOG_DEBUG_ZH << "  width_confidence: " << lightglue.width_confidence << " (宽度置信度)\n";
            LOG_DEBUG_ZH << "  filter_threshold: " << lightglue.filter_threshold << " (匹配置信度阈值)\n";
            LOG_DEBUG_ZH << "  flash_attention: " << (lightglue.flash_attention ? "true" : "false") << " (FlashAttention优化)\n";
            LOG_DEBUG_ZH << "  mixed_precision: " << (lightglue.mixed_precision ? "true" : "false") << " (混合精度)\n";
            LOG_DEBUG_ZH << "  compile_model: " << (lightglue.compile_model ? "true" : "false") << " (模型编译)\n";
            LOG_DEBUG_ZH << "  python_executable: " << lightglue.python_executable << "\n";
            LOG_DEBUG_ZH << "  script_path: " << (lightglue.script_path.empty() ? "自动检测" : lightglue.script_path) << "\n";
            LOG_DEBUG_EN << "LightGlue Deep Learning Matcher Configuration:\n";
            LOG_DEBUG_EN << "  feature_type: " << Img2MatchesParameterConverter::LightGlueFeatureTypeToString(lightglue.feature_type) << "\n";
            LOG_DEBUG_EN << "  max_num_keypoints: " << lightglue.max_num_keypoints << " (maximum number of keypoints)\n";
            LOG_DEBUG_EN << "  depth_confidence: " << lightglue.depth_confidence << " (depth confidence)\n";
            LOG_DEBUG_EN << "  width_confidence: " << lightglue.width_confidence << " (width confidence)\n";
            LOG_DEBUG_EN << "  filter_threshold: " << lightglue.filter_threshold << " (matching confidence threshold)\n";
            LOG_DEBUG_EN << "  flash_attention: " << (lightglue.flash_attention ? "true" : "false") << " (FlashAttention optimization)\n";
            LOG_DEBUG_EN << "  mixed_precision: " << (lightglue.mixed_precision ? "true" : "false") << " (mixed precision)\n";
            LOG_DEBUG_EN << "  compile_model: " << (lightglue.compile_model ? "true" : "false") << " (model compilation)\n";
            LOG_DEBUG_EN << "  python_executable: " << lightglue.python_executable << "\n";
            LOG_DEBUG_EN << "  script_path: " << (lightglue.script_path.empty() ? "auto-detect" : lightglue.script_path) << "\n";
        }

        LOG_DEBUG_ZH << "导出配置:\n";
        LOG_DEBUG_ZH << "  export_features: " << (feature_export.export_features ? "true" : "false") << "\n";
        LOG_DEBUG_ZH << "  export_fea_path: " << feature_export.export_fea_path << "\n";
        LOG_DEBUG_ZH << "  export_matches: " << (matches_export.export_matches ? "true" : "false") << "\n";
        LOG_DEBUG_ZH << "  export_match_path: " << matches_export.export_match_path << "\n";
        LOG_DEBUG_EN << "Export Configuration:\n";
        LOG_DEBUG_EN << "  export_features: " << (feature_export.export_features ? "true" : "false") << "\n";
        LOG_DEBUG_EN << "  export_fea_path: " << feature_export.export_fea_path << "\n";
        LOG_DEBUG_EN << "  export_matches: " << (matches_export.export_matches ? "true" : "false") << "\n";
        LOG_DEBUG_EN << "  export_match_path: " << matches_export.export_match_path << "\n";
    }

    // ==================== Img2MatchesParameterConverter Implementation | Img2MatchesParameterConverter 实现 ====================

    // Convert parameters to method options | 将参数转换为方法选项
    std::unordered_map<std::string, std::string> Img2MatchesParameterConverter::ToMethodOptions(const Img2MatchesParameters &params)
    {
        std::unordered_map<std::string, std::string> options = {
            {"ProfileCommit", params.base.profile_commit},
            {"enable_profiling", params.base.enable_profiling ? "true" : "false"},
            {"enable_evaluator", params.base.enable_evaluator ? "true" : "false"},
            {"log_level", std::to_string(params.base.log_level)},
            {"run_mode", RunModeToString(params.base.run_mode)},
            {"detector_type", params.base.detector_type},
            {"num_threads", std::to_string(params.base.num_threads)},
            {"export_features", params.feature_export.export_features ? "ON" : "OFF"},
            {"export_fea_path", params.feature_export.export_fea_path},
            {"export_matches", params.matches_export.export_matches ? "ON" : "OFF"},
            {"export_match_path", params.matches_export.export_match_path},
            {"matcher_type", MatcherTypeToString(params.matching.matcher_type)},
            {"cross_check", params.matching.cross_check ? "true" : "false"},
            {"ratio_thresh", std::to_string(params.matching.ratio_thresh)},
            {"max_matches", std::to_string(params.matching.max_matches)},
            {"show_view_pair_i", std::to_string(params.visualization.show_view_pair_i)},
            {"show_view_pair_j", std::to_string(params.visualization.show_view_pair_j)}};

        // Add FLANN-specific parameters (using section|key format) | 添加FLANN特定参数（使用section|key格式）
        if (params.matching.matcher_type == MatcherType::FLANN)
        {
            options["FLANN|use_advanced_control"] = params.flann.use_advanced_control ? "true" : "false";
            options["FLANN|preset"] = Img2MatchesParameterConverter::FLANNPresetToString(params.flann.preset);
            options["FLANN|algorithm"] = Img2MatchesParameterConverter::FLANNAlgorithmToString(params.flann.algorithm);
            options["FLANN|trees"] = std::to_string(params.flann.trees);
            options["FLANN|table_number"] = std::to_string(params.flann.table_number);
            options["FLANN|key_size"] = std::to_string(params.flann.key_size);
            options["FLANN|multi_probe_level"] = std::to_string(params.flann.multi_probe_level);
            options["FLANN|branching"] = std::to_string(params.flann.branching);
            options["FLANN|iterations"] = std::to_string(params.flann.iterations);
            options["FLANN|centers_init"] = Img2MatchesParameterConverter::FLANNCentersInitToString(params.flann.centers_init);
            options["FLANN|checks"] = std::to_string(params.flann.checks);
            options["FLANN|eps"] = std::to_string(params.flann.eps);
            options["FLANN|sorted"] = params.flann.sorted ? "true" : "false";
            options["FLANN|max_neighbors"] = std::to_string(params.flann.max_neighbors);
        }

        // Add SuperPoint-specific parameters (using section|key format) | 添加SuperPoint特定参数（使用section|key格式）
        if (params.base.detector_type == "SUPERPOINT")
        {
            options["SUPERPOINT|max_keypoints"] = std::to_string(params.superpoint.max_keypoints);
            options["SUPERPOINT|detection_threshold"] = std::to_string(params.superpoint.detection_threshold);
            options["SUPERPOINT|nms_radius"] = std::to_string(params.superpoint.nms_radius);
            options["SUPERPOINT|remove_borders"] = std::to_string(params.superpoint.remove_borders);
            options["SUPERPOINT|python_executable"] = params.superpoint.python_executable;
        }

        // Add LightGlue-specific parameters (using section|key format) | 添加LightGlue特定参数（使用section|key格式）
        if (params.matching.matcher_type == MatcherType::LIGHTGLUE)
        {
            options["LIGHTGLUE|feature_type"] = Img2MatchesParameterConverter::LightGlueFeatureTypeToString(params.lightglue.feature_type);
            options["LIGHTGLUE|max_num_keypoints"] = std::to_string(params.lightglue.max_num_keypoints);
            options["LIGHTGLUE|depth_confidence"] = std::to_string(params.lightglue.depth_confidence);
            options["LIGHTGLUE|width_confidence"] = std::to_string(params.lightglue.width_confidence);
            options["LIGHTGLUE|filter_threshold"] = std::to_string(params.lightglue.filter_threshold);
            options["LIGHTGLUE|flash_attention"] = params.lightglue.flash_attention ? "true" : "false";
            options["LIGHTGLUE|mixed_precision"] = params.lightglue.mixed_precision ? "true" : "false";
            options["LIGHTGLUE|compile_model"] = params.lightglue.compile_model ? "true" : "false";
            options["LIGHTGLUE|python_executable"] = params.lightglue.python_executable;
            options["LIGHTGLUE|script_path"] = params.lightglue.script_path;
        }

        return options;
    }

    // Convert matcher type to string | 将匹配器类型转换为字符串
    std::string Img2MatchesParameterConverter::MatcherTypeToString(MatcherType type)
    {
        switch (type)
        {
        case MatcherType::FASTCASCADEHASHINGL2:
            return "FASTCASCADEHASHINGL2";
        case MatcherType::FLANN:
            return "FLANN";
        case MatcherType::BF:
            return "BF";
        case MatcherType::BF_NORM_L1:
            return "BF_NORM_L1";
        case MatcherType::BF_HAMMING:
            return "BF_HAMMING";
        case MatcherType::LIGHTGLUE:
            return "LIGHTGLUE";
        default:
            return "FASTCASCADEHASHINGL2";
        }
    }

    // Convert string to matcher type | 将字符串转换为匹配器类型
    MatcherType Img2MatchesParameterConverter::StringToMatcherType(const std::string &str)
    {
        if (boost::iequals(str, "FASTCASCADEHASHINGL2"))
            return MatcherType::FASTCASCADEHASHINGL2;
        else if (boost::iequals(str, "FLANN"))
            return MatcherType::FLANN;
        else if (boost::iequals(str, "BF"))
            return MatcherType::BF;
        else if (boost::iequals(str, "BF_NORM_L1"))
            return MatcherType::BF_NORM_L1;
        else if (boost::iequals(str, "BF_HAMMING"))
            return MatcherType::BF_HAMMING;
        else if (boost::iequals(str, "LIGHTGLUE"))
            return MatcherType::LIGHTGLUE;
        else
        {
            LOG_DEBUG_ZH << "未知的匹配器类型: " << str << "，使用默认的FASTCASCADEHASHINGL2";
            LOG_DEBUG_EN << "Unknown matcher type: " << str << ", using default FASTCASCADEHASHINGL2";
            return MatcherType::FASTCASCADEHASHINGL2;
        }
    }

    // Convert run mode to string | 将运行模式转换为字符串
    std::string Img2MatchesParameterConverter::RunModeToString(RunMode mode)
    {
        switch (mode)
        {
        case RunMode::Fast:
            return "fast";
        case RunMode::Viewer:
            return "viewer";
        default:
            return "fast";
        }
    }

    // Convert string to run mode | 将字符串转换为运行模式
    RunMode Img2MatchesParameterConverter::StringToRunMode(const std::string &str)
    {
        if (boost::iequals(str, "fast"))
            return RunMode::Fast;
        else if (boost::iequals(str, "viewer"))
            return RunMode::Viewer;
        else
        {
            LOG_DEBUG_ZH << "未知的运行模式: " << str << "，使用默认的fast";
            LOG_DEBUG_EN << "Unknown run mode: " << str << ", using default fast";
            return RunMode::Fast;
        }
    }

    // Convert data types mode to string | 将数据类型模式转换为字符串
    std::string Img2MatchesParameterConverter::DataTypesModeToString(DataTypesMode mode)
    {
        switch (mode)
        {
        case DataTypesMode::Full:
            return "full";
        case DataTypesMode::Single:
            return "single";
        default:
            return "full";
        }
    }

    // Convert string to data types mode | 将字符串转换为数据类型模式
    DataTypesMode Img2MatchesParameterConverter::StringToDataTypesMode(const std::string &str)
    {
        if (boost::iequals(str, "full"))
            return DataTypesMode::Full;
        else if (boost::iequals(str, "single"))
            return DataTypesMode::Single;
        else
        {
            LOG_DEBUG_ZH << "未知的数据类型模式: " << str << "，使用默认的full";
            LOG_DEBUG_EN << "Unknown data types mode: " << str << ", using default full";
            return DataTypesMode::Full;
        }
    }

    // Convert SIFT preset to string | 将SIFT预设转换为字符串
    std::string Img2MatchesParameterConverter::SIFTPresetToString(SIFTPreset preset)
    {
        switch (preset)
        {
        case SIFTPreset::NORMAL:
            return "NORMAL";
        case SIFTPreset::HIGH:
            return "HIGH";
        case SIFTPreset::ULTRA:
            return "ULTRA";
        case SIFTPreset::CUSTOM:
        default:
            return "CUSTOM";
        }
    }

    // Convert string to SIFT preset | 将字符串转换为SIFT预设
    SIFTPreset Img2MatchesParameterConverter::StringToSIFTPreset(const std::string &str)
    {
        if (boost::iequals(str, "NORMAL"))
            return SIFTPreset::NORMAL;
        else if (boost::iequals(str, "HIGH"))
            return SIFTPreset::HIGH;
        else if (boost::iequals(str, "ULTRA"))
            return SIFTPreset::ULTRA;
        else if (boost::iequals(str, "CUSTOM"))
            return SIFTPreset::CUSTOM;
        else
        {
            LOG_DEBUG_ZH << "未知的SIFT预设类型: " << str << "，使用默认的CUSTOM";
            LOG_DEBUG_EN << "Unknown SIFT preset type: " << str << ", using default CUSTOM";
            return SIFTPreset::CUSTOM;
        }
    }

    // ==================== FLANN Conversion Functions | FLANN转换函数 ====================

    // Convert FLANN algorithm to string | 将FLANN算法转换为字符串
    std::string Img2MatchesParameterConverter::FLANNAlgorithmToString(FLANNAlgorithm algorithm)
    {
        switch (algorithm)
        {
        case FLANNAlgorithm::AUTO:
            return "AUTO";
        case FLANNAlgorithm::KDTREE:
            return "KDTREE";
        case FLANNAlgorithm::LSH:
            return "LSH";
        case FLANNAlgorithm::KMEANS:
            return "KMEANS";
        case FLANNAlgorithm::COMPOSITE:
            return "COMPOSITE";
        case FLANNAlgorithm::LINEAR:
            return "LINEAR";
        default:
            return "AUTO";
        }
    }

    // Convert string to FLANN algorithm | 将字符串转换为FLANN算法
    FLANNAlgorithm Img2MatchesParameterConverter::StringToFLANNAlgorithm(const std::string &str)
    {
        if (boost::iequals(str, "AUTO"))
            return FLANNAlgorithm::AUTO;
        else if (boost::iequals(str, "KDTREE"))
            return FLANNAlgorithm::KDTREE;
        else if (boost::iequals(str, "LSH"))
            return FLANNAlgorithm::LSH;
        else if (boost::iequals(str, "KMEANS"))
            return FLANNAlgorithm::KMEANS;
        else if (boost::iequals(str, "COMPOSITE"))
            return FLANNAlgorithm::COMPOSITE;
        else if (boost::iequals(str, "LINEAR"))
            return FLANNAlgorithm::LINEAR;
        else
        {
            LOG_DEBUG_ZH << "未知的FLANN算法类型: " << str << "，使用默认的AUTO";
            LOG_DEBUG_EN << "Unknown FLANN algorithm type: " << str << ", using default AUTO";
            return FLANNAlgorithm::AUTO;
        }
    }

    // Convert FLANN preset to string | 将FLANN预设转换为字符串
    std::string Img2MatchesParameterConverter::FLANNPresetToString(FLANNPreset preset)
    {
        switch (preset)
        {
        case FLANNPreset::FAST:
            return "FAST";
        case FLANNPreset::BALANCED:
            return "BALANCED";
        case FLANNPreset::ACCURATE:
            return "ACCURATE";
        case FLANNPreset::CUSTOM:
        default:
            return "CUSTOM";
        }
    }

    // Convert string to FLANN preset | 将字符串转换为FLANN预设
    FLANNPreset Img2MatchesParameterConverter::StringToFLANNPreset(const std::string &str)
    {
        if (boost::iequals(str, "FAST"))
            return FLANNPreset::FAST;
        else if (boost::iequals(str, "BALANCED"))
            return FLANNPreset::BALANCED;
        else if (boost::iequals(str, "ACCURATE"))
            return FLANNPreset::ACCURATE;
        else if (boost::iequals(str, "CUSTOM"))
            return FLANNPreset::CUSTOM;
        else
        {
            LOG_DEBUG_ZH << "未知的FLANN预设类型: " << str << "，使用默认的BALANCED";
            LOG_DEBUG_EN << "Unknown FLANN preset type: " << str << ", using default BALANCED";
            return FLANNPreset::BALANCED;
        }
    }

    // Convert FLANN centers initialization to string | 将FLANN中心初始化方式转换为字符串
    std::string Img2MatchesParameterConverter::FLANNCentersInitToString(FLANNCentersInit centers_init)
    {
        switch (centers_init)
        {
        case FLANNCentersInit::CENTERS_RANDOM:
            return "CENTERS_RANDOM";
        case FLANNCentersInit::CENTERS_GONZALES:
            return "CENTERS_GONZALES";
        case FLANNCentersInit::CENTERS_KMEANSPP:
            return "CENTERS_KMEANSPP";
        default:
            return "CENTERS_RANDOM";
        }
    }

    // Convert string to FLANN centers initialization | 将字符串转换为FLANN中心初始化方式
    FLANNCentersInit Img2MatchesParameterConverter::StringToFLANNCentersInit(const std::string &str)
    {
        if (boost::iequals(str, "CENTERS_RANDOM"))
            return FLANNCentersInit::CENTERS_RANDOM;
        else if (boost::iequals(str, "CENTERS_GONZALES"))
            return FLANNCentersInit::CENTERS_GONZALES;
        else if (boost::iequals(str, "CENTERS_KMEANSPP"))
            return FLANNCentersInit::CENTERS_KMEANSPP;
        else
        {
            LOG_DEBUG_ZH << "未知的FLANN中心初始化方式: " << str << "，使用默认的CENTERS_RANDOM";
            LOG_DEBUG_EN << "Unknown FLANN centers initialization type: " << str << ", using default CENTERS_RANDOM";
            return FLANNCentersInit::CENTERS_RANDOM;
        }
    }

    // Convert LightGlue feature type to string | 将LightGlue特征类型转换为字符串
    std::string Img2MatchesParameterConverter::LightGlueFeatureTypeToString(LightGlueFeatureType type)
    {
        switch (type)
        {
        case LightGlueFeatureType::SUPERPOINT:
            return "SUPERPOINT";
        case LightGlueFeatureType::DISK:
            return "DISK";
        case LightGlueFeatureType::SIFT:
            return "SIFT";
        case LightGlueFeatureType::ALIKED:
            return "ALIKED";
        case LightGlueFeatureType::DOGHARDNET:
            return "DOGHARDNET";
        default:
            return "SUPERPOINT";
        }
    }

    // Convert string to LightGlue feature type | 将字符串转换为LightGlue特征类型
    LightGlueFeatureType Img2MatchesParameterConverter::StringToLightGlueFeatureType(const std::string &str)
    {
        if (boost::iequals(str, "SUPERPOINT"))
            return LightGlueFeatureType::SUPERPOINT;
        else if (boost::iequals(str, "DISK"))
            return LightGlueFeatureType::DISK;
        else if (boost::iequals(str, "SIFT"))
            return LightGlueFeatureType::SIFT;
        else if (boost::iequals(str, "ALIKED"))
            return LightGlueFeatureType::ALIKED;
        else if (boost::iequals(str, "DOGHARDNET"))
            return LightGlueFeatureType::DOGHARDNET;
        else
        {
            LOG_DEBUG_ZH << "未知的LightGlue特征类型: " << str << "，使用默认的SUPERPOINT";
            LOG_DEBUG_EN << "Unknown LightGlue feature type: " << str << ", using default SUPERPOINT";
            return LightGlueFeatureType::SUPERPOINT;
        }
    }

} // namespace PluginMethods
