/**
 * @file posdk_globalsfm.cpp
 * @brief PoSDK GlobalSfM Pipeline 可执行程序 / PoSDK GlobalSfM Pipeline Executable
 * @author PoSDK Team
 */

#include <gflags/gflags.h>
#include <po_core.hpp>

#include <filesystem>
#include <iostream>
#include <chrono>
#include <unordered_map>

using namespace PoSDK;
using namespace Interface;

// 命令行参数定义 / Command Line Parameters Definition
DEFINE_string(dataset_dir, "", "数据集根目录路径（必须参数） / Dataset root directory path (required parameter)");
DEFINE_string(image_folder, "", "图像文件夹路径（可选，如果不指定则处理dataset_dir中的所有数据集） / Image folder path (optional, processes all datasets in dataset_dir if not specified)");
DEFINE_string(preprocess_type, "posdk", "预处理类型：openmvg, posdk, colmap, glomap / Preprocessing type: openmvg, posdk, colmap, glomap");
DEFINE_string(work_dir, "", "工作目录（可选，默认为当前目录/globalsfm_pipeline_work） / Working directory (optional, defaults to current_dir/globalsfm_pipeline_work)");
DEFINE_bool(enable_evaluation, true, "是否启用精度评估 / Whether to enable accuracy evaluation");
DEFINE_int32(max_iterations, 5, "迭代优化最大次数 / Maximum number of iterative optimization");
DEFINE_bool(enable_summary_table, false, "是否启用统一制表功能（批处理时生成汇总表格） / Whether to enable unified table function (generate summary table for batch processing)");
DEFINE_bool(enable_profiling, false, "是否启用性能分析 / Whether to enable performance profiling");
DEFINE_bool(enable_csv_export, true, "是否启用评估结果CSV导出 / Whether to enable CSV export of evaluation results");
DEFINE_string(evaluation_print_mode, "summary", "评估结果打印模式：none, summary, detailed, comparison / Evaluation result print mode: none, summary, detailed, comparison");
DEFINE_string(compared_pipelines, "", "对比流水线列表（逗号分隔）：openmvg, colmap, glomap / Comparison pipeline list (comma-separated): openmvg, colmap, glomap");
DEFINE_int32(log_level, 0, "日志级别 / Log level");
DEFINE_string(language, "ZH", "语言设置：ZH(中文), EN(英文) / Language setting: ZH(Chinese), EN(English)");
DEFINE_string(preset, "default", "参数预设模式：default(使用配置文件默认值), custom(使用命令行自定义参数) / Parameter preset mode: default(use config file defaults), custom(use command line custom parameters)");

// 早期配置双语日志系统 / Configure Early Bilingual Logging System
void ConfigureEarlyLogging()
{
    // 将原有日志级别映射到新系统 / Map legacy log levels to new system
    PoSDK::Logger::LogLevel log_level = PoSDK::Logger::LogLevel::INFO;
    switch (FLAGS_log_level)
    {
    case 0: // PO_LOG_NONE -> INFO
    case 1: // PO_LOG_NORMAL -> INFO
        log_level = PoSDK::Logger::LogLevel::INFO;
        break;
    case 2: // PO_LOG_VERBOSE -> DEBUG
        log_level = PoSDK::Logger::LogLevel::DEBUG;
        break;
    case 3:
        log_level = PoSDK::Logger::LogLevel::WARNING;
        break;
    case 4:
        log_level = PoSDK::Logger::LogLevel::ERROR;
        break;
    case 5:
        log_level = PoSDK::Logger::LogLevel::OFF;
        break;
    default:
        log_level = PoSDK::Logger::LogLevel::INFO;
        break;
    }

    // 配置双语日志系统 / Configure bilingual logging system
    PoSDK::ConfigureLogging(log_level, FLAGS_language);

    // 显示日志配置状态 / Display logging configuration status
    BILINGUAL_LOG_INFO(ZH) << "双语日志系统已配置 - 级别: " << static_cast<int>(log_level) << ", 语言: " << FLAGS_language;
    BILINGUAL_LOG_INFO(EN) << "Bilingual logging system configured - Level: " << static_cast<int>(log_level) << ", Language: " << FLAGS_language;
}

