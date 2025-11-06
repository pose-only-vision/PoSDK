#include "converter_openmvg_file.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <nlohmann/json.hpp> // 需要添加这个第三方库来解析JSON | Need to add this third-party library for JSON parsing
#include <po_core/po_logger.hpp>

// 为了支持OpenMVG的二进制匹配文件，需要添加cereal库支持
// Add cereal library support for OpenMVG binary match files
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>

// 添加必要的STL头文件 | Add necessary STL headers
#include <map>
#include <utility>

namespace PoSDK
{
    namespace Converter
    {
        // 前向声明辅助函数 | Forward declaration of helper functions
        static bool LoadSfMDataFromJSON(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses);

        static bool LoadSfMDataFromBinary(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses);

        static bool LoadSfMDataFromXML(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses);

        // 定义一个临时的IndMatch结构，用于cereal反序列化OpenMVG的匹配文件
        // Define a temporary IndMatch structure for cereal deserialization of OpenMVG match files
        struct IndMatch
        {
            types::IndexT i_, j_;

            IndMatch() : i_(0), j_(0) {}
            IndMatch(const types::IndexT i, const types::IndexT j) : i_(i), j_(j) {}

            // Serialization
            template <class Archive>
            void serialize(Archive &ar)
            {
                ar(i_, j_);
            }

            // 转换为PoMVG的IdMatch | Convert to PoMVG IdMatch
            types::IdMatch toPoSDK() const
            {
                types::IdMatch match;
                match.i = i_;
                match.j = j_;
                match.is_inlier = true; // 默认为内点 | Default as inlier
                return match;
            }
        };

        // 定义PairWiseMatches类型，对应OpenMVG中的map<pair<ViewId,ViewId>, vector<IndMatch>>结构
        // Define PairWiseMatches type corresponding to OpenMVG's map<pair<ViewId,ViewId>, vector<IndMatch>> structure
        using PairWiseMatches = std::map<std::pair<types::IndexT, types::IndexT>, std::vector<IndMatch>>;

        // 定义临时的Pose3结构，用于解析OpenMVG的pose数据
        // Define temporary Pose3 structure for parsing OpenMVG pose data
        struct TempPose3
        {
            std::vector<std::vector<double>> rotation; // 3x3旋转矩阵 | 3x3 rotation matrix
            std::vector<double> center;                // 3D中心点 | 3D center point

            TempPose3() : rotation(3, std::vector<double>(3, 0.0)), center(3, 0.0) {}

            // 转换为PoSDK的格式 | Convert to PoSDK format
            void ToPoSDK(types::Matrix3d &R, types::Vector3d &t) const
            {
                // 设置旋转矩阵 | Set rotation matrix
                for (int i = 0; i < 3; ++i)
                {
                    for (int j = 0; j < 3; ++j)
                    {
                        R(i, j) = rotation[i][j];
                    }
                }

                // 设置平移向量 | Set translation vector
                // 根据测试结果，OpenMVG的"center"字段实际上可能已经是平移向量
                // 而不是相机中心，所以我们直接使用它
                // According to test results, OpenMVG's "center" field might actually be the translation vector
                // rather than camera center, so we use it directly
                types::Vector3d center_vec(center[0], center[1], center[2]);
                t = center_vec;
            }

            // Cereal序列化支持 | Cereal serialization support
            template <class Archive>
            void serialize(Archive &ar)
            {
                ar(cereal::make_nvp("rotation", rotation));
                ar(cereal::make_nvp("center", center));
            }
        };

        using json = nlohmann::json;

