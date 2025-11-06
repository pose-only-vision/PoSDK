#pragma once

#include <string>
#include <vector>
#include <memory>
#include <po_core.hpp>

namespace PoSDK
{
    namespace Converter
    {

        /**
         * @brief OpenMVG file converter | OpenMVG文件转换器
         * @details Directly parse files output by OpenMVG tools without depending on OpenMVG library
         *          直接解析OpenMVG工具输出的文件，不依赖OpenMVG库
         */
        class OpenMVGFileConverter
        {
        public:
            /**
             * @brief Load feature points file directly into ImageFeatureInfo
             *        将特征点文件直接加载到ImageFeatureInfo中
             * @param features_file Feature file path | 特征文件路径
             * @param image_features Output image feature info | 输出图像特征信息
             * @return Success status | 是否成功
             */
            static bool LoadFeatures(
                const std::string &features_file,
                ImageFeatureInfo &image_features);

            /**
             * @brief Load matches file | 加载匹配文件
             * @param matches_file Matches file path | 匹配文件路径
             * @param matches Output match information | 输出匹配信息
             * @return Success status | 是否成功
             */
            static bool LoadMatches(
                const std::string &matches_file,
                types::Matches &matches);

            /**
             * @brief Load SfM data file | 加载SfM数据文件
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param image_paths Output image paths | 输出图像路径
             * @param views_only Whether to load only view information | 是否只加载视图信息
             * @return Success status | 是否成功
             */
            static bool LoadSfMData(
                const std::string &sfm_data_file,
                std::vector<std::pair<types::IndexT, std::string>> &image_paths,
                bool views_only = true);

            /**
             * @brief Convert OpenMVG SfM data file to PoSDK image data
             *        将OpenMVG的SfM数据文件转换为PoSDK的图像数据
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param images_base_dir Image base directory | 图像基础目录
             * @param images_data Output image data | 输出图像数据
             * @return Success status | 是否成功
             */
            static bool ToDataImages(
                const std::string &sfm_data_file,
                const std::string &images_base_dir,
                Interface::DataPtr &images_data);

            /**
             * @brief Convert OpenMVG feature files to PoSDK feature data
             *        将OpenMVG的特征文件转换为PoSDK的特征数据
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param features_dir Features directory | 特征目录
             * @param images_base_dir Image base directory | 图像基础目录
             * @param features_data Output feature data | 输出特征数据
             * @return Success status | 是否成功
             */
            static bool ToDataFeatures(
                const std::string &sfm_data_file,
                const std::string &features_dir,
                const std::string &images_base_dir,
                Interface::DataPtr &features_data);

            /**
             * @brief Convert OpenMVG matches file to PoSDK matches data
             *        将OpenMVG的匹配文件转换为PoSDK的匹配数据
             * @param matches_file Matches file path | 匹配文件路径
             * @param matches_data Output matches data | 输出匹配数据
             * @return Success status | 是否成功
             */
            static bool ToDataMatches(
                const std::string &matches_file,
                Interface::DataPtr &matches_data);

            /**
             * @brief Load global pose information from SfM data file
             *        加载SfM数据文件中的全局位姿信息
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param global_poses Output global pose information | 输出全局位姿信息
             * @return Success status | 是否成功
             */
            static bool LoadSfMData(
                const std::string &sfm_data_file,
                types::GlobalPoses &global_poses);

            /**
             * @brief Convert OpenMVG SfM data file to PoSDK global poses data
             *        将OpenMVG的SfM数据文件转换为PoSDK的全局位姿数据
             * @param sfm_data_file SfM data file path | SfM数据文件路径
             * @param global_poses_data Output global poses data | 输出全局位姿数据
             * @return Success status | 是否成功
             */
            static bool ToDataGlobalPoses(
                const std::string &sfm_data_file,
                Interface::DataPtr &global_poses_data);
        };

    } // namespace Converter
} // namespace PoSDK
