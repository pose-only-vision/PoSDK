#include "converter_colmap_file.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <tuple>
#include <vector>
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

// 添加必要的STL头文件 | Add necessary STL header files
#include <map>
#include <utility>

namespace PoSDK
{
    namespace Converter
    {
        namespace Colmap
        {
            bool LoadMatches(
                const std::string &matches_folder,
                types::Matches &matches,
                std::map<std::string, int> &file_name_to_id)
            {
                matches.clear();

                // 判断文件夹是否存在 | Check if folder exists
                if (std::filesystem::exists(matches_folder))
                {
                    // 遍历文件夹中的所有文件 | Iterate through all files in the folder
                    for (const auto &entry : std::filesystem::directory_iterator(matches_folder))
                    {
                        // 判断文件是否是txt文件 | Check if file is a txt file
                        if (entry.path().extension() == ".txt")
                        {
                            std::string filename = entry.path().filename().string();

                            // 解析文件名格式: matches_0000_0001.txt | Parse filename format: matches_0000_0001.txt
                            if (filename.find("matches_") == 0)
                            {
                                // 去掉 "matches_" 前缀和 ".txt" 后缀
                                // Remove "matches_" prefix and ".txt" suffix
                                std::string name_part = filename.substr(8); // 去掉 "matches_" | Remove "matches_"
                                size_t dot_pos = name_part.find_last_of('.');
                                if (dot_pos != std::string::npos)
                                {
                                    name_part = name_part.substr(0, dot_pos); // 去掉 ".txt" | Remove ".txt"
                                }

                                // 找到中间的下划线分割两个文件名 | Find middle underscore to split two filenames
                                size_t underscore_pos = name_part.find('_');
                                if (underscore_pos != std::string::npos)
                                {
                                    std::string first_name = name_part.substr(0, underscore_pos);
                                    std::string second_name = name_part.substr(underscore_pos + 1);

                                    // 通过file_name_to_id映射获取对应的ID | Get corresponding ID through file_name_to_id mapping
                                    auto it1 = file_name_to_id.find(first_name);
                                    auto it2 = file_name_to_id.find(second_name);

                                    if (it1 != file_name_to_id.end() && it2 != file_name_to_id.end())
                                    {
                                        types::IndexT I = it1->second;
                                        types::IndexT J = it2->second;

                                        // 读取匹配文件 | Read match file
                                        std::ifstream file(entry.path());
                                        if (!file.is_open())
                                        {
                                            LOG_ERROR_ZH << "[ColmapConverter] 无法打开匹配文件: " << entry.path();
                                            LOG_ERROR_EN << "[ColmapConverter] Cannot open matches file: " << entry.path();
                                            continue;
                                        }

                                        // 创建新的视图对 | Create new view pair
                                        types::ViewPair view_pair(I, J);
                                        types::IdMatches id_matches;

                                        types::IndexT number;
                                        if (file >> number)
                                        {
                                            id_matches.reserve(number);

                                            // 读取匹配对 | Read match pairs
                                            for (types::IndexT i = 0; i < number; ++i)
                                            {
                                                types::IndexT id_i, id_j;
                                                if (file >> id_i >> id_j)
                                                {
                                                    types::IdMatch match;
                                                    match.i = id_i;
                                                    match.j = id_j;
                                                    match.is_inlier = true; // 默认为内点 | Default as inlier
                                                    id_matches.push_back(match);
                                                }
                                            }

                                            // 只有在读取到匹配时才添加 | Only add when matches are read
                                            if (!id_matches.empty())
                                            {
                                                matches[view_pair] = std::move(id_matches);
                                            }
                                        }

                                        file.close();
                                    }
                                    else
                                    {
                                        LOG_ERROR_ZH << "[ColmapConverter] 无法找到ID映射: " << first_name << " 或 " << second_name;
                                        LOG_ERROR_EN << "[ColmapConverter] Cannot find ID mapping for: " << first_name
                                                     << " or " << second_name;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 匹配文件夹不存在: " << matches_folder;
                    LOG_ERROR_EN << "[ColmapConverter] Matches folder does not exist: " << matches_folder;
                    return false;
                }

                return !matches.empty();
            }

            /**
             * @brief 将Colmap的匹配文件转换为PoSDK的匹配数据
             * Convert Colmap match files to PoSDK match data
             * @param matches_folder 匹配文件夹路径 | Match folder path
             * @param matches_data 输出匹配数据 | Output match data
             * @return 是否成功 | Whether successful
             */
            bool ToDataMatches(
                const std::string &matches_folder,
                Interface::DataPtr &matches_data,
                std::map<std::string, int> &file_name_to_id)
            {
                try
                {
                    // 获取匹配信息指针 | Get matches pointer
                    auto matches_ptr = GetDataPtr<types::Matches>(matches_data);
                    if (!matches_ptr)
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 获取匹配指针失败";
                        LOG_ERROR_EN << "[ColmapConverter] Failed to get matches pointer";
                        return false;
                    }

                    // 加载匹配 | Load matches
                    if (!LoadMatches(matches_folder, *matches_ptr, file_name_to_id))
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 从路径加载匹配失败: " << matches_folder;
                        LOG_ERROR_EN << "[ColmapConverter] Failed to load matches from: " << matches_folder;
                        return false;
                    }

                    LOG_INFO_ZH << "[ColmapConverter] 成功转换 " << matches_ptr->size() << " 个图像对的匹配";
                    LOG_INFO_EN << "[ColmapConverter] Successfully converted " << matches_ptr->size() << " image pairs with matches";

                    return !matches_ptr->empty();
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 转换匹配时发生错误: " << e.what();
                    LOG_ERROR_EN << "[ColmapConverter] Error converting matches: " << e.what();
                    return false;
                }
            }

            /**
             * @brief 四元数转换为旋转矩阵
             * Convert quaternion to rotation matrix
             * @param qw, qx, qy, qz 四元数分量 | Quaternion components
             * @return 旋转矩阵 | Rotation matrix
             */
            Eigen::Matrix3d QuaternionToRotationMatrix(double qw, double qx, double qy, double qz)
            {
                // 归一化四元数 | Normalize quaternion
                double norm = std::sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
                qw /= norm;
                qx /= norm;
                qy /= norm;
                qz /= norm;

                // 转换为旋转矩阵 | Convert to rotation matrix
                Eigen::Matrix3d R;
                R(0, 0) = 1 - 2 * qy * qy - 2 * qz * qz;
                R(0, 1) = 2 * qx * qy - 2 * qz * qw;
                R(0, 2) = 2 * qx * qz + 2 * qy * qw;
                R(1, 0) = 2 * qx * qy + 2 * qz * qw;
                R(1, 1) = 1 - 2 * qx * qx - 2 * qz * qz;
                R(1, 2) = 2 * qy * qz - 2 * qx * qw;
                R(2, 0) = 2 * qx * qz - 2 * qy * qw;
                R(2, 1) = 2 * qy * qz + 2 * qx * qw;
                R(2, 2) = 1 - 2 * qx * qx - 2 * qy * qy;

                return R;
            }

            /**
             * @brief 将Colmap的images.txt文件转换为PoSDK的全局位姿数据
             * Convert Colmap images.txt files to PoSDK global pose data
             * @param global_poses_file images.txt文件路径 | images.txt file path
             * @param global_poses_data 输出全局位姿数据 | Output global pose data
             * @return 是否成功 | Whether successful
             */
            bool ToDataGlobalPoses(
                const std::string &global_poses_file,
                Interface::DataPtr &global_poses_data,
                std::map<std::string, int> &file_name_to_id)
            {
                try
                {
                    // 获取GlobalPoses指针 | Get GlobalPoses pointer
                    auto global_poses_ptr = GetDataPtr<types::GlobalPoses>(global_poses_data);
                    if (!global_poses_ptr)
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 获取全局位姿指针失败";
                        LOG_ERROR_EN << "[ColmapConverter] Failed to get global poses pointer";
                        return false;
                    }

                    // 读取images.txt文件 | Read images.txt file
                    std::ifstream file(global_poses_file);
                    if (!file.is_open())
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 无法打开images.txt文件: " << global_poses_file;
                        LOG_ERROR_EN << "[ColmapConverter] Cannot open images.txt file: " << global_poses_file;
                        return false;
                    }

                    LOG_DEBUG_ZH << "[ColmapConverter] 正在读取COLMAP images.txt文件...";
                    LOG_DEBUG_EN << "[ColmapConverter] Reading COLMAP images.txt file...";

                    std::string line;
                    int valid_poses_count = 0;

                    // 存储位姿数据用于导出 | Store pose data for export
                    std::vector<std::tuple<std::string, Eigen::Matrix3d, Eigen::Vector3d>> export_poses;

                    // 逐行读取文件 | Read file line by line
                    while (std::getline(file, line))
                    {
                        // 跳过空行和注释行 | Skip empty lines and comment lines
                        if (line.empty() || line[0] == '#')
                        {
                            continue;
                        }

                        // 解析图像信息行：IMAGE_ID QW QX QY QZ TX TY TZ CAMERA_ID NAME
                        // Parse image info line: IMAGE_ID QW QX QY QZ TX TY TZ CAMERA_ID NAME
                        std::istringstream iss(line);
                        int image_id;
                        double qw, qx, qy, qz, tx, ty, tz;
                        int camera_id;
                        std::string image_name;

                        if (!(iss >> image_id >> qw >> qx >> qy >> qz >> tx >> ty >> tz >> camera_id >> image_name))
                        {
                            LOG_ERROR_ZH << "[ColmapConverter] 解析图像信息行失败: " << line;
                            LOG_ERROR_EN << "[ColmapConverter] Failed to parse image info line: " << line;
                            continue;
                        }

                        // 读取下一行（POINTS2D数据），但我们不需要处理它，只是跳过
                        // Read next line (POINTS2D data), but we don't need to process it, just skip
                        std::string points2d_line;
                        if (!std::getline(file, points2d_line))
                        {
                            LOG_WARNING_ZH << "[ColmapConverter] 缺少POINTS2D行，图像ID: " << image_id;
                            LOG_WARNING_EN << "[ColmapConverter] Missing POINTS2D line for image ID: " << image_id;
                        }

                        // 去掉图像名的扩展名 | Remove image name extension
                        std::string filename_without_ext = image_name;
                        size_t dot_pos = image_name.find_last_of('.');
                        if (dot_pos != std::string::npos)
                        {
                            filename_without_ext = image_name.substr(0, dot_pos);
                        }

                        // 通过file_name_to_id映射获取pose id | Get pose id through file_name_to_id mapping
                        auto it = file_name_to_id.find(filename_without_ext);
                        if (it == file_name_to_id.end())
                        {
                            LOG_WARNING_ZH << "[ColmapConverter] 无法找到图像的位姿ID: " << filename_without_ext;
                            LOG_WARNING_EN << "[ColmapConverter] Cannot find pose ID for image: " << filename_without_ext;
                            continue;
                        }

                        types::IndexT pose_id = it->second;

                        // 将四元数转换为旋转矩阵 | Convert quaternion to rotation matrix
                        Eigen::Matrix3d rotation_matrix = QuaternionToRotationMatrix(qw, qx, qy, qz);

                        // 构建平移向量 | Build translation vector
                        Eigen::Vector3d translation_vector(tx, ty, tz);

                        // Colmap and PoSDK RwTc use the SAME format, no conversion needed!
                        // Colmap和PoSDK的RwTc使用相同的格式，不需要转换！
                        //
                        // Colmap format: xc = R * Xw + T
                        // PoSDK RwTc format: xc = Rw * Xw + tc
                        //
                        // Both R=Rw (world-to-camera rotation) and T=tc (translation in camera coordinates)
                        // R和Rw都是世界到相机的旋转，T和tc都是相机坐标系中的平移
                        //
                        // Therefore: just use the values directly, NO conversion!
                        // 因此：直接使用这些值，不需要转换！
                        // translation_vector remains as-is
                        // translation_vector保持原样

                        // 初始化GlobalPoses容器（如果还没有初始化）
                        // Initialize GlobalPoses container (if not already initialized)
                        if (valid_poses_count == 0)
                        {
                            // 预估容器大小，使用图像数量作为初始值
                            // Estimate container size, use number of images as initial value
                            int estimated_size = std::max(static_cast<int>(file_name_to_id.size()), 100);
                            global_poses_ptr->Init(estimated_size);
                        }

                        // 设置全局位姿 | Set global pose
                        global_poses_ptr->SetRotation(pose_id, rotation_matrix);
                        global_poses_ptr->SetTranslation(pose_id, translation_vector);

                        // 存储位姿数据用于导出 | Store pose data for export
                        // 注意：这里存储的是原始的COLMAP位姿（相机到世界），用于导出文件
                        // Note: Store original COLMAP pose (camera-to-world) for export
                        Eigen::Vector3d original_translation(tx, ty, tz);
                        // export_poses.emplace_back(image_name, rotation_matrix, original_translation);

                        valid_poses_count++;

                        LOG_DEBUG_ZH << "[ColmapConverter] 加载位姿: " << filename_without_ext
                                     << " (image_id: " << image_id << ", pose_id: " << pose_id << ")";
                        LOG_DEBUG_EN << "[ColmapConverter] Loaded pose for " << filename_without_ext
                                     << " (image_id: " << image_id << ", pose_id: " << pose_id << ")";
                    }

                    file.close();

                    // 导出global_poses.txt文件 | Export global_poses.txt file
                    if (!export_poses.empty())
                    {
                        // 获取输出目录（images.txt文件所在的目录）| Get output directory (directory containing images.txt)
                        std::string output_dir = std::filesystem::path(global_poses_file).parent_path().string();
                        std::string export_file_path = std::filesystem::path(output_dir) / "global_poses.txt";

                        try
                        {
                            std::ofstream export_file(export_file_path);
                            if (export_file.is_open())
                            {
                                // 第一行：相机数量 | First line: number of cameras
                                export_file << export_poses.size() << "\n";

                                // 每行：图像名 + 旋转矩阵（按列优先）+ 平移向量
                                // Each line: image name + rotation matrix (column-major) + translation vector
                                for (const auto &pose : export_poses)
                                {
                                    const std::string &name = std::get<0>(pose);
                                    const Eigen::Matrix3d &R = std::get<1>(pose);
                                    const Eigen::Vector3d &t = std::get<2>(pose);

                                    // 输出旋转矩阵按列优先顺序：R(0,0) R(1,0) R(2,0) R(0,1) R(1,1) R(2,1) R(0,2) R(1,2) R(2,2)
                                    // Output rotation matrix in column-major order
                                    export_file << name << " "
                                                << std::fixed << std::setprecision(8)
                                                << R(0, 0) << " " << R(1, 0) << " " << R(2, 0) << " "
                                                << R(0, 1) << " " << R(1, 1) << " " << R(2, 1) << " "
                                                << R(0, 2) << " " << R(1, 2) << " " << R(2, 2) << " "
                                                << t(0) << " " << t(1) << " " << t(2) << "\n";
                                }

                                export_file.close();

                                LOG_INFO_ZH << "[ColmapConverter] 成功导出global_poses.txt文件: " << export_file_path;
                                LOG_INFO_EN << "[ColmapConverter] Successfully exported global_poses.txt file: " << export_file_path;
                            }
                            else
                            {
                                LOG_ERROR_ZH << "[ColmapConverter] 无法创建global_poses.txt文件: " << export_file_path;
                                LOG_ERROR_EN << "[ColmapConverter] Cannot create global_poses.txt file: " << export_file_path;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            LOG_ERROR_ZH << "[ColmapConverter] 导出global_poses.txt文件时发生错误: " << e.what();
                            LOG_ERROR_EN << "[ColmapConverter] Error exporting global_poses.txt file: " << e.what();
                        }
                    }

                    // Set pose format to RwTc (same as Colmap format)
                    // 设置位姿格式为RwTc（与Colmap格式相同）
                    global_poses_ptr->SetPoseFormat(types::PoseFormat::RwTc);

                    // Resize container to actual number of valid poses
                    // 调整容器大小为实际的有效位姿数量
                    if (valid_poses_count < static_cast<int>(global_poses_ptr->Size()))
                    {
                        LOG_INFO_ZH << "[ColmapConverter] 调整容器大小: " << global_poses_ptr->Size() << " -> " << valid_poses_count;
                        LOG_INFO_EN << "[ColmapConverter] Resizing container: " << global_poses_ptr->Size() << " -> " << valid_poses_count;
                        // Note: GlobalPoses doesn't have a resize method, so we need to create a new container
                        // 注意：GlobalPoses没有resize方法，所以需要创建一个新容器
                        types::GlobalPoses trimmed_poses;
                        trimmed_poses.SetPoseFormat(types::PoseFormat::RwTc);
                        trimmed_poses.Init(valid_poses_count);
                        for (int i = 0; i < valid_poses_count; ++i)
                        {
                            trimmed_poses.SetRotation(i, global_poses_ptr->GetRotation(i));
                            trimmed_poses.SetTranslation(i, global_poses_ptr->GetTranslation(i));
                        }
                        *global_poses_ptr = std::move(trimmed_poses);
                    }

                    LOG_INFO_ZH << "[ColmapConverter] 成功转换 " << valid_poses_count << " 个全局位姿";
                    LOG_INFO_EN << "[ColmapConverter] Successfully converted " << valid_poses_count << " global poses";

                    return valid_poses_count > 0;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 转换全局位姿时发生错误: " << e.what();
                    LOG_ERROR_EN << "[ColmapConverter] Error converting global poses: " << e.what();
                    return false;
                }
            }

            bool SfMFileToIdMap(const std::string &sfm_data_file, std::map<std::string, int> &file_name_to_id)
            {
                // 读取SfM数据文件 | Read SfM data file
                std::ifstream file(sfm_data_file);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 无法打开SfM数据文件: " << sfm_data_file;
                    LOG_ERROR_EN << "[ColmapConverter] Cannot open SfM data file: " << sfm_data_file;
                    return false;
                }

                nlohmann::json sfm_data;
                try
                {
                    file >> sfm_data;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 解析JSON时发生错误: " << e.what();
                    LOG_ERROR_EN << "[ColmapConverter] Error parsing JSON: " << e.what();
                    file.close();
                    return false;
                }
                file.close();

                // 解析views | Parse views
                if (!sfm_data.contains("views") || !sfm_data["views"].is_array())
                {
                    LOG_ERROR_ZH << "[ColmapConverter] SfM数据中未找到views";
                    LOG_ERROR_EN << "[ColmapConverter] No views found in SfM data";
                    return false;
                }

                // 清空输出map | Clear output map
                file_name_to_id.clear();

                // 遍历views数组 | Iterate through views array
                const auto &views = sfm_data["views"];
                for (const auto &view_item : views)
                {
                    try
                    {
                        // 检查view结构是否完整 | Check if view structure is complete
                        if (!view_item.contains("value") ||
                            !view_item["value"].contains("ptr_wrapper") ||
                            !view_item["value"]["ptr_wrapper"].contains("data"))
                        {
                            LOG_ERROR_ZH << "[ColmapConverter] JSON中的view结构无效";
                            LOG_ERROR_EN << "[ColmapConverter] Invalid view structure in JSON";
                            continue;
                        }

                        const auto &data = view_item["value"]["ptr_wrapper"]["data"];

                        // 检查必要字段是否存在 | Check if necessary fields exist
                        if (!data.contains("filename") || !data.contains("id_pose"))
                        {
                            LOG_ERROR_ZH << "[ColmapConverter] view数据中缺少filename或id_pose";
                            LOG_ERROR_EN << "[ColmapConverter] Missing filename or id_pose in view data";
                            continue;
                        }

                        // 提取filename和id_pose | Extract filename and id_pose
                        std::string filename = data["filename"].get<std::string>();
                        int id_pose = data["id_pose"].get<int>();

                        // 去掉文件扩展名，只保留文件名部分 | Remove file extension, keep only filename part
                        std::string filename_without_ext = filename;
                        size_t dot_pos = filename.find_last_of('.');
                        if (dot_pos != std::string::npos)
                        {
                            filename_without_ext = filename.substr(0, dot_pos);
                        }

                        // 存储到map中 | Store in map
                        file_name_to_id[filename_without_ext] = id_pose;

                        LOG_DEBUG_ZH << "[ColmapConverter] 映射: " << filename_without_ext << " -> " << id_pose;
                        LOG_DEBUG_EN << "[ColmapConverter] Mapped: " << filename_without_ext << " -> " << id_pose;
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 处理view时发生错误: " << e.what();
                        LOG_ERROR_EN << "[ColmapConverter] Error processing view: " << e.what();
                        continue;
                    }
                }

                LOG_INFO_ZH << "[ColmapConverter] 成功映射 " << file_name_to_id.size() << " 个文件名到ID的对";
                LOG_INFO_EN << "[ColmapConverter] Successfully mapped " << file_name_to_id.size() << " filename-to-id pairs";

                return !file_name_to_id.empty();
            }

            template <typename T>
            void WriteBinary(std::ofstream &file, const T &data)
            {
                file.write(reinterpret_cast<const char *>(&data), sizeof(T));
            }

            void WriteString(std::ofstream &file, const std::string &str)
            {
                // COLMAP字符串格式: 标准的null-terminated字符串
                file.write(str.c_str(), str.length() + 1); // 包含'\0'终止符
            }

            void WriteCameras(const std::string &path, const std::vector<Camera> &cameras)
            {
                std::ofstream file(path, std::ios::binary);
                if (!file.is_open())
                {
                    std::cerr << "无法打开文件: " << path << std::endl;
                    return;
                }

                uint64_t num_cameras = cameras.size();
                WriteBinary(file, num_cameras);

                for (const auto &cam : cameras)
                {
                    WriteBinary(file, cam.camera_id);
                    WriteBinary(file, cam.model_id);
                    WriteBinary(file, cam.width);
                    WriteBinary(file, cam.height);

                    // 写入参数数组
                    for (double param : cam.params)
                    {
                        WriteBinary(file, param);
                    }
                }

                file.close();
                std::cout << "成功写入 " << num_cameras << " 个相机到 " << path << std::endl;
            }

            // 写入images.bin
            void WriteImages(const std::string &path, const std::vector<Image> &images)
            {
                std::ofstream file(path, std::ios::binary);
                if (!file.is_open())
                {
                    std::cerr << "无法打开文件: " << path << std::endl;
                    return;
                }

                uint64_t num_images = images.size();
                WriteBinary(file, num_images);

                for (const auto &img : images)
                {
                    WriteBinary(file, img.image_id);

                    // 写入四元数 (qw, qx, qy, qz)
                    WriteBinary(file, img.qw);
                    WriteBinary(file, img.qx);
                    WriteBinary(file, img.qy);
                    WriteBinary(file, img.qz);

                    // 写入平移 (tx, ty, tz)
                    WriteBinary(file, img.tx);
                    WriteBinary(file, img.ty);
                    WriteBinary(file, img.tz);

                    WriteBinary(file, img.camera_id);
                    WriteString(file, img.name);

                    // 写入2D观测点数量
                    uint64_t num_points2D = img.xys.size();
                    WriteBinary(file, num_points2D);

                    // 写入每个2D点和对应的3D点ID
                    for (size_t i = 0; i < num_points2D; ++i)
                    {
                        WriteBinary(file, img.xys[i].first);   // x
                        WriteBinary(file, img.xys[i].second);  // y
                        WriteBinary(file, img.point3D_ids[i]); // 3D点ID (-1表示无对应)
                    }
                }

                file.close();
                std::cout << "成功写入 " << num_images << " 张图像到 " << path << std::endl;
            }

            void WritePoints3D(const std::string &path, const std::vector<Point3D> &points)
            {
                std::ofstream file(path, std::ios::binary);
                if (!file.is_open())
                {
                    std::cerr << "无法打开文件: " << path << std::endl;
                    return;
                }

                uint64_t num_points = points.size();
                WriteBinary(file, num_points);

                for (const auto &pt : points)
                {
                    WriteBinary(file, pt.point3D_id);
                    WriteBinary(file, pt.x);
                    WriteBinary(file, pt.y);
                    WriteBinary(file, pt.z);
                    WriteBinary(file, pt.r);
                    WriteBinary(file, pt.g);
                    WriteBinary(file, pt.b);
                    WriteBinary(file, pt.error);

                    // 写入track长度
                    uint64_t track_length = pt.image_ids.size();
                    WriteBinary(file, track_length);

                    // 写入track (image_id, point2D_idx pairs)
                    for (size_t i = 0; i < track_length; ++i)
                    {
                        WriteBinary(file, pt.image_ids[i]);
                        WriteBinary(file, pt.point2D_idxs[i]);
                    }
                }

                file.close();
                std::cout << "成功写入 " << num_points << " 个3D点到 " << path << std::endl;
            }

            void NormalizeQuaternion(double &qw, double &qx, double &qy, double &qz)
            {
                double norm = std::sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
                qw /= norm;
                qx /= norm;
                qy /= norm;
                qz /= norm;
            }

            void RotationMatrixToQuaternion(const double R[9], double &qw, double &qx, double &qy, double &qz)
            {
                double trace = R[0] + R[4] + R[8];

                if (trace > 0)
                {
                    double s = 0.5 / std::sqrt(trace + 1.0);
                    qw = 0.25 / s;
                    qx = (R[7] - R[5]) * s;
                    qy = (R[2] - R[6]) * s;
                    qz = (R[3] - R[1]) * s;
                }
                else if (R[0] > R[4] && R[0] > R[8])
                {
                    double s = 2.0 * std::sqrt(1.0 + R[0] - R[4] - R[8]);
                    qw = (R[7] - R[5]) / s;
                    qx = 0.25 * s;
                    qy = (R[1] + R[3]) / s;
                    qz = (R[2] + R[6]) / s;
                }
                else if (R[4] > R[8])
                {
                    double s = 2.0 * std::sqrt(1.0 + R[4] - R[0] - R[8]);
                    qw = (R[2] - R[6]) / s;
                    qx = (R[1] + R[3]) / s;
                    qy = 0.25 * s;
                    qz = (R[5] + R[7]) / s;
                }
                else
                {
                    double s = 2.0 * std::sqrt(1.0 + R[8] - R[0] - R[4]);
                    qw = (R[3] - R[1]) / s;
                    qx = (R[2] + R[6]) / s;
                    qy = (R[5] + R[7]) / s;
                    qz = 0.25 * s;
                }

                NormalizeQuaternion(qw, qx, qy, qz);
            }

            void OutputPoSDK2Colmap(const std::string &output_path,
                                    const types::GlobalPosesPtr &global_poses,
                                    const types::CameraModelsPtr &camera_models,
                                    const types::FeaturesInfoPtr &features,
                                    const types::TracksPtr &tracks,
                                    const types::Points3dPtr &pts3d)
            {
                LOG_INFO_ZH << "[ColmapConverter] 开始转换PoSDK数据到Colmap数据...(相机数据、图像数据、3D点数据)";
                LOG_INFO_EN << "[ColmapConverter] Start converting PoSDK data to Colmap data...(camera data, image data, 3D point data)";
                LOG_INFO_ZH << "[ColmapConverter] 输出路径: " << output_path;
                LOG_INFO_EN << "[ColmapConverter] Output path: " << output_path;

                int num_poses = global_poses->Size();
                // ========== 1. 准备相机数据 ==========
                std::vector<Camera> cameras;
                for (int i = 0; i < num_poses; ++i)
                {
                    Camera cam;
                    cam.camera_id = i;
                    cam.model_id = 1; // PINHOLE模型

                    cam.width = camera_models->at(0).GetIntrinsics().GetWidth();
                    cam.height = camera_models->at(0).GetIntrinsics().GetHeight();
                    cam.params = {
                        camera_models->at(0).GetIntrinsics().GetFx(),
                        camera_models->at(0).GetIntrinsics().GetFy(),
                        camera_models->at(0).GetIntrinsics().GetCx(),
                        camera_models->at(0).GetIntrinsics().GetCy()}; // fx, fy, cx, cy
                    cameras.push_back(cam);
                }
                // ========== 2. 准备图像数据 ==========
                std::vector<Image> images;
                images.reserve(num_poses);

                for (int i = 0; i < num_poses; ++i)
                {
                    Image img;
                    img.name = std::filesystem::path(features->at(i).GetImagePath()).filename().string();
                    img.camera_id = i;
                    img.image_id = i;

                    Eigen::Matrix3d R = global_poses->GetRotation(i);
                    double R_array[9];
                    R_array[0] = R(0, 0);
                    R_array[1] = R(0, 1);
                    R_array[2] = R(0, 2);
                    R_array[3] = R(1, 0);
                    R_array[4] = R(1, 1);
                    R_array[5] = R(1, 2);
                    R_array[6] = R(2, 0);
                    R_array[7] = R(2, 1);
                    R_array[8] = R(2, 2);

                    RotationMatrixToQuaternion(R_array, img.qw, img.qx, img.qy, img.qz);

                    Eigen::Vector3d tc = global_poses->GetTranslation(i); // tc is translation in camera coordinates | tc是相机坐标系中的平移
                    img.tx = tc(0);
                    img.ty = tc(1);
                    img.tz = tc(2);

                    images.push_back(img);
                }
                for (int i = 0; i < tracks->GetTrackCount(); ++i)
                {
                    auto track = tracks->GetTrack(i);
                    int pts_id = i;
                    for (int j = 0; j < track.GetObservationCount(); ++j)
                    {
                        if (!track[j]->IsUsed())
                            continue;
                        ViewId view_id = track[j]->GetViewId();
                        auto coord = track[j]->GetOriginalCoord();
                        // std::cout << "GetOriginalCoord: " << coord.x() << ", " << coord.y() << std::endl;
                        // std::cout << "GetCoord: " << track[j]->GetCoord().x() << ", " << track[j]->GetCoord().y() << std::endl;
                        // std::cout << "ReprojectionError: " << track[j]->GetReprojectionError() << std::endl;
                        images[view_id].xys.push_back({coord.x(), coord.y()});
                        images[view_id].point3D_ids.push_back(pts_id);
                    }
                }

                // ========== 3. 准备3D点数据 ==========
                std::vector<Point3D> points3D;
                for (int i = 0; i < pts3d->cols() - 1; ++i)
                {
                    Point3D pt;
                    pt.point3D_id = i;
                    pt.x = (*pts3d)(0, i);
                    pt.y = (*pts3d)(1, i);
                    pt.z = (*pts3d)(2, i);

                    auto track = tracks->GetTrack(i);

                    double r = 0;
                    double g = 0;
                    double b = 0;

                    double error = 0;
                    int cnt = 0;
                    std::vector<uint32_t> image_ids;    // 观测到该点的图像ID
                    std::vector<uint32_t> point2D_idxs; // 在对应图像中的2D点索引
                    for (int j = 0; j < track.GetObservationCount(); ++j)
                    {
                        if (!track[j]->IsUsed())
                            continue;
                        cnt++;
                        r += track[j]->GetColorRGB().at(0);
                        g += track[j]->GetColorRGB().at(1);
                        b += track[j]->GetColorRGB().at(2);
                        error += track[j]->GetReprojectionError();
                        image_ids.push_back(track[j]->GetViewId());
                        point2D_idxs.push_back(track[j]->GetFeatureId());
                    }
                    r = r / cnt;
                    g = g / cnt;
                    b = b / cnt;
                    error = error / cnt;

                    pt.error = error;
                    pt.r = r;
                    pt.g = g;
                    pt.b = b;
                    pt.image_ids = image_ids;
                    pt.point2D_idxs = point2D_idxs; // 在各自图像中的索引
                    points3D.push_back(pt);
                }

                // ========== 4. 计算场景尺度并进行缩放 ==========
                LOG_INFO_ZH << "[ColmapConverter] 正在计算PoSDK场景尺度（基于最小相机间距离）...";
                LOG_INFO_EN << "[ColmapConverter] Computing PoSDK scene scale (based on minimum camera distance)...";

                // Camera center in world: C = -R^T * t
                std::vector<Eigen::Vector3d> camera_centers;
                camera_centers.reserve(images.size());

                for (size_t i = 0; i < images.size(); ++i)
                {
                    const auto &img = images[i];

                    // 从四元数重建旋转矩阵
                    Eigen::Quaterniond q(img.qw, img.qx, img.qy, img.qz);
                    Eigen::Matrix3d R = q.toRotationMatrix();

                    // 平移向量
                    Eigen::Vector3d t(img.tx, img.ty, img.tz);

                    // 相机中心 = -R^T * t
                    Eigen::Vector3d C = -R.transpose() * t;
                    camera_centers.push_back(C);
                }

                // 第一遍：计算所有相机两两之间的距离
                std::vector<double> all_distances;
                all_distances.reserve(camera_centers.size() * (camera_centers.size() - 1) / 2);

                for (size_t i = 0; i < camera_centers.size(); ++i)
                {
                    for (size_t j = i + 1; j < camera_centers.size(); ++j)
                    {
                        Eigen::Vector3d diff = camera_centers[i] - camera_centers[j];
                        double dist = diff.norm();
                        all_distances.push_back(dist);
                    }
                }

                // 计算最大距离，用于确定相对阈值
                double max_distance = 0.0;
                for (double dist : all_distances)
                {
                    if (dist > max_distance)
                    {
                        max_distance = dist;
                    }
                }

                // 如果最大距离为0或接近0，说明所有相机位置基本相同
                if (max_distance < 1e-10)
                {
                    LOG_WARNING_ZH << "[ColmapConverter] 所有相机位置基本相同，使用默认缩放因子1.0";
                    LOG_WARNING_EN << "[ColmapConverter] All cameras at same position, using default scale factor 1.0";
                    max_distance = 1.0; // 设置为1.0以避免除零
                }

                // 相对阈值：最大距离的1%
                // 小于此阈值的相机对被认为是纯旋转（位置基本相同）
                const double pure_rotation_ratio = 0.01; // 1%
                double pure_rotation_threshold = max_distance * pure_rotation_ratio;

                // 第二遍：应用阈值，找到最小的有效距离
                double min_distance = std::numeric_limits<double>::max();
                int valid_pairs = 0;
                int pure_rotation_pairs = 0;

                for (double dist : all_distances)
                {
                    // 如果距离太小（小于最大距离的1%），认为是纯旋转，跳过
                    if (dist < pure_rotation_threshold)
                    {
                        pure_rotation_pairs++;
                        continue;
                    }

                    valid_pairs++;
                    if (dist < min_distance)
                    {
                        min_distance = dist;
                    }
                }

                LOG_INFO_ZH << "[ColmapConverter] PoSDK场景统计:";
                LOG_INFO_EN << "[ColmapConverter] PoSDK scene statistics:";
                LOG_INFO_ZH << "  相机数量: " << camera_centers.size();
                LOG_INFO_EN << "  Number of cameras: " << camera_centers.size();
                LOG_INFO_ZH << "  相机对总数: " << all_distances.size();
                LOG_INFO_EN << "  Total camera pairs: " << all_distances.size();
                LOG_INFO_ZH << "  最大相机间距离: " << max_distance;
                LOG_INFO_EN << "  Maximum camera distance: " << max_distance;
                LOG_INFO_ZH << "  纯旋转判定阈值: " << pure_rotation_threshold
                            << " (" << (pure_rotation_ratio * 100) << "% of max)";
                LOG_INFO_EN << "  Pure rotation threshold: " << pure_rotation_threshold
                            << " (" << (pure_rotation_ratio * 100) << "% of max)";
                LOG_INFO_ZH << "  有效相机对数量: " << valid_pairs;
                LOG_INFO_EN << "  Valid camera pairs: " << valid_pairs;
                LOG_INFO_ZH << "  纯旋转对数量: " << pure_rotation_pairs;
                LOG_INFO_EN << "  Pure rotation pairs: " << pure_rotation_pairs;
                LOG_INFO_ZH << "  最小相机间距离: " << min_distance;
                LOG_INFO_EN << "  Minimum camera distance: " << min_distance;

                // 缩放因子 = 1.0 / 最小相机间距离
                // 这样缩放后，最接近的两个相机之间的距离将变成1.0
                double scale_factor = 1.0;
                if (valid_pairs > 0 && min_distance < std::numeric_limits<double>::max())
                {
                    scale_factor = 1.0 / min_distance;
                    LOG_INFO_ZH << "  缩放策略: 标准化最小距离为1.0";
                    LOG_INFO_EN << "  Scaling strategy: Normalize minimum distance to 1.0";
                }
                else
                {
                    LOG_WARNING_ZH << "[ColmapConverter] 没有找到有效的相机对（非纯旋转），使用默认缩放因子1.0";
                    LOG_WARNING_EN << "[ColmapConverter] No valid camera pairs found (non-pure-rotation), using default scale factor 1.0";
                }

                // 限制缩放因子在合理范围内（避免极端值）
                scale_factor = std::max(1e-6, std::min(1e6, scale_factor));

                LOG_INFO_ZH << "[ColmapConverter] 应用缩放因子: " << scale_factor;
                LOG_INFO_EN << "[ColmapConverter] Applying scale factor: " << scale_factor;

                // 缩放3D点
                for (auto &pt : points3D)
                {
                    pt.x *= scale_factor;
                    pt.y *= scale_factor;
                    pt.z *= scale_factor;
                }

                // 缩放相机平移向量（注意：旋转不需要缩放）
                for (auto &img : images)
                {
                    img.tx *= scale_factor;
                    img.ty *= scale_factor;
                    img.tz *= scale_factor;
                }

                // 重新计算缩放后的统计信息
                double scaled_min_distance = min_distance * scale_factor;

                LOG_INFO_ZH << "[ColmapConverter] 缩放后场景统计:";
                LOG_INFO_EN << "[ColmapConverter] Scaled scene statistics:";
                LOG_INFO_ZH << "  最小相机间距离: " << scaled_min_distance << " (目标值: 1.0)";
                LOG_INFO_EN << "  Minimum camera distance: " << scaled_min_distance << " (target: 1.0)";

                // ========== 5. 写入文件 ==========
                std::filesystem::path output_dir = output_path;

                // 创建输出目录 (需要手动创建或使用系统调用)
                std::filesystem::create_directories(output_dir);

                WriteCameras(output_dir / "cameras.bin", cameras);
                WriteImages(output_dir / "images.bin", images);
                WritePoints3D(output_dir / "points3D.bin", points3D);

                LOG_INFO_ZH << "[ColmapConverter] 二进制文件写入完成";
                LOG_INFO_EN << "[ColmapConverter] Binary files written successfully";

                // ========== 6. 转换bin到txt格式（可选）==========
                try
                {
                    LOG_INFO_ZH << "[ColmapConverter] 正在转换bin到txt格式...";
                    LOG_INFO_EN << "[ColmapConverter] Converting bin to txt format...";

                    // 使用colmap model_converter转换
                    std::string convert_cmd = "colmap model_converter --input_path " + output_dir.string() +
                                              " --output_path " + output_dir.string() + " --output_type TXT";

                    int convert_ret = system(convert_cmd.c_str());
                    if (convert_ret == 0)
                    {
                        LOG_INFO_ZH << "[ColmapConverter] txt文件转换成功";
                        LOG_INFO_EN << "[ColmapConverter] txt files converted successfully";
                    }
                    else
                    {
                        // 检查是否是命令未找到的错误（返回码127或32512）
                        if (convert_ret == 127 || convert_ret == 32512)
                        {
                            LOG_WARNING_ZH << "[ColmapConverter] colmap命令未找到，跳过txt文件转换（请确保colmap已安装并在PATH中）";
                            LOG_WARNING_EN << "[ColmapConverter] colmap command not found, skipping txt conversion (please ensure colmap is installed and in PATH)";
                        }
                        else
                        {
                            LOG_WARNING_ZH << "[ColmapConverter] txt文件转换失败，返回码: " << convert_ret;
                            LOG_WARNING_EN << "[ColmapConverter] txt conversion failed with return code: " << convert_ret;
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_WARNING_ZH << "[ColmapConverter] txt文件转换过程中发生异常: " << e.what() << "，跳过此步骤";
                    LOG_WARNING_EN << "[ColmapConverter] Exception occurred during txt conversion: " << e.what() << ", skipping this step";
                }
                catch (...)
                {
                    LOG_WARNING_ZH << "[ColmapConverter] txt文件转换过程中发生未知异常，跳过此步骤";
                    LOG_WARNING_EN << "[ColmapConverter] Unknown exception occurred during txt conversion, skipping this step";
                }

                // ========== 7. 生成PLY文件（可选）==========
                try
                {
                    LOG_INFO_ZH << "[ColmapConverter] 正在生成PLY文件...";
                    LOG_INFO_EN << "[ColmapConverter] Generating PLY file...";

                    // 生成包含点云和相机的PLY文件
                    std::filesystem::path ply_path = output_dir / "posdk2colmap_scene.ply";
                    LOG_INFO_ZH << "[ColmapConverter] PLY文件路径: " << ply_path;
                    LOG_INFO_EN << "[ColmapConverter] PLY file path: " << ply_path;

                    if (WritePointsAndCamerasToPLY(ply_path.string(), points3D, images))
                    {
                        LOG_INFO_ZH << "[ColmapConverter] PLY文件生成成功";
                        LOG_INFO_EN << "[ColmapConverter] PLY file generated successfully";
                    }
                    else
                    {
                        LOG_WARNING_ZH << "[ColmapConverter] PLY文件生成失败，跳过此步骤";
                        LOG_WARNING_EN << "[ColmapConverter] PLY file generation failed, skipping this step";
                    }

                    // 生成只包含点云的PLY文件（便于在Blender等软件中查看）
                    std::filesystem::path ply_points_only_path = output_dir / "posdk2colmap_points_only.ply";
                    LOG_INFO_ZH << "[ColmapConverter] 正在生成点云PLY文件（仅点云）...";
                    LOG_INFO_EN << "[ColmapConverter] Generating point cloud PLY file (points only)...";

                    if (WritePointsOnlyToPLY(ply_points_only_path.string(), points3D))
                    {
                        LOG_INFO_ZH << "[ColmapConverter] 点云PLY文件生成成功: " << ply_points_only_path;
                        LOG_INFO_EN << "[ColmapConverter] Point cloud PLY file generated successfully: " << ply_points_only_path;
                    }
                    else
                    {
                        LOG_WARNING_ZH << "[ColmapConverter] 点云PLY文件生成失败，跳过此步骤";
                        LOG_WARNING_EN << "[ColmapConverter] Point cloud PLY file generation failed, skipping this step";
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_WARNING_ZH << "[ColmapConverter] PLY文件生成过程中发生异常: " << e.what() << "，跳过此步骤";
                    LOG_WARNING_EN << "[ColmapConverter] Exception occurred during PLY generation: " << e.what() << ", skipping this step";
                }
                catch (...)
                {
                    LOG_WARNING_ZH << "[ColmapConverter] PLY文件生成过程中发生未知异常，跳过此步骤";
                    LOG_WARNING_EN << "[ColmapConverter] Unknown exception occurred during PLY generation, skipping this step";
                }

                return;
            }
            bool ReadCamerasTxt(const std::string &cameras_txt_path, std::vector<Camera> &cameras)
            {
                cameras.clear();
                std::ifstream file(cameras_txt_path);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 无法打开cameras.txt文件: " << cameras_txt_path;
                    LOG_ERROR_EN << "[ColmapConverter] Cannot open cameras.txt file: " << cameras_txt_path;
                    return false;
                }

                std::string line;
                while (std::getline(file, line))
                {
                    // 跳过空行和注释行 | Skip empty lines and comments
                    if (line.empty() || line[0] == '#')
                        continue;

                    std::istringstream iss(line);
                    Camera cam;
                    iss >> cam.camera_id >> cam.model_id >> cam.width >> cam.height;

                    // 读取相机参数 | Read camera parameters
                    double param;
                    while (iss >> param)
                    {
                        cam.params.push_back(param);
                    }

                    cameras.push_back(cam);
                }

                file.close();
                LOG_DEBUG_ZH << "[ColmapConverter] 读取了 " << cameras.size() << " 个相机";
                LOG_DEBUG_EN << "[ColmapConverter] Read " << cameras.size() << " cameras";
                return !cameras.empty();
            }

            bool ReadImagesTxt(const std::string &images_txt_path, std::vector<Image> &images)
            {
                images.clear();
                std::ifstream file(images_txt_path);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 无法打开images.txt文件: " << images_txt_path;
                    LOG_ERROR_EN << "[ColmapConverter] Cannot open images.txt file: " << images_txt_path;
                    return false;
                }

                std::string line;
                while (std::getline(file, line))
                {
                    // 跳过空行和注释行 | Skip empty lines and comments
                    if (line.empty() || line[0] == '#')
                        continue;

                    // 解析图像信息行：IMAGE_ID QW QX QY QZ TX TY TZ CAMERA_ID NAME
                    // Parse image info line: IMAGE_ID QW QX QY QZ TX TY TZ CAMERA_ID NAME
                    std::istringstream iss(line);
                    Image img;

                    if (!(iss >> img.image_id >> img.qw >> img.qx >> img.qy >> img.qz >> img.tx >> img.ty >> img.tz >> img.camera_id >> img.name))
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 解析图像信息行失败: " << line;
                        LOG_ERROR_EN << "[ColmapConverter] Failed to parse image info line: " << line;
                        continue;
                    }

                    // 读取下一行（POINTS2D数据）| Read next line (POINTS2D data)
                    std::string points2d_line;
                    if (!std::getline(file, points2d_line))
                    {
                        LOG_WARNING_ZH << "[ColmapConverter] 缺少POINTS2D行，图像ID: " << img.image_id;
                        LOG_WARNING_EN << "[ColmapConverter] Missing POINTS2D line for image ID: " << img.image_id;
                    }
                    else
                    {
                        // 解析POINTS2D: x y point3D_id ...
                        std::istringstream points_iss(points2d_line);
                        double x, y;
                        uint64_t point3D_id;
                        while (points_iss >> x >> y >> point3D_id)
                        {
                            img.xys.push_back({x, y});
                            img.point3D_ids.push_back(point3D_id);
                        }
                    }

                    images.push_back(img);
                }

                file.close();
                LOG_DEBUG_ZH << "[ColmapConverter] 读取了 " << images.size() << " 张图像";
                LOG_DEBUG_EN << "[ColmapConverter] Read " << images.size() << " images";
                return !images.empty();
            }

            bool ReadPoints3DTxt(const std::string &points3D_txt_path, std::vector<Point3D> &points)
            {
                points.clear();
                std::ifstream file(points3D_txt_path);
                if (!file.is_open())
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 无法打开points3D.txt文件: " << points3D_txt_path;
                    LOG_ERROR_EN << "[ColmapConverter] Cannot open points3D.txt file: " << points3D_txt_path;
                    return false;
                }

                std::string line;
                while (std::getline(file, line))
                {
                    // 跳过空行和注释行 | Skip empty lines and comments
                    if (line.empty() || line[0] == '#')
                        continue;

                    // 解析3D点信息行：POINT3D_ID X Y Z R G B ERROR TRACK[] as (IMAGE_ID, POINT2D_IDX)
                    // Parse 3D point info line: POINT3D_ID X Y Z R G B ERROR TRACK[] as (IMAGE_ID, POINT2D_IDX)
                    std::istringstream iss(line);
                    Point3D pt;

                    int r, g, b;
                    if (!(iss >> pt.point3D_id >> pt.x >> pt.y >> pt.z >> r >> g >> b >> pt.error))
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 解析3D点信息行失败: " << line;
                        LOG_ERROR_EN << "[ColmapConverter] Failed to parse 3D point info line: " << line;
                        continue;
                    }

                    pt.r = static_cast<uint8_t>(r);
                    pt.g = static_cast<uint8_t>(g);
                    pt.b = static_cast<uint8_t>(b);

                    // 读取track数据 (IMAGE_ID, POINT2D_IDX) pairs
                    uint32_t image_id, point2D_idx;
                    while (iss >> image_id >> point2D_idx)
                    {
                        pt.image_ids.push_back(image_id);
                        pt.point2D_idxs.push_back(point2D_idx);
                    }

                    points.push_back(pt);
                }

