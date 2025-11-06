/**
 * @file converter_opencv.cpp
 * @brief OpenCV data type converter implementation | OpenCV数据类型转换器实现
 * @details High-performance conversion implementation with SIMD-friendly batch operations
 *          高性能转换实现，支持SIMD友好的批量操作
 * @copyright Copyright (c) 2024 Qi Cai
 * Licensed under the Mozilla Public License Version 2.0
 */

#include "converter_opencv.hpp"
#include <po_core/types.hpp>
#include <algorithm>

namespace PoSDK
{
    namespace Converter
    {
        using namespace PoSDK::types;

        // ==================================================================================
        // Descriptor Type Parsing | 描述子类型解析
        // ==================================================================================

        OpenCVConverter::DescriptorType OpenCVConverter::ParseDescriptorType(const std::string &detector_type)
        {
            // SIFT and SURF use FLOAT32 descriptors | SIFT和SURF使用FLOAT32描述子
            if (detector_type == "SIFT" || detector_type == "SURF")
            {
                return DescriptorType::FLOAT32;
            }
            // BRISK, ORB, AKAZE use UINT8 descriptors | BRISK、ORB、AKAZE使用UINT8描述子
            else if (detector_type == "BRISK" || detector_type == "ORB" || detector_type == "AKAZE")
            {
                return DescriptorType::UINT8;
            }
            // Default to FLOAT32 | 默认使用FLOAT32
            return DescriptorType::FLOAT32;
        }

        // ==================================================================================
        // Level 1: FeaturePoints ↔ std::vector<cv::KeyPoint> (SOA Batch Conversion)
        //          FeaturePoints ↔ std::vector<cv::KeyPoint> (SOA批量转换)
        // ==================================================================================

