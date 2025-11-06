/**
 * @file converter_opencv.hpp
 * @brief OpenCV data type converter | OpenCV数据类型转换器
 * @details Provides high-performance conversion functionality between OpenCV and PoSDK data types
 *          提供OpenCV与PoSDK数据类型之间的高性能转换功能
 *
 * @architecture Three-level conversion hierarchy | 三层转换架构：
 *   Level 1: FeaturePoints ↔ std::vector<cv::KeyPoint> (SOA batch conversion | SOA批量转换)
 *   Level 2: ImageFeatureInfo ↔ CV Features (single image | 单图像)
 *   Level 3: FeaturesInfo ↔ CV Features (multiple images | 多图像)
 *
 * @copyright Copyright (c) 2024 Qi Cai
 * Licensed under the Mozilla Public License Version 2.0
 */

#ifndef _CONVERTER_OPENCV_
#define _CONVERTER_OPENCV_

#include "converter_base.hpp"
#include "po_core.hpp"
#include <po_core/po_logger.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <vector>

namespace PoSDK
{
    namespace Converter
    {

        using namespace PoSDK::types;

        /**
         * @brief OpenCV and PoSDK data converter | OpenCV与PoSDK数据转换器
         * @details High-performance converter with SIMD-friendly batch operations
         *          高性能转换器，支持SIMD友好的批量操作
         */
        class OpenCVConverter
        {
        public:
            // Descriptor type enumeration | 描述子类型枚举
            enum class DescriptorType
            {
                UINT8,  // 8-bit unsigned integer type (BRISK/ORB/etc.) | 8位无符号整数类型(BRISK/ORB等)
                FLOAT32 // 32-bit floating point type (SIFT/SURF/etc.) | 32位浮点数类型(SIFT/SURF等)
            };

            /**
             * @brief Get corresponding descriptor type based on detector type
             *        根据检测器类型获取对应的描述子类型
             */
            static DescriptorType ParseDescriptorType(const std::string &detector_type);

            // ==================================================================================
            // Level 1: FeaturePoints ↔ std::vector<cv::KeyPoint> (SOA Batch Conversion)
            //          FeaturePoints ↔ std::vector<cv::KeyPoint> (SOA批量转换)
            // ==================================================================================

            /**
             * @brief 🚀 Batch convert OpenCV keypoints to PoSDK FeaturePoints (SIMD-optimized)
             *        批量转换OpenCV关键点为PoSDK FeaturePoints（SIMD优化）
             * @param keypoints OpenCV keypoints | OpenCV关键点集合
             * @param feature_points [out] PoSDK FeaturePoints (SOA storage) | [out] PoSDK特征点（SOA存储）
             * @return Whether conversion was successful | 转换是否成功
             * @note Uses efficient batch allocation and contiguous memory writes
             *       使用高效的批量分配和连续内存写入
             */
            static bool CVKeyPoints2FeaturePoints(
                const std::vector<cv::KeyPoint> &keypoints,
                FeaturePoints &feature_points);

            /**
             * @brief 🚀 Batch convert PoSDK FeaturePoints to OpenCV keypoints (zero-copy read)
             *        批量转换PoSDK FeaturePoints为OpenCV关键点（零拷贝读取）
             * @param feature_points PoSDK FeaturePoints (SOA storage) | PoSDK特征点（SOA存储）
             * @param keypoints [out] OpenCV keypoints | [out] OpenCV关键点集合
             * @return Whether conversion was successful | 转换是否成功
             * @note Uses SOA batch accessors for SIMD-friendly memory access
             *       使用SOA批量访问器实现SIMD友好的内存访问
             */
            static bool FeaturePoints2CVKeyPoints(
                const FeaturePoints &feature_points,
                std::vector<cv::KeyPoint> &keypoints);

            /**
             * @brief 🚀 SIMD-optimized batch convert OpenCV descriptors to Descriptors (SOA format)
             *        SIMD优化的批量转换OpenCV描述子为Descriptors
             * @param descriptors_cv OpenCV descriptor matrix (N×D) | OpenCV描述子矩阵 (N×D)
             * @param descriptors_out [out] Descriptors (SOA format) | [out] Descriptors（SOA格式）
             * @param detector_type Detector type (SIFT/ORB/etc.) | 检测器类型（SIFT/ORB等）
             * @return Whether conversion was successful | 转换是否成功
             * @note Single contiguous memory allocation with SIMD-optimized conversion
             *       单次连续内存分配，SIMD优化转换
             */
            static bool CVDescriptors2Descriptors(
                const cv::Mat &descriptors_cv,
                Descriptors &descriptors_out,
                const std::string &detector_type = "SIFT");

