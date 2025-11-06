/**
 * @file img2matches_pipeline.hpp
 * @brief 图像特征匹配处理流水线
 * @copyright Copyright (c) 2024 PoSDK
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <common/image_viewer/image_viewer.hpp>
#include "Img2MatchesParams.hpp"
#include "../Img2Features/img2features_pipeline.hpp"
#include <opencv2/features2d.hpp>
#include <filesystem>
#include <vector>
#include <memory>
#include <po_core/po_logger.hpp>
#include <atomic>
#include <mutex>

// OpenMP support for multi-threading | OpenMP支持多线程处理
#ifdef USE_OPENMP
#include <omp.h>
#endif
namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace types;
    using namespace Converter;
    using namespace common;

    /**
     * @brief 图像特征匹配处理流水线
     *
     * 封装了从图像特征提取到特征匹配的完整处理流程，
     * 支持快速模式和可视化模式两种运行方式。
     */
    class Img2MatchesPipeline : public PluginMethods::Img2FeaturesPipeline
    {
    public:
        /**
         * @brief 构造函数
         */
        Img2MatchesPipeline();

        /**
         * @brief 析构函数
         */
        virtual ~Img2MatchesPipeline() = default;

        /**
         * @brief 获取方法类型
         * @return 方法类型字符串
         */
        const std::string &GetType() const override;

        /**
         * @brief 执行特征匹配流水线
         * @return 处理结果数据
         */
        DataPtr Run() override;

    private:
        // 流水线步骤函数
        /**
         * @brief 快速模式执行函数
         * @return 匹配结果数据
         */
        DataPtr RunFastMode();

        /**
         * @brief 可视化模式执行函数
         * @return 匹配结果数据
         */
        DataPtr RunViewerMode();

        // 核心匹配功能函数
        /**
         * @brief 执行特征匹配的核心函数
         * @param descriptors1 第一幅图像的描述子
         * @param descriptors2 第二幅图像的描述子
         * @return 匹配结果
         */
        std::vector<cv::DMatch> MatchFeatures(const cv::Mat &descriptors1,
                                              const cv::Mat &descriptors2);

        /**
         * @brief 线程安全的特征匹配函数（确保确定性结果）
         * @param descriptors1 第一幅图像的描述子
         * @param descriptors2 第二幅图像的描述子
         * @param view_id1 第一幅图像的视图ID
         * @param view_id2 第二幅图像的视图ID
         * @return 匹配结果
         */
        std::vector<cv::DMatch> MatchFeaturesThreadSafe(const cv::Mat &descriptors1,
                                                        const cv::Mat &descriptors2,
                                                        IndexT view_id1, IndexT view_id2);

        /**
         * @brief 执行LightGlue特征匹配（需要完整图像和特征点信息）
         * @param img1 第一张图像
         * @param img2 第二张图像
         * @param keypoints1 第一张图像的特征点
         * @param keypoints2 第二张图像的特征点
         * @param descriptors1 第一张图像的描述子
         * @param descriptors2 第二张图像的描述子
         * @return 匹配结果
         */
        std::vector<cv::DMatch> MatchFeaturesWithLightGlue(
            const cv::Mat &img1, const cv::Mat &img2,
            const std::vector<cv::KeyPoint> &keypoints1,
            const std::vector<cv::KeyPoint> &keypoints2,
            const cv::Mat &descriptors1, const cv::Mat &descriptors2);

        /**
         * @brief 可视化匹配结果
         * @param img1 第一幅图像
         * @param img2 第二幅图像
         * @param keypoints1 第一幅图像的特征点
         * @param keypoints2 第二幅图像的特征点
         * @param matches 匹配结果
         * @param window_name 显示窗口名称
         */
        void VisualizeMatches(const cv::Mat &img1, const cv::Mat &img2,
                              const std::vector<cv::KeyPoint> &keypoints1,
                              const std::vector<cv::KeyPoint> &keypoints2,
                              const std::vector<cv::DMatch> &matches,
                              const std::string &window_name);

        // 辅助函数
        /**
         * @brief 解析视图对索引
         * @return 视图对索引
         */
        std::pair<size_t, size_t> ParseViewPair();

        /**
         * @brief 验证视图对索引的有效性
         * @param i 第一个视图索引
         * @param j 第二个视图索引
         * @param max_size 最大有效索引
         */
        void ValidateViewPairIndices(size_t i, size_t j, size_t max_size);

        /**
         * @brief 处理已存在的特征数据
         * @param features_info_ptr 特征信息指针
         * @param all_keypoints 输出的关键点集合
         * @param all_descriptors 输出的描述子集合
         * @param all_view_ids 输出的视图ID集合
         * @param all_image_paths 输出的图像路径集合
         */
        void ProcessExistingFeatures(FeaturesInfoPtr features_info_ptr,
                                     std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
                                     std::vector<cv::Mat> &all_descriptors,
                                     std::vector<IndexT> &all_view_ids,
                                     std::vector<std::string> &all_image_paths,
                                     std::vector<cv::Mat> *all_images = nullptr);

        /**
         * @brief 提取新的特征数据
         * @param image_paths_ptr 图像路径指针
         * @param features_info_ptr 特征信息指针
         * @param all_keypoints 输出的关键点集合
         * @param all_descriptors 输出的描述子集合
         * @param all_view_ids 输出的视图ID集合
         * @param all_image_paths 输出的图像路径集合
         */
        void ExtractNewFeatures(ImagePathsPtr image_paths_ptr,
                                FeaturesInfoPtr features_info_ptr,
                                std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
                                std::vector<cv::Mat> &all_descriptors,
                                std::vector<IndexT> &all_view_ids,
                                std::vector<std::string> &all_image_paths,
                                std::vector<cv::Mat> *all_images = nullptr);

        /**
         * @brief 执行全对匹配（单线程版本）
         * @param all_descriptors 所有描述子
         * @param all_view_ids 所有视图ID
         * @param matches_ptr 匹配结果指针
         * @return 成功匹配的对数
         */
        size_t PerformPairwiseMatching(const std::vector<cv::Mat> &all_descriptors,
                                       const std::vector<IndexT> &all_view_ids,
                                       MatchesPtr matches_ptr,
                                       const std::vector<std::vector<cv::KeyPoint>> *all_keypoints = nullptr,
                                       const std::vector<cv::Mat> *all_images = nullptr);

        /**
         * @brief 执行全对匹配（多线程版本）
         * @param all_descriptors 所有描述子
         * @param all_view_ids 所有视图ID
         * @param matches_ptr 匹配结果指针
         * @return 成功匹配的对数
         */
        size_t PerformPairwiseMatchingMultiThreads(const std::vector<cv::Mat> &all_descriptors,
                                                   const std::vector<IndexT> &all_view_ids,
                                                   MatchesPtr matches_ptr,
                                                   const std::vector<std::vector<cv::KeyPoint>> *all_keypoints = nullptr,
                                                   const std::vector<cv::Mat> *all_images = nullptr);

        /**
         * @brief 导出结果数据
         * @param features_data_ptr 特征数据指针
         * @param matches_data_ptr 匹配数据指针
         */
        void ExportResults(DataPtr features_data_ptr, DataPtr matches_data_ptr);

    private:
        /**
         * @brief 在运行时加载配置
         * @details 加载主配置和特定检测器/匹配器配置，避免在构造函数中加载
         */
        void LoadConfigurationAtRuntime();

        /**
         * @brief 应用RootSIFT归一化到描述子
         * @param descriptors 输入输出的描述子矩阵
         */
        void ApplyRootSIFTNormalization(cv::Mat &descriptors);

        /**
         * @brief 应用first_octave图像预处理（上采样/下采样）
         * @param img 输入图像
         * @return 处理后的图像
         */
        cv::Mat ApplyFirstOctaveProcessing(const cv::Mat &img);

        /**
         * @brief 根据图像缩放调整特征点坐标
         * @param keypoints 特征点（会被修改）
         * @param first_octave first_octave参数值
         */
        void AdjustKeypointsForScaling(std::vector<cv::KeyPoint> &keypoints, int first_octave);

        /**
         * @brief 根据FLANN参数配置创建FLANN匹配器
         * @return 配置好的FLANN匹配器
         */
        cv::Ptr<cv::DescriptorMatcher> CreateFLANNMatcher();

        /**
         * @brief 显示进度条
         * @param current 当前进度
         * @param total 总数
         * @param task_name 任务名称
         * @param bar_width 进度条宽度（默认50）
         */
        void ShowProgressBar(size_t current, size_t total, const std::string &task_name, int bar_width = 50);

        // 匹配参数调整回调数据结构
        struct MatcherCallbackData
        {
            cv::Mat *img1 = nullptr;
            cv::Mat *img2 = nullptr;
            std::vector<cv::KeyPoint> *keypoints1 = nullptr;
            std::vector<cv::KeyPoint> *keypoints2 = nullptr;
            std::vector<cv::DMatch> *matches = nullptr;
            cv::Mat *descriptors1 = nullptr;
            cv::Mat *descriptors2 = nullptr;
            Img2MatchesPipeline *plugin = nullptr;
            ImageViewer *viewer = nullptr;
        };

        // 参数容器
        Img2MatchesParameters params_;
    };

} // namespace PluginMethods