// 验证命令行参数 / Validate Command Line Parameters
bool ValidateParameters()
{
    // 验证语言设置 / Validate language setting
    if (FLAGS_language != "ZH" && FLAGS_language != "EN" && FLAGS_language != "CN")
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：不支持的语言设置: " << FLAGS_language;
        BILINGUAL_LOG_ERROR(EN) << "Error: Unsupported language setting: " << FLAGS_language;
        BILINGUAL_LOG_ERROR(ZH) << "支持的语言：ZH(中文), EN(英文)，也支持CN作为ZH的别名";
        BILINGUAL_LOG_ERROR(EN) << "Supported languages: ZH(Chinese), EN(English), CN is also accepted as alias for ZH";
        return false;
    }

    // 验证预设模式 / Validate preset mode
    if (FLAGS_preset != "default" && FLAGS_preset != "custom")
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：不支持的预设模式: " << FLAGS_preset;
        BILINGUAL_LOG_ERROR(EN) << "Error: Unsupported preset mode: " << FLAGS_preset;
        BILINGUAL_LOG_ERROR(ZH) << "支持的模式：default(使用配置文件默认值), custom(使用命令行自定义参数)";
        BILINGUAL_LOG_ERROR(EN) << "Supported modes: default(use config file defaults), custom(use command line custom parameters)";
        return false;
    }

    // 如果是default模式，跳过参数验证，使用配置文件默认值 / If default mode, skip parameter validation and use config file defaults
    if (FLAGS_preset == "default")
    {
        BILINGUAL_LOG_INFO(ZH) << "使用default预设模式，将使用配置文件默认参数";
        BILINGUAL_LOG_INFO(EN) << "Using default preset mode, will use config file default parameters";
        return true;
    }

    // custom模式：验证必须参数 / custom mode: validate required parameters
    if (FLAGS_dataset_dir.empty())
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：custom模式下必须指定 --dataset_dir 参数";
        BILINGUAL_LOG_ERROR(EN) << "Error: --dataset_dir parameter must be specified in custom mode";
        return false;
    }

    if (!std::filesystem::exists(FLAGS_dataset_dir))
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：数据集目录不存在: " << FLAGS_dataset_dir;
        BILINGUAL_LOG_ERROR(EN) << "Error: Dataset directory does not exist: " << FLAGS_dataset_dir;
        return false;
    }

    // 如果指定了image_folder，验证其存在性 / If image_folder is specified, validate its existence
    if (!FLAGS_image_folder.empty() && !std::filesystem::exists(FLAGS_image_folder))
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：图像文件夹不存在: " << FLAGS_image_folder;
        BILINGUAL_LOG_ERROR(EN) << "Error: Image folder does not exist: " << FLAGS_image_folder;
        return false;
    }

    // 验证预处理类型 / Validate preprocessing type
    if (FLAGS_preprocess_type != "openmvg" && FLAGS_preprocess_type != "posdk" &&
        FLAGS_preprocess_type != "colmap" && FLAGS_preprocess_type != "glomap")
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：不支持的预处理类型: " << FLAGS_preprocess_type;
        BILINGUAL_LOG_ERROR(EN) << "Error: Unsupported preprocessing type: " << FLAGS_preprocess_type;
        BILINGUAL_LOG_ERROR(ZH) << "支持的类型：openmvg, posdk, colmap, glomap";
        BILINGUAL_LOG_ERROR(EN) << "Supported types: openmvg, posdk, colmap, glomap";
        return false;
    }

    return true;
}

// 打印程序信息（双语） / Print Program Information (Bilingual)
void PrintProgramInfo()
{
    LOG_INFO_ALL << "========================================";
    LOG_INFO_ALL << "       PoSDK GlobalSfM Pipeline        ";
    LOG_INFO_ALL << "----------------------------------------";

    BILINGUAL_LOG_INFO(ZH) << "版本: v1.0";
    BILINGUAL_LOG_INFO(EN) << "Version: v1.0";

    BILINGUAL_LOG_INFO(ZH) << "功能: 基于PoSDK的全局SfM重建流水线";
    BILINGUAL_LOG_INFO(EN) << "Function: PoSDK-based Global SfM Reconstruction Pipeline";

    LOG_INFO_ALL << "========================================\n";
}

