/**
 * @file img2features_pipeline.hpp
 * @brief Image feature extraction pipeline | 图像特征提取流水线
 * @details Supports SIFT, ORB, AKAZE, SuperPoint and other feature detectors | 支持SIFT、ORB、AKAZE、SuperPoint等多种特征检测器
 * @copyright Copyright (c) 2024 PoSDK
 */

#pragma once

#include <po_core/po_logger.hpp>
#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <string>
#include <filesystem>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;
    using namespace Converter;

    /**
     * @brief Image feature extraction pipeline | 图像特征提取流水线
     *
     * Encapsulates the complete processing flow of multiple feature detectors, | 封装了多种特征检测器的完整处理流程，
     * supports traditional features (SIFT, ORB, etc.) and deep learning features (SuperPoint) | 支持传统特征（SIFT、ORB等）和深度学习特征（SuperPoint）
     */
    class Img2FeaturesPipeline : public MethodPresetProfiler
    {
    public:
        /**
         * @brief Constructor | 构造函数
         */
        Img2FeaturesPipeline()
        {

            required_package_["data_images"] = nullptr;
        }

        /**
         * @brief Destructor | 析构函数
         */
        virtual ~Img2FeaturesPipeline() = default;

        /**
         * @brief Get method type | 获取方法类型
         * @return Method type string | 方法类型字符串
         */
        const std::string &GetType() const override;

        /**
         * @brief Execute feature extraction pipeline | 执行特征提取流水线
         * @return Processing result data | 处理结果数据
         */
        DataPtr Run() override;

        /**
         * @brief Feature detection core function | 特征检测核心函数
         * @param image Input image | 输入图像
         * @param keypoints Output keypoints | 输出特征点
         * @param descriptors Output descriptors | 输出描述子
         */
        void DetectFeatures(cv::Mat &image,
                            std::vector<cv::KeyPoint> &keypoints,
                            cv::Mat &descriptors);

    private:
        /**
         * @brief Interactive running mode using image viewer | 使用图像查看器的交互式运行方式
         */
        DataPtr RunWithImageViewer();

        /**
         * @brief Fast running mode, no GUI | 快速运行方式，无GUI
         */
        DataPtr RunFast();

        /**
         * @brief Create feature detector | 创建特征检测器
         */
        cv::Ptr<cv::Feature2D> CreateDetector();

        // Feature detection strategy base class | 特征检测策略基类
        class DetectorStrategy
        {
        public:
            virtual ~DetectorStrategy() = default;
            virtual void Process(const cv::Mat &image,
                                 std::vector<cv::KeyPoint> &keypoints,
                                 cv::Mat &descriptors,
                                 cv::Ptr<cv::Feature2D> detector) = 0;
        };

        // Standard detector strategy (SIFT, ORB etc.) | 常规检测器策略（SIFT, ORB等）
        class StandardDetectorStrategy : public DetectorStrategy
        {
        public:
            void Process(const cv::Mat &image,
                         std::vector<cv::KeyPoint> &keypoints,
                         cv::Mat &descriptors,
                         cv::Ptr<cv::Feature2D> detector) override
            {
                detector->detectAndCompute(image, cv::Mat(), keypoints, descriptors);
            }
        };

        // Pure keypoint detector strategy (FAST, AGAST) | 纯关键点检测器策略（FAST, AGAST）
        class KeypointOnlyDetectorStrategy : public DetectorStrategy
        {
        public:
            void Process(const cv::Mat &image,
                         std::vector<cv::KeyPoint> &keypoints,
                         cv::Mat &descriptors,
                         cv::Ptr<cv::Feature2D> detector) override
            {
                detector->detect(image, keypoints);
                if (!keypoints.empty())
                {
                    auto descriptor_extractor = cv::ORB::create();
                    descriptor_extractor->compute(image, keypoints, descriptors);
                }
            }
        };

        // AKAZE special processing strategy | AKAZE特殊处理策略
        class AKAZEDetectorStrategy : public DetectorStrategy
        {
        public:
            void Process(const cv::Mat &image,
                         std::vector<cv::KeyPoint> &keypoints,
                         cv::Mat &descriptors,
                         cv::Ptr<cv::Feature2D> detector) override
            {
                detector->detect(image, keypoints);
                if (!keypoints.empty())
                {
                    detector->compute(image, keypoints, descriptors);
                }
            }
        };

        // SuperPoint feature extraction strategy (via Python script) | SuperPoint特征提取策略（通过Python脚本）
        class SuperPointDetectorStrategy : public DetectorStrategy
        {
        private:
            Img2FeaturesPipeline *plugin_;

        public:
            SuperPointDetectorStrategy(Img2FeaturesPipeline *plugin) : plugin_(plugin) {}

            void Process(const cv::Mat &image,
                         std::vector<cv::KeyPoint> &keypoints,
                         cv::Mat &descriptors,
                         cv::Ptr<cv::Feature2D> detector) override;

        private:
            bool RunSuperPointExtraction(const cv::Mat &image,
                                         std::vector<cv::KeyPoint> &keypoints,
                                         cv::Mat &descriptors);
            std::string FindSuperPointScript();
            std::string CreateTempDirectory();
            bool LoadSuperPointFeatures(const std::string &features_path,
                                        std::vector<cv::KeyPoint> &keypoints,
                                        cv::Mat &descriptors);
            void CleanupTempFiles(const std::vector<std::string> &file_paths);
            std::string CheckAndSetupPythonEnvironment();
        };

        // Get the corresponding detection strategy | 获取对应的检测策略
        std::unique_ptr<DetectorStrategy> GetDetectorStrategy(const std::string &detector_type)
        {
            if (detector_type == "FAST" || detector_type == "AGAST")
            {
                return std::make_unique<KeypointOnlyDetectorStrategy>();
            }
            else if (detector_type == "AKAZE")
            {
                return std::make_unique<AKAZEDetectorStrategy>();
            }
            else if (detector_type == "SUPERPOINT")
            {
                return std::make_unique<SuperPointDetectorStrategy>(this);
            }
            return std::make_unique<StandardDetectorStrategy>();
        }
    };

} // namespace PluginMethods
