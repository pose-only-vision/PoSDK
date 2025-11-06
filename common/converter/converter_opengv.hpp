/**
 * @file converter_opengv.hpp
 * @brief OpenGV Data Type Converter | OpenGV数据类型转换器
 * @details Provides conversion functionality between PoMVG and OpenGV data types
 *          提供PoMVG与OpenGV数据类型之间的转换功能
 *
 * @copyright Copyright (c) 2024 Qi Cai
 * Licensed under the Mozilla Public License Version 2.0
 */

#ifndef _CONVERTER_OPENGV_
#define _CONVERTER_OPENGV_

#include "converter_base.hpp"
#include <po_core/types.hpp>
#include <po_core/interfaces_robust_estimator.hpp>
#include <po_core/po_logger.hpp>
#include <opengv/types.hpp>
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <Eigen/Core>
#include <memory>

namespace PoSDK
{
    namespace Converter
    {

        class OpenGVConverter : public ConverterBase
        {
        public:
            /**
             * @brief Convert PoMVG matches and feature data to OpenGV bearing vectors
             *        将PoMVG匹配和特征数据转换为OpenGV的bearing vectors
             * @param matches Feature matches | 特征匹配
             * @param features_info Feature information | 特征信息
             * @param camera_models Camera model collection | 相机模型集合
             * @param view_pair View pair | 视图对
             * @param[out] bearingVectors1 Bearing vectors of the first frame | 第一帧的单位向量
             * @param[out] bearingVectors2 Bearing vectors of the second frame | 第二帧的单位向量
             * @return Whether the conversion was successful | 转换是否成功
             */
            static bool MatchesToBearingVectors(
                const IdMatches &matches,
                const FeaturesInfo &features_info,
                const CameraModels &camera_models,
                const ViewPair &view_pair,
                opengv::bearingVectors_t &bearingVectors1,
                opengv::bearingVectors_t &bearingVectors2);

            /**
             * @brief Convert PoMVG match samples to OpenGV bearing vectors
             *        将PoMVG匹配样本转换为OpenGV的bearing vectors
             * @param matches_sample Match samples | 匹配样本
             * @param features_info Feature information | 特征信息
             * @param camera_models Camera model collection | 相机模型集合
             * @param view_pair View pair | 视图对
             * @param[out] bearingVectors1 Bearing vectors of the first frame | 第一帧的单位向量
             * @param[out] bearingVectors2 Bearing vectors of the second frame | 第二帧的单位向量
             * @return Whether the conversion was successful | 转换是否成功
             */
            static bool MatchesToBearingVectors(
                const DataSample<IdMatches> &matches_sample,
                const FeaturesInfo &features_info,
                const CameraModels &camera_models,
                const ViewPair &view_pair,
                opengv::bearingVectors_t &bearingVectors1,
                opengv::bearingVectors_t &bearingVectors2);

            /**
             * @brief Convert OpenGV pose result to PoMVG relative pose
             *        OpenGV位姿结果转换为PoMVG相对位姿
             * @param T OpenGV transformation matrix | OpenGV的变换矩阵
             * @param[out] relative_pose PoMVG relative pose | PoMVG的相对位姿
             * @return Whether the conversion was successful | 转换是否成功
             */
            static bool OpenGVPose2RelativePose(
                const opengv::transformation_t &T,
                RelativePose &relative_pose);

            /**
             * @brief Convert PoMVG camera intrinsics to OpenGV camera parameters
             *        将PoMVG相机内参转换为OpenGV相机参数
             * @param camera_model PoMVG camera model | PoMVG相机模型
             * @param[out] K OpenGV camera intrinsic matrix | OpenGV相机内参矩阵
             * @return Whether the conversion was successful | 转换是否成功
             */
            static bool CameraModel2OpenGVCalibration(
                const CameraModel &camera_model,
                Eigen::Matrix3d &K);

            /**
             * @brief Convert pixel coordinates to bearing vector
             *        将像素坐标转换为单位向量
             * @param pixel_coord Pixel coordinates | 像素坐标
             * @param camera_model Camera model | 相机模型
             * @return Bearing vector | 单位向量
             */
            static opengv::bearingVector_t
            PixelToBearingVector(
                const Vector2d &pixel_coord,
                const CameraModel &camera_model);

            /**
             * @brief Check if pixel coordinates are within image bounds
             *        检查像素坐标是否在图像范围内
             */
            static bool IsPixelInImage(
                const Vector2d &pixel_coord,
                const CameraModel &camera_model);

            /**
             * @brief Convert BearingPairs to BearingVectors
             *        将BearingPairs转换为BearingVectors
             * @param bearing_pairs BearingPairs type data | BearingPairs类型数据
             * @param bearing_vectors1 Output BearingVectors1 | 输出BearingVectors1
             * @param bearing_vectors2 Output BearingVectors2 | 输出BearingVectors2
             */
            static void BearingPairsToBearingVectors(const BearingPairs &bearing_pairs, BearingVectors &bearing_vectors1, BearingVectors &bearing_vectors2);

            /**
             * @brief Convert BearingVectors to BearingPairs
             *        将BearingVectors转换为BearingPairs
             * @param bearing_vectors1 BearingVectors1 type data | BearingVectors1类型数据
             * @param bearing_vectors2 BearingVectors2 type data | BearingVectors2类型数据
             * @param bearing_pairs Output BearingPairs | 输出BearingPairs
             */
            static void BearingVectorsToBearingPairs(const BearingVectors &bearing_vectors1, const BearingVectors &bearing_vectors2, BearingPairs &bearing_pairs);
        };

    } // namespace Converter
} // namespace PoSDK

#endif // _CONVERTER_OPENGV_
