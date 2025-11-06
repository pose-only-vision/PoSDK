/**
 * @file method_calibrator_plugin.hpp
 * @brief 相机标定插件
 * @details 支持针孔相机模型标定,使用OpenCV实现
 *
 * @copyright Copyright (c) 2024 Qi Cai
 */

#pragma once

#include <po_core.hpp>
#include <common/converter/converter_opencv.hpp>
#include <opencv2/core.hpp>
#include <string>
#include <po_core/po_logger.hpp>

namespace PoSDKPlugin
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace Converter;

    class MethodCalibratorPlugin : public MethodPresetProfiler
    {
    public:
        MethodCalibratorPlugin()
        {
            // 设置需要的输入数据包
            required_package_["data_images"] = nullptr;

            // 加载配置
            InitializeDefaultConfigPath();
        }

        virtual ~MethodCalibratorPlugin() = default;

        // 快速运行方式
        DataPtr RunFast();

        // 交互式运行方式
        DataPtr RunWithViewer();

        // 原始Run函数调用RunFast
        virtual DataPtr Run() override;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

    private:
        // 获取标定标志位
        int GetCalibrationFlags();

        /**
         * @brief 检测棋盘格角点
         * @param image 输入图像
         * @param pattern_type 标定板类型
         * @param pattern_size 标定板尺寸
         * @param corners 检测到的角点
         * @return 是否检测到角点
         */
        bool DetectChessboardCorners(const cv::Mat &image,
                                     const std::string &pattern_type,
                                     const cv::Size &pattern_size,
                                     std::vector<cv::Point2f> &corners);

        // 从图像路径中尝试识别相机信息
        bool DetectCameraInfo(const ImagePaths &image_paths,
                              std::string &make,
                              std::string &model,
                              std::string &serial);

        // 根据OpenCV标定标志位和相机类型选择合适的标定方法
        bool CalibrateCameraWithOpenCV(const std::vector<std::vector<cv::Point2f>> &imagePoints,
                                       const std::vector<std::vector<cv::Point3f>> &objectPoints,
                                       cv::Size imageSize,
                                       cv::Mat &cameraMatrix,
                                       cv::Mat &distCoeffs,
                                       CameraModelType model_type = CameraModelType::PINHOLE);

        // 验证标定结果
        bool ValidateCalibrationResults(const cv::Mat &cameraMatrix,
                                        const cv::Mat &distCoeffs,
                                        const cv::Size &imageSize,
                                        double rms);

        /**
         * @brief 从字符串获取畸变类型
         * @param distortion_model_str 畸变模型字符串
         * @return DistortionType 畸变类型枚举值
         */
        DistortionType GetDistortionType(const std::string &distortion_model_str);

    private:
        /**
         * @brief 保存检测到的角点图像
         * @param image 输入图像
         * @param corners 检测到的角点
         * @param pattern_size 标定板尺寸
         * @param found 是否检测到角点
         * @return 是否保存成功
         */
        bool SaveDebugImage(const cv::Mat &image,
                            const std::vector<cv::Point2f> &corners,
                            const cv::Size &pattern_size,
                            bool found);

        /**
         * @brief 生成标定板的标准3D点
         * @param board_width 标定板宽度（角点数）
         * @param board_height 标定板高度（角点数）
         * @param square_size 方格大小（毫米）
         * @param center_points 是否将点集中心化（默认false）
         * @return 标准3D点集
         */
        std::vector<cv::Point3f> GenerateStandardObjectPoints(
            int board_width,
            int board_height,
            float square_size,
            bool center_points = false);

        /**
         * @brief 创建并转换相机模型
         * @param cameraMatrix OpenCV相机内参矩阵
         * @param distCoeffs OpenCV畸变系数
         * @param imageSize 图像尺寸
         * @return 创建的相机模型数据指针，失败返回nullptr
         */
        DataPtr CreateCameraModel(
            const cv::Mat &cameraMatrix,
            const cv::Mat &distCoeffs,
            const cv::Size &imageSize);

    private:
        IndexT debug_image_count_{0};
    };

} // namespace PoSDKPlugin