        bool OpenMVGFileConverter::LoadFeatures(
            const std::string &features_file,
            ImageFeatureInfo &image_features)
        {
            image_features.ClearAllFeatures();

            std::ifstream file(features_file);
            if (!file.is_open())
            {
                LOG_ERROR_ZH << "无法打开特征文件: " << features_file;
                LOG_ERROR_EN << "Cannot open features file: " << features_file;
                return false;
            }

            std::string line;
            float x, y, scale = 0.0f, orientation = 0.0f;

            // Parse each line of the features file | 解析特征文件的每一行
            while (std::getline(file, line))
            {
                std::istringstream iss(line);

                // Try to read coordinate values | 尝试读取坐标值
                if (!(iss >> x >> y))
                {
                    continue; // Skip invalid lines | 跳过无效行
                }

                // Try to read optional scale and orientation (if present) | 尝试读取可选的尺度和方向（如果存在）
                if (iss >> scale >> orientation)
                {
                    // SIOPointFeature format | SIOPointFeature格式
                    image_features.AddFeature(Feature(x, y), scale, orientation);
                }
                else
                {
                    // Basic PointFeature format | 基本PointFeature格式
                    image_features.AddFeature(Feature(x, y));
                }
            }

            file.close();
            return image_features.GetNumFeatures() > 0;
        }

        bool OpenMVGFileConverter::LoadMatches(
            const std::string &matches_file,
            types::Matches &matches)
        {
            matches.clear();

            // Determine file type | 判断文件类型
            std::string ext = std::filesystem::path(matches_file).extension().string();
            if (ext == ".txt")
            {
                // Text format matches file | 文本格式匹配文件
                std::ifstream file(matches_file);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "无法打开匹配文件: " << matches_file;
                    LOG_ERROR_EN << "Cannot open matches file: " << matches_file;
                    return false;
                }

                types::IndexT I, J, number;
                // Format | 格式：
                // I J
                // #matches count
                // idx idx
                // ...
                while (file >> I >> J >> number)
                {
                    // Create new view pair | 创建新的视图对
                    types::ViewPair view_pair(I, J);
                    types::IdMatches id_matches;
                    id_matches.reserve(number);

                    // Read match pairs | 读取匹配对
                    for (types::IndexT i = 0; i < number; ++i)
                    {
                        types::IndexT id_i, id_j;
                        if (file >> id_i >> id_j)
                        {
                            types::IdMatch match;
                            match.i = id_i;
                            match.j = id_j;
                            match.is_inlier = true; // Default as inlier | 默认为内点
                            id_matches.push_back(match);
                        }
                    }

                    // Only add when matches are read | 只有在读取到匹配时才添加
                    if (!id_matches.empty())
                    {
                        matches[view_pair] = std::move(id_matches);
                    }
                }

                file.close();
            }
            else if (ext == ".bin")
            {
                // Binary format matches file - use cereal library for deserialization
                // 二进制格式匹配文件 - 使用cereal库反序列化
                std::ifstream file(matches_file, std::ios::binary);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "无法打开二进制匹配文件: " << matches_file;
                    LOG_ERROR_EN << "Cannot open binary matches file: " << matches_file;
                    return false;
                }

                try
                {
                    // Use cereal library to deserialize OpenMVG matches file
                    // 使用cereal库反序列化OpenMVG的匹配文件
                    cereal::PortableBinaryInputArchive archive(file);

                    // Deserialize to our temporarily defined PairWiseMatches structure
                    // 反序列化到我们临时定义的PairWiseMatches结构
                    PairWiseMatches openmvg_matches;
                    archive(openmvg_matches);
                    file.close();

                    // Convert to PoMVG's match structure | 转换为PoMVG的匹配结构
                    for (const auto &pair_match : openmvg_matches)
                    {
                        types::ViewPair view_pair(pair_match.first.first, pair_match.first.second);
                        types::IdMatches id_matches;
                        id_matches.reserve(pair_match.second.size());

                        // Convert each match pair | 转换每个匹配对
                        for (const auto &ind_match : pair_match.second)
                        {
                            id_matches.push_back(ind_match.toPoSDK());
                        }

                        // Only add when there are matches | 只有在有匹配时才添加
                        if (!id_matches.empty())
                        {
                            matches[view_pair] = std::move(id_matches);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "反序列化二进制匹配文件时出错: " << e.what();
                    LOG_ERROR_EN << "Error deserializing binary matches: " << e.what();
                    file.close();
                    return false;
                }
            }
            else
            {
                LOG_ERROR_ZH << "不支持的匹配文件格式: " << ext;
                LOG_ERROR_EN << "Unsupported matches file format: " << ext;
                return false;
            }

            return !matches.empty();
        }

