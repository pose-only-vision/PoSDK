/**
 * @file GlobalSfMPipelineParams.cpp
 * @brief GlobalSfM pipeline parameter configuration system implementation | GlobalSfM流水线参数配置系统实现
 * @copyright Copyright (c) 2024 PoSDK
 */

#include "GlobalSfMPipelineParams.hpp"
#include <po_core/po_logger.hpp>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <boost/algorithm/string.hpp>

namespace PluginMethods
{
    // ==================== PipelineParameters Implementation | PipelineParameters 实现 ====================

    void PipelineParameters::LoadFromConfig(Interface::MethodPreset *config_loader)
    {
        // Load basic parameters | 加载基础参数
        base.dataset_dir = config_loader->GetOptionAsPath("dataset_dir", "", "");
        base.image_folder = config_loader->GetOptionAsPath("image_folder", base.dataset_dir, "");
        base.work_dir = config_loader->GetOptionAsPath("work_dir", "", "{exe_dir}/globalsfm_pipeline_work");
        base.gt_folder = config_loader->GetOptionAsPath("gt_folder", base.dataset_dir, "");
        base.enable_evaluation = config_loader->GetOptionAsBool("enable_evaluation", true);
        base.max_iterations = static_cast<int>(config_loader->GetOptionAsIndexT("max_iterations", 3));
        base.enable_summary_table = config_loader->GetOptionAsBool("enable_summary_table", false);
        base.enable_matches_visualization = config_loader->GetOptionAsBool("enable_matches_visualization", false);
        base.enable_locker = config_loader->GetOptionAsBool("enable_locker", true);
        base.enable_csv_export = config_loader->GetOptionAsBool("enable_csv_export", true);
        base.enable_manual_eval = config_loader->GetOptionAsBool("enable_manual_eval", false);
        base.enable_3d_points_output = config_loader->GetOptionAsBool("enable_3d_points_output", false);
        base.enable_iter_evaluation = config_loader->GetOptionAsBool("enable_iter_evaluation", false);
        base.enable_meshlab_export = config_loader->GetOptionAsBool("enable_meshlab_export", false);
        base.enable_features_info_print = config_loader->GetOptionAsBool("enable_features_info_print", false);
        base.enable_data_statistics = config_loader->GetOptionAsBool("enable_data_statistics", false);
        base.evaluation_print_mode = config_loader->GetOptionAsString("evaluation_print_mode", "summary");
        base.compared_pipelines = config_loader->GetOptionAsString("compared_pipelines", "");

        // Load preprocessing type - use boost library for case-insensitive comparison
        // 加载预处理类型 - 使用boost库兼容大小写的方式
        std::string preprocess_type_str = config_loader->GetOptionAsString("preprocess_type", "openmvg");

        // Use boost::iequals for case-insensitive string comparison
        // 使用boost::iequals进行大小写不敏感的字符串比较
        if (boost::iequals(preprocess_type_str, "openmvg"))
        {
            base.preprocess_type = PreprocessType::OpenMVG;
            LOG_DEBUG_ZH << "预处理类型识别: " << preprocess_type_str << " -> OpenMVG";
            LOG_DEBUG_EN << "Preprocessing type recognized: " << preprocess_type_str << " -> OpenMVG";
        }
        else if (boost::iequals(preprocess_type_str, "opencv"))
        {
            base.preprocess_type = PreprocessType::OpenCV;
            LOG_DEBUG_ZH << "预处理类型识别: " << preprocess_type_str << " -> OpenCV (method_img2matches)";
            LOG_DEBUG_EN << "Preprocessing type recognized: " << preprocess_type_str << " -> OpenCV (method_img2matches)";
        }
        else if (boost::iequals(preprocess_type_str, "posdk"))
        {
            base.preprocess_type = PreprocessType::PoSDK;
            LOG_DEBUG_ZH << "预处理类型识别: " << preprocess_type_str << " -> PoSDK (posdk_preprocessor)";
            LOG_DEBUG_EN << "Preprocessing type recognized: " << preprocess_type_str << " -> PoSDK (posdk_preprocessor)";
        }
        else
        {
            LOG_WARNING_ZH << "未知的预处理类型: " << preprocess_type_str << "，使用默认的PoSDK";
            LOG_WARNING_EN << "Unknown preprocessing type: " << preprocess_type_str << ", using default PoSDK";
            base.preprocess_type = PreprocessType::PoSDK;
        }

        base.profile_commit = config_loader->GetOptionAsString("ProfileCommit", "GlobalSfM Pipeline");

        // Load OpenMVG parameters - use default values, actual parameters passed through PassingMethodOptions
        // 加载OpenMVG参数 - 使用默认值，实际参数通过PassingMethodOptions传递
        // Note: These parameters are now defined in [openmvg_pipeline] section, automatically passed through PassingMethodOptions
        // 注意：这些参数现在在[openmvg_pipeline]section中定义，通过PassingMethodOptions自动传递
        openmvg.root_dir = base.dataset_dir; // Set at runtime | 运行时设置
        openmvg.dataset_dir = base.dataset_dir;
        openmvg.images_folder = base.image_folder;
        openmvg.work_dir = base.work_dir;
        // Set reconstruction_dir default value, consistent with default value in [openmvg_pipeline] section
        // 设置reconstruction_dir默认值，与[openmvg_pipeline]section中的默认值保持一致
        // This value will be combined with dataset_work_dir in UpdateDynamicParameters to build complete path
        // 这个值会在UpdateDynamicParameters中与dataset_work_dir组合构建完整路径
        openmvg.reconstruction_dir = "reconstruction_global";
        // Other OpenMVG parameters use struct default values, actual values loaded from [openmvg_pipeline] section
        // 其他OpenMVG参数使用结构体默认值，实际值从[openmvg_pipeline]section加载

        // Other parameters use default values, set dynamically at runtime
        // 其他参数使用默认值，运行时动态设置
    }