                file.close();
                LOG_DEBUG_ZH << "[ColmapConverter] 读取了 " << points.size() << " 个3D点";
                LOG_DEBUG_EN << "[ColmapConverter] Read " << points.size() << " 3D points";
                return !points.empty();
            }

            bool WritePointsAndCamerasToPLY(const std::string &ply_path,
                                            const std::vector<Point3D> &points,
                                            const std::vector<Image> &images)
            {
                try
                {
                    std::ofstream ply_file(ply_path);
                    if (!ply_file.is_open())
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 无法创建PLY文件: " << ply_path;
                        LOG_ERROR_EN << "[ColmapConverter] Cannot create PLY file: " << ply_path;
                        return false;
                    }

                    // 计算相机可视化所需的顶点数
                    // 每个相机：1个中心点 + 5个视锥体顶点（4个角点+1个焦点）= 6个顶点
                    // 每个相机的边：8条线（4条从焦点到角点，4条角点连线）
                    size_t camera_vertices_per_cam = 6;
                    size_t total_vertices = points.size() + images.size() * camera_vertices_per_cam;
                    size_t total_edges = images.size() * 8;

                    // 写入PLY头部
                    ply_file << "ply\n";
                    ply_file << "format ascii 1.0\n";
                    ply_file << "comment Created by PoSDK ColmapConverter\n";
                    ply_file << "comment 3D points and camera frustums (red)\n";
                    ply_file << "element vertex " << total_vertices << "\n";
                    ply_file << "property float x\n";
                    ply_file << "property float y\n";
                    ply_file << "property float z\n";
                    ply_file << "property uchar red\n";
                    ply_file << "property uchar green\n";
                    ply_file << "property uchar blue\n";
                    ply_file << "element edge " << total_edges << "\n";
                    ply_file << "property int vertex1\n";
                    ply_file << "property int vertex2\n";
                    ply_file << "property uchar red\n";
                    ply_file << "property uchar green\n";
                    ply_file << "property uchar blue\n";
                    ply_file << "end_header\n";

                    // ========== 写入3D点（使用原始颜色）==========
                    for (const auto &pt : points)
                    {
                        ply_file << pt.x << " " << pt.y << " " << pt.z << " "
                                 << static_cast<int>(pt.r) << " "
                                 << static_cast<int>(pt.g) << " "
                                 << static_cast<int>(pt.b) << "\n";
                    }

                    // ========== 计算场景边界框，自适应相机大小 ==========
                    double min_x = std::numeric_limits<double>::max();
                    double min_y = std::numeric_limits<double>::max();
                    double min_z = std::numeric_limits<double>::max();
                    double max_x = std::numeric_limits<double>::lowest();
                    double max_y = std::numeric_limits<double>::lowest();
                    double max_z = std::numeric_limits<double>::lowest();

                    // 计算3D点的边界框
                    for (const auto &pt : points)
                    {
                        min_x = std::min(min_x, pt.x);
                        min_y = std::min(min_y, pt.y);
                        min_z = std::min(min_z, pt.z);
                        max_x = std::max(max_x, pt.x);
                        max_y = std::max(max_y, pt.y);
                        max_z = std::max(max_z, pt.z);
                    }

                    // 计算场景的对角线长度
                    double dx = max_x - min_x;
                    double dy = max_y - min_y;
                    double dz = max_z - min_z;
                    double scene_diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);

