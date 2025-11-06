/**
 * @file method_matches_visualizer.hpp
 * @brief 双视图匹配可视化插件
 * @details 用于绘制双视图匹配图像，支持区分内点和外点
 * @copyright Copyright (c) 2024 XXX公司
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <common/image_viewer/image_viewer.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <filesystem>
#include <po_core/po_logger.hpp>
namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;
    using namespace Converter;
    using namespace common;

    /**
     * @brief 双视图匹配可视化插件
     *
     * 该插件用于将匹配数据可视化为图像，支持：
     * - 绘制特征点匹配线
     * - 区分内点(绿色)和外点(红色)
     * - 输出为PNG格式图像
     * - 批量处理所有视图对
     */
    class MethodMatchesVisualizer : public MethodPresetProfiler
    {
    public:
        MethodMatchesVisualizer();
        ~MethodMatchesVisualizer() override = default;

        /**
         * @brief 运行匹配可视化
         * @return 处理完成的数据指针（通常返回输入的匹配数据）
         */
        DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        /**
         * @brief 绘制单个视图对的匹配图
         * @param view_pair 视图对(i,j)
         * @param matches 匹配数据
         * @param image_paths 图像路径列表
         * @param features_info 特征点信息
         * @param output_folder 输出文件夹路径
         * @return 是否成功绘制
         */
        bool DrawMatchesForViewPair(
            const ViewPair &view_pair,
            const IdMatches &matches,
            const ImagePaths &image_paths,
            const FeaturesInfo &features_info,
            const std::filesystem::path &output_folder);

        /**
         * @brief 将PoSDK的匹配数据转换为OpenCV格式
         * @param id_matches PoSDK匹配数据
         * @param cv_matches 输出OpenCV匹配数据
         * @param enhance_outliers 是否区分内外点
         * @return 转换的匹配数量
         */
        size_t ConvertIdMatchesToCVMatches(
            const IdMatches &id_matches,
            std::vector<cv::DMatch> &cv_matches,
            bool enhance_outliers = false);

        /**
         * @brief 绘制匹配图像
         * @param img1 第一个图像
         * @param img2 第二个图像
         * @param keypoints1 第一个图像的特征点
         * @param keypoints2 第二个图像的特征点
         * @param matches 匹配数据
         * @param inlier_matches 内点匹配（用于区分颜色）
         * @param output_path 输出文件路径
         * @param enhance_outliers 是否增强外点显示
         * @return 是否成功保存
         */
        bool DrawAndSaveMatches(
            const cv::Mat &img1,
            const cv::Mat &img2,
            const std::vector<cv::KeyPoint> &keypoints1,
            const std::vector<cv::KeyPoint> &keypoints2,
            const std::vector<cv::DMatch> &matches,
            const std::vector<bool> &inlier_flags,
            const std::filesystem::path &output_path,
            bool enhance_outliers = false);

        /**
         * @brief 从特征信息中提取OpenCV关键点
         * @param features_info 特征信息
         * @param view_id 视图ID
         * @param keypoints 输出关键点列表
         * @return 是否成功提取
         */
        bool ExtractKeyPointsFromFeatures(
            const FeaturesInfo &features_info,
            ViewId view_id,
            std::vector<cv::KeyPoint> &keypoints);

        /**
         * @brief 验证视图ID的有效性
         * @param view_i 第一个视图ID
         * @param view_j 第二个视图ID
         * @param max_views 最大视图数量
         * @return 是否有效
         */
        bool ValidateViewIds(ViewId view_i, ViewId view_j, size_t max_views);

        /**
         * @brief 创建输出文件夹
         * @param output_folder 输出文件夹路径
         * @return 是否成功创建
         */
        bool CreateOutputFolder(const std::filesystem::path &output_folder);

        /**
         * @brief 生成视图对的输出文件名
         * @param view_i 第一个视图ID
         * @param view_j 第二个视图ID
         * @return 文件名字符串
         */
        std::string GenerateOutputFileName(ViewId view_i, ViewId view_j);

        /**
         * @brief 统计匹配结果
         * @param matches 匹配数据
         * @param total_matches 总匹配数
         * @param inlier_count 内点数量
         * @param outlier_count 外点数量
         */
        void StatisticsMatches(
            const IdMatches &matches,
            size_t &total_matches,
            size_t &inlier_count,
            size_t &outlier_count);

        /**
         * @brief 打印处理进度信息
         * @param current 当前处理数量
         * @param total 总数量
         * @param view_pair 当前处理的视图对
         */
        void PrintProgress(size_t current, size_t total, const ViewPair &view_pair);

        /**
         * @brief 分布式选择匹配点，使匹配点在图像中分布更均匀
         * @param all_matches 所有匹配
         * @param id_matches 原始ID匹配数据
         * @param keypoints1 第一张图像的关键点
         * @param keypoints2 第二张图像的关键点
         * @param img1_size 第一张图像尺寸
         * @param img2_size 第二张图像尺寸
         * @param target_count 目标选择数量
         * @param selected_matches 输出的选择匹配
         * @param selected_inlier_flags 输出的选择匹配的内外点标志
         */
        void SelectDistributedMatches(
            const std::vector<cv::DMatch> &all_matches,
            const IdMatches &id_matches,
            const std::vector<cv::KeyPoint> &keypoints1,
            const std::vector<cv::KeyPoint> &keypoints2,
            const cv::Size &img1_size,
            const cv::Size &img2_size,
            size_t target_count,
            std::vector<cv::DMatch> &selected_matches,
            std::vector<bool> &selected_inlier_flags);

        /**
         * @brief 生成区分度高的颜色
         * @param index 当前索引
         * @param total_count 总数量
         * @param mode 颜色模式
         * @return BGR颜色值
         */
        cv::Scalar GenerateDistinctColor(size_t index, size_t total_count, const std::string &mode);

        /**
         * @brief 生成暖色系颜色（用于内点）
         */
        cv::Scalar GenerateWarmColor(size_t index, size_t total_count);

        /**
         * @brief 生成冷色系颜色（用于外点）
         */
        cv::Scalar GenerateCoolColor(size_t index, size_t total_count);

        /**
         * @brief HSV颜色空间转BGR
         */
        cv::Scalar HSVtoBGR(float hue, float saturation, float brightness);
    };

} // namespace PluginMethods