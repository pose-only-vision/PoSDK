/**
 * @file globalsfm_pipeline.cpp
 * @brief Global SfM processing pipeline implementation | 全局SfM处理流水线实现
 */

#include "globalsfm_pipeline.hpp"
#include "GlobalSfMPipelineParams.hpp"
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <common/converter/converter_colmap_file.hpp>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <numeric>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <po_core/po_logger.hpp>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    GlobalSfMPipeline::GlobalSfMPipeline()
    {
        // Register required data types | 注册所需数据类型
        required_package_["data_images"] = nullptr;

        // Initialize default configuration path | 初始化默认配置
        InitializeDefaultConfigPath();

        // Initialize time statistics variables | 初始化时间统计变量
        total_pipeline_time_ = 0.0;
        // Note: Individual step time statistics are now managed by Profiler system | 注意：单个步骤时间统计现在由Profiler系统管理
        pipeline_name_ = "PoSDK";
    }

    // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
    // ✨ GetType() 由 REGISTRATION_PLUGIN 宏自动实现（位于文件末尾）

    DataPtr GlobalSfMPipeline::Run()
    {
        SetEvaluatorAlgorithm(pipeline_name_);
        // Start total time statistics | 开始总时间统计
        auto total_start_time = std::chrono::high_resolution_clock::now();

        // Reset time statistics | 重置时间统计
        total_pipeline_time_ = 0.0;
        // Note: Individual dataset core time is now managed by Profiler system | 注意：单个数据集的核心时间现在由Profiler系统管理

        DisplayConfigInfo();

        // Load configuration parameters | 加载配置参数
        params_.LoadFromConfig(this);

        // ==================== Bilingual Log Management Demo | 双语日志管理演示内容 ====================
        // Note: Log system is already configured uniformly in MethodPreset::Build
        // 注意：日志系统已经在MethodPreset::Build中统一配置

        // ================================================================
        // Validate parameters | 验证参数
        if (!params_.Validate(this))
        {
            // Use bilingual error logs | 使用双语错误日志
            LOG_ERROR_ZH << "参数验证失败，GlobalSfM流水线无法继续执行";
            LOG_ERROR_EN << "Parameter validation failed, GlobalSfM pipeline cannot continue";
            return nullptr;
        }

        // Parameter validation success bilingual log | 参数验证成功的双语日志
        LOG_INFO_ZH << "参数验证成功，继续执行流水线";
        LOG_INFO_EN << "Parameter validation successful, continuing pipeline execution";

        // Print parameter summary | 打印参数摘要
        params_.PrintSummary(this);

        // Parse compared pipeline configuration | 解析对比流水线配置
        ParseComparedPipelines();

        // If unified table feature is enabled, prepare to collect dataset names
        // 如果启用了统一制表功能，准备收集数据集名称
        std::vector<std::string> processed_dataset_names;
        if (params_.base.enable_summary_table)
        {
            // Use bilingual logs to show unified table feature status
            // 使用双语日志显示统一制表功能状态
            LOG_INFO_ZH << "统一制表功能已启用，将收集数据集名称用于汇总";
            LOG_INFO_EN << "Summary table feature enabled, will collect dataset names for aggregation";
        }

        // Prepare camera model data | 准备相机模型数据
        camera_model_data_ = PrepareCameraModel();
        if (!camera_model_data_)
        {
            LOG_ERROR_ZH << "无法准备相机模型数据";
            LOG_ERROR_EN << "Unable to prepare camera model data";
            return nullptr;
        }

        // Process dataset configuration logic | 处理数据集配置逻辑
        std::vector<std::string> dataset_list;
        if (!params_.base.image_folder.empty())
        {
            // If image_folder is specified, process only that dataset
            // 如果指定了image_folder，只处理该数据集
            dataset_list.push_back(params_.base.image_folder);
            LOG_INFO_ZH << "处理单个数据集: " << params_.base.image_folder;
            LOG_INFO_EN << "Processing single dataset: " << params_.base.image_folder;
        }
        else if (!params_.base.dataset_dir.empty())
        {
            // If only dataset_dir is specified, scan all datasets within it
            // 如果只指定了dataset_dir，扫描其中的所有数据集
            dataset_list = ScanDatasetDirectory(params_.base.dataset_dir);
            if (dataset_list.empty())
            {
                LOG_ERROR_ZH << "在dataset_dir中未找到任何数据集: " << params_.base.dataset_dir;
                LOG_ERROR_EN << "No datasets found in dataset_dir: " << params_.base.dataset_dir;
                return nullptr;
            }
            LOG_INFO_ZH << "找到 " << dataset_list.size() << " 个数据集进行批处理";
            LOG_INFO_EN << "Found " << dataset_list.size() << " datasets for batch processing";
        }
        else
        {
            LOG_ERROR_ZH << "必须指定dataset_dir或image_folder";
            LOG_ERROR_EN << "Must specify dataset_dir or image_folder";
            return nullptr;
        }

        // Batch process all datasets | 批处理所有数据集
        Interface::DataPtr final_result = nullptr;

        for (size_t dataset_idx = 0; dataset_idx < dataset_list.size(); ++dataset_idx)
        {
            const auto &current_image_folder = dataset_list[dataset_idx];

            LOG_INFO_ZH << "=== 开始处理数据集 " << (dataset_idx + 1) << "/" << dataset_list.size()
                        << ": " << current_image_folder << " ===";
            LOG_INFO_EN << "=== Starting dataset processing " << (dataset_idx + 1) << "/" << dataset_list.size()
                        << ": " << current_image_folder << " ===";

            // Update current dataset's image_folder | 更新当前数据集的image_folder
            params_.base.image_folder = current_image_folder;

            // Update dynamic parameters based on dataset name | 根据数据集名称更新动态参数
            std::string dataset_name = ExtractDatasetName(params_.base.image_folder);
            current_dataset_name_ = dataset_name; // Set current dataset name for visualization output
            params_.UpdateDynamicParameters(dataset_name);

            // Clear ground truth data from previous dataset, reload for current dataset
            // 清空之前数据集的真值数据，为当前数据集重新加载
            ClearGroundTruthData();

            // Clear historical evaluation data in EvaluatorManager to ensure evaluation data independence for each dataset
            // 清空EvaluatorManager中的历史评估数据，确保每个数据集的评估数据独立
            Interface::EvaluatorManager::ClearAllEvaluators();
            LOG_DEBUG_ZH << "已清空EvaluatorManager历史数据，确保数据集 [" << dataset_name << "] 评估数据独立";
            LOG_DEBUG_EN << "Cleared EvaluatorManager historical data to ensure evaluation data independence for dataset [" << dataset_name << "]";

            // Note: Dataset time statistics are now managed by Profiler system | 注意：数据集时间统计现在由Profiler系统管理
            LOG_DEBUG_ZH << "开始记录数据集 [" << dataset_name << "] 的执行时间";
            LOG_DEBUG_EN << "Starting to record execution time for dataset [" << dataset_name << "]";

            // Create independent working directory structure for current dataset
            // 为当前数据集创建独立的工作目录结构
            std::string dataset_work_dir = params_.base.work_dir + "/" + dataset_name;

            // Clean current dataset's working directory (ensure work_dir is clean)
            // 清理当前数据集的工作目录（保证work_dir的干净）
            if (std::filesystem::exists(dataset_work_dir))
            {
                try
                {
                    std::filesystem::remove_all(dataset_work_dir);
                    LOG_INFO_ZH << "清理数据集工作目录: " << dataset_work_dir;
                    LOG_INFO_EN << "Cleaning dataset working directory: " << dataset_work_dir;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "清理数据集工作目录失败 " << dataset_work_dir << ": " << e.what();
                    LOG_ERROR_EN << "Failed to clean dataset working directory " << dataset_work_dir << ": " << e.what();
                }
            }

            std::vector<std::string> dataset_cache_dirs = {
                dataset_work_dir + "/features",
                dataset_work_dir + "/matches",
                dataset_work_dir + "/poses",
                dataset_work_dir + "/reconstruction",
                dataset_work_dir + "/visualizeMatches"};

            // Update cache_directories configuration to dataset-specific directories
            // 更新cache_directories配置为数据集特定目录
            params_.base.cache_directories = {
                dataset_work_dir + "/features",
                dataset_work_dir + "/matches",
                dataset_work_dir + "/poses"};

            LOG_INFO_ZH << "更新缓存目录配置为数据集特定目录: " << dataset_name;
            LOG_INFO_EN << "Updated cache directory configuration for dataset-specific directories: " << dataset_name;

            // Create dataset-specific cache directories | 创建数据集特定的缓存目录
            for (const auto &cache_dir : dataset_cache_dirs)
            {
                try
                {
                    std::filesystem::create_directories(cache_dir);
                    LOG_DEBUG_ZH << "创建缓存目录: " << cache_dir;
                    LOG_DEBUG_EN << "Created cache directory: " << cache_dir;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "无法创建缓存目录 " << cache_dir << ": " << e.what();
                    LOG_ERROR_EN << "Unable to create cache directory " << cache_dir << ": " << e.what();
                    continue; // Continue processing next dataset
                }
            }

            // Load ground truth data (if evaluation is enabled) | 加载真值数据（如果启用评估）
            if (params_.base.enable_evaluation && !params_.base.gt_folder.empty())
            {
                if (!LoadGroundTruthData())
                {
                    LOG_ERROR_ZH << "真值数据加载失败，但继续执行流水线";
                    LOG_ERROR_EN << "Ground truth data loading failed, but continuing pipeline execution";
                }
            }

            // Initialize data statistics (if enabled) | 初始化数据统计（如果启用）
            if (params_.base.enable_data_statistics)
            {
                InitializeDataStatistics(dataset_name);
            }

            // Record dataset start time (for total time calculation) | 记录数据集开始时间（用于总时间计算）
            dataset_start_time_ = std::chrono::high_resolution_clock::now();

            try
            {
                LOG_INFO_ZH << "开始执行GlobalSfMPipeline流水线 [" << dataset_name << "]...";
                LOG_INFO_EN << "Starting GlobalSfMPipeline execution [" << dataset_name << "]...";

                // Add dataset name to processing list (for unified table) | 添加数据集名称到处理列表（用于统一制表）
                if (params_.base.enable_summary_table)
                {
                    processed_dataset_names.push_back(dataset_name);
                    LOG_DEBUG_ZH << "已添加数据集 [" << dataset_name << "] 到汇总列表";
                    LOG_DEBUG_EN << "Added dataset [" << dataset_name << "] to summary list";
                }

                final_result = RunPoSDKPipeline();
                RunComparedPipelinesIfNeeded();

                // Finalize data statistics (if enabled) | 完成数据统计（如果启用）
                if (params_.base.enable_data_statistics)
                {
                    FinalizeDataStatistics();
                }
                PrintRelativePosesAccuracy();
                PrintGlobalPosesAccuracy();

                LOG_INFO_ZH << "=== 数据集 [" << dataset_name << "] 处理完成 ===";
                LOG_INFO_EN << "=== Dataset [" << dataset_name << "] processing completed ===";
            }
            catch (const std::exception &e)
            {
                // Note: PROFILER_END will be called automatically when _profiler_session_ goes out of scope | 注意：当_profiler_session_离开作用域时会自动调用PROFILER_END

                LOG_ERROR_ZH << "数据集 [" << dataset_name << "] 处理过程中发生异常: " << e.what();
                LOG_ERROR_EN << "Exception occurred during dataset [" << dataset_name << "] processing: " << e.what();

                // Finalize data statistics even if exception occurs (if enabled) | 即使异常也要完成数据统计（如果启用）
                if (params_.base.enable_data_statistics)
                {
                    FinalizeDataStatistics();
                }

                LOG_INFO_ZH << "继续处理下一个数据集...";
                LOG_INFO_EN << "Continuing to process next dataset...";

                // Don't return nullptr, continue processing next dataset | 不返回nullptr，继续处理下一个数据集
            }
        }

        // Batch processing complete | 批处理完成
        if (dataset_list.size() > 1)
        {
            LOG_INFO_ZH << "=== 批处理完成，共处理 " << dataset_list.size() << " 个数据集 ===";
            LOG_INFO_EN << "=== Batch processing completed, total " << dataset_list.size() << " datasets processed ===";
        }

        // If unified table feature is enabled and datasets were processed, generate summary table
        // 如果启用了统一制表功能且处理了数据集，生成汇总表格
        if (params_.base.enable_summary_table && !processed_dataset_names.empty())
        {
            LOG_INFO_ZH << "=== 生成统一汇总表格 (基于CSV文件合并) ===";
            LOG_INFO_EN << "=== Generating unified summary table (based on CSV file merging) ===";
            bool summary_success = GenerateSummaryTable(processed_dataset_names);
            if (summary_success)
            {
                LOG_INFO_ZH << "✓ 统一汇总表格生成成功";
                LOG_INFO_EN << "✓ Unified summary table generation successful";
            }
            else
            {
                LOG_ERROR_ZH << "✗ 统一汇总表格生成失败";
                LOG_ERROR_EN << "✗ Unified summary table generation failed";
            }
        }
        else if (params_.base.enable_summary_table && processed_dataset_names.empty())
        {
            LOG_WARNING_ZH << "统一制表功能已启用，但没有处理任何数据集";
            LOG_WARNING_EN << "Unified table feature enabled, but no datasets were processed";
        }

        // Complete total time statistics (for logging in batch mode) | 完成总时间统计（批处理模式下用于日志）
        auto total_end_time = std::chrono::high_resolution_clock::now();
        total_pipeline_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(total_end_time - total_start_time).count();

        LOG_INFO_ZH << "=== 所有数据集处理完成 ===";
        LOG_INFO_ZH << "总流水线执行时间: " << total_pipeline_time_ << " ms";
        LOG_INFO_EN << "=== All datasets processing completed ===";
        LOG_INFO_EN << "Total pipeline execution time: " << total_pipeline_time_ << " ms";

        return final_result;
    }

    // Load ground truth data from specified folder | 从指定文件夹加载真值数据
    bool GlobalSfMPipeline::LoadGroundTruthData()
    {
        if (params_.base.gt_folder.empty())
        {
            LOG_ERROR_ZH << "gt_folder未设置";
            LOG_ERROR_EN << "gt_folder not set";
            return false;
        }

        // Load ground truth global pose data | 加载真值全局位姿数据
        if (!LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
        {
            LOG_ERROR_ZH << "无法加载真值全局位姿数据从: " << params_.base.gt_folder;
            LOG_ERROR_EN << "Unable to load ground truth global pose data from: " << params_.base.gt_folder;
            return false;
        }

        // Calculate relative pose ground truth from global poses | 从全局位姿计算相对位姿真值
        if (!types::Global2RelativePoses(gt_global_poses_, gt_relative_poses_))
        {
            LOG_ERROR_ZH << "无法从全局位姿计算相对位姿真值";
            LOG_ERROR_EN << "Unable to calculate relative pose ground truth from global poses";
            return false;
        }

        LOG_INFO_ZH << "成功从 " << params_.base.gt_folder << " 加载了 " << gt_global_poses_.GetRotations().size() << " 个真值相机位姿";
        LOG_INFO_ZH << "成功加载真值数据: " << gt_global_poses_.GetRotations().size() << " 个全局位姿, " << gt_relative_poses_.size() << " 个相对位姿";
        LOG_INFO_EN << "Successfully loaded " << gt_global_poses_.GetRotations().size() << " ground truth camera poses from " << params_.base.gt_folder;
        LOG_INFO_EN << "Successfully loaded ground truth data: " << gt_global_poses_.GetRotations().size() << " global poses, " << gt_relative_poses_.size() << " relative poses";

        // Create DataPackage containing both global and relative pose ground truth data
        // 创建包含全局位姿和相对位姿真值数据的DataPackage
        auto gt_data_package = std::make_shared<DataPackage>();

        // Add global pose ground truth data | 添加全局位姿真值数据
        auto gt_global_poses_datamap = std::make_shared<DataMap<GlobalPoses>>(gt_global_poses_, "data_global_poses");
        DataPtr gt_global_poses_data = std::static_pointer_cast<DataIO>(gt_global_poses_datamap);
        gt_data_package->AddData("data_global_poses", gt_global_poses_data);

        // Add relative pose ground truth data | 添加相对位姿真值数据
        auto gt_relative_poses_datamap = std::make_shared<DataMap<RelativePoses>>(gt_relative_poses_, "data_relative_poses");
        DataPtr gt_relative_poses_data = std::static_pointer_cast<DataIO>(gt_relative_poses_datamap);
        gt_data_package->AddData("data_relative_poses", gt_relative_poses_data);

        // Set ground truth data package to GlobalSfMPipeline to support automatic evaluation
        // 将真值数据包设置给GlobalSfMPipeline以支持自动评估
        DataPtr gt_package_data = std::static_pointer_cast<DataIO>(gt_data_package);
        SetGTData(gt_package_data);
        LOG_INFO_ZH << "真值数据包已设置给GlobalSfMPipeline用于自动评估，包含全局位姿和相对位姿数据";
        LOG_INFO_EN << "Ground truth data package has been set to GlobalSfMPipeline for automatic evaluation, containing global and relative pose data";

        return true;
    }

    // Clear ground truth data to prepare for new dataset | 清空真值数据，为新数据集做准备
    void GlobalSfMPipeline::ClearGroundTruthData()
    {
        // Clear ground truth data to prepare for new dataset | 清空真值数据，为新数据集做准备
        gt_global_poses_.GetRotations().clear();
        gt_global_poses_.GetTranslations().clear();

        // Clear all data in EstInfo by initializing with 0 views | 通过初始化为0个视图来清空EstInfo的所有数据
        gt_global_poses_.GetEstInfo().Init(0);

        gt_relative_poses_.clear();

        // Clear GT data settings in method | 清空method的GT数据设置
        ResetPriorInfo();

        LOG_INFO_ZH << "已清空真值数据缓存，准备加载新数据集的真值";
        LOG_INFO_EN << "Ground truth data cache cleared, ready to load ground truth for new dataset";
    }

    // Step 1: Image preprocessing | 步骤1：图像预处理
    DataPtr GlobalSfMPipeline::Step1_ImagePreprocessing()
    {

        LOG_INFO_ZH << "使用预处理类型: " << GetPreprocessTypeStr();
        LOG_INFO_EN << "Using preprocessing type: " << GetPreprocessTypeStr();

        // Execute main preprocessing according to selected preprocessing type | 根据选择的预处理类型执行主预处理
        switch (params_.base.preprocess_type)
        {
        case PreprocessType::OpenMVG:
            return RunOpenMVGPipeline();
        case PreprocessType::OpenCV:
            // OpenCV preprocessing uses PoSDK method with method_img2matches | OpenCV预处理使用PoSDK方法但采用method_img2matches
            return RunPoSDKPreprocess();
        case PreprocessType::PoSDK:
            // PoSDK preprocessing uses optimized posdk_preprocessor | PoSDK预处理使用优化的posdk_preprocessor
            return RunPoSDKPreprocess();
        default:
            LOG_ERROR_ZH << "未知的预处理类型";
            LOG_ERROR_EN << "Unknown preprocessing type";
            return nullptr;
        }
    }

    // Run OpenMVG pipeline for preprocessing | 运行OpenMVG流水线进行预处理
    DataPtr GlobalSfMPipeline::RunOpenMVGPipeline()
    {
        LOG_INFO_ZH << "=== 使用OpenMVG进行预处理 ===";
        LOG_INFO_EN << "=== Using OpenMVG for preprocessing ===";

        // Create OpenMVGPipeline | 创建OpenMVGPipeline
        openmvg_pipeline_ = CreateAndConfigureSubMethod("openmvg_pipeline");
        if (!openmvg_pipeline_)
        {
            return nullptr;
        }

        // First create and load image data | 首先创建并加载图像数据
        auto images_data = FactoryData::Create("data_images");
        if (!images_data)
        {
            LOG_ERROR_ZH << "无法创建data_images";
            LOG_ERROR_EN << "Unable to create data_images";
            return nullptr;
        }

        // Load image paths | 加载图像路径
        if (!images_data->Load(params_.base.image_folder))
        {
            LOG_ERROR_ZH << "无法从路径加载图像: " << params_.base.image_folder;
            LOG_ERROR_EN << "Unable to load images from path: " << params_.base.image_folder;
            return nullptr;
        }

        // Ensure path parameters are correctly set - directly use main configuration parameters
        // 确保路径参数正确设置 - 直接使用主配置参数
        LOG_INFO_ZH << "OpenMVG预处理配置: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;
        LOG_INFO_EN << "OpenMVG preprocessing configuration: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;

        // Parameter passing mechanism explanation: | 参数传递机制说明：
        // 1. Static configuration parameters: automatically passed from [openmvg_pipeline] section via PassingMethodOptions
        // 1. 静态配置参数：通过PassingMethodOptions自动从[openmvg_pipeline]section传递
        // 2. Dynamic path parameters: managed uniformly by main configuration [globalsfm_pipeline], passed dynamically at runtime via SetMethodOptions
        // 2. 动态路径参数：由主配置[globalsfm_pipeline]统一管理，运行时通过SetMethodOptions传递
        // 3. Format conversion parameters: parameters requiring special handling (such as intrinsic format conversion)
        // 3. 格式转换参数：需要特殊处理的参数（如内参格式转换）

        // Pass dynamic path parameters: these parameters are managed uniformly by main configuration and set dynamically at runtime
        // 传递动态路径参数：这些参数由主配置统一管理，运行时动态设置
        // Use dataset-specific work_dir for OpenMVG preprocessor to avoid multi-dataset conflicts
        // 为OpenMVG预处理器使用数据集特定的work_dir，避免多数据集冲突
        std::string dataset_specific_work_dir = params_.base.work_dir + "/" + current_dataset_name_;

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},       // Directly use main configuration parameters | 直接使用主配置参数
            {"dataset_dir", params_.base.dataset_dir},    // Directly use main configuration parameters | 直接使用主配置参数
            {"images_folder", params_.base.image_folder}, // Directly use main configuration parameters | 直接使用主配置参数
            {"work_dir", dataset_specific_work_dir}       // Use dataset-specific work_dir | 使用数据集特定的work_dir
        };

        LOG_INFO_ZH << "OpenMVG预处理器使用数据集特定work_dir: " << dataset_specific_work_dir;
        LOG_INFO_EN << "OpenMVG preprocessor using dataset-specific work_dir: " << dataset_specific_work_dir;

        // First pass dynamic path parameters | 先传递动态路径参数
        openmvg_pipeline_->SetMethodOptions(dynamic_options);

        // Then handle intrinsic format conversion: convert from comma-separated to semicolon-separated (required by OpenMVG, because ; is comment symbol in ini files)
        // 然后处理内参格式转换：从逗号分隔转换为分号分隔（OpenMVG要求，因为ini文件中;是注释符）
        std::string intrinsics_semicolon = params_.openmvg.intrinsics;
        std::replace(intrinsics_semicolon.begin(), intrinsics_semicolon.end(), ',', ';');

        // Format conversion parameters | 格式转换参数
        MethodOptions format_options = {
            {"intrinsics", intrinsics_semicolon} // Pass after format conversion | 格式转换后传递
        };

        // If comparison pipeline feature is enabled, force enable OpenMVG SfM reconstruction
        // 如果启用了对比流水线功能，强制开启OpenMVG的SfM重建
        if (is_compared_openmvg_)
        {
            format_options["enable_sfm_reconstruction"] = "true";
            LOG_DEBUG_ZH << "检测到需要对比OpenMVG，已启用OpenMVG的SfM重建功能";
            LOG_DEBUG_EN << "Detected need to compare OpenMVG, OpenMVG SfM reconstruction feature enabled";
        }

        // If main preprocessor is OpenMVG, also need to enable SfM reconstruction to support evaluation
        // 如果主预处理器就是OpenMVG，也需要启用SfM重建以支持评估
        if (params_.base.preprocess_type == PreprocessType::OpenMVG)
        {
            format_options["enable_sfm_reconstruction"] = "true";
            LOG_DEBUG_ZH << "主预处理器为OpenMVG，已启用SfM重建功能";
            LOG_DEBUG_EN << "Main preprocessor is OpenMVG, SfM reconstruction feature enabled";
        }

        // Finally pass format-converted parameters (override parameters loaded by PassingMethodOptions)
        // 最后传递格式转换后的参数（覆盖PassingMethodOptions加载的参数）
        openmvg_pipeline_->SetMethodOptions(format_options);

        // Set image data as input | 设置图像数据作为输入
        openmvg_pipeline_->SetRequiredData(images_data);

        // Execute preprocessing | 执行预处理

        auto preprocess_result = openmvg_pipeline_->Build();
        if (!preprocess_result)
        {
            LOG_ERROR_ZH << "OpenMVG预处理失败";
            LOG_ERROR_EN << "OpenMVG preprocessing failed";
            return nullptr;
        }

        // If OpenMVG comparison pipeline is enabled or main preprocessor is OpenMVG, perform OpenMVG global pose evaluation
        // 如果启用了OpenMVG对比流水线或主预处理器是OpenMVG，进行OpenMVG全局位姿评估
        if (is_compared_openmvg_ || params_.base.preprocess_type == PreprocessType::OpenMVG)
        {
            LOG_DEBUG_ZH << "执行OpenMVG全局位姿评估...";
            LOG_DEBUG_EN << "Executing OpenMVG global pose evaluation...";
            EvaluateOpenMVGGlobalPoses(current_dataset_name_);
        }

        // Try to load and evaluate OpenMVG relative poses if export_relative_poses_file is configured
        // 如果配置了export_relative_poses_file，尝试加载并评估OpenMVG相对位姿
        std::string export_relative_poses_file = openmvg_pipeline_->GetOptionAsPath("export_relative_poses_file", "");
        if (!export_relative_poses_file.empty())
        {
            LOG_INFO_ZH << "=== OpenMVG相对位姿评估 ===";
            LOG_INFO_EN << "=== OpenMVG Relative Pose Evaluation ===";
            LOG_INFO_ZH << "从OpenMVG导出的相对位姿文件读取数据: " << export_relative_poses_file;
            LOG_INFO_EN << "Loading relative poses from OpenMVG exported file: " << export_relative_poses_file;

            // Check if the relative poses file exists | 检查相对位姿文件是否存在
            if (!std::filesystem::exists(export_relative_poses_file))
            {
                LOG_WARNING_ZH << "OpenMVG相对位姿文件不存在: " << export_relative_poses_file;
                LOG_WARNING_EN << "OpenMVG relative poses file does not exist: " << export_relative_poses_file;
            }
            else
            {
                // Create data_relative_poses to hold the loaded data | 创建data_relative_poses来保存加载的数据
                auto openmvg_relative_poses_data = FactoryData::Create("data_relative_poses");
                if (!openmvg_relative_poses_data)
                {
                    LOG_ERROR_ZH << "无法创建data_relative_poses数据对象";
                    LOG_ERROR_EN << "Cannot create data_relative_poses data object";
                }
                else
                {
                    // Load relative poses from G2O file (reference test_Strecha.cpp implementation)
                    // 从G2O文件加载相对位姿（参考test_Strecha.cpp实现）
                    if (!openmvg_relative_poses_data->Load(export_relative_poses_file, "g2o"))
                    {
                        LOG_ERROR_ZH << "无法从G2O文件加载OpenMVG相对位姿数据: " << export_relative_poses_file;
                        LOG_ERROR_EN << "Cannot load OpenMVG relative poses from G2O file: " << export_relative_poses_file;
                    }
                    else
                    {
                        // Verify loaded data | 验证加载的数据
                        auto openmvg_relative_poses_ptr = GetDataPtr<types::RelativePoses>(openmvg_relative_poses_data);
                        if (!openmvg_relative_poses_ptr || openmvg_relative_poses_ptr->empty())
                        {
                            LOG_WARNING_ZH << "从文件加载的OpenMVG相对位姿数据为空";
                            LOG_WARNING_EN << "Loaded OpenMVG relative poses data is empty";
                        }
                        else
                        {
                            LOG_INFO_ZH << "成功加载 " << openmvg_relative_poses_ptr->size() << " 个OpenMVG相对位姿";
                            LOG_INFO_EN << "Successfully loaded " << openmvg_relative_poses_ptr->size() << " OpenMVG relative poses";

                            // Set evaluator algorithm name to distinguish from PoSDK relative poses
                            // 设置评估器算法名称以区分PoSDK相对位姿
                            std::string original_algorithm = GetEvaluatorAlgorithm();
                            SetEvaluatorAlgorithm("openmvg_pipeline");

                            // Perform automatic evaluation using CallEvaluator | 使用CallEvaluator进行自动评估
                            if (GetGTData())
                            {
                                LOG_INFO_ZH << "开始执行OpenMVG相对位姿自动评估...";
                                LOG_INFO_EN << "Starting OpenMVG relative pose automatic evaluation...";

                                bool evaluation_success = CallEvaluator(openmvg_relative_poses_data);
                                if (evaluation_success)
                                {
                                    LOG_INFO_ZH << "OpenMVG相对位姿自动评估完成，结果已添加到EvaluatorManager";
                                    LOG_INFO_EN << "OpenMVG relative pose automatic evaluation completed, results added to EvaluatorManager";
                                }
                                else
                                {
                                    LOG_ERROR_ZH << "OpenMVG相对位姿自动评估失败";
                                    LOG_ERROR_EN << "OpenMVG relative pose automatic evaluation failed";
                                }
                            }
                            else
                            {
                                LOG_WARNING_ZH << "真值数据未设置，无法进行OpenMVG相对位姿自动评估";
                                LOG_WARNING_EN << "Ground truth data not set, cannot perform OpenMVG relative pose automatic evaluation";
                            }

                            // Restore original algorithm name | 恢复原始算法名称
                            SetEvaluatorAlgorithm(original_algorithm);
                        }
                    }
                }
            }
        }

        // Preprocessing completed, Strecha validation already completed above | 预处理完成，Strecha验证已在前面完成
        return preprocess_result;
    }

    DataPtr GlobalSfMPipeline::RunPoSDKPipeline()
    {
        // 首先运行对比流水线（如果设置了compared_pipelines且不是主预处理器）
        SetProfilerLabels({{"pipeline", "PoSDK"}, {"dataset", current_dataset_name_}});
        try
        {
            LOG_INFO_ZH << "开始执行GlobalSfMPipeline流水线 [" << current_dataset_name_ << "]...";
            LOG_INFO_EN << "Starting GlobalSfMPipeline execution [" << current_dataset_name_ << "]...";

            SetProfilerLabels({{"pipeline", "PoSDK"}, {"dataset", current_dataset_name_}});
            // Start profiling for current dataset processing (after comparison pipelines) | 开始对当前数据集处理进行性能分析（在对比流水线之后）

            // Step 1: Image preprocessing and feature extraction | 步骤1: 图像预处理和特征提取
            auto preprocess_result = Step1_ImagePreprocessing();
            if (!preprocess_result)
            {
                LOG_ERROR_ZH << "图像预处理失败";
                LOG_ERROR_EN << "Image preprocessing failed";
                return nullptr;
            }

            // Note: Step 1 core time is now managed by Profiler system | 注意：步骤1的核心时间现在由Profiler系统管理

            // Add Step 1 data statistics | 添加步骤1数据统计
            if (params_.base.enable_data_statistics)
            {
                AddStepDataStatistics("Step1_ImagePreprocessing", preprocess_result,
                                      Interface::LanguageEnvironment::GetText(
                                          "图像预处理和特征提取：包含特征点提取、描述子计算和特征匹配",
                                          "Image preprocessing and feature extraction: includes feature point extraction, descriptor computation and feature matching"));
            }

            // Print feature information (if enabled) | 打印特征信息（如果启用）
            if (params_.base.enable_features_info_print)
            {
                LOG_INFO_ZH << "预处理后特征信息打印已启用，开始输出特征详细信息...";
                LOG_INFO_EN << "Post-processing feature information printing enabled, starting to output detailed feature information...";

                // Reference Step2_TwoViewEstimation data acquisition method
                // 参考Step2_TwoViewEstimation的数据获取方式
                auto data_package = std::dynamic_pointer_cast<DataPackage>(preprocess_result);
                if (data_package)
                {
                    auto features_data = data_package->GetData("data_features");
                    auto matches_data = data_package->GetData("data_matches");

                    if (features_data)
                    {
                        auto features_info = GetDataPtr<FeaturesInfo>(features_data);
                        if (features_info)
                        {
                            features_info->Print(false); // Default not to print unused images
                        }
                        else
                        {
                            LOG_WARNING_ZH << "未能从features_data中获取FeaturesInfo，跳过特征信息打印";
                            LOG_WARNING_EN << "Unable to get FeaturesInfo from features_data, skipping feature information printing";
                        }
                    }
                    else
                    {
                        LOG_WARNING_ZH << "未能从预处理结果中获取data_features，跳过特征信息打印";
                        LOG_WARNING_EN << "Unable to get data_features from preprocessing result, skipping feature information printing";
                    }

                    // Print ViewPair matches information | 打印ViewPair匹配信息
                    if (matches_data)
                    {
                        auto matches_info = GetDataPtr<Matches>(matches_data);
                        if (matches_info)
                        {
                            LOG_INFO_ZH << "开始输出ViewPair匹配详细信息...";
                            LOG_INFO_EN << "Starting to output ViewPair matches detailed information...";

                            LOG_INFO_ZH << "总ViewPair数量: " << matches_info->size();
                            LOG_INFO_EN << "Total ViewPair count: " << matches_info->size();

                            for (const auto &[view_pair, id_matches] : *matches_info)
                            {
                                LOG_INFO_ZH << "ViewPair (" << view_pair.first << ", " << view_pair.second
                                            << ") - 匹配数量: " << id_matches.size();
                                LOG_INFO_EN << "ViewPair (" << view_pair.first << ", " << view_pair.second
                                            << ") - Match count: " << id_matches.size();
                            }
                        }
                        else
                        {
                            LOG_WARNING_ZH << "未能从matches_data中获取Matches，跳过匹配信息打印";
                            LOG_WARNING_EN << "Unable to get Matches from matches_data, skipping matches information printing";
                        }
                    }
                    else
                    {
                        LOG_WARNING_ZH << "未能从预处理结果中获取data_matches，跳过匹配信息打印";
                        LOG_WARNING_EN << "Unable to get data_matches from preprocessing result, skipping matches information printing";
                    }
                }
                else
                {
                    LOG_WARNING_ZH << "预处理结果不是DataPackage类型，跳过特征信息打印";
                    LOG_WARNING_EN << "Preprocessing result is not DataPackage type, skipping feature information printing";
                }
            }

            // Step 2: Two-view pose estimation | 步骤2: 双视图位姿估计
            LOG_INFO_ZH << "=== 步骤2: 双视图位姿估计 ===";
            LOG_INFO_EN << "=== Step 2: Two-view pose estimation ===";
            auto relative_poses_result = Step2_TwoViewEstimation(preprocess_result);
            if (!relative_poses_result)
            {
                LOG_ERROR_ZH << "双视图位姿估计失败";
                LOG_ERROR_EN << "Two-view pose estimation failed";
                return nullptr;
            }

            // Note: Step 2 core time is now managed by Profiler system | 注意：步骤2的核心时间现在由Profiler系统管理

            // Add Step 2 data statistics | 添加步骤2数据统计
            if (params_.base.enable_data_statistics)
            {
                AddStepDataStatistics("Step2_TwoViewEstimation", relative_poses_result,
                                      Interface::LanguageEnvironment::GetText(
                                          "双视图位姿估计：通过特征匹配计算相机间的相对位姿关系",
                                          "Two-view pose estimation: computing relative pose relationships between cameras through feature matching"));
            }

            // If manual evaluation is enabled, perform manual evaluation immediately (for verifying automatic evaluation results)
            // 如果启用了手动评估，立即进行手动评估（用于验证自动评估结果的正确性）
            if (params_.base.enable_manual_eval)
            {
                PerformManualRelativePoseEvaluation(relative_poses_result, current_dataset_name_);
            }

            // Evaluate relative pose accuracy | 评估相对位姿精度
            EvaluatePoseAccuracy(relative_poses_result, "relative");

            // Step 2.5 (Optional): Rotation refinement using color-based block matching | 步骤2.5（可选）: 基于颜色块匹配的旋转优化
            bool enable_rotation_refine = GetOptionAsBool("enable_rotation_refine", false);
            if (enable_rotation_refine)
            {
                LOG_INFO_ZH << "=== 步骤2.5: 旋转优化（基于颜色块匹配）===";
                LOG_INFO_EN << "=== Step 2.5: Rotation refinement (color-based block matching) ===";

                auto refined_poses_result = Step2_5_RotationRefinement(relative_poses_result, preprocess_result);
                if (refined_poses_result)
                {
                    relative_poses_result = refined_poses_result;
                    LOG_INFO_ZH << "旋转优化完成";
                    LOG_INFO_EN << "Rotation refinement completed";

                    // Add Step 2.5 data statistics | 添加步骤2.5数据统计
                    if (params_.base.enable_data_statistics)
                    {
                        AddStepDataStatistics("Step2_5_RotationRefinement", relative_poses_result,
                                              Interface::LanguageEnvironment::GetText(
                                                  "旋转优化：使用颜色块匹配优化相对旋转",
                                                  "Rotation refinement: refining relative rotations using color-based block matching"));
                    }
                }
                else
                {
                    LOG_WARNING_ZH << "旋转优化失败，继续使用原始相对位姿";
                    LOG_WARNING_EN << "Rotation refinement failed, continuing with original relative poses";
                }
            }
            else
            {
                LOG_INFO_ZH << "跳过旋转优化步骤（enable_rotation_refine=false）";
                LOG_INFO_EN << "Skipping rotation refinement step (enable_rotation_refine=false)";
            }

            // Extract data from preprocessing result | 从预处理结果中提取数据
            auto data_package = std::dynamic_pointer_cast<DataPackage>(preprocess_result);
            auto camera_models = data_package->GetData("data_camera_models");
            auto matches_data = data_package->GetData("data_matches");
            auto features_data = data_package->GetData("data_features");
            auto images_data = data_package->GetData("data_images");

            // If no camera model data, create default Strecha camera model
            // 如果没有相机模型数据，创建默认的Strecha相机模型
            if (!camera_models)
            {
                camera_models = CreateStrechaCameraModel();
                if (!camera_models)
                {
                    LOG_ERROR_ZH << "无法创建相机模型数据用于旋转平均";
                    LOG_ERROR_EN << "Unable to create camera model data for rotation averaging";
                    return nullptr;
                }
            }

            // Step 3: Rotation averaging | 步骤3: 旋转平均
            LOG_INFO_ZH << "=== 步骤3: 旋转平均 ===";
            LOG_INFO_EN << "=== Step 3: Rotation averaging ===";
            auto rotation_result = Step3_RotationAveraging(relative_poses_result, camera_models);
            if (!rotation_result)
            {
                LOG_ERROR_ZH << "旋转平均失败";
                LOG_ERROR_EN << "Rotation averaging failed";
                return nullptr;
            }

            // Note: Step 3 core time is now managed by Profiler system | 注意：步骤3的核心时间现在由Profiler系统管理

            // Add Step 3 data statistics | 添加步骤3数据统计
            if (params_.base.enable_data_statistics)
            {
                AddStepDataStatistics("Step3_RotationAveraging", rotation_result,
                                      Interface::LanguageEnvironment::GetText(
                                          "旋转平均：从相对位姿估计全局旋转矩阵",
                                          "Rotation averaging: estimating global rotation matrices from relative poses"));
            }

            // Step 4: Feature track building | 步骤4: 特征轨迹构建
            LOG_INFO_ZH << "=== 步骤4: 特征轨迹构建 ===";
            LOG_INFO_EN << "=== Step 4: Feature track building ===";
            auto tracks_result = Step4_TrackBuilding(matches_data, features_data);
            auto initial_tracks_result = tracks_result->CopyData(); // Keep initial tracks for coordinate change comparison | 保存初始轨迹用于坐标变化比较
            if (!tracks_result)
            {
                LOG_ERROR_ZH << "特征轨迹构建失败";
                LOG_ERROR_EN << "Feature track building failed";
                return nullptr;
            }

            // Note: Step 4 core time is now managed by Profiler system | 注意：步骤4的核心时间现在由Profiler系统管理

            // Add Step 4 data statistics | 添加步骤4数据统计
            if (params_.base.enable_data_statistics)
            {
                AddStepDataStatistics("Step4_TrackBuilding", tracks_result,
                                      Interface::LanguageEnvironment::GetText(
                                          "特征轨迹构建：将多视图特征匹配关系构建为3D特征轨迹",
                                          "Feature track building: building multi-view feature correspondences into 3D feature tracks"));
            }

            // Step 5-7: 使用PoSDK Global SfM核心引擎 | Use PoSDK Global SfM Core Engine
            LOG_INFO_ZH << "=== 步骤5-7: PoSDK Global SfM核心引擎 ===";
            LOG_INFO_EN << "=== Step 5-7: PoSDK Global SfM Core Engine ===";

            // 创建并配置核心引擎 | Create and configure core engine
            auto global_sfm_engine = CreateAndConfigureSubMethod("PoGlobalSfMEngine");
            if (!global_sfm_engine)
            {
                LOG_ERROR_ZH << "无法创建PoGlobalSfMEngine";
                LOG_ERROR_EN << "Failed to create PoGlobalSfMEngine";
                return nullptr;
            }

            // 设置输入数据 | Set input data
            global_sfm_engine->SetRequiredData(tracks_result);   // data_tracks
            global_sfm_engine->SetRequiredData(rotation_result); // data_global_poses (initial from rotation averaging)
            global_sfm_engine->SetRequiredData(camera_models);   // data_camera_models

            // 执行核心引擎 | Execute core engine
            auto engine_result = global_sfm_engine->Build();
            if (!engine_result)
            {
                LOG_ERROR_ZH << "PoGlobalSfMEngine执行失败";
                LOG_ERROR_EN << "PoGlobalSfMEngine execution failed";
                return nullptr;
            }

            // 提取结果：优化后的位姿和3D点 | Extract results: optimized poses and 3D points
            auto engine_package = std::dynamic_pointer_cast<DataPackage>(engine_result);
            if (!engine_package)
            {
                LOG_ERROR_ZH << "PoGlobalSfMEngine输出不是DataPackage类型";
                LOG_ERROR_EN << "PoGlobalSfMEngine output is not DataPackage type";
                return nullptr;
            }

            auto final_global_poses = engine_package->GetData("data_global_poses");
            auto reconstruction_result = engine_package->GetData("data_points_3d");
            auto optimized_tracks = engine_package->GetData("data_tracks");

            if (!final_global_poses)
            {
                LOG_ERROR_ZH << "无法从引擎结果中提取data_global_poses";
                LOG_ERROR_EN << "Failed to extract data_global_poses from engine results";
                return nullptr;
            }

            // 更新tracks_result为引擎输出的优化后的轨迹（已还原为原始像素坐标）| Update tracks_result to optimized tracks from engine (coordinates converted back to original pixel coordinates)
            if (optimized_tracks)
            {
                tracks_result = optimized_tracks;
                LOG_INFO_ZH << "已更新tracks_result为PoGlobalSfMEngine输出的优化轨迹（坐标已还原为原始像素坐标）";
                LOG_INFO_EN << "Updated tracks_result to optimized tracks from PoGlobalSfMEngine (coordinates converted back to original pixel coordinates)";
            }
            else
            {
                LOG_WARNING_ZH << "PoGlobalSfMEngine未返回data_tracks，继续使用原始tracks_result";
                LOG_WARNING_EN << "PoGlobalSfMEngine did not return data_tracks, continuing with original tracks_result";
            }

            // Evaluate global pose accuracy | 评估全局位姿精度
            EvaluatePoseAccuracy(final_global_poses, "global");

            // Determine final output content based on configuration | 根据配置决定最终输出内容
            DataPtr dataset_final_result = nullptr;

            if (params_.base.enable_3d_points_output && reconstruction_result)
            {
                // Create DataPackage containing poses and 3D points | 创建包含位姿和3D点的DataPackage
                auto final_package = std::make_shared<DataPackage>();
                final_package->AddData(final_global_poses);
                final_package->AddData(reconstruction_result);
                dataset_final_result = final_package;

                LOG_INFO_ZH << "最终结果: 全局位姿 + 3D点重建";
                LOG_INFO_EN << "Final result: Global poses + 3D point reconstruction";

                // Export Meshlab project file (if enabled) | 导出Meshlab工程文件（如果启用）
                ExportMeshlabProject(final_global_poses, reconstruction_result, camera_models, images_data, current_dataset_name_);
            }
            else
            {
                // Return only global poses | 仅返回全局位姿
                dataset_final_result = final_global_poses;
                LOG_INFO_ZH << "最终结果: 仅全局位姿";
                LOG_INFO_EN << "Final result: Global poses only";

                // If meshlab export is enabled but no 3D points, log warning
                // 如果启用了meshlab导出但没有3D点，记录警告
                if (params_.base.enable_meshlab_export && !reconstruction_result)
                {
                    LOG_WARNING_ZH << "Meshlab导出已启用但无3D点数据，无法导出Meshlab工程文件";
                    LOG_WARNING_EN << "Meshlab export enabled but no 3D point data, cannot export Meshlab project file";
                }
            }

            // Export PoSDK2Colmap (if enabled) | 导出PoSDK2Colmap（如果启用）
            ExportPoSDK2Colmap(final_global_poses, camera_models, features_data, tracks_result, reconstruction_result, current_dataset_name_);

            // Save current dataset result as final result (result of last dataset) | 保存当前数据集的结果作为最终结果（最后一个数据集的结果）
            return dataset_final_result;
        }
        catch (const std::exception &e)
        {
            // Note: PROFILER_END will be called automatically when _profiler_session_ goes out of scope | 注意：当_profiler_session_离开作用域时会自动调用PROFILER_END

            LOG_ERROR_ZH << "数据集 [" << current_dataset_name_ << "] 处理过程中发生异常: " << e.what();
            LOG_ERROR_EN << "Exception occurred during dataset [" << current_dataset_name_ << "] processing: " << e.what();

            // Don't return nullptr, continue processing next dataset | 不返回nullptr，继续处理下一个数据集
            return nullptr;
        }
    }

    // Check and run comparison pipelines if needed | 检查并运行对比流水线（如果需要）
    void GlobalSfMPipeline::RunComparedPipelinesIfNeeded()
    {
        LOG_INFO_ALL << " ";

        LOG_INFO_ZH << "=== 检查对比流水线需求 ===";
        LOG_INFO_ZH << "  当前主预处理器: " << GetPreprocessTypeStr();
        LOG_INFO_ZH << "  对比流水线配置: " << params_.base.compared_pipelines;
        LOG_INFO_ZH << "  对比标志位: ";
        LOG_INFO_ZH << "    - OpenMVG: " << (is_compared_openmvg_ ? "是" : "否");
        LOG_INFO_ZH << "    - Colmap: " << (is_compared_colmap_ ? "是" : "否");
        LOG_INFO_ZH << "    - Glomap: " << (is_compared_glomap_ ? "是" : "否");

        LOG_INFO_EN << "=== Checking comparison pipeline requirements ===";
        LOG_INFO_EN << "  Current main preprocessor: " << GetPreprocessTypeStr();
        LOG_INFO_EN << "  Comparison pipeline configuration: " << params_.base.compared_pipelines;
        LOG_INFO_EN << "  Comparison flags: ";
        LOG_INFO_EN << "    - OpenMVG: " << (is_compared_openmvg_ ? "Yes" : "No");
        LOG_INFO_EN << "    - Colmap: " << (is_compared_colmap_ ? "Yes" : "No");
        LOG_INFO_EN << "    - Glomap: " << (is_compared_glomap_ ? "Yes" : "No");

        bool any_comparison_run = false;

        LOG_INFO_ALL << " ";
        // Run OpenMVG comparison (if needed and not the main preprocessor) | 运行OpenMVG对比（如果需要且不是主预处理器）
        if (is_compared_openmvg_ && params_.base.preprocess_type != PreprocessType::OpenMVG)
        {
            LOG_INFO_ZH << "→ 运行OpenMVG对比流水线";
            LOG_INFO_ZH << "  主预处理器: " << GetPreprocessTypeStr();
            LOG_INFO_EN << "→ Running OpenMVG comparison pipeline";
            RunOpenMVGForComparison();
            any_comparison_run = true;
        }
        else if (is_compared_openmvg_ && params_.base.preprocess_type == PreprocessType::OpenMVG)
        {
            LOG_INFO_ZH << "→ 主预处理器已是OpenMVG，将在主流水线中进行评估";
            LOG_INFO_EN << "→ Main preprocessor is already OpenMVG, evaluation will be performed in main pipeline";
        }

        // Run Colmap comparison (if needed) | 运行Colmap对比（如果需要）
        if (is_compared_colmap_)
        {
            LOG_INFO_ZH << "→ 运行Colmap对比流水线";
            LOG_INFO_ZH << "  主预处理器: " << GetPreprocessTypeStr();
            LOG_INFO_EN << "→ Running Colmap comparison pipeline";
            RunColmapForComparison();
            any_comparison_run = true;
        }

        // Run Glomap comparison (if needed) | 运行Glomap对比（如果需要）
        if (is_compared_glomap_)
        {
            LOG_INFO_ZH << "→ 运行Glomap对比流水线";
            LOG_INFO_ZH << "  主预处理器: " << GetPreprocessTypeStr();
            LOG_INFO_EN << "→ Running Glomap comparison pipeline";
            RunGlomapForComparison();
            any_comparison_run = true;
        }

        if (!any_comparison_run && !params_.base.compared_pipelines.empty())
        {
            LOG_INFO_ZH << "→ 所有对比流水线与主预处理器相同，无需额外运行";
            LOG_INFO_EN << "→ All comparison pipelines are same as main preprocessor, no additional runs needed";
        }
        else if (!any_comparison_run && params_.base.compared_pipelines.empty())
        {
            LOG_DEBUG_ZH << "→ 未配置对比流水线";
            LOG_DEBUG_EN << "→ No comparison pipelines configured";
        }

        LOG_INFO_ZH << "对比流水线检查完成";
        LOG_INFO_EN << "Comparison pipeline check completed";
    }

    // Run OpenMVG pipeline for comparison | 运行OpenMVG流水线进行对比
    void GlobalSfMPipeline::RunOpenMVGForComparison()
    {
        LOG_INFO_ALL << " ";
        LOG_INFO_ZH << "=== [对比运行] OpenMVG流水线 ===";
        LOG_INFO_EN << "=== [Comparison Run] OpenMVG Pipeline ===";

        // Create dedicated OpenMVGPipeline instance for comparison | 创建专门的OpenMVGPipeline实例用于对比
        auto openmvg_comparison = CreateAndConfigureSubMethod("openmvg_pipeline");
        if (!openmvg_comparison)
        {
            LOG_ERROR_ZH << "[对比运行] 无法创建OpenMVGPipeline实例";
            LOG_ERROR_EN << "[Comparison Run] Unable to create OpenMVGPipeline instance";
            return;
        }

        // Set profiler label for comparison pipeline | 为对比流水线设置性能分析标签
        openmvg_comparison->SetProfilerLabels({{"pipeline", "OpenMVG"}, {"dataset", current_dataset_name_}});

        // First create and load image data | 首先创建并加载图像数据
        auto images_data = FactoryData::Create("data_images");
        if (!images_data)
        {
            LOG_ERROR_ZH << "[对比运行] 无法创建data_images";
            LOG_ERROR_EN << "[Comparison Run] Unable to create data_images";
            return;
        }

        // Load image paths | 加载图像路径
        if (!images_data->Load(params_.base.image_folder))
        {
            LOG_ERROR_ZH << "[对比运行] 无法从路径加载图像: " << params_.base.image_folder;
            LOG_ERROR_EN << "[Comparison Run] Unable to load images from path: " << params_.base.image_folder;
            return;
        }

        // Set dedicated working directory for comparison run | 设置对比运行专用的工作目录
        std::string comparison_work_dir = params_.base.work_dir + "/" + current_dataset_name_ + "_openmvg_comparison";

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},
            {"dataset_dir", params_.base.dataset_dir},
            {"images_folder", params_.base.image_folder},
            {"work_dir", comparison_work_dir}};

        openmvg_comparison->SetMethodOptions(dynamic_options);

        // Handle intrinsic format conversion and force enable SfM reconstruction | 处理内参格式转换并强制启用SfM重建
        std::string intrinsics_semicolon = params_.openmvg.intrinsics;
        std::replace(intrinsics_semicolon.begin(), intrinsics_semicolon.end(), ',', ';');

        MethodOptions format_options = {
            {"intrinsics", intrinsics_semicolon},
            {"enable_sfm_reconstruction", "true"} // Force enable complete SfM reconstruction for comparison | 强制启用完整SfM重建用于对比
        };

        openmvg_comparison->SetMethodOptions(format_options);
        openmvg_comparison->SetRequiredData(images_data);

        LOG_INFO_ZH << "[对比运行] 开始执行OpenMVG完整SfM重建...";
        LOG_INFO_EN << "[Comparison Run] Starting OpenMVG complete SfM reconstruction...";

        auto comparison_result = openmvg_comparison->Build();

        if (comparison_result)
        {
            // Record time statistics for OpenMVG comparison pipeline | 记录OpenMVG对比流水线的时间统计

            LOG_INFO_ZH << "✓ [对比运行] OpenMVG流水线执行成功";
            LOG_INFO_EN << "✓ [Comparison Run] OpenMVG pipeline execution successful";

            // Add OpenMVG comparison pipeline time statistics to evaluation system (unified formatting as integer milliseconds)
            // 添加OpenMVG对比流水线的时间统计到评估系统（统一格式化为整数毫秒）
            if (params_.base.enable_evaluation)
            {

                // Interface::EvaluatorManager::AddEvaluationResult("Performance", "openmvg_pipeline",
                //                                                  "OpenMVG Pipeline Comparison", "CoreTime",
                //                                                  static_cast<int>(std::round(comparison_core_time)));
                LOG_DEBUG_ZH << "[对比运行] OpenMVG时间统计已添加到评估系统";
                LOG_DEBUG_EN << "[Comparison Run] OpenMVG time statistics added to evaluation system";
            }

            // Update reconstruction_dir to comparison run path for correct evaluation | 更新reconstruction_dir为对比运行的路径，以便正确评估
            std::string original_reconstruction_dir = params_.openmvg.reconstruction_dir;
            params_.openmvg.reconstruction_dir = comparison_work_dir + "/reconstruction_global";
            LOG_DEBUG_ZH << "[对比运行] 更新reconstruction_dir: " << params_.openmvg.reconstruction_dir;
            LOG_DEBUG_EN << "[Comparison Run] Updated reconstruction_dir: " << params_.openmvg.reconstruction_dir;

            // Immediately perform OpenMVG global pose evaluation | 立即进行OpenMVG全局位姿评估
            EvaluateOpenMVGGlobalPoses(current_dataset_name_);

            // Try to load and evaluate OpenMVG relative poses if export_relative_poses_file is configured (comparison run)
            // 如果配置了export_relative_poses_file，尝试加载并评估OpenMVG相对位姿（对比运行）
            std::string export_relative_poses_file = openmvg_comparison->GetOptionAsPath("export_relative_poses_file", "");
            if (!export_relative_poses_file.empty())
            {
                LOG_INFO_ZH << "=== [对比运行] OpenMVG相对位姿评估 ===";
                LOG_INFO_EN << "=== [Comparison Run] OpenMVG Relative Pose Evaluation ===";
                LOG_INFO_ZH << "[对比运行] 从OpenMVG导出的相对位姿文件读取数据: " << export_relative_poses_file;
                LOG_INFO_EN << "[Comparison Run] Loading relative poses from OpenMVG exported file: " << export_relative_poses_file;

                // Check if the relative poses file exists | 检查相对位姿文件是否存在
                if (!std::filesystem::exists(export_relative_poses_file))
                {
                    LOG_WARNING_ZH << "[对比运行] OpenMVG相对位姿文件不存在: " << export_relative_poses_file;
                    LOG_WARNING_EN << "[Comparison Run] OpenMVG relative poses file does not exist: " << export_relative_poses_file;
                }
                else
                {
                    // Create data_relative_poses to hold the loaded data | 创建data_relative_poses来保存加载的数据
                    auto openmvg_relative_poses_data = FactoryData::Create("data_relative_poses");
                    if (!openmvg_relative_poses_data)
                    {
                        LOG_ERROR_ZH << "[对比运行] 无法创建data_relative_poses数据对象";
                        LOG_ERROR_EN << "[Comparison Run] Cannot create data_relative_poses data object";
                    }
                    else
                    {
                        // Load relative poses from G2O file (reference test_Strecha.cpp implementation)
                        // 从G2O文件加载相对位姿（参考test_Strecha.cpp实现）
                        if (!openmvg_relative_poses_data->Load(export_relative_poses_file, "g2o"))
                        {
                            LOG_ERROR_ZH << "[对比运行] 无法从G2O文件加载OpenMVG相对位姿数据: " << export_relative_poses_file;
                            LOG_ERROR_EN << "[Comparison Run] Cannot load OpenMVG relative poses from G2O file: " << export_relative_poses_file;
                        }
                        else
                        {
                            // Verify loaded data | 验证加载的数据
                            auto openmvg_relative_poses_ptr = GetDataPtr<types::RelativePoses>(openmvg_relative_poses_data);
                            if (!openmvg_relative_poses_ptr || openmvg_relative_poses_ptr->empty())
                            {
                                LOG_WARNING_ZH << "[对比运行] 从文件加载的OpenMVG相对位姿数据为空";
                                LOG_WARNING_EN << "[Comparison Run] Loaded OpenMVG relative poses data is empty";
                            }
                            else
                            {
                                LOG_INFO_ZH << "[对比运行] 成功加载 " << openmvg_relative_poses_ptr->size() << " 个OpenMVG相对位姿";
                                LOG_INFO_EN << "[Comparison Run] Successfully loaded " << openmvg_relative_poses_ptr->size() << " OpenMVG relative poses";

                                // Set evaluator algorithm name to distinguish from PoSDK relative poses
                                // 设置评估器算法名称以区分PoSDK相对位姿
                                std::string original_algorithm = GetEvaluatorAlgorithm();
                                SetEvaluatorAlgorithm("openmvg_pipeline");

                                // Perform automatic evaluation using CallEvaluator | 使用CallEvaluator进行自动评估
                                if (GetGTData())
                                {
                                    LOG_INFO_ZH << "[对比运行] 开始执行OpenMVG相对位姿自动评估...";
                                    LOG_INFO_EN << "[Comparison Run] Starting OpenMVG relative pose automatic evaluation...";

                                    bool evaluation_success = CallEvaluator(openmvg_relative_poses_data);
                                    if (evaluation_success)
                                    {
                                        LOG_INFO_ZH << "[对比运行] OpenMVG相对位姿自动评估完成，结果已添加到EvaluatorManager";
                                        LOG_INFO_EN << "[Comparison Run] OpenMVG relative pose automatic evaluation completed, results added to EvaluatorManager";
                                    }
                                    else
                                    {
                                        LOG_ERROR_ZH << "[对比运行] OpenMVG相对位姿自动评估失败";
                                        LOG_ERROR_EN << "[Comparison Run] OpenMVG relative pose automatic evaluation failed";
                                    }
                                }
                                else
                                {
                                    LOG_WARNING_ZH << "[对比运行] 真值数据未设置，无法进行OpenMVG相对位姿自动评估";
                                    LOG_WARNING_EN << "[Comparison Run] Ground truth data not set, cannot perform OpenMVG relative pose automatic evaluation";
                                }

                                // Restore original algorithm name | 恢复原始算法名称
                                SetEvaluatorAlgorithm(original_algorithm);
                            }
                        }
                    }
                }
            }

            // Restore original reconstruction_dir | 恢复原始的reconstruction_dir
            params_.openmvg.reconstruction_dir = original_reconstruction_dir;
            LOG_DEBUG_ZH << "[对比运行] 恢复原始reconstruction_dir: " << params_.openmvg.reconstruction_dir;
            LOG_DEBUG_EN << "[Comparison Run] Restored original reconstruction_dir: " << params_.openmvg.reconstruction_dir;
        }
        else
        {
            LOG_ERROR_ZH << "✗ [对比运行] OpenMVG流水线执行失败";
            LOG_ERROR_EN << "✗ [Comparison Run] OpenMVG pipeline execution failed";
        }
    }

    // Run Colmap pipeline for comparison | 运行Colmap流水线进行对比
    void GlobalSfMPipeline::RunColmapForComparison()
    {
        LOG_INFO_ZH << "=== [对比运行] Colmap流水线 ===";
        LOG_INFO_EN << "=== [Comparison Run] Colmap Pipeline ===";

        // Create Colmap preprocessor for comparison | 创建Colmap预处理器用于对比
        auto colmap_comparison = CreateAndConfigureSubMethod("colmap_pipeline");
        if (!colmap_comparison)
        {
            LOG_ERROR_ZH << "[对比运行] 无法创建Colmap预处理器";
            LOG_ERROR_EN << "[Comparison Run] Unable to create Colmap preprocessor";
            return;
        }

        // Set profiler label for comparison pipeline | 为对比流水线设置性能分析标签
        colmap_comparison->SetProfilerLabels({{"pipeline", "COLMAP"}, {"dataset", current_dataset_name_}});

        // Set dedicated working directory for comparison run | 设置对比运行专用的工作目录
        std::string comparison_work_dir = params_.base.work_dir + "/" + current_dataset_name_ + "_colmap_comparison";

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},
            {"dataset_dir", params_.base.dataset_dir},
            {"images_folder", params_.base.image_folder},
            {"work_dir", comparison_work_dir}};

        colmap_comparison->SetMethodOptions(dynamic_options);

        // Pass camera parameters (if available) | 传递相机参数（如果有的话）
        MethodOptions camera_options = {
            {"intrinsics", params_.openmvg.intrinsics},
            {"camera_model", std::to_string(params_.openmvg.camera_model)},
            {"ProfileCommit", "Colmap comparison pipeline - " + current_dataset_name_}};

        colmap_comparison->SetMethodOptions(camera_options);

        LOG_INFO_ZH << "[对比运行] 开始执行Colmap重建...";
        LOG_INFO_EN << "[Comparison Run] Starting Colmap reconstruction...";
        auto start_time = std::chrono::high_resolution_clock::now();
        auto comparison_result = colmap_comparison->Build();
        auto end_time = std::chrono::high_resolution_clock::now();

        if (comparison_result)
        {
            // Record time statistics for Colmap comparison pipeline | 记录Colmap对比流水线的时间统计
            double comparison_total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            // Note: Core time is now managed by Profiler system | 注意：核心时间现在由Profiler系统管理

            LOG_INFO_ZH << "✓ [对比运行] Colmap流水线执行成功";
            LOG_INFO_ZH << "[对比运行] Colmap执行时间: 总时间=" << comparison_total_time << "ms";
            LOG_INFO_EN << "✓ [Comparison Run] Colmap pipeline execution successful";
            LOG_INFO_EN << "[Comparison Run] Colmap execution time: Total=" << comparison_total_time << "ms";

            // Add Colmap comparison pipeline time statistics to evaluation system (unified formatting as integer milliseconds)
            // 添加Colmap对比流水线的时间统计到评估系统（统一格式化为整数毫秒）
            if (params_.base.enable_evaluation)
            {

                // Interface::EvaluatorManager::AddEvaluationResult("Performance", "colmap_pipeline",
                //                                                  "Colmap Pipeline Comparison", "CoreTime",
                //                                                  static_cast<int>(std::round(comparison_core_time)));

                LOG_DEBUG_ZH << "[对比运行] Colmap时间统计已添加到评估系统";
                LOG_DEBUG_EN << "[Comparison Run] Colmap time statistics added to evaluation system";
            }

            // Immediately perform COLMAP global pose evaluation | 立即进行COLMAP全局位姿评估
            EvaluateColmapGlobalPoses(current_dataset_name_);
        }
        else
        {
            LOG_ERROR_ZH << "✗ [对比运行] Colmap流水线执行失败";
            LOG_ERROR_EN << "✗ [Comparison Run] Colmap pipeline execution failed";
        }
    }
    void GlobalSfMPipeline::RunGlomapForComparison()
    {
        // Starting Glomap comparison pipeline | 开始Glomap对比流水线
        LOG_INFO_ZH << "=== [对比运行] Glomap流水线 ===";
        LOG_INFO_EN << "=== [Comparison Run] Glomap Pipeline ===";

        // Create Glomap preprocessor for comparison | 创建Glomap预处理器用于对比
        auto glomap_comparison = CreateAndConfigureSubMethod("glomap_pipeline");
        if (!glomap_comparison)
        {
            LOG_ERROR_ZH << "[对比运行] 无法创建Glomap预处理器";
            LOG_ERROR_EN << "[Comparison Run] Failed to create Glomap preprocessor";
            return;
        }

        // Set profiler label for comparison pipeline | 为对比流水线设置性能分析标签
        glomap_comparison->SetProfilerLabels({{"pipeline", "GLOMAP"}, {"dataset", current_dataset_name_}});

        // Set comparison-specific work directory | 设置对比运行专用的工作目录
        std::string comparison_work_dir = params_.base.work_dir + "/" + current_dataset_name_ + "_glomap_comparison";

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},
            {"dataset_dir", params_.base.dataset_dir},
            {"images_folder", params_.base.image_folder},
            {"work_dir", comparison_work_dir}};

        glomap_comparison->SetMethodOptions(dynamic_options);

        // Pass camera parameters (if available) | 传递相机参数（如果有的话）
        MethodOptions camera_options = {
            {"intrinsics", params_.openmvg.intrinsics},
            {"camera_model", std::to_string(params_.openmvg.camera_model)},
            {"ProfileCommit", "Glomap comparison pipeline - " + current_dataset_name_}};

        glomap_comparison->SetMethodOptions(camera_options);

        LOG_INFO_ZH << "[对比运行] 开始执行Glomap重建...";
        LOG_INFO_EN << "[Comparison Run] Starting Glomap reconstruction...";
        auto start_time = std::chrono::high_resolution_clock::now();
        auto comparison_result = glomap_comparison->Build();
        auto end_time = std::chrono::high_resolution_clock::now();

        if (comparison_result)
        {
            // Record time statistics for Glomap comparison pipeline | 记录Glomap对比流水线的时间统计
            double comparison_total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            // Note: Core time is now managed by Profiler system | 注意：核心时间现在由Profiler系统管理

            LOG_INFO_ZH << "✓ [对比运行] Glomap流水线执行成功";
            LOG_INFO_ZH << "[对比运行] Glomap执行时间: 总时间=" << comparison_total_time << "ms";
            LOG_INFO_EN << "✓ [Comparison Run] Glomap pipeline execution successful";
            LOG_INFO_EN << "[Comparison Run] Glomap execution time: Total=" << comparison_total_time << "ms";

            // Add Glomap comparison pipeline time statistics to evaluation system (unified formatting as integer milliseconds)
            // 添加Glomap对比流水线的时间统计到评估系统（统一格式化为整数毫秒）
            if (params_.base.enable_evaluation)
            {

                // Interface::EvaluatorManager::AddEvaluationResult("Performance", "glomap_pipeline",
                //                                                  "Glomap Pipeline Comparison", "CoreTime",
                //                                                  static_cast<int>(std::round(comparison_core_time)));
                LOG_DEBUG_ZH << "[对比运行] Glomap时间统计已添加到评估系统";
                LOG_DEBUG_EN << "[Comparison Run] Glomap time statistics added to evaluation system";
            }

            // Immediately perform GLOMAP global pose evaluation | 立即进行GLOMAP全局位姿评估
            EvaluateGlomapGlobalPoses(current_dataset_name_);
        }
        else
        {
            LOG_ERROR_ZH << "✗ [对比运行] Glomap流水线执行失败";
            LOG_ERROR_EN << "✗ [Comparison Run] Glomap pipeline execution failed";
        }
    }

    DataPtr GlobalSfMPipeline::RunColmapPreprocess()
    {
        // Starting Colmap preprocessing | 开始使用Colmap进行预处理
        LOG_INFO_ZH << "=== 使用Colmap进行预处理 ===";
        LOG_INFO_EN << "=== Preprocessing with Colmap ===";

        // Create Colmap preprocessor | 创建Colmap预处理器
        colmap_preprocess_ = CreateAndConfigureSubMethod("colmap_pipeline");
        if (!colmap_preprocess_)
        {
            LOG_ERROR_ZH << "无法创建Colmap预处理器";
            LOG_ERROR_EN << "Failed to create Colmap preprocessor";
            return nullptr;
        }

        // Configure Colmap preprocessing parameters (consistent with OpenMVG parameter passing) | 配置Colmap预处理参数（与OpenMVG参数传递方式一致）
        LOG_INFO_ZH << "Colmap预处理配置: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;
        LOG_INFO_EN << "Colmap preprocessing configuration: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;

        // Use dataset-specific work_dir for Colmap preprocessor to avoid multi-dataset conflicts
        // 为Colmap预处理器使用数据集特定的work_dir，避免多数据集冲突
        std::string dataset_specific_work_dir = params_.base.work_dir + "/" + current_dataset_name_;

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},       // Directly use main configuration parameters | 直接使用主配置参数
            {"dataset_dir", params_.base.dataset_dir},    // Directly use main configuration parameters | 直接使用主配置参数
            {"images_folder", params_.base.image_folder}, // Directly use main configuration parameters | 直接使用主配置参数
            {"work_dir", dataset_specific_work_dir}       // Use dataset-specific work_dir | 使用数据集特定的work_dir
        };

        LOG_INFO_ZH << "Colmap预处理器使用数据集特定work_dir: " << dataset_specific_work_dir;
        LOG_INFO_EN << "Colmap preprocessor using dataset-specific work_dir: " << dataset_specific_work_dir;

        // Pass dynamic path parameters | 传递动态路径参数
        colmap_preprocess_->SetMethodOptions(dynamic_options);

        // Execute preprocessing | 执行预处理
        auto preprocess_result = colmap_preprocess_->Build();
        if (!preprocess_result)
        {
            LOG_ERROR_ZH << "Colmap预处理失败";
            LOG_ERROR_EN << "Colmap preprocessing failed";
            return nullptr;
        }

        // Note: This function is only used for comparison pipeline, evaluation is handled in RunColmapForComparison
        // 注意：此函数仅用于对比流水线，评估在RunColmapForComparison中处理

        return preprocess_result;
    }

    DataPtr GlobalSfMPipeline::RunGlomapPreprocess()
    {
        // Starting Glomap preprocessing | 开始使用Glomap进行预处理
        LOG_INFO_ZH << "=== 使用Glomap进行预处理 ===";
        LOG_INFO_EN << "=== Preprocessing with Glomap ===";

        // Create Glomap preprocessor | 创建Glomap预处理器
        glomap_preprocess_ = CreateAndConfigureSubMethod("glomap_pipeline");
        if (!glomap_preprocess_)
        {
            LOG_ERROR_ZH << "无法创建Glomap预处理器";
            LOG_ERROR_EN << "Failed to create Glomap preprocessor";
            return nullptr;
        }

        // Configure Glomap preprocessing parameters (consistent with OpenMVG parameter passing) | 配置Glomap预处理参数（与OpenMVG参数传递方式一致）
        LOG_INFO_ZH << "Glomap预处理配置: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;
        LOG_INFO_EN << "Glomap preprocessing configuration: dataset_dir=" << params_.base.dataset_dir << ", images_folder=" << params_.base.image_folder << ", work_dir=" << params_.base.work_dir;

        // Use dataset-specific work_dir for Glomap preprocessor to avoid multi-dataset conflicts
        // 为Glomap预处理器使用数据集特定的work_dir，避免多数据集冲突
        std::string dataset_specific_work_dir = params_.base.work_dir + "/" + current_dataset_name_;

        MethodOptions dynamic_options = {
            {"root_dir", params_.base.dataset_dir},       // Directly use main configuration parameters | 直接使用主配置参数
            {"dataset_dir", params_.base.dataset_dir},    // Directly use main configuration parameters | 直接使用主配置参数
            {"images_folder", params_.base.image_folder}, // Directly use main configuration parameters | 直接使用主配置参数
            {"work_dir", dataset_specific_work_dir}       // Use dataset-specific work_dir | 使用数据集特定的work_dir
        };

        LOG_INFO_ZH << "Glomap预处理器使用数据集特定work_dir: " << dataset_specific_work_dir;
        LOG_INFO_EN << "Glomap preprocessor using dataset-specific work_dir: " << dataset_specific_work_dir;

        // Pass dynamic path parameters | 传递动态路径参数
        glomap_preprocess_->SetMethodOptions(dynamic_options);

        // Execute preprocessing | 执行预处理
        auto preprocess_result = glomap_preprocess_->Build();
        if (!preprocess_result)
        {
            LOG_ERROR_ZH << "Glomap预处理失败";
            LOG_ERROR_EN << "Glomap preprocessing failed";
            return nullptr;
        }

        // Note: This function is only used for comparison pipeline, evaluation is handled in RunGlomapForComparison
        // 注意：此函数仅用于对比流水线，评估在RunGlomapForComparison中处理

        return preprocess_result;
    }

    DataPtr GlobalSfMPipeline::RunPoSDKPreprocess()
    {
        // Starting PoSDK preprocessing | 开始使用PoSDK进行预处理
        LOG_INFO_ZH << "=== 使用PoSDK进行预处理 ===";
        LOG_INFO_EN << "=== Preprocessing with PoSDK ===";

        // Determine which feature extraction + matching method to use based on preprocessing type | 根据预处理类型确定使用哪个特征提取+匹配方法
        std::string matcher_plugin_name;
        if (params_.base.preprocess_type == PreprocessType::OpenCV)
        {
            matcher_plugin_name = "method_img2matches";
            LOG_INFO_ZH << "  使用OpenCV方法: method_img2matches";
            LOG_INFO_EN << "  Using OpenCV method: method_img2matches";
        }
        else // PreprocessType::PoSDK
        {
            matcher_plugin_name = "posdk_preprocessor";
            LOG_INFO_ZH << "  使用PoSDK优化方法: posdk_preprocessor";
            LOG_INFO_EN << "  Using PoSDK optimized method: posdk_preprocessor";
        }

        // Create PoSDK integrated feature extraction + matcher | 创建PoSDK一体化特征提取+匹配器
        auto img2matches_ = CreateAndConfigureSubMethod(matcher_plugin_name);

        // std::cout << "[GlobalSfMPipeline]enable_profiling_:" << img2matches_->GetOptionAsBool("enable_profiling") << std::endl;
        if (!img2matches_)
        {
            LOG_ERROR_ZH << "无法创建PoSDK预处理器: " << matcher_plugin_name;
            LOG_ERROR_EN << "Failed to create PoSDK preprocessor: " << matcher_plugin_name;
            return nullptr;
        }

        // First create and load image data | 首先创建并加载图像数据
        auto images_data = FactoryData::Create("data_images");
        if (!images_data)
        {
            LOG_ERROR_ZH << "无法创建data_images";
            LOG_ERROR_EN << "Failed to create data_images";
            return nullptr;
        }

        // Load image paths | 加载图像路径
        if (!images_data->Load(params_.base.image_folder))
        {
            LOG_ERROR_ZH << "无法从路径加载图像: " << params_.base.image_folder;
            LOG_ERROR_EN << "Failed to load images from path: " << params_.base.image_folder;
            return nullptr;
        }

        // Use dataset-specific storage path for PoSDK preprocessor | 为PoSDK预处理器使用数据集特定的存储路径
        std::string dataset_specific_work_dir = params_.base.work_dir + "/" + current_dataset_name_;
        std::string features_export_path = dataset_specific_work_dir + "/features/features_all";
        std::string matches_export_path = dataset_specific_work_dir + "/matches/matches_all";

        LOG_DEBUG_ZH << "PoSDK预处理器使用数据集特定存储路径:";
        LOG_DEBUG_ZH << "  特征导出路径: " << features_export_path;
        LOG_DEBUG_ZH << "  匹配导出路径: " << matches_export_path;
        LOG_DEBUG_EN << "PoSDK preprocessor using dataset-specific storage path:";
        LOG_DEBUG_EN << "  Features export path: " << features_export_path;
        LOG_DEBUG_EN << "  Matches export path: " << matches_export_path;

        // Set PoSDK integrated feature extraction + matching parameters (precisely aligned with OpenMVG HIGH preset)
        // 设置PoSDK一体化特征提取+匹配参数（精确对齐OpenMVG HIGH预设）
        img2matches_->SetMethodOptions({{"export_fea_path", features_export_path}, // Dataset-specific feature export path | 数据集特定的特征导出路径

                                        // Matching parameters | 匹配参数

                                        {"max_matches", "0"},                       // No limit on match count (consistent with OpenMVG) | 不限制匹配数量（与OpenMVG一致）
                                        {"export_match_path", matches_export_path}, // Dataset-specific match export path | 数据集特定的匹配导出路径

                                        {"ProfileCommit", "GlobalSfM pipeline PoSDK integrated feature extraction and matching (OpenMVG aligned)"}});

        // Set image data as input | 设置图像数据作为输入
        img2matches_->SetRequiredData(images_data);

        // Set camera model data as input (required by Img2MatchesV2 for bearing pairs calculation) | 设置相机模型数据作为输入（Img2MatchesV2计算视线向量对需要）
        if (camera_model_data_)
        {
            img2matches_->SetRequiredData(camera_model_data_);
            LOG_DEBUG_ZH << "已设置相机模型数据到" << matcher_plugin_name;
            LOG_DEBUG_EN << "Camera model data set to " << matcher_plugin_name;
        }
        else
        {
            LOG_WARNING_ZH << "相机模型数据不可用，" << matcher_plugin_name << "可能无法计算视线向量对";
            LOG_WARNING_EN << "Camera model data unavailable, " << matcher_plugin_name << " may not compute bearing pairs";
        }

        // Execute integrated feature extraction + matching | 执行一体化特征提取+匹配
        img2matches_->SetProfilerLabels({{"pipeline", "PoSDK"}, {"dataset", current_dataset_name_}});

        auto features_matches_result = img2matches_->Build();

        if (!features_matches_result)
        {
            LOG_ERROR_ZH << "PoSDK一体化特征提取+匹配失败";
            LOG_ERROR_EN << "PoSDK integrated feature extraction + matching failed";
            return nullptr;
        }

        // Convert result to DataPackage and add image data | 将结果转换为DataPackage，并添加图像数据
        auto result_package_ptr = std::dynamic_pointer_cast<DataPackage>(features_matches_result);
        if (!result_package_ptr)
        {
            LOG_ERROR_ZH << "无法转换特征匹配结果为DataPackage";
            LOG_ERROR_EN << "Failed to convert feature matching result to DataPackage";
            return nullptr;
        }

        // Add image data to result package | 添加图像数据到结果包中
        result_package_ptr->AddData(images_data);

        return std::static_pointer_cast<DataIO>(result_package_ptr);
    }

    std::string GlobalSfMPipeline::GetPreprocessTypeStr() const
    {
        switch (params_.base.preprocess_type)
        {
        case PreprocessType::OpenMVG:
            return "OpenMVG";
        case PreprocessType::OpenCV:
            return "OpenCV (method_img2matches)";
        case PreprocessType::PoSDK:
            return "PoSDK (posdk_preprocessor)";
        default:
            return "Unknown";
        }
    }

    MethodPresetProfilerPtr GlobalSfMPipeline::CreateAndConfigureSubMethod(const std::string &method_type)
    {
        auto method = FactoryMethod::Create(method_type);
        if (!method)
        {
            LOG_ERROR_ZH << "无法创建方法: " << method_type;
            LOG_ERROR_EN << "Failed to create method: " << method_type;
            return nullptr;
        }

        // Try to convert to MethodPresetProfiler type | 尝试转换为MethodPresetProfiler类型
        auto method_preset_profiler = std::dynamic_pointer_cast<MethodPresetProfiler>(method);
        if (!method_preset_profiler)
        {
            LOG_ERROR_ZH << "方法 " << method_type << " 不是MethodPresetProfiler类型";
            LOG_ERROR_EN << "Method " << method_type << " is not MethodPresetProfiler type";
            return nullptr;
        }

        // Pass parameters using PassingMethodOptions | 使用PassingMethodOptions传递参数
        PassingMethodOptions(method_preset_profiler);

        return method_preset_profiler;
    }

    DataPtr GlobalSfMPipeline::Step2_TwoViewEstimation(DataPtr preprocess_result)
    {
        // Executing two-view pose estimation | 执行双视图位姿估计
        LOG_INFO_ZH << "=== 执行双视图位姿估计 ===";
        LOG_INFO_EN << "=== Executing Two-View Pose Estimation ===";

        // Create two-view estimator | 创建双视图估计器
        two_view_estimator_ = CreateAndConfigureSubMethod("TwoViewEstimator");
        if (!two_view_estimator_)
        {
            return nullptr;
        }

        // Set input data | 设置输入数据
        auto data_package = std::dynamic_pointer_cast<DataPackage>(preprocess_result);

        // Check and get required data | 检查并获取必要的数据
        auto matches_data = data_package->GetData("data_matches");
        auto features_data = data_package->GetData("data_features");
        auto camera_models_data = data_package->GetData("data_camera_models");

        if (!matches_data || !features_data)
        {
            LOG_ERROR_ZH << "缺少必要的匹配或特征数据";
            LOG_ERROR_EN << "Missing required matching or feature data";
            return nullptr;
        }

        // If no camera model data, create default Strecha camera model | 如果没有相机模型数据，创建默认的Strecha相机模型
        if (!camera_models_data)
        {
            camera_models_data = CreateStrechaCameraModel();
            if (!camera_models_data)
            {
                LOG_ERROR_ZH << "无法创建相机模型数据";
                LOG_ERROR_EN << "Failed to create camera model data";
                return nullptr;
            }
        }

        two_view_estimator_->SetRequiredData(matches_data);
        two_view_estimator_->SetRequiredData(features_data);
        two_view_estimator_->SetRequiredData(camera_models_data);

        // Visualize original matching relationships (before two-view estimation) - consistent with test_Strecha.cpp
        // 可视化原始匹配关系（双视图估计前）- 与test_Strecha.cpp一致
        if (params_.base.enable_matches_visualization)
        {
            VisualizeMatches(data_package, "before", "原始匹配关系可视化 - 双视图估计前");
        }

        // Set GT data for automatic evaluation (if evaluator is enabled) | 设置GT数据用于自动评估（如果启用了evaluator）
        if (params_.base.enable_evaluation && !params_.base.gt_folder.empty())
        {
            // Try to get GT data from already loaded GT data package | 尝试从已加载的GT数据包获取GT数据
            auto gt_data_package = GetGTData();
            if (gt_data_package)
            {
                // Try to extract relative pose GT data from the data package | 尝试从数据包中提取相对位姿GT数据
                auto gt_data_pkg = std::dynamic_pointer_cast<DataPackage>(gt_data_package);
                if (gt_data_pkg)
                {
                    auto gt_relative_poses_data = gt_data_pkg->GetData("data_relative_poses");
                    if (gt_relative_poses_data)
                    {
                        two_view_estimator_->SetGTData(gt_relative_poses_data);
                    }
                }
                else
                {
                    LOG_WARNING_ZH << "GT数据不是DataPackage格式，无法提取相对位姿数据";
                    LOG_WARNING_EN << "GT data is not in DataPackage format, cannot extract relative pose data";
                }
            }
            else
            {
                // Fallback to loading ground truth data if not already loaded | 如果尚未加载，回退到加载真值数据
                LOG_DEBUG_ZH << "GT数据包未设置，尝试加载真值数据...";
                LOG_DEBUG_EN << "GT data package not set, attempting to load ground truth data...";

                if (gt_relative_poses_.empty() && !LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
                {
                    LOG_ERROR_ZH << "无法加载真值数据用于评估";
                    LOG_ERROR_EN << "Failed to load ground truth data for evaluation";
                }
                else
                {
                    // Calculate relative pose ground truth from global poses (if not done yet)
                    // 从全局位姿计算相对位姿真值（如果还没有）
                    if (gt_relative_poses_.empty() && !gt_global_poses_.GetRotations().empty())
                    {
                        if (!types::Global2RelativePoses(gt_global_poses_, gt_relative_poses_))
                        {
                            LOG_ERROR_ZH << "无法从全局位姿计算相对位姿真值";
                            LOG_ERROR_EN << "Failed to calculate relative pose ground truth from global poses";
                        }
                    }

                    if (!gt_relative_poses_.empty())
                    {
                        auto gt_relative_poses_datamap = std::make_shared<DataMap<RelativePoses>>(gt_relative_poses_, "data_relative_poses");
                        DataPtr gt_relative_poses_data = std::static_pointer_cast<DataIO>(gt_relative_poses_datamap);
                        two_view_estimator_->SetGTData(gt_relative_poses_data);
                        LOG_DEBUG_ZH << "已设置GT相对位姿数据用于自动评估，包含 " << gt_relative_poses_.size() << " 个位姿对";
                        LOG_DEBUG_EN << "GT relative pose data set for automatic evaluation, containing " << gt_relative_poses_.size() << " pose pairs";
                    }
                }
            }
        }

        PROFILER_START_AUTO(true);
        PROFILER_STAGE("step2_two_view_estimation"); // Mark Step 2 stage | 标记步骤2阶段
        // Execute two-view estimation | 执行双视图估计
        auto result = two_view_estimator_->Build();
        PROFILER_END();
        if (!result)
        {
            LOG_ERROR_ZH << "双视图位姿估计失败";
            LOG_ERROR_EN << "Two-view pose estimation failed";
            return nullptr;
        }

        // Visualize matching relationships after two-view estimation (enhanced outlier display) - consistent with test_Strecha.cpp
        // 可视化双视图估计后的匹配关系（强化outlier显示）- 与test_Strecha.cpp一致
        if (params_.base.enable_matches_visualization)
        {
            VisualizeMatches(result, "after", "双视图估计后匹配关系可视化 - 强化outlier显示");
        }
        return result;
    }

    void GlobalSfMPipeline::VisualizeMatches(DataPtr data_package, const std::string &stage, const std::string &description)
    {

        LOG_INFO_ZH << "=== " << description << " ===";
        LOG_INFO_EN << "=== " << description << " ===";

        // Create match visualizer (if not created yet) | 创建匹配可视化器（如果还没有创建）
        if (!matches_visualizer_)
        {
            matches_visualizer_ = CreateAndConfigureSubMethod("method_matches_visualizer");
            if (!matches_visualizer_)
            {
                LOG_ERROR_ZH << "无法创建匹配可视化器";
                LOG_ERROR_EN << "Failed to create match visualizer";
                return;
            }
        }

        // Set visualization parameters - unified output to dataset directory under work_dir
        // 设置可视化参数 - 统一输出到work_dir下的数据集目录
        std::string visualize_dir;

        // Get current dataset name | 获取当前数据集名称
        std::string current_dataset_name = GetCurrentDatasetName();

        if (!current_dataset_name.empty())
        {
            // Has dataset name: output to dataset subdirectory under work_dir | 有数据集名称：输出到work_dir下的数据集子目录
            visualize_dir = params_.base.work_dir + "/" + current_dataset_name + "/visualizeMatches";
            LOG_DEBUG_ZH << "可视化输出到数据集工作目录: " << current_dataset_name;
            LOG_DEBUG_EN << "Visualization output to dataset work directory: " << current_dataset_name;
        }
        else
        {
            // No dataset name: output directly to work_dir | 无数据集名称：直接输出到work_dir
            visualize_dir = params_.base.work_dir + "/visualizeMatches";
            LOG_DEBUG_ZH << "可视化输出到工作目录";
            LOG_DEBUG_EN << "Visualization output to work directory";
        }

        std::string export_folder = visualize_dir + "/matches_" + stage;
        std::filesystem::create_directories(export_folder);

        LOG_INFO_ZH << "可视化输出路径: " << export_folder;
        LOG_INFO_EN << "Visualization output path: " << export_folder;

        matches_visualizer_->SetMethodOptions({{"export_folder", export_folder},
                                               {"enhance_outliers", stage == "after" ? "true" : "false"}, // Enhanced outlier display after two-view estimation | 双视图估计后强化outlier显示
                                               {"ProfileCommit", description}});

        // Execute visualization | 执行可视化
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("visualize_matches"); // Mark VisualizeMatches stage | 标记VisualizeMatches阶段
        auto vis_result = matches_visualizer_->Build(data_package);
        PROFILER_END();
        if (vis_result)
        {
            LOG_INFO_ZH << description << "完成，输出目录: " << export_folder;
            LOG_INFO_EN << description << " completed, output directory: " << export_folder;
            if (stage == "after")
            {
                LOG_INFO_ZH << "注意: 绿色线条为内点匹配，红色线条为外点匹配";
                LOG_INFO_EN << "Note: Green lines are inlier matches, red lines are outlier matches";
            }
        }
        else
        {
            LOG_ERROR_ZH << description << "失败";
            LOG_ERROR_EN << description << " failed";
        }
    }

    std::string GlobalSfMPipeline::GetCurrentDatasetName()
    {
        // Prioritize using the set current dataset name | 优先使用已设置的当前数据集名称
        if (!current_dataset_name_.empty())
        {
            return current_dataset_name_;
        }

        // If not set, try to infer from image_folder | 如果没有设置，尝试从image_folder推断
        if (!params_.base.image_folder.empty())
        {
            return ExtractDatasetName(params_.base.image_folder);
        }

        return ""; // Unable to determine dataset name | 无法确定数据集名称
    }

    DataPtr GlobalSfMPipeline::CreateStrechaCameraModel()
    {
        auto camera_model_data = FactoryData::Create("data_camera_models");
        if (!camera_model_data)
        {
            LOG_ERROR_ZH << "无法创建data_camera_models";
            LOG_ERROR_EN << "Failed to create data_camera_models";
            return nullptr;
        }

        auto camera_models = GetDataPtr<types::CameraModels>(camera_model_data);
        if (!camera_models)
        {
            LOG_ERROR_ZH << "无法获取CameraModels指针";
            LOG_ERROR_EN << "Failed to get CameraModels pointer";
            return nullptr;
        }

        // Create camera model for Strecha dataset (read parameters from config file)
        // 创建Strecha数据集的相机模型（从配置文件读取参数）
        types::CameraModel camera;

        // 设置Strecha数据集默认相机参数 | Set default camera parameters for Strecha dataset
        double fx = GetOptionAsDouble("camera_fx", 2759.48);
        double fy = GetOptionAsDouble("camera_fy", 2764.16);
        double cx = GetOptionAsDouble("camera_cx", 1520.69);
        double cy = GetOptionAsDouble("camera_cy", 1006.81);
        uint32_t width = static_cast<uint32_t>(GetOptionAsIndexT("camera_width", 3040));
        uint32_t height = static_cast<uint32_t>(GetOptionAsIndexT("camera_height", 2014));

        // 设置相机内参 | Set camera intrinsics
        camera.SetCameraIntrinsics(fx, fy, cx, cy, width, height);

        // 设置畸变参数 | Set distortion parameters
        std::vector<double> default_radial_distortion;
        std::vector<double> default_tangential_distortion;
        camera.SetDistortionParams(types::DistortionType::RADIAL_K3, default_radial_distortion, default_tangential_distortion);

        // 初始化畸变参数为零 | Initialize distortion parameters to zero
        auto &intrinsics = camera.GetIntrinsics();
        const_cast<types::CameraIntrinsics &>(intrinsics).InitDistortionParams();

        camera_models->push_back(camera);

        // 输出相机参数信息 | Output camera parameter information
        LOG_DEBUG_ZH << "创建了Strecha相机模型: fx=" << intrinsics.GetFx()
                     << ", fy=" << intrinsics.GetFy()
                     << ", cx=" << intrinsics.GetCx()
                     << ", cy=" << intrinsics.GetCy();
        LOG_DEBUG_EN << "Created Strecha camera model: fx=" << intrinsics.GetFx()
                     << ", fy=" << intrinsics.GetFy()
                     << ", cx=" << intrinsics.GetCx()
                     << ", cy=" << intrinsics.GetCy();

        return camera_model_data;
    }

    DataPtr GlobalSfMPipeline::Step2_5_RotationRefinement(DataPtr relative_poses_result, DataPtr preprocess_result)
    {
        // Create rotation refiner | 创建旋转优化器
        LOG_INFO_ZH << "创建RotationRefineByFeatures方法实例";
        LOG_INFO_EN << "Creating RotationRefineByFeatures method instance";

        auto rotation_refiner = CreateAndConfigureSubMethod("rotation_refine_by_features");
        if (!rotation_refiner)
        {
            return nullptr;
        }

        // Configure rotation refinement parameters | 配置旋转优化参数
        rotation_refiner->SetMethodOptions({{"ProfileCommit", "GlobalSfM pipeline rotation refinement"},
                                            {"enable_profiling", GetOptionAsString("enable_profiling", "false")},
                                            {"metrics_config", "time"},
                                            {"min_block_pixels", "100"},
                                            {"color_similarity_threshold", "30.0"},
                                            {"min_overlap_ratio", "0.3"},
                                            {"min_matched_pixels", "50"},
                                            {"grid_search_levels", "3"},
                                            {"initial_step_size", "0.1"},
                                            {"step_size_decay", "0.5"},
                                            {"max_iterations", "50"},
                                            {"enable_visualize_patches", "false"},
                                            {"visualize_output_dir", params_.base.work_dir + "/" + GetCurrentDatasetName() + "/rotation_refine_viz"}});

        // Prepare input data package | 准备输入数据包
        auto input_package = std::make_shared<DataPackage>();

        // Extract data from preprocessing result | 从预处理结果中提取数据
        auto preprocess_package = std::dynamic_pointer_cast<DataPackage>(preprocess_result);
        if (!preprocess_package)
        {
            LOG_ERROR_ZH << "预处理结果不是DataPackage类型";
            LOG_ERROR_EN << "Preprocessing result is not DataPackage type";
            return nullptr;
        }

        auto images_data = preprocess_package->GetData("data_images");
        auto matches_data = preprocess_package->GetData("data_matches");
        auto features_data = preprocess_package->GetData("data_features");
        auto camera_models = preprocess_package->GetData("data_camera_models");

        if (!images_data || !matches_data)
        {
            LOG_ERROR_ZH << "预处理结果中缺少必要的数据（images或matches）";
            LOG_ERROR_EN << "Missing required data (images or matches) in preprocessing result";
            return nullptr;
        }

        // If no camera model data, create default Strecha camera model
        // 如果没有相机模型数据，创建默认的Strecha相机模型
        if (!camera_models)
        {
            camera_models = CreateStrechaCameraModel();
            if (!camera_models)
            {
                LOG_ERROR_ZH << "无法创建相机模型数据用于旋转优化";
                LOG_ERROR_EN << "Unable to create camera model data for rotation refinement";
                return nullptr;
            }
        }

        // Add data to input package | 添加数据到输入包
        input_package->AddData("data_images", images_data);
        input_package->AddData("data_matches", matches_data);
        input_package->AddData("data_camera_models", camera_models);

        // Extract relative poses from relative_poses_result | 从相对位姿结果中提取相对位姿
        if (auto package = std::dynamic_pointer_cast<DataPackage>(relative_poses_result))
        {
            auto relative_poses_data = package->GetData("data_relative_poses");
            if (relative_poses_data)
            {
                input_package->AddData("data_relative_poses", relative_poses_data);
            }
            else
            {
                LOG_ERROR_ZH << "无法从双视图估计结果中获取data_relative_poses";
                LOG_ERROR_EN << "Failed to get data_relative_poses from two-view estimation result";
                return nullptr;
            }
        }
        else
        {
            // Compatibility handling: directly add relative_poses_result as data_relative_poses
            // 兼容性处理：直接添加relative_poses_result作为data_relative_poses
            input_package->AddData("data_relative_poses", relative_poses_result);
        }

        // Execute rotation refinement | 执行旋转优化
        LOG_INFO_ZH << "开始执行旋转优化";
        LOG_INFO_EN << "Starting rotation refinement";

        PROFILER_START_AUTO(true);
        PROFILER_STAGE("step2_5_rotation_refinement"); // Mark Step 2.5 stage | 标记步骤2.5阶段
        auto result = rotation_refiner->Build(input_package);
        PROFILER_END();

        if (!result)
        {
            LOG_ERROR_ZH << "旋转优化失败";
            LOG_ERROR_EN << "Rotation refinement failed";
            return nullptr;
        }

        LOG_INFO_ZH << "旋转优化成功完成";
        LOG_INFO_EN << "Rotation refinement completed successfully";

        // Return refined relative poses in the same format as input | 以与输入相同的格式返回优化后的相对位姿
        if (std::dynamic_pointer_cast<DataPackage>(relative_poses_result))
        {
            // If input was a DataPackage, return DataPackage | 如果输入是DataPackage，返回DataPackage
            auto output_package = std::make_shared<DataPackage>();
            output_package->AddData("data_relative_poses", result);
            return output_package;
        }
        else
        {
            // If input was direct data, return direct data | 如果输入是直接数据，返回直接数据
            return result;
        }
    }

    DataPtr GlobalSfMPipeline::Step3_RotationAveraging(DataPtr relative_poses_result, DataPtr camera_models)
    {
        // Create rotation averager | 创建旋转平均器
        rotation_averager_ = CreateAndConfigureSubMethod("method_rotation_averaging");
        if (!rotation_averager_)
        {
            return nullptr;
        }

        // Configure rotation averaging parameters | 配置旋转平均参数
        rotation_averager_->SetMethodOptions({{"ProfileCommit", "GlobalSfM pipeline rotation averaging"}});

        // Prepare input data package - rotation averaging only needs relative pose data
        // 准备输入数据包 - 旋转平均只需要相对位姿数据
        auto input_package = std::make_shared<DataPackage>();

        // Check relative_poses_result type and add data correctly
        // 检查relative_poses_result的类型并正确添加数据
        if (auto package = std::dynamic_pointer_cast<DataPackage>(relative_poses_result))
        {
            auto relative_poses_data = package->GetData("data_relative_poses");
            if (relative_poses_data)
            {
                input_package->AddData("data_relative_poses", relative_poses_data);
            }
            else
            {
                LOG_ERROR_ZH << "无法从双视图估计结果中获取data_relative_poses";
                LOG_ERROR_EN << "Failed to get data_relative_poses from two-view estimation result";
                return nullptr;
            }
        }
        else
        {
            // Compatibility handling: directly add relative_poses_result as data_relative_poses
            // 兼容性处理：直接添加relative_poses_result作为data_relative_poses
            input_package->AddData("data_relative_poses", relative_poses_result);
        }

        // Execute rotation averaging | 执行旋转平均
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("step3_rotation_averaging"); // Mark Step 3 stage | 标记步骤3阶段
        auto result = rotation_averager_->Build(input_package);
        PROFILER_END();
        if (!result)
        {
            LOG_ERROR_ZH << "旋转平均失败";
            LOG_ERROR_EN << "Rotation averaging failed";
            return nullptr;
        }

        return result;
    }

    DataPtr GlobalSfMPipeline::Step4_TrackBuilding(DataPtr matches_data, DataPtr features_data)
    {
        // Create track builder | 创建轨迹构建器
        track_builder_ = CreateAndConfigureSubMethod("method_matches2tracks");
        if (!track_builder_)
        {
            return nullptr;
        }

        // Configure track building parameters | 配置轨迹构建参数
        track_builder_->SetMethodOptions({{"ProfileCommit", "GlobalSfM pipeline track building"}});

        // Prepare input data package | 准备输入数据包
        auto input_package = std::make_shared<DataPackage>();
        input_package->AddData(matches_data);
        input_package->AddData(features_data);

        // Execute track building | 执行轨迹构建
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("step4_track_building"); // Mark Step 4 stage | 标记步骤4阶段
        auto result = track_builder_->Build(input_package);
        PROFILER_END();
        if (!result)
        {
            LOG_ERROR_ZH << "轨迹构建失败";
            LOG_ERROR_EN << "Track building failed";
            return nullptr;
        }

        return result;
    }

    bool GlobalSfMPipeline::LoadGTFiles(const std::string &gt_folder, types::GlobalPoses &global_poses)
    {
        if (!std::filesystem::exists(gt_folder))
        {
            LOG_ERROR_ZH << "真值文件夹不存在: " << gt_folder;
            LOG_ERROR_EN << "Ground truth folder does not exist: " << gt_folder;
            return false;
        }

        // Find all .jpg.camera files | 查找所有.jpg.camera文件
        std::vector<std::filesystem::path> camera_files;
        for (const auto &entry : std::filesystem::directory_iterator(gt_folder))
        {
            if (entry.path().extension() == ".camera" &&
                entry.path().stem().extension() == ".jpg")
            {
                camera_files.push_back(entry.path());
            }
        }

        if (camera_files.empty())
        {
            LOG_ERROR_ZH << "未找到任何.jpg.camera文件: " << gt_folder;
            LOG_ERROR_EN << "No .jpg.camera files found: " << gt_folder;
            return false;
        }

        // Initialize global pose data | 初始化全局位姿数据
        global_poses.GetRotations().resize(camera_files.size(), types::Matrix3d::Identity());
        global_poses.GetTranslations().resize(camera_files.size(), types::Vector3d::Zero());
        global_poses.SetPoseFormat(types::PoseFormat::RwTw); // Strecha dataset uses RwTw format | Strecha数据集使用RwTw格式

        // Sort by filename to ensure correct order | 按文件名排序以确保顺序正确
        std::sort(camera_files.begin(), camera_files.end());

        // Read each file | 读取每个文件
        for (const auto &file_path : camera_files)
        {
            // Extract view ID | 提取视图ID
            std::string filename = file_path.stem().stem().string();
            int view_id = -1;
            try
            {
                view_id = std::stoi(filename);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "无法从文件名提取视图ID: " << filename;
                LOG_ERROR_EN << "Cannot extract view ID from filename: " << filename;
                continue;
            }

            // Open file | 打开文件
            std::ifstream file(file_path);
            if (!file.is_open())
            {
                LOG_ERROR_ZH << "无法打开文件: " << file_path;
                LOG_ERROR_EN << "Cannot open file: " << file_path;
                continue;
            }

            // Skip intrinsic matrix K (3 rows) | 跳过内参矩阵K (3行)
            std::string line;
            for (int i = 0; i < 3; ++i)
            {
                if (!std::getline(file, line))
                {
                    LOG_ERROR_ZH << "文件格式错误 (内参): " << file_path;
                    LOG_ERROR_EN << "File format error (intrinsics): " << file_path;
                    file.close();
                    return false;
                }
            }

            // Skip zero row | 跳过零行
            if (!std::getline(file, line))
            {
                LOG_ERROR_ZH << "文件格式错误 (零行): " << file_path;
                LOG_ERROR_EN << "File format error (zero row): " << file_path;
                file.close();
                return false;
            }

            // Read rotation matrix R (3 rows) | 读取旋转矩阵R (3行)
            types::Matrix3d R = types::Matrix3d::Identity();
            for (int row = 0; row < 3; ++row)
            {
                if (!std::getline(file, line))
                {
                    LOG_ERROR_ZH << "文件格式错误 (旋转矩阵): " << file_path;
                    LOG_ERROR_EN << "File format error (rotation matrix): " << file_path;
                    file.close();
                    return false;
                }

                std::istringstream iss(line);
                for (int col = 0; col < 3; ++col)
                {
                    if (!(iss >> R(row, col)))
                    {
                        LOG_ERROR_ZH << "无法解析旋转矩阵元素: " << line;
                        LOG_ERROR_EN << "Cannot parse rotation matrix element: " << line;
                        file.close();
                        return false;
                    }
                }
            }

            // Read translation vector t (1 row) | 读取平移向量t (1行)
            if (!std::getline(file, line))
            {
                LOG_ERROR_ZH << "文件格式错误 (平移向量): " << file_path;
                LOG_ERROR_EN << "File format error (translation vector): " << file_path;
                file.close();
                return false;
            }

            types::Vector3d t = types::Vector3d::Zero();
            std::istringstream iss_t(line);
            for (int i = 0; i < 3; ++i)
            {
                if (!(iss_t >> t(i)))
                {
                    LOG_ERROR_ZH << "无法解析平移向量元素: " << line;
                    LOG_ERROR_EN << "Cannot parse translation vector element: " << line;
                    file.close();
                    return false;
                }
            }

            file.close();

            // In Strecha dataset, camera parameters are Rw (world->camera) and tw (camera center in world coordinates)
            // That is RwTw format, so we can use them directly
            // 在Strecha数据集中，相机参数是Rw (世界->相机) 和 tw (相机中心在世界坐标系中的位置)
            // 也就是 RwTw 格式，所以我们可以直接使用
            global_poses.GetRotations()[view_id] = R.transpose(); // Strecha stores transpose of Rw | Strecha存储的是Rw的转置
            global_poses.GetTranslations()[view_id] = t;
        }

        LOG_INFO_ZH << "成功从 " << gt_folder << " 加载了 " << camera_files.size() << " 个真值相机位姿";
        LOG_INFO_EN << "Successfully loaded " << camera_files.size() << " ground truth camera poses from " << gt_folder;
        return true;
    }

    DataPtr GlobalSfMPipeline::EvaluatePoseAccuracy(DataPtr estimated_poses, const std::string &pose_type)
    {
        if (pose_type == "relative")
        {
            // Execute automatic evaluation before checking evaluation results for relative poses
            // 在检查相对位姿评估结果之前，先执行自动评估
            if (estimated_poses && GetGTData())
            {
                LOG_INFO_ZH << "开始执行PoSDK相对位姿自动评估...";
                LOG_INFO_EN << "Starting PoSDK relative pose automatic evaluation...";

                // Set evaluator algorithm name to distinguish PoSDK from OpenMVG relative poses
                // 设置评估器算法名称以区分PoSDK和OpenMVG相对位姿

                bool evaluation_success = CallEvaluator(estimated_poses);
                if (evaluation_success)
                {
                    LOG_INFO_ZH << "PoSDK相对位姿自动评估完成，结果已添加到EvaluatorManager";
                    LOG_INFO_EN << "PoSDK relative pose automatic evaluation completed, results added to EvaluatorManager";
                }
                else
                {
                    LOG_ERROR_ZH << "PoSDK相对位姿自动评估失败";
                    LOG_ERROR_EN << "PoSDK relative pose automatic evaluation failed";
                }
            }
            else
            {
                if (!estimated_poses)
                {
                    LOG_ERROR_ZH << "估计位姿数据为空，无法进行自动评估";
                    LOG_ERROR_EN << "Estimated pose data is empty, cannot perform automatic evaluation";
                }
                if (!GetGTData())
                {
                    LOG_WARNING_ZH << "真值数据未设置，无法进行PoSDK相对位姿自动评估";
                    LOG_WARNING_EN << "Ground truth data not set, cannot perform PoSDK relative pose automatic evaluation";
                }
            }
            // PrintRelativePosesAccuracy()
            return estimated_poses;
        }
        else if (pose_type == "global")
        {
            // Execute automatic evaluation before checking evaluation results
            // 在检查评估结果之前，先执行自动评估
            if (estimated_poses && GetGTData())
            {
                LOG_INFO_ZH << "开始执行全局位姿自动评估...";
                LOG_INFO_EN << "Starting automatic global pose evaluation...";
                bool evaluation_success = CallEvaluator(estimated_poses);
                if (evaluation_success)
                {
                    LOG_INFO_ZH << "全局位姿自动评估完成，结果已添加到EvaluatorManager";
                    LOG_INFO_EN << "Automatic global pose evaluation completed, results added to EvaluatorManager";
                }
                else
                {
                    LOG_ERROR_ZH << "全局位姿自动评估失败";
                    LOG_ERROR_EN << "Automatic global pose evaluation failed";
                }
            }
            else
            {
                if (!estimated_poses)
                {
                    LOG_ERROR_ZH << "估计位姿数据为空，无法进行自动评估";
                    LOG_ERROR_EN << "Estimated pose data is empty, cannot perform automatic evaluation";
                }
                if (!GetGTData())
                {
                    LOG_ERROR_ZH << "真值数据未设置，无法进行自动评估";
                    LOG_ERROR_EN << "Ground truth data not set, cannot perform automatic evaluation";
                }
            }
            // PrintGlobalPosesAccuracy()
            return estimated_poses;
        }
        else
        {
            LOG_ERROR_ZH << "未知的位姿类型: " << pose_type;
            LOG_ERROR_EN << "Unknown pose type: " << pose_type;
            return nullptr;
        }
    }

    void GlobalSfMPipeline::PrintRelativePosesAccuracy()
    {
        LOG_INFO_ZH << "=== 相对位姿精度评估结果检查 ===";
        LOG_INFO_ZH << "注意: 此函数仅检查评估数据是否存在，详细统计由PrintEvaluationResults统一处理";
        LOG_INFO_EN << "=== Relative Pose Accuracy Evaluation Results Check ===";
        LOG_INFO_EN << "Note: This function only checks if evaluation data exists, detailed statistics are handled by PrintEvaluationResults";

        // Use EvaluatorManager interface to get evaluation results
        // 使用EvaluatorManager接口获取评估结果
        const std::string eval_type = "RelativePoses";

        // Get all algorithms | 获取所有算法
        auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);

        LOG_DEBUG_ZH << "调试: 找到 " << algorithms.size() << " 个算法用于评估类型 " << eval_type;
        LOG_DEBUG_EN << "Debug: Found " << algorithms.size() << " algorithms for evaluation type " << eval_type;
        for (const auto &alg : algorithms)
        {
            LOG_DEBUG_ZH << "  - 算法: " << alg;
            LOG_DEBUG_EN << "  - Algorithm: " << alg;
        }

        if (algorithms.empty())
        {
            LOG_INFO_ZH << "未找到RelativePoses评估类型的算法";
            LOG_INFO_ZH << "提示: 确保two_view_estimator配置中enable_evaluator=true";
            LOG_INFO_EN << "No algorithms found for RelativePoses evaluation type";
            LOG_INFO_EN << "Hint: Ensure enable_evaluator=true in two_view_estimator configuration";

            // Show all available evaluation types | 显示所有可用的评估类型
            auto all_eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();
            LOG_INFO_ZH << "当前可用的评估类型: ";
            LOG_INFO_EN << "Currently available evaluation types: ";
            for (const auto &type : all_eval_types)
            {
                LOG_INFO_ZH << "  - " << type;
                LOG_INFO_EN << "  - " << type;
            }

            LOG_INFO_ZH << "未找到RelativePoses评估结果，跳过相对位姿精度评估";
            LOG_INFO_EN << "RelativePoses evaluation results not found, skipping relative pose accuracy evaluation";
        }

        bool found_results = false;

        // Iterate through all algorithms | 遍历所有算法
        for (const auto &algorithm : algorithms)
        {
            LOG_INFO_ZH << "====== 相对位姿评估结果 (算法: " << algorithm << ") ======";
            LOG_INFO_EN << "====== Relative Pose Evaluation Results (Algorithm: " << algorithm << ") ======";

            // Get all metrics for this algorithm | 获取该算法的所有指标
            auto metrics = Interface::EvaluatorManager::GetAllMetrics(eval_type, algorithm);

            for (const auto &metric : metrics)
            {
                LOG_INFO_ZH << "--- 指标: " << metric << " ---";
                LOG_INFO_EN << "--- Metric: " << metric << " ---";

                // Get all evaluation commits for this metric | 获取该指标的所有评估提交
                auto eval_commits = Interface::EvaluatorManager::GetAllEvalCommits(eval_type, algorithm, metric);

                // Get evaluator and extract data | 获取评估器并提取数据
                auto evaluator = Interface::EvaluatorManager::GetOrCreateEvaluator(eval_type, algorithm, metric);
                if (evaluator)
                {
                    for (const auto &eval_commit : eval_commits)
                    {
                        // Directly access eval_commit_data member | 直接访问eval_commit_data成员
                        auto it = evaluator->eval_commit_data.find(eval_commit);
                        if (it != evaluator->eval_commit_data.end() && !it->second.empty())
                        {
                            LOG_INFO_ZH << "评估配置: " << eval_commit;
                            LOG_INFO_EN << "Evaluation configuration: " << eval_commit;

                            // Confirm evaluation data found (detailed statistics will be handled by PrintEvaluationResults)
                            // 确认找到评估数据（详细统计将由PrintEvaluationResults统一处理）
                            auto stats = evaluator->GetStatistics(eval_commit);
                            LOG_INFO_ZH << "  找到 " << stats.count << " 个数据点";
                            LOG_INFO_EN << "  Found " << stats.count << " data points";

                            found_results = true;
                        }
                    }
                }
            }
        }

        // Export CSV results and print evaluation results | 导出CSV结果和打印评估结果
        if (found_results)
        {
            if (params_.base.enable_csv_export)
            {
                ExportSpecificEvaluationToCSV("RelativePoses");
            }

            PrintEvaluationResults(params_.base.evaluation_print_mode);
        }
        else
        {
            LOG_INFO_ZH << "未找到RelativePoses评估结果";
            LOG_INFO_ZH << "提示: 确保two_view_estimator配置中enable_evaluator=true";
            LOG_INFO_EN << "RelativePoses evaluation results not found";
            LOG_INFO_EN << "Hint: Ensure enable_evaluator=true in two_view_estimator configuration";
        }
    }

    void GlobalSfMPipeline::PrintGlobalPosesAccuracy()
    {
        LOG_INFO_ZH << "=== 全局位姿精度评估结果检查 ===";
        LOG_INFO_ZH << "注意: 此函数仅检查评估数据是否存在，详细统计由PrintEvaluationResults统一处理";
        LOG_INFO_EN << "=== Global Pose Accuracy Evaluation Results Check ===";
        LOG_INFO_EN << "Note: This function only checks if evaluation data exists, detailed statistics are handled by PrintEvaluationResults";

        // Get existing evaluation results from GlobalEvaluator | 从GlobalEvaluator获取已有的评估结果
        auto &global_evaluator = Interface::EvaluatorManager::GetGlobalEvaluator();

        // Search for global pose evaluation results | 查找全局位姿评估结果
        std::vector<std::string> possible_eval_types = {"GlobalPoses"};
        std::vector<std::string> possible_metrics = {"rotation_error", "position_error", "translation_error"};

        bool found_results = false;

        for (const auto &eval_type : possible_eval_types)
        {
            for (const auto &metric : possible_metrics)
            {
                Interface::EvaluationKey key(eval_type, pipeline_name_, metric);
                auto iter = global_evaluator.find(key);

                if (iter != global_evaluator.end() && iter->second)
                {
                    if (!found_results)
                    {
                        LOG_INFO_ZH << "====== 全局位姿评估结果 (来自GlobalEvaluator) ======";
                        LOG_INFO_EN << "====== Global Pose Evaluation Results (from GlobalEvaluator) ======";
                        found_results = true;
                    }

                    auto evaluation_data = iter->second;
                    for (const auto &[eval_commit, values] : evaluation_data->eval_commit_data)
                    {
                        if (!values.empty())
                        {
                            LOG_INFO_ZH << "评估类型: " << eval_type << ", 指标: " << metric;
                            LOG_INFO_ZH << "评估配置: " << eval_commit;
                            LOG_INFO_EN << "Evaluation type: " << eval_type << ", metric: " << metric;
                            LOG_INFO_EN << "Evaluation configuration: " << eval_commit;

                            std::string unit = (metric.find("rotation") != std::string::npos) ? "°" : "";
                            std::string title = metric;
                            if (metric == "rotation_error")
                                title = "旋转误差";
                            else if (metric == "position_error")
                                title = "位置误差";
                            else if (metric == "translation_error")
                                title = "平移误差";

                            // Confirm evaluation data found (detailed statistics will be handled by PrintEvaluationResults)
                            // 确认找到评估数据（详细统计将由PrintEvaluationResults统一处理）
                            auto stats = evaluation_data->GetStatistics(eval_commit);
                            LOG_INFO_ZH << "  找到 " << stats.count << " 个数据点";
                            LOG_INFO_EN << "  Found " << stats.count << " data points";
                        }
                    }
                }
            }
        }

        if (!found_results)
        {
            LOG_INFO_ZH << "未找到全局位姿评估结果";
            LOG_INFO_ZH << "提示: 确保相关方法配置中enable_evaluator=true";
            LOG_INFO_EN << "Global pose evaluation results not found";
            LOG_INFO_EN << "Hint: Ensure enable_evaluator=true in related method configurations";
        }
        else
        {
            // Export CSV results and print evaluation results | 导出CSV结果和打印评估结果
            if (params_.base.enable_csv_export)
            {
                // Export all possible global pose evaluation types | 导出所有可能的全局位姿评估类型
                std::vector<std::string> global_eval_types = {"GlobalPoses", "Poses", "GlobalPose"};
                for (const auto &eval_type : global_eval_types)
                {
                    auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);
                    if (!algorithms.empty())
                    {
                        ExportSpecificEvaluationToCSV(eval_type);
                    }
                }
            }

            PrintEvaluationResults(params_.base.evaluation_print_mode);
        }
    }

    void GlobalSfMPipeline::ExportAllEvaluationResultsToCSV()
    {
        LOG_INFO_ZH << "=== 导出所有评估结果到CSV ===";
        LOG_INFO_EN << "=== Export All Evaluation Results to CSV ===";

        // Get all evaluation types | 获取所有评估类型
        auto eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();

        if (eval_types.empty())
        {
            LOG_INFO_ZH << "未找到任何评估类型数据";
            LOG_INFO_EN << "No evaluation type data found";
            return;
        }

        LOG_INFO_ZH << "找到 " << eval_types.size() << " 个评估类型";
        LOG_INFO_EN << "Found " << eval_types.size() << " evaluation types";

        // Check if Performance evaluation type (time statistics) is included
        // 检查是否包含Performance评估类型（时间统计）
        bool has_performance = std::find(eval_types.begin(), eval_types.end(), "Performance") != eval_types.end();
        if (has_performance)
        {
            LOG_INFO_ZH << "✓ 检测到Performance评估类型，将导出时间统计结果";
            LOG_INFO_EN << "✓ Detected Performance evaluation type, will export time statistics results";
        }

        // Call specialized export function for each evaluation type | 为每个评估类型调用专门的导出函数
        for (const auto &eval_type : eval_types)
        {
            auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);
            if (!algorithms.empty())
            {
                ExportSpecificEvaluationToCSV(eval_type);

                // Provide special explanation for Performance type | 为Performance类型提供特殊说明
                if (eval_type == "Performance")
                {
                    LOG_INFO_ZH << "  → Performance评估包含时间统计: CoreTime(核心计算时间), TotalTime(总执行时间)";
                    LOG_INFO_EN << "  → Performance evaluation includes time statistics: CoreTime(core computation time), TotalTime(total execution time)";
                }
            }
            else
            {
                LOG_DEBUG_ZH << "跳过评估类型 " << eval_type << "（无算法数据）";
                LOG_DEBUG_EN << "Skip evaluation type " << eval_type << " (no algorithm data)";
            }
        }

        LOG_INFO_ZH << "所有评估结果CSV导出完成";
        LOG_INFO_EN << "All evaluation results CSV export completed";
    }

    void GlobalSfMPipeline::ExportSpecificEvaluationToCSV(const std::string &eval_type)
    {
        // Create CSV export directory | 创建CSV导出目录
        std::string dataset_name = current_dataset_name_;
        if (dataset_name.empty())
        {
            dataset_name = "unknown_dataset";
        }

        std::filesystem::path csv_output_dir = params_.base.work_dir + "/" + dataset_name + "/evaluation_csv";
        std::filesystem::create_directories(csv_output_dir);

        LOG_INFO_ZH << "=== 导出 " << eval_type << " 评估结果到CSV ===";
        LOG_INFO_ZH << "输出目录: " << csv_output_dir;
        LOG_INFO_EN << "=== Export " << eval_type << " Evaluation Results to CSV ===";
        LOG_INFO_EN << "Output directory: " << csv_output_dir;

        // Create independent subdirectory for specified evaluation type
        // 为指定评估类型创建独立的子目录
        std::filesystem::path eval_type_dir = csv_output_dir / eval_type;
        std::filesystem::create_directories(eval_type_dir);
        LOG_DEBUG_ZH << "创建评估类型目录: " << eval_type_dir;
        LOG_DEBUG_EN << "Create evaluation type directory: " << eval_type_dir;

        // Get all algorithms for this evaluation type | 获取该评估类型的所有算法
        auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);

        if (algorithms.empty())
        {
            LOG_DEBUG_ZH << "未找到评估类型 " << eval_type << " 的算法数据";
            LOG_DEBUG_EN << "No algorithm data found for evaluation type " << eval_type;
            return;
        }

        // Export detailed statistics | 导出详细统计
        for (const auto &algorithm : algorithms)
        {
            std::filesystem::path detailed_path = eval_type_dir / (algorithm + "_detailed.csv");
            bool detail_success = Interface::EvaluatorManager::ExportDetailedStatsToCSV(eval_type, algorithm, detailed_path);
            LOG_DEBUG_ZH << "导出详细统计 " << eval_type << "::" << algorithm << ": "
                         << (detail_success ? "成功" : "失败") << " -> " << detailed_path.filename();
            LOG_DEBUG_EN << "Export detailed statistics " << eval_type << "::" << algorithm << ": "
                         << (detail_success ? "success" : "failed") << " -> " << detailed_path.filename();
        }

        // Export metric comparisons | 导出指标对比
        auto metrics = Interface::EvaluatorManager::GetAllMetrics(eval_type, algorithms[0]);
        for (const auto &metric : metrics)
        {
            // Export algorithm comparison for single metric | 导出单个指标的算法对比
            std::filesystem::path comparison_path = eval_type_dir / (metric + "_comparison.csv");
            bool comparison_success = Interface::EvaluatorManager::ExportAlgorithmComparisonToCSV(
                eval_type, metric, comparison_path, "mean");
            LOG_DEBUG_ZH << "导出指标对比 " << eval_type << "::" << metric << ": "
                         << (comparison_success ? "成功" : "失败") << " -> " << comparison_path.filename();
            LOG_DEBUG_EN << "Export metric comparison " << eval_type << "::" << metric << ": "
                         << (comparison_success ? "success" : "failed") << " -> " << comparison_path.filename();

            // Export all statistics types | 导出所有统计类型
            std::filesystem::path all_stats_path = eval_type_dir / (metric + "_ALL_STATS.csv");

            bool all_stats_success = Interface::EvaluatorManager::ExportMetricAllStatsToCSV(
                eval_type, metric, all_stats_path);

            // Post-processing: Clean N/A rows in generated CSV files to improve table readability
            // 后处理：清理生成的CSV文件中的N/A行，提高表格可读性
            if (all_stats_success)
            {
                CleanCSVFile(all_stats_path);
            }

            LOG_DEBUG_ZH << "导出所有统计 " << eval_type << "::" << metric << ": "
                         << (all_stats_success ? "成功" : "失败") << " -> " << all_stats_path.filename();
            LOG_DEBUG_EN << "Export all statistics " << eval_type << "::" << metric << ": "
                         << (all_stats_success ? "success" : "failed") << " -> " << all_stats_path.filename();
        }

        // Export raw evaluation values to evaluation type subdirectory
        // 导出原始评估值到评估类型子目录
        std::filesystem::path raw_values_dir = eval_type_dir / "raw_values";
        bool raw_success = Interface::EvaluatorManager::ExportAllRawValuesToCSV(eval_type, raw_values_dir, "ALL");
        LOG_DEBUG_ZH << "导出原始评估值 " << eval_type << ": "
                     << (raw_success ? "成功" : "失败") << " -> raw_values/";
        LOG_DEBUG_EN << "Export raw evaluation values " << eval_type << ": "
                     << (raw_success ? "success" : "failed") << " -> raw_values/";

        LOG_INFO_ZH << eval_type << " CSV导出完成，文件保存在: " << eval_type_dir;
        LOG_INFO_EN << eval_type << " CSV export completed, files saved in: " << eval_type_dir;
    }
    // Clean CSV file by removing rows with excessive N/A values | 清理CSV文件，移除包含过多N/A值的行
    void GlobalSfMPipeline::CleanCSVFile(const std::filesystem::path &csv_file_path)
    {
        if (!std::filesystem::exists(csv_file_path))
        {
            LOG_DEBUG_ZH << "CSV清理: 文件不存在 " << csv_file_path;
            LOG_DEBUG_EN << "CSV cleanup: File does not exist " << csv_file_path;
            return;
        }

        // Read original file content | 读取原始文件内容
        std::ifstream input_file(csv_file_path);
        if (!input_file.is_open())
        {
            LOG_ERROR_ZH << "CSV清理: 无法打开文件 " << csv_file_path;
            LOG_ERROR_EN << "CSV cleanup: Unable to open file " << csv_file_path;
            return;
        }

        std::vector<std::string> clean_lines;
        std::string line;
        bool header_saved = false;
        int na_rows_removed = 0;
        int total_rows = 0;

        while (std::getline(input_file, line))
        {
            total_rows++;

            // Save header | 保存表头
            if (!header_saved)
            {
                clean_lines.push_back(line);
                header_saved = true;
                continue;
            }

            // Check if it's a meaningless N/A row | 检查是否为无意义的N/A行
            // Typical N/A rows contain consecutive N/A values: algorithm,evalcommit,N/A,N/A,N/A,N/A,N/A
            // 典型的N/A行包含连续的N/A值：algorithm,evalcommit,N/A,N/A,N/A,N/A,N/A
            bool is_meaningless_na_row = false;
            if (!line.empty())
            {
                // Count N/A occurrences | 计算N/A的出现次数
                size_t na_count = 0;
                size_t pos = 0;
                while ((pos = line.find("N/A", pos)) != std::string::npos)
                {
                    na_count++;
                    pos += 3;
                }

                // If N/A appears >= 4 times (at least 4 N/As in Mean, Median, Min, Max, StdDev), consider it meaningless
                // 如果N/A出现次数 >= 4（Mean, Median, Min, Max, StdDev中至少4个N/A），认为是无意义行
                is_meaningless_na_row = (na_count >= 4);
            }

            if (!is_meaningless_na_row)
            {
                clean_lines.push_back(line);
            }
            else
            {
                na_rows_removed++;
                LOG_DEBUG_ZH << "CSV清理: 移除N/A行 -> " << (line.length() > 80 ? line.substr(0, 80) + "..." : line);
                LOG_DEBUG_EN << "CSV cleanup: Remove N/A row -> " << (line.length() > 80 ? line.substr(0, 80) + "..." : line);
            }
        }
        input_file.close();

        // If no rows need to be removed, return directly | 如果没有需要移除的行，直接返回
        if (na_rows_removed == 0)
        {
            LOG_DEBUG_ZH << "CSV清理: 文件 " << csv_file_path.filename() << " 无需清理";
            LOG_DEBUG_EN << "CSV cleanup: File " << csv_file_path.filename() << " needs no cleanup";
            return;
        }

        // Rewrite file | 重写文件
        std::ofstream output_file(csv_file_path);
        if (!output_file.is_open())
        {
            LOG_ERROR_ZH << "CSV清理: 无法重写文件 " << csv_file_path;
            LOG_ERROR_EN << "CSV cleanup: Unable to rewrite file " << csv_file_path;
            return;
        }

        for (const auto &clean_line : clean_lines)
        {
            output_file << clean_line << std::endl;
        }
        output_file.close();

        LOG_INFO_ZH << "CSV清理: " << csv_file_path.filename() << " -> 移除 " << na_rows_removed << "/" << total_rows << " 个N/A行，保留 " << (clean_lines.size() - 1) << " 行有效数据";
        LOG_INFO_EN << "CSV cleanup: " << csv_file_path.filename() << " -> removed " << na_rows_removed << "/" << total_rows << " N/A rows, kept " << (clean_lines.size() - 1) << " valid data rows";
    }

    // Print evaluation results based on specified mode | 根据指定模式打印评估结果
    void GlobalSfMPipeline::PrintEvaluationResults(const std::string &print_mode)
    {
        if (print_mode == "none")
        {
            LOG_INFO_ZH << "评估结果打印已禁用";
            LOG_INFO_EN << "Evaluation result printing is disabled";
            return;
        }

        LOG_INFO_ZH << "=== 评估结果打印 (模式: " << print_mode << ") ===";
        LOG_INFO_EN << "=== Evaluation Result Printing (mode: " << print_mode << ") ===";

        if (print_mode == "summary")
        {
            // Print brief reports for all evaluation types | 打印所有评估类型的简要报告
            Interface::EvaluatorManager::PrintAllEvaluationReports();
        }
        else if (print_mode == "detailed")
        {
            // Print detailed evaluation reports | 打印详细的评估报告
            auto eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();
            for (const auto &eval_type : eval_types)
            {
                LOG_INFO_ZH << "--- 详细评估报告: " << eval_type << " ---";
                LOG_INFO_EN << "--- Detailed Evaluation Report: " << eval_type << " ---";
                Interface::EvaluatorManager::PrintEvaluationReport(eval_type);
            }
        }
        else if (print_mode == "comparison")
        {
            // Print algorithm comparison reports | 打印算法对比报告
            auto eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();
            for (const auto &eval_type : eval_types)
            {
                auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);
                if (!algorithms.empty())
                {
                    auto metrics = Interface::EvaluatorManager::GetAllMetrics(eval_type, algorithms[0]);
                    for (const auto &metric : metrics)
                    {
                        LOG_INFO_ZH << "--- 算法对比: " << eval_type << "::" << metric << " ---";
                        LOG_INFO_EN << "--- Algorithm Comparison: " << eval_type << "::" << metric << " ---";
                        Interface::EvaluatorManager::PrintAlgorithmComparison(eval_type, metric);
                    }
                }
            }
        }
        else
        {
            LOG_INFO_ZH << "未知的打印模式: " << print_mode << "，使用默认的summary模式";
            LOG_INFO_EN << "Unknown print mode: " << print_mode << ", using default summary mode";
            Interface::EvaluatorManager::PrintAllEvaluationReports();
        }
    }

    // Scan dataset directory to find all available datasets | 扫描数据集目录以查找所有可用数据集
    std::vector<std::string> GlobalSfMPipeline::ScanDatasetDirectory(const std::string &dataset_dir)
    {
        std::vector<std::string> dataset_list;

        try
        {
            if (!std::filesystem::exists(dataset_dir) || !std::filesystem::is_directory(dataset_dir))
            {
                LOG_ERROR_ZH << "数据集目录不存在或不是目录: " << dataset_dir;
                LOG_ERROR_EN << "Dataset directory does not exist or is not a directory: " << dataset_dir;
                return dataset_list;
            }

            // Scan dataset directory to find folders containing images subdirectory
            // 扫描数据集目录，查找包含images子目录的文件夹
            for (const auto &entry : std::filesystem::directory_iterator(dataset_dir))
            {
                if (entry.is_directory())
                {
                    std::string potential_dataset = entry.path().string();
                    std::string images_path = potential_dataset + "/images";

                    if (std::filesystem::exists(images_path) && std::filesystem::is_directory(images_path))
                    {
                        dataset_list.push_back(images_path);
                        LOG_DEBUG_ZH << "发现数据集: " << images_path;
                        LOG_DEBUG_EN << "Discovered dataset: " << images_path;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "扫描数据集目录时发生错误: " << e.what();
            LOG_ERROR_EN << "Error occurred while scanning dataset directory: " << e.what();
        }

        return dataset_list;
    }

    // Extract dataset name from image folder path | 从图像文件夹路径中提取数据集名称
    std::string GlobalSfMPipeline::ExtractDatasetName(const std::string &image_folder_path)
    {
        try
        {
            std::filesystem::path path(image_folder_path);
            // Get parent directory name of images folder as dataset name
            // 获取images文件夹的父目录名称作为数据集名称
            if (path.filename() == "images" && path.has_parent_path())
            {
                return path.parent_path().filename().string();
            }
            else
            {
                // If path doesn't end with /images, use the last directory name
                // 如果路径不是以/images结尾，使用最后一个目录名
                return path.filename().string();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "提取数据集名称时发生错误: " << e.what();
            LOG_ERROR_EN << "Error occurred while extracting dataset name: " << e.what();
            return "unknown_dataset";
        }
    }

    // Prepare camera model data from parameters | 从参数中准备相机模型数据
    Interface::DataPtr GlobalSfMPipeline::PrepareCameraModel()
    {
        auto camera_model_data = FactoryData::Create("data_camera_models");
        if (camera_model_data == nullptr)
        {
            LOG_ERROR_ZH << "无法创建相机模型数据";
            LOG_ERROR_EN << "Unable to create camera model data";
            return nullptr;
        }

        auto camera_models = GetDataPtr<types::CameraModels>(camera_model_data);
        if (camera_models == nullptr)
        {
            LOG_ERROR_ZH << "无法获取相机模型数据指针";
            LOG_ERROR_EN << "Unable to get camera model data pointer";
            return nullptr;
        }

        // Get camera intrinsics from OpenMVG parameters | 从OpenMVG参数中获取相机内参
        types::CameraModel camera;

        // Parse intrinsics string (format: fx,0,cx,0,fy,cy,0,0,1)
        // 解析内参字符串 (格式: fx,0,cx,0,fy,cy,0,0,1)
        std::string intrinsics_str = params_.openmvg.intrinsics;
        std::vector<double> intrinsics_values;

        // Replace semicolons with commas if any | 将分号替换为逗号（如果有的话）
        std::replace(intrinsics_str.begin(), intrinsics_str.end(), ';', ',');

        std::stringstream ss(intrinsics_str);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            try
            {
                intrinsics_values.push_back(std::stod(item));
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "解析内参失败: " << item;
                LOG_ERROR_EN << "Failed to parse intrinsics: " << item;
                return nullptr;
            }
        }

        if (intrinsics_values.size() >= 9)
        {
            // Standard 3x3 intrinsics matrix format: fx,0,cx,0,fy,cy,0,0,1
            // 标准3x3内参矩阵格式: fx,0,cx,0,fy,cy,0,0,1
            camera.SetCameraIntrinsics(
                intrinsics_values[0], // fx
                intrinsics_values[4], // fy
                intrinsics_values[2], // cx
                intrinsics_values[5], // cy
                3040,                 // width (Strecha dataset standard)
                2014                  // height (Strecha dataset standard)
            );
        }
        else
        {
            // Use default Strecha dataset intrinsics | 使用默认Strecha数据集内参
            LOG_WARNING_ZH << "内参格式不正确，使用默认Strecha数据集内参";
            LOG_WARNING_EN << "Incorrect intrinsics format, using default Strecha dataset intrinsics";
            camera.SetCameraIntrinsics(
                2759.48, // fx
                2764.16, // fy
                1520.69, // cx
                1006.81, // cy
                3040,    // width
                2014     // height
            );
        }

        // Set RADIAL_K3 distortion type | 设置RADIAL_K3畸变类型
        std::vector<double> radial_dist = {0.0, 0.0, 0.0}; // k1, k2, k3
        std::vector<double> tangential_dist = {};          // No tangential distortion
        camera.SetDistortionParams(types::DistortionType::RADIAL_K3, radial_dist, tangential_dist);

        camera_models->push_back(camera);

        const auto &intrinsics = camera.GetIntrinsics();
        LOG_INFO_ZH << "相机模型已准备: fx=" << intrinsics.GetFx()
                    << ", fy=" << intrinsics.GetFy()
                    << ", cx=" << intrinsics.GetCx()
                    << ", cy=" << intrinsics.GetCy();
        LOG_INFO_EN << "Camera model prepared: fx=" << intrinsics.GetFx()
                    << ", fy=" << intrinsics.GetFy()
                    << ", cx=" << intrinsics.GetCx()
                    << ", cy=" << intrinsics.GetCy();

        return camera_model_data;
    }

    // ==================== Unified table generation implementation | 统一制表功能实现 ====================
    // Generate summary table for all datasets | 为所有数据集生成汇总表格
    bool GlobalSfMPipeline::GenerateSummaryTable(const std::vector<std::string> &dataset_names)
    {
        if (dataset_names.empty())
        {
            LOG_INFO_ZH << "没有数据集，跳过生成汇总表格";
            LOG_INFO_EN << "No datasets, skipping summary table generation";
            return false;
        }

        LOG_INFO_ZH << "=== 开始生成汇总表格 (基于CSV文件合并) ===";
        LOG_INFO_ZH << "待处理数据集: " << dataset_names.size() << " 个";
        LOG_INFO_EN << "=== Starting summary table generation (based on CSV file merging) ===";
        LOG_INFO_EN << "Datasets to process: " << dataset_names.size();

        // Create output directory | 创建输出目录
        std::string summary_dir = params_.base.work_dir + "/summary";
        std::filesystem::create_directories(summary_dir);

        // Export profiler performance data to CSV | 导出性能分析数据到CSV
        std::string profiler_csv_path = summary_dir + "/profiler_performance_summary.csv";
        bool profiler_export_success = PoSDK::Profiler::ProfilerManager::GetInstance().ExportToCSV(profiler_csv_path);

        if (profiler_export_success)
        {
            LOG_INFO_ZH << "性能分析数据已导出到: " << profiler_csv_path;
            LOG_INFO_EN << "Profiler data exported to: " << profiler_csv_path;
        }
        else
        {
            LOG_WARNING_ZH << "性能分析数据导出失败";
            LOG_WARNING_EN << "Failed to export profiler data";
        }

        // Display all profiling data in console | 在控制台显示所有性能分析数据
        LOG_INFO_ZH << "\n=== 性能分析统计汇总 ===";
        LOG_INFO_EN << "\n=== Performance Profiling Statistics Summary ===";
        PoSDK::Profiler::ProfilerManager::GetInstance().DisplayAllProfilingData();

        // Get all possible evaluation types | 获取所有可能的评估类型
        std::vector<std::string> eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();
        if (eval_types.empty())
        {
            LOG_INFO_ZH << "未找到任何评估类型，跳过汇总表格生成";
            LOG_INFO_EN << "No evaluation types found, skipping summary table generation";
            return profiler_export_success; // Still return success if profiler export worked
        }

        int total_files_generated = 0;
        int total_successful = 0;

        // Generate summary table for each evaluation type | 为每个评估类型生成汇总表格
        for (const auto &eval_type : eval_types)
        {
            LOG_INFO_ZH << "--- 处理评估类型: " << eval_type << " ---";
            LOG_INFO_EN << "--- Processing evaluation type: " << eval_type << " ---";

            // Get all algorithms for this evaluation type | 获取该评估类型的所有算法
            auto all_algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);
            if (all_algorithms.empty())
            {
                LOG_INFO_ZH << "评估类型 " << eval_type << " 没有算法数据，跳过";
                LOG_INFO_EN << "Evaluation type " << eval_type << " has no algorithm data, skipping";
                continue;
            }

            // Get all metrics from the first algorithm as reference
            // 获取第一个算法的所有指标作为参考
            auto metrics = Interface::EvaluatorManager::GetAllMetrics(eval_type, all_algorithms[0]);
            for (const auto &metric : metrics)
            {
                total_files_generated++;
                if (GenerateSummaryTableForMetric(eval_type, metric, dataset_names, summary_dir))
                {
                    total_successful++;
                }
            }
        }

        LOG_INFO_ZH << "=== 汇总表格生成完成 ===";
        LOG_INFO_ZH << "处理了 " << eval_types.size() << " 个评估类型";
        LOG_INFO_ZH << "成功生成 " << total_successful << "/" << total_files_generated << " 个汇总文件";
        LOG_INFO_ZH << "输出目录: " << summary_dir;
        LOG_INFO_EN << "=== Summary table generation completed ===";
        LOG_INFO_EN << "Processed " << eval_types.size() << " evaluation types";
        LOG_INFO_EN << "Successfully generated " << total_successful << "/" << total_files_generated << " summary files";
        LOG_INFO_EN << "Output directory: " << summary_dir;

        return (total_successful > 0) || profiler_export_success;
    }

    // Generate summary table for a specific metric | 为特定指标生成汇总表格
    bool GlobalSfMPipeline::GenerateSummaryTableForMetric(const std::string &eval_type,
                                                          const std::string &metric,
                                                          const std::vector<std::string> &dataset_names,
                                                          const std::string &summary_dir)
    {
        LOG_DEBUG_ZH << "  生成指标 " << metric << " 的汇总表格";
        LOG_DEBUG_EN << "  Generating summary table for metric " << metric;

        // Generate summary table filename | 生成汇总表格文件名
        std::string summary_filename = "summary_" + eval_type + "_" + metric + "_ALL_STATS.csv";
        std::string summary_path = summary_dir + "/" + summary_filename;

        std::ofstream summary_file(summary_path);
        if (!summary_file.is_open())
        {
            LOG_ERROR_ZH << "无法创建汇总表格文件: " << summary_path;
            LOG_ERROR_EN << "Unable to create summary table file: " << summary_path;
            return false;
        }

        bool header_written = false;
        int datasets_processed = 0;
        int total_rows_merged = 0;

        // Iterate through all datasets and read corresponding CSV files
        // 遍历所有数据集，读取对应的CSV文件
        for (const auto &dataset_name : dataset_names)
        {
            // Construct CSV file path for this dataset | 构建该数据集的CSV文件路径
            std::string dataset_csv_path = params_.base.work_dir + "/" + dataset_name +
                                           "/evaluation_csv/" + eval_type + "/" + metric + "_ALL_STATS.csv";

            if (!std::filesystem::exists(dataset_csv_path))
            {
                LOG_DEBUG_ZH << "    数据集 " << dataset_name << " 缺少文件: " << metric << "_ALL_STATS.csv，跳过";
                LOG_DEBUG_EN << "    Dataset " << dataset_name << " missing file: " << metric << "_ALL_STATS.csv, skipping";
                continue;
            }

            std::ifstream dataset_file(dataset_csv_path);
            if (!dataset_file.is_open())
            {
                LOG_ERROR_ZH << "无法打开数据集CSV文件: " << dataset_csv_path;
                LOG_ERROR_EN << "Unable to open dataset CSV file: " << dataset_csv_path;
                continue;
            }

            std::string line;
            bool first_line = true;
            int rows_from_this_dataset = 0;

            while (std::getline(dataset_file, line))
            {
                if (first_line)
                {
                    // Process CSV header | 处理CSV表头
                    if (!header_written)
                    {
                        // Add Dataset field to first column | 在第一列添加Dataset字段
                        summary_file << "Dataset," << line << std::endl;
                        header_written = true;
                        LOG_DEBUG_ZH << "    写入表头: Dataset," << line;
                        LOG_DEBUG_EN << "    Writing header: Dataset," << line;
                    }
                    first_line = false;
                    continue;
                }

                // Process data rows: add dataset name to first column
                // 处理数据行：在第一列添加数据集名称
                if (!line.empty())
                {
                    summary_file << dataset_name << "," << line << std::endl;
                    rows_from_this_dataset++;
                    total_rows_merged++;
                }
            }

            dataset_file.close();

            if (rows_from_this_dataset > 0)
            {
                datasets_processed++;
                LOG_DEBUG_ZH << "    数据集 " << dataset_name << ": 合并了 "
                             << rows_from_this_dataset << " 行数据";
                LOG_DEBUG_EN << "    Dataset " << dataset_name << ": merged "
                             << rows_from_this_dataset << " rows of data";
            }
        }

        // Add specific footnote for CoreTime metric | 为CoreTime指标添加特定脚注
        if (metric == "CoreTime" && datasets_processed > 0)
        {
            std::string footnote = "Thread count: PoSDK(" + std::to_string(4) +
                                   ") | OpenMVG(" + std::to_string(params_.openmvg.num_threads) +
                                   ") | COLMAP(full) | GLOMAP(full)";
            summary_file << "\n# " << footnote << "\n";
            LOG_DEBUG_ZH << "    已添加CoreTime脚注: " << footnote;
            LOG_DEBUG_EN << "    Added CoreTime footnote: " << footnote;
        }

        summary_file.close();

        if (datasets_processed > 0)
        {
            LOG_INFO_ZH << "  ✓ " << eval_type << "::" << metric << " 汇总表格生成成功";
            LOG_INFO_ZH << "    -> " << summary_filename;
            LOG_INFO_ZH << "    -> 合并了 " << datasets_processed << " 个数据集, "
                        << total_rows_merged << " 行数据";
            LOG_INFO_EN << "  ✓ " << eval_type << "::" << metric << " summary table generated successfully";
            LOG_INFO_EN << "    -> " << summary_filename;
            LOG_INFO_EN << "    -> Merged " << datasets_processed << " datasets, "
                        << total_rows_merged << " rows of data";
            return true;
        }
        else
        {
            LOG_INFO_ZH << "  ✗ " << eval_type << "::" << metric << " 没有找到有效数据";
            LOG_INFO_EN << "  ✗ " << eval_type << "::" << metric << " no valid data found";
            // Delete empty file | 删除空文件
            std::filesystem::remove(summary_path);
            return false;
        }
    }

    // Export Meshlab project file | 导出Meshlab工程文件
    void GlobalSfMPipeline::ExportMeshlabProject(DataPtr global_poses_result, DataPtr reconstruction_result,
                                                 DataPtr camera_models, DataPtr images_data, const std::string &dataset_name)
    {
        if (!params_.base.enable_meshlab_export)
        {
            LOG_DEBUG_ZH << "Meshlab导出功能已禁用";
            LOG_DEBUG_EN << "Meshlab export function is disabled";
            return;
        }

        LOG_INFO_ZH << "=== [Meshlab导出] 开始导出Meshlab工程文件 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_EN << "=== [Meshlab Export] Starting Meshlab project file export ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;

        // Prepare export directory | 准备导出目录
        std::filesystem::path meshlab_export_base = params_.base.work_dir + "/" + dataset_name + "/meshlab_export";
        std::filesystem::create_directories(meshlab_export_base);

        std::string export_path = meshlab_export_base.string();
        LOG_INFO_ZH << "导出目录: " << export_path;
        LOG_INFO_EN << "Export directory: " << export_path;

        // Check input data | 检查输入数据
        if (!global_poses_result || !reconstruction_result || !camera_models || !images_data)
        {
            LOG_ERROR_ZH << "[Meshlab导出] 输入数据不完整，跳过导出";
            LOG_ERROR_EN << "[Meshlab Export] Incomplete input data, skipping export";
            if (!global_poses_result)
            {
                LOG_ERROR_ZH << "  - 缺少全局位姿数据";
                LOG_ERROR_EN << "  - Missing global pose data";
            }
            if (!reconstruction_result)
            {
                LOG_ERROR_ZH << "  - 缺少3D重建点数据";
                LOG_ERROR_EN << "  - Missing 3D reconstruction point data";
            }
            if (!camera_models)
            {
                LOG_ERROR_ZH << "  - 缺少相机模型数据";
                LOG_ERROR_EN << "  - Missing camera model data";
            }
            if (!images_data)
            {
                LOG_ERROR_ZH << "  - 缺少图像数据";
                LOG_ERROR_EN << "  - Missing image data";
            }
            return;
        }

        // Check validity of reconstruction points | 检查重建点的有效性
        auto reconstructed_points = GetDataPtr<WorldPointInfo>(reconstruction_result);
        if (!reconstructed_points || reconstructed_points->getValidPointsCount() == 0)
        {
            LOG_ERROR_ZH << "[Meshlab导出] 无有效的3D重建点，跳过导出";
            LOG_ERROR_EN << "[Meshlab Export] No valid 3D reconstruction points, skipping export";
            return;
        }

        LOG_INFO_ZH << "准备导出数据：";
        LOG_INFO_ZH << "  - 有效重建点数: " << reconstructed_points->getValidPointsCount();
        LOG_INFO_EN << "Preparing export data:";
        LOG_INFO_EN << "  - Valid reconstruction points: " << reconstructed_points->getValidPointsCount();

        // Get global pose data and check | 获取全局位姿数据并检查
        auto global_poses = GetDataPtr<types::GlobalPoses>(global_poses_result);
        if (global_poses)
        {
            LOG_INFO_ZH << "  - 全局位姿数: " << global_poses->Size();
            LOG_INFO_EN << "  - Global poses: " << global_poses->Size();
        }

        // Use unified ply filename: {dataset_name}_reconstruction.ply (same as Step7 output)
        // 使用统一的ply文件名: {dataset_name}_reconstruction.ply (与Step7输出一致)
        std::string unified_ply_filename = dataset_name + "_reconstruction.ply";

        // Call file::ExportToMeshLab for export | 调用file::ExportToMeshLab进行导出
        try
        {
            bool export_success = file::ExportToMeshLab(
                export_path,                // Export directory | 导出目录
                global_poses_result,        // Global pose data | 全局位姿数据
                camera_models,              // Camera model data | 相机模型数据
                reconstruction_result,      // 3D point data | 3D点数据
                images_data,                // Image data | 图像数据
                unified_ply_filename,       // PLY filename (unified) | PLY文件名（统一路径）
                dataset_name + "_scene.mlp" // Meshlab project filename | Meshlab工程文件名
            );

            if (export_success)
            {
                LOG_INFO_ZH << "[Meshlab导出] 成功导出到: " << export_path;
                LOG_INFO_ZH << "  - 工程文件: " << dataset_name << "_scene.mlp";
                LOG_INFO_ZH << "  - 点云文件: " << unified_ply_filename;
                LOG_INFO_ZH << "  - 重建点数: " << reconstructed_points->getValidPointsCount();
                LOG_INFO_EN << "[Meshlab Export] Successfully exported to: " << export_path;
                LOG_INFO_EN << "  - Project file: " << dataset_name << "_scene.mlp";
                LOG_INFO_EN << "  - Point cloud file: " << unified_ply_filename;
                LOG_INFO_EN << "  - Reconstruction points: " << reconstructed_points->getValidPointsCount();
            }
            else
            {
                LOG_ERROR_ZH << "[Meshlab导出] 导出失败";
                LOG_ERROR_EN << "[Meshlab Export] Export failed";
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[Meshlab导出] 异常: " << e.what();
            LOG_ERROR_EN << "[Meshlab Export] Exception: " << e.what();
        }
    }

    // Export PoSDK data to Colmap format | 导出PoSDK数据到Colmap格式
    void GlobalSfMPipeline::ExportPoSDK2Colmap(DataPtr global_poses_result, DataPtr camera_models, DataPtr features_data,
                                               DataPtr tracks_result, DataPtr reconstruction_result, const std::string &dataset_name)
    {
        if (!GetOptionAsBool("enable_posdk2colmap_export", false))
        {
            LOG_INFO_ZH << "PoSDK2Colmap导出功能已禁用，跳过导出";
            LOG_INFO_EN << "PoSDK2Colmap export function is disabled, skipping export";
            return;
        }

        LOG_INFO_ZH << "=== [PoSDK2Colmap导出] 开始导出Colmap格式数据 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_EN << "=== [PoSDK2Colmap Export] Starting Colmap format data export ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;

        // Prepare export directory | 准备导出目录
        std::filesystem::path posdk2colmap_path = params_.base.work_dir + "/" + dataset_name + "/posdk2colmap_export";
        std::filesystem::create_directories(posdk2colmap_path);

        std::string export_path = posdk2colmap_path.string();
        LOG_INFO_ZH << "导出目录: " << export_path;
        LOG_INFO_EN << "Export directory: " << export_path;

        // Check input data | 检查输入数据
        if (!global_poses_result || !camera_models || !features_data || !tracks_result)
        {
            LOG_ERROR_ZH << "[PoSDK2Colmap导出] 输入数据不完整，跳过导出";
            LOG_ERROR_EN << "[PoSDK2Colmap Export] Incomplete input data, skipping export";
            if (!global_poses_result)
            {
                LOG_ERROR_ZH << "  - 缺少全局位姿数据";
                LOG_ERROR_EN << "  - Missing global pose data";
            }
            if (!camera_models)
            {
                LOG_ERROR_ZH << "  - 缺少相机模型数据";
                LOG_ERROR_EN << "  - Missing camera model data";
            }
            if (!features_data)
            {
                LOG_ERROR_ZH << "  - 缺少特征数据";
                LOG_ERROR_EN << "  - Missing feature data";
            }
            if (!tracks_result)
            {
                LOG_ERROR_ZH << "  - 缺少轨迹数据";
                LOG_ERROR_EN << "  - Missing track data";
            }
            return;
        }

        // Get global pose data and check | 获取全局位姿数据并检查
        auto global_poses = GetDataPtr<GlobalPoses>(global_poses_result);
        if (global_poses)
        {
            LOG_INFO_ZH << "准备导出数据：";
            LOG_INFO_ZH << "  - 全局位姿数: " << global_poses->Size();
            LOG_INFO_EN << "Preparing export data:";
            LOG_INFO_EN << "  - Global poses: " << global_poses->Size();
        }

        // Note: reconstruction_result is optional for Colmap export
        // 注意：reconstruction_result对Colmap导出是可选的
        // Get WorldPointInfo from DataPoints3D | 从DataPoints3D获取WorldPointInfo
        types::WorldPointInfoPtr world_point_info_ptr = nullptr;
        if (reconstruction_result)
        {
            auto world_point_info = GetDataPtr<WorldPointInfo>(reconstruction_result);
            if (world_point_info)
            {
                world_point_info_ptr = world_point_info;
                size_t total_points = world_point_info->size();
                size_t valid_points = world_point_info->getValidPointsCount();
                LOG_INFO_ZH << "  - 3D重建点总数: " << total_points;
                LOG_INFO_ZH << "  - 有效3D点数量: " << valid_points;
                LOG_INFO_EN << "  - Total 3D reconstruction points: " << total_points;
                LOG_INFO_EN << "  - Valid 3D points: " << valid_points;
            }
            else
            {
                LOG_WARNING_ZH << "  - 无法从reconstruction_result获取WorldPointInfo数据";
                LOG_WARNING_EN << "  - Cannot get WorldPointInfo from reconstruction_result";
            }
        }

        LOG_INFO_ZH << "缩放方法: 最小相机间距离标准化为1.0（排除纯旋转）";
        LOG_INFO_EN << "Scaling method: Normalize minimum camera distance to 1.0 (excluding pure rotation)";

        // Call Colmap converter for export | 调用Colmap转换器进行导出
        try
        {
            auto tmp_points = world_point_info_ptr->GetWorldPoints();
            auto points_3d_ptr = std::make_shared<types::Points3d>(tmp_points);
            PoSDK::Converter::Colmap::OutputPoSDK2Colmap(
                posdk2colmap_path,
                GetDataPtr<GlobalPoses>(global_poses_result),
                GetDataPtr<CameraModels>(camera_models),
                GetDataPtr<FeaturesInfo>(features_data),
                GetDataPtr<Tracks>(tracks_result),
                points_3d_ptr);

            LOG_INFO_ZH << "[PoSDK2Colmap导出] 成功导出到: " << export_path;
            LOG_INFO_EN << "[PoSDK2Colmap Export] Successfully exported to: " << export_path;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[PoSDK2Colmap导出] 异常: " << e.what();
            LOG_ERROR_EN << "[PoSDK2Colmap Export] Exception: " << e.what();
        }
    }

    // Perform manual relative pose evaluation | 执行手动相对位姿评估
    void GlobalSfMPipeline::PerformManualRelativePoseEvaluation(DataPtr relative_poses_result, const std::string &dataset_name)
    {
        LOG_INFO_ZH << "=== [手动评估] 相对位姿精度评估 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_ZH << "目的: 验证自动评估结果的正确性";
        LOG_INFO_EN << "=== [Manual Evaluation] Relative pose accuracy evaluation ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;
        LOG_INFO_EN << "Purpose: Verify correctness of automatic evaluation results";

        // Load ground truth relative pose data (if not already loaded) | 加载真值相对位姿数据（如果尚未加载）
        if (gt_relative_poses_.empty() && !LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
        {
            LOG_ERROR_ZH << "[手动评估] 无法加载真值全局位姿数据";
            LOG_ERROR_EN << "[Manual Evaluation] Cannot load ground truth global pose data";
            return;
        }

        // Calculate relative pose ground truth from global poses (if not available) | 从全局位姿计算相对位姿真值（如果还没有）
        if (gt_relative_poses_.empty() && !gt_global_poses_.GetRotations().empty())
        {
            if (!types::Global2RelativePoses(gt_global_poses_, gt_relative_poses_))
            {
                LOG_ERROR_ZH << "[手动评估] 无法从全局位姿计算相对位姿真值";
                LOG_ERROR_EN << "[Manual Evaluation] Cannot compute relative pose ground truth from global poses";
                return;
            }
            LOG_DEBUG_ZH << "[手动评估] 从全局位姿成功计算 " << gt_relative_poses_.size() << " 个相对位姿真值";
            LOG_DEBUG_EN << "[Manual Evaluation] Successfully computed " << gt_relative_poses_.size() << " relative pose ground truths from global poses";
        }

        if (!gt_relative_poses_.empty())
        {
            types::RelativePoses estimated_relative_poses;
            // Get relative pose data from DataPackage or directly | 从DataPackage或直接获取相对位姿数据
            auto relative_poses_ptr = GetDataPtr<RelativePoses>(relative_poses_result, "data_relative_poses");
            if (!relative_poses_ptr)
            {
                relative_poses_ptr = GetDataPtr<RelativePoses>(relative_poses_result);
            }

            if (relative_poses_ptr && !relative_poses_ptr->empty())
            {
                estimated_relative_poses = *relative_poses_ptr;
                std::vector<double> rotation_errors, translation_errors;
                size_t matched_pairs = estimated_relative_poses.EvaluateAgainst(gt_relative_poses_,
                                                                                rotation_errors, translation_errors);

                if (matched_pairs > 0)
                {
                    LOG_INFO_ZH << "====== [手动评估] 相对位姿评估结果 ======";
                    LOG_INFO_ZH << "数据来源: 双视图位姿估计";
                    LOG_INFO_ZH << "匹配位姿对数量: " << matched_pairs << " / " << estimated_relative_poses.size();
                    LOG_INFO_EN << "====== [Manual Evaluation] Relative pose evaluation results ======";
                    LOG_INFO_EN << "Data source: Two-view pose estimation";
                    LOG_INFO_EN << "Matched pose pairs: " << matched_pairs << " / " << estimated_relative_poses.size();

                    // Calculate rotation error statistics | 计算旋转误差统计
                    if (!rotation_errors.empty())
                    {
                        double rot_sum = std::accumulate(rotation_errors.begin(), rotation_errors.end(), 0.0);
                        double rot_mean = rot_sum / static_cast<double>(rotation_errors.size());

                        std::vector<double> sorted_rot_errors = rotation_errors;
                        std::sort(sorted_rot_errors.begin(), sorted_rot_errors.end());
                        double rot_median = sorted_rot_errors.size() % 2 == 0 ? (sorted_rot_errors[sorted_rot_errors.size() / 2 - 1] + sorted_rot_errors[sorted_rot_errors.size() / 2]) / 2.0 : sorted_rot_errors[sorted_rot_errors.size() / 2];

                        double rot_min = *std::min_element(rotation_errors.begin(), rotation_errors.end());
                        double rot_max = *std::max_element(rotation_errors.begin(), rotation_errors.end());

                        LOG_INFO_ZH << "旋转误差 (" << rotation_errors.size() << " 组数据):";
                        LOG_INFO_ZH << "  平均值: " << std::fixed << std::setprecision(6) << rot_mean << "°";
                        LOG_INFO_ZH << "  中位数: " << rot_median << "°";
                        LOG_INFO_ZH << "  最小值: " << rot_min << "°";
                        LOG_INFO_ZH << "  最大值: " << rot_max << "°";
                        LOG_INFO_EN << "Rotation error (" << rotation_errors.size() << " data points):";
                        LOG_INFO_EN << "  Mean: " << std::fixed << std::setprecision(6) << rot_mean << "°";
                        LOG_INFO_EN << "  Median: " << rot_median << "°";
                        LOG_INFO_EN << "  Min: " << rot_min << "°";
                        LOG_INFO_EN << "  Max: " << rot_max << "°";
                    }

                    // Calculate translation direction error statistics | 计算平移方向误差统计
                    if (!translation_errors.empty())
                    {
                        double trans_sum = std::accumulate(translation_errors.begin(), translation_errors.end(), 0.0);
                        double trans_mean = trans_sum / static_cast<double>(translation_errors.size());

                        std::vector<double> sorted_trans_errors = translation_errors;
                        std::sort(sorted_trans_errors.begin(), sorted_trans_errors.end());
                        double trans_median = sorted_trans_errors.size() % 2 == 0 ? (sorted_trans_errors[sorted_trans_errors.size() / 2 - 1] + sorted_trans_errors[sorted_trans_errors.size() / 2]) / 2.0 : sorted_trans_errors[sorted_trans_errors.size() / 2];

                        double trans_min = *std::min_element(translation_errors.begin(), translation_errors.end());
                        double trans_max = *std::max_element(translation_errors.begin(), translation_errors.end());

                        LOG_INFO_ZH << "平移方向误差 (" << translation_errors.size() << " 组数据):";
                        LOG_INFO_ZH << "  平均值: " << std::fixed << std::setprecision(6) << trans_mean << "°";
                        LOG_INFO_ZH << "  中位数: " << trans_median << "°";
                        LOG_INFO_ZH << "  最小值: " << trans_min << "°";
                        LOG_INFO_ZH << "  最大值: " << trans_max << "°";
                        LOG_INFO_EN << "Translation direction error (" << translation_errors.size() << " data points):";
                        LOG_INFO_EN << "  Mean: " << std::fixed << std::setprecision(6) << trans_mean << "°";
                        LOG_INFO_EN << "  Median: " << trans_median << "°";
                        LOG_INFO_EN << "  Min: " << trans_min << "°";
                        LOG_INFO_EN << "  Max: " << trans_max << "°";
                    }

                    LOG_INFO_ZH << "====== [手动评估] 评估完成 ======";
                    LOG_INFO_ZH << "提示: 请对比此结果与自动评估结果，确保两者一致";
                    LOG_INFO_EN << "====== [Manual Evaluation] Evaluation completed ======";
                    LOG_INFO_EN << "Note: Please compare this result with automatic evaluation results to ensure consistency";
                }
                else
                {
                    LOG_ERROR_ZH << "[手动评估] 未找到匹配的位姿对，无法评估相对位姿精度";
                    LOG_ERROR_EN << "[Manual Evaluation] No matching pose pairs found, cannot evaluate relative pose accuracy";
                }
            }
            else
            {
                LOG_ERROR_ZH << "[手动评估] 无法从双视图估计结果中获取相对位姿数据";
                LOG_ERROR_EN << "[Manual Evaluation] Cannot get relative pose data from two-view estimation results";
            }
        }
        else
        {
            LOG_ERROR_ZH << "[手动评估] 未加载真值数据，跳过相对位姿精度评估";
            LOG_ERROR_EN << "[Manual Evaluation] Ground truth data not loaded, skipping relative pose accuracy evaluation";
        }
    }

    // Parse compared pipelines configuration | 解析对比流水线配置
    void GlobalSfMPipeline::ParseComparedPipelines()
    {
        // Reset all flags | 重置所有标志
        is_compared_openmvg_ = false;
        is_compared_colmap_ = false;
        is_compared_glomap_ = false;

        if (params_.base.compared_pipelines.empty())
        {
            LOG_DEBUG_ZH << "compared_pipelines为空，不启用任何对比流水线";
            LOG_DEBUG_EN << "compared_pipelines is empty, no comparison pipelines enabled";
            return;
        }

        // Use boost library to split compared_pipelines string | 使用boost库分割compared_pipelines字符串
        std::vector<std::string> pipeline_list;
        boost::split(pipeline_list, params_.base.compared_pipelines, boost::is_any_of(","));

        LOG_INFO_ZH << "解析对比流水线配置: " << params_.base.compared_pipelines;
        LOG_INFO_EN << "Parsing comparison pipeline configuration: " << params_.base.compared_pipelines;

        // Check each pipeline (case insensitive) | 检查每个流水线（大小写不敏感）
        for (const auto &pipeline : pipeline_list)
        {
            std::string trimmed_pipeline = boost::trim_copy(pipeline);

            if (boost::iequals(trimmed_pipeline, "openmvg"))
            {
                is_compared_openmvg_ = true;
                LOG_INFO_ZH << "  ✓ 启用OpenMVG对比流水线";
                LOG_INFO_EN << "  ✓ Enable OpenMVG comparison pipeline";
            }
            else if (boost::iequals(trimmed_pipeline, "colmap"))
            {
                is_compared_colmap_ = true;
                LOG_INFO_ZH << "  ✓ 启用Colmap对比流水线";
                LOG_INFO_EN << "  ✓ Enable Colmap comparison pipeline";
            }
            else if (boost::iequals(trimmed_pipeline, "glomap"))
            {
                is_compared_glomap_ = true;
                LOG_INFO_ZH << "  ✓ 启用Glomap对比流水线";
                LOG_INFO_EN << "  ✓ Enable Glomap comparison pipeline";
            }
            else if (!trimmed_pipeline.empty())
            {
                LOG_WARNING_ZH << "  ⚠ 未知的对比流水线: " << trimmed_pipeline;
                LOG_WARNING_EN << "  ⚠ Unknown comparison pipeline: " << trimmed_pipeline;
            }
        }

        // Output parsing result summary | 输出解析结果摘要
        std::vector<std::string> enabled_pipelines;
        if (is_compared_openmvg_)
            enabled_pipelines.push_back("OpenMVG");
        if (is_compared_colmap_)
            enabled_pipelines.push_back("Colmap");
        if (is_compared_glomap_)
            enabled_pipelines.push_back("Glomap");

        if (!enabled_pipelines.empty())
        {
            std::string summary = "已启用的对比流水线: ";
            for (size_t i = 0; i < enabled_pipelines.size(); ++i)
            {
                summary += enabled_pipelines[i];
                if (i < enabled_pipelines.size() - 1)
                    summary += ", ";
            }
            LOG_INFO_ZH << summary;

            summary = "Enabled comparison pipelines: ";
            for (size_t i = 0; i < enabled_pipelines.size(); ++i)
            {
                summary += enabled_pipelines[i];
                if (i < enabled_pipelines.size() - 1)
                    summary += ", ";
            }
            LOG_INFO_EN << summary;
        }
        else
        {
            LOG_INFO_ZH << "未启用任何对比流水线";
            LOG_INFO_EN << "No comparison pipelines enabled";
        }
    }

    // Evaluate OpenMVG global poses accuracy | 评估OpenMVG全局位姿精度
    void GlobalSfMPipeline::EvaluateOpenMVGGlobalPoses(const std::string &dataset_name)
    {
        LOG_INFO_ALL << " ";
        LOG_INFO_ZH << "=== [对比评估] OpenMVG全局位姿精度评估 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_ZH << "算法: openmvg_pipeline";
        LOG_INFO_EN << "=== [Comparison Evaluation] OpenMVG global pose accuracy evaluation ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;
        LOG_INFO_EN << "Algorithm: openmvg_pipeline";

        // Check if ground truth data has been loaded | 检查真值数据是否已加载
        if (gt_global_poses_.GetRotations().empty())
        {
            if (!params_.base.enable_evaluation || params_.base.gt_folder.empty())
            {
                LOG_DEBUG_ZH << "[对比评估] 未启用评估或未设置真值文件夹，跳过OpenMVG全局位姿评估";
                LOG_DEBUG_EN << "[Comparison Evaluation] Evaluation not enabled or ground truth folder not set, skipping OpenMVG global pose evaluation";
                return;
            }

            // Try to load ground truth data | 尝试加载真值数据
            if (!LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
            {
                LOG_ERROR_ZH << "[对比评估] 无法加载真值数据，跳过OpenMVG全局位姿评估";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load ground truth data, skipping OpenMVG global pose evaluation";
                return;
            }
        }

        // Build OpenMVG reconstruction result file path (using dynamically constructed path from parameters)
        // 构建OpenMVG重建结果文件路径（使用参数中动态构建的路径）
        std::string openmvg_reconstruction_dir = params_.openmvg.reconstruction_dir;
        LOG_DEBUG_ZH << "[对比评估] 使用OpenMVG重建目录: " << openmvg_reconstruction_dir;
        LOG_DEBUG_EN << "[Comparison Evaluation] Using OpenMVG reconstruction directory: " << openmvg_reconstruction_dir;

        // Check multiple possible file formats (prioritize .json files as in test_Strecha.cpp)
        // 检查多种可能的文件格式（参考test_Strecha.cpp优先检查.json文件）
        std::vector<std::string> possible_files = {
            openmvg_reconstruction_dir + "/sfm_data.json", // JSON format (reference test_Strecha.cpp) | JSON格式（参考test_Strecha.cpp）
            openmvg_reconstruction_dir + "/sfm_data.bin"   // Binary format | 二进制格式
        };

        std::string openmvg_sfm_data_file;
        bool file_found = false;
        for (const auto &file_path : possible_files)
        {
            if (std::filesystem::exists(file_path))
            {
                openmvg_sfm_data_file = file_path;
                file_found = true;
                break;
            }
        }

        if (!file_found)
        {
            LOG_ERROR_ZH << "[对比评估] OpenMVG重建结果文件不存在";
            LOG_ERROR_ZH << "检查过的路径:";
            LOG_ERROR_EN << "[Comparison Evaluation] OpenMVG reconstruction result file does not exist";
            LOG_ERROR_EN << "Checked paths:";
            for (const auto &file_path : possible_files)
            {
                LOG_DEBUG_ZH << "  - " << file_path << " (存在: " << (std::filesystem::exists(file_path) ? "是" : "否") << ")";
                LOG_DEBUG_EN << "  - " << file_path << " (exists: " << (std::filesystem::exists(file_path) ? "yes" : "no") << ")";
            }
            LOG_DEBUG_ZH << "当前工作目录: " << std::filesystem::current_path();
            LOG_DEBUG_ZH << "OpenMVG重建目录: " << openmvg_reconstruction_dir;
            LOG_DEBUG_EN << "Current working directory: " << std::filesystem::current_path();
            LOG_DEBUG_EN << "OpenMVG reconstruction directory: " << openmvg_reconstruction_dir;

            // Try to list files in reconstruction_dir | 尝试列出reconstruction_dir中的文件
            if (std::filesystem::exists(openmvg_reconstruction_dir))
            {
                LOG_DEBUG_ZH << "重建目录存在，其中包含的文件:";
                LOG_DEBUG_EN << "Reconstruction directory exists, contained files:";
                try
                {
                    for (const auto &entry : std::filesystem::directory_iterator(openmvg_reconstruction_dir))
                    {
                        LOG_DEBUG_ZH << "  - " << entry.path().filename();
                        LOG_DEBUG_EN << "  - " << entry.path().filename();
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "无法列出重建目录内容: " << e.what();
                    LOG_ERROR_EN << "Cannot list reconstruction directory contents: " << e.what();
                }
            }
            else
            {
                LOG_DEBUG_ZH << "重建目录不存在: " << openmvg_reconstruction_dir;
                LOG_DEBUG_EN << "Reconstruction directory does not exist: " << openmvg_reconstruction_dir;
            }

            LOG_DEBUG_ZH << "提示: 确保OpenMVG的SfM重建已成功完成，且enable_sfm_reconstruction=true";
            LOG_DEBUG_EN << "Hint: Ensure OpenMVG SfM reconstruction has completed successfully and enable_sfm_reconstruction=true";
            return;
        }

        LOG_INFO_ZH << "找到OpenMVG重建结果文件: " << openmvg_sfm_data_file;
        LOG_INFO_EN << "Found OpenMVG reconstruction result file: " << openmvg_sfm_data_file;

        // Use OpenMVGFileConverter to load global poses (reference test_Strecha.cpp implementation)
        // 使用OpenMVGFileConverter加载全局位姿（参考test_Strecha.cpp的实现）
        Interface::DataPtr openmvg_global_poses_data = nullptr;
        types::GlobalPoses openmvg_global_poses;
        bool openmvg_poses_loaded = false;

        try
        {
            // Create data_global_poses data object | 创建data_global_poses数据对象
            openmvg_global_poses_data = FactoryData::Create("data_global_poses");
            if (!openmvg_global_poses_data)
            {
                LOG_ERROR_ZH << "[对比评估] 无法创建data_global_poses数据对象";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot create data_global_poses data object";
                return;
            }

            // Use OpenMVGFileConverter to load global poses (reference test_Strecha.cpp)
            // 使用OpenMVGFileConverter加载全局位姿（参考test_Strecha.cpp）
            if (!PoSDK::Converter::OpenMVGFileConverter::ToDataGlobalPoses(openmvg_sfm_data_file, openmvg_global_poses_data))
            {
                LOG_ERROR_ZH << "[对比评估] 无法从OpenMVG SfM数据文件加载全局位姿";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load global poses from OpenMVG SfM data file";
                return;
            }

            auto openmvg_poses_ptr = GetDataPtr<types::GlobalPoses>(openmvg_global_poses_data);
            if (!openmvg_poses_ptr || openmvg_poses_ptr->GetRotations().empty())
            {
                LOG_ERROR_ZH << "[对比评估] OpenMVG SfM数据文件中没有有效的全局位姿数据";
                LOG_ERROR_EN << "[Comparison Evaluation] No valid global pose data in OpenMVG SfM data file";
                return;
            }

            openmvg_global_poses = *openmvg_poses_ptr; // Copy data for evaluation | 复制数据用于评估
            openmvg_poses_loaded = true;
            LOG_INFO_ZH << "成功加载OpenMVG全局位姿，共 " << openmvg_global_poses.Size() << " 个位姿";
            LOG_INFO_EN << "Successfully loaded OpenMVG global poses, total " << openmvg_global_poses.Size() << " poses";

            // Set evaluator algorithm name to distinguish from PoSDK global poses
            // 设置评估器算法名称以区分PoSDK全局位姿
            std::string original_algorithm = GetEvaluatorAlgorithm();
            SetEvaluatorAlgorithm("openmvg_pipeline");

            // Perform automatic evaluation using CallEvaluator | 使用CallEvaluator进行自动评估
            if (GetGTData())
            {
                LOG_INFO_ZH << "开始执行OpenMVG全局位姿自动评估...";
                LOG_INFO_EN << "Starting OpenMVG global pose automatic evaluation...";

                bool evaluation_success = CallEvaluator(openmvg_global_poses_data);
                if (evaluation_success)
                {
                    LOG_INFO_ZH << "✓ OpenMVG全局位姿自动评估完成，结果已添加到EvaluatorManager";
                    LOG_INFO_ZH << "  算法: openmvg_pipeline";
                    LOG_INFO_ZH << "  评估类型: GlobalPoses";
                    LOG_INFO_ZH << "  指标: rotation_error_deg, translation_error";
                    LOG_INFO_ZH << "注意: OpenMVG对比结果将在最终的评估报告中显示";
                    LOG_INFO_EN << "✓ OpenMVG global pose automatic evaluation completed, results added to EvaluatorManager";
                    LOG_INFO_EN << "  Algorithm: openmvg_pipeline";
                    LOG_INFO_EN << "  Evaluation type: GlobalPoses";
                    LOG_INFO_EN << "  Metrics: rotation_error_deg, translation_error";
                    LOG_INFO_EN << "Note: OpenMVG comparison results will be shown in the final evaluation report";
                }
                else
                {
                    LOG_ERROR_ZH << "✗ OpenMVG全局位姿自动评估失败";
                    LOG_ERROR_EN << "✗ OpenMVG global pose automatic evaluation failed";
                }
            }
            else
            {
                LOG_WARNING_ZH << "真值数据未设置，无法进行OpenMVG全局位姿自动评估";
                LOG_WARNING_EN << "Ground truth data not set, cannot perform OpenMVG global pose automatic evaluation";
            }

            // Restore original algorithm name | 恢复原始算法名称
            SetEvaluatorAlgorithm(original_algorithm);

            LOG_INFO_ZH << "====== [对比评估] OpenMVG评估完成 ======";
            LOG_INFO_EN << "====== [Comparison Evaluation] OpenMVG evaluation completed ======";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[对比评估] OpenMVG全局位姿评估异常: " << e.what();
            LOG_ERROR_EN << "[Comparison Evaluation] OpenMVG global pose evaluation exception: " << e.what();
        }
    }

    // Evaluate Colmap global poses accuracy | 评估Colmap全局位姿精度
    void GlobalSfMPipeline::EvaluateColmapGlobalPoses(const std::string &dataset_name)
    {
        LOG_INFO_ZH << "=== [对比评估] COLMAP全局位姿精度评估 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_ZH << "算法: colmap_pipeline";
        LOG_INFO_EN << "=== [Comparison Evaluation] COLMAP global pose accuracy evaluation ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;
        LOG_INFO_EN << "Algorithm: colmap_pipeline";

        // Check if ground truth data has been loaded | 检查真值数据是否已加载
        if (gt_global_poses_.GetRotations().empty())
        {
            if (!params_.base.enable_evaluation || params_.base.gt_folder.empty())
            {
                LOG_DEBUG_ZH << "[对比评估] 未启用评估或未设置真值文件夹，跳过COLMAP全局位姿评估";
                LOG_DEBUG_EN << "[Comparison Evaluation] Evaluation not enabled or ground truth folder not set, skipping COLMAP global pose evaluation";
                return;
            }

            // Try to load ground truth data | 尝试加载真值数据
            if (!LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
            {
                LOG_ERROR_ZH << "[对比评估] 无法加载真值数据，跳过COLMAP全局位姿评估";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load ground truth data, skipping COLMAP global pose evaluation";
                return;
            }
        }

        // Build COLMAP reconstruction result file path (using dynamically constructed path from parameters)
        // 构建COLMAP重建结果文件路径（使用参数中动态构建的路径）
        std::string colmap_work_dir = params_.base.work_dir + "/" + current_dataset_name_ + "_colmap_comparison";
        std::string colmap_sparse_dir = colmap_work_dir + "/sparse";

        LOG_DEBUG_ZH << "[对比评估] 使用COLMAP重建目录: " << colmap_work_dir;
        LOG_DEBUG_EN << "[Comparison Evaluation] Using COLMAP reconstruction directory: " << colmap_work_dir;

        // Check multiple possible COLMAP output paths
        // 检查多种可能的COLMAP输出路径
        std::vector<std::string> possible_model_dirs = {
            colmap_sparse_dir + "/0", // Standard COLMAP output path | 标准COLMAP输出路径
            colmap_sparse_dir,        // Sometimes directly in sparse directory | 有时直接在sparse目录
            colmap_work_dir + "/0"    // Alternative path | 备选路径
        };

        std::string colmap_model_dir;
        bool model_found = false;
        for (const auto &model_path : possible_model_dirs)
        {
            // Check if COLMAP model files exist (support both .txt and .bin formats)
            // 检查COLMAP模型文件是否存在（支持.txt和.bin格式）
            bool has_cameras = std::filesystem::exists(model_path + "/cameras.txt") ||
                               std::filesystem::exists(model_path + "/cameras.bin");
            bool has_images = std::filesystem::exists(model_path + "/images.txt") ||
                              std::filesystem::exists(model_path + "/images.bin");

            if (has_cameras && has_images)
            {
                colmap_model_dir = model_path;
                model_found = true;
                break;
            }
        }

        if (!model_found)
        {
            LOG_ERROR_ZH << "[对比评估] COLMAP重建结果文件不存在";
            LOG_ERROR_ZH << "检查过的路径:";
            LOG_ERROR_EN << "[Comparison Evaluation] COLMAP reconstruction result files do not exist";
            LOG_ERROR_EN << "Checked paths:";
            for (const auto &model_path : possible_model_dirs)
            {
                LOG_DEBUG_ZH << "  - " << model_path << "/cameras.txt (存在: " << (std::filesystem::exists(model_path + "/cameras.txt") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/cameras.bin (存在: " << (std::filesystem::exists(model_path + "/cameras.bin") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/images.txt (存在: " << (std::filesystem::exists(model_path + "/images.txt") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/images.bin (存在: " << (std::filesystem::exists(model_path + "/images.bin") ? "是" : "否") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/cameras.txt (exists: " << (std::filesystem::exists(model_path + "/cameras.txt") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/cameras.bin (exists: " << (std::filesystem::exists(model_path + "/cameras.bin") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/images.txt (exists: " << (std::filesystem::exists(model_path + "/images.txt") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/images.bin (exists: " << (std::filesystem::exists(model_path + "/images.bin") ? "yes" : "no") << ")";
            }

            // Try to list files in working directory | 尝试列出工作目录中的文件
            if (std::filesystem::exists(colmap_work_dir))
            {
                LOG_DEBUG_ZH << "COLMAP工作目录存在，其中包含的文件:";
                LOG_DEBUG_EN << "COLMAP working directory exists, containing files:";
                try
                {
                    for (const auto &entry : std::filesystem::recursive_directory_iterator(colmap_work_dir))
                    {
                        if (entry.is_regular_file())
                        {
                            LOG_DEBUG_ZH << "  - " << entry.path().string();
                            LOG_DEBUG_EN << "  - " << entry.path().string();
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "无法列出COLMAP工作目录内容: " << e.what();
                    LOG_ERROR_EN << "Cannot list COLMAP working directory contents: " << e.what();
                }
            }

            LOG_INFO_ZH << "提示: 确保COLMAP重建已成功完成";
            LOG_INFO_EN << "Hint: Ensure COLMAP reconstruction has completed successfully";
            return;
        }

        LOG_INFO_ZH << "找到COLMAP重建结果目录: " << colmap_model_dir;
        LOG_INFO_EN << "Found COLMAP reconstruction result directory: " << colmap_model_dir;

        // Use ColmapFileConverter to load global poses | 使用ColmapFileConverter加载全局位姿
        Interface::DataPtr colmap_global_poses_data = nullptr;
        types::GlobalPoses colmap_global_poses;

        try
        {
            // Create data_global_poses data object | 创建data_global_poses数据对象
            colmap_global_poses_data = FactoryData::Create("data_global_poses");
            if (!colmap_global_poses_data)
            {
                LOG_ERROR_ZH << "[对比评估] 无法创建data_global_poses数据对象";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot create data_global_poses data object";
                return;
            }

            // Create filename to ID mapping (read from sfm_data.json file)
            // 创建文件名到ID的映射（从sfm_data.json文件中读取）
            std::map<std::string, int> file_name_to_id;
            std::string sfm_data_file = colmap_work_dir + "/matches/sfm_data.json";

            if (!std::filesystem::exists(sfm_data_file))
            {
                LOG_ERROR_ZH << "[对比评估] sfm_data.json文件不存在: " << sfm_data_file;
                LOG_ERROR_EN << "[Comparison Evaluation] sfm_data.json file does not exist: " << sfm_data_file;
                return;
            }

            if (!PoSDK::Converter::Colmap::SfMFileToIdMap(sfm_data_file, file_name_to_id))
            {
                LOG_ERROR_ZH << "[对比评估] 无法从sfm_data.json创建文件名到ID映射";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot create filename to ID mapping from sfm_data.json";
                return;
            }

            LOG_DEBUG_ZH << "成功创建文件名到ID映射，包含 " << file_name_to_id.size() << " 个条目";
            LOG_DEBUG_EN << "Successfully created filename to ID mapping with " << file_name_to_id.size() << " entries";

            // Use ColmapFileConverter to load global poses (images.txt is in working directory root)
            // 使用ColmapFileConverter加载全局位姿（images.txt位于工作目录根目录）
            std::string global_poses_file = colmap_work_dir + "/images.txt";
            if (!PoSDK::Converter::Colmap::ToDataGlobalPoses(global_poses_file,
                                                             colmap_global_poses_data, file_name_to_id))
            {
                LOG_ERROR_ZH << "[对比评估] 无法从COLMAP模型文件加载全局位姿";
                LOG_ERROR_ZH << "注意: COLMAP需要先导出images.txt文件";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load global poses from COLMAP model files";
                LOG_ERROR_EN << "Note: COLMAP needs to export images.txt file first";
                return;
            }

            auto colmap_poses_ptr = GetDataPtr<types::GlobalPoses>(colmap_global_poses_data);
            if (!colmap_poses_ptr || colmap_poses_ptr->GetRotations().empty())
            {
                LOG_ERROR_ZH << "[对比评估] COLMAP模型文件中没有有效的全局位姿数据";
                LOG_ERROR_EN << "[Comparison Evaluation] No valid global pose data in COLMAP model files";
                return;
            }

            colmap_global_poses = *colmap_poses_ptr; // Copy data for evaluation | 复制数据用于评估
            LOG_INFO_ZH << "成功加载COLMAP全局位姿，共 " << colmap_global_poses.Size() << " 个位姿";
            LOG_INFO_EN << "Successfully loaded COLMAP global poses, total " << colmap_global_poses.Size() << " poses";

            // Set evaluator algorithm name to distinguish from PoSDK global poses
            // 设置评估器算法名称以区分PoSDK全局位姿
            std::string original_algorithm = GetEvaluatorAlgorithm();
            SetEvaluatorAlgorithm("colmap_pipeline");

            // Perform automatic evaluation using CallEvaluator | 使用CallEvaluator进行自动评估
            if (GetGTData())
            {
                LOG_INFO_ZH << "开始执行COLMAP全局位姿自动评估...";
                LOG_INFO_EN << "Starting COLMAP global pose automatic evaluation...";

                bool evaluation_success = CallEvaluator(colmap_global_poses_data);
                if (evaluation_success)
                {
                    LOG_INFO_ZH << "✓ COLMAP全局位姿自动评估完成，结果已添加到EvaluatorManager";
                    LOG_INFO_ZH << "  算法: colmap_pipeline";
                    LOG_INFO_ZH << "  评估类型: GlobalPoses";
                    LOG_INFO_ZH << "  指标: rotation_error_deg, translation_error";
                    LOG_INFO_ZH << "注意: COLMAP对比结果将在最终的评估报告中显示";
                    LOG_INFO_EN << "✓ COLMAP global pose automatic evaluation completed, results added to EvaluatorManager";
                    LOG_INFO_EN << "  Algorithm: colmap_pipeline";
                    LOG_INFO_EN << "  Evaluation type: GlobalPoses";
                    LOG_INFO_EN << "  Metrics: rotation_error_deg, translation_error";
                    LOG_INFO_EN << "Note: COLMAP comparison results will be shown in the final evaluation report";
                }
                else
                {
                    LOG_ERROR_ZH << "✗ COLMAP全局位姿自动评估失败";
                    LOG_ERROR_EN << "✗ COLMAP global pose automatic evaluation failed";
                }
            }
            else
            {
                LOG_WARNING_ZH << "真值数据未设置，无法进行COLMAP全局位姿自动评估";
                LOG_WARNING_EN << "Ground truth data not set, cannot perform COLMAP global pose automatic evaluation";
            }

            // Restore original algorithm name | 恢复原始算法名称
            SetEvaluatorAlgorithm(original_algorithm);

            LOG_INFO_ZH << "====== [对比评估] COLMAP评估完成 ======";
            LOG_INFO_EN << "====== [Comparison Evaluation] COLMAP evaluation completed ======";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[对比评估] COLMAP全局位姿评估异常: " << e.what();
            LOG_ERROR_EN << "[Comparison Evaluation] COLMAP global pose evaluation exception: " << e.what();
        }
    }

    // Evaluate Glomap global poses accuracy | 评估Glomap全局位姿精度
    void GlobalSfMPipeline::EvaluateGlomapGlobalPoses(const std::string &dataset_name)
    {
        LOG_INFO_ZH << "=== [对比评估] GLOMAP全局位姿精度评估 ===";
        LOG_INFO_ZH << "数据集: " << dataset_name;
        LOG_INFO_ZH << "算法: glomap_pipeline";
        LOG_INFO_EN << "=== [Comparison Evaluation] GLOMAP global pose accuracy evaluation ===";
        LOG_INFO_EN << "Dataset: " << dataset_name;
        LOG_INFO_EN << "Algorithm: glomap_pipeline";

        // Check if ground truth data has been loaded | 检查真值数据是否已加载
        if (gt_global_poses_.GetRotations().empty())
        {
            if (!params_.base.enable_evaluation || params_.base.gt_folder.empty())
            {
                LOG_DEBUG_ZH << "[对比评估] 未启用评估或未设置真值文件夹，跳过GLOMAP全局位姿评估";
                LOG_DEBUG_EN << "[Comparison Evaluation] Evaluation not enabled or ground truth folder not set, skipping GLOMAP global pose evaluation";
                return;
            }

            // Try to load ground truth data | 尝试加载真值数据
            if (!LoadGTFiles(params_.base.gt_folder, gt_global_poses_))
            {
                LOG_ERROR_ZH << "[对比评估] 无法加载真值数据，跳过GLOMAP全局位姿评估";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load ground truth data, skipping GLOMAP global pose evaluation";
                return;
            }
        }

        // Build GLOMAP reconstruction result file path | 构建GLOMAP重建结果文件路径
        std::string glomap_work_dir = params_.base.work_dir + "/" + current_dataset_name_ + "_glomap_comparison";
        std::string glomap_output_dir = glomap_work_dir + "/glomap_output";

        // Check multiple possible GLOMAP output paths | 检查多种可能的GLOMAP输出路径
        std::vector<std::string> possible_model_dirs = {
            glomap_output_dir + "/0", // Standard GLOMAP output path | 标准GLOMAP输出路径
            glomap_output_dir,        // Sometimes directly in output directory | 有时直接在output目录
            glomap_work_dir + "/0"    // Alternative path | 备选路径
        };

        std::string glomap_model_dir;
        bool model_found = false;
        for (const auto &model_path : possible_model_dirs)
        {
            // Check if GLOMAP model files exist (support both .txt and .bin formats)
            // 检查GLOMAP模型文件是否存在（支持.txt和.bin格式）
            bool has_cameras = std::filesystem::exists(model_path + "/cameras.txt") ||
                               std::filesystem::exists(model_path + "/cameras.bin");
            bool has_images = std::filesystem::exists(model_path + "/images.txt") ||
                              std::filesystem::exists(model_path + "/images.bin");

            if (has_cameras && has_images)
            {
                glomap_model_dir = model_path;
                model_found = true;
                break;
            }
        }

        if (!model_found)
        {
            LOG_ERROR_ZH << "[对比评估] GLOMAP重建结果文件不存在";
            LOG_ERROR_ZH << "检查过的路径:";
            LOG_ERROR_EN << "[Comparison Evaluation] GLOMAP reconstruction result files do not exist";
            LOG_ERROR_EN << "Checked paths:";
            for (const auto &model_path : possible_model_dirs)
            {
                LOG_DEBUG_ZH << "  - " << model_path << "/cameras.txt (存在: " << (std::filesystem::exists(model_path + "/cameras.txt") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/cameras.bin (存在: " << (std::filesystem::exists(model_path + "/cameras.bin") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/images.txt (存在: " << (std::filesystem::exists(model_path + "/images.txt") ? "是" : "否") << ")";
                LOG_DEBUG_ZH << "  - " << model_path << "/images.bin (存在: " << (std::filesystem::exists(model_path + "/images.bin") ? "是" : "否") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/cameras.txt (exists: " << (std::filesystem::exists(model_path + "/cameras.txt") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/cameras.bin (exists: " << (std::filesystem::exists(model_path + "/cameras.bin") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/images.txt (exists: " << (std::filesystem::exists(model_path + "/images.txt") ? "yes" : "no") << ")";
                LOG_DEBUG_EN << "  - " << model_path << "/images.bin (exists: " << (std::filesystem::exists(model_path + "/images.bin") ? "yes" : "no") << ")";
            }

            // Try to list files in working directory | 尝试列出工作目录中的文件
            if (std::filesystem::exists(glomap_work_dir))
            {
                LOG_DEBUG_ZH << "GLOMAP工作目录存在，其中包含的文件:";
                LOG_DEBUG_EN << "GLOMAP working directory exists, containing files:";
                try
                {
                    for (const auto &entry : std::filesystem::recursive_directory_iterator(glomap_work_dir))
                    {
                        if (entry.is_regular_file())
                        {
                            LOG_DEBUG_ZH << "  - " << entry.path().string();
                            LOG_DEBUG_EN << "  - " << entry.path().string();
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "无法列出GLOMAP工作目录内容: " << e.what();
                    LOG_ERROR_EN << "Cannot list GLOMAP working directory contents: " << e.what();
                }
            }

            LOG_INFO_ZH << "提示: 确保GLOMAP重建已成功完成";
            LOG_INFO_EN << "Hint: Ensure GLOMAP reconstruction has completed successfully";
            return;
        }

        LOG_INFO_ZH << "找到GLOMAP重建结果目录: " << glomap_model_dir;
        LOG_INFO_EN << "Found GLOMAP reconstruction result directory: " << glomap_model_dir;

        // Use ColmapFileConverter to load global poses (GLOMAP uses same format as COLMAP)
        // 使用ColmapFileConverter加载全局位姿（GLOMAP使用与COLMAP相同的格式）
        Interface::DataPtr glomap_global_poses_data = nullptr;
        types::GlobalPoses glomap_global_poses;

        try
        {
            // Create data_global_poses data object | 创建data_global_poses数据对象
            glomap_global_poses_data = FactoryData::Create("data_global_poses");
            if (!glomap_global_poses_data)
            {
                LOG_ERROR_ZH << "[对比评估] 无法创建data_global_poses数据对象";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot create data_global_poses data object";
                return;
            }

            // Create filename to ID mapping (read from sfm_data.json file)
            // 创建文件名到ID的映射（从sfm_data.json文件中读取）
            std::map<std::string, int> file_name_to_id;
            std::string sfm_data_file = glomap_work_dir + "/matches/sfm_data.json";

            if (!std::filesystem::exists(sfm_data_file))
            {
                LOG_ERROR_ZH << "[对比评估] sfm_data.json文件不存在: " << sfm_data_file;
                LOG_ERROR_EN << "[Comparison Evaluation] sfm_data.json file does not exist: " << sfm_data_file;
                return;
            }

            if (!PoSDK::Converter::Colmap::SfMFileToIdMap(sfm_data_file, file_name_to_id))
            {
                LOG_ERROR_ZH << "[对比评估] 无法从sfm_data.json创建文件名到ID映射";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot create filename to ID mapping from sfm_data.json";
                return;
            }

            LOG_DEBUG_ZH << "成功创建文件名到ID映射，包含 " << file_name_to_id.size() << " 个条目";
            LOG_DEBUG_EN << "Successfully created filename to ID mapping with " << file_name_to_id.size() << " entries";

            // Use ColmapFileConverter to load global poses (GLOMAP uses COLMAP format, images.txt is in working directory root)
            // 使用ColmapFileConverter加载全局位姿（GLOMAP使用COLMAP格式，images.txt位于工作目录根目录）
            std::string global_poses_file = glomap_work_dir + "/images.txt";
            if (!PoSDK::Converter::Colmap::ToDataGlobalPoses(global_poses_file,
                                                             glomap_global_poses_data, file_name_to_id))
            {
                LOG_ERROR_ZH << "[对比评估] 无法从GLOMAP模型文件加载全局位姿";
                LOG_ERROR_ZH << "注意: GLOMAP需要先导出images.txt文件";
                LOG_ERROR_EN << "[Comparison Evaluation] Cannot load global poses from GLOMAP model files";
                LOG_ERROR_EN << "Note: GLOMAP needs to export images.txt file first";
                return;
            }

            auto glomap_poses_ptr = GetDataPtr<types::GlobalPoses>(glomap_global_poses_data);
            if (!glomap_poses_ptr || glomap_poses_ptr->GetRotations().empty())
            {
                LOG_ERROR_ZH << "[对比评估] GLOMAP模型文件中没有有效的全局位姿数据";
                LOG_ERROR_EN << "[Comparison Evaluation] No valid global pose data in GLOMAP model files";
                return;
            }

            glomap_global_poses = *glomap_poses_ptr; // Copy data for evaluation | 复制数据用于评估
            LOG_INFO_ZH << "成功加载GLOMAP全局位姿，共 " << glomap_global_poses.Size() << " 个位姿";
            LOG_INFO_EN << "Successfully loaded GLOMAP global poses, total " << glomap_global_poses.Size() << " poses";

            // Set evaluator algorithm name to distinguish from PoSDK global poses
            // 设置评估器算法名称以区分PoSDK全局位姿
            std::string original_algorithm = GetEvaluatorAlgorithm();
            SetEvaluatorAlgorithm("glomap_pipeline");

            // Perform automatic evaluation using CallEvaluator | 使用CallEvaluator进行自动评估
            if (GetGTData())
            {
                LOG_INFO_ZH << "开始执行GLOMAP全局位姿自动评估...";
                LOG_INFO_EN << "Starting GLOMAP global pose automatic evaluation...";

                bool evaluation_success = CallEvaluator(glomap_global_poses_data);
                if (evaluation_success)
                {
                    LOG_INFO_ZH << "✓ GLOMAP全局位姿自动评估完成，结果已添加到EvaluatorManager";
                    LOG_INFO_ZH << "  算法: glomap_pipeline";
                    LOG_INFO_ZH << "  评估类型: GlobalPoses";
                    LOG_INFO_ZH << "  指标: rotation_error_deg, translation_error";
                    LOG_INFO_ZH << "注意: GLOMAP对比结果将在最终的评估报告中显示";
                    LOG_INFO_EN << "✓ GLOMAP global pose automatic evaluation completed, results added to EvaluatorManager";
                    LOG_INFO_EN << "  Algorithm: glomap_pipeline";
                    LOG_INFO_EN << "  Evaluation type: GlobalPoses";
                    LOG_INFO_EN << "  Metrics: rotation_error_deg, translation_error";
                    LOG_INFO_EN << "Note: GLOMAP comparison results will be shown in the final evaluation report";
                }
                else
                {
                    LOG_ERROR_ZH << "✗ GLOMAP全局位姿自动评估失败";
                    LOG_ERROR_EN << "✗ GLOMAP global pose automatic evaluation failed";
                }
            }
            else
            {
                LOG_WARNING_ZH << "真值数据未设置，无法进行GLOMAP全局位姿自动评估";
                LOG_WARNING_EN << "Ground truth data not set, cannot perform GLOMAP global pose automatic evaluation";
            }

            // Restore original algorithm name | 恢复原始算法名称
            SetEvaluatorAlgorithm(original_algorithm);

            LOG_INFO_ZH << "====== [对比评估] GLOMAP评估完成 ======";
            LOG_INFO_EN << "====== [Comparison Evaluation] GLOMAP evaluation completed ======";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[对比评估] GLOMAP全局位姿评估异常: " << e.what();
            LOG_ERROR_EN << "[Comparison Evaluation] GLOMAP global pose evaluation exception: " << e.what();
        }
    }

    // ==================== Data Statistics Feature Implementation | 数据统计功能实现 ====================

    // Initialize data statistics | 初始化数据统计
    void GlobalSfMPipeline::InitializeDataStatistics(const std::string &dataset_name)
    {
        try
        {
            // Build data statistics file path | 构建数据统计文件路径
            std::string dataset_work_dir = params_.base.work_dir + "/" + dataset_name;
            std::filesystem::create_directories(dataset_work_dir);

            data_statistics_file_path_ = dataset_work_dir + "/pipeline_data_statistics.md";

            // Close previously opened file stream | 关闭之前可能打开的文件流
            if (data_statistics_stream_.is_open())
            {
                data_statistics_stream_.close();
            }

            // Open new statistics file | 打开新的统计文件
            data_statistics_stream_.open(data_statistics_file_path_, std::ios::out | std::ios::trunc);
            if (!data_statistics_stream_.is_open())
            {
                LOG_ERROR_ZH << "无法创建数据统计文件: " << data_statistics_file_path_;
                LOG_ERROR_EN << "Cannot create data statistics file: " << data_statistics_file_path_;
                return;
            }

            // Write MD document header | 写入MD文档头部
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            data_statistics_stream_ << "# GlobalSfM Pipeline "
                                    << Interface::LanguageEnvironment::GetText("数据统计报告", "Data Statistics Report") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("数据集名称", "Dataset Name")
                                    << "**: " << dataset_name << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("生成时间", "Generation Time")
                                    << "**: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("预处理类型", "Preprocessing Type")
                                    << "**: " << GetPreprocessTypeStr() << "\n\n";
            data_statistics_stream_ << "---\n\n";

            data_statistics_stream_.flush();

            LOG_INFO_ZH << "数据统计功能已初始化，输出文件: " << data_statistics_file_path_;
            LOG_INFO_EN << "Data statistics feature initialized, output file: " << data_statistics_file_path_;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "初始化数据统计功能失败: " << e.what();
            LOG_ERROR_EN << "Failed to initialize data statistics feature: " << e.what();
        }
    }
    // Add step data statistics | 添加步骤数据统计
    void GlobalSfMPipeline::AddStepDataStatistics(const std::string &step_name, DataPtr data_ptr, const std::string &description)
    {
        if (!data_statistics_stream_.is_open() || !data_ptr)
        {
            return;
        }

        try
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            data_statistics_stream_ << "## " << step_name << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("描述", "Description")
                                    << "**: " << description << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("完成时间", "Completion Time")
                                    << "**: " << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "\n\n";

            // Note: Core time information is now managed by Profiler system | 注意：核心时间信息现在由Profiler系统管理
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("核心计算时间", "Core Computation Time")
                                    << "**: " << Interface::LanguageEnvironment::GetText("由Profiler系统管理", "Managed by Profiler system") << "\n\n";

            // Analyze data content | 分析数据内容
            std::string analysis = AnalyzeDataPtr(data_ptr);
            data_statistics_stream_ << analysis << "\n";

            data_statistics_stream_ << "---\n\n";
            data_statistics_stream_.flush();

            LOG_DEBUG_ZH << "已添加 " << step_name << " 的数据统计信息";
            LOG_DEBUG_EN << "Added data statistics for " << step_name;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "添加步骤数据统计失败 " << step_name << ": " << e.what();
            LOG_ERROR_EN << "Failed to add step data statistics " << step_name << ": " << e.what();
        }
    }

    // Analyze data pointer | 分析数据指针
    std::string GlobalSfMPipeline::AnalyzeDataPtr(DataPtr data_ptr)
    {
        if (!data_ptr)
        {
            return "**" + Interface::LanguageEnvironment::GetText("错误", "Error") + "**: " +
                   Interface::LanguageEnvironment::GetText("数据指针为空", "Data pointer is null") + "\n\n";
        }

        std::stringstream analysis;
        std::string data_type = data_ptr->GetType();

        analysis << "### " << Interface::LanguageEnvironment::GetText("数据类型分析", "Data Type Analysis") << "\n\n";
        analysis << "**" << Interface::LanguageEnvironment::GetText("数据类型", "Data Type") << "**: `" << data_type << "`\n\n";

        try
        {
            // Analyze according to data type | 根据数据类型进行具体分析
            if (data_type == "data_package" || data_type == "DataPackage")
            {
                // DataPackage type analysis | DataPackage类型分析
                auto data_package = std::dynamic_pointer_cast<DataPackage>(data_ptr);
                if (data_package)
                {
                    analysis << "**" << Interface::LanguageEnvironment::GetText("数据包类型", "Data Package Type")
                             << "**: DataPackage " << Interface::LanguageEnvironment::GetText("复合数据包", "Composite Data Package") << "\n\n";

                    // Try to get known data types | 尝试获取已知的数据类型
                    std::vector<std::string> known_keys = {"data_images", "data_features", "data_matches",
                                                           "data_relative_poses", "data_global_poses",
                                                           "data_tracks", "data_3d_points", "data_camera_models"};

                    int found_data_count = 0;
                    analysis << "**" << Interface::LanguageEnvironment::GetText("数据包内容", "Data Package Contents") << "**:\n\n";

                    for (const std::string &key : known_keys)
                    {
                        auto data = data_package->GetData(key);
                        if (data)
                        {
                            found_data_count++;
                            analysis << "- **" << key << "** (`" << data->GetType() << "`)\n";

                            // Recursively analyze detailed information of data within the package
                            // 递归分析包内数据的详细信息
                            std::string sub_analysis = AnalyzeSpecificDataType(data);
                            if (!sub_analysis.empty())
                            {
                                // Add indentation | 添加缩进
                                std::stringstream indented;
                                std::string line;
                                std::stringstream sub_stream(sub_analysis);
                                while (std::getline(sub_stream, line))
                                {
                                    indented << "  " << line << "\n";
                                }
                                analysis << indented.str();
                            }
                        }
                    }
                }
            }
            else
            {
                // Directly analyze specific data types | 直接分析特定数据类型
                std::string specific_analysis = AnalyzeSpecificDataType(data_ptr);
                analysis << specific_analysis;
            }
        }
        catch (const std::exception &e)
        {
            analysis << "**" << Interface::LanguageEnvironment::GetText("数据分析错误", "Data Analysis Error")
                     << "**: " << e.what() << "\n\n";
        }

        return analysis.str();
    }

    // Analyze specific data type | 分析特定数据类型
    std::string GlobalSfMPipeline::AnalyzeSpecificDataType(DataPtr data_ptr)
    {
        if (!data_ptr)
            return "";

        std::stringstream analysis;
        std::string data_type = data_ptr->GetType();

        try
        {
            if (data_type == "data_images" || data_type == "ImagePaths")
            {
                auto image_paths = GetDataPtr<ImagePaths>(data_ptr);
                if (image_paths)
                {
                    analysis << Interface::LanguageEnvironment::GetText("图像数量", "Image Count") << ": **" << image_paths->size() << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效图像数量", "Valid Image Count") << ": **" << std::count_if(image_paths->begin(), image_paths->end(), [](const auto &p)
                                                                                                                                        { return p.second; })
                             << "**\n";
                }
            }
            else if (data_type == "data_features" || data_type == "FeaturesInfo")
            {
                auto features_info = GetDataPtr<FeaturesInfo>(data_ptr);
                if (features_info)
                {
                    size_t total_features = 0;
                    size_t valid_images = 0;
                    for (const auto &image_feature : *features_info)
                    {
                        if (image_feature.GetNumFeatures() > 0)
                        {
                            total_features += image_feature.GetNumFeatures();
                            valid_images++;
                        }
                    }
                    analysis << Interface::LanguageEnvironment::GetText("特征图像数量", "Feature Image Count") << ": **" << valid_images << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("特征点总数", "Total Feature Points") << ": **" << total_features << "**\n";
                    if (valid_images > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("平均每图像特征点数", "Average Features Per Image") << ": **" << (total_features / valid_images) << "**\n";
                    }
                }
            }
            else if (data_type == "data_matches" || data_type == "Matches")
            {
                auto matches = GetDataPtr<Matches>(data_ptr);
                if (matches)
                {
                    size_t total_matches = 0;
                    for (const auto &[pair_id, match_data] : *matches)
                    {
                        total_matches += match_data.size();
                    }
                    analysis << Interface::LanguageEnvironment::GetText("图像对数量", "Image Pair Count") << ": **" << matches->size() << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("匹配点总数", "Total Match Points") << ": **" << total_matches << "**\n";
                    if (matches->size() > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("平均每对匹配数", "Average Matches Per Pair") << ": **" << (total_matches / matches->size()) << "**\n";
                    }
                }
            }
            else if (data_type == "data_relative_poses" || data_type == "RelativePoses")
            {
                auto relative_poses = GetDataPtr<RelativePoses>(data_ptr);
                if (relative_poses)
                {
                    // Count valid pose information (check if rotation and translation are non-zero)
                    // 统计有效位姿信息（判断旋转和平移是否为零）
                    size_t valid_poses = 0;
                    size_t total_poses = relative_poses->size();

                    for (const auto &pose : *relative_poses)
                    {
                        if (!pose.GetRotation().isIdentity() && !pose.GetTranslation().isZero())
                        {
                            valid_poses++;
                        }
                    }

                    // Estimate original image pair count: assuming N images, theoretical max pairs = N*(N-1)/2
                    // 估算原始图像对数量：假设有N张图像，理论最大图像对数量为 N*(N-1)/2
                    // Approximate image count from actual pose count | 从实际位姿数量反推图像数量（近似）
                    size_t estimated_image_count = static_cast<size_t>(std::sqrt(2 * total_poses) + 1);
                    size_t theoretical_max_pairs = estimated_image_count * (estimated_image_count - 1) / 2;

                    analysis << Interface::LanguageEnvironment::GetText("相对位姿数量", "Relative Pose Count") << ": **" << total_poses << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效相对位姿数", "Valid Relative Pose Count") << ": **" << valid_poses << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("估算图像数量", "Estimated Image Count") << ": **" << estimated_image_count << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("理论最大图像对数", "Theoretical Max Pairs") << ": **" << theoretical_max_pairs << "**\n";

                    if (total_poses > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("位姿质量比例", "Pose Quality Ratio") << ": **" << std::fixed << std::setprecision(2)
                                 << (100.0 * valid_poses / total_poses) << "%** (" << Interface::LanguageEnvironment::GetText("有效/实际", "Valid/Actual") << ")\n";
                    }

                    if (theoretical_max_pairs > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("图像对覆盖率", "Image Pair Coverage") << ": **" << std::fixed << std::setprecision(2)
                                 << (100.0 * total_poses / theoretical_max_pairs) << "%** (" << Interface::LanguageEnvironment::GetText("实际/理论", "Actual/Theoretical") << ")\n";
                    }
                }
            }
            else if (data_type == "data_global_poses" || data_type == "GlobalPoses")
            {
                auto global_poses = GetDataPtr<GlobalPoses>(data_ptr);
                if (global_poses)
                {
                    size_t total_poses = global_poses->Size(); // Actual number of processed poses | 实际处理的位姿数量

                    // Count valid poses (non-zero rotation or translation) | 统计有效位姿（非零的旋转或平移）
                    size_t valid_poses = 0;
                    for (size_t i = 0; i < global_poses->GetRotations().size(); ++i)
                    {
                        if (!global_poses->GetRotations()[i].isZero() || !global_poses->GetTranslations()[i].isZero())
                        {
                            valid_poses++;
                        }
                    }

                    analysis << Interface::LanguageEnvironment::GetText("图像数量（原始）", "Image Count (Original)") << ": **" << total_poses << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效位姿数量", "Valid Pose Count") << ": **" << valid_poses << "**\n";

                    if (total_poses > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("位姿重建成功率", "Pose Reconstruction Success Rate") << ": **" << std::fixed << std::setprecision(2)
                                 << (100.0 * valid_poses / total_poses) << "%** (" << Interface::LanguageEnvironment::GetText("有效/原始", "Valid/Original") << ")\n";
                    }
                }
            }
            else if (data_type == "data_tracks" || data_type == "Tracks")
            {
                auto tracks = GetDataPtr<Tracks>(data_ptr);
                if (tracks)
                {
                    size_t total_observations = 0;
                    size_t valid_tracks = 0;
                    size_t valid_observations = 0; // Added: valid observation count statistics | 新增：有效观测数量统计

                    for (const auto &track : *tracks)
                    {
                        total_observations += track.GetTrack().size();

                        if (track.GetTrack().size() >= 2) // Valid tracks need at least 2 observations | 至少需要2个观测的有效轨迹
                        {
                            valid_tracks++;
                        }

                        // Count valid observations | 统计有效观测数量
                        valid_observations += track.GetValidObservationCount();
                    }

                    analysis << Interface::LanguageEnvironment::GetText("轨迹总数", "Total Tracks") << ": **" << tracks->size() << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效轨迹数", "Valid Track Count") << ": **" << valid_tracks << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("观测总数", "Total Observations") << ": **" << total_observations << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效观测数", "Valid Observations") << ": **" << valid_observations << "**\n"; // Added display | 新增显示

                    if (valid_tracks > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("平均每轨迹观测数", "Average Observations Per Track") << ": **" << (total_observations / tracks->size()) << "**\n";
                        analysis << Interface::LanguageEnvironment::GetText("平均每有效轨迹观测数", "Average Observations Per Valid Track") << ": **" << (valid_observations / valid_tracks) << "**\n";
                    }

                    if (total_observations > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("观测有效率", "Observation Validity Rate") << ": **" << std::fixed << std::setprecision(2)
                                 << (100.0 * valid_observations / total_observations) << "%**\n";
                    }
                }
            }
            else if (data_type == "data_3d_points" || data_type == "WorldPointInfo")
            {
                auto world_points = GetDataPtr<WorldPointInfo>(data_ptr);
                if (world_points)
                {
                    size_t valid_points = world_points->getValidPointsCount();
                    size_t total_points = world_points->size();
                    analysis << Interface::LanguageEnvironment::GetText("3D点总数", "Total 3D Points") << ": **" << total_points << "**\n";
                    analysis << Interface::LanguageEnvironment::GetText("有效3D点数", "Valid 3D Points") << ": **" << valid_points << "**\n";
                    if (total_points > 0)
                    {
                        analysis << Interface::LanguageEnvironment::GetText("有效率", "Validity Rate") << ": **" << std::fixed << std::setprecision(2)
                                 << (100.0 * valid_points / total_points) << "%**\n";
                    }
                }
            }
            else
            {
                analysis << Interface::LanguageEnvironment::GetText("数据类型", "Data Type") << ": **" << data_type << "** (" << Interface::LanguageEnvironment::GetText("详细分析暂不支持", "Detailed analysis not supported") << ")\n";
            }
        }
        catch (const std::exception &e)
        {
            analysis << Interface::LanguageEnvironment::GetText("分析", "Analysis error for") << " " << data_type << ": " << e.what() << "\n";
        }

        return analysis.str();
    }

    // Add iteration data statistics | 添加迭代数据统计
    void GlobalSfMPipeline::AddIterationDataStatistics(int iteration, DataPtr tracks_result, DataPtr poses_result,
                                                       double angle_threshold)
    {
        if (!data_statistics_stream_.is_open())
        {
            return;
        }

        try
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            data_statistics_stream_ << "### " << Interface::LanguageEnvironment::GetText("迭代", "Iteration")
                                    << " " << iteration << " " << Interface::LanguageEnvironment::GetText("数据统计", "Data Statistics") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("时间", "Time")
                                    << "**: " << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("角度阈值", "Angle Threshold")
                                    << "**: " << std::fixed << std::setprecision(4) << angle_threshold << "°\n\n";

            // Analyze track data changes | 分析轨迹数据变化
            if (tracks_result)
            {
                data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("轨迹数据", "Track Data") << "**:\n\n";
                std::string tracks_analysis = AnalyzeSpecificDataType(tracks_result);
                data_statistics_stream_ << tracks_analysis << "\n";
            }

            // Analyze pose data | 分析位姿数据
            if (poses_result)
            {
                data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("位姿数据", "Pose Data") << "**:\n\n";
                std::string poses_analysis = AnalyzeSpecificDataType(poses_result);
                data_statistics_stream_ << poses_analysis << "\n";
            }

            data_statistics_stream_.flush();

            LOG_DEBUG_ZH << "已添加迭代 " << iteration << " 的数据统计信息";
            LOG_DEBUG_EN << "Added data statistics for iteration " << iteration;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "添加迭代数据统计失败 (迭代 " << iteration << "): " << e.what();
            LOG_ERROR_EN << "Failed to add iteration data statistics (iteration " << iteration << "): " << e.what();
        }
    }

    // Add Step6 final statistics | 添加Step6最终统计
    void GlobalSfMPipeline::AddStep6FinalStatistics(DataPtr final_poses,
                                                    const std::vector<double> &iteration_rot_errors,
                                                    const std::vector<double> &iteration_pos_errors,
                                                    const std::vector<std::string> &iteration_residual_info)
    {
        if (!data_statistics_stream_.is_open())
        {
            return;
        }

        try
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            data_statistics_stream_ << "### " << Interface::LanguageEnvironment::GetText("迭代优化最终结果", "Final Iterative Optimization Results") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("完成时间", "Completion Time") << "**: " << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "\n\n";

            // Analyze final pose data | 分析最终位姿数据
            if (final_poses)
            {
                data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("最终位姿数据", "Final Pose Data") << "**:\n\n";
                std::string poses_analysis = AnalyzeSpecificDataType(final_poses);
                data_statistics_stream_ << poses_analysis << "\n";
            }

            // Output iterative optimization summary information (if there is iteration evaluation data)
            // 输出迭代优化汇总信息（如果有迭代评估数据）
            if (!iteration_rot_errors.empty() && !iteration_pos_errors.empty())
            {
                data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("迭代优化汇总", "Iterative Optimization Summary") << "**:\n\n";

                // Iteration accuracy change table | 迭代精度变化表格
                data_statistics_stream_ << "| " << Interface::LanguageEnvironment::GetText("迭代次数", "Iteration")
                                        << " | " << Interface::LanguageEnvironment::GetText("旋转误差(°)", "Rotation Error(°)")
                                        << " | " << Interface::LanguageEnvironment::GetText("位置误差", "Position Error")
                                        << " | " << Interface::LanguageEnvironment::GetText("相对改进", "Relative Improvement") << " |\n";
                data_statistics_stream_ << "|----------|-------------|----------|----------|\n";

                for (size_t i = 0; i < iteration_rot_errors.size(); ++i)
                {
                    data_statistics_stream_ << "| " << (i + 1) << " | "
                                            << std::fixed << std::setprecision(6) << iteration_rot_errors[i]
                                            << " | " << iteration_pos_errors[i];

                    if (i > 0)
                    {
                        double rot_improvement = iteration_rot_errors[i - 1] - iteration_rot_errors[i];
                        double pos_improvement = iteration_pos_errors[i - 1] - iteration_pos_errors[i];
                        data_statistics_stream_ << " | " << Interface::LanguageEnvironment::GetText("旋转", "Rotation") << ": " << std::fixed << std::setprecision(4) << rot_improvement
                                                << "°, " << Interface::LanguageEnvironment::GetText("位置", "Position") << ": " << pos_improvement;
                    }
                    else
                    {
                        data_statistics_stream_ << " | " << Interface::LanguageEnvironment::GetText("初始值", "Initial Value");
                    }
                    data_statistics_stream_ << " |\n";
                }
                data_statistics_stream_ << "\n";

                // Residual and threshold information | 残差与阈值信息
                if (!iteration_residual_info.empty())
                {
                    data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("残差与阈值变化", "Residual and Threshold Changes") << "**:\n\n";
                    for (const auto &residual_info : iteration_residual_info)
                    {
                        data_statistics_stream_ << "- " << residual_info << "\n";
                    }
                    data_statistics_stream_ << "\n";
                }

                // Overall optimization effect | 总体优化效果
                if (iteration_rot_errors.size() > 1)
                {
                    double final_rot_error = iteration_rot_errors.back();
                    double final_pos_error = iteration_pos_errors.back();
                    double initial_rot_error = iteration_rot_errors[0];
                    double initial_pos_error = iteration_pos_errors[0];

                    data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("总体优化效果", "Overall Optimization Effect") << "**:\n\n";
                    data_statistics_stream_ << "- **" << Interface::LanguageEnvironment::GetText("初始误差", "Initial Error") << "**: " << Interface::LanguageEnvironment::GetText("旋转", "Rotation") << "=" << std::fixed << std::setprecision(6)
                                            << initial_rot_error << "°, " << Interface::LanguageEnvironment::GetText("位置", "Position") << "=" << initial_pos_error << "\n";
                    data_statistics_stream_ << "- **" << Interface::LanguageEnvironment::GetText("最终误差", "Final Error") << "**: " << Interface::LanguageEnvironment::GetText("旋转", "Rotation") << "=" << final_rot_error << "°, " << Interface::LanguageEnvironment::GetText("位置", "Position") << "=" << final_pos_error << "\n";
                    data_statistics_stream_ << "- **" << Interface::LanguageEnvironment::GetText("总体改进", "Overall Improvement") << "**: " << Interface::LanguageEnvironment::GetText("旋转", "Rotation") << "=" << (initial_rot_error - final_rot_error)
                                            << "°, " << Interface::LanguageEnvironment::GetText("位置", "Position") << "=" << (initial_pos_error - final_pos_error) << "\n\n";
                }
            }

            data_statistics_stream_ << "---\n\n";
            data_statistics_stream_.flush();

            LOG_DEBUG_ZH << "已添加Step6最终统计信息";
            LOG_DEBUG_EN << "Added Step6 final statistics";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "添加Step6最终统计失败: " << e.what();
            LOG_ERROR_EN << "Failed to add Step6 final statistics: " << e.what();
        }
    }

    // Finalize data statistics | 完成数据统计
    void GlobalSfMPipeline::FinalizeDataStatistics()
    {
        if (!data_statistics_stream_.is_open())
        {
            return;
        }

        try
        {
            // Write CSV summary information (if CSV export is enabled) | 写入CSV摘要信息（如果启用了CSV导出）
            if (params_.base.enable_csv_export)
            {
                data_statistics_stream_ << "## " << Interface::LanguageEnvironment::GetText("评估结果摘要", "Evaluation Results Summary") << "\n\n";
                data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("CSV文件位置", "CSV File Location") << "**: `work_dir/" << current_dataset_name_ << "/evaluation_csv/`\n\n";

                // Get all evaluation types and display | 获取所有评估类型并显示
                auto eval_types = Interface::EvaluatorManager::GetAllEvaluationTypes();
                if (!eval_types.empty())
                {
                    data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("可用的评估类型", "Available Evaluation Types") << "**:\n\n";
                    for (const auto &eval_type : eval_types)
                    {
                        auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms(eval_type);
                        if (!algorithms.empty())
                        {
                            data_statistics_stream_ << "- **" << eval_type << "**: ";
                            for (size_t i = 0; i < algorithms.size(); ++i)
                            {
                                data_statistics_stream_ << "`" << algorithms[i] << "`";
                                if (i < algorithms.size() - 1)
                                    data_statistics_stream_ << ", ";
                            }
                            data_statistics_stream_ << "\n";
                        }
                    }
                    data_statistics_stream_ << "\n";
                }
            }

            // Write time statistics summary | 写入时间统计摘要
            data_statistics_stream_ << "## " << Interface::LanguageEnvironment::GetText("时间统计摘要", "Time Statistics Summary") << "\n\n";

            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("核心时间统计", "Core Time Statistics") << "**: " << Interface::LanguageEnvironment::GetText("由Profiler系统管理", "Managed by Profiler system") << "\n\n";

            // Write summary information | 写入总结信息
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            data_statistics_stream_ << "## " << Interface::LanguageEnvironment::GetText("流水线完成", "Pipeline Completed") << "\n\n";
            data_statistics_stream_ << "**" << Interface::LanguageEnvironment::GetText("完成时间", "Completion Time") << "**: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
            data_statistics_stream_ << "---\n\n";
            data_statistics_stream_ << "*" << Interface::LanguageEnvironment::GetText("该报告由 GlobalSfMPipeline 自动生成", "This report is automatically generated by GlobalSfMPipeline") << "*\n";

            data_statistics_stream_.flush();
            data_statistics_stream_.close();

            LOG_INFO_ZH << "数据统计报告已完成，保存至: " << data_statistics_file_path_;
            LOG_INFO_EN << "Data statistics report completed, saved to: " << data_statistics_file_path_;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "完成数据统计时出错: " << e.what();
            LOG_ERROR_EN << "Error finalizing data statistics: " << e.what();
            if (data_statistics_stream_.is_open())
            {
                data_statistics_stream_.close();
            }
        }
    }

    // ==================== Time Statistics Feature Implementation | 时间统计功能实现 ====================

    // Note: Step core time recording is now handled by Profiler system | 注意：步骤核心时间记录现在由Profiler系统处理

    // Note: Step1 core time recording is now handled by Profiler system | 注意：Step1核心时间记录现在由Profiler系统处理

    // Note: Time statistics summary is now handled by Profiler system | 注意：时间统计摘要现在由Profiler系统处理

    // Evaluate time statistics | 评估时间统计
    void GlobalSfMPipeline::EvaluateTimeStatistics(const std::string &dataset_name)
    {
        LOG_INFO_ZH << "=== 时间统计评估 [" << dataset_name << "] ===";
        LOG_INFO_EN << "=== Time statistics evaluation [" << dataset_name << "] ===";

        // Get dataset core time from Profiler system | 从Profiler系统获取数据集核心时间
        // double dataset_core_time = PROFILER_GET_CURRENT_TIME(enable_profiling_);
        double dataset_core_time = 0;
        // Add current dataset's time statistics to evaluation system
        // 添加当前数据集的时间统计到评估系统
        AddDatasetTimeStatisticsToEvaluator(dataset_name, dataset_core_time);

        // Print time statistics results | 打印时间统计结果
        // PrintTimeStatisticsAccuracy(dataset_name);

        // Export time statistics CSV (export immediately to avoid being cleared by next dataset)
        // 导出时间统计CSV（立即导出，避免被下一个数据集清空）
        if (params_.base.enable_csv_export)
        {
            ExportSpecificEvaluationToCSV("Performance");
        }

        // Note: Time statistics are now managed by Profiler system | 注意：时间统计现在由Profiler系统管理
    }

    // Add dataset time statistics to evaluator | 添加数据集时间统计到评估器
    void GlobalSfMPipeline::AddDatasetTimeStatisticsToEvaluator(const std::string &dataset_name, double dataset_core_time)
    {
        LOG_INFO_ZH << "=== 添加数据集时间统计到评估系统 ===";
        LOG_INFO_EN << "=== Adding dataset time statistics to evaluation system ===";

        std::string algorithm = GetEvaluatorAlgorithm();
        std::string eval_commit = "GlobalSfM Pipeline - " + dataset_name;

        // Add core time statistics (unified format as integer milliseconds)
        // 添加核心时间统计（统一格式化为整数毫秒）
        bool core_time_added = Interface::EvaluatorManager::AddEvaluationResult(
            "Performance",                                  // eval_type
            algorithm,                                      // algorithm
            eval_commit,                                    // eval_commit
            "CoreTime",                                     // metric
            static_cast<int>(std::round(dataset_core_time)) // value (integer milliseconds, consistent with OpenMVG format)
        );

        if (core_time_added)
        {
            LOG_INFO_ZH << "✓ 数据集 [" << dataset_name << "] 时间统计已成功添加到评估系统";
            LOG_INFO_ZH << "  算法: " << algorithm;
            LOG_INFO_ZH << "  评估类型: Performance";
            LOG_INFO_ZH << "  指标: CoreTime(" << dataset_core_time << "ms)";
            LOG_INFO_ZH << "  评估配置: " << eval_commit;
            LOG_INFO_EN << "✓ Dataset [" << dataset_name << "] time statistics successfully added to evaluation system";
            LOG_INFO_EN << "  Algorithm: " << algorithm;
            LOG_INFO_EN << "  Evaluation type: Performance";
            LOG_INFO_EN << "  Metrics: CoreTime(" << dataset_core_time << "ms)";
            LOG_INFO_EN << "  Evaluation configuration: " << eval_commit;
        }
        else
        {
            LOG_ERROR_ZH << "✗ 添加数据集时间统计到评估系统失败";
            LOG_ERROR_EN << "✗ Failed to add dataset time statistics to evaluation system";
            LOG_ERROR_ZH << "  - 核心时间添加失败";
            LOG_ERROR_EN << "  - Core time addition failed";
        }
    }

    // Print time statistics accuracy | 打印时间统计精度
    DataPtr GlobalSfMPipeline::PrintTimeStatisticsAccuracy(const std::string &dataset_name)
    {
        LOG_INFO_ZH << "=== 时间统计结果检查 [" << dataset_name << "] ===";
        LOG_INFO_EN << "=== Time statistics result check [" << dataset_name << "] ===";

        // Get all algorithms of Performance evaluation type | 获取Performance评估类型的所有算法
        auto algorithms = Interface::EvaluatorManager::GetAllAlgorithms("Performance");

        if (algorithms.empty())
        {
            LOG_INFO_ZH << "未找到Performance评估类型的算法";
            LOG_INFO_EN << "No algorithms found for Performance evaluation type";
            return nullptr;
        }

        bool found_results = false;

        // Iterate through all algorithms | 遍历所有算法
        for (const auto &algorithm : algorithms)
        {
            LOG_INFO_ZH << "====== 时间统计结果 (算法: " << algorithm << ") ======";
            LOG_INFO_EN << "====== Time statistics results (algorithm: " << algorithm << ") ======";

            // Get all metrics for this algorithm | 获取该算法的所有指标
            auto metrics = Interface::EvaluatorManager::GetAllMetrics("Performance", algorithm);

            for (const auto &metric : metrics)
            {
                LOG_INFO_ZH << "--- 指标: " << metric << " ---";
                LOG_INFO_EN << "--- Metric: " << metric << " ---";

                // Get all evaluation commits for this metric | 获取该指标的所有评估提交
                auto eval_commits = Interface::EvaluatorManager::GetAllEvalCommits("Performance", algorithm, metric);

                // Get evaluator and extract data | 获取评估器并提取数据
                auto evaluator = Interface::EvaluatorManager::GetOrCreateEvaluator("Performance", algorithm, metric);
                if (evaluator)
                {
                    for (const auto &eval_commit : eval_commits)
                    {
                        auto it = evaluator->eval_commit_data.find(eval_commit);
                        if (it != evaluator->eval_commit_data.end() && !it->second.empty())
                        {
                            LOG_INFO_ZH << "评估配置: " << eval_commit;
                            LOG_INFO_EN << "Evaluation configuration: " << eval_commit;

                            // Get statistics | 获取统计x信息
                            auto stats = evaluator->GetStatistics(eval_commit);
                            std::string unit = (metric == "CoreTime") ? " ms" : "";
                            LOG_INFO_ZH << "  " << metric << ": " << stats.mean << unit << " (数据点: " << stats.count << ")";
                            LOG_INFO_EN << "  " << metric << ": " << stats.mean << unit << " (data points: " << stats.count << ")";

                            found_results = true;
                        }
                    }
                }
            }
        }

        if (!found_results)
        {
            LOG_INFO_ZH << "未找到时间统计结果";
            LOG_INFO_EN << "No time statistics results found";
        }
        else
        {
            // Note: Time statistics summary is now handled by Profiler system | 注意：时间统计摘要现在由Profiler系统处理
        }

        return nullptr; // Time statistics do not return data | 时间统计不返回数据
    }

    double GlobalSfMPipeline::ComputeCoordinateChanges(DataPtr current_tracks_data,
                                                       DataPtr initial_tracks_data)
    {
        LOG_INFO_ZH << "[GlobalSfMPipeline::ComputeCoordinateChanges] 开始计算轨迹坐标变化";
        LOG_INFO_EN << "[GlobalSfMPipeline::ComputeCoordinateChanges] Starting track coordinate change computation";

        if (!current_tracks_data || !initial_tracks_data)
        {
            LOG_ERROR_ZH << "[GlobalSfMPipeline] 错误: 输入的轨迹数据为空";
            LOG_ERROR_EN << "[GlobalSfMPipeline] Error: Input track data is null";
            return -1.0;
        }

        // Extract Tracks from DataPtr | 从DataPtr中提取Tracks
        auto current_tracks_ptr = GetDataPtr<Tracks>(current_tracks_data, "data_tracks");
        auto initial_tracks_ptr = GetDataPtr<Tracks>(initial_tracks_data, "data_tracks");

        if (!current_tracks_ptr || !initial_tracks_ptr)
        {
            LOG_ERROR_ZH << "[GlobalSfMPipeline] 错误: 无法从DataPtr中提取Tracks数据";
            LOG_ERROR_EN << "[GlobalSfMPipeline] Error: Failed to extract Tracks data from DataPtr";
            return -1.0;
        }

        const Tracks &current_tracks = *current_tracks_ptr;
        const Tracks &initial_tracks = *initial_tracks_ptr;

        double total_coordinate_change = 0.0;
        Size processed_observations = 0;

        // Check if track sizes match | 检查轨迹大小是否匹配
        if (current_tracks.size() != initial_tracks.size())
        {
            LOG_ERROR_ZH << "[GlobalSfMPipeline] 错误: 当前轨迹数量(" << current_tracks.size()
                         << ")与初始轨迹数量(" << initial_tracks.size() << ")不匹配";
            LOG_ERROR_EN << "[GlobalSfMPipeline] Error: Current tracks size(" << current_tracks.size()
                         << ") doesn't match initial tracks size(" << initial_tracks.size() << ")";
            return -1.0;
        }

        // Iterate through all tracks | 遍历所有轨迹
        for (Size track_idx = 0; track_idx < current_tracks.size(); ++track_idx)
        {
            const TrackInfo *current_track_ptr = current_tracks[track_idx];
            const TrackInfo *initial_track_ptr = initial_tracks[track_idx];

            if (!current_track_ptr || !initial_track_ptr ||
                !current_track_ptr->IsUsed() || !initial_track_ptr->IsUsed())
                continue;

            const auto &current_track = current_track_ptr->GetTrack();
            const auto &initial_track = initial_track_ptr->GetTrack();

            // Check if observation sizes match | 检查观测大小是否匹配
            if (current_track.size() != initial_track.size())
            {
                LOG_WARNING_ZH << "[GlobalSfMPipeline] 警告: 轨迹" << track_idx << "的观测数量不匹配";
                LOG_WARNING_EN << "[GlobalSfMPipeline] Warning: Track " << track_idx << " observation count mismatch";
                continue;
            }

            // Compare each observation | 比较每个观测
            for (Size obs_idx = 0; obs_idx < current_track.size(); ++obs_idx)
            {
                const auto &current_obs = current_track[obs_idx];
                const auto &initial_obs = initial_track[obs_idx];

                if (!current_obs.IsUsed() || !initial_obs.IsUsed())
                    continue;

                // Check if view IDs match | 检查视图ID是否匹配
                if (current_obs.GetViewId() != initial_obs.GetViewId())
                {
                    LOG_WARNING_ZH << "[GlobalSfMPipeline] 警告: 轨迹" << track_idx << "观测" << obs_idx << "的视图ID不匹配";
                    LOG_WARNING_EN << "[GlobalSfMPipeline] Warning: Track " << track_idx << " observation " << obs_idx << " view ID mismatch";
                    continue;
                }

                // Compute coordinate difference | 计算坐标差异
                const Vector2d &current_coord = current_obs.GetCoord();
                const Vector2d &initial_coord = initial_obs.GetCoord();

                Vector2d coord_diff = current_coord - initial_coord;
                double squared_change = coord_diff[0] * coord_diff[0] + coord_diff[1] * coord_diff[1];
                total_coordinate_change += squared_change;
                processed_observations++;
            }
        }

        // Calculate final coordinate change (0.5*sum(v*v)) | 计算最终坐标变化(0.5*sum(v*v))
        double final_coordinate_change = 0.5 * total_coordinate_change;

        LOG_INFO_ZH << "[GlobalSfMPipeline::ComputeCoordinateChanges] 坐标变化计算完成";
        LOG_INFO_ZH << "  - 处理观测数: " << processed_observations;
        LOG_INFO_ZH << "  - 总坐标变化 (0.5*sum(v*v)): " << final_coordinate_change;
        if (processed_observations > 0)
        {
            double avg_change = total_coordinate_change / processed_observations;
            LOG_INFO_ZH << "  - 平均坐标变化: " << avg_change << " 像素²";
        }

        LOG_INFO_EN << "[GlobalSfMPipeline::ComputeCoordinateChanges] Coordinate change computation completed";
        LOG_INFO_EN << "  - Observations processed: " << processed_observations;
        LOG_INFO_EN << "  - Total coordinate change (0.5*sum(v*v)): " << final_coordinate_change;
        if (processed_observations > 0)
        {
            double avg_change = total_coordinate_change / processed_observations;
            LOG_INFO_EN << "  - Average coordinate change: " << avg_change << " pixels²";
        }

        return final_coordinate_change;
    }

    // Register plugin | 注册插件
    // ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
    REGISTRATION_PLUGIN(PluginMethods::GlobalSfMPipeline);

} // namespace PluginMethods