            /**
             * @brief 🚀 SIMD-optimized batch convert Descriptors to OpenCV descriptors
             *        SIMD优化的批量转换Descriptors为OpenCV描述子
             * @param descriptors Descriptors (SOA format) | Descriptors（SOA格式）
             * @param descriptors_cv_out [out] OpenCV descriptor matrix | [out] OpenCV描述子矩阵
             * @param detector_type Detector type (SIFT/ORB/etc.) | 检测器类型（SIFT/ORB等）
             * @return Whether conversion was successful | 转换是否成功
             * @note Zero-copy optimization for FLOAT32, SIMD conversion for UINT8
             *       FLOAT32零拷贝优化，UINT8 SIMD转换
             */
            static bool Descriptors2CVDescriptors(
                const Descriptors &descriptors,
                cv::Mat &descriptors_cv_out,
                const std::string &detector_type = "SIFT");

            // ==================================================================================
            // Level 2: ImageFeatureInfo ↔ CV Features (Single Image Conversion)
            //          ImageFeatureInfo ↔ CV Features (单图像转换)
            // ==================================================================================

            /**
             * @brief Batch convert OpenCV features to ImageFeatureInfo (without descriptors)
             *        批量转换OpenCV特征为ImageFeatureInfo（不含描述子）
             * @param keypoints OpenCV keypoints | OpenCV关键点
             * @param image_features [out] ImageFeatureInfo | [out] 图像特征信息
             * @param image_path Image file path | 图像文件路径
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool CVFeatures2ImageFeatureInfo(
                const std::vector<cv::KeyPoint> &keypoints,
                ImageFeatureInfo &image_features,
                const std::string &image_path = "");

            /**
             * @brief Batch convert OpenCV features to ImageFeatureInfo (with descriptors)
             *        批量转换OpenCV特征为ImageFeatureInfo（含描述子）
             * @param keypoints OpenCV keypoints | OpenCV关键点
             * @param descriptors_cv OpenCV descriptor matrix | OpenCV描述子矩阵
             * @param image_features [out] ImageFeatureInfo | [out] 图像特征信息
             * @param descriptors_out [out] PoSDK descriptors | [out] PoSDK描述子
             * @param image_path Image file path | 图像文件路径
             * @param detector_type Detector type | 检测器类型
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool CVFeatures2ImageFeatureInfoWithDesc(
                const std::vector<cv::KeyPoint> &keypoints,
                const cv::Mat &descriptors_cv,
                ImageFeatureInfo &image_features,
                Descriptors &descriptors_out,
                const std::string &image_path = "",
                const std::string &detector_type = "SIFT");

            /**
             * @brief 🚀 Batch convert ImageFeatureInfo to OpenCV features (without descriptors)
             *        批量转换ImageFeatureInfo为OpenCV特征（不含描述子）
             * @param image_features ImageFeatureInfo | 图像特征信息
             * @param keypoints [out] OpenCV keypoints | [out] OpenCV关键点
             * @return Whether conversion was successful | 转换是否成功
             * @note Uses zero-copy SOA batch access for maximum performance
             *       使用零拷贝SOA批量访问实现最大性能
             */
            static bool ImageFeatureInfo2CVFeatures(
                const ImageFeatureInfo &image_features,
                std::vector<cv::KeyPoint> &keypoints);

            /**
             * @brief 🚀 Batch convert ImageFeatureInfo to OpenCV features (with descriptors)
             *        批量转换ImageFeatureInfo为OpenCV特征（含描述子）
             * @param image_features ImageFeatureInfo | 图像特征信息
             * @param descriptors PoSDK descriptors | PoSDK描述子
             * @param keypoints [out] OpenCV keypoints | [out] OpenCV关键点
             * @param descriptors_out [out] OpenCV descriptor matrix | [out] OpenCV描述子矩阵
             * @param detector_type Detector type | 检测器类型
             * @return Whether conversion was successful | 转换是否成功
             * @note Handles descriptor count mismatch gracefully (converts keypoints only)
             *       优雅处理描述子数量不匹配（仅转换关键点）
             */
            static bool ImageFeatureInfo2CVFeaturesWithDesc(
                const ImageFeatureInfo &image_features,
                const Descriptors &descriptors,
                std::vector<cv::KeyPoint> &keypoints,
                cv::Mat &descriptors_out,
                const std::string &detector_type = "SIFT");