    void PipelineParameters::UpdateDynamicParameters(const std::string &dataset_name)
    {
        // Create independent working directory for each dataset
        // 为每个数据集创建独立的工作目录
        std::string dataset_work_dir = base.work_dir + "/" + dataset_name;

        // Automatically set gt_folder based on dataset name
        // 根据数据集名称自动设置gt_folder
        // In batch processing mode, gt_folder needs to be updated for each dataset
        // 在批处理模式下，每个数据集都需要更新gt_folder
        if (!base.dataset_dir.empty())
        {
            base.gt_folder = base.dataset_dir + "/" + dataset_name + "/gt_dense_cameras";
        }

        // Update OpenMVG parameters using dataset-specific working directory
        // 更新OpenMVG参数，使用数据集特定的工作目录
        openmvg.images_folder = base.image_folder; // Set current processing image folder | 设置当前处理的图像文件夹
        openmvg.dataset_dir = base.dataset_dir;    // Set dataset root directory | 设置数据集根目录
        openmvg.work_dir = dataset_work_dir;       // Set dataset-specific working directory | 设置数据集特定的工作目录
        openmvg.sfm_data_filename = dataset_work_dir + "/sfm_data.bin";
        openmvg.matches_dir = dataset_work_dir + "/matches";
        // Use original relative path to build complete path, avoid path accumulation in batch processing
        // 使用原始相对路径构建完整路径，避免批处理时的路径累积问题
        openmvg.reconstruction_dir = dataset_work_dir + "/reconstruction_global";

        // Update rotation averaging parameters | 更新旋转平均参数
        rotation_averaging.profile_commit = "Rotation averaging on " + dataset_name + " dataset";

        // Update track building parameters | 更新轨迹构建参数
        track_building.profile_commit = "Track building on " + dataset_name + " dataset";
    }

    bool PipelineParameters::Validate(Interface::MethodPreset *method_ptr) const
    {
        // Validate required parameters | 验证必需参数
        if (base.dataset_dir.empty() && base.image_folder.empty())
        {
            LOG_ERROR_ZH << "必须指定dataset_dir或image_folder";
            LOG_ERROR_EN << "Must specify dataset_dir or image_folder";
            return false;
        }

        if (base.work_dir.empty())
        {
            LOG_ERROR_ZH << "work_dir不能为空";
            LOG_ERROR_EN << "work_dir cannot be empty";
            return false;
        }

        if (base.max_iterations <= 0)
        {
            LOG_ERROR_ZH << "max_iterations必须大于0";
            LOG_ERROR_EN << "max_iterations must be greater than 0";
            return false;
        }

        return true;
    }

    void PipelineParameters::PrintSummary(Interface::MethodPreset *method_ptr) const
    {
        LOG_INFO_ZH << "=== GlobalSfM Pipeline 参数摘要 ===";
        LOG_INFO_EN << "=== GlobalSfM Pipeline Parameter Summary ===";
        LOG_INFO_ZH << "基础配置:";
        LOG_INFO_EN << "Basic Configuration:";
        LOG_INFO_ZH << "  dataset_dir: " << base.dataset_dir;
        LOG_INFO_ZH << "  image_folder: " << base.image_folder;
        LOG_INFO_ZH << "  work_dir: " << base.work_dir;
        LOG_INFO_ZH << "  gt_folder: " << base.gt_folder;
        LOG_INFO_ZH << "  enable_evaluation: " << (base.enable_evaluation ? "true" : "false");
        LOG_INFO_ZH << "  max_iterations: " << base.max_iterations;
        LOG_INFO_EN << "  dataset_dir: " << base.dataset_dir;
        LOG_INFO_EN << "  image_folder: " << base.image_folder;
        LOG_INFO_EN << "  work_dir: " << base.work_dir;
        LOG_INFO_EN << "  gt_folder: " << base.gt_folder;
        LOG_INFO_EN << "  enable_evaluation: " << (base.enable_evaluation ? "true" : "false");
        LOG_INFO_EN << "  max_iterations: " << base.max_iterations;

        LOG_INFO_ZH << "OpenMVG配置:";
        LOG_INFO_ZH << "  camera_model: " << openmvg.camera_model;
        LOG_INFO_ZH << "  describer_method: " << openmvg.describer_method;
        LOG_INFO_ZH << "  num_threads: " << openmvg.num_threads;
        LOG_INFO_EN << "OpenMVG Configuration:";
        LOG_INFO_EN << "  camera_model: " << openmvg.camera_model;
        LOG_INFO_EN << "  describer_method: " << openmvg.describer_method;
        LOG_INFO_EN << "  num_threads: " << openmvg.num_threads;
    }

} // namespace PluginMethods
