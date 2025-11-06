#include "converter_opengv.hpp"
#include <po_core/po_logger.hpp>

namespace PoSDK
{
    namespace Converter
    {

        bool OpenGVConverter::MatchesToBearingVectors(
            const IdMatches &matches,
            const FeaturesInfo &features_info,
            const CameraModels &camera_models,
            const ViewPair &view_pair,
            opengv::bearingVectors_t &bearingVectors1,
            opengv::bearingVectors_t &bearingVectors2)
        {
            try
            {
                // Clear output vectors | 清空输出向量
                bearingVectors1.clear();
                bearingVectors2.clear();

                // Return success directly for empty matches | 如果是空匹配，直接返回成功
                if (matches.empty())
                {
                    return true;
                }

                // Get corresponding camera models | 获取对应的相机模型
                const CameraModel *camera1 = camera_models[view_pair.first];
                const CameraModel *camera2 = camera_models[view_pair.second];

                if (!camera1 || !camera2)
                {
                    LOG_ERROR_ZH << "[OpenGVConverter] 获取视图相机模型失败";
                    LOG_ERROR_EN << "[OpenGVConverter] Failed to get camera models for views";
                    return false;
                }

                // Pre-allocate space | 预分配空间
                bearingVectors1.reserve(matches.size());
                bearingVectors2.reserve(matches.size());

                // Convert each matched point | 转换每个匹配点
                for (const auto &match : matches)
                {
                    // Get feature point coordinates | 获取特征点坐标
                    const Vector2d &pt1 = features_info[view_pair.first]->GetFeaturePoints()[match.i].GetCoord();
                    const Vector2d &pt2 = features_info[view_pair.second]->GetFeaturePoints()[match.j].GetCoord();

                    // // Check if pixel coordinates are within image bounds | 检查像素坐标是否在图像范围内
                    // if (!IsPixelInImage(pt1, *camera1) || !IsPixelInImage(pt2, *camera2)) {
                    //     continue;
                    // }

                    // Convert to unit vectors | 转换为单位向量
                    bearingVectors1.push_back(PixelToBearingVector(pt1, *camera1));
                    bearingVectors2.push_back(PixelToBearingVector(pt2, *camera2));
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenGVConverter] 转换过程中出现错误: " << e.what();
                LOG_ERROR_EN << "[OpenGVConverter] Error in conversion: " << e.what();
                return false;
            }
        }
        bool OpenGVConverter::MatchesToBearingVectors(
            const DataSample<IdMatches> &matches_sample,
            const FeaturesInfo &features_info,
            const CameraModels &camera_models,
            const ViewPair &view_pair,
            opengv::bearingVectors_t &bearingVectors1,
            opengv::bearingVectors_t &bearingVectors2)
        {
            try
            {
                // Clear output vectors | 清空输出向量
                bearingVectors1.clear();
                bearingVectors2.clear();

                // Return success directly for empty sample | 如果是空样本，直接返回成功
                if (matches_sample.empty())
                {
                    return true;
                }

                // Get corresponding camera models | 获取对应的相机模型
                const CameraModel *camera1 = camera_models[view_pair.first];
                const CameraModel *camera2 = camera_models[view_pair.second];

                if (!camera1 || !camera2)
                {
                    LOG_ERROR_ZH << "[OpenGVConverter] 获取视图相机模型失败";
                    LOG_ERROR_EN << "[OpenGVConverter] Failed to get camera models for views";
                    return false;
                }

                // Pre-allocate space | 预分配空间
                bearingVectors1.reserve(matches_sample.size());
                bearingVectors2.reserve(matches_sample.size());

                // Directly use DataSample iterator to traverse matched points | 直接使用DataSample的迭代器遍历匹配点
                for (const auto &match : matches_sample)
                {
                    // Get feature point coordinates | 获取特征点坐标
                    const Vector2d &pt1 = features_info[view_pair.first]->GetFeaturePoints()[match.i].GetCoord();
                    const Vector2d &pt2 = features_info[view_pair.second]->GetFeaturePoints()[match.j].GetCoord();

                    // Convert to unit vectors | 转换为单位向量
                    bearingVectors1.push_back(PixelToBearingVector(pt1, *camera1));
                    bearingVectors2.push_back(PixelToBearingVector(pt2, *camera2));
                }

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenGVConverter] 转换过程中出现错误: " << e.what();
                LOG_ERROR_EN << "[OpenGVConverter] Error in conversion: " << e.what();
                return false;
            }
        }