            // ==================================================================================
            // Level 3: FeaturesInfo ↔ CV Features (Multiple Images Batch Conversion)
            //          FeaturesInfo ↔ CV Features (多图像批量转换)
            // ==================================================================================

            /**
             * @brief Batch convert OpenCV features to FeaturesInfo (without descriptors)
             *        批量转换OpenCV特征为FeaturesInfo（不含描述子）
             * @param keypoints OpenCV keypoints | OpenCV关键点
             * @param features_info_ptr [out] FeaturesInfo pointer | [out] FeaturesInfo指针
             * @param image_path Image file path | 图像文件路径
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool CVFeatures2FeaturesInfo(
                const std::vector<cv::KeyPoint> &keypoints,
                FeaturesInfoPtr &features_info_ptr,
                const std::string &image_path);

            /**
             * @brief Batch convert OpenCV features to FeaturesInfo (with descriptors)
             *        批量转换OpenCV特征为FeaturesInfo（含描述子）
             * @param keypoints OpenCV keypoints | OpenCV关键点
             * @param descriptors OpenCV descriptor matrix | OpenCV描述子矩阵
             * @param features_info_ptr [out] FeaturesInfo pointer | [out] FeaturesInfo指针
             * @param descriptors_out [out] PoSDK descriptors | [out] PoSDK描述子
             * @param image_path Image file path | 图像文件路径
             * @param detector_type Detector type | 检测器类型
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool CVFeatures2FeaturesInfoWithDesc(
                const std::vector<cv::KeyPoint> &keypoints,
                const cv::Mat &descriptors,
                FeaturesInfoPtr &features_info_ptr,
                Descriptors &descriptors_out,
                const std::string &image_path,
                const std::string &detector_type = "SIFT");

            /**
             * @brief Batch convert FeaturesInfo to OpenCV features (without descriptors)
             *        批量转换FeaturesInfo为OpenCV特征（不含描述子）
             * @param image_features ImageFeatureInfo from FeaturesInfo | 来自FeaturesInfo的ImageFeatureInfo
             * @param keypoints [out] OpenCV keypoints | [out] OpenCV关键点
             * @return Whether conversion was successful | 转换是否成功
             * @note This is an alias for ImageFeatureInfo2CVFeatures for consistency
             *       这是ImageFeatureInfo2CVFeatures的别名以保持一致性
             */
            static bool FeaturesInfo2CVFeatures(
                const ImageFeatureInfo &image_features,
                std::vector<cv::KeyPoint> &keypoints);

            /**
             * @brief Batch convert FeaturesInfo to OpenCV features (with descriptors)
             *        批量转换FeaturesInfo为OpenCV特征（含描述子）
             * @param image_features ImageFeatureInfo from FeaturesInfo | 来自FeaturesInfo的ImageFeatureInfo
             * @param descriptors PoSDK descriptors | PoSDK描述子
             * @param keypoints [out] OpenCV keypoints | [out] OpenCV关键点
             * @param descriptors_out [out] OpenCV descriptor matrix | [out] OpenCV描述子矩阵
             * @param detector_type Detector type | 检测器类型
             * @return Whether conversion was successful | 转换是否成功
             * @note This is an alias for ImageFeatureInfo2CVFeaturesWithDesc for consistency
             *       这是ImageFeatureInfo2CVFeaturesWithDesc的别名以保持一致性
             */
            static bool FeaturesInfo2CVFeaturesWithDesc(
                const ImageFeatureInfo &image_features,
                const Descriptors &descriptors,
                std::vector<cv::KeyPoint> &keypoints,
                cv::Mat &descriptors_out,
                const std::string &detector_type = "SIFT");

            // ==================================================================================
            // Match Conversion Functions | 匹配转换函数
            // ==================================================================================

            /**
             * @brief Convert OpenCV DMatch to PoSDK IdMatch (single match)
             *        转换OpenCV DMatch到PoSDK IdMatch（单个匹配）
             */
            static void CVDMatch2IdMatch(const cv::DMatch &cv_match, IdMatch &match);