        bool OpenMVGFileConverter::LoadSfMData(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses)
        {
            try
            {
                // Determine file format | 判断文件格式
                std::string ext = std::filesystem::path(sfm_data_file).extension().string();

                if (ext == ".json")
                {
                    return LoadSfMDataFromJSON(sfm_data_file, global_poses);
                }
                else if (ext == ".bin")
                {
                    return LoadSfMDataFromBinary(sfm_data_file, global_poses);
                }
                else if (ext == ".xml")
                {
                    return LoadSfMDataFromXML(sfm_data_file, global_poses);
                }
                else
                {
                    LOG_ERROR_ZH << "不支持的SfM数据文件格式: " << ext;
                    LOG_ERROR_EN << "Unsupported SfM data file format: " << ext;
                    return false;
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "加载SfM数据的全局位姿时出错: " << e.what();
                LOG_ERROR_EN << "Error loading SfM data for global poses: " << e.what();
                return false;
            }
        }

        bool OpenMVGFileConverter::LoadSfMData(
            const std::string &sfm_data_file,
            std::vector<std::pair<types::IndexT, std::string>> &image_paths,
            bool views_only)
        {
            image_paths.clear();

            try
            {
                // Read and parse JSON file | 读取并解析JSON文件
                std::ifstream file(sfm_data_file);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "无法打开SfM数据文件: " << sfm_data_file;
                    LOG_ERROR_EN << "Cannot open SfM data file: " << sfm_data_file;
                    return false;
                }

                json sfm_data;
                try
                {
                    file >> sfm_data;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "解析JSON时出错: " << e.what();
                    LOG_ERROR_EN << "Error parsing JSON: " << e.what();
                    file.close();
                    return false;
                }

                file.close();

                // Get root path | 获取根路径
                std::string root_path = "";
                if (sfm_data.contains("root_path"))
                {
                    root_path = sfm_data["root_path"].get<std::string>();
                }

                // Parse views - OpenMVG's views are an array | 解析视图 - OpenMVG的视图是一个数组
                if (sfm_data.contains("views") && sfm_data["views"].is_array())
                {
                    for (const auto &view_entry : sfm_data["views"])
                    {
                        // Read view ID | 读取视图ID
                        types::IndexT view_id = view_entry["key"].get<types::IndexT>();

                        // Get view value object | 获取视图值对象
                        const auto &view_value = view_entry["value"];
                        if (!view_value.contains("ptr_wrapper") ||
                            !view_value["ptr_wrapper"].contains("data"))
                        {
                            continue; // Skip invalid views | 跳过无效视图
                        }

                        const auto &view_data = view_value["ptr_wrapper"]["data"];

                        // Read image file name | 读取图像文件名
                        std::string local_path = "";
                        if (view_data.contains("local_path"))
                        {
                            local_path = view_data["local_path"].get<std::string>();
                        }

                        std::string filename = "";
                        if (view_data.contains("filename"))
                        {
                            filename = view_data["filename"].get<std::string>();
                        }

                        if (filename.empty())
                        {
                            continue; // Skip views without filename | 跳过没有文件名的视图
                        }

                        // Build complete image path | 构建完整的图像路径
                        std::string img_path;
                        if (!local_path.empty())
                        {
                            img_path = std::filesystem::path(local_path) / filename;
                        }
                        else if (!root_path.empty())
                        {
                            img_path = std::filesystem::path(root_path) / filename;
                        }
                        else
                        {
                            img_path = filename;
                        }

                        // Add to result set | 添加到结果集
                        image_paths.emplace_back(view_id, img_path);
                    }
                }

                return !image_paths.empty();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "加载SfM数据时出错: " << e.what();
                LOG_ERROR_EN << "Error loading SfM data: " << e.what();
                return false;
            }
        }

