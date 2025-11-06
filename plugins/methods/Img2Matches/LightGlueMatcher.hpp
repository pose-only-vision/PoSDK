#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <vector>
#include <memory>
#include <string>
#include "Img2MatchesParams.hpp"
#include <po_core/po_logger.hpp>
namespace PluginMethods
{
    /**
     * @brief LightGlue深度学习匹配器
     *
     * 通过调用Python脚本使用LightGlue深度学习模型进行特征匹配
     * 支持多种特征类型：SuperPoint、DISK、SIFT、ALIKED、DoGHardNet
     */
    class LightGlueMatcher
    {
    public:
        /**
         * @brief 构造函数
         * @param params LightGlue参数配置
         */
        explicit LightGlueMatcher(const LightGlueParameters &params);

        /**
         * @brief 析构函数
         */
        ~LightGlueMatcher();

        /**
         * @brief 初始化LightGlue匹配器
         * @return 是否成功初始化
         */
        bool Initialize();

        /**
         * @brief 使用LightGlue进行特征匹配
         * @param img1 第一张图像
         * @param img2 第二张图像
         * @param keypoints1 第一张图像的特征点
         * @param keypoints2 第二张图像的特征点
         * @param descriptors1 第一张图像的描述子
         * @param descriptors2 第二张图像的描述子
         * @param matches 输出匹配结果
         * @return 是否匹配成功
         */
        bool MatchFeatures(
            const cv::Mat &img1, const cv::Mat &img2,
            const std::vector<cv::KeyPoint> &keypoints1,
            const std::vector<cv::KeyPoint> &keypoints2,
            const cv::Mat &descriptors1, const cv::Mat &descriptors2,
            std::vector<cv::DMatch> &matches);

        /**
         * @brief 静态方法：执行特征匹配
         * @param params LightGlue参数
         * @param img1 第一张图像
         * @param img2 第二张图像
         * @param keypoints1 第一张图像的特征点
         * @param keypoints2 第二张图像的特征点
         * @param descriptors1 第一张图像的描述子
         * @param descriptors2 第二张图像的描述子
         * @param matches 输出匹配结果
         * @return 是否匹配成功
         */
        static bool Match(
            const LightGlueParameters &params,
            const cv::Mat &img1, const cv::Mat &img2,
            const std::vector<cv::KeyPoint> &keypoints1,
            const std::vector<cv::KeyPoint> &keypoints2,
            const cv::Mat &descriptors1, const cv::Mat &descriptors2,
            std::vector<cv::DMatch> &matches);

        /**
         * @brief 获取匹配器名称
         */
        static std::string GetMatcherName() { return "LIGHTGLUE"; }

        /**
         * @brief 检查是否支持当前特征类型
         * @param feature_type 特征类型
         * @return 是否支持
         */
        static bool IsSupportedFeatureType(LightGlueFeatureType feature_type);

        /**
         * @brief 检查Python环境和依赖
         * @param python_exe Python可执行文件路径
         * @return 是否环境就绪
         */
        static bool CheckEnvironment(const std::string &python_exe = "python3");

        /**
         * @brief 尝试设置Python环境
         * @return 是否设置成功
         */
        static bool TrySetupEnvironment();

    private:
        LightGlueParameters params_; ///< 参数配置
        std::string script_path_;    ///< Python脚本路径
        std::string temp_dir_;       ///< 临时目录路径
        bool initialized_;           ///< 是否已初始化

        /**
         * @brief 查找LightGlue Python脚本路径
         * @return 脚本路径，空字符串表示未找到
         */
        std::string FindLightGlueScript();

        /**
         * @brief 创建临时目录
         * @return 临时目录路径
         */
        std::string CreateTempDirectory();

        /**
         * @brief 保存特征数据到临时文件
         * @param keypoints 特征点
         * @param descriptors 描述子
         * @param output_path 输出文件路径
         * @return 是否保存成功
         */
        bool SaveFeaturesToFile(
            const std::vector<cv::KeyPoint> &keypoints,
            const cv::Mat &descriptors,
            const std::string &output_path);

        /**
         * @brief 从文件加载匹配结果
         * @param matches_path 匹配结果文件路径
         * @param matches 输出匹配结果
         * @return 是否加载成功
         */
        bool LoadMatchesFromFile(
            const std::string &matches_path,
            std::vector<cv::DMatch> &matches);

        /**
         * @brief 执行Python脚本
         * @param img1_path 第一张图像路径
         * @param img2_path 第二张图像路径
         * @param features1_path 第一张图像特征文件路径
         * @param features2_path 第二张图像特征文件路径
         * @param output_path 输出匹配结果路径
         * @return 是否执行成功
         */
        bool RunPythonScript(
            const std::string &img1_path,
            const std::string &img2_path,
            const std::string &features1_path,
            const std::string &features2_path,
            const std::string &output_path);

        /**
         * @brief 清理临时文件
         * @param file_paths 要清理的文件路径列表
         */
        void CleanupTempFiles(const std::vector<std::string> &file_paths);

        /**
         * @brief 将LightGlue特征类型转换为字符串
         * @param type 特征类型
         * @return 字符串表示
         */
        static std::string FeatureTypeToString(LightGlueFeatureType type);
    };

} // namespace PluginMethods
