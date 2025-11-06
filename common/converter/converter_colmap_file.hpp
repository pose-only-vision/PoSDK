#ifndef _CONVERTER_COLMAP_FILE_
#define _CONVERTER_COLMAP_FILE_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <po_core.hpp>           // Include PoSDK core definitions | 包含PoSDK的核心定义
#include <po_core/po_logger.hpp> // Include bilingual logging | 包含双语日志

namespace PoSDK
{
    namespace Converter
    {
        namespace Colmap
        {

            // 数据结构定义
            struct Camera
            {
                uint32_t camera_id;
                int model_id; // 1=PINHOLE, 2=RADIAL, etc.
                uint64_t width;
                uint64_t height;
                std::vector<double> params; // fx, fy, cx, cy (PINHOLE) 或更多参数
            };

            struct Image
            {
                uint32_t image_id;
                double qw, qx, qy, qz; // 旋转四元数
                double tx, ty, tz;     // 平移
                uint32_t camera_id;
                std::string name;
                std::vector<std::pair<double, double>> xys; // 2D观测点
                std::vector<int64_t> point3D_ids;           // 对应的3D点ID
            };

            struct Point3D
            {
                uint64_t point3D_id;
                double x, y, z;
                uint8_t r, g, b;
                double error;
                std::vector<uint32_t> image_ids;    // 观测到该点的图像ID
                std::vector<uint32_t> point2D_idxs; // 在对应图像中的2D点索引
            };

            /**
             * @brief Convert Colmap SfM file to filename-ID mapping | 将Colmap的SfM文件转换为文件名-ID映射
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param file_name_to_id Output filename to ID mapping | 输出文件名到ID的映射
             * @return Success status | 是否成功
             */
            bool SfMFileToIdMap(const std::string &sfm_data_file, std::map<std::string, int> &file_name_to_id);

            /**
             * @brief Convert Colmap matches to PoSDK match data | 将Colmap的匹配文件转换为PoSDK的匹配数据
             * @param matches_folder Matches folder path | 匹配文件夹路径
             * @param matches_data Output match data | 输出匹配数据
             * @param file_name_to_id Filename to ID mapping | 文件名到ID的映射
             * @return Success status | 是否成功
             */
            bool ToDataMatches(
                const std::string &matches_folder,
                Interface::DataPtr &matches_data,
                std::map<std::string, int> &file_name_to_id);

            /**
             * @brief Convert Colmap SfM data to PoSDK global pose data
             * 将Colmap的SfM数据文件转换为PoSDK的全局位姿数据
             * @param global_poses_file Global poses file path | 全局位姿文件路径
             * @param global_poses_data Output global pose data | 输出全局位姿数据
             * @param file_name_to_id Filename to ID mapping | 文件名到ID的映射
             * @return Success status | 是否成功
             */
            bool ToDataGlobalPoses(
                const std::string &global_poses_file,
                Interface::DataPtr &global_poses_data,
                std::map<std::string, int> &file_name_to_id);

            /**
             * @brief Load matches from Colmap format | 从Colmap格式加载匹配数据
             * @param matches_folder Matches folder path | 匹配文件夹路径
             * @param matches Output matches container | 输出匹配容器
             * @param file_name_to_id Filename to ID mapping | 文件名到ID的映射
             * @return Success status | 是否成功
             */
            bool LoadMatches(
                const std::string &matches_folder,
                types::Matches &matches,
                std::map<std::string, int> &file_name_to_id);

            /**
             * @brief Write a value to a binary file stream | 将一个值写入二进制文件流
             * @tparam T Type of the data to be written | 要写入的数据类型
             * @param file Output binary file stream | 输出的二进制文件流
             * @param data Data to be written | 要写入的数据
             */
            template <typename T>
            void WriteBinary(std::ofstream &file, const T &data);

            /**
             * @brief Write a string to a binary file stream | 将一个字符串写入二进制文件流
             * @param file Output binary file stream | 输出的二进制文件流
             * @param str String to be written | 要写入的字符串
             */
            void WriteString(std::ofstream &file, const std::string &str);