// 打印参数信息（双语） / Print Parameter Information (Bilingual)
void PrintParameters()
{
    BILINGUAL_LOG_INFO(ZH) << "=== 运行参数 ===";
    BILINGUAL_LOG_INFO(EN) << "=== Runtime Parameters ===";

    BILINGUAL_LOG_INFO(ZH) << "语言设置: " << FLAGS_language;
    BILINGUAL_LOG_INFO(EN) << "Language: " << FLAGS_language;

    BILINGUAL_LOG_INFO(ZH) << "参数预设模式: " << FLAGS_preset;
    BILINGUAL_LOG_INFO(EN) << "Parameter preset mode: " << FLAGS_preset;

    if (FLAGS_preset == "default")
    {
        BILINGUAL_LOG_INFO(ZH) << "使用配置文件默认参数，无需设置命令行参数";
        BILINGUAL_LOG_INFO(EN) << "Using default parameters from config file, no command-line arguments needed";

        BILINGUAL_LOG_INFO(ZH) << "配置文件路径: src/plugins/methods/GlobalSfMPipeline/globalsfm_pipeline.ini";
        BILINGUAL_LOG_INFO(EN) << "Config file path: src/plugins/methods/GlobalSfMPipeline/globalsfm_pipeline.ini";
    }
    else // custom模式 / custom mode
    {
        BILINGUAL_LOG_INFO(ZH) << "数据集目录: " << FLAGS_dataset_dir;
        BILINGUAL_LOG_INFO(EN) << "Dataset directory: " << FLAGS_dataset_dir;

        if (!FLAGS_image_folder.empty())
        {
            BILINGUAL_LOG_INFO(ZH) << "图像文件夹: " << FLAGS_image_folder;
            BILINGUAL_LOG_INFO(EN) << "Image folder: " << FLAGS_image_folder;
        }
        else
        {
            BILINGUAL_LOG_INFO(ZH) << "模式: 批处理模式（处理数据集目录中的所有数据集）";
            BILINGUAL_LOG_INFO(EN) << "Mode: Batch processing (process all datasets in the dataset directory)";
        }

        BILINGUAL_LOG_INFO(ZH) << "预处理类型: " << FLAGS_preprocess_type;
        BILINGUAL_LOG_INFO(EN) << "Preprocess type: " << FLAGS_preprocess_type;

        BILINGUAL_LOG_INFO(ZH) << "工作目录: " << (FLAGS_work_dir.empty() ? "默认" : FLAGS_work_dir);
        BILINGUAL_LOG_INFO(EN) << "Work directory: " << (FLAGS_work_dir.empty() ? "default" : FLAGS_work_dir);

        BILINGUAL_LOG_INFO(ZH) << "启用评估: " << (FLAGS_enable_evaluation ? "是" : "否");
        BILINGUAL_LOG_INFO(EN) << "Enable evaluation: " << (FLAGS_enable_evaluation ? "yes" : "no");

        BILINGUAL_LOG_INFO(ZH) << "最大迭代次数: " << FLAGS_max_iterations;
        BILINGUAL_LOG_INFO(EN) << "Max iterations: " << FLAGS_max_iterations;

        BILINGUAL_LOG_INFO(ZH) << "统一制表: " << (FLAGS_enable_summary_table ? "是" : "否");
        BILINGUAL_LOG_INFO(EN) << "Summary table: " << (FLAGS_enable_summary_table ? "yes" : "no");

        BILINGUAL_LOG_INFO(ZH) << "性能分析: " << (FLAGS_enable_profiling ? "是" : "否");
        BILINGUAL_LOG_INFO(EN) << "Performance profiling: " << (FLAGS_enable_profiling ? "yes" : "no");

        BILINGUAL_LOG_INFO(ZH) << "评估结果打印模式: " << FLAGS_evaluation_print_mode;
        BILINGUAL_LOG_INFO(EN) << "Evaluation print mode: " << FLAGS_evaluation_print_mode;

        if (!FLAGS_compared_pipelines.empty())
        {
            BILINGUAL_LOG_INFO(ZH) << "对比流水线: " << FLAGS_compared_pipelines;
            BILINGUAL_LOG_INFO(EN) << "Compared pipelines: " << FLAGS_compared_pipelines;
        }
    }

    LOG_INFO_ALL << "==================\n";
}