        bool OpenMVGFileConverter::ToDataGlobalPoses(
            const std::string &sfm_data_file,
            Interface::DataPtr &global_poses_data)
        {
            try
            {
                // Get GlobalPoses pointer | 获取GlobalPoses指针
                auto global_poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
                if (!global_poses_ptr)
                {
                    LOG_ERROR_ZH << "获取全局位姿指针失败";
                    LOG_ERROR_EN << "Failed to get global poses pointer";
                    return false;
                }

                // Load global poses from SfM data | 加载SfM数据中的全局位姿
                if (!LoadSfMData(sfm_data_file, *global_poses_ptr))
                {
                    LOG_ERROR_ZH << "从文件加载全局位姿失败: " << sfm_data_file;
                    LOG_ERROR_EN << "Failed to load global poses from: " << sfm_data_file;
                    return false;
                }

                LOG_INFO_ZH << "成功转换全局位姿，共 " << global_poses_ptr->Size() << " 个位姿";
                LOG_INFO_EN << "Successfully converted global poses with " << global_poses_ptr->Size() << " poses";

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "转换全局位姿时出错: " << e.what();
                LOG_ERROR_EN << "Error converting global poses: " << e.what();
                return false;
            }
        }

        bool OpenMVGFileConverter::ToDataImages(
            const std::string &sfm_data_file,
            const std::string &images_base_dir,
            Interface::DataPtr &images_data)
        {
            try
            {
                // Get types::ImagePaths pointer | 获取types::ImagePaths指针
                auto image_paths_ptr = GetDataPtr<types::ImagePaths>(images_data);
                if (!image_paths_ptr)
                {
                    LOG_ERROR_ZH << "获取图像路径指针失败";
                    LOG_ERROR_EN << "Failed to get image paths pointer";
                    return false;
                }

                // Load SfM data | 加载SfM数据
                std::vector<std::pair<types::IndexT, std::string>> view_paths;
                if (!LoadSfMData(sfm_data_file, view_paths, true))
                {
                    LOG_ERROR_ZH << "从文件加载SfM数据失败: " << sfm_data_file;
                    LOG_ERROR_EN << "Failed to load SfM data from: " << sfm_data_file;
                    return false;
                }

                // Clear existing data | 清除现有数据
                image_paths_ptr->clear();

                // Determine required size | 确定需要的大小
                types::IndexT max_id = 0;
                for (const auto &path_pair : view_paths)
                {
                    max_id = std::max(max_id, path_pair.first);
                }

                // Resize container | 调整容器大小
                image_paths_ptr->resize(max_id + 1, std::make_pair("", false));

                // Fill image paths | 填充图像路径
                for (const auto &path_pair : view_paths)
                {
                    types::IndexT view_id = path_pair.first;
                    std::string image_path = path_pair.second;

                    // Convert relative path to absolute path if needed
                    // 如果路径是相对路径，需要转换为绝对路径
                    if (!std::filesystem::path(image_path).is_absolute() && !images_base_dir.empty())
                    {
                        // Only add images_base_dir when path is not absolute and images_base_dir is not empty
                        // 仅当路径不是绝对路径且images_base_dir不为空时，才添加images_base_dir
                        // Handle image paths relative to root directory
                        // 处理图像相对于根路径的情况
                        std::filesystem::path full_path;
                        if (image_path.find("./") == 0)
                        {
                            // If path starts with ./, replace with images_base_dir
                            // 如果路径以./开头，替换为images_base_dir
                            full_path = std::filesystem::path(images_base_dir) / image_path.substr(2);
                        }
                        else
                        {
                            // Otherwise, concatenate directly | 否则直接拼接
                            full_path = std::filesystem::path(images_base_dir) / image_path;
                        }

                        image_path = full_path.string();
                    }

                    // Set path and valid flag | 设置路径和有效标志
                    (*image_paths_ptr)[view_id] = std::make_pair(image_path, true);
                }

                LOG_INFO_ZH << "成功创建图像数据，共 " << view_paths.size() << " 张图像";
                LOG_INFO_EN << "Successfully created image data with " << view_paths.size() << " images";

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "创建图像数据时出错: " << e.what();
                LOG_ERROR_EN << "Error creating images data: " << e.what();
                return false;
            }
        }