        bool OpenGVConverter::OpenGVPose2RelativePose(
            const opengv::transformation_t &T,
            RelativePose &relative_pose)
        {
            try
            {
                // Use correct member variable names Rij and tij | 使用正确的成员变量名 Rij 和 tij
                relative_pose.SetRotation(T.block<3, 3>(0, 0));
                relative_pose.SetTranslation(T.block<3, 1>(0, 3));
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenGVConverter] OpenGVPose2RelativePose转换错误: " << e.what();
                LOG_ERROR_EN << "[OpenGVConverter] Error in OpenGVPose2RelativePose: " << e.what();
                return false;
            }
        }

        opengv::bearingVector_t
        OpenGVConverter::PixelToBearingVector(
            const Vector2d &pixel_coord,
            const CameraModel &camera_model)
        {
            // Manually calculate normalized coordinates | 手动计算归一化坐标
            const auto &intrinsics = camera_model.GetIntrinsics();

            // Calculate normalized coordinates (note direction) | 计算归一化坐标（注意方向）
            double x = (pixel_coord.x() - intrinsics.GetCx()) / intrinsics.GetFx();
            double y = (pixel_coord.y() - intrinsics.GetCy()) / intrinsics.GetFy();

            // Construct unit vector | 构造单位向量
            opengv::bearingVector_t bearing;
            bearing << x, y, 1.0;
            bearing.normalize();

            return bearing;
        }

        bool OpenGVConverter::CameraModel2OpenGVCalibration(
            const CameraModel &camera_model,
            Eigen::Matrix3d &K)
        {
            try
            {
                const auto &intrinsics = camera_model.GetIntrinsics();
                K << intrinsics.GetFx(), 0, intrinsics.GetCx(),
                    0, intrinsics.GetFy(), intrinsics.GetCy(),
                    0, 0, 1;
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR_ZH << "[OpenGVConverter] CameraModel2OpenGVCalibration转换错误: " << e.what();
                LOG_ERROR_EN << "[OpenGVConverter] Error in CameraModel2OpenGVCalibration: " << e.what();
                return false;
            }
        }

        /**
         * @brief Check if pixel coordinates are within image bounds
         * 检查像素坐标是否在图像范围内
         */
        bool OpenGVConverter::IsPixelInImage(
            const Vector2d &pixel_coord,
            const CameraModel &camera_model)
        {
            const auto &intrinsics = camera_model.GetIntrinsics();
            return pixel_coord.x() >= 0 && pixel_coord.x() < intrinsics.GetWidth() &&
                   pixel_coord.y() >= 0 && pixel_coord.y() < intrinsics.GetHeight();
        }

        /**
         * @brief Convert BearingPairs to BearingVectors
         * 将BearingPairs转换为BearingVectors
         * @param bearing_pairs BearingPairs type data | BearingPairs类型数据
         * @param bearing_vectors1 Output BearingVectors1 | 输出BearingVectors1
         * @param bearing_vectors2 Output BearingVectors2 | 输出BearingVectors2
         */
        void OpenGVConverter::BearingPairsToBearingVectors(
            const BearingPairs &bearing_pairs,
            BearingVectors &bearing_vectors1,
            BearingVectors &bearing_vectors2)
        {

            const size_t num_points = bearing_pairs.size();
            bearing_vectors1.resize(3, num_points);
            bearing_vectors2.resize(3, num_points);

            for (size_t i = 0; i < num_points; i++)
            {
                // Each bearing_pair is a 6x1 vector, first 3 are the first vector, last 3 are the second vector
                // 每个bearing_pair是6x1的向量,前3个是第一个向量,后3个是第二个向量
                bearing_vectors1.col(i) = bearing_pairs[i].head<3>();
                bearing_vectors2.col(i) = bearing_pairs[i].tail<3>();
            }
        }

        /**
         * @brief Convert BearingVectors to BearingPairs
         * 将BearingVectors转换为BearingPairs
         * @param bearing_vectors1 BearingVectors1 type data | BearingVectors1类型数据
         * @param bearing_vectors2 BearingVectors2 type data | BearingVectors2类型数据
         * @param bearing_pairs Output BearingPairs | 输出BearingPairs
         */
        void OpenGVConverter::BearingVectorsToBearingPairs(
            const BearingVectors &bearing_vectors1,
            const BearingVectors &bearing_vectors2,
            BearingPairs &bearing_pairs)
        {

            const size_t num_points = bearing_vectors1.cols();
            bearing_pairs.clear();
            bearing_pairs.reserve(num_points);

            for (size_t i = 0; i < num_points; i++)
            {
                Eigen::Matrix<double, 6, 1> match_pair;
                match_pair.head<3>() = bearing_vectors1.col(i);
                match_pair.tail<3>() = bearing_vectors2.col(i);
                bearing_pairs.push_back(match_pair);
            }
        }

    } // namespace Converter
} // namespace PoSDK