                    // 自适应设置相机大小：场景对角线的0.1-0.2%
                    double frustum_size = scene_diagonal * 0.0015; // 视锥体宽度（对角线的0.15%）
                    double focal_length = scene_diagonal * 0.0025; // 焦距长度（对角线的0.25%）

                    LOG_DEBUG_ZH << "[ColmapConverter] 场景边界框: [" << min_x << "," << max_x << "] x ["
                                 << min_y << "," << max_y << "] x [" << min_z << "," << max_z << "]";
                    LOG_DEBUG_EN << "[ColmapConverter] Scene bounding box: [" << min_x << "," << max_x << "] x ["
                                 << min_y << "," << max_y << "] x [" << min_z << "," << max_z << "]";
                    LOG_DEBUG_ZH << "[ColmapConverter] 场景对角线: " << scene_diagonal
                                 << ", 相机尺寸: " << frustum_size << ", 焦距: " << focal_length;
                    LOG_DEBUG_EN << "[ColmapConverter] Scene diagonal: " << scene_diagonal
                                 << ", Camera size: " << frustum_size << ", Focal length: " << focal_length;

                    // ========== 写入相机顶点（红色视锥体）==========
                    size_t vertex_offset = points.size();

                    LOG_DEBUG_ZH << "[ColmapConverter] 正在写入 " << images.size() << " 个相机视锥体...";
                    LOG_DEBUG_EN << "[ColmapConverter] Writing " << images.size() << " camera frustums...";