        bool OpenMVGFileConverter::ToDataFeatures(
            const std::string &sfm_data_file,
            const std::string &features_dir,
            const std::string &images_base_dir,
            Interface::DataPtr &features_data)
        {
            try
            {
                // Get features info pointer | 获取特征信息指针
                auto features_info = GetDataPtr<types::FeaturesInfo>(features_data);
                if (!features_info)
                {
                    LOG_ERROR_ZH << "获取特征信息指针失败";
                    LOG_ERROR_EN << "Failed to get features info pointer";
                    return false;
                }

                // Load SfM data to get view information | 加载SfM数据以获取视图信息
                std::vector<std::pair<types::IndexT, std::string>> view_paths;
                if (!LoadSfMData(sfm_data_file, view_paths, true))
                {
                    LOG_ERROR_ZH << "从文件加载SfM数据失败: " << sfm_data_file;
                    LOG_ERROR_EN << "Failed to load SfM data from: " << sfm_data_file;
                    return false;
                }

                // Clear existing data | 清除现有数据
                features_info->clear();

                // Pre-allocate space | 预分配空间
                types::IndexT max_id = 0;
                for (const auto &path_pair : view_paths)
                {
                    max_id = std::max(max_id, path_pair.first);
                }
                features_info->resize(max_id + 1);

                // Process features for each view | 处理每个视图的特征
                for (const auto &path_pair : view_paths)
                {
                    types::IndexT view_id = path_pair.first;
                    std::string image_path = path_pair.second;

                    // Construct feature file path | 构建特征文件路径
                    std::string img_filename = std::filesystem::path(image_path).stem().string();
                    std::string feat_file = std::filesystem::path(features_dir) / (img_filename + ".feat");

                    // Check if feature file exists | 检查特征文件是否存在
                    if (!std::filesystem::exists(feat_file))
                    {
                        // Try using view ID as feature filename
                        // 尝试使用视图ID作为特征文件名
                        feat_file = std::filesystem::path(features_dir) / (std::to_string(view_id) + ".feat");
                        if (!std::filesystem::exists(feat_file))
                        {
                            LOG_DEBUG_ZH << "特征文件未找到: " << img_filename << " 或 " << view_id;
                            LOG_DEBUG_EN << "Features file not found for: " << img_filename << " or " << view_id;
                            continue;
                        }
                    }

                    // Create image feature info | 创建图像特征信息
                    types::ImageFeatureInfo image_feature;
                    image_feature.SetImagePath(image_path);

                    // Load features | 加载特征
                    if (!LoadFeatures(feat_file, image_feature))
                    {
                        LOG_DEBUG_ZH << "无法从文件加载特征: " << feat_file;
                        LOG_DEBUG_EN << "Cannot load features from: " << feat_file;
                        continue;
                    }

                    // Add feature info to the appropriate view ID position
                    // 将特征信息添加到适当的视图ID位置
                    // 使用原地构造避免指针赋值问题 | Use in-place construction to avoid pointer assignment issues
                    if (view_id < features_info->size()) {
                        *(*features_info)[view_id] = std::move(image_feature);
                    }
                }

                // Check if at least one view has features | 检查是否至少有一个视图有特征
                bool has_features = false;
                for (const auto &img_feature : *features_info)
                {
                    if (img_feature.GetNumFeatures() > 0)
                    {
                        has_features = true;
                        break;
                    }
                }

                if (!has_features)
                {
                    LOG_ERROR_ZH << "未找到任何视图的特征";
                    LOG_ERROR_EN << "No features found for any view";
                    return false;
                }

                LOG_INFO_ZH << "成功转换特征，共 " << features_info->size() << " 张图像";
                LOG_INFO_EN << "Successfully converted features for " << features_info->size() << " images";

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "转换特征时出错: " << e.what();
                LOG_ERROR_EN << "Error converting features: " << e.what();
                return false;
            }
        }