            /**
             * @brief Convert OpenCV DMatch to PoSDK IdMatches (batch conversion)
             *        转换OpenCV DMatch到PoSDK IdMatches（批量转换）
             */
            static bool CVDMatch2IdMatches(const std::vector<cv::DMatch> &cv_matches,
                                           IdMatches &matches);

            /**
             * @brief Convert OpenCV DMatch to PoSDK Matches with view pair
             *        转换OpenCV DMatch到PoSDK Matches（带视图对）
             */
            static bool CVDMatch2Matches(const std::vector<cv::DMatch> &cv_matches,
                                         const IndexT view_id1,
                                         const IndexT view_id2,
                                         MatchesPtr &matches_ptr);

            /**
             * @brief Convert PoSDK IdMatch to OpenCV DMatch (single match)
             *        转换PoSDK IdMatch到OpenCV DMatch（单个匹配）
             */
            static void IdMatch2CVDMatch(const IdMatch &match, cv::DMatch &cv_match);

            /**
             * @brief Convert PoSDK IdMatches to OpenCV point pairs
             *        将PoSDK IdMatches转换为OpenCV点对
             * @param matches Feature matches | 特征匹配
             * @param features_info Feature information | 特征信息
             * @param camera_models Camera model collection | 相机模型集合
             * @param view_pair View pair | 视图对
             * @param points1 [out] First view point set | [out] 第一视图点集
             * @param points2 [out] Second view point set | [out] 第二视图点集
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool IdMatches2CVPoints(
                const IdMatches &matches,
                const FeaturesInfo &features_info,
                const CameraModels &camera_models,
                const ViewPair &view_pair,
                std::vector<cv::Point2f> &points1,
                std::vector<cv::Point2f> &points2);

            /**
             * @brief Convert PoSDK match DataPtr to OpenCV point pairs
             *        将PoSDK匹配DataPtr转换为OpenCV点对
             * @param matches_data_ptr Match data pointer | 匹配数据指针
             * @param features_info Feature information | 特征信息
             * @param camera_models Camera model collection | 相机模型集合
             * @param view_pair View pair | 视图对
             * @param points1 [out] First view point set | [out] 第一视图点集
             * @param points2 [out] Second view point set | [out] 第二视图点集
             * @return Whether conversion was successful | 转换是否成功
             */
            static bool MatchesDataPtr2CVPoints(
                const DataPtr &matches_data_ptr,
                const FeaturesInfo &features_info,
                const CameraModels &camera_models,
                const ViewPair &view_pair,
                std::vector<cv::Point2f> &points1,
                std::vector<cv::Point2f> &points2);

            // ==================================================================================
            // Camera Model Conversion Functions | 相机模型转换函数
            // ==================================================================================

            /**
             * @brief Convert OpenCV camera calibration to PoSDK CameraModel
             *        转换OpenCV相机标定到PoSDK CameraModel
             */
            static bool CVCalibration2CameraModel(const cv::Mat &camera_matrix,
                                                  const cv::Mat &dist_coeffs,
                                                  const cv::Size &image_size,
                                                  CameraModel &camera_model,
                                                  DistortionType distortion_type = DistortionType::BROWN_CONRADY);

            /**
             * @brief Convert PoSDK CameraModel to OpenCV camera calibration
             *        转换PoSDK CameraModel到OpenCV相机标定
             */
            static bool CameraModel2CVCalibration(const CameraModel &camera_model,
                                                  cv::Mat &camera_matrix,
                                                  cv::Mat &dist_coeffs);

        private:
            // ==================================================================================
            // Internal Helper Functions | 内部辅助函数
            // ==================================================================================

            /**
             * @brief Convert single OpenCV keypoint to PoSDK feature data
             *        单个OpenCV关键点转换为PoSDK特征数据
             */
            static void CVKeyPoint2Feature(
                const cv::KeyPoint &kp,
                Feature &coord,
                float &size,
                float &angle);

            /**
             * @brief Convert single PoSDK feature to OpenCV keypoint
             *        单个PoSDK特征转换为OpenCV关键点
             */
            static void Feature2CVKeyPoint(
                const FeaturePoints &feature_points,
                size_t feature_index,
                cv::KeyPoint &kp);
        };

    } // namespace Converter
} // namespace PoSDK

#endif // _CONVERTER_OPENCV_
