#include "colmap_pipeline.hpp"
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp>

#include <common/converter/converter_colmap_file.hpp>
#include <common/converter/converter_openmvg_file.hpp>

namespace PoSDKPlugin
{
    ColmapPreprocess::ColmapPreprocess()
    {
        // No need for input data package, only method configuration parameters are required | 不再需要输入数据包，只需方法配置参数

        // Initialize configuration | 初始化配置
        InitializeDefaultConfigPath();

        // Automatically detect and cache Colmap binary directory | 自动检测Colmap二进制文件目录并缓存
        colmap_bin_folder_ = DetectColmapBinPath();
        OpenMVG_bin_folder_ = DetectOpenMVGBinPath();
    }

    DataPtr ColmapPreprocess::Run()
    {
        // colmap_pipeline.py
        // /export_matches_from_db.py
        // ./export_global_poses_from_model.py  --model_path=/home/parallels/test/sparse/0/ --output_folder=/home/parallels/test/

        try
        {
            // Display configuration information | 显示配置信息
            DisplayConfigInfo();

            // Get input image folder from method options | 从方法选项获取输入图像文件夹
            images_folder_ = GetOptionAsPath("images_folder", "");
            if (images_folder_.empty())
            {
                LOG_ERROR_ZH << "方法选项中未指定图像文件夹" << std::endl;
                LOG_ERROR_EN << "No image folder specified in method options" << std::endl;
                return nullptr;
            }

            // Check if image folder exists | 检查图像文件夹是否存在
            if (!std::filesystem::exists(images_folder_))
            {
                LOG_ERROR_ZH << "图像文件夹不存在: " << images_folder_ << std::endl;
                LOG_ERROR_EN << "Image folder does not exist: " << images_folder_ << std::endl;
                return nullptr;
            }

            // Set working directory | 设置工作目录
            work_dir_ = GetOptionAsPath("work_dir", "");
            // Use new configuration item to determine intermediate file directory name | 使用新的配置项来确定中间文件目录名
            std::string matchdir_name = GetOptionAsString("sfm_out_dir", "matches");
            matches_dir_ = work_dir_ + "/" + matchdir_name;

            // Check if working directory needs to be cleared | 检查是否需要清空工作目录
            bool is_reclear_workdir = GetOptionAsBool("is_reclear_workdir", true);
            if (is_reclear_workdir && std::filesystem::exists(work_dir_))
            {
                try
                {
                    LOG_DEBUG_ZH << "清空工作目录: " << work_dir_ << std::endl;
                    LOG_DEBUG_EN << "Clearing working directory: " << work_dir_ << std::endl;
                    std::filesystem::remove_all(work_dir_);
                    LOG_DEBUG_ZH << "工作目录已清空" << std::endl;
                    LOG_DEBUG_EN << "Working directory cleared" << std::endl;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "清空工作目录失败: " << e.what() << std::endl;
                    LOG_ERROR_EN << "Failed to clear working directory: " << e.what() << std::endl;
                    return nullptr;
                }
            }

            // Create image directory and export images | 创建图像目录并导出图像
            images_dir_ = work_dir_ + "/images";
            std::filesystem::create_directories(images_dir_);

            // Scan image folder and collect image paths | 扫描图像文件夹并收集图像路径
            for (const auto &entry : std::filesystem::directory_iterator(images_folder_))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".tif" || ext == ".tiff")
                    {
                        image_paths_.push_back(entry.path().string());
                    }
                }
            }

            if (image_paths_.empty())
            {
                LOG_ERROR_ZH << "在文件夹中未找到有效图像: " << images_folder_ << std::endl;
                LOG_ERROR_EN << "No valid images found in folder: " << images_folder_ << std::endl;
                return nullptr;
            }

            // Copy images to working directory | 将图像复制到工作目录
            for (const auto &img_path : image_paths_)
            {
                std::string filename = std::filesystem::path(img_path).filename().string();
                std::string dest_path = images_dir_ + "/" + filename;

                try
                {
                    std::filesystem::copy_file(
                        img_path, dest_path,
                        std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "复制图像失败: " << e.what() << std::endl;
                    LOG_ERROR_EN << "Failed to copy image: " << e.what() << std::endl;
                    return nullptr;
                }
            }

            // Create working directories | 创建工作目录
            if (!CreateWorkDirectories())
            {
                LOG_ERROR_ZH << "创建工作目录失败" << std::endl;
                LOG_ERROR_EN << "Failed to create working directories" << std::endl;
                return nullptr;
            }

            // Run OpenMVG reading module | 运行OpenMVG读取模块
            std::map<std::string, int> file_name_to_id;
            if (RunSfMInitImageListing())
            {
                LOG_DEBUG_ZH << "SfMInitImageListing 成功" << std::endl;
                LOG_DEBUG_EN << "SfMInitImageListing success" << std::endl;
                if (!PoSDK::Converter::Colmap::SfMFileToIdMap(matches_dir_ + "/sfm_data.json", file_name_to_id))
                {
                    LOG_ERROR_ZH << "从 " << matches_dir_ + "/sfm_data.json" << " 转换 SfMFileToIdMap 失败" << std::endl;
                    LOG_ERROR_EN << "Failed to convert SfMFileToIdMap from " << matches_dir_ + "/sfm_data.json" << std::endl;
                    return nullptr;
                }
            }
            else
            {
                LOG_ERROR_ZH << "SfMInitImageListing 失败" << std::endl;
                LOG_ERROR_EN << "SfMInitImageListing failed" << std::endl;
                return nullptr;
            }

            // Run Colmap pipeline | 运行Colmap工具链
            if (!RunColmapPipeline())
            {
                LOG_ERROR_ZH << "ColmapPipeline 失败" << std::endl;
                LOG_ERROR_EN << "ColmapPipeline failed" << std::endl;
                return nullptr;
            }

            auto output_package = std::make_shared<DataPackage>();

            // auto matches_data = FactoryData::Create("data_matches");
            // // Read information | 读取信息
            // if (RunExportMatchesFromDB()) {
            //     // Read matching information | 读取匹配信息
            //     if (!PoSDK::Converter::ColmapFileConverter::ToDataMatches(matches_dir_, matches_data)) {
            //         LOG_ERROR_ZH << "从 " << matches_dir_ << " 转换匹配数据失败" << std::endl;
            //         LOG_ERROR_EN << "Failed to convert matches from " << matches_dir_ << std::endl;
            //         return nullptr;
            //     }
            // } else {
            //     LOG_ERROR_ZH << "ExportMatchesFromDB 失败" << std::endl;
            //     LOG_ERROR_EN << "ExportMatchesFromDB failed" << std::endl;
            //     return nullptr;
            // }
            // output_package->AddData(matches_data);

            auto gpose_data_ = FactoryData::Create("data_global_poses");
            if (RunExportGlobalPosesFromModel())
            {
                // Read pose information | 读取位姿信息
                std::string global_pose_file = work_dir_ + "/images.txt";
                if (!PoSDK::Converter::Colmap::ToDataGlobalPoses(global_pose_file, gpose_data_, file_name_to_id))
                {
                    LOG_ERROR_ZH << "从 " << global_pose_file << " 转换匹配数据失败" << std::endl;
                    LOG_ERROR_EN << "Failed to convert matches from " << global_pose_file << std::endl;
                    return nullptr;
                }
            }
            else
            {
                LOG_ERROR_ZH << "ExportGlobalPosesFromModel 失败" << std::endl;
                LOG_ERROR_EN << "ExportGlobalPosesFromModel failed" << std::endl;
                return nullptr;
            }
            output_package->AddData(gpose_data_);

            return output_package;
        }
        catch (const std::exception &e)
        {

            LOG_ERROR_ZH << "错误: " << e.what() << std::endl;
            LOG_ERROR_EN << "Error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    bool ColmapPreprocess::RunExportGlobalPosesFromModel()
    {
        PROFILER_START_AUTO(true);
        if (colmap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到Colmap二进制目录" << std::endl;
            LOG_ERROR_EN << "Colmap binary directory not found" << std::endl;
            return false;
        }

        // Use project source directory to find Python script | 使用项目源码目录查找Python脚本
        std::string scripts_dir = std::string(PROJECT_SOURCE_DIR) + "/plugins/methods/GLOMAP";

        // Build model path: work_dir/sparse/0 | 构建模型路径：work_dir/sparse/0
        std::string model_path = work_dir_ + "/sparse/0";

        // Check if model path exists | 检查模型路径是否存在
        if (!std::filesystem::exists(model_path))
        {
            LOG_ERROR_ZH << "模型路径不存在: " << model_path << std::endl;
            LOG_ERROR_EN << "Model path does not exist: " << model_path << std::endl;
            return false;
        }

        // Output directory is work_dir | 输出目录就是work_dir
        std::string output_folder = work_dir_;

        // 替换原来的Python调用
        std::stringstream cmd_converter;
        cmd_converter << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap model_converter ";
        cmd_converter << "--input_path " << model_path << " ";
        cmd_converter << "--output_path " << output_folder << " ";
        cmd_converter << "--output_type TXT ";

        LOG_DEBUG_ZH << "正在导出相机姿态信息:" << std::endl;
        LOG_DEBUG_ZH << "  模型路径: " << model_path << std::endl;
        LOG_DEBUG_ZH << "  输出文件夹: " << output_folder << std::endl;
        LOG_DEBUG_ZH << "命令: " << cmd_converter.str() << std::endl;
        LOG_DEBUG_EN << "Exporting camera poses:" << std::endl;
        LOG_DEBUG_EN << "  Model path: " << model_path << std::endl;
        LOG_DEBUG_EN << "  Output folder: " << output_folder << std::endl;
        LOG_DEBUG_EN << "Command: " << cmd_converter.str() << std::endl;

        // 执行命令
        int ret = POSDK_SYSTEM(cmd_converter.str().c_str());
        PROFILER_STAGE("export_global_poses_from_model");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "相机姿态导出失败" << std::endl;
            LOG_ERROR_EN << "Camera pose export failed" << std::endl;
            return false;
        }

        LOG_INFO_ZH << "相机姿态导出成功，文件保存到: " << output_folder << std::endl;
        LOG_INFO_EN << "Camera pose export successful, files saved to: " << output_folder << std::endl;
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return true;
    }

    std::string ColmapPreprocess::DetectColmapBinPath() const
    {
        // Try multiple possible Colmap build paths | 尝试多个可能的Colmap构建路径
        std::vector<std::string> candidate_paths = {
            // Local installation (install_local/bin) - highest priority | 本地安装（install_local/bin）- 最高优先级
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/install_local/bin",
            // Build directory (build_local) - used by install_colmap.sh | 构建目录（build_local）- install_colmap.sh使用
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build_local/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build_local/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build_local",
            // Legacy build directory | 遗留构建目录
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/colmap-main/build",
            // Alternative relative paths | 其他相对路径
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build_local/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build_local/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build_local",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/colmap-main/build",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build_local/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build_local/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build_local",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build/src/colmap/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build/src/exe",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/colmap-main/build",
            // Path specified in configuration file (manual configuration) | 配置文件指定的路径（手动配置）
            GetOptionAsString("colmap_bin_folder", "")};

        for (const auto &path : candidate_paths)
        {
            if (path.empty())
                continue;

            std::string test_binary = path + "/colmap";
            if (CheckColmapBinary(test_binary))
            {
                LOG_DEBUG_ZH << "找到 colmap 位于: " << path << std::endl;
                LOG_DEBUG_EN << "Found colmap at: " << path << std::endl;
                return path;
            }
        }

        // Try system PATH | 尝试系统PATH
        if (CheckColmapBinary("colmap"))
        {
            LOG_DEBUG_ZH << "在系统 PATH 中找到 Colmap" << std::endl;
            LOG_DEBUG_EN << "Found Colmap in system PATH" << std::endl;
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "在任何候选路径中都未找到 Colmap 二进制文件" << std::endl;
        LOG_ERROR_EN << "Colmap binaries not found in any candidate paths" << std::endl;
        LOG_WARNING_ZH << "候选路径列表: " << std::endl;
        LOG_WARNING_EN << "candidate_paths: " << std::endl;
        for (const auto &path : candidate_paths)
        {
            LOG_WARNING_ZH << path << std::endl;
            LOG_WARNING_EN << path << std::endl;
        }
        return "";
    }

    std::string ColmapPreprocess::DetectOpenMVGBinPath() const
    {
        // Priority 1: User-specified path from configuration (highest priority) | 优先级1：从配置指定的用户路径（最高优先级）
        std::string user_path = GetOptionAsString("openmvg_bin_folder", "");
        if (!user_path.empty())
        {
            std::string test_binary = user_path + "/openMVG_main_SfMInit_ImageListing";
            if (CheckColmapBinary(test_binary))
            {
                LOG_DEBUG_ZH << "使用配置指定的OpenMVG路径: " << user_path;
                LOG_DEBUG_EN << "Using user-specified OpenMVG path: " << user_path;
                return user_path;
            }
        }

        // Priority 2: Standard installation paths based on install_openmvg.sh structure
        // 优先级2：基于install_openmvg.sh结构的标准安装路径
        std::vector<std::string> candidate_paths = {
            // Standard install_local/bin directory (unified structure) | 标准install_local/bin目录（统一结构）
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/openMVG/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/openMVG/build_local",

            // Parent directory variations | 上级目录变体
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/openMVG/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/openMVG/build_local",

            // Relative path alternatives (runtime paths) | 相对路径备选（运行时路径）
            "../../dependencies/openMVG/install_local/bin",
            "../../dependencies/openMVG/build_local",
            "../dependencies/openMVG/install_local/bin",
            "../dependencies/openMVG/build_local"};

        for (const auto &path : candidate_paths)
        {
            if (path.empty())
                continue;

            std::string test_binary = path + "/openMVG_main_SfMInit_ImageListing";
            if (CheckColmapBinary(test_binary))
            {
                LOG_DEBUG_ZH << "找到OpenMVG位于: " << path;
                LOG_DEBUG_EN << "Found OpenMVG at: " << path;
                return path;
            }
        }

        // Priority 3: System PATH (fallback) | 优先级3：系统PATH（备选）
        if (CheckColmapBinary("openMVG_main_SfMInit_ImageListing"))
        {
            LOG_DEBUG_ZH << "在系统PATH中找到OpenMVG";
            LOG_DEBUG_EN << "Found OpenMVG in system PATH";
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "在任何候选路径中都未找到OpenMVG二进制文件";
        LOG_ERROR_EN << "OpenMVG binaries not found in any candidate paths";
        LOG_WARNING_ZH << "候选路径列表:";
        LOG_WARNING_EN << "Candidate paths:";
        if (!user_path.empty())
        {
            LOG_WARNING_ZH << "  [用户指定] " << user_path;
            LOG_WARNING_EN << "  [User-specified] " << user_path;
        }
        for (const auto &path : candidate_paths)
        {
            LOG_WARNING_ZH << "  " << path;
            LOG_WARNING_EN << "  " << path;
        }
        return "";
    }

    bool ColmapPreprocess::CheckColmapBinary(const std::string &bin_path) const
    {
        // Check if the file exists | 检查文件是否存在
        if (!std::filesystem::exists(bin_path))
        {
            return false;
        }

        // For Python scripts, only check file existence | 对于Python脚本，只需要检查文件存在性
        if (bin_path.find(".py") != std::string::npos)
        {
            return true;
        }

        // For binary files, check if executable | 对于二进制文件，检查是否可执行
#ifdef _WIN32
        // Windows platform | Windows平台
        std::string check_cmd = "where \"" + bin_path + "\" > nul 2>&1";
#else
        // Linux/Unix platform | Linux/Unix平台
        std::string check_cmd = "which \"" + bin_path + "\" > /dev/null 2>&1";
#endif

        int ret = system(check_cmd.c_str());

        return ret == 0;
    }

    bool ColmapPreprocess::CreateWorkDirectories() const
    {
        try
        {
            // Create main working directory | 创建工作主目录
            std::filesystem::create_directories(work_dir_);

            // Create matching directory | 创建匹配目录
            std::filesystem::create_directories(matches_dir_);

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "创建目录失败: " << e.what() << std::endl;
            LOG_ERROR_EN << "Failed to create directories: " << e.what() << std::endl;
            return false;
        }
    }

    bool ColmapPreprocess::RunEvalQuality()
    {
        PROFILER_START_AUTO(true);
        if (OpenMVG_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到 Colmap 二进制目录" << std::endl;
            LOG_ERROR_EN << "Colmap binary directory not found" << std::endl;
            return false;
        }
        std::string bin_path = OpenMVG_bin_folder_ + "/Colmap_main_evalQuality";

        if (!CheckColmapBinary(bin_path))
        {
            LOG_ERROR_ZH << "未找到 Colmap 二进制文件: " << bin_path << std::endl;
            LOG_ERROR_EN << "Colmap binary not found: " << bin_path << std::endl;
            return false;
        }

        // Get ground truth dataset path | 获取真值数据集路径
        std::string gt_dataset_path = GetOptionAsPath("gt_dataset_path");
        if (gt_dataset_path.empty())
        {
            LOG_ERROR_ZH << "未指定质量评估的真值数据集路径" << std::endl;
            LOG_ERROR_EN << "Ground truth dataset path not specified for quality evaluation" << std::endl;
            return false;
        }

        // Check if ground truth dataset exists | 检查真值数据集是否存在
        if (!std::filesystem::exists(gt_dataset_path))
        {
            LOG_ERROR_ZH << "真值数据集不存在: " << gt_dataset_path << std::endl;
            LOG_ERROR_EN << "Ground truth dataset does not exist: " << gt_dataset_path << std::endl;
            return false;
        }

        // Set quality evaluation output directory | 设置质量评估输出目录
        std::string eval_dirname = GetOptionAsString("eval_output_dir", "quality_evaluation");
        eval_output_dir_ = work_dir_ + "/" + eval_dirname;
        std::filesystem::create_directories(eval_output_dir_);

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << gt_dataset_path;      // Ground truth dataset path | 真值数据集路径
        cmd << " -c " << final_sfm_data_path_; // Reconstruction result path | 重建结果路径
        cmd << " -o " << eval_output_dir_;     // Evaluation result output directory | 评估结果输出目录

        LOG_INFO_ZH << "正在运行质量评估:" << std::endl;
        LOG_INFO_ZH << "  真值数据集: " << gt_dataset_path << std::endl;
        LOG_INFO_ZH << "  重建结果: " << final_sfm_data_path_ << std::endl;
        LOG_INFO_ZH << "  输出目录: " << eval_output_dir_ << std::endl;
        LOG_INFO_ZH << "命令: " << cmd.str() << std::endl;
        LOG_INFO_EN << "Running quality evaluation:" << std::endl;
        LOG_INFO_EN << "  Ground Truth: " << gt_dataset_path << std::endl;
        LOG_INFO_EN << "  Reconstruction: " << final_sfm_data_path_ << std::endl;
        LOG_INFO_EN << "  Output: " << eval_output_dir_ << std::endl;
        LOG_INFO_EN << "Command: " << cmd.str() << std::endl;

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());
        PROFILER_STAGE("quality_evaluation");

        if (ret == 0)
        {
            LOG_INFO_ZH << "质量评估成功完成!" << std::endl;
            LOG_INFO_ZH << "结果保存至: " << eval_output_dir_ << std::endl;
            LOG_INFO_EN << "Quality evaluation completed successfully!" << std::endl;
            LOG_INFO_EN << "Results saved to: " << eval_output_dir_ << std::endl;

            // Check generated files | 检查生成的文件
            std::string html_report = eval_output_dir_ + "/ExternalCalib_Report.html";
            std::string json_stats = eval_output_dir_ + "/gt_eval_stats_blob.json";

            if (std::filesystem::exists(html_report))
            {
                LOG_DEBUG_ZH << "HTML 报告: " << html_report << std::endl;
                LOG_DEBUG_EN << "HTML report: " << html_report << std::endl;
            }

            if (std::filesystem::exists(json_stats))
            {
                LOG_DEBUG_ZH << "JSON 统计数据: " << json_stats << std::endl;
                LOG_DEBUG_EN << "JSON statistics: " << json_stats << std::endl;
            }
        }
        else
        {
            LOG_ERROR_ZH << "质量评估失败，返回码: " << ret << std::endl;
            LOG_ERROR_EN << "Quality evaluation failed with return code: " << ret << std::endl;
        }
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return (ret == 0);
    }

    bool ColmapPreprocess::RunColmapPipeline()
    {
        PROFILER_START_AUTO(true);

        if (colmap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到 Colmap 二进制目录" << std::endl;
            LOG_ERROR_EN << "Colmap binary directory not found" << std::endl;
            return false;
        }

        PROFILER_STAGE("parameter_setup");

        // Set camera parameters (fx,fy,cx,cy format, from user-provided intrinsic matrix) | 设置相机内参 (fx,fy,cx,cy格式，来自用户提供的内参矩阵)
        std::string camera_params = "2759.48,2764.16,1520.69,1006.81";

        // Run Colmap CLI commands equivalently
        std::string database_path = work_dir_ + "/database.db";
        std::string sparse_path = work_dir_ + "/sparse";
        // 创建sparse目录
        std::filesystem::create_directories(sparse_path);

        // Feature extractor
        std::stringstream cmd_extractor;
        cmd_extractor << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap feature_extractor ";
        cmd_extractor << "--database_path " << database_path << " ";
        cmd_extractor << "--image_path " << images_dir_ << " ";
        cmd_extractor << "--ImageReader.camera_model PINHOLE ";
        cmd_extractor << "--ImageReader.camera_params \" " << camera_params << "\" ";
        cmd_extractor << "--FeatureExtraction.use_gpu false ";
        cmd_extractor << "--FeatureExtraction.gpu_index -1 ";
        cmd_extractor << "--FeatureExtraction.num_threads 4 "; // 限制线程数为4

        LOG_INFO_ZH << "运行特征提取: " << cmd_extractor.str() << std::endl;
        LOG_INFO_EN << "Running feature extraction: " << cmd_extractor.str() << std::endl;

        int ret = POSDK_SYSTEM(cmd_extractor.str().c_str());
        PROFILER_STAGE("feature_extraction");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "特征提取失败" << std::endl;
            LOG_ERROR_EN << "Feature extraction failed" << std::endl;
            return false;
        }
        PROFILER_STAGE("feature_extraction");

        // Exhaustive matcher
        std::stringstream cmd_matcher;
        cmd_matcher << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap exhaustive_matcher ";
        cmd_matcher << "--database_path " << database_path << " ";
        cmd_matcher << "--FeatureMatching.num_threads 4 ";
        cmd_matcher << "--FeatureMatching.use_gpu false ";
        cmd_matcher << "--FeatureMatching.gpu_index -1 ";
        // cmd_matcher << "--SiftMatching.cpu_brute_force_matcher 1 ";  // 强制使用CPU匹配器

        LOG_INFO_ZH << "运行特征匹配: " << cmd_matcher.str() << std::endl;
        LOG_INFO_EN << "Running feature matching: " << cmd_matcher.str() << std::endl;

        ret = POSDK_SYSTEM(cmd_matcher.str().c_str());
        PROFILER_STAGE("feature_matching");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "特征匹配失败" << std::endl;
            LOG_ERROR_EN << "Feature matching failed" << std::endl;
            return false;
        }

        // Mapper
        std::stringstream cmd_mapper;
        cmd_mapper << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap mapper ";
        cmd_mapper << "--database_path " << database_path << " ";
        cmd_mapper << "--image_path " << images_dir_ << " ";
        cmd_mapper << "--output_path " << sparse_path << " ";
        cmd_mapper << "--Mapper.num_threads 4 ";
        cmd_mapper << "--Mapper.ba_use_gpu false ";

        LOG_INFO_ZH << "运行增量重建: " << cmd_mapper.str() << std::endl;
        LOG_INFO_EN << "Running incremental mapping: " << cmd_mapper.str() << std::endl;

        ret = POSDK_SYSTEM(cmd_mapper.str().c_str());
        PROFILER_STAGE("incremental_mapping");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "增量重建失败" << std::endl;
            LOG_ERROR_EN << "Incremental mapping failed" << std::endl;
            return false;
        }

        // ========== 转换bin到txt并生成PLY ==========
        PROFILER_STAGE("export_to_ply");

        std::string model_path = sparse_path + "/0";

        // 1. 使用colmap model_converter转换bin到txt
        LOG_INFO_ZH << "正在转换COLMAP模型到TXT格式..." << std::endl;
        LOG_INFO_EN << "Converting COLMAP model to TXT format..." << std::endl;

        std::stringstream cmd_txt_converter;
        cmd_txt_converter << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap model_converter ";
        cmd_txt_converter << "--input_path " << model_path << " ";
        cmd_txt_converter << "--output_path " << model_path << " ";
        cmd_txt_converter << "--output_type TXT ";

        ret = POSDK_SYSTEM(cmd_txt_converter.str().c_str());
        if (ret != 0)
        {
            LOG_WARNING_ZH << "模型转换为TXT格式失败，跳过PLY生成" << std::endl;
            LOG_WARNING_EN << "Failed to convert model to TXT format, skipping PLY generation" << std::endl;
        }
        else
        {
            // 2. 读取txt文件
            std::string cameras_txt = model_path + "/cameras.txt";
            std::string images_txt = model_path + "/images.txt";
            std::string points3D_txt = model_path + "/points3D.txt";

            std::vector<PoSDK::Converter::Colmap::Camera> cameras;
            std::vector<PoSDK::Converter::Colmap::Image> images;
            std::vector<PoSDK::Converter::Colmap::Point3D> points3D;

            bool read_success = true;

            if (!PoSDK::Converter::Colmap::ReadCamerasTxt(cameras_txt, cameras))
            {
                LOG_WARNING_ZH << "读取cameras.txt失败" << std::endl;
                LOG_WARNING_EN << "Failed to read cameras.txt" << std::endl;
                read_success = false;
            }

            if (!PoSDK::Converter::Colmap::ReadImagesTxt(images_txt, images))
            {
                LOG_WARNING_ZH << "读取images.txt失败" << std::endl;
                LOG_WARNING_EN << "Failed to read images.txt" << std::endl;
                read_success = false;
            }

            if (!PoSDK::Converter::Colmap::ReadPoints3DTxt(points3D_txt, points3D))
            {
                LOG_WARNING_ZH << "读取points3D.txt失败" << std::endl;
                LOG_WARNING_EN << "Failed to read points3D.txt" << std::endl;
                read_success = false;
            }

            // 3. 生成PLY文件
            if (read_success)
            {
                std::string ply_path = work_dir_ + "/colmap_reconstruction.ply";
                LOG_INFO_ZH << "正在生成PLY文件: " << ply_path << std::endl;
                LOG_INFO_EN << "Generating PLY file: " << ply_path << std::endl;

                if (PoSDK::Converter::Colmap::WritePointsAndCamerasToPLY(ply_path, points3D, images))
                {
                    LOG_INFO_ZH << "PLY文件生成成功: " << ply_path << std::endl;
                    LOG_INFO_EN << "PLY file generated successfully: " << ply_path << std::endl;
                }
                else
                {
                    LOG_WARNING_ZH << "PLY文件生成失败" << std::endl;
                    LOG_WARNING_EN << "Failed to generate PLY file" << std::endl;
                }
            }
        }

        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return true;
    }

    bool ColmapPreprocess::RunExportMatchesFromDB()
    {
        PROFILER_START_AUTO(true);
        if (colmap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到 Colmap 二进制目录" << std::endl;
            LOG_ERROR_EN << "Colmap binary directory not found" << std::endl;
            return false;
        }

        // Use project source directory to find Python script | 使用项目源码目录查找Python脚本
        std::string scripts_dir = std::string(PROJECT_SOURCE_DIR) + "/plugins/methods/GLOMAP";
        std::string python_file = scripts_dir + "/export_matches_from_db.py";

        if (!CheckColmapBinary(python_file))
        {
            LOG_ERROR_ZH << "未找到 export_matches_from_db.py: " << python_file << std::endl;
            LOG_ERROR_EN << "export_matches_from_db.py not found: " << python_file << std::endl;
            return false;
        }

        // Build command line, set environment variable to run COLMAP in headless mode | 构建命令行，设置环境变量让COLMAP以无头模式运行
        std::stringstream cmd;
        cmd << "QT_QPA_PLATFORM=offscreen "; // Set Qt headless mode | 设置Qt无头模式
        cmd << "python3 " << python_file;
        cmd << " --database_path " << images_dir_;
        cmd << " --output_folder " << matches_dir_;

        LOG_INFO_ZH << "正在运行: " << cmd.str() << std::endl;
        LOG_INFO_EN << "Running: " << cmd.str() << std::endl;

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());
        PROFILER_STAGE("export_matches_from_db");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "export_matches_from_db.py 执行失败" << std::endl;
            LOG_ERROR_EN << "export_matches_from_db.py execution failed" << std::endl;
            return false;
        }
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return true;
    }

    bool ColmapPreprocess::RunSfMInitImageListing()
    {
        PROFILER_START_AUTO(true);
        // Check if OpenMVG binary directory exists | 检查OpenMVG二进制目录是否存在
        if (OpenMVG_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到OpenMVG二进制目录" << std::endl;
            LOG_ERROR_EN << "OpenMVG binary directory not found" << std::endl;
            return false;
        }
        std::string bin_path = OpenMVG_bin_folder_ + "/openMVG_main_SfMInit_ImageListing";

        // Check if OpenMVG binary file exists | 检查OpenMVG二进制文件是否存在
        if (!CheckColmapBinary(bin_path))
        {
            LOG_ERROR_ZH << "未找到OpenMVG二进制文件: " << bin_path << std::endl;
            LOG_ERROR_EN << "OpenMVG binary not found: " << bin_path << std::endl;
            return false;
        }

        // Prepare parameters for SfMInit_ImageListing | 准备SfMInit_ImageListing参数
        std::string camera_sensor_db = GetOptionAsString("camera_sensor_db", "");
        std::string camera_model = GetOptionAsString("camera_model", "3");
        std::string intrinsics_str = GetOptionAsString("intrinsics", "");
        std::string focal_pixels_str = GetOptionAsString("focal_pixels", "-1.0");
        std::string group_camera_model = GetOptionAsString("group_camera_model", "1");
        std::string use_pose_prior = GetOptionAsBool("use_pose_prior", false) ? " -P" : "";
        std::string prior_weights = GetOptionAsString("prior_weights", "1.0;1.0;1.0");
        std::string gps_to_xyz_method = GetOptionAsString("gps_to_xyz_method", "0");

        // Convert comma-separated intrinsics to semicolon-separated if necessary
        // 如果intrinsics使用逗号分隔，需要转换为分号分隔
        if (!intrinsics_str.empty())
        {
            std::replace(intrinsics_str.begin(), intrinsics_str.end(), ',', ';');
        }

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << images_dir_;
        cmd << " -o " << matches_dir_;

        if (!camera_sensor_db.empty())
        {
            cmd << " -d " << camera_sensor_db;
        }

        if (!intrinsics_str.empty())
        {
            cmd << " -k \"" << intrinsics_str << "\"";
        }

        if (!focal_pixels_str.empty() && focal_pixels_str != "-1.0" && focal_pixels_str != "-1")
        {
            cmd << " -f " << focal_pixels_str;
        }

        cmd << " -c " << camera_model;
        cmd << " -g " << group_camera_model;
        cmd << use_pose_prior;

        if (!prior_weights.empty() && GetOptionAsBool("use_pose_prior", false))
        {
            cmd << " -W \"" << prior_weights << "\"";
        }

        cmd << " -m " << gps_to_xyz_method;

        // Log the command being executed | 记录正在执行的命令
        LOG_DEBUG_ZH << "正在运行: " << cmd.str() << std::endl;
        LOG_DEBUG_EN << "Running: " << cmd.str() << std::endl;

        // Execute the command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());
        PROFILER_STAGE("sfm_init_image_listing");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "SfMInitImageListing执行失败" << std::endl;
            LOG_ERROR_EN << "SfMInitImageListing execution failed" << std::endl;
            return false;
        }

        // Set sfm_data file path using the new configuration item
        // 设置sfm_data文件路径，使用新的配置项
        std::string sfm_data_filename = GetOptionAsString("sfm_data_file", "sfm_data.json");
        sfm_json_path_ = matches_dir_ + "/" + sfm_data_filename;

        // Verify if sfm_data.json was created successfully
        // 验证sfm_data.json是否创建成功
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return std::filesystem::exists(sfm_json_path_);
    }
} // namespace PoSDKPlugin

// ✨ 插件注册 - 使用双参数版本（自定义类型名）
// Plugin registration - using two-parameter version (custom type name)
//
// 说明 | Note:
// - GetType() 函数由宏自动实现，无需手动编写
// - GetType() is automatically implemented by the macro, no manual implementation needed
// - 插件类型为 "colmap_pipeline"（小写，下划线分隔）
// - Plugin type is "colmap_pipeline" (lowercase, underscore-separated)
// - 调用时使用: FactoryMethod::Create("colmap_pipeline")
// - Call with: FactoryMethod::Create("colmap_pipeline")
//
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PoSDKPlugin::ColmapPreprocess)