// 执行GlobalSfM流水线 / Execute GlobalSfM Pipeline
bool RunGlobalSfMPipeline()
{
    try
    {
        // 设置环境变量 / Set environment variables
        setenv("PROJECT_SOURCE_DIR", PROJECT_SOURCE_DIR, 1);

        // 创建GlobalSfMPipeline实例 / Create GlobalSfMPipeline instance
        auto method = FactoryMethod::Create("globalsfm_pipeline");
        auto globalsfm_pipeline = std::dynamic_pointer_cast<MethodPreset>(method);
        if (!globalsfm_pipeline)
        {
            BILINGUAL_LOG_ERROR(ZH) << "错误：无法创建GlobalSfMPipeline实例";
            BILINGUAL_LOG_ERROR(EN) << "Error: Failed to create GlobalSfMPipeline instance";
            return false;
        }

        // 根据预设模式设置参数 / Set parameters according to preset mode
        if (FLAGS_preset == "custom")
        {
            BILINGUAL_LOG_INFO(ZH) << "使用custom模式，根据命令行参数设置配置";
            BILINGUAL_LOG_INFO(EN) << "Using custom mode, configuring with command line parameters";

            // 设置工作目录 / Set working directory
            std::string work_dir = FLAGS_work_dir;
            if (work_dir.empty())
            {
                work_dir = std::filesystem::current_path().string() + "/globalsfm_pipeline_work";
            }

            // 设置方法选项 / Set method options
            std::unordered_map<std::string, std::string> options = {
                {"dataset_dir", FLAGS_dataset_dir},
                {"work_dir", work_dir},
                {"preprocess_type", FLAGS_preprocess_type},
                {"enable_evaluation", FLAGS_enable_evaluation ? "true" : "false"},
                {"max_iterations", std::to_string(FLAGS_max_iterations)},
                {"enable_summary_table", FLAGS_enable_summary_table ? "true" : "false"},
                {"enable_profiling", FLAGS_enable_profiling ? "true" : "false"},
                {"enable_csv_export", FLAGS_enable_csv_export ? "true" : "false"},
                {"evaluation_print_mode", FLAGS_evaluation_print_mode},
                {"log_level", std::to_string(FLAGS_log_level)},
                {"ProfileCommit", "PoSDK GlobalSfM Pipeline - Custom Mode"}};

            // 如果指定了image_folder，添加到选项中 / If image_folder is specified, add to options
            if (!FLAGS_image_folder.empty())
            {
                options["image_folder"] = FLAGS_image_folder;
            }

            // 如果指定了对比流水线，添加到选项中 / If compared pipelines are specified, add to options
            if (!FLAGS_compared_pipelines.empty())
            {
                options["compared_pipelines"] = FLAGS_compared_pipelines;
            }

            globalsfm_pipeline->SetMethodOptions(options);

            // 如果是单数据集模式，创建并设置输入数据 / If single dataset mode, create and set input data
            if (!FLAGS_image_folder.empty())
            {
                auto images_data = FactoryData::Create("data_images");
                if (!images_data || !images_data->Load(FLAGS_image_folder))
                {
                    BILINGUAL_LOG_ERROR(ZH) << "错误：无法加载图像数据: " << FLAGS_image_folder;
                    BILINGUAL_LOG_ERROR(EN) << "Error: Failed to load image data: " << FLAGS_image_folder;
                    return false;
                }
                globalsfm_pipeline->SetRequiredData(images_data);
            }
        }

        // 执行流水线 / Execute pipeline
        BILINGUAL_LOG_INFO(ZH) << "开始执行GlobalSfM流水线...";
        BILINGUAL_LOG_INFO(EN) << "Starting GlobalSfM pipeline execution...";
        auto start_time = std::chrono::high_resolution_clock::now();

        auto result = globalsfm_pipeline->Build();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // 检查结果 / Check results
        if (result)
        {
            BILINGUAL_LOG_INFO(ZH) << "\n✓ GlobalSfM流水线执行成功！";
            BILINGUAL_LOG_INFO(EN) << "\n✓ GlobalSfM pipeline execution successful!";
            BILINGUAL_LOG_INFO(ZH) << "执行时间: " << duration.count() << " ms";
            BILINGUAL_LOG_INFO(EN) << "Execution time: " << duration.count() << " ms";

            // 如果启用了统一制表功能，显示汇总信息 / If unified table function is enabled, display summary information
            // 在default模式下，需要从配置文件获取work_dir / In default mode, need to get work_dir from config file
            if ((FLAGS_preset == "custom" && FLAGS_enable_summary_table) ||
                (FLAGS_preset == "default"))
            {
                std::string summary_work_dir;
                if (FLAGS_preset == "custom")
                {
                    std::string work_dir = FLAGS_work_dir;
                    if (work_dir.empty())
                    {
                        work_dir = std::filesystem::current_path().string() + "/globalsfm_pipeline_work";
                    }
                    summary_work_dir = work_dir;
                }
                else // default模式，尝试使用默认的工作目录 / default mode, try to use default working directory
                {
                    summary_work_dir = std::filesystem::current_path().string() + "/globalsfm_pipeline_work";
                }

                std::string summary_file = summary_work_dir + "/summary/dataset_summary.csv";
                if (std::filesystem::exists(summary_file))
                {
                    BILINGUAL_LOG_INFO(ZH) << "汇总表格已生成: " << summary_file;
                    BILINGUAL_LOG_INFO(EN) << "Summary table generated: " << summary_file;
                }
                else
                {
                    BILINGUAL_LOG_INFO(ZH) << "提示：检查工作目录 " << summary_work_dir << "/summary/ 是否有汇总文件";
                    BILINGUAL_LOG_INFO(EN) << "Note: Check if summary files exist in working directory " << summary_work_dir << "/summary/";
                }
            }

            return true;
        }
        else
        {
            BILINGUAL_LOG_ERROR(ZH) << "\n✗ GlobalSfM流水线执行失败";
            BILINGUAL_LOG_ERROR(EN) << "\n✗ GlobalSfM pipeline execution failed";
            return false;
        }
    }
    catch (const std::exception &e)
    {
        BILINGUAL_LOG_ERROR(ZH) << "错误：执行过程中发生异常: " << e.what();
        BILINGUAL_LOG_ERROR(EN) << "Error: Exception occurred during execution: " << e.what();
        return false;
    }
}