        bool OpenMVGFileConverter::ToDataMatches(
            const std::string &matches_file,
            Interface::DataPtr &matches_data)
        {
            try
            {
                // Get matches info pointer | 获取匹配信息指针
                auto matches_ptr = GetDataPtr<types::Matches>(matches_data);
                if (!matches_ptr)
                {
                    LOG_ERROR_ZH << "获取匹配指针失败";
                    LOG_ERROR_EN << "Failed to get matches pointer";
                    return false;
                }

                // Load matches | 加载匹配
                if (!LoadMatches(matches_file, *matches_ptr))
                {
                    LOG_ERROR_ZH << "从文件加载匹配失败: " << matches_file;
                    LOG_ERROR_EN << "Failed to load matches from: " << matches_file;
                    return false;
                }

                LOG_INFO_ZH << "成功转换匹配，共 " << matches_ptr->size() << " 个图像对";
                LOG_INFO_EN << "Successfully converted " << matches_ptr->size() << " image pairs with matches";

                return !matches_ptr->empty();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "转换匹配时出错: " << e.what();
                LOG_ERROR_EN << "Error converting matches: " << e.what();
                return false;
            }
        }

        // Static helper function: Load global poses from JSON file
        // 静态辅助函数：从JSON文件加载全局位姿
        static bool LoadSfMDataFromJSON(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses)
        {
            std::ifstream file(sfm_data_file);
            if (!file.is_open())
            {
                LOG_ERROR_ZH << "无法打开SfM数据文件: " << sfm_data_file;
                LOG_ERROR_EN << "Cannot open SfM data file: " << sfm_data_file;
                return false;
            }

            json sfm_data;
            try
            {
                file >> sfm_data;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "解析JSON时出错: " << e.what();
                LOG_ERROR_EN << "Error parsing JSON: " << e.what();
                file.close();
                return false;
            }
            file.close();

            // Parse extrinsics (poses) | 解析extrinsics（位姿）
            if (!sfm_data.contains("extrinsics") || !sfm_data["extrinsics"].is_array())
            {
                LOG_ERROR_ZH << "SfM数据中未找到extrinsics";
                LOG_ERROR_EN << "No extrinsics found in SfM data";
                return false;
            }

            // Collect all pose IDs to determine maximum ID
            // 收集所有位姿ID以确定最大ID
            types::IndexT max_pose_id = 0;
            std::map<types::IndexT, TempPose3> poses_map;

            for (const auto &pose_entry : sfm_data["extrinsics"])
            {
                types::IndexT pose_id = pose_entry["key"].get<types::IndexT>();
                max_pose_id = std::max(max_pose_id, pose_id);

                const auto &pose_value = pose_entry["value"];
                if (!pose_value.contains("rotation") || !pose_value.contains("center"))
                {
                    continue; // Skip invalid poses | 跳过无效位姿
                }

                TempPose3 temp_pose;
                temp_pose.rotation = pose_value["rotation"].get<std::vector<std::vector<double>>>();
                temp_pose.center = pose_value["center"].get<std::vector<double>>();

                poses_map[pose_id] = temp_pose;
            }

            if (poses_map.empty())
            {
                LOG_ERROR_ZH << "SfM数据中未找到有效位姿";
                LOG_ERROR_EN << "No valid poses found in SfM data";
                return false;
            }

            // Initialize GlobalPoses | 初始化GlobalPoses
            global_poses.Init(max_pose_id + 1);
            global_poses.SetPoseFormat(types::PoseFormat::RwTw); // OpenMVG uses RwTw format | OpenMVG使用RwTw格式

            // Convert and set poses | 转换并设置位姿
            for (const auto &pose_pair : poses_map)
            {
                types::IndexT pose_id = pose_pair.first;
                const TempPose3 &temp_pose = pose_pair.second;

                types::Matrix3d R;
                types::Vector3d t;
                temp_pose.ToPoSDK(R, t);

                global_poses.SetRotation(pose_id, R);
                global_poses.SetTranslation(pose_id, t);
                global_poses.AddEstimatedView(pose_id);
            }

            return true;
        }

