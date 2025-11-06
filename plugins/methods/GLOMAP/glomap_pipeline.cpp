#include "glomap_pipeline.hpp"
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp>
#include <sstream>
#include <cstdlib>
#include <chrono>

#include <common/converter/converter_colmap_file.hpp>
#include <common/converter/converter_openmvg_file.hpp>

namespace PoSDKPlugin
{
    GlomapPreprocess::GlomapPreprocess()
    {
        // No need for input data package, only method configuration parameters are required
        // 不再需要输入数据包，只需方法配置参数

        // Initialize configuration
        // 初始化配置
        InitializeDefaultConfigPath();

        // Automatically detect and cache Glomap, Colmap, and OpenMVG binary directories
        // 自动检测Glomap、Colmap和OpenMVG二进制文件目录并缓存
        glomap_bin_folder_ = DetectGlomapBinPath();
        colmap_bin_folder_ = DetectColmapBinPath();
        OpenMVG_bin_folder_ = DetectOpenMVGBinPath();
    }

    DataPtr GlomapPreprocess::Run()
    {
        // colmap_pipeline.py -> glomap_pipeline.py -> export_global_poses_from_model.py
        // colmap_pipeline.py -> glomap_pipeline.py -> export_global_poses_from_model.py

        try
        {
            // Display configuration information
            // 显示配置信息
            DisplayConfigInfo();

            // Get input image folder from method options
            // 从方法选项获取输入图像文件夹
            images_folder_ = GetOptionAsPath("images_folder", "");
            if (images_folder_.empty())
            {
                LOG_ERROR_ZH << "方法选项中未指定图像文件夹";
                LOG_ERROR_EN << "No image folder specified in method options";
                return nullptr;
            }

            // Check if image folder exists
            // 检查图像文件夹是否存在
            if (!std::filesystem::exists(images_folder_))
            {
                LOG_ERROR_ZH << "图像文件夹不存在: " << images_folder_;
                LOG_ERROR_EN << "Image folder does not exist: " << images_folder_;
                return nullptr;
            }

            // Set working directory - use Glomap-specific name
            // 设置工作目录 - 使用glomap专用名称
            work_dir_ = GetOptionAsPath("work_dir", "glomap_strecha_test_work");
            // Use new configuration item to determine intermediate file directory name
            // 使用新的配置项来确定中间文件目录名
            std::string matchdir_name = GetOptionAsString("sfm_out_dir", "matches");
            matches_dir_ = work_dir_ + "/" + matchdir_name;

            // Check if working directory needs to be cleared
            // 检查是否需要清空工作目录
            bool is_reclear_workdir = GetOptionAsBool("is_reclear_workdir", true);
            if (is_reclear_workdir && std::filesystem::exists(work_dir_))
            {
                try
                {
                    LOG_DEBUG_ZH << "清空工作目录: " << work_dir_;
                    LOG_DEBUG_EN << "Clearing working directory: " << work_dir_;
                    std::filesystem::remove_all(work_dir_);
                    LOG_DEBUG_ZH << "工作目录已清空";
                    LOG_DEBUG_EN << "Working directory cleared";
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "清空工作目录失败: " << e.what();
                    LOG_ERROR_EN << "Failed to clear working directory: " << e.what();
                    return nullptr;
                }
            }

            // Create image directory and export images
            // 创建图像目录并导出图像
            images_dir_ = work_dir_ + "/images";
            std::filesystem::create_directories(images_dir_);

            // Scan image folder and collect image paths
            // 扫描图像文件夹并收集图像路径
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
                LOG_ERROR_ZH << "在文件夹中未找到有效图像: " << images_folder_;
                LOG_ERROR_EN << "No valid images found in folder: " << images_folder_;
                return nullptr;
            }

            // Copy images to working directory
            // 将图像复制到工作目录
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
                    LOG_ERROR_ZH << "复制图像失败: " << e.what();
                    LOG_ERROR_EN << "Failed to copy image: " << e.what();
                    return nullptr;
                }
            }

            // Create working directories
            // 创建工作目录
            if (!CreateWorkDirectories())
            {
                LOG_ERROR_ZH << "创建工作目录失败";
                LOG_ERROR_EN << "Failed to create working directories";
                return nullptr;
            }

            // Run OpenMVG reading module
            // 运行OpenMVG读取模块
            std::map<std::string, int> file_name_to_id;
            if (RunSfMInitImageListing())
            {
                LOG_DEBUG_ZH << "SfMInitImageListing 成功";
                LOG_DEBUG_EN << "SfMInitImageListing success";
                if (!PoSDK::Converter::Colmap::SfMFileToIdMap(matches_dir_ + "/sfm_data.json", file_name_to_id))
                {
                    LOG_ERROR_ZH << "从 " << matches_dir_ + "/sfm_data.json" << " 转换 SfMFileToIdMap 失败";
                    LOG_ERROR_EN << "Failed to convert SfMFileToIdMap from " << matches_dir_ + "/sfm_data.json";
                    return nullptr;
                }
            }
            else
            {
                LOG_ERROR_ZH << "SfMInitImageListing 失败";
                LOG_ERROR_EN << "SfMInitImageListing failed";
                return nullptr;
            }

            // Run Colmap toolchain first
            // 先运行Colmap工具链
            if (!RunColmapPipeline())
            {
                LOG_ERROR_ZH << "ColmapPipeline 失败";
                LOG_ERROR_EN << "ColmapPipeline failed";
                return nullptr;
            }

            // Then run Glomap toolchain
            // 然后运行Glomap工具链
            if (!RunGlomapPipeline())
            {
                LOG_ERROR_ZH << "GlomapPipeline 失败";
                LOG_ERROR_EN << "GlomapPipeline failed";
                return nullptr;
            }

            auto output_package = std::make_shared<DataPackage>();

            auto gpose_data_ = FactoryData::Create("data_global_poses");
            if (RunExportGlobalPosesFromModel())
            {
                // Read pose information
                // 读取位姿信息
                std::string global_pose_file = work_dir_ + "/images.txt";
                if (!PoSDK::Converter::Colmap::ToDataGlobalPoses(global_pose_file, gpose_data_, file_name_to_id))
                {
                    LOG_ERROR_ZH << "从 " << global_pose_file << " 转换匹配失败";
                    LOG_ERROR_EN << "Failed to convert matches from " << global_pose_file;
                    return nullptr;
                }
            }
            else
            {
                LOG_ERROR_ZH << "ExportGlobalPosesFromModel 失败";
                LOG_ERROR_EN << "ExportGlobalPosesFromModel failed";
                return nullptr;
            }
            output_package->AddData(gpose_data_);

            return output_package;
        }
        catch (const std::exception &e)
        {

            LOG_ERROR_ZH << "错误: " << e.what();
            LOG_ERROR_EN << "Error: " << e.what();
            return nullptr;
        }
    }

    bool GlomapPreprocess::RunExportGlobalPosesFromModel()
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

        // Build model path: work_dir/glomap_output/0 | 构建模型路径：work_dir/glomap_output/0
        std::string model_path = work_dir_ + "/glomap_output/0";

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
        int ret = system(cmd_converter.str().c_str());
        PROFILER_STAGE("export_global_poses_from_model");

        if (ret != 0)
        {
            LOG_ERROR_ZH << "相机姿态导出失败" << std::endl;
            LOG_ERROR_EN << "Camera pose export failed" << std::endl;
            return false;
        }

        LOG_INFO_ZH << "相机姿态导出成功，文件保存到: " << output_folder << std::endl;
        LOG_INFO_EN << "Camera pose export successful, files saved to: " << output_folder << std::endl;

        // ========== 生成PLY文件 ==========
        PROFILER_STAGE("export_to_ply");

        // txt文件已经通过model_converter生成在output_folder中
        std::string cameras_txt = output_folder + "/cameras.txt";
        std::string images_txt = output_folder + "/images.txt";
        std::string points3D_txt = output_folder + "/points3D.txt";

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

        // 生成PLY文件
        if (read_success)
        {
            std::string ply_path = work_dir_ + "/glomap_reconstruction.ply";
            LOG_INFO_ZH << "正在生成GLOMAP重建PLY文件: " << ply_path << std::endl;
            LOG_INFO_EN << "Generating GLOMAP reconstruction PLY file: " << ply_path << std::endl;

            if (PoSDK::Converter::Colmap::WritePointsAndCamerasToPLY(ply_path, points3D, images))
            {
                LOG_INFO_ZH << "GLOMAP PLY文件生成成功: " << ply_path << std::endl;
                LOG_INFO_EN << "GLOMAP PLY file generated successfully: " << ply_path << std::endl;
            }
            else
            {
                LOG_WARNING_ZH << "GLOMAP PLY文件生成失败" << std::endl;
                LOG_WARNING_EN << "Failed to generate GLOMAP PLY file" << std::endl;
            }
        }

        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return true;
    }

    std::string GlomapPreprocess::DetectGlomapBinPath() const
    {
        // Try multiple possible Glomap installation paths
        // 尝试多个可能的Glomap安装路径
        std::vector<std::string> candidate_paths = {
            // Local installation (install_local/bin) - highest priority | 本地安装（install_local/bin）- 最高优先级
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/install_local/bin",
            // Build directory (build_local) - used by install_glomap.sh | 构建目录（build_local）- install_glomap.sh使用
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/build_local",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/build_local/glomap",
            // Legacy build directory | 遗留构建目录
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/build",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/build/glomap",
            std::string(PROJECT_SOURCE_DIR) + "/dependencies/glomap-main/install/bin", // Legacy system installation | 遗留系统安装
            // Alternative relative paths | 其他相对路径
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/build_local",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/build_local/glomap",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/build",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/build/glomap",
            std::string(PROJECT_SOURCE_DIR) + "/../dependencies/glomap-main/install/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/install_local/bin",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/build_local",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/build_local/glomap",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/build",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/build/glomap",
            std::string(PROJECT_SOURCE_DIR) + "/../../dependencies/glomap-main/install/bin",
            // Path specified in configuration (manual configuration)
            // 配置文件指定的路径（手动配置）
            GetOptionAsString("glomap_bin_folder", "")};

        for (const auto &path : candidate_paths)
        {
            if (path.empty())
                continue;

            std::string test_binary = path + "/glomap";
            if (CheckGlomapBinary(test_binary))
            {
                LOG_DEBUG_ZH << "找到glomap于: " << path;
                LOG_DEBUG_EN << "Found glomap at: " << path;
                return path;
            }
        }

        // Try system PATH
        // 尝试系统PATH
        if (CheckGlomapBinary("glomap"))
        {
            LOG_DEBUG_ZH << "在系统PATH中找到Glomap";
            LOG_DEBUG_EN << "Found Glomap in system PATH";
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "在任何候选路径中均未找到Glomap二进制文件";
        LOG_ERROR_EN << "Glomap binaries not found in any candidate paths";
        LOG_WARNING_ZH << "候选路径列表: ";
        LOG_WARNING_EN << "candidate_paths: ";
        for (const auto &path : candidate_paths)
        {
            LOG_WARNING_ZH << path;
            LOG_WARNING_EN << path;
        }
        return "";
    }

    std::string GlomapPreprocess::DetectColmapBinPath() const
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

    bool GlomapPreprocess::CheckColmapBinary(const std::string &bin_path) const
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

        int res = system(check_cmd.c_str());

        return res == 0;
    }

    std::string GlomapPreprocess::DetectOpenMVGBinPath() const
    {
        // Priority 1: User-specified path from configuration (highest priority) | 优先级1：从配置指定的用户路径（最高优先级）
        std::string user_path = GetOptionAsString("openmvg_bin_folder", "");
        if (!user_path.empty())
        {
            std::string test_binary = user_path + "/openMVG_main_SfMInit_ImageListing";
            if (CheckGlomapBinary(test_binary))
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
            if (CheckGlomapBinary(test_binary))
            {
                LOG_DEBUG_ZH << "找到OpenMVG于: " << path;
                LOG_DEBUG_EN << "Found OpenMVG at: " << path;
                return path;
            }
        }

        // Priority 3: System PATH (fallback) | 优先级3：系统PATH（备选）
        if (CheckGlomapBinary("openMVG_main_SfMInit_ImageListing"))
        {
            LOG_DEBUG_ZH << "在系统PATH中找到OpenMVG";
            LOG_DEBUG_EN << "Found OpenMVG in system PATH";
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "在任何候选路径中均未找到OpenMVG二进制文件";
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

    bool GlomapPreprocess::CheckGlomapBinary(const std::string &bin_path) const
    {

        // Check if file exists | 检查文件是否存在
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

        int res = system(check_cmd.c_str());

        return res == 0;
    }

    bool GlomapPreprocess::CreateWorkDirectories() const
    {
        try
        {
            // Create main working directory | 创建工作主目录
            std::filesystem::create_directories(work_dir_);

            // Create matches directory | 创建匹配目录
            std::filesystem::create_directories(matches_dir_);

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "创建目录失败: " << e.what();
            LOG_ERROR_EN << "Failed to create directories: " << e.what();
            return false;
        }
    }

    bool GlomapPreprocess::RunEvalQuality()
    {
        PROFILER_START_AUTO(true);
        if (OpenMVG_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = OpenMVG_bin_folder_ + "/openMVG_main_evalQuality";

        if (!CheckGlomapBinary(bin_path))
        {
            LOG_ERROR_ZH << "未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Get ground truth dataset path | 获取真值数据集路径
        std::string gt_dataset_path = GetOptionAsPath("gt_dataset_path");
        if (gt_dataset_path.empty())
        {
            LOG_ERROR_ZH << "未指定用于质量评估的真值数据集路径";
            LOG_ERROR_EN << "Ground truth dataset path not specified for quality evaluation";
            return false;
        }

        // Check if ground truth dataset exists | 检查真值数据集是否存在
        if (!std::filesystem::exists(gt_dataset_path))
        {
            LOG_ERROR_ZH << "真值数据集不存在: " << gt_dataset_path;
            LOG_ERROR_EN << "Ground truth dataset does not exist: " << gt_dataset_path;
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

        LOG_DEBUG_ZH << "正在运行质量评估:";
        LOG_DEBUG_ZH << "  真值数据集: " << gt_dataset_path;
        LOG_DEBUG_ZH << "  重建结果: " << final_sfm_data_path_;
        LOG_DEBUG_ZH << "  输出目录: " << eval_output_dir_;
        LOG_DEBUG_ZH << "命令: " << cmd.str();
        LOG_DEBUG_EN << "Running quality evaluation:";
        LOG_DEBUG_EN << "  Ground Truth: " << gt_dataset_path;
        LOG_DEBUG_EN << "  Reconstruction: " << final_sfm_data_path_;
        LOG_DEBUG_EN << "  Output: " << eval_output_dir_;
        LOG_DEBUG_EN << "Command: " << cmd.str();

        // Execute command | 执行命令
        int ret = system(cmd.str().c_str());
        PROFILER_STAGE("quality_evaluation");

        if (ret == 0)
        {
            LOG_INFO_ZH << "质量评估成功完成!";
            LOG_INFO_ZH << "结果保存至: " << eval_output_dir_;
            LOG_INFO_EN << "Quality evaluation completed successfully!";
            LOG_INFO_EN << "Results saved to: " << eval_output_dir_;

            // Check generated files | 检查生成的文件
            std::string html_report = eval_output_dir_ + "/ExternalCalib_Report.html";
            std::string json_stats = eval_output_dir_ + "/gt_eval_stats_blob.json";

            if (std::filesystem::exists(html_report))
            {
                LOG_DEBUG_ZH << "HTML报告: " << html_report;
                LOG_DEBUG_EN << "HTML report: " << html_report;
            }

            if (std::filesystem::exists(json_stats))
            {
                LOG_DEBUG_ZH << "JSON统计数据: " << json_stats;
                LOG_DEBUG_EN << "JSON statistics: " << json_stats;
            }
        }
        else
        {
            LOG_ERROR_ZH << "质量评估失败，返回码: " << ret;
            LOG_ERROR_EN << "Quality evaluation failed with return code: " << ret;
        }
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool GlomapPreprocess::RunColmapPipeline()
    {
        PROFILER_START_AUTO(true);
        if (colmap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到 Colmap 二进制目录" << std::endl;
            LOG_ERROR_EN << "Colmap binary directory not found" << std::endl;
            return false;
        }

        // Set camera parameters (fx,fy,cx,cy format, from user-provided intrinsic matrix) | 设置相机内参 (fx,fy,cx,cy格式，来自用户提供的内参矩阵)
        std::string camera_params = "2759.48,2764.16,1520.69,1006.81";

        // Run Colmap CLI commands equivalently
        std::string database_path = work_dir_ + "/database.db";
        std::string sparse_path = work_dir_ + "/sparse";
        // 创建sparse目录
        std::filesystem::create_directories(sparse_path);

        PROFILER_STAGE("feature_extraction");
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

        int ret = system(cmd_extractor.str().c_str());
        PROFILER_STAGE("feature_extraction");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "特征提取失败" << std::endl;
            LOG_ERROR_EN << "Feature extraction failed" << std::endl;
            return false;
        }

        PROFILER_STAGE("feature_matching");
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

        ret = system(cmd_matcher.str().c_str());
        PROFILER_STAGE("feature_matching");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "特征匹配失败" << std::endl;
            LOG_ERROR_EN << "Feature matching failed" << std::endl;
            return false;
        }

        // // Mapper
        // std::stringstream cmd_mapper;
        // cmd_mapper << colmap_bin_folder_ << (colmap_bin_folder_.empty() ? "" : "/") << "colmap mapper ";
        // cmd_mapper << "--database_path " << database_path << " ";
        // cmd_mapper << "--image_path " << images_dir_ << " ";
        // cmd_mapper << "--output_path " << sparse_path << " ";
        // cmd_mapper << "--Mapper.num_threads 4 ";
        // cmd_mapper << "--Mapper.ba_use_gpu false ";
        //
        // LOG_INFO_ZH << "运行增量重建: " << cmd_mapper.str() << std::endl;
        // LOG_INFO_EN << "Running incremental mapping: " << cmd_mapper.str() << std::endl;
        //
        // if (system(cmd_mapper.str().c_str()) != 0) {
        //     LOG_ERROR_ZH << "增量重建失败" << std::endl;
        //     LOG_ERROR_EN << "Incremental mapping failed" << std::endl;
        //     return false;
        // }

        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return true;
    }

    bool GlomapPreprocess::RunGlomapPipeline()
    {
        PROFILER_START_AUTO(true);

        if (glomap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "未找到Glomap二进制目录";
            LOG_ERROR_EN << "Glomap binary directory not found";
            return false;
        }

        PROFILER_STAGE("glomap_binary_check");
        // Check if glomap binary exists | 检查glomap二进制文件是否存在
        std::string glomap_executable = glomap_bin_folder_ + (glomap_bin_folder_.empty() ? "" : "/") + "glomap";

        if (!CheckGlomapBinary(glomap_executable))
        {
            LOG_ERROR_ZH << "未找到glomap二进制文件: " << glomap_executable;
            LOG_ERROR_EN << "glomap binary not found: " << glomap_executable;
            return false;
        }

        PROFILER_STAGE("database_validation");
        // Build database.db path - generated by Colmap in work_dir root directory
        // 构建database.db路径 - 由Colmap生成在work_dir根目录下
        std::string database_path = work_dir_ + "/database.db";

        // Check if database.db exists | 检查database.db是否存在
        if (!std::filesystem::exists(database_path))
        {
            LOG_ERROR_ZH << "数据库文件不存在: " << database_path;
            LOG_ERROR_ZH << "请确保colmap_pipeline已成功执行";
            LOG_ERROR_EN << "Database file does not exist: " << database_path;
            LOG_ERROR_EN << "Make sure colmap_pipeline has been executed successfully";
            return false;
        }

        PROFILER_STAGE("output_directory_setup");
        // Set Glomap output path to work_dir/glomap_output | 设置Glomap输出路径为work_dir/glomap_output
        std::string glomap_output_dir = work_dir_ + "/glomap_output";

        // Create output directory if it doesn't exist | 创建输出目录（如果不存在）
        if (!std::filesystem::exists(glomap_output_dir))
        {
            try
            {
                std::filesystem::create_directories(glomap_output_dir);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "创建输出目录失败: " << glomap_output_dir << " - " << e.what();
                LOG_ERROR_EN << "Failed to create output directory: " << glomap_output_dir << " - " << e.what();
                return false;
            }
        }

        PROFILER_STAGE("glomap_mapper_execution");
        // Build GLOMAP mapper command line | 构建GLOMAP mapper命令行
        std::stringstream cmd;
        cmd << glomap_executable << " mapper ";
        cmd << "--database_path " << database_path << " ";
        cmd << "--image_path " << images_dir_ << " ";
        cmd << "--output_path " << glomap_output_dir;

        LOG_INFO_ZH << "运行GLOMAP全局优化重建: " << cmd.str();
        LOG_INFO_EN << "Running GLOMAP global optimization reconstruction: " << cmd.str();

        LOG_DEBUG_ZH << "正在运行GLOMAP mapper:";
        LOG_DEBUG_ZH << "  数据库路径: " << database_path;
        LOG_DEBUG_ZH << "  图像路径: " << images_dir_;
        LOG_DEBUG_ZH << "  输出路径: " << glomap_output_dir;
        LOG_DEBUG_ZH << "  GLOMAP可执行文件: " << glomap_executable;
        LOG_DEBUG_EN << "Running GLOMAP mapper:";
        LOG_DEBUG_EN << "  Database path: " << database_path;
        LOG_DEBUG_EN << "  Image path: " << images_dir_;
        LOG_DEBUG_EN << "  Output path: " << glomap_output_dir;
        LOG_DEBUG_EN << "  GLOMAP executable: " << glomap_executable;

        // Execute GLOMAP command | 执行GLOMAP命令
        int ret = system(cmd.str().c_str());
        if (ret != 0)
        {
            LOG_ERROR_ZH << "GLOMAP mapper执行失败，返回码: " << ret;
            LOG_ERROR_EN << "GLOMAP mapper execution failed with return code: " << ret;
            return false;
        }

        PROFILER_STAGE("result_validation");
        // Check if reconstruction results exist | 检查重建结果是否存在
        std::vector<std::string> model_files = {"cameras.bin", "images.bin", "points3D.bin"};
        int success_count = 0;

        for (const auto &file : model_files)
        {
            std::string file_path = glomap_output_dir + "/0/" + file;
            if (std::filesystem::exists(file_path))
            {
                LOG_DEBUG_ZH << "  - " << file << ": ✓";
                LOG_DEBUG_EN << "  - " << file << ": ✓";
                success_count++;
            }
            else
            {
                LOG_WARNING_ZH << "  - " << file << ": ✗";
                LOG_WARNING_EN << "  - " << file << ": ✗";
            }
        }

        if (success_count > 0)
        {
            LOG_INFO_ZH << "GLOMAP重建成功完成! 模型保存在: " << glomap_output_dir;
            LOG_INFO_EN << "GLOMAP reconstruction completed successfully! Model saved at: " << glomap_output_dir;
            LOG_INFO_ZH << "生成的文件: " << success_count << "/" << model_files.size();
            LOG_INFO_EN << "Generated files: " << success_count << "/" << model_files.size();
        }
        else
        {
            LOG_WARNING_ZH << "重建完成但未找到预期的模型文件";
            LOG_WARNING_EN << "Reconstruction completed but expected model files not found";
        }

        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return true;
    }

    bool GlomapPreprocess::RunExportMatchesFromDB()
    {
        PROFILER_START_AUTO(true);
        if (colmap_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "Colmap二进制目录未找到";
            LOG_ERROR_EN << "Colmap binary directory not found";
            return false;
        }

        // Use project source directory to find Python script | 使用项目源码目录查找Python脚本
        std::string scripts_dir = std::string(PROJECT_SOURCE_DIR) + "/plugins/methods/GLOMAP";
        std::string python_file = scripts_dir + "/export_matches_from_db.py";

        if (!CheckGlomapBinary(python_file))
        {
            LOG_ERROR_ZH << "未找到export_matches_from_db.py: " << python_file;
            LOG_ERROR_EN << "export_matches_from_db.py not found: " << python_file;
            return false;
        }

        // Build command line and set environment variable to run COLMAP in headless mode
        // 构建命令行，设置环境变量让COLMAP以无头模式运行
        std::stringstream cmd;
        cmd << "QT_QPA_PLATFORM=offscreen "; // Set Qt headless mode | 设置Qt无头模式
        cmd << "python3 " << python_file;
        cmd << " --database_path " << images_dir_;
        cmd << " --output_folder " << matches_dir_;

        LOG_DEBUG_ZH << "正在运行: " << cmd.str();
        LOG_DEBUG_EN << "Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = system(cmd.str().c_str());
        PROFILER_STAGE("export_matches_from_db");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "export_matches_from_db.py执行失败";
            LOG_ERROR_EN << "export_matches_from_db.py execution failed";
            return false;
        }
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return true;
    }

    bool GlomapPreprocess::RunSfMInitImageListing()
    {
        PROFILER_START_AUTO(true);
        if (OpenMVG_bin_folder_.empty())
        {
            LOG_ERROR_ZH << "OpenMVG二进制目录未找到";
            LOG_ERROR_EN << "OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = OpenMVG_bin_folder_ + "/openMVG_main_SfMInit_ImageListing";

        if (!CheckGlomapBinary(bin_path))
        {
            LOG_ERROR_ZH << "未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Prepare parameters for SfMInit_ImageListing
        // 准备SfMInit_ImageListing参数
        std::string camera_sensor_db = GetOptionAsString("camera_sensor_db", "");
        std::string camera_model = GetOptionAsString("camera_model", "3");
        std::string intrinsics_str = GetOptionAsString("intrinsics", "");
        std::string focal_pixels_str = GetOptionAsString("focal_pixels", "-1.0");
        std::string group_camera_model = GetOptionAsString("group_camera_model", "1");
        std::string use_pose_prior = GetOptionAsBool("use_pose_prior", false) ? " -P" : "";
        std::string prior_weights = GetOptionAsString("prior_weights", "1.0;1.0;1.0");
        std::string gps_to_xyz_method = GetOptionAsString("gps_to_xyz_method", "0");

        // If intrinsics are comma-separated, convert to semicolon-separated
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

        LOG_DEBUG_ZH << "正在运行: " << cmd.str();
        LOG_DEBUG_EN << "Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = system(cmd.str().c_str());
        PROFILER_STAGE("sfm_init_image_listing");
        if (ret != 0)
        {
            LOG_ERROR_ZH << "SfMInitImageListing执行失败";
            LOG_ERROR_EN << "SfMInitImageListing execution failed";
            return false;
        }

        // Set sfm_data file path using new configuration item
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

// ✨ 插件注册 - GetType() 由宏自动实现
// Plugin registration - GetType() automatically implemented by macro
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PoSDKPlugin::GlomapPreprocess)