int main(int argc, char *argv[])
{
    // 设置gflags / Setup gflags
    gflags::SetUsageMessage("PoSDK GlobalSfM Pipeline - 基于PoSDK的全局SfM重建流水线 / PoSDK-based Global SfM Reconstruction Pipeline\n"
                            "\n运行模式 / Running Modes:\n"
                            "  1. Default模式 / Default Mode: PoSDK (使用配置文件默认参数 / Use default parameters from config file)\n"
                            "  2. Custom模式 / Custom Mode:  PoSDK --preset=custom --dataset_dir=/path/to/dataset [其他选项 / other options]\n"
                            "\n参数说明 / Parameter Description:\n"
                            "  --preset: 参数预设模式 / Parameter preset mode (default, custom)\n"
                            "    - default: 使用配置文件默认参数，无需其他参数 / Use default parameters from config file, no other parameters needed\n"
                            "    - custom:  使用命令行自定义参数，需要指定数据集路径 / Use custom command line parameters, dataset path required\n"
                            "\nCustom模式必须参数 / Required Parameters for Custom Mode:\n"
                            "  --dataset_dir: 数据集根目录路径 / Dataset root directory path\n"
                            "\nCustom模式可选参数 / Optional Parameters for Custom Mode:\n"
                            "  --image_folder: 图像文件夹路径（不指定则批处理所有数据集） / Image folder path (batch process all datasets if not specified)\n"
                            "  --preprocess_type: 预处理类型 / Preprocessing type (openmvg, posdk, colmap, glomap)\n"
                            "  --work_dir: 工作目录 / Working directory\n"
                            "  --enable_evaluation: 是否启用精度评估 / Whether to enable accuracy evaluation\n"
                            "  --max_iterations: 迭代优化最大次数 / Maximum number of iterative optimization\n"
                            "  --enable_summary_table: 是否启用统一制表功能 / Whether to enable unified table function\n"
                            "  --evaluation_print_mode: 评估结果打印模式 / Evaluation result print mode\n"
                            "  --compared_pipelines: 对比流水线列表 / Comparison pipeline list (openmvg,colmap,glomap)\n");

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // 早期配置双语日志系统 / Configure early bilingual logging system
    ConfigureEarlyLogging();

    // 打印程序信息 / Print program information
    PrintProgramInfo();

    // 验证参数 / Validate parameters
    if (!ValidateParameters())
    {
        BILINGUAL_LOG_ERROR(ZH) << "参数验证失败，程序退出";
        BILINGUAL_LOG_ERROR(EN) << "Parameter validation failed, program exiting";
        gflags::ShowUsageWithFlags(argv[0]);
        return 1;
    }

    // 打印参数信息 / Print parameter information
    PrintParameters();

    // 执行流水线 / Execute pipeline
    bool success = RunGlobalSfMPipeline();

    // 显示版权信息汇总 / Display copyright information summary
    BILINGUAL_LOG_INFO(ZH) << "\n========================================";
    BILINGUAL_LOG_INFO(ZH) << "版权信息汇总 | Copyright Information Summary";
    BILINGUAL_LOG_INFO(ZH) << "========================================";
    BILINGUAL_LOG_INFO(EN) << "\n========================================";
    BILINGUAL_LOG_INFO(EN) << "Copyright Information Summary";
    BILINGUAL_LOG_INFO(EN) << "========================================";

    PoSDK::Interface::DisplayCopyrightSummary();

    // 清理 / Cleanup
    gflags::ShutDownCommandLineFlags();

    if (success)
    {
        BILINGUAL_LOG_INFO(ZH) << "\n程序执行完成！";
        BILINGUAL_LOG_INFO(EN) << "\nProgram execution completed!";
        return 0;
    }
    else
    {
        BILINGUAL_LOG_INFO(ZH) << "\n程序执行失败！";
        BILINGUAL_LOG_INFO(EN) << "\nProgram execution failed!";
        return 1;
    }
}
