#include "LightGlueMatcher.hpp"
#include <po_core/po_logger.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <random>

namespace PluginMethods
{
    using namespace PoSDK;

    LightGlueMatcher::LightGlueMatcher(const LightGlueParameters &params)
        : params_(params), initialized_(false)
    {
    }

    LightGlueMatcher::~LightGlueMatcher()
    {
        // Clean up temporary directory | 清理临时目录
        if (!temp_dir_.empty() && std::filesystem::exists(temp_dir_))
        {
            std::error_code ec;
            std::filesystem::remove_all(temp_dir_, ec);
        }
    }

    bool LightGlueMatcher::Initialize()
    {
        if (initialized_)
            return true;

        // 1. Check Python environment | 检查Python环境
        if (!CheckEnvironment(params_.python_executable))
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] Python环境检查失败: " << params_.python_executable;
            LOG_ERROR_EN << "[LightGlueMatcher] Python environment check failed for: " << params_.python_executable;
            return false;
        }

        // 2. Find LightGlue script | 查找LightGlue脚本
        script_path_ = params_.script_path.empty() ? FindLightGlueScript() : params_.script_path;
        if (script_path_.empty())
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 未找到LightGlue Python脚本";
            LOG_ERROR_EN << "[LightGlueMatcher] LightGlue Python script not found";
            return false;
        }

        // 3. Create temporary directory | 创建临时目录
        temp_dir_ = CreateTempDirectory();
        if (temp_dir_.empty())
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 创建临时目录失败";
            LOG_ERROR_EN << "[LightGlueMatcher] Failed to create temporary directory";
            return false;
        }

        // 4. Check feature type support | 检查特征类型支持
        if (!IsSupportedFeatureType(params_.feature_type))
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 不支持的特征类型: " << FeatureTypeToString(params_.feature_type);
            LOG_ERROR_EN << "[LightGlueMatcher] Unsupported feature type: " << FeatureTypeToString(params_.feature_type);
            return false;
        }

        initialized_ = true;
        LOG_INFO_ZH << "[LightGlueMatcher] LightGlue匹配器初始化成功";
        LOG_INFO_ZH << "[LightGlueMatcher] 脚本路径: " << script_path_;
        LOG_INFO_ZH << "[LightGlueMatcher] 临时目录: " << temp_dir_;
        LOG_INFO_ZH << "[LightGlueMatcher] 特征类型: " << FeatureTypeToString(params_.feature_type);
        LOG_INFO_EN << "[LightGlueMatcher] LightGlue matcher initialized successfully";
        LOG_INFO_EN << "[LightGlueMatcher] Script path: " << script_path_;
        LOG_INFO_EN << "[LightGlueMatcher] Temp dir: " << temp_dir_;
        LOG_INFO_EN << "[LightGlueMatcher] Feature type: " << FeatureTypeToString(params_.feature_type);

        return true;
    }

    bool LightGlueMatcher::MatchFeatures(
        const cv::Mat &img1, const cv::Mat &img2,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const cv::Mat &descriptors1, const cv::Mat &descriptors2,
        std::vector<cv::DMatch> &matches)
    {
        if (!initialized_ && !Initialize())
        {
            return false;
        }

        if (keypoints1.empty() || keypoints2.empty() || descriptors1.empty() || descriptors2.empty())
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 关键点或描述子为空";
            LOG_ERROR_EN << "[LightGlueMatcher] Empty keypoints or descriptors";
            return false;
        }

        // Generate temporary file paths | 生成临时文件路径
        std::string temp_id = std::to_string(std::rand());
        std::string img1_path = temp_dir_ + "/img1_" + temp_id + ".png";
        std::string img2_path = temp_dir_ + "/img2_" + temp_id + ".png";
        std::string features1_path = temp_dir_ + "/features1_" + temp_id + ".txt";
        std::string features2_path = temp_dir_ + "/features2_" + temp_id + ".txt";
        std::string matches_path = temp_dir_ + "/matches_" + temp_id + ".txt";

        std::vector<std::string> temp_files = {img1_path, img2_path, features1_path, features2_path, matches_path};

        try
        {
            // 1. Save images | 保存图像
            if (!cv::imwrite(img1_path, img1))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 保存图像1失败: " << img1_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Failed to save image1 to: " << img1_path;
                return false;
            }
            if (!cv::imwrite(img2_path, img2))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 保存图像2失败: " << img2_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Failed to save image2 to: " << img2_path;
                CleanupTempFiles(temp_files);
                return false;
            }

            // 2. Save feature data | 保存特征数据
            if (!SaveFeaturesToFile(keypoints1, descriptors1, features1_path))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 保存特征1失败: " << features1_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Failed to save features1 to: " << features1_path;
                CleanupTempFiles(temp_files);
                return false;
            }
            if (!SaveFeaturesToFile(keypoints2, descriptors2, features2_path))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 保存特征2失败: " << features2_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Failed to save features2 to: " << features2_path;
                CleanupTempFiles(temp_files);
                return false;
            }

            // 3. Run Python script | 运行Python脚本
            if (!RunPythonScript(img1_path, img2_path, features1_path, features2_path, matches_path))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] Python脚本执行失败";
                LOG_ERROR_EN << "[LightGlueMatcher] Python script execution failed";
                CleanupTempFiles(temp_files);
                return false;
            }

            // 4. Load matching results | 加载匹配结果
            if (!LoadMatchesFromFile(matches_path, matches))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 从文件加载匹配结果失败: " << matches_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Failed to load matches from: " << matches_path;
                CleanupTempFiles(temp_files);
                return false;
            }

            // 5. Clean up temporary files | 清理临时文件
            CleanupTempFiles(temp_files);

            LOG_INFO_ZH << "[LightGlueMatcher] LightGlue匹配成功，找到 " << matches.size() << " 个匹配";
            LOG_INFO_EN << "[LightGlueMatcher] LightGlue matching successful, found " << matches.size() << " matches";
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] LightGlue匹配过程中出现异常: " << e.what();
            LOG_ERROR_EN << "[LightGlueMatcher] Exception in LightGlue matching: " << e.what();
            CleanupTempFiles(temp_files);
            return false;
        }
    }

    bool LightGlueMatcher::Match(
        const LightGlueParameters &params,
        const cv::Mat &img1, const cv::Mat &img2,
        const std::vector<cv::KeyPoint> &keypoints1,
        const std::vector<cv::KeyPoint> &keypoints2,
        const cv::Mat &descriptors1, const cv::Mat &descriptors2,
        std::vector<cv::DMatch> &matches)
    {
        LightGlueMatcher matcher(params);
        return matcher.MatchFeatures(img1, img2, keypoints1, keypoints2, descriptors1, descriptors2, matches);
    }

    bool LightGlueMatcher::IsSupportedFeatureType(LightGlueFeatureType feature_type)
    {
        switch (feature_type)
        {
        case LightGlueFeatureType::SUPERPOINT:
        case LightGlueFeatureType::DISK:
        case LightGlueFeatureType::SIFT:
        case LightGlueFeatureType::ALIKED:
        case LightGlueFeatureType::DOGHARDNET:
            return true;
        default:
            return false;
        }
    }

    bool LightGlueMatcher::CheckEnvironment(const std::string &python_exe)
    {
        // 1. First check basic Python environment | 首先检查基本Python环境
        std::string check_cmd = python_exe + " --version >/dev/null 2>&1";
        int result = std::system(check_cmd.c_str());

        if (result != 0)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] Python可执行文件未找到或无法工作: " << python_exe;
            LOG_ERROR_EN << "[LightGlueMatcher] Python executable not found or not working: " << python_exe;
            return false;
        }

        // 2. Check required Python packages | 检查必要的Python包
        std::vector<std::string> required_packages = {"torch", "numpy", "cv2"};
        for (const std::string &package : required_packages)
        {
            std::string import_cmd = python_exe + " -c \"import " + package + "\" >/dev/null 2>&1";
            if (std::system(import_cmd.c_str()) != 0)
            {
                LOG_WARNING_ZH << "[LightGlueMatcher] 未找到必要的Python包: " << package;
                LOG_WARNING_EN << "[LightGlueMatcher] Required Python package not found: " << package;

                // Try to setup environment | 尝试配置环境
                if (!TrySetupEnvironment())
                {
                    LOG_ERROR_ZH << "[LightGlueMatcher] 环境设置失败";
                    LOG_ERROR_EN << "[LightGlueMatcher] Environment setup failed";
                    return false;
                }

                // Recheck | 重新检查
                if (std::system(import_cmd.c_str()) != 0)
                {
                    LOG_ERROR_ZH << "[LightGlueMatcher] 环境设置后包仍然不可用: " << package;
                    LOG_ERROR_EN << "[LightGlueMatcher] Package still not available after environment setup: " << package;
                    return false;
                }
            }
        }

        LOG_INFO_ZH << "[LightGlueMatcher] Python环境检查通过: " << python_exe;
        LOG_INFO_EN << "[LightGlueMatcher] Python environment check passed for: " << python_exe;
        return true;
    }

    bool LightGlueMatcher::TrySetupEnvironment()
    {
        LOG_INFO_ZH << "[LightGlueMatcher] 尝试设置Python环境...";
        LOG_INFO_EN << "[LightGlueMatcher] Attempting to setup Python environment...";

        // Find environment configuration script | 查找环境配置脚本
        std::vector<std::string> config_scripts = {
            "../Img2Features/configure_lightglue_env.sh",
            "../../Img2Features/configure_lightglue_env.sh",
            "../../../po_core/drawer/configure_drawer_env.sh"};

        for (const std::string &script_path : config_scripts)
        {
            if (std::filesystem::exists(script_path))
            {
                LOG_INFO_ZH << "[LightGlueMatcher] 运行环境配置脚本: " << script_path;
                LOG_INFO_EN << "[LightGlueMatcher] Running environment config script: " << script_path;

                std::string config_cmd = "bash \"" + script_path + "\" >/dev/null 2>&1";
                int result = std::system(config_cmd.c_str());

                if (result == 0)
                {
                    LOG_INFO_ZH << "[LightGlueMatcher] 环境配置成功";
                    LOG_INFO_EN << "[LightGlueMatcher] Environment configuration successful";
                    return true;
                }
                else
                {
                    LOG_ERROR_ZH << "[LightGlueMatcher] 环境配置失败，错误码: " << result;
                    LOG_ERROR_EN << "[LightGlueMatcher] Environment configuration failed with code: " << result;
                }

                break; // Only try the first script found | 只尝试第一个找到的脚本
            }
        }

        return false;
    }

    std::string LightGlueMatcher::FindLightGlueScript()
    {
        // Find script location based on plugin-config.cmake installation logic
        // 根据plugin-config.cmake的安装逻辑查找脚本位置
        std::vector<std::string> possible_paths = {
            // 1. Plugin Python directory in build output directory (location installed by plugin-config.cmake)
            // 1. 构建输出目录中的插件Python目录（plugin-config.cmake安装的位置）
            "plugins/methods/lightglue_matcher.py",
            "output/plugins/methods/lightglue_matcher.py",
            "../output/plugins/methods/lightglue_matcher.py",
            "../../output/plugins/methods/lightglue_matcher.py",

            // 2. Plugin source code directory | 2. 插件源代码目录
            "src/plugins/methods/Img2Matches/lightglue_matcher.py",
            "../plugins/methods/Img2Matches/lightglue_matcher.py",
            "../../plugins/methods/Img2Matches/lightglue_matcher.py",

            // 3. Alternative locations (backward compatibility) | 3. 备选位置（向后兼容）
            "src/dependencies/LightGlue-main/lightglue_matcher.py",
            "dependencies/LightGlue-main/lightglue_matcher.py"};

        for (const std::string &path : possible_paths)
        {
            if (std::filesystem::exists(path))
            {
                LOG_INFO_ZH << "[LightGlueMatcher] 在以下位置找到LightGlue脚本: " << path;
                LOG_INFO_EN << "[LightGlueMatcher] Found LightGlue script at: " << path;
                return std::filesystem::absolute(path).string();
            }
        }

        LOG_ERROR_ZH << "[LightGlueMatcher] 在标准位置未找到LightGlue脚本";
        LOG_ERROR_EN << "[LightGlueMatcher] LightGlue script not found in standard locations";
        return "";
    }

    std::string LightGlueMatcher::CreateTempDirectory()
    {
        try
        {
            // Generate unique temporary directory name | 生成唯一的临时目录名
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);

            std::string temp_name = "lightglue_temp_" + std::to_string(dis(gen));
            std::filesystem::path temp_path = std::filesystem::temp_directory_path() / temp_name;

            if (std::filesystem::create_directories(temp_path))
            {
                return temp_path.string();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 创建临时目录时出现异常: " << e.what();
            LOG_ERROR_EN << "[LightGlueMatcher] Exception creating temp directory: " << e.what();
        }

        return "";
    }

    bool LightGlueMatcher::SaveFeaturesToFile(
        const std::vector<cv::KeyPoint> &keypoints,
        const cv::Mat &descriptors,
        const std::string &output_path)
    {
        try
        {
            std::ofstream file(output_path);
            if (!file.is_open())
            {
                return false;
            }

            // Write number of keypoints | 写入特征点数量
            file << keypoints.size() << std::endl;

            // Write keypoint information | 写入特征点信息
            for (size_t i = 0; i < keypoints.size(); ++i)
            {
                const cv::KeyPoint &kp = keypoints[i];
                file << kp.pt.x << " " << kp.pt.y << " " << kp.size << " " << kp.angle << " " << kp.response;

                // Write descriptors | 写入描述子
                if (!descriptors.empty() && i < static_cast<size_t>(descriptors.rows))
                {
                    cv::Mat desc = descriptors.row(i);
                    for (int j = 0; j < desc.cols; ++j)
                    {
                        file << " " << desc.at<float>(0, j);
                    }
                }
                file << std::endl;
            }

            file.close();
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 保存特征时出现异常: " << e.what();
            LOG_ERROR_EN << "[LightGlueMatcher] Exception saving features: " << e.what();
            return false;
        }
    }

    bool LightGlueMatcher::LoadMatchesFromFile(
        const std::string &matches_path,
        std::vector<cv::DMatch> &matches)
    {
        try
        {
            std::ifstream file(matches_path);
            if (!file.is_open())
            {
                return false;
            }

            matches.clear();
            std::string line;

            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                int query_idx, train_idx;
                float distance;

                if (iss >> query_idx >> train_idx >> distance)
                {
                    matches.emplace_back(query_idx, train_idx, distance);
                }
            }

            file.close();
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 加载匹配结果时出现异常: " << e.what();
            LOG_ERROR_EN << "[LightGlueMatcher] Exception loading matches: " << e.what();
            return false;
        }
    }

    bool LightGlueMatcher::RunPythonScript(
        const std::string &img1_path,
        const std::string &img2_path,
        const std::string &features1_path,
        const std::string &features2_path,
        const std::string &output_path)
    {
        try
        {
            // Build Python command | 构建Python命令
            std::ostringstream cmd;
            cmd << params_.python_executable << " \"" << script_path_ << "\""
                << " --img1 \"" << img1_path << "\""
                << " --img2 \"" << img2_path << "\""
                << " --features1 \"" << features1_path << "\""
                << " --features2 \"" << features2_path << "\""
                << " --output \"" << output_path << "\""
                << " --feature_type " << FeatureTypeToString(params_.feature_type)
                << " --max_keypoints " << params_.max_num_keypoints
                << " --depth_confidence " << params_.depth_confidence
                << " --width_confidence " << params_.width_confidence
                << " --filter_threshold " << params_.filter_threshold;

            if (params_.flash_attention)
                cmd << " --flash_attention";
            if (params_.mixed_precision)
                cmd << " --mixed_precision";
            if (params_.compile_model)
                cmd << " --compile_model";

            cmd << " 2>&1"; // Redirect error output | 重定向错误输出

            LOG_DEBUG_ZH << "[LightGlueMatcher] 执行Python命令: " << cmd.str();
            LOG_DEBUG_EN << "[LightGlueMatcher] Executing Python command: " << cmd.str();

            // Execute command | 执行命令
            int result = std::system(cmd.str().c_str());

            if (result != 0)
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] Python脚本执行失败，错误码: " << result;
                LOG_ERROR_EN << "[LightGlueMatcher] Python script execution failed with code: " << result;
                return false;
            }

            // Check if output file exists | 检查输出文件是否存在
            if (!std::filesystem::exists(output_path))
            {
                LOG_ERROR_ZH << "[LightGlueMatcher] 输出文件未创建: " << output_path;
                LOG_ERROR_EN << "[LightGlueMatcher] Output file not created: " << output_path;
                return false;
            }

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[LightGlueMatcher] 运行Python脚本时出现异常: " << e.what();
            LOG_ERROR_EN << "[LightGlueMatcher] Exception running Python script: " << e.what();
            return false;
        }
    }

    void LightGlueMatcher::CleanupTempFiles(const std::vector<std::string> &file_paths)
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
                LOG_WARNING_ZH << "[LightGlueMatcher] 清理文件失败 " << path << ": " << e.what();
                LOG_WARNING_EN << "[LightGlueMatcher] Failed to cleanup file " << path << ": " << e.what();
            }
        }
    }

    std::string LightGlueMatcher::FeatureTypeToString(LightGlueFeatureType type)
    {
        switch (type)
        {
        case LightGlueFeatureType::SUPERPOINT:
            return "superpoint";
        case LightGlueFeatureType::DISK:
            return "disk";
        case LightGlueFeatureType::SIFT:
            return "sift";
        case LightGlueFeatureType::ALIKED:
            return "aliked";
        case LightGlueFeatureType::DOGHARDNET:
            return "doghardnet";
        default:
            return "superpoint";
        }
    }

} // namespace PluginMethods