                    for (size_t cam_idx = 0; cam_idx < images.size(); ++cam_idx)
                    {
                        const auto &img = images[cam_idx];

                        // 从四元数和平移向量恢复相机位姿
                        // Colmap格式: xc = R * Xw + T
                        // 相机中心: C = -R^T * T

                        // 构建旋转矩阵
                        Eigen::Quaterniond q(img.qw, img.qx, img.qy, img.qz);
                        q.normalize();
                        Eigen::Matrix3d R = q.toRotationMatrix();
                        Eigen::Vector3d T(img.tx, img.ty, img.tz);

                        // 计算相机中心（世界坐标）
                        Eigen::Vector3d camera_center = -R.transpose() * T;

                        // 1. 写入相机中心点（红色）
                        ply_file << camera_center.x() << " "
                                 << camera_center.y() << " "
                                 << camera_center.z() << " "
                                 << "255 0 0\n"; // 红色

                        // 2. 生成相机视锥体的5个点（4个角点 + 1个焦点）
                        // 使用自适应的相机大小

                        // 相机坐标系中的视锥体角点（在焦平面上）
                        std::vector<Eigen::Vector3d> frustum_corners_cam = {
                            {-frustum_size, -frustum_size, focal_length}, // 左下
                            {frustum_size, -frustum_size, focal_length},  // 右下
                            {frustum_size, frustum_size, focal_length},   // 右上
                            {-frustum_size, frustum_size, focal_length},  // 左上
                            {0, 0, 0}                                     // 相机原点（焦点）
                        };

                        // 转换到世界坐标系并写入（红色）
                        for (const auto &corner_cam : frustum_corners_cam)
                        {
                            Eigen::Vector3d corner_world = R.transpose() * corner_cam + camera_center;
                            ply_file << corner_world.x() << " "
                                     << corner_world.y() << " "
                                     << corner_world.z() << " "
                                     << "255 0 0\n"; // 红色
                        }
                    }

