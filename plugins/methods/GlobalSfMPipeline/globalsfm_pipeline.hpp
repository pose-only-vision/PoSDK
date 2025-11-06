/**
 * @file globalsfm_pipeline.hpp
 * @brief Global SfM Processing Pipeline | 全局SfM处理流水线
 * @copyright Copyright (c) 2024 PoSDK
 */

#pragma once

#include <po_core.hpp>
#include <po_core/po_logger.hpp>
#include <common/converter/converter_openmvg_file.hpp>
#include "GlobalSfMPipelineParams.hpp"
#include <filesystem>
#include <vector>
#include <memory>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    /**
     * @brief Global SfM Processing Pipeline | 全局SfM处理流水线
     *
     * Encapsulates the complete global SfM processing workflow from image preprocessing to final reconstruction,
     * including feature extraction, matching, two-view estimation, rotation averaging, track building,
     * global optimization, and analytical reconstruction.
     * 封装了从图像预处理到最终重建的完整全局SfM处理流程，
     * 包括特征提取、匹配、双视图估计、旋转平均、轨迹构建、
     * 全局优化和解析式重建等步骤。
     */
    class GlobalSfMPipeline : public Interface::MethodPresetProfiler
    {
    public:
        /**
         * @brief Constructor | 构造函数
         */
        GlobalSfMPipeline();

        /**
         * @brief Destructor | 析构函数
         */
        virtual ~GlobalSfMPipeline() = default;

        /**
         * @brief Get method type | 获取方法类型
         * @return Method type string | 方法类型字符串
         */
        const std::string &GetType() const override;

        /**
         * @brief Execute complete global SfM pipeline | 执行完整的全局SfM流水线
         * @return Processing result data | 处理结果数据
         */
        DataPtr Run() override;

    private:
        // Pipeline step functions | 流水线步骤函数
        /**
         * @brief Step 1: Image preprocessing and feature extraction | 步骤1: 图像预处理和特征提取
         * @return Preprocessing result data | 预处理结果数据
         */
        DataPtr Step1_ImagePreprocessing();

        // Preprocessing helper methods | 预处理器辅助方法
        void RunComparedPipelinesIfNeeded();
        DataPtr RunOpenMVGPipeline(); // Main preprocessor: OpenMVG | 主预处理器：OpenMVG
        DataPtr RunPoSDKPreprocess(); // Main preprocessor: OpenCV/PoSDK (both use this function) | 主预处理器：OpenCV/PoSDK（两者都使用此函数）

        // Comparison pipeline helper methods (used by comparison runners) | 对比流水线辅助方法（供对比运行器使用）
        DataPtr RunColmapPreprocess(); // Helper for Colmap comparison | Colmap对比辅助函数
        DataPtr RunGlomapPreprocess(); // Helper for Glomap comparison | Glomap对比辅助函数

        // Comparison pipeline specific methods | 对比流水线专用方法
        void RunOpenMVGForComparison();
        void RunColmapForComparison();
        void RunGlomapForComparison();

        std::string GetPreprocessTypeStr() const;

        // Visualization helper methods | 可视化辅助方法
        void VisualizeMatches(DataPtr data_package, const std::string &stage, const std::string &description);
        std::string GetCurrentDatasetName(); // Get current dataset name | 获取当前处理的数据集名称

        /**
         * @brief Step 2: Two-view pose estimation | 步骤2: 双视图位姿估计
         * @param preprocess_result Preprocessing result | 预处理结果
         * @return Relative pose estimation result | 相对位姿估计结果
         */
        DataPtr Step2_TwoViewEstimation(DataPtr preprocess_result);

        /**
         * @brief Step 2.5: Rotation refinement using color-based block matching | 步骤2.5: 基于颜色块匹配的旋转优化
         * @details Refines relative rotations before rotation averaging by matching color-connected region blocks
         *          在旋转平均前通过匹配颜色连通区域块来优化相对旋转
         * @param relative_poses_result Relative pose result from Step 2 | 来自步骤2的相对位姿结果
         * @param preprocess_result Preprocessing result (contains image paths, matches, features, camera models) | 预处理结果（包含图像路径、匹配、特征、相机模型）
         * @return Refined relative pose result | 优化后的相对位姿结果
         */
        DataPtr Step2_5_RotationRefinement(DataPtr relative_poses_result, DataPtr preprocess_result);

        /**
         * @brief Step 3: Rotation averaging | 步骤3: 旋转平均
         * @param relative_poses_result Relative pose result | 相对位姿结果
         * @param camera_models Camera model data | 相机模型数据
         * @return Rotation averaging result | 旋转平均结果
         */
        DataPtr Step3_RotationAveraging(DataPtr relative_poses_result, DataPtr camera_models);

        /**
         * @brief Step 4: Feature track building | 步骤4: 特征轨迹构建
         * @param matches_data Match data | 匹配数据
         * @param features_data Feature data | 特征数据
         * @return Track building result | 轨迹构建结果
         */
        DataPtr Step4_TrackBuilding(DataPtr matches_data, DataPtr features_data);

        // Helper functions | 辅助函数

        /**
         * @brief Load ground truth data | 加载真值数据
         * @return Whether loading was successful | 是否成功加载
         */
        bool LoadGroundTruthData();

        /**
         * @brief Clear ground truth data (for switching datasets in batch mode) | 清空真值数据（用于批处理模式下切换数据集）
         */
        void ClearGroundTruthData();

        /**
         * @brief Create camera model for Strecha dataset | 创建Strecha数据集的相机模型
         * @return Camera model data | 相机模型数据
         */
        DataPtr CreateStrechaCameraModel();

        /**
         * @brief Load ground truth global poses from Strecha dataset | 从Strecha数据集加载真值全局位姿
         * @param gt_folder Ground truth folder path | 真值文件夹路径
         * @param global_poses Output global pose data | 输出的全局位姿数据
         * @return Whether loading was successful | 是否成功加载
         */
        bool LoadGTFiles(const std::string &gt_folder, types::GlobalPoses &global_poses);

        /**
         * @brief Evaluate relative pose accuracy | 评估相对位姿精度
         * @param estimated_poses Estimated pose data | 估计的位姿数据
         * @return Evaluation result data | 评估结果数据
         */
        DataPtr EvaluateRelativePoseAccuracy(DataPtr estimated_poses);

        /**
         * @brief Evaluate global pose accuracy | 评估全局位姿精度
         * @param estimated_poses Estimated pose data | 估计的位姿数据
         * @return Evaluation result data | 评估结果数据
         */
        DataPtr EvaluateGlobalPoseAccuracy(DataPtr estimated_poses);

        /**
         * @brief Create sub-method and configure parameters | 创建子方法并配置参数
         * @param method_type Method type | 方法类型
         * @return Method instance | 方法实例
         */
        MethodPresetProfilerPtr CreateAndConfigureSubMethod(const std::string &method_type);

        /**
         * @brief Evaluate pose accuracy | 评估位姿精度
         * @param estimated_poses Estimated pose data | 估计的位姿数据
         * @param pose_type Pose type ("relative" or "global") | 位姿类型（"relative"或"global"）
         * @return Evaluation result data | 评估结果数据
         */
        DataPtr EvaluatePoseAccuracy(DataPtr estimated_poses, const std::string &pose_type);

        /**
         * @brief Perform manual relative pose evaluation (for verifying automatic evaluation correctness)
         * 执行手动相对位姿评估（用于验证自动评估结果的正确性）
         * @param relative_poses_result Two-view estimator output relative pose result | 双视图估计器输出的相对位姿结果
         * @param dataset_name Currently processed dataset name | 当前处理的数据集名称
         */
        void PerformManualRelativePoseEvaluation(DataPtr relative_poses_result, const std::string &dataset_name);

        /**
         * @brief Evaluate OpenMVG global pose results (for comparison pipeline functionality)
         * 评估OpenMVG全局位姿结果（用于对比流水线功能）
         * @param dataset_name Currently processed dataset name | 当前处理的数据集名称
         */
        void EvaluateOpenMVGGlobalPoses(const std::string &dataset_name);

        /**
         * @brief Evaluate COLMAP global pose results (for comparison pipeline functionality)
         * 评估COLMAP全局位姿结果（用于对比流水线功能）
         * @param dataset_name Currently processed dataset name | 当前处理的数据集名称
         */
        void EvaluateColmapGlobalPoses(const std::string &dataset_name);

        /**
         * @brief Evaluate GLOMAP global pose results (for comparison pipeline functionality)
         * 评估GLOMAP全局位姿结果（用于对比流水线功能）
         * @param dataset_name Currently processed dataset name | 当前处理的数据集名称
         */
        void EvaluateGlomapGlobalPoses(const std::string &dataset_name);

        /**
         * @brief Parse compared_pipelines parameter and set comparison pipeline flags
         * 解析compared_pipelines参数，设置对比流水线标志
         */
        void ParseComparedPipelines();

        /**
         * @brief Run PoSDK pipeline | 运行PoSDK流水线
         */
        DataPtr RunPoSDKPipeline();

        /**
         * @brief Export Meshlab project file (based on test_Strecha.cpp implementation)
         * 导出Meshlab工程文件（参考test_Strecha.cpp实现）
         * @param global_poses_result Global pose result | 全局位姿结果
         * @param reconstruction_result Analytical reconstruction 3D point result | 解析式重建的3D点结果
         * @param camera_models Camera model data | 相机模型数据
         * @param images_data Image data | 图像数据
         * @param dataset_name Dataset name | 数据集名称
         */
        void ExportMeshlabProject(DataPtr global_poses_result, DataPtr reconstruction_result,
                                  DataPtr camera_models, DataPtr images_data, const std::string &dataset_name);

        /**
         * @brief Export PoSDK data to Colmap format | 导出PoSDK数据到Colmap格式
         * @param global_poses_result Global pose data | 全局位姿数据
         * @param camera_models Camera model data | 相机模型数据
         * @param features_data Feature data | 特征数据
         * @param tracks_result Track data | 轨迹数据
         * @param reconstruction_result 3D point reconstruction data (optional) | 3D点重建数据（可选）
         * @param dataset_name Dataset name | 数据集名称
         */
        void ExportPoSDK2Colmap(DataPtr global_poses_result, DataPtr camera_models, DataPtr features_data,
                                DataPtr tracks_result, DataPtr reconstruction_result, const std::string &dataset_name);

        /**
         * @brief Check if relative pose accuracy evaluation data exists
         * 检查相对位姿精度评估数据是否存在
         * @param estimated_poses Estimated relative pose data | 估计的相对位姿数据
         * @return Input data (for chain calling) | 输入数据（用于链式调用）
         * @note This function only checks evaluation data existence, detailed statistics are handled uniformly by PrintEvaluationResults
         * 此函数仅检查评估数据存在性，详细统计由PrintEvaluationResults统一处理
         */
        void PrintRelativePosesAccuracy();

        /**
         * @brief Export all evaluation results to CSV files (for debugging and batch analysis)
         * 导出所有评估结果到CSV文件（调试和批量分析用）
         */
        void ExportAllEvaluationResultsToCSV();

        /**
         * @brief Export specific evaluation type results to CSV file
         * 导出指定评估类型的结果到CSV文件
         * @param eval_type Evaluation type (e.g., "RelativePoses", "GlobalPoses") | 评估类型（如"RelativePoses", "GlobalPoses"等）
         */
        void ExportSpecificEvaluationToCSV(const std::string &eval_type);

        /**
         * @brief Clean meaningless N/A rows in CSV file | 清理CSV文件中的无意义N/A行
         * @param csv_file_path CSV file path | CSV文件路径
         */
        void CleanCSVFile(const std::filesystem::path &csv_file_path);

        /**
         * @brief Print evaluation results | 打印评估结果
         * @param print_mode Print mode: "none", "summary", "detailed", "comparison" | 打印模式："none", "summary", "detailed", "comparison"
         */
        void PrintEvaluationResults(const std::string &print_mode);

        /**
         * @brief Check if global pose accuracy evaluation data exists
         * 检查全局位姿精度评估数据是否存在
         * @param estimated_poses Estimated global pose data | 估计的全局位姿数据
         * @return Input data (for chain calling) | 输入数据（用于链式调用）
         * @note This function only checks evaluation data existence, detailed statistics are handled uniformly by PrintEvaluationResults
         * 此函数仅检查评估数据存在性，详细统计由PrintEvaluationResults统一处理
         */
        void PrintGlobalPosesAccuracy();

        /**
         * @brief Save result data | 保存结果数据
         * @param result_data Result data | 结果数据
         * @param filename File name | 文件名
         * @param extension File extension | 文件扩展名
         * @return Whether saving was successful | 是否保存成功
         */
        bool SaveResults(DataPtr result_data, const std::string &filename, const std::string &extension);

        /**
         * @brief Compute coordinate changes between two track datasets
         * 计算两个轨迹数据集之间的坐标变化
         * @param current_tracks_data Current track data | 当前轨迹数据
         * @param initial_tracks_data Initial track data | 初始轨迹数据
         * @return Coordinate change value (0.5*sum(v*v)) | 坐标变化值(0.5*sum(v*v))
         */
        double ComputeCoordinateChanges(DataPtr current_tracks_data,
                                        DataPtr initial_tracks_data);

        /**
         * @brief Scan dataset directory to find all datasets containing images subdirectory
         * 扫描数据集目录，查找所有包含images子目录的数据集
         * @param dataset_dir Dataset root directory | 数据集根目录
         * @return List of dataset image folder paths | 数据集图像文件夹路径列表
         */
        std::vector<std::string> ScanDatasetDirectory(const std::string &dataset_dir);

        /**
         * @brief Extract dataset name from image folder path | 从图像文件夹路径提取数据集名称
         * @param image_folder_path Image folder path | 图像文件夹路径
         * @return Dataset name | 数据集名称
         */
        std::string ExtractDatasetName(const std::string &image_folder_path);

        /**
         * @brief Prepare camera model data | 准备相机模型数据
         * @return Camera model data pointer | 相机模型数据指针
         */
        Interface::DataPtr PrepareCameraModel();

        // ==================== Unified Table Generation Functionality | 统一制表功能 ====================

        /**
         * @brief Generate unified summary table and output to working directory (based on CSV file merging)
         * 生成统一制表并输出到工作目录（基于CSV文件合并）
         * @param dataset_names List of all dataset names | 所有数据集名称列表
         * @return Whether successful | 是否成功
         */
        bool GenerateSummaryTable(const std::vector<std::string> &dataset_names);

        /**
         * @brief Generate summary table for specified metric | 为指定指标生成汇总表格
         * @param eval_type Evaluation type | 评估类型
         * @param metric Metric name | 指标名称
         * @param dataset_names List of dataset names | 数据集名称列表
         * @param summary_dir Summary output directory | 汇总输出目录
         * @return Whether successful | 是否成功
         */
        bool GenerateSummaryTableForMetric(const std::string &eval_type,
                                           const std::string &metric,
                                           const std::vector<std::string> &dataset_names,
                                           const std::string &summary_dir);

    private:
        // Parameter container | 参数容器
        PipelineParameters params_;

        // Ground truth data | 真值数据
        types::GlobalPoses gt_global_poses_;
        types::RelativePoses gt_relative_poses_;

        // Camera model data | 相机模型数据
        Interface::DataPtr camera_model_data_;

        // Sub-method instances | 子方法实例
        MethodPresetProfilerPtr openmvg_pipeline_;
        MethodPresetProfilerPtr colmap_preprocess_;
        MethodPresetProfilerPtr glomap_preprocess_;
        MethodPresetProfilerPtr img2features_;       // PoSDK feature extraction (compatibility) | PoSDK特征提取（暂时保留兼容性）
        MethodPresetProfilerPtr img2matches_;        // PoSDK integrated feature extraction + matching | PoSDK一体化特征提取+匹配
        MethodPresetProfilerPtr matches_visualizer_; // Match relation visualizer | 匹配关系可视化器
        MethodPresetProfilerPtr two_view_estimator_;
        MethodPresetProfilerPtr rotation_averager_;
        MethodPresetProfilerPtr track_builder_;
        MethodPresetProfilerPtr ligt_optimizer_;
        MethodPresetProfilerPtr outlier_remover_;
        MethodPresetProfilerPtr pose_optimizer_;
        MethodPresetProfilerPtr analytical_reconstructor_;
        std::string pipeline_name_;

        // Current processing dataset name (for visualization output path) | 当前处理的数据集名称（用于可视化输出路径）
        std::string current_dataset_name_;

        // Comparison pipeline flags (set after parsing compared_pipelines parameter) | 对比流水线标志（解析compared_pipelines参数后设置）
        bool is_compared_openmvg_ = false; // Whether to compare with OpenMVG | 是否需要对比OpenMVG
        bool is_compared_colmap_ = false;  // Whether to compare with Colmap | 是否需要对比Colmap
        bool is_compared_glomap_ = false;  // Whether to compare with Glomap | 是否需要对比Glomap

        // ==================== Data Statistics Functionality | 数据统计功能 ====================

        /**
         * @brief Initialize data statistics document | 初始化数据统计文档
         * @param dataset_name Dataset name | 数据集名称
         */
        void InitializeDataStatistics(const std::string &dataset_name);

        /**
         * @brief Add step data statistics information | 添加步骤数据统计信息
         * @param step_name Step name | 步骤名称
         * @param data_ptr Data pointer | 数据指针
         * @param description Description information | 描述信息
         */
        void AddStepDataStatistics(const std::string &step_name, DataPtr data_ptr, const std::string &description);

        /**
         * @brief Analyze DataPtr and generate statistics information | 分析DataPtr并生成统计信息
         * @param data_ptr Data pointer | 数据指针
         * @return Statistics information string (Markdown format) | 统计信息字符串（Markdown格式）
         */
        std::string AnalyzeDataPtr(DataPtr data_ptr);

        /**
         * @brief Analyze specific data type and generate statistics information | 分析特定数据类型并生成统计信息
         * @param data_ptr Data pointer | 数据指针
         * @return Statistics information string (Markdown format) | 统计信息字符串（Markdown格式）
         */
        std::string AnalyzeSpecificDataType(DataPtr data_ptr);

        /**
         * @brief Complete data statistics and output final report | 完成数据统计并输出最终报告
         */
        void FinalizeDataStatistics();

        /**
         * @brief Add iterative optimization process data statistics | 添加迭代优化过程的数据统计
         * @param iteration Iteration count | 迭代次数
         * @param tracks_result Track data | 轨迹数据
         * @param poses_result Pose data | 位姿数据
         * @param angle_threshold Angle threshold | 角度阈值
         */
        void AddIterationDataStatistics(int iteration, DataPtr tracks_result, DataPtr poses_result,
                                        double angle_threshold);

        /**
         * @brief Add Step6 final statistics (including iteration summary)
         * 添加Step6最终统计信息（包含迭代汇总）
         * @param final_poses Final pose data | 最终位姿数据
         * @param iteration_rot_errors Iterative rotation errors | 迭代旋转误差
         * @param iteration_pos_errors Iterative position errors | 迭代位置误差
         * @param iteration_residual_info Iterative residual information | 迭代残差信息
         */
        void AddStep6FinalStatistics(DataPtr final_poses,
                                     const std::vector<double> &iteration_rot_errors,
                                     const std::vector<double> &iteration_pos_errors,
                                     const std::vector<std::string> &iteration_residual_info);

        // Time statistics helper methods | 时间统计辅助方法
        void RecordStepCoreTime(const std::string &step_name, MethodPresetProfilerPtr method);
        void RecordStep1CoreTime();                                   // Specialized handling for Step1 complex time statistics | 专门处理Step1的复杂时间统计
        void EvaluateTimeStatistics(const std::string &dataset_name); // Main time evaluation function | 主要时间评估函数
        void AddDatasetTimeStatisticsToEvaluator(const std::string &dataset_name, double dataset_core_time);
        DataPtr PrintTimeStatisticsAccuracy(const std::string &dataset_name);
        void PrintTimeStatisticsSummary();

        // Data statistics related member variables | 数据统计相关成员变量
        std::string data_statistics_file_path_; // Data statistics file path | 数据统计文件路径
        std::ofstream data_statistics_stream_;  // Data statistics file stream | 数据统计文件流

        // Time statistics related member variables | 时间统计相关成员变量
        double total_pipeline_time_;                                        // Total pipeline time (milliseconds) | 总流水线时间（毫秒）
        double accumulated_core_time_;                                      // Accumulated core computation time (milliseconds) | 累积核心计算时间（毫秒）
        std::vector<std::pair<std::string, double>> step_core_times_;       // Step core time records | 各步骤核心时间记录
        std::chrono::high_resolution_clock::time_point dataset_start_time_; // Current dataset start time | 当前数据集开始时间
    };

} // namespace PluginMethods
