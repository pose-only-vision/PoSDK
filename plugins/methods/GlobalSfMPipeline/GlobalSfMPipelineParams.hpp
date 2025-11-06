/**
 * @file GlobalSfMPipelineParams.hpp
 * @brief Parameter configuration system for GlobalSfM pipeline | GlobalSfM流水线参数配置系统
 * @copyright Copyright (c) 2024 PoSDK
 */

#pragma once

#include <po_core.hpp>
#include <po_core/po_logger.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;

    // ==================== Preprocessing Type Enumeration | 预处理类型枚举 ====================

    /**
     * @brief Preprocessing type enumeration | 预处理类型枚举
     */
    enum class PreprocessType
    {
        OpenMVG, // Use OpenMVG preprocessing (default) | 使用OpenMVG预处理（默认）
        OpenCV,  // Use OpenCV-based PoSDK preprocessing (method_img2matches) | 使用基于OpenCV的PoSDK预处理（method_img2matches）
        PoSDK    // Use optimized PoSDK preprocessing (posdk_preprocessor) | 使用优化的PoSDK预处理（posdk_preprocessor）
    };

    // ==================== Parameter Structure Definitions | 参数结构体定义 ====================

    /**
     * @brief Base configuration parameters | 基础配置参数
     */
    struct BaseParameters
    {
        std::string dataset_dir;                                // Dataset root directory | 数据集根目录
        std::string image_folder;                               // Image folder path | 图像文件夹路径
        std::string work_dir;                                   // Working directory | 工作目录
        std::string gt_folder;                                  // Ground truth data folder | 真值数据文件夹
        bool enable_evaluation = true;                          // Enable evaluation | 是否启用评估
        int max_iterations = 5;                                 // Maximum number of iterations | 最大迭代次数
        bool enable_summary_table = false;                      // Enable unified table function (generate summary table during batch processing) | 是否启用统一制表功能（批处理时生成汇总表格）
        PreprocessType preprocess_type = PreprocessType::PoSDK; // Preprocessing type: OpenMVG/OpenCV/PoSDK (default OpenMVG) | 预处理类型：OpenMVG/OpenCV/PoSDK（默认OpenMVG）
                                                                // - OpenMVG: Use OpenMVG pipeline | 使用OpenMVG流水线
                                                                // - OpenCV: Use PoSDK with method_img2matches | 使用PoSDK的method_img2matches
                                                                // - PoSDK: Use PoSDK with posdk_preprocessor | 使用PoSDK的posdk_preprocessor
        bool enable_matches_visualization = false;              // Enable matching relationship visualization (before and after two-view estimation) | 是否启用匹配关系可视化（双视图估计前后）
        bool enable_locker = true;                              // Enable Strecha dataset lock validation (pipeline level) | 是否启用Strecha数据集锁定验证（pipeline层面）
        bool enable_csv_export = true;                          // Enable CSV export | 是否启用CSV导出
        bool enable_manual_eval = false;                        // Enable manual evaluation (for verifying correctness of automatic evaluation results) | 是否启用手动评估（用于验证自动评估结果的正确性）
        bool enable_3d_points_output = false;                   // Output 3D points in final results (default only output poses) | 是否在最终结果中输出3D点（默认只输出位姿）
        bool enable_iter_evaluation = false;                    // Enable accuracy evaluation during iterative optimization process | 是否启用迭代优化过程中的精度评估
        bool enable_meshlab_export = false;                     // Enable Meshlab project file export (includes pose + 3D point visualization) | 是否启用Meshlab工程文件导出（包含位姿+3D点可视化）
        bool enable_features_info_print = false;                // Enable feature information printing after preprocessing (display image ID, path, number of feature points) | 是否启用预处理后特征信息打印（显示图像ID、路径、特征点数量）
        bool enable_data_statistics = false;                    // Enable pipeline data statistics function | 是否启用流水线数据统计功能
        std::string evaluation_print_mode = "summary";          // Evaluation result print mode: "none", "summary", "detailed", "comparison" | 评估结果打印模式："none", "summary", "detailed", "comparison"
        std::string compared_pipelines = "";                    // Comparison pipeline list (comma separated): "openmvg", "colmap", "glomap" - Complete pipelines for performance comparison | 对比流水线列表（逗号分隔）："openmvg", "colmap", "glomap" - 用于性能对比的完整流水线
                                                                // NOTE: Different from preprocess_type (which is for main preprocessing) | 注意：不同于preprocess_type（用于主预处理）
        std::string profile_commit;                             // Performance analysis identifier | 性能分析标识

        // Cache directory configuration | 缓存目录配置
        std::vector<std::string> cache_directories = {
            "storage/features", "storage/matches", "storage/logs", "storage/poses"};
    };

    /**
     * @brief OpenMVG preprocessing parameters | OpenMVG预处理参数
     */
    struct OpenMVGParameters
    {
        // Basic settings | 基础设置
        std::string root_dir; // Root directory path (consistent with test_Strecha.cpp) | 根目录路径（与test_Strecha.cpp一致）
        std::string dataset_dir;
        std::string images_folder;
        std::string work_dir;
        bool force_compute = true;
        bool debug_output = true;
        int num_threads = 4;
        bool save_features = true;
        bool save_matches = true;

        // Camera parameters | 相机参数
        std::string intrinsics = "2759.48,0,1520.69,0,2764.16,1006.81,0,0,1"; // Strecha default intrinsics | Strecha默认内参
        int camera_model = 1;
        int group_camera_model = 1;

        // Feature extraction parameters | 特征提取参数
        std::string describer_method = "SIFT";
        std::string describer_preset = "HIGH";

        // Feature matching parameters | 特征匹配参数
        std::string nearest_matching_method = "FASTCASCADEHASHINGL2";
        double ratio = 0.8; // Lowe ratio test threshold | Lowe比率测试阈值

        // Geometric filtering parameters | 几何过滤参数
        std::string geometric_model = "e"; // Use essential matrix (consistent with test_Strecha.cpp) | 使用本质矩阵（与test_Strecha.cpp一致）

        // SfM reconstruction settings | SfM重建设置
        bool enable_sfm_reconstruction = true;
        std::string sfm_engine = "GLOBAL";

        // Dynamic path parameters (set at runtime) | 动态路径参数（运行时设置）
        std::string sfm_data_filename;  // SfM data file path | SfM数据文件路径
        std::string matches_dir;        // Match data directory | 匹配数据目录
        std::string reconstruction_dir; // Reconstruction result directory | 重建结果目录

        std::string profile_commit = "GlobalSfM pipeline preprocessing";
    };

    /**
     * @brief Rotation averaging parameters | 旋转平均参数
     */
    struct RotationAveragingParameters
    {
        std::string rotation_estimator = "GraphOptim";
        std::string temp_dir = "./temp";
        std::string g2o_filename = "relative_poses.g2o";
        std::string estimator_output_g2o = "optimized_poses.g2o";
        std::string profile_commit = "GlobalSfM pipeline rotation averaging";
    };

    /**
     * @brief Track building parameters | 轨迹构建参数
     */
    struct TrackBuildingParameters
    {
        int min_track_length = 2;
        int max_track_length = 1000;
        std::string profile_commit = "GlobalSfM pipeline track building";
    };

    /**
     * @brief Overall pipeline parameter container | 总的流水线参数容器
     */
    struct PipelineParameters
    {
        BaseParameters base;
        OpenMVGParameters openmvg;
        RotationAveragingParameters rotation_averaging;
        TrackBuildingParameters track_building;

        /**
         * @brief Load parameters from configuration file | 从配置文件加载参数
         * @param config_loader Configuration loader | 配置加载器
         */
        void LoadFromConfig(Interface::MethodPreset *config_loader);

        /**
         * @brief Update dynamic parameters based on dataset name | 根据数据集名称更新动态参数
         * @param dataset_name Dataset name | 数据集名称
         */
        void UpdateDynamicParameters(const std::string &dataset_name);

        /**
         * @brief Validate parameter validity | 验证参数有效性
         * @param method_ptr Method pointer (for log output) | 方法指针（用于日志输出）
         * @return Whether valid | 是否有效
         */
        bool Validate(Interface::MethodPreset *method_ptr = nullptr) const;

        /**
         * @brief Print parameter summary | 打印参数摘要
         * @param method_ptr Method pointer (for log output) | 方法指针（用于日志输出）
         */
        void PrintSummary(Interface::MethodPreset *method_ptr = nullptr) const;
    };

} // namespace PluginMethods