                    // ========== 写入相机边（红色）==========
                    LOG_DEBUG_ZH << "[ColmapConverter] 正在写入相机边线...";
                    LOG_DEBUG_EN << "[ColmapConverter] Writing camera edges...";

                    for (size_t cam_idx = 0; cam_idx < images.size(); ++cam_idx)
                    {
                        size_t base_idx = vertex_offset + cam_idx * camera_vertices_per_cam;
                        size_t center_idx = base_idx;    // 相机中心
                        size_t focal_idx = base_idx + 5; // 焦点（第6个顶点）

                        // 4条从焦点到角点的边
                        for (int i = 0; i < 4; ++i)
                        {
                            ply_file << focal_idx << " " << (base_idx + 1 + i)
                                     << " 255 0 0\n"; // 红色
                        }

                        // 4条角点之间的边（形成矩形）
                        for (int i = 0; i < 4; ++i)
                        {
                            int next = (i + 1) % 4;
                            ply_file << (base_idx + 1 + i) << " " << (base_idx + 1 + next)
                                     << " 255 0 0\n"; // 红色
                        }
                    }

                    ply_file.close();

                    LOG_INFO_ZH << "[ColmapConverter] PLY文件包含 " << points.size()
                                << " 个3D点和 " << images.size() << " 个相机";
                    LOG_INFO_EN << "[ColmapConverter] PLY file contains " << points.size()
                                << " 3D points and " << images.size() << " cameras";

