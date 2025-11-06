#include "openmvg_pipeline.hpp"
#include <po_core/po_logger.hpp>
#include <po_core/ProfilerManager.hpp> // Profiler system | 性能分析系统
#include <sstream>
#include <cstdlib>
#include <chrono>

#include "common/converter/converter_openmvg_file.hpp"

namespace PoSDKPlugin
{

    OpenMVGPipeline::OpenMVGPipeline()
    {
        // No longer need input data packages, only method configuration parameters | 不再需要输入数据包，只需方法配置参数

        // Initialize configuration | 初始化配置
        InitializeDefaultConfigPath();

        // Automatically detect OpenMVG binary directory and cache | 自动检测OpenMVG二进制文件目录并缓存
        bin_folder_ = DetectOpenMVGBinPath();
    }

    DataPtr OpenMVGPipeline::Run()
    {
        // Start profiling for the entire OpenMVGPipeline::Run function | 开始对整个OpenMVGPipeline::Run函数进行性能分析

        try
        {
            // Display configuration information | 显示配置信息
            DisplayConfigInfo();

            // Get input image folder from method options | 从方法选项获取输入图像文件夹
            images_folder_ = GetOptionAsPath("images_folder", "");
            if (images_folder_.empty())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 方法选项中未指定图像文件夹";
                LOG_ERROR_EN << "[OpenMVGPipeline] No image folder specified in method options";
                return nullptr;
            }

            // Check if image folder exists | 检查图像文件夹是否存在
            if (!std::filesystem::exists(images_folder_))
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 图像文件夹不存在: " << images_folder_;
                LOG_ERROR_EN << "[OpenMVGPipeline] Image folder does not exist: " << images_folder_;
                return nullptr;
            }

            // Set working directory | 设置工作目录
            work_dir_ = GetOptionAsPath("work_dir", "./openmvg_work");
            // Use new configuration item to determine intermediate file directory name | 使用新的配置项来确定中间文件目录名
            std::string sfm_out_dir_name = GetOptionAsString("sfm_out_dir", "matches");
            matches_dir_ = work_dir_ + "/" + sfm_out_dir_name;

            // Check if need to clear working directory | 检查是否需要清空工作目录
            bool is_reclear_workdir = GetOptionAsBool("is_reclear_workdir", true);
            if (is_reclear_workdir && std::filesystem::exists(work_dir_))
            {
                try
                {
                    LOG_DEBUG_ZH << "[OpenMVGPipeline] 清空工作目录: " << work_dir_;
                    LOG_DEBUG_EN << "[OpenMVGPipeline] Clearing working directory: " << work_dir_;
                    std::filesystem::remove_all(work_dir_);
                    LOG_DEBUG_ZH << "[OpenMVGPipeline] 工作目录已清空";
                    LOG_DEBUG_EN << "[OpenMVGPipeline] Working directory cleared";
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[OpenMVGPipeline] 清空工作目录失败: " << e.what();
                    LOG_ERROR_EN << "[OpenMVGPipeline] Failed to clear working directory: " << e.what();
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
                LOG_ERROR_ZH << "[OpenMVGPipeline] 在文件夹中未找到有效图像: " << images_folder_;
                LOG_ERROR_EN << "[OpenMVGPipeline] No valid images found in folder: " << images_folder_;
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
                    LOG_ERROR_ZH << "[OpenMVGPipeline] 复制图像失败: " << e.what();
                    LOG_ERROR_EN << "[OpenMVGPipeline] Failed to copy image: " << e.what();
                    return nullptr;
                }
            }