        // Helper function: Load global poses from binary file
        // 辅助函数：从二进制文件加载全局位姿
        static bool LoadSfMDataFromBinary(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses)
        {
            // For binary files, direct parsing encounters compatibility issues due to
            // OpenMVG's use of complex polymorphic object serialization.
            // It is recommended that users convert to JSON format.
            // 对于二进制文件，由于OpenMVG使用了复杂的多态对象序列化，
            // 直接解析会遇到兼容性问题。建议用户转换为JSON格式。
            LOG_ERROR_ZH << "二进制SfM数据格式解析复杂，由于多态序列化问题";
            LOG_ERROR_ZH << "请转换为JSON格式以获得更好的兼容性:";
            LOG_ERROR_ZH << "  openMVG_main_ConvertSfM_DataFormat -i " << sfm_data_file << " -o output.json";
            LOG_ERROR_ZH << "或者，您可以尝试使用SfM数据的JSON版本";
            LOG_ERROR_EN << "Binary SfM data format parsing is complex due to polymorphic serialization.";
            LOG_ERROR_EN << "Please convert to JSON format for better compatibility:";
            LOG_ERROR_EN << "  openMVG_main_ConvertSfM_DataFormat -i " << sfm_data_file << " -o output.json";
            LOG_ERROR_EN << "Alternatively, you can try using the JSON version of your SfM data.";

            // Try simple binary parsing, but return false if it fails
            // 尝试简单的二进制解析，但如果失败就返回false
            std::ifstream file(sfm_data_file, std::ios::binary);
            if (!file.is_open())
            {
                LOG_ERROR_ZH << "无法打开二进制SfM数据文件: " << sfm_data_file;
                LOG_ERROR_EN << "Cannot open binary SfM data file: " << sfm_data_file;
                return false;
            }

            try
            {
                cereal::PortableBinaryInputArchive archive(file);

                // Read version information | 读取版本信息
                std::string version;
                archive(cereal::make_nvp("sfm_data_version", version));

                // Read root_path | 读取root_path
                std::string root_path;
                archive(cereal::make_nvp("root_path", root_path));

                // Since the views section contains polymorphic objects, we cannot safely skip it
                // Return false directly and recommend users to use JSON format
                // 由于views部分包含多态对象，我们无法安全地跳过它
                // 直接返回false，建议用户使用JSON格式
                file.close();

                LOG_DEBUG_ZH << "检测到二进制格式 (版本: " << version << ")，但不支持解析";
                LOG_DEBUG_ZH << "请使用JSON格式";
                LOG_DEBUG_EN << "Binary format detected (version: " << version << "), but parsing is not supported.";
                LOG_DEBUG_EN << "Please use JSON format instead.";
                return false;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "读取二进制SfM数据头部时出错: " << e.what();
                LOG_ERROR_EN << "Error reading binary SfM data header: " << e.what();
                file.close();
                return false;
            }
        }

        // Helper function: Load global poses from XML file
        // 辅助函数：从XML文件加载全局位姿
        static bool LoadSfMDataFromXML(
            const std::string &sfm_data_file,
            types::GlobalPoses &global_poses)
        {
            // Note: OpenMVG's XML format also contains complex polymorphic object serialization
            // Currently not fully supported, JSON format is recommended
            // 注意：OpenMVG的XML格式也包含复杂的多态对象序列化
            // 目前暂不支持，建议使用JSON格式
            LOG_ERROR_ZH << "XML SfM数据格式尚未完全支持";
            LOG_ERROR_ZH << "请使用JSON格式 (.json) 的SfM数据文件";
            LOG_ERROR_ZH << "您可以使用OpenMVG工具将XML转换为JSON:";
            LOG_ERROR_ZH << "  openMVG_main_ConvertSfM_DataFormat -i input.xml -o output.json";
            LOG_ERROR_EN << "XML SfM data format is not fully supported yet.";
            LOG_ERROR_EN << "Please use JSON format (.json) for SfM data files.";
            LOG_ERROR_EN << "You can convert XML to JSON using OpenMVG tools:";
            LOG_ERROR_EN << "  openMVG_main_ConvertSfM_DataFormat -i input.xml -o output.json";
            return false;
        }

    } // namespace Converter
} // namespace PoSDK