                    return true;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 生成PLY文件时发生错误: " << e.what();
                    LOG_ERROR_EN << "[ColmapConverter] Error generating PLY file: " << e.what();
                    return false;
                }
            }

            bool WritePointsOnlyToPLY(const std::string &ply_path,
                                      const std::vector<Point3D> &points)
            {
                try
                {
                    std::ofstream ply_file(ply_path);
                    if (!ply_file.is_open())
                    {
                        LOG_ERROR_ZH << "[ColmapConverter] 无法创建PLY文件: " << ply_path;
                        LOG_ERROR_EN << "[ColmapConverter] Cannot create PLY file: " << ply_path;
                        return false;
                    }

                    // 写入PLY头部（只包含点，不包含边）
                    ply_file << "ply\n";
                    ply_file << "format ascii 1.0\n";
                    ply_file << "comment Created by PoSDK ColmapConverter\n";
                    ply_file << "comment Point cloud only (no cameras)\n";
                    ply_file << "element vertex " << points.size() << "\n";
                    ply_file << "property float x\n";
                    ply_file << "property float y\n";
                    ply_file << "property float z\n";
                    ply_file << "property uchar red\n";
                    ply_file << "property uchar green\n";
                    ply_file << "property uchar blue\n";
                    ply_file << "end_header\n";

                    // 写入3D点（使用原始颜色）
                    for (const auto &pt : points)
                    {
                        ply_file << pt.x << " " << pt.y << " " << pt.z << " "
                                 << static_cast<int>(pt.r) << " "
                                 << static_cast<int>(pt.g) << " "
                                 << static_cast<int>(pt.b) << "\n";
                    }

                    ply_file.close();

                    LOG_INFO_ZH << "[ColmapConverter] 点云PLY文件包含 " << points.size() << " 个3D点";
                    LOG_INFO_EN << "[ColmapConverter] Point cloud PLY file contains " << points.size() << " 3D points";

                    return true;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR_ZH << "[ColmapConverter] 生成点云PLY文件时发生错误: " << e.what();
                    LOG_ERROR_EN << "[ColmapConverter] Error generating point cloud PLY file: " << e.what();
                    return false;
                }
            }

        }
    }
} // namespace PoSDK