        bool OpenCVConverter::CVKeyPoints2FeaturePoints(
            const std::vector<cv::KeyPoint> &keypoints,
            FeaturePoints &feature_points)
        {
            try
            {
                const size_t num_features = keypoints.size();
                if (num_features == 0)
                {
                    LOG_DEBUG_ZH << "[OpenCVConverter] 警告: 输入关键点为空";
                    LOG_DEBUG_EN << "[OpenCVConverter] Warning: Input keypoints are empty";
                    return true;
                }

                // 🚀 Batch allocate SOA storage (SIMD-friendly contiguous memory)
                // 🚀 批量分配SOA存储（SIMD友好的连续内存）
                feature_points.resize(num_features);

                // 🚀 Batch write using SOA accessors (compiler can vectorize)
                // 🚀 使用SOA访问器批量写入（编译器可以向量化）
                auto &coords = feature_points.GetCoordsRef(); // 2×N Eigen matrix | 2×N Eigen矩阵
                auto &sizes = feature_points.GetSizesRef();   // vector<float>
                auto &angles = feature_points.GetAnglesRef(); // vector<float>

                // Batch conversion loop (SIMD-optimizable) | 批量转换循环（可SIMD优化）
                for (size_t i = 0; i < num_features; ++i)
                {
                    coords(0, i) = static_cast<double>(keypoints[i].pt.x);
                    coords(1, i) = static_cast<double>(keypoints[i].pt.y);
                    sizes[i] = keypoints[i].size;
                    angles[i] = keypoints[i].angle;
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVKeyPoints2FeaturePoints转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVKeyPoints2FeaturePoints: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::FeaturePoints2CVKeyPoints(
            const FeaturePoints &feature_points,
            std::vector<cv::KeyPoint> &keypoints)
        {
            try
            {
                const size_t num_features = feature_points.size();

                if (num_features == 0)
                {
                    keypoints.clear();
                    return true;
                }

                // 🚀 Batch allocate all cv::KeyPoint objects at once (reduces memory allocations)
                // 🚀 批量分配所有cv::KeyPoint对象（减少内存分配次数）
                keypoints.resize(num_features);

                // 🚀 Zero-copy batch read using SOA accessors (SIMD-friendly)
                // 🚀 使用SOA访问器零拷贝批量读取（SIMD友好）
                const auto &coords = feature_points.GetCoordsRef(); // 2×N Eigen matrix | 2×N Eigen矩阵
                const auto &sizes = feature_points.GetSizesRef();   // const vector<float>&
                const auto &angles = feature_points.GetAnglesRef(); // const vector<float>&

                // 🚀 Direct write to pre-allocated memory (better cache locality, compiler can vectorize)
                // 🚀 直接写入预分配内存（更好的缓存局部性，编译器可以向量化）
                for (size_t i = 0; i < num_features; ++i)
                {
                    keypoints[i].pt.x = static_cast<float>(coords(0, i));
                    keypoints[i].pt.y = static_cast<float>(coords(1, i));
                    keypoints[i].size = sizes[i];
                    keypoints[i].angle = angles[i];
                    // Set default values for other fields | 设置其他字段的默认值
                    keypoints[i].response = 0.0f;
                    keypoints[i].octave = 0;
                    keypoints[i].class_id = -1;
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] FeaturePoints2CVKeyPoints转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in FeaturePoints2CVKeyPoints: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::CVDescriptors2Descriptors(
            const cv::Mat &descriptors_cv,
            Descriptors &descriptors_out,
            const std::string &detector_type)
        {
            try
            {
                if (descriptors_cv.empty())
                {
                    descriptors_out.clear();
                    return true;
                }

                const DescriptorType desc_type = ParseDescriptorType(detector_type);
                const size_t num_features = descriptors_cv.rows;
                const size_t descriptor_dim = descriptors_cv.cols;

                // 🚀 Batch allocate all memory at once (single allocation) | 一次性批量分配所有内存（单次分配）
                descriptors_out.resize(num_features, descriptor_dim);

                if (desc_type == DescriptorType::UINT8)
                {
                    // UINT8 → FLOAT32 conversion with SIMD optimization | UINT8 → FLOAT32转换，使用SIMD优化
#if defined(POSDK_SIMD_ENABLED)
                    for (size_t i = 0; i < num_features; ++i)
                    {
                        const uint8_t *src = descriptors_cv.ptr<uint8_t>(i);
                        float *dest = descriptors_out[i];

                        size_t j = 0;
                        const size_t simd_end = descriptor_dim - (descriptor_dim % 8);

                        // AVX2: convert 8 uint8_t to 8 floats at a time | AVX2：一次将8个uint8_t转换为8个float
                        for (; j < simd_end; j += 8)
                        {
                            // Load 8 uint8_t values | 加载8个uint8_t值
                            __m128i u8_vals = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(src + j));
                            // Convert to int32 | 转换为int32
                            __m256i i32_vals = _mm256_cvtepu8_epi32(u8_vals);
                            // Convert to float | 转换为float
                            __m256 f32_vals = _mm256_cvtepi32_ps(i32_vals);
                            // Store result | 存储结果
                            _mm256_storeu_ps(dest + j, f32_vals);
                        }

                        // Handle remaining elements | 处理剩余元素
                        for (; j < descriptor_dim; ++j)
                        {
                            dest[j] = static_cast<float>(src[j]);
                        }
                    }
#else
                    // Fallback: element-wise conversion | 回退：逐元素转换
                    for (size_t i = 0; i < num_features; ++i)
                    {
                        const uint8_t *src = descriptors_cv.ptr<uint8_t>(i);
                        float *dest = descriptors_out[i];
                        for (size_t j = 0; j < descriptor_dim; ++j)
                        {
                            dest[j] = static_cast<float>(src[j]);
                        }
                    }
#endif
                }
                else
                {
                    // 🚀 FLOAT32 → FLOAT32: Single memcpy (zero-copy optimization) | FLOAT32 → FLOAT32：单次memcpy（零拷贝优化）
                    if (descriptors_cv.isContinuous())
                    {
                        // Best case: cv::Mat is contiguous, single memcpy for entire buffer | 最佳情况：cv::Mat连续，整个缓冲区单次memcpy
                        std::memcpy(descriptors_out.data(), descriptors_cv.ptr<float>(0), num_features * descriptor_dim * sizeof(float));
                    }
                    else
                    {
                        // Row-by-row memcpy | 逐行memcpy
                        for (size_t i = 0; i < num_features; ++i)
                        {
                            const float *src = descriptors_cv.ptr<float>(i);
                            float *dest = descriptors_out[i];
                            std::memcpy(dest, src, descriptor_dim * sizeof(float));
                        }
                    }
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVDescriptors2Descriptors转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVDescriptors2Descriptors: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::Descriptors2CVDescriptors(
            const Descriptors &descriptors,
            cv::Mat &descriptors_cv_out,
            const std::string &detector_type)
        {
            try
            {
                if (descriptors.empty())
                {
                    descriptors_cv_out.release();
                    return true;
                }

                const DescriptorType desc_type = ParseDescriptorType(detector_type);
                const size_t num_features = descriptors.size();
                const size_t descriptor_dim = descriptors.dim();

                if (desc_type == DescriptorType::UINT8)
                {
                    // FLOAT32 → UINT8 conversion with SIMD optimization | FLOAT32 → UINT8转换，使用SIMD优化
                    descriptors_cv_out.create(num_features, descriptor_dim, CV_8U);

#if defined(POSDK_SIMD_ENABLED)
                    for (size_t i = 0; i < num_features; ++i)
                    {
                        const float *src = descriptors[i];
                        uint8_t *dest = descriptors_cv_out.ptr<uint8_t>(i);

                        size_t j = 0;
                        const size_t simd_end = descriptor_dim - (descriptor_dim % 8);

                        // AVX2: convert 8 floats to 8 uint8_t at a time | AVX2：一次将8个float转换为8个uint8_t
                        for (; j < simd_end; j += 8)
                        {
                            // Load 8 floats | 加载8个float
                            __m256 f32_vals = _mm256_loadu_ps(src + j);
                            // Convert to int32 | 转换为int32
                            __m256i i32_vals = _mm256_cvtps_epi32(f32_vals);
                            // Pack int32 to int16 | 将int32打包为int16
                            __m128i i16_vals = _mm_packs_epi32(_mm256_castsi256_si128(i32_vals),
                                                               _mm256_extracti128_si256(i32_vals, 1));
                            // Pack int16 to uint8 | 将int16打包为uint8
                            __m128i u8_vals = _mm_packus_epi16(i16_vals, _mm_setzero_si128());
                            // Store result | 存储结果
                            _mm_storel_epi64(reinterpret_cast<__m128i *>(dest + j), u8_vals);
                        }

                        // Handle remaining elements | 处理剩余元素
                        for (; j < descriptor_dim; ++j)
                        {
                            dest[j] = static_cast<uint8_t>(src[j]);
                        }
                    }
#else
                    // Fallback: element-wise conversion | 回退：逐元素转换
                    for (size_t i = 0; i < num_features; ++i)
                    {
                        const float *src = descriptors[i];
                        uint8_t *dest = descriptors_cv_out.ptr<uint8_t>(i);
                        for (size_t j = 0; j < descriptor_dim; ++j)
                        {
                            dest[j] = static_cast<uint8_t>(src[j]);
                        }
                    }
#endif
                }
                else
                {
                    // 🚀 FLOAT32 → FLOAT32: Single memcpy (zero-copy optimization) | FLOAT32 → FLOAT32：单次memcpy（零拷贝优化）
                    descriptors_cv_out.create(num_features, descriptor_dim, CV_32F);

                    if (descriptors_cv_out.isContinuous())
                    {
                        // Best case: single memcpy for entire buffer | 最佳情况：整个缓冲区单次memcpy
                        std::memcpy(descriptors_cv_out.ptr<float>(0), descriptors.data(), num_features * descriptor_dim * sizeof(float));
                    }
                    else
                    {
                        // Row-by-row memcpy | 逐行memcpy
                        for (size_t i = 0; i < num_features; ++i)
                        {
                            const float *src = descriptors[i];
                            float *dest = descriptors_cv_out.ptr<float>(i);
                            std::memcpy(dest, src, descriptor_dim * sizeof(float));
                        }
                    }
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] Descriptors2CVDescriptors转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in Descriptors2CVDescriptors: " << e.what();
                return false;
            }
        }

        // ==================================================================================
        // Level 2: ImageFeatureInfo ↔ CV Features (Single Image Conversion)
        //          ImageFeatureInfo ↔ CV Features (单图像转换)
        // ==================================================================================

        bool OpenCVConverter::CVFeatures2ImageFeatureInfo(
            const std::vector<cv::KeyPoint> &keypoints,
            ImageFeatureInfo &image_features,
            const std::string &image_path)
        {
            try
            {
                image_features = ImageFeatureInfo(image_path);
                FeaturePoints &feature_points = image_features.GetFeaturePoints();

                // Use Level 1 batch converter | 使用Level 1批量转换器
                return CVKeyPoints2FeaturePoints(keypoints, feature_points);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVFeatures2ImageFeatureInfo转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVFeatures2ImageFeatureInfo: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
            const std::vector<cv::KeyPoint> &keypoints,
            const cv::Mat &descriptors_cv,
            ImageFeatureInfo &image_features,
            Descriptors &descriptors_out,
            const std::string &image_path,
            const std::string &detector_type)
        {
            try
            {
                // Convert features using Level 1 batch converter | 使用Level 1批量转换器转换特征
                if (!CVFeatures2ImageFeatureInfo(keypoints, image_features, image_path))
                {
                    return false;
                }

                // Convert descriptors using Level 1 batch converter | 使用Level 1批量转换器转换描述子
                return CVDescriptors2Descriptors(descriptors_cv, descriptors_out, detector_type);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVFeatures2ImageFeatureInfoWithDesc转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVFeatures2ImageFeatureInfoWithDesc: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::ImageFeatureInfo2CVFeatures(
            const ImageFeatureInfo &image_features,
            std::vector<cv::KeyPoint> &keypoints)
        {
            try
            {
                const FeaturePoints &feature_points = image_features.GetFeaturePoints();

                // Use Level 1 batch converter with zero-copy SOA access | 使用Level 1批量转换器和零拷贝SOA访问
                return FeaturePoints2CVKeyPoints(feature_points, keypoints);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] ImageFeatureInfo2CVFeatures转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in ImageFeatureInfo2CVFeatures: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::ImageFeatureInfo2CVFeaturesWithDesc(
            const ImageFeatureInfo &image_features,
            const Descriptors &descriptors,
            std::vector<cv::KeyPoint> &keypoints,
            cv::Mat &descriptors_out,
            const std::string &detector_type)
        {
            try
            {
                const size_t num_features = image_features.GetNumFeatures();

                // Handle descriptor count mismatch gracefully | 优雅处理描述子数量不匹配
                if (descriptors.size() != num_features)
                {
                    LOG_DEBUG_ZH << "[OpenCVConverter] 警告: 描述子数量 (" << descriptors.size()
                                 << ") 与特征点数量 (" << num_features << ") 不匹配，仅转换关键点";
                    LOG_DEBUG_EN << "[OpenCVConverter] Warning: Descriptor count (" << descriptors.size()
                                 << ") does not match feature count (" << num_features << "), converting keypoints only";
                    descriptors_out.release();
                    return ImageFeatureInfo2CVFeatures(image_features, keypoints);
                }

                // Convert features using Level 1 batch converter | 使用Level 1批量转换器转换特征
                if (!ImageFeatureInfo2CVFeatures(image_features, keypoints))
                {
                    return false;
                }

                // Convert descriptors using Level 1 batch converter | 使用Level 1批量转换器转换描述子
                return Descriptors2CVDescriptors(descriptors, descriptors_out, detector_type);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] ImageFeatureInfo2CVFeaturesWithDesc转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in ImageFeatureInfo2CVFeaturesWithDesc: " << e.what();
                return false;
            }
        }

        // ==================================================================================
        // Level 3: FeaturesInfo ↔ CV Features (Multiple Images Batch Conversion)
        //          FeaturesInfo ↔ CV Features (多图像批量转换)
        // ==================================================================================

        bool OpenCVConverter::CVFeatures2FeaturesInfo(
            const std::vector<cv::KeyPoint> &keypoints,
            FeaturesInfoPtr &features_info_ptr,
            const std::string &image_path)
        {
            try
            {
                if (!features_info_ptr)
                {
                    features_info_ptr = std::make_shared<FeaturesInfo>();
                }

                ImageFeatureInfo image_features;
                if (!CVFeatures2ImageFeatureInfo(keypoints, image_features, image_path))
                {
                    return false;
                }

                features_info_ptr->push_back(std::move(image_features));
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVFeatures2FeaturesInfo转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVFeatures2FeaturesInfo: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::CVFeatures2FeaturesInfoWithDesc(
            const std::vector<cv::KeyPoint> &keypoints,
            const cv::Mat &descriptors,
            FeaturesInfoPtr &features_info_ptr,
            Descriptors &descriptors_out,
            const std::string &image_path,
            const std::string &detector_type)
        {
            try
            {
                if (!features_info_ptr)
                {
                    features_info_ptr = std::make_shared<FeaturesInfo>();
                }

                ImageFeatureInfo image_features;
                if (!CVFeatures2ImageFeatureInfoWithDesc(keypoints, descriptors, image_features,
                                                         descriptors_out, image_path, detector_type))
                {
                    return false;
                }

                features_info_ptr->push_back(std::move(image_features));
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVFeatures2FeaturesInfoWithDesc转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVFeatures2FeaturesInfoWithDesc: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::FeaturesInfo2CVFeatures(
            const ImageFeatureInfo &image_features,
            std::vector<cv::KeyPoint> &keypoints)
        {
            // Alias for ImageFeatureInfo2CVFeatures | ImageFeatureInfo2CVFeatures的别名
            return ImageFeatureInfo2CVFeatures(image_features, keypoints);
        }

        bool OpenCVConverter::FeaturesInfo2CVFeaturesWithDesc(
            const ImageFeatureInfo &image_features,
            const Descriptors &descriptors,
            std::vector<cv::KeyPoint> &keypoints,
            cv::Mat &descriptors_out,
            const std::string &detector_type)
        {
            // Alias for ImageFeatureInfo2CVFeaturesWithDesc | ImageFeatureInfo2CVFeaturesWithDesc的别名
            return ImageFeatureInfo2CVFeaturesWithDesc(image_features, descriptors, keypoints,
                                                       descriptors_out, detector_type);
        }

        // ==================================================================================
        // Match Conversion Functions | 匹配转换函数
        // ==================================================================================

        void OpenCVConverter::CVDMatch2IdMatch(const cv::DMatch &cv_match, IdMatch &match)
        {
            match.i = cv_match.queryIdx;
            match.j = cv_match.trainIdx;
            match.is_inlier = false; // Default to false | 默认为false
        }

        bool OpenCVConverter::CVDMatch2IdMatches(const std::vector<cv::DMatch> &cv_matches,
                                                 IdMatches &matches)
        {
            try
            {
                matches.clear();
                matches.reserve(cv_matches.size());

                for (const auto &cv_match : cv_matches)
                {
                    IdMatch match;
                    CVDMatch2IdMatch(cv_match, match);
                    matches.push_back(match);
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVDMatch2IdMatches转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVDMatch2IdMatches: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::CVDMatch2Matches(const std::vector<cv::DMatch> &cv_matches,
                                               const IndexT view_id1,
                                               const IndexT view_id2,
                                               MatchesPtr &matches_ptr)
        {
            try
            {
                if (!matches_ptr)
                {
                    matches_ptr = std::make_shared<Matches>();
                }

                IdMatches id_matches;
                if (!CVDMatch2IdMatches(cv_matches, id_matches))
                {
                    return false;
                }

                ViewPair view_pair(view_id1, view_id2);
                (*matches_ptr)[view_pair] = std::move(id_matches);

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVDMatch2Matches转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVDMatch2Matches: " << e.what();
                return false;
            }
        }

        void OpenCVConverter::IdMatch2CVDMatch(const IdMatch &match, cv::DMatch &cv_match)
        {
            cv_match.queryIdx = match.i;
            cv_match.trainIdx = match.j;
            cv_match.distance = 0.0f; // Not used in PoSDK | PoSDK中不使用
        }

        bool OpenCVConverter::IdMatches2CVPoints(
            const IdMatches &matches,
            const FeaturesInfo &features_info,
            const CameraModels &camera_models,
            const ViewPair &view_pair,
            std::vector<cv::Point2f> &points1,
            std::vector<cv::Point2f> &points2)
        {
            try
            {
                points1.clear();
                points2.clear();
                points1.reserve(matches.size());
                points2.reserve(matches.size());

                // Check if feature information exists | 检查特征信息是否存在
                if (features_info.size() <= std::max(view_pair.first, view_pair.second))
                {
                    LOG_ERROR_ZH << "[OpenCVConverter] 特征信息大小 (" << features_info.size()
                                 << ") 对视图对 (" << view_pair.first << ", " << view_pair.second << ") 不足";
                    LOG_ERROR_EN << "[OpenCVConverter] Features info size (" << features_info.size()
                                 << ") insufficient for view pair (" << view_pair.first
                                 << ", " << view_pair.second << ")";
                    return false;
                }

                // Access ImageFeatureInfo using array indexing | 使用数组索引访问ImageFeatureInfo
                const ImageFeatureInfo *features1 = &features_info.at(view_pair.first);
                const ImageFeatureInfo *features2 = &features_info.at(view_pair.second);

                if (!features1 || !features2)
                {
                    LOG_ERROR_ZH << "[OpenCVConverter] 特征信息指针为空";
                    LOG_ERROR_EN << "[OpenCVConverter] Features info pointer is null";
                    return false;
                }

                const auto &feature_points1 = features1->GetFeaturePoints();
                const auto &feature_points2 = features2->GetFeaturePoints();

                for (const auto &match : matches)
                {
                    // Check index validity | 检查索引有效性
                    if (match.i >= feature_points1.size() || match.j >= feature_points2.size())
                    {
                        LOG_DEBUG_ZH << "[OpenCVConverter] 警告: 匹配中特征索引无效: (" << match.i << ", " << match.j << ")";
                        LOG_DEBUG_EN << "[OpenCVConverter] Warning: Invalid feature index in match: ("
                                     << match.i << ", " << match.j << ")";
                        continue;
                    }

                    // Get feature point coordinates | 获取特征点坐标
                    const Feature f1_coord = feature_points1.GetCoord(match.i);
                    const Feature f2_coord = feature_points2.GetCoord(match.j);

                    points1.emplace_back(static_cast<float>(f1_coord.x()), static_cast<float>(f1_coord.y()));
                    points2.emplace_back(static_cast<float>(f2_coord.x()), static_cast<float>(f2_coord.y()));
                }

                if (points1.size() != matches.size())
                {
                    LOG_DEBUG_ZH << "[OpenCVConverter] 警告: 由于无效索引，部分匹配被跳过。原始: "
                                 << matches.size() << ", 有效: " << points1.size();
                    LOG_DEBUG_EN << "[OpenCVConverter] Warning: Some matches were skipped due to invalid indices. "
                                 << "Original: " << matches.size() << ", Valid: " << points1.size();
                }

                return !points1.empty();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] IdMatches2CVPoints转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in IdMatches2CVPoints: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::MatchesDataPtr2CVPoints(
            const DataPtr &matches_data_ptr,
            const FeaturesInfo &features_info,
            const CameraModels &camera_models,
            const ViewPair &view_pair,
            std::vector<cv::Point2f> &points1,
            std::vector<cv::Point2f> &points2)
        {
            try
            {
                points1.clear();
                points2.clear();

                // Get match data from DataPtr | 从DataPtr获取匹配数据
                void *raw_data = matches_data_ptr->GetData();
                if (!raw_data)
                {
                    LOG_ERROR_ZH << "[OpenCVConverter] 错误: matches_data_ptr->GetData() 返回空指针";
                    LOG_ERROR_EN << "[OpenCVConverter] Error: matches_data_ptr->GetData() returned null";
                    return false;
                }

                // Convert raw pointer to IdMatches type | 将原始指针转换为IdMatches类型
                const IdMatches *matches_ptr = static_cast<const IdMatches *>(raw_data);
                if (!matches_ptr)
                {
                    LOG_ERROR_ZH << "[OpenCVConverter] 错误: 转换为IdMatches*失败";
                    LOG_ERROR_EN << "[OpenCVConverter] Error: Failed to cast to IdMatches*";
                    return false;
                }

                return IdMatches2CVPoints(*matches_ptr, features_info, camera_models, view_pair, points1, points2);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] MatchesDataPtr2CVPoints转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in MatchesDataPtr2CVPoints: " << e.what();
                return false;
            }
        }

        // ==================================================================================
        // Camera Model Conversion Functions | 相机模型转换函数
        // ==================================================================================

        bool OpenCVConverter::CVCalibration2CameraModel(const cv::Mat &camera_matrix,
                                                        const cv::Mat &dist_coeffs,
                                                        const cv::Size &image_size,
                                                        CameraModel &camera_model,
                                                        DistortionType distortion_type)
        {
            try
            {
                // Extract camera intrinsics from OpenCV matrix | 从OpenCV矩阵提取相机内参
                double fx = camera_matrix.at<double>(0, 0);
                double fy = camera_matrix.at<double>(1, 1);
                double cx = camera_matrix.at<double>(0, 2);
                double cy = camera_matrix.at<double>(1, 2);

                // Set basic camera intrinsics | 设置基本相机内参
                camera_model.SetCameraIntrinsics(fx, fy, cx, cy, image_size.width, image_size.height);

                // Process distortion coefficients | 处理畸变系数
                if (dist_coeffs.empty() || distortion_type == DistortionType::NO_DISTORTION)
                {
                    camera_model.SetDistortionParams(DistortionType::NO_DISTORTION, {}, {});
                    return true;
                }

                std::vector<double> radial_distortion;
                std::vector<double> tangential_distortion;

                // Parse distortion coefficients based on type | 根据类型解析畸变系数
                switch (distortion_type)
                {
                case DistortionType::RADIAL_K1:
                    if (dist_coeffs.total() >= 1)
                    {
                        radial_distortion = {dist_coeffs.at<double>(0)}; // k1
                    }
                    break;

                case DistortionType::RADIAL_K3:
                    if (dist_coeffs.total() >= 5)
                    {
                        radial_distortion = {
                            dist_coeffs.at<double>(0), // k1
                            dist_coeffs.at<double>(1), // k2
                            dist_coeffs.at<double>(4)  // k3
                        };
                    }
                    break;

                case DistortionType::BROWN_CONRADY:
                    if (dist_coeffs.total() >= 5)
                    {
                        radial_distortion = {
                            dist_coeffs.at<double>(0), // k1
                            dist_coeffs.at<double>(1), // k2
                            dist_coeffs.at<double>(4)  // k3
                        };
                        tangential_distortion = {
                            dist_coeffs.at<double>(2), // p1
                            dist_coeffs.at<double>(3)  // p2
                        };
                    }
                    break;

                default:
                    LOG_ERROR_ZH << "[OpenCVConverter] 不支持的畸变类型";
                    LOG_ERROR_EN << "[OpenCVConverter] Unsupported distortion type";
                    return false;
                }

                camera_model.SetDistortionParams(distortion_type, radial_distortion, tangential_distortion);
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CVCalibration2CameraModel转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CVCalibration2CameraModel: " << e.what();
                return false;
            }
        }

        bool OpenCVConverter::CameraModel2CVCalibration(const CameraModel &camera_model,
                                                        cv::Mat &camera_matrix,
                                                        cv::Mat &dist_coeffs)
        {
            try
            {
                // Get camera intrinsics | 获取相机内参
                const auto &intrinsics = camera_model.GetIntrinsics();

                // Create OpenCV camera matrix | 创建OpenCV相机矩阵
                camera_matrix = cv::Mat::eye(3, 3, CV_64F);
                camera_matrix.at<double>(0, 0) = intrinsics.GetFx();
                camera_matrix.at<double>(1, 1) = intrinsics.GetFy();
                camera_matrix.at<double>(0, 2) = intrinsics.GetCx();
                camera_matrix.at<double>(1, 2) = intrinsics.GetCy();

                // Create distortion coefficient matrix based on model type | 根据畸变模型类型创建畸变系数
                switch (intrinsics.GetDistortionType())
                {
                case DistortionType::NO_DISTORTION:
                    dist_coeffs = cv::Mat(); // Empty matrix for no distortion | 空矩阵表示无畸变
                    break;

                case DistortionType::RADIAL_K1:
                    if (intrinsics.GetRadialDistortion().size() != 1)
                    {
                        LOG_ERROR_ZH << "[OpenCVConverter] 错误: K1模型的径向畸变参数数量无效";
                        LOG_ERROR_EN << "[OpenCVConverter] Error: Invalid number of radial distortion parameters for K1 model";
                        return false;
                    }
                    dist_coeffs = cv::Mat::zeros(1, 5, CV_64F);
                    dist_coeffs.at<double>(0) = intrinsics.GetRadialDistortion()[0]; // k1
                    break;

                case DistortionType::RADIAL_K3:
                    if (intrinsics.GetRadialDistortion().size() != 3)
                    {
                        LOG_ERROR_ZH << "[OpenCVConverter] 错误: K3模型的径向畸变参数数量无效";
                        LOG_ERROR_EN << "[OpenCVConverter] Error: Invalid number of radial distortion parameters for K3 model";
                        return false;
                    }
                    dist_coeffs = cv::Mat::zeros(1, 5, CV_64F);
                    dist_coeffs.at<double>(0) = intrinsics.GetRadialDistortion()[0]; // k1
                    dist_coeffs.at<double>(1) = intrinsics.GetRadialDistortion()[1]; // k2
                    dist_coeffs.at<double>(4) = intrinsics.GetRadialDistortion()[2]; // k3
                    break;

                case DistortionType::BROWN_CONRADY:
                    if (intrinsics.GetRadialDistortion().size() != 3 ||
                        intrinsics.GetTangentialDistortion().size() != 2)
                    {
                        LOG_ERROR_ZH << "[OpenCVConverter] 错误: Brown-Conrady模型的畸变参数数量无效";
                        LOG_ERROR_EN << "[OpenCVConverter] Error: Invalid number of distortion parameters for Brown-Conrady model";
                        return false;
                    }
                    dist_coeffs = cv::Mat::zeros(1, 5, CV_64F);
                    dist_coeffs.at<double>(0) = intrinsics.GetRadialDistortion()[0];     // k1
                    dist_coeffs.at<double>(1) = intrinsics.GetRadialDistortion()[1];     // k2
                    dist_coeffs.at<double>(2) = intrinsics.GetTangentialDistortion()[0]; // p1
                    dist_coeffs.at<double>(3) = intrinsics.GetTangentialDistortion()[1]; // p2
                    dist_coeffs.at<double>(4) = intrinsics.GetRadialDistortion()[2];     // k3
                    break;

                default:
                    LOG_ERROR_ZH << "[OpenCVConverter] 错误: 不支持的畸变类型";
                    LOG_ERROR_EN << "[OpenCVConverter] Error: Unsupported distortion type";
                    return false;
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenCVConverter] CameraModel2CVCalibration转换失败: " << e.what();
                LOG_ERROR_EN << "[OpenCVConverter] Error in CameraModel2CVCalibration: " << e.what();
                return false;
            }
        }

        // ==================================================================================
        // Internal Helper Functions | 内部辅助函数
        // ==================================================================================

        void OpenCVConverter::CVKeyPoint2Feature(
            const cv::KeyPoint &kp,
            Feature &coord,
            float &size,
            float &angle)
        {
            coord = Feature(kp.pt.x, kp.pt.y);
            size = kp.size;
            angle = kp.angle;
        }

        void OpenCVConverter::Feature2CVKeyPoint(
            const FeaturePoints &feature_points,
            size_t feature_index,
            cv::KeyPoint &kp)
        {
            const Feature coord = feature_points.GetCoord(feature_index);
            kp.pt.x = static_cast<float>(coord.x());
            kp.pt.y = static_cast<float>(coord.y());
            kp.size = feature_points.GetSize(feature_index);
            kp.angle = feature_points.GetAngle(feature_index);
        }

    } // namespace Converter
} // namespace PoSDK