            /**
             * @brief Write cameras data to a binary file | 将相机数据写入二进制文件
             * @param path Output file path | 输出的文件路径
             * @param cameras Camera data to be written | 要写入的相机数据
             */
            void WriteCameras(const std::string &path, const std::vector<Camera> &cameras);

            /**
             * @brief Write images data to a binary file | 将图像数据写入二进制文件
             * @param path Output file path | 输出的文件路径
             * @param images Image data to be written | 要写入的图像数据
             */
            void WriteImages(const std::string &path, const std::vector<Image> &images);

            /**
             * @brief Write points3D data to a binary file | 将3D点数据写入二进制文件
             * @param path Output file path | 输出的文件路径
             * @param points Points3D data to be written | 要写入的3D点数据
             */
            void WritePoints3D(const std::string &path, const std::vector<Point3D> &points);

            /**
             * @brief Normalize a quaternion | 归一化一个四元数
             * @param qw Quaternion w component | 四元数w分量
             * @param qx Quaternion x component | 四元数x分量
             * @param qy Quaternion y component | 四元数y分量
             * @param qz Quaternion z component | 四元数z分量
             */
            void NormalizeQuaternion(double &qw, double &qx, double &qy, double &qz);

            /**
             * @brief Convert a rotation matrix to a quaternion | 将旋转矩阵转换为四元数
             * @param R Rotation matrix | 旋转矩阵
             * @param qw Quaternion w component | 四元数w分量
             * @param qx Quaternion x component | 四元数x分量
             * @param qy Quaternion y component | 四元数y分量
             * @param qz Quaternion z component | 四元数z分量
             */
            void RotationMatrixToQuaternion(const double R[9], double &qw, double &qx, double &qy, double &qz);

            void OutputPoSDK2Colmap(const std::string &output_path,
                                    const types::GlobalPosesPtr &global_poses,
                                    const types::CameraModelsPtr &camera_models,
                                    const types::FeaturesInfoPtr &features,
                                    const types::TracksPtr &tracks,
                                    const types::Points3dPtr &pts3d);

            /**
             * @brief 从COLMAP txt文件读取相机数据 | Read camera data from COLMAP txt file
             * @param cameras_txt_path cameras.txt文件路径 | cameras.txt file path
             * @param cameras 输出相机数据 | Output camera data
             * @return 是否成功 | Success status
             */
            bool ReadCamerasTxt(const std::string &cameras_txt_path, std::vector<Camera> &cameras);

            /**
             * @brief 从COLMAP txt文件读取图像数据 | Read image data from COLMAP txt file
             * @param images_txt_path images.txt文件路径 | images.txt file path
             * @param images 输出图像数据 | Output image data
             * @return 是否成功 | Success status
             */
            bool ReadImagesTxt(const std::string &images_txt_path, std::vector<Image> &images);

            /**
             * @brief 从COLMAP txt文件读取3D点数据 | Read 3D point data from COLMAP txt file
             * @param points3D_txt_path points3D.txt文件路径 | points3D.txt file path
             * @param points 输出3D点数据 | Output 3D point data
             * @return 是否成功 | Success status
             */
            bool ReadPoints3DTxt(const std::string &points3D_txt_path, std::vector<Point3D> &points);

            bool WritePointsAndCamerasToPLY(const std::string &ply_path,
                                            const std::vector<Point3D> &points,
                                            const std::vector<Image> &images);

            /**
             * @brief 只导出点云到PLY文件（不包含相机和边数据）
             * Export only point cloud to PLY file (without cameras and edges)
             * @param ply_path PLY文件路径 | PLY file path
             * @param points 3D点数据 | 3D point data
             * @return 是否成功 | Success status
             */
            bool WritePointsOnlyToPLY(const std::string &ply_path,
                                      const std::vector<Point3D> &points);

        }

    } // namespace Converter
} // namespace PoSDK

#endif // _CONVERTER_COLMAP_FILE_