            // Create working directories | 创建工作目录
            if (!CreateWorkDirectories())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 创建工作目录失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create working directories";
                return nullptr;
            }

            // Run OpenMVG toolchain | 运行OpenMVG工具链
            if (!RunSfMInitImageListing())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] SfMInitImageListing失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] SfMInitImageListing failed";
                return nullptr;
            }

            if (!RunComputeFeatures())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] ComputeFeatures失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] ComputeFeatures failed";
                return nullptr;
            }

            // Optional step: Run PairGenerator (must be before ComputeMatches) | 可选步骤：运行PairGenerator（必须在ComputeMatches之前）
            if (GetOptionAsBool("enable_pair_generator", false))
            {
                if (!RunPairGenerator())
                {
                    LOG_ERROR_ZH << "[OpenMVGPipeline] PairGenerator失败";
                    LOG_ERROR_EN << "[OpenMVGPipeline] PairGenerator failed";
                    return nullptr;
                }
            }

            if (!RunComputeMatches())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] ComputeMatches失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] ComputeMatches failed";
                return nullptr;
            }

            if (!RunGeometricFilter())
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] GeometricFilter失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] GeometricFilter failed";
                return nullptr;
            }

            // Optional step: Run SfM reconstruction (if configuration enabled) | 可选步骤：运行SfM重建（如果配置启用）
            if (GetOptionAsBool("enable_sfm_reconstruction", false))
            {
                if (!RunSfM())
                {
                    LOG_ERROR_ZH << "[OpenMVGPipeline] SfM重建失败";
                    LOG_ERROR_EN << "[OpenMVGPipeline] SfM reconstruction failed";
                    return nullptr;
                }

                // Optional step: Run point cloud coloring (if configuration enabled) | 可选步骤：运行点云着色（如果配置启用）
                if (GetOptionAsBool("enable_point_cloud_coloring", false))
                {
                    if (!RunComputeSfM_DataColor())
                    {
                        LOG_ERROR_ZH << "[OpenMVGPipeline] 点云着色失败";
                        LOG_ERROR_EN << "[OpenMVGPipeline] Point cloud coloring failed";
                        return nullptr;
                    }
                }

                // Optional step: Run quality evaluation (if configuration enabled and ground truth provided) | 可选步骤：运行质量评估（如果配置启用且提供了真值数据）
                if (GetOptionAsBool("enable_quality_evaluation", false))
                {
                    if (!RunEvalQuality())
                    {
                        LOG_ERROR_ZH << "[OpenMVGPipeline] 质量评估失败";
                        LOG_ERROR_EN << "[OpenMVGPipeline] Quality evaluation failed";
                        return nullptr;
                    }
                }
            }

            // Create output data package | 创建输出数据包
            auto output_package = std::make_shared<DataPackage>();

            // Convert and add image data | 转换并添加图像数据
            auto images_data = FactoryData::Create("data_images");
            if (!images_data)
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 创建图像数据失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create images data";
                return nullptr;
            }

            // Use new OpenMVGFileConverter to convert image data | 使用新的OpenMVGFileConverter转换图像数据
            if (!PoSDK::Converter::OpenMVGFileConverter::ToDataImages(sfm_data_path_, images_folder_, images_data))
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 创建图像数据失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create images data";
                return nullptr;
            }

            output_package->AddData(images_data);

            // Convert and add feature data | 转换并添加特征数据
            auto features_data = FactoryData::Create("data_features");
            if (!features_data)
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 创建特征数据失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create features data";
                return nullptr;
            }

            // Use new OpenMVGFileConverter to convert feature data | 使用新的OpenMVGFileConverter转换特征数据
            if (!PoSDK::Converter::OpenMVGFileConverter::ToDataFeatures(sfm_data_path_, matches_dir_, images_folder_, features_data))
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 转换特征失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to convert features";
                return nullptr;
            }

            // Save features to file | 保存特征到文件
            if (GetOptionAsBool("save_features", true))
            {
                std::string save_path = GetOptionAsString("features_save_path", "storage/features/features_all");
                std::filesystem::create_directories(std::filesystem::path(save_path).parent_path());
                features_data->Save("", save_path);
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 保存特征到: " << save_path;
                LOG_DEBUG_EN << "[OpenMVGPipeline] Saved features to: " << save_path;
            }

            output_package->AddData(features_data);

            // Convert and add match data | 转换并添加匹配数据
            auto matches_data = FactoryData::Create("data_matches");
            if (!matches_data)
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 创建匹配数据失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create matches data";
                return nullptr;
            }

            // Select match file to convert based on convert_putative_data configuration | 根据 convert_putative_data 配置选择要转换的匹配文件
            bool use_putative = GetOptionAsBool("convert_putative_data", false);
            std::string matches_file_to_convert;
            if (use_putative)
            {
                matches_file_to_convert = putative_matches_path_;
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 从 " << matches_file_to_convert << " 转换推测匹配";
                LOG_DEBUG_EN << "[OpenMVGPipeline] Converting putative matches from: " << matches_file_to_convert;
            }
            else
            {
                matches_file_to_convert = final_matches_path_;
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 从 " << matches_file_to_convert << " 转换最终（几何过滤）匹配";
                LOG_DEBUG_EN << "[OpenMVGPipeline] Converting final (geometrically filtered) matches from: " << matches_file_to_convert;
            }

            // Use new OpenMVGFileConverter to convert match data | 使用新的OpenMVGFileConverter转换匹配数据
            if (!PoSDK::Converter::OpenMVGFileConverter::ToDataMatches(matches_file_to_convert, matches_data))
            {
                LOG_ERROR_ZH << "[OpenMVGPipeline] 从 " << matches_file_to_convert << " 转换匹配失败";
                LOG_ERROR_EN << "[OpenMVGPipeline] Failed to convert matches from " << matches_file_to_convert;
                return nullptr;
            }

            // Save matches to file | 保存匹配到文件
            if (GetOptionAsBool("save_matches", true))
            {
                std::string save_path = GetOptionAsString("matches_save_path", "storage/matches/matches_all");
                std::filesystem::create_directories(std::filesystem::path(save_path).parent_path());
                matches_data->Save("", save_path);
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 保存匹配到: " << save_path;
                LOG_DEBUG_EN << "[OpenMVGPipeline] Saved matches to: " << save_path;
            }

            output_package->AddData(matches_data);

            // Display accumulated statistics for all submodules | 显示所有子模块的累积统计
            LOG_INFO_ZH << "\n=== OpenMVG Pipeline 子模块性能统计 | Submodule Performance Statistics ===";
            LOG_INFO_EN << "\n=== OpenMVG Pipeline 子模块性能统计 | Submodule Performance Statistics ===";
            PoSDK::Profiler::ProfilerManager::GetInstance().DisplayAllProfilingData();

            return output_package;
        }
        catch (const std::exception &e)
        {
            // Note: PROFILER_END will be called automatically when _profiler_session_ goes out of scope | 注意：当_profiler_session_离开作用域时会自动调用PROFILER_END
            LOG_ERROR_ZH << "[OpenMVGPipeline] 错误: " << e.what();
            LOG_ERROR_EN << "[OpenMVGPipeline] Error: " << e.what();
            return nullptr;
        }
    }

    std::string OpenMVGPipeline::DetectOpenMVGBinPath() const
    {
        // Priority 1: User-specified path from configuration (highest priority) | 优先级1：从配置指定的用户路径（最高优先级）
        std::string user_path = GetOptionAsString("openmvg_bin_folder", "");
        if (!user_path.empty())
        {
            std::string test_binary = user_path + "/openMVG_main_SfMInit_ImageListing";
            if (CheckOpenMVGBinary(test_binary))
            {
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 使用配置指定的OpenMVG路径: " << user_path;
                LOG_DEBUG_EN << "[OpenMVGPipeline] Using user-specified OpenMVG path: " << user_path;
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
            if (CheckOpenMVGBinary(test_binary))
            {
                LOG_DEBUG_ZH << "[OpenMVGPipeline] 在 " << path << " 找到OpenMVG";
                LOG_DEBUG_EN << "[OpenMVGPipeline] Found OpenMVG at: " << path;
                return path;
            }
        }

        // Priority 3: System PATH (fallback) | 优先级3：系统PATH（备选）
        if (CheckOpenMVGBinary("openMVG_main_SfMInit_ImageListing"))
        {
            LOG_DEBUG_ZH << "[OpenMVGPipeline] 在系统PATH中找到OpenMVG";
            LOG_DEBUG_EN << "[OpenMVGPipeline] Found OpenMVG in system PATH";
            return ""; // Empty string indicates in system PATH | 空字符串表示在系统PATH中
        }

        LOG_ERROR_ZH << "[OpenMVGPipeline] 在任何候选路径中未找到OpenMVG二进制文件";
        LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binaries not found in any candidate paths";
        LOG_WARNING_ZH << "[OpenMVGPipeline] 候选路径:";
        LOG_WARNING_EN << "[OpenMVGPipeline] Candidate paths:";
        if (!user_path.empty())
        {
            LOG_WARNING_ZH << "  [用户指定] " << user_path;
            LOG_WARNING_EN << "  [User-specified] " << user_path;
        }
        for (const auto &path : candidate_paths)
        {
            if (!path.empty())
            {
                LOG_WARNING_ZH << "  " << path;
                LOG_WARNING_EN << "  " << path;
            }
        }
        return "";
    }

    bool OpenMVGPipeline::CheckOpenMVGBinary(const std::string &bin_path) const
    {
        // Check if file exists | 检查文件是否存在
        if (!std::filesystem::exists(bin_path))
        {
            return false;
        }

        // Check if executable | 检查是否可执行
#ifdef _WIN32
        // Windows platform | Windows平台
        std::string check_cmd = "where \"" + bin_path + "\" > nul 2>&1";
#else
        // Linux/Unix platform | Linux/Unix平台
        std::string check_cmd = "which \"" + bin_path + "\" > /dev/null 2>&1";
#endif

        return std::system(check_cmd.c_str()) == 0;
    }

    bool OpenMVGPipeline::CreateWorkDirectories() const
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
            LOG_ERROR_ZH << "[OpenMVGPipeline] 创建目录失败: " << e.what();
            LOG_ERROR_EN << "[OpenMVGPipeline] Failed to create directories: " << e.what();
            return false;
        }
    }

    bool OpenMVGPipeline::RunSfMInitImageListing()
    {
        // Start profiling for SfMInitImageListing subprocess | 开始SfMInitImageListing子进程性能分析
        PROFILER_START_AUTO(true);

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_SfMInit_ImageListing";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Prepare SfMInit_ImageListing parameters | 准备SfMInit_ImageListing参数
        std::string camera_sensor_db = GetOptionAsPath("camera_sensor_db", "");
        std::string camera_model = GetOptionAsString("camera_model", "3");
        std::string intrinsics_str = GetOptionAsString("intrinsics", "");
        std::string focal_pixels_str = GetOptionAsString("focal_pixels", "-1.0");
        std::string group_camera_model = GetOptionAsString("group_camera_model", "1");
        std::string use_pose_prior = GetOptionAsBool("use_pose_prior", false) ? " -P" : "";
        std::string prior_weights = GetOptionAsString("prior_weights", "1.0;1.0;1.0");
        std::string gps_to_xyz_method = GetOptionAsString("gps_to_xyz_method", "0");

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

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command with subprocess monitoring | 执行命令并监控子进程
        int ret = POSDK_SYSTEM(cmd.str().c_str());
        if (ret != 0)
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] SfMInitImageListing执行失败";
            LOG_ERROR_EN << "[OpenMVGPipeline] SfMInitImageListing execution failed";
            return false;
        }

        // Set sfm_data file path, using new configuration item | 设置sfm_data文件路径，使用新的配置项
        std::string sfm_data_filename = GetOptionAsString("sfm_data_file", "sfm_data.json");
        sfm_data_path_ = matches_dir_ + "/" + sfm_data_filename;

        // Verify if sfm_data.json was created successfully | 验证sfm_data.json是否创建成功
        bool success = std::filesystem::exists(sfm_data_path_);

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return success;
    }

    bool OpenMVGPipeline::RunComputeFeatures()
    {
        // Start profiling for ComputeFeatures subprocess | 开始ComputeFeatures子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openMVG_main_ComputeFeatures");
        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_ComputeFeatures";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Prepare ComputeFeatures parameters | 准备ComputeFeatures参数
        std::string describer_method = GetOptionAsString("describer_method", "SIFT");
        std::string describer_preset = GetOptionAsString("describer_preset", "NORMAL");
        std::string upright = GetOptionAsBool("upright", false) ? "-u 1" : "";
        std::string force = GetOptionAsBool("force_compute", false) ? "-f 1" : "";
        std::string num_threads = GetOptionAsString("num_threads", "0");

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << sfm_data_path_;
        cmd << " -o " << matches_dir_;
        cmd << " -m " << describer_method;

        if (!upright.empty())
        {
            cmd << " " << upright;
        }

        if (!force.empty())
        {
            cmd << " " << force;
        }

        if (!describer_preset.empty())
        {
            cmd << " -p " << describer_preset;
        }

        if (!num_threads.empty())
        {
            cmd << " -n " << num_threads;
        }

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunComputeMatches()
    {
        // Start profiling for ComputeMatches subprocess | 开始ComputeMatches子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_compute_matches");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_ComputeMatches";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Prepare ComputeMatches parameters | 准备ComputeMatches参数
        std::string pair_list = GetOptionAsString("pair_list", "");
        std::string ratio = GetOptionAsString("ratio", "0.8");
        std::string nearest_matching_method = GetOptionAsString("nearest_matching_method", "AUTO");
        std::string force = GetOptionAsBool("force_compute", false) ? "-f 1" : "";
        std::string cache_size = GetOptionAsString("cache_size", "0");
        std::string preemptive_feature_count = GetOptionAsString("preemptive_feature_count", "200");

        // Get putative matches filename configuration and set member variable | 获取推测匹配文件名配置并设置成员变量
        std::string put_match_fn = GetOptionAsString("putative_matches", "matches.putative.bin"); // User updated default to .bin | 用户更新了默认值为 .bin
        putative_matches_path_ = matches_dir_ + "/" + put_match_fn;

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << sfm_data_path_;         // Input sfm_data file | 输入sfm_data文件
        cmd << " -o " << putative_matches_path_; // Output putative matches file, using member variable | 输出推测匹配文件，使用成员变量

        if (!pair_list.empty())
        {
            cmd << " -p " << pair_list;
        }
        // If PairGenerator enabled, use generated pairs file | 如果启用了PairGenerator，使用生成的pairs文件
        else if (GetOptionAsBool("enable_pair_generator", false) && !pairs_path_.empty())
        {
            cmd << " -p " << pairs_path_;
        }

        if (!ratio.empty())
        {
            cmd << " -r " << ratio;
        }

        if (!nearest_matching_method.empty())
        {
            cmd << " -n " << nearest_matching_method;
        }

        if (!force.empty())
        {
            cmd << " " << force;
        }

        if (!cache_size.empty() && cache_size != "0")
        {
            cmd << " -c " << cache_size;
        }

        if (GetOptionAsBool("use_preemptive", false) && !preemptive_feature_count.empty())
        {
            cmd << " -P " << preemptive_feature_count;
        }

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunGeometricFilter()
    {
        // Start profiling for GeometricFilter subprocess | 开始GeometricFilter子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_geometric_filter");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_GeometricFilter";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Prepare GeometricFilter parameters | 准备GeometricFilter参数
        std::string geometric_model = GetOptionAsString("geometric_model", "f");
        std::string input_pairs = GetOptionAsString("input_pairs", "");
        std::string output_pairs = GetOptionAsString("output_pairs", "");
        std::string force = GetOptionAsBool("force_compute", false) ? "-f 1" : "";
        std::string guided_matching = GetOptionAsBool("guided_matching", false) ? "-r 1" : "";
        std::string max_iteration = GetOptionAsString("max_iteration", "2048");
        std::string cache_size = GetOptionAsString("cache_size", "0");

        // Get input matches file path (for -m parameter) | 获取输入匹配文件路径 (用于 -m 参数)
        std::string geom_in_cfg = GetOptionAsString("geom_filter_in", "");
        std::string geom_in_path;
        if (geom_in_cfg.empty())
        {
            geom_in_path = putative_matches_path_; // Use member variable directly | 直接使用成员变量
        }
        else
        {
            geom_in_path = matches_dir_ + "/" + geom_in_cfg;
        }

        // Set final matches file path (for -o parameter) | 设置最终匹配文件路径 (用于 -o 参数)
        std::string final_tpl = GetOptionAsString("geom_filter_out_tpl", "matches.{GEOM_MODEL}.bin");
        std::string final_fn = final_tpl;
        size_t ph_pos = final_fn.find("{GEOM_MODEL}");
        if (ph_pos != std::string::npos)
        {
            final_fn.replace(ph_pos, std::string("{GEOM_MODEL}").length(), geometric_model);
        }
        final_matches_path_ = matches_dir_ + "/" + final_fn;

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << sfm_data_path_;      // Input sfm_data file | 输入sfm_data文件
        cmd << " -o " << final_matches_path_; // Output final matches file, path generated based on configuration | 输出最终匹配文件，路径根据配置生成
        cmd << " -m " << geom_in_path;        // Input putative matches file, path based on configuration or member variable | 输入推测匹配文件，路径根据配置或成员变量生成

        if (!input_pairs.empty())
        {
            cmd << " -p " << input_pairs;
        }

        if (!output_pairs.empty())
        {
            cmd << " -s " << output_pairs;
        }

        cmd << " -g " << geometric_model;

        if (!force.empty())
        {
            cmd << " " << force;
        }

        if (!guided_matching.empty())
        {
            cmd << " " << guided_matching;
        }

        if (!max_iteration.empty())
        {
            cmd << " -I " << max_iteration;
        }

        if (!cache_size.empty() && cache_size != "0")
        {
            cmd << " -c " << cache_size;
        }

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunPairGenerator()
    {
        // Start profiling for PairGenerator subprocess | 开始PairGenerator子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_pair_generator");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_PairGenerator";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Set pairs file path | 设置pairs文件路径
        std::string pairs_filename = GetOptionAsString("pairs_file", "pairs.bin");
        pairs_path_ = matches_dir_ + "/" + pairs_filename;

        // Prepare PairGenerator parameters | 准备PairGenerator参数
        std::string pair_mode = GetOptionAsString("pair_mode", "");
        std::string contiguous_count = GetOptionAsString("contiguous_count", "");

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << sfm_data_path_; // Input sfm_data file | 输入sfm_data文件
        cmd << " -o " << pairs_path_;    // Output pairs file | 输出pairs文件

        if (!pair_mode.empty())
        {
            cmd << " -m " << pair_mode;
        }

        if (!contiguous_count.empty())
        {
            cmd << " -c " << contiguous_count;
        }

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunSfM()
    {
        // Start profiling for SfM subprocess | 开始SfM子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_sfm_reconstruction");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_SfM";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Set reconstruction output directory | 设置重建输出目录
        std::string reconstruction_dirname = GetOptionAsString("reconstruction_dir", "reconstruction_global");
        reconstruction_dir_ = work_dir_ + "/" + reconstruction_dirname;
        std::filesystem::create_directories(reconstruction_dir_);

        // Prepare SfM parameters | 准备SfM参数
        std::string sfm_engine = GetOptionAsString("sfm_engine", "GLOBAL");
        std::string match_file_for_sfm;

        // Select match file based on whether PairGenerator is enabled | 根据是否启用了PairGenerator来选择匹配文件
        if (GetOptionAsBool("enable_pair_generator", false))
        {
            match_file_for_sfm = final_matches_path_;
        }
        else
        {
            match_file_for_sfm = final_matches_path_;
        }

        // SfM engine specific parameters | SfM引擎特定参数
        std::string refine_intrinsic = GetOptionAsString("refine_intrinsic_config", "ADJUST_ALL");
        std::string refine_extrinsic = GetOptionAsString("refine_extrinsic_config", "ADJUST_ALL");
        std::string triangulation_method = GetOptionAsString("triangulation_method", "");
        std::string resection_method = GetOptionAsString("resection_method", "");
        std::string sfm_camera_model = GetOptionAsString("sfm_camera_model", "3");

        // Global SfM parameters | Global SfM 参数
        std::string rotation_averaging = GetOptionAsString("rotation_averaging", "2");
        std::string translation_averaging = GetOptionAsString("translation_averaging", "3");

        // Incremental SfM parameters | Incremental SfM 参数
        std::string initial_pair_a = GetOptionAsString("initial_pair_a", "");
        std::string initial_pair_b = GetOptionAsString("initial_pair_b", "");
        std::string sfm_initializer = GetOptionAsString("sfm_initializer", "STELLAR");

        // Stellar SfM parameters | Stellar SfM 参数
        std::string graph_simplification = GetOptionAsString("graph_simplification", "MST_X");
        std::string graph_simplification_value = GetOptionAsString("graph_simplification_value", "5");

        // PoSDK tracks export | PoSDK tracks导出
        std::string export_tracks_file = GetOptionAsPath("export_tracks_file", "");

        // PoSDK relative poses export | PoSDK relative poses导出
        std::string export_relative_poses_file = GetOptionAsPath("export_relative_poses_file", "");

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << sfm_data_path_;      // Input sfm_data file | 输入sfm_data文件
        cmd << " -M " << match_file_for_sfm;  // Matches file | 匹配文件
        cmd << " -o " << reconstruction_dir_; // Output directory | 输出目录
        cmd << " -s " << sfm_engine;          // SfM engine | SfM引擎

        // Export tracks if requested | 如果请求导出tracks
        if (!export_tracks_file.empty())
        {
            cmd << " -E " << export_tracks_file;
        }

        // Export relative poses if requested | 如果请求导出relative poses
        if (!export_relative_poses_file.empty())
        {
            cmd << " -Q " << export_relative_poses_file;
        }

        // Bundle adjustment parameters | 束调优化参数
        if (!refine_intrinsic.empty())
        {
            cmd << " -f " << refine_intrinsic;
        }
        if (!refine_extrinsic.empty())
        {
            cmd << " -e " << refine_extrinsic;
        }

        // Priors | 先验信息
        if (GetOptionAsBool("use_motion_priors", false))
        {
            cmd << " -P";
        }

        // Engine specific parameters | 引擎特定参数
        if (sfm_engine == "INCREMENTAL" || sfm_engine == "INCREMENTALV2")
        {
            if (!triangulation_method.empty())
            {
                cmd << " -t " << triangulation_method;
            }
            if (!resection_method.empty())
            {
                cmd << " -r " << resection_method;
            }
            if (!sfm_camera_model.empty())
            {
                cmd << " -c " << sfm_camera_model;
            }

            if (sfm_engine == "INCREMENTAL")
            {
                if (!initial_pair_a.empty() && !initial_pair_b.empty())
                {
                    cmd << " -a " << initial_pair_a;
                    cmd << " -b " << initial_pair_b;
                }
            }
            else if (sfm_engine == "INCREMENTALV2")
            {
                if (!sfm_initializer.empty())
                {
                    cmd << " -S " << sfm_initializer;
                }
            }
        }
        else if (sfm_engine == "GLOBAL")
        {
            if (!rotation_averaging.empty())
            {
                cmd << " -R " << rotation_averaging;
            }
            if (!translation_averaging.empty())
            {
                cmd << " -T " << translation_averaging;
            }
        }
        else if (sfm_engine == "STELLAR")
        {
            if (!graph_simplification.empty())
            {
                cmd << " -G " << graph_simplification;
            }
            if (!graph_simplification_value.empty())
            {
                cmd << " -g " << graph_simplification_value;
            }
        }

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        if (ret == 0)
        {
            // Set final SfM data file path | 设置最终SfM数据文件路径
            final_sfm_data_path_ = reconstruction_dir_ + "/sfm_data.bin";
        }

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunComputeSfM_DataColor()
    {
        // Start profiling for ComputeSfM_DataColor subprocess | 开始ComputeSfM_DataColor子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_compute_sfm_data_color");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_ComputeSfM_DataColor";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Set colored point cloud file path | 设置着色点云文件路径
        std::string colored_ply_filename = GetOptionAsString("colored_ply_file", "colorized.ply");
        colored_ply_path_ = reconstruction_dir_ + "/" + colored_ply_filename;

        // Build command line | 构建命令行
        std::stringstream cmd;
        cmd << bin_path;
        cmd << " -i " << final_sfm_data_path_; // Input reconstructed sfm_data.bin | 输入重建后的sfm_data.bin
        cmd << " -o " << colored_ply_path_;    // Output colored point cloud file | 输出着色点云文件

        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

    bool OpenMVGPipeline::RunEvalQuality()
    {
        // Start profiling for EvalQuality subprocess | 开始EvalQuality子进程性能分析
        PROFILER_START_AUTO(true);
        PROFILER_STAGE("openmvg_eval_quality");

        if (bin_folder_.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制目录";
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary directory not found";
            return false;
        }
        std::string bin_path = bin_folder_ + "/openMVG_main_evalQuality";

        if (!CheckOpenMVGBinary(bin_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未找到OpenMVG二进制文件: " << bin_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] OpenMVG binary not found: " << bin_path;
            return false;
        }

        // Get ground truth dataset path | 获取真值数据集路径
        std::string gt_dataset_path = GetOptionAsPath("gt_dataset_path");
        if (gt_dataset_path.empty())
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 未指定质量评估的真值数据集路径";
            LOG_ERROR_EN << "[OpenMVGPipeline] Ground truth dataset path not specified for quality evaluation";
            return false;
        }

        // Check if ground truth dataset exists | 检查真值数据集是否存在
        if (!std::filesystem::exists(gt_dataset_path))
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 真值数据集不存在: " << gt_dataset_path;
            LOG_ERROR_EN << "[OpenMVGPipeline] Ground truth dataset does not exist: " << gt_dataset_path;
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
        LOG_DEBUG_ZH << "[OpenMVGPipeline] 运行质量评估:";
        LOG_DEBUG_ZH << "  真值: " << gt_dataset_path;
        LOG_DEBUG_ZH << "  重建: " << final_sfm_data_path_;
        LOG_DEBUG_ZH << "  输出: " << eval_output_dir_;
        LOG_DEBUG_ZH << "命令: " << cmd.str();
        LOG_DEBUG_EN << "[OpenMVGPipeline] Running quality evaluation:";
        LOG_DEBUG_EN << "  Ground Truth: " << gt_dataset_path;
        LOG_DEBUG_EN << "  Reconstruction: " << final_sfm_data_path_;
        LOG_DEBUG_EN << "  Output: " << eval_output_dir_;
        LOG_DEBUG_EN << "Command: " << cmd.str();

        // Execute command | 执行命令
        int ret = POSDK_SYSTEM(cmd.str().c_str());

        if (ret == 0)
        {
            LOG_INFO_ZH << "[OpenMVGPipeline] 质量评估成功完成!";
            LOG_INFO_ZH << "结果保存到: " << eval_output_dir_;
            LOG_INFO_EN << "[OpenMVGPipeline] Quality evaluation completed successfully!";
            LOG_INFO_EN << "Results saved to: " << eval_output_dir_;

            // Check generated files | 检查生成的文件
            std::string html_report = eval_output_dir_ + "/ExternalCalib_Report.html";
            std::string json_stats = eval_output_dir_ + "/gt_eval_stats_blob.json";

            if (std::filesystem::exists(html_report))
            {
                LOG_DEBUG_ZH << "[OpenMVGPipeline] HTML报告: " << html_report;
                LOG_DEBUG_EN << "[OpenMVGPipeline] HTML report: " << html_report;
            }

            if (std::filesystem::exists(json_stats))
            {
                LOG_DEBUG_ZH << "[OpenMVGPipeline] JSON统计: " << json_stats;
                LOG_DEBUG_EN << "[OpenMVGPipeline] JSON statistics: " << json_stats;
            }
        }
        else
        {
            LOG_ERROR_ZH << "[OpenMVGPipeline] 质量评估失败，返回码: " << ret;
            LOG_ERROR_EN << "[OpenMVGPipeline] Quality evaluation failed with return code: " << ret;
        }

        // End profiling and display statistics | 结束性能分析并显示统计信息
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return (ret == 0);
    }

} // namespace PoSDKPlugin

// Register plugin - updated to new name | 注册插件 - 更新为新名称
// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PoSDKPlugin::OpenMVGPipeline)
