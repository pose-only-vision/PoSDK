/**
 * @file method_calibrator_plugin.cpp
 * @brief Camera calibration plugin implementation | 相机标定插件实现
 */

#include "method_calibrator_plugin.hpp"
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <po_core/po_logger.hpp>
#include "CirclesPatternDetector.hpp"

namespace PoSDKPlugin
{

    // ----------------- MethodCalibratorPlugin -----------------

    DataPtr MethodCalibratorPlugin::Run()
    {
        DisplayConfigInfo();

        if (method_options_["run_mode"] == "viewer")
        {
            return RunWithViewer();
        }
        else
        {
            return RunFast();
        }
    }

    bool MethodCalibratorPlugin::SaveDebugImage(const cv::Mat &image,
                                                const std::vector<cv::Point2f> &corners,
                                                const cv::Size &pattern_size,
                                                bool found)
    {
        try
        {
            if (!GetOptionAsBool("save_debug_images", false))
            {
                return true;
            }

            // 1. Prepare debug image | 1. 准备调试图像
            cv::Mat debug_image;
            if (image.channels() == 1)
            {
                cv::cvtColor(image, debug_image, cv::COLOR_GRAY2BGR);
            }
            else
            {
                debug_image = image.clone();
            }

            // 2. Draw detected corners | 2. 绘制检测到的角点
            cv::drawChessboardCorners(debug_image, pattern_size, corners, found);

            // 3. Ensure debug directory exists | 3. 确保调试目录存在
            std::filesystem::path debug_dir(method_options_["debug_image_path"]);
            if (!std::filesystem::exists(debug_dir))
            {
                std::filesystem::create_directories(debug_dir);
            }

            // 4. Generate and save debug image | 4. 生成并保存调试图像
            std::string debug_path = (debug_dir /
                                      ("corners_" + std::to_string(debug_image_count_++) + ".jpg"))
                                         .string();

            if (!cv::imwrite(debug_path, debug_image))
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 保存调试图像失败: " << debug_path;
                LOG_ERROR_EN << "[MethodCalibrator] Failed to save debug image: " << debug_path;
                return false;
            }

            LOG_DEBUG_ZH << "[MethodCalibrator] 已保存调试图像: " << debug_path;
            LOG_DEBUG_EN << "[MethodCalibrator] Saved debug image: " << debug_path;
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 保存调试图像错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error saving debug image: " << e.what();
            return false;
        }
    }

    /**
     * @brief Detect chessboard corners | 检测棋盘格角点
     * @param image Input image | 输入图像
     * @param pattern_type Calibration pattern type | 标定板类型
     * @param pattern_size Calibration pattern size | 标定板尺寸
     * @param corners Detected corners | 检测到的角点
     * @return Whether corners were detected | 是否检测到角点
     */
    bool MethodCalibratorPlugin::DetectChessboardCorners(
        const cv::Mat &image,
        const std::string &pattern_type,
        const cv::Size &pattern_size,
        std::vector<cv::Point2f> &corners)
    {

        try
        {
            if (image.empty())
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 错误: 输入图像为空";
                LOG_ERROR_EN << "[MethodCalibrator] Error: Input image is empty";
                return false;
            }

            // 1. Preprocess image | 1. 预处理图像
            cv::Mat processed;
            if (image.channels() == 3)
            {
                cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
            }
            else
            {
                processed = image.clone();
            }
            cv::normalize(processed, processed, 0, 255, cv::NORM_MINMAX);

            // 2. Detect corners based on pattern type | 2. 根据不同标定板类型检测角点
            bool found = false;
            if (pattern_type == "chessboard")
            {
                int flags = cv::CALIB_CB_ADAPTIVE_THRESH +
                            cv::CALIB_CB_NORMALIZE_IMAGE +
                            cv::CALIB_CB_FAST_CHECK;
                found = cv::findChessboardCorners(processed, pattern_size, corners, flags);
            }
            else if (pattern_type == "circles")
            {
                int flags = cv::CALIB_CB_SYMMETRIC_GRID;
                // auto detector = cv::SimpleBlobDetector::create();
                found = cv::findCirclesGrid(processed, pattern_size, corners, flags);
            }
            else if (pattern_type == "acircles")
            {
                int flags = cv::CALIB_CB_ASYMMETRIC_GRID;
                auto detector = cv::SimpleBlobDetector::create();
                found = cv::findCirclesGrid(processed, pattern_size, corners, flags, detector);
            }
            else
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 不支持的标定板类型: " << pattern_type;
                LOG_ERROR_EN << "[MethodCalibrator] Unsupported pattern type: " << pattern_type;
                return false;
            }

            // 3. Post-processing if corners found | 3. 角点检测成功后的处理
            if (found)
            {
                // Subpixel refinement | 亚像素优化
                cv::cornerSubPix(processed, corners, pattern_size, cv::Size(-1, -1),
                                 cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

                // Validate corner count | 验证角点数量
                if (corners.size() != static_cast<size_t>(pattern_size.width * pattern_size.height))
                {
                    LOG_WARNING_ZH << "[MethodCalibrator] 警告: 角点数量不匹配. "
                                   << "预期: " << pattern_size.width * pattern_size.height
                                   << ", 实际: " << corners.size();
                    LOG_WARNING_EN << "[MethodCalibrator] Warning: Corner count mismatch. "
                                   << "Expected: " << pattern_size.width * pattern_size.height
                                   << ", Found: " << corners.size();
                }

                // Save debug image | 保存调试图像
                SaveDebugImage(processed, corners, pattern_size, found);
            }

            return found;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 角点检测错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error in corner detection: " << e.what();
            return false;
        }
    }

    std::vector<cv::Point3f> MethodCalibratorPlugin::GenerateStandardObjectPoints(
        int board_width,
        int board_height,
        float square_size,
        bool center_points)
    {

        try
        {
            std::vector<cv::Point3f> object_points;
            object_points.reserve(board_width * board_height);

            // Calculate center offset (if needed) | 计算中心偏移（如果需要）
            float offset_x = center_points ? (board_width - 1) * square_size / 2.0f : 0.0f;
            float offset_y = center_points ? (board_height - 1) * square_size / 2.0f : 0.0f;

            // Generate standard 3D points | 生成标准3D点
            for (int i = 0; i < board_height; i++)
            {
                for (int j = 0; j < board_width; j++)
                {
                    float x = j * square_size - offset_x; // square_size unit: mm | square_size 单位：毫米
                    float y = i * square_size - offset_y;
                    object_points.emplace_back(x, y, 0.0f);
                }
            }

            // Validate generated points count | 验证生成的点数量
            if (object_points.size() != static_cast<size_t>(board_width * board_height))
            {
                throw std::runtime_error("Generated points count mismatch");
            }

            return object_points;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 生成对象点错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error generating object points: " << e.what();
            return std::vector<cv::Point3f>();
        }
    }

    DataPtr MethodCalibratorPlugin::CreateCameraModel(
        const cv::Mat &cameraMatrix,
        const cv::Mat &distCoeffs,
        const cv::Size &imageSize)
    {

        try
        {
            // 1. Create camera model data | 1. 创建相机模型数据
            auto camera_model_data = FactoryData::Create("data_camera_models");
            if (!camera_model_data)
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 创建相机模型数据失败";
                LOG_ERROR_EN << "[MethodCalibrator] Failed to create camera model data";
                return nullptr;
            }

            // 2. Get camera models pointer | 2. 获取相机模型指针
            auto camera_models_ptr = GetDataPtr<CameraModels>(camera_model_data);
            if (!camera_models_ptr)
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 获取相机模型指针失败";
                LOG_ERROR_EN << "[MethodCalibrator] Failed to get camera models pointer";
                return nullptr;
            }

            // 3. Create and convert camera model | 3. 创建并转换相机模型
            CameraModel camera_model;
            if (!OpenCVConverter::CVCalibration2CameraModel(
                    cameraMatrix,
                    distCoeffs,
                    imageSize,
                    camera_model,
                    GetDistortionType(method_options_["distortion_model"])))
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 转换OpenCV标定失败";
                LOG_ERROR_EN << "[MethodCalibrator] Failed to convert OpenCV calibration";
                return nullptr;
            }

            // 4. Add to camera models list | 4. 添加到相机模型列表
            camera_models_ptr->push_back(std::move(camera_model));

            return camera_model_data;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 创建相机模型错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error creating camera model: " << e.what();
            return nullptr;
        }
    }

    DataPtr MethodCalibratorPlugin::RunFast()
    {
        try
        {
            // 1. Get image paths data | 1. 获取图像路径数据
            auto image_paths_ptr = GetDataPtr<ImagePaths>(required_package_["data_images"]);
            if (!image_paths_ptr || image_paths_ptr->empty())
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 无有效输入图像";
                LOG_ERROR_EN << "[MethodCalibrator] No valid input images";
                return nullptr;
            }

            // 2. Get calibration board parameters | 2. 获取标定板参数
            const std::string pattern_type = method_options_["pattern_type"];
            const IndexT board_width = GetOptionAsIndexT("board_width", 9);
            const IndexT board_height = GetOptionAsIndexT("board_height", 6);
            const float square_size = GetOptionAsFloat("square_size", 25.0f);

            // 3. Prepare calibration data | 3. 准备标定数据
            std::vector<std::vector<cv::Point2f>> imagePoints;
            std::vector<std::vector<cv::Point3f>> objectPoints;
            cv::Size imageSize;

            // 4. Detect calibration board size | 4. 检测标定板尺寸
            cv::Mat img = cv::imread(image_paths_ptr->front().first, cv::IMREAD_GRAYSCALE);
            cv::Size patternSize;

            if (pattern_type == "circles")
            {
                // For circles pattern, try to detect automatically | 对于圆点标定板，尝试自动检测
                if (CirclesPatternDetector::detectPattern(img, patternSize))
                {
                    LOG_DEBUG_ZH << "自动检测到圆点标定板尺寸: " << patternSize;
                    LOG_DEBUG_EN << "Automatically detected circles pattern size: " << patternSize;
                }
                else
                {
                    LOG_DEBUG_ZH << "无法自动检测圆点标定板尺寸，使用用户定义尺寸: " << board_width << "x" << board_height;
                    LOG_DEBUG_EN << "Could not auto-detect circles pattern size, using user-defined: " << board_width << "x" << board_height;
                    patternSize = cv::Size(board_width, board_height);
                }
            }
            else if (pattern_type == "chessboard")
            {
                // For chessboard pattern, use user-defined size directly | 对于棋盘格，直接使用用户定义尺寸
                // Note: OpenCV does not provide automatic chessboard size detection
                // 注意：OpenCV不提供自动检测棋盘格尺寸的功能
                patternSize = cv::Size(board_width, board_height);
                LOG_DEBUG_ZH << "使用用户定义的棋盘格尺寸: " << patternSize;
                LOG_DEBUG_EN << "Using user-defined chessboard size: " << patternSize;

                // Verify the pattern can be detected with this size | 验证该尺寸能否检测到棋盘格
                std::vector<cv::Point2f> test_corners;
                int flags = cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK;
                if (cv::findChessboardCorners(img, patternSize, test_corners, flags))
                {
                    LOG_DEBUG_ZH << "验证成功：在第一张图像中检测到棋盘格角点";
                    LOG_DEBUG_EN << "Verification successful: chessboard corners detected in first image";
                }
                else
                {
                    LOG_WARNING_ZH << "警告：在第一张图像中无法检测到指定尺寸的棋盘格，请检查 board_width 和 board_height 参数";
                    LOG_WARNING_EN << "Warning: Could not detect chessboard with specified size in first image, please check board_width and board_height parameters";
                }
            }
            else if (pattern_type == "acircles")
            {
                // For asymmetric circles, use user-defined size | 对于非对称圆点，使用用户定义尺寸
                patternSize = cv::Size(board_width, board_height);
                LOG_DEBUG_ZH << "使用用户定义的非对称圆点标定板尺寸: " << patternSize;
                LOG_DEBUG_EN << "Using user-defined asymmetric circles pattern size: " << patternSize;
            }
            else
            {
                // Unknown pattern type, use user-defined size | 未知标定板类型，使用用户定义尺寸
                LOG_WARNING_ZH << "未知标定板类型: " << pattern_type << "，使用用户定义尺寸";
                LOG_WARNING_EN << "Unknown pattern type: " << pattern_type << ", using user-defined size";
                patternSize = cv::Size(board_width, board_height);
            }

            // 5. Generate standard 3D points | 5. 生成标准3D点
            std::vector<cv::Point3f> standard_object_points =
                GenerateStandardObjectPoints(patternSize.width, patternSize.height, square_size);
            if (standard_object_points.empty())
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 生成标准对象点失败";
                LOG_ERROR_EN << "[MethodCalibrator] Failed to generate standard object points";
                return nullptr;
            }

            // 6. Process each image | 6. 处理每张图像
            const IndexT min_images = GetOptionAsIndexT("min_images", 3);
            const IndexT max_images = GetOptionAsIndexT("max_images", 100);
            IndexT valid_images = 0;

            for (const auto &[img_path, is_valid] : *image_paths_ptr)
            {
                if (!is_valid)
                    continue;

                cv::Mat image = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
                if (image.empty())
                {
                    LOG_ERROR_ZH << "[MethodCalibrator] 加载图像失败: " << img_path;
                    LOG_ERROR_EN << "[MethodCalibrator] Failed to load image: " << img_path;
                    continue;
                }
                LOG_DEBUG_ZH << "image_path = " << img_path;
                LOG_DEBUG_EN << "image_path = " << img_path;

                // Set image size | 设置图像尺寸
                if (imageSize.empty())
                {
                    imageSize = image.size();
                }

                // Detect corners | 检测角点
                std::vector<cv::Point2f> corners;

                if (DetectChessboardCorners(image, pattern_type, patternSize, corners))
                {
                    imagePoints.push_back(corners);
                    objectPoints.push_back(standard_object_points);
                    valid_images++;
                }

                if (valid_images >= max_images)
                    break;
            }

            // 6. Check valid images count | 6. 检查有效图像数量
            if (valid_images < min_images)
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 有效图像不足: "
                             << valid_images << "/" << min_images;
                LOG_ERROR_EN << "[MethodCalibrator] Not enough valid images: "
                             << valid_images << "/" << min_images;
                return nullptr;
            }

            // 7. Perform camera calibration | 7. 执行相机标定
            cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
            cv::Mat distCoeffs;

            if (!CalibrateCameraWithOpenCV(imagePoints, objectPoints, imageSize,
                                           cameraMatrix, distCoeffs,
                                           CameraModelType::PINHOLE))
            {
                return nullptr;
            }

            // 8. Create and return camera model | 8. 创建并返回相机模型
            return CreateCameraModel(cameraMatrix, distCoeffs, imageSize);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] RunFast错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error in RunFast: " << e.what();
            return nullptr;
        }
    }

    DataPtr MethodCalibratorPlugin::RunWithViewer()
    {

        return nullptr;
    }

    bool MethodCalibratorPlugin::DetectCameraInfo(const ImagePaths &image_paths,
                                                  std::string &make,
                                                  std::string &model,
                                                  std::string &serial)
    {
        try
        {
            // Try to read camera info from EXIF data | 尝试从EXIF数据中读取相机信息
            for (const auto &[path, valid] : image_paths)
            {
                if (!valid)
                    continue;

                cv::Mat img = cv::imread(path);
                if (img.empty())
                    continue;

                // TODO: Use exiv2 or other library to read EXIF data | TODO: 使用exiv2或其他库读取EXIF数据
                // If read successfully, set camera info and return true | 如果读取成功，设置相机信息并返回true

                // If unable to read from EXIF, try to parse from filename | 如果无法从EXIF读取，尝试从文件名解析
                std::filesystem::path img_path(path);
                std::string filename = img_path.stem().string();

                // Simple filename parsing example | 简单的文件名解析示例
                // Assume filename format: Make_Model_Serial_*.jpg | 假设文件名格式: Make_Model_Serial_*.jpg
                std::vector<std::string> tokens;
                std::stringstream ss(filename);
                std::string token;
                while (std::getline(ss, token, '_'))
                {
                    tokens.push_back(token);
                }

                if (tokens.size() >= 3)
                {
                    make = tokens[0];
                    model = tokens[1];
                    serial = tokens[2];
                    return true;
                }
            }

            LOG_WARNING_ZH << "[MethodCalibrator] 警告: 无法从图像中检测相机信息";
            LOG_WARNING_EN << "[MethodCalibrator] Warning: Could not detect camera info from images";
            return false;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 检测相机信息错误: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Error detecting camera info: " << e.what();
            return false;
        }
    }

    bool MethodCalibratorPlugin::CalibrateCameraWithOpenCV(
        const std::vector<std::vector<cv::Point2f>> &imagePoints,
        const std::vector<std::vector<cv::Point3f>> &objectPoints,
        cv::Size imageSize,
        cv::Mat &cameraMatrix,
        cv::Mat &distCoeffs,
        CameraModelType model_type)
    {

        try
        {
            // 1. Use initCameraMatrix2D for better initial values | 1. 使用initCameraMatrix2D获取更好的初始值
            cameraMatrix = cv::initCameraMatrix2D(objectPoints, imagePoints, imageSize, 1.0);

            // 2. Prepare calibration parameters | 2. 准备标定参数
            std::vector<cv::Mat> rvecs, tvecs;
            int flags = GetCalibrationFlags();

            // 3. Set iteration criteria | 3. 设置迭代条件
            IndexT max_iter = GetOptionAsIndexT("max_iter", 30);
            double eps = GetOptionAsFloat("eps", 1e-6);
            cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS,
                                      max_iter, eps);

            double rms = 0.0;

            // 4. Perform calibration based on camera model | 4. 根据不同相机模型执行标定
            switch (model_type)
            {
            case CameraModelType::PINHOLE:
                rms = cv::calibrateCamera(
                    objectPoints, imagePoints, imageSize,
                    cameraMatrix, distCoeffs, rvecs, tvecs,
                    flags, criteria);
                break;

            case CameraModelType::FISHEYE:
            {
                // Fisheye uses 4-parameter model [k1,k2,k3,k4] | 鱼眼相机使用4参数模型 [k1,k2,k3,k4]
                distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
                rms = cv::fisheye::calibrate(
                    objectPoints, imagePoints, imageSize,
                    cameraMatrix, distCoeffs, rvecs, tvecs,
                    flags | cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC,
                    criteria);
                break;
            }

            case CameraModelType::OMNIDIRECTIONAL:
            {
                LOG_ERROR_ZH << "[MethodCalibrator] OpenCV不支持全向相机模型标定";
                LOG_ERROR_EN << "[MethodCalibrator] Omnidirectional camera model calibration "
                             << "not directly supported by OpenCV";
                return false;
            }

            case CameraModelType::SPHERICAL:
                LOG_ERROR_ZH << "[MethodCalibrator] OpenCV不支持球面相机模型标定";
                LOG_ERROR_EN << "[MethodCalibrator] Spherical camera model calibration "
                             << "not directly supported by OpenCV";
                return false;

            default:
                LOG_ERROR_ZH << "[MethodCalibrator] 未知相机模型类型";
                LOG_ERROR_EN << "[MethodCalibrator] Unknown camera model type";
                return false;
            }

            // 5. Validate calibration results | 5. 验证标定结果
            if (!ValidateCalibrationResults(cameraMatrix, distCoeffs, imageSize, rms))
            {
                return false;
            }

            // 6. Output calibration info | 6. 输出标定信息
            LOG_INFO_ZH << "\n[MethodCalibrator] 标定结果:";
            LOG_INFO_ZH << "- 相机模型: " << static_cast<int>(model_type);
            LOG_INFO_ZH << "- RMS误差: " << rms;
            LOG_INFO_ZH << "- 相机矩阵:\n"
                        << cameraMatrix;
            LOG_INFO_ZH << "- 畸变系数: " << distCoeffs.t();
            LOG_INFO_EN << "\n[MethodCalibrator] Calibration results:";
            LOG_INFO_EN << "- Camera model: " << static_cast<int>(model_type);
            LOG_INFO_EN << "- RMS error: " << rms;
            LOG_INFO_EN << "- Camera matrix:\n"
                        << cameraMatrix;
            LOG_INFO_EN << "- Distortion coefficients: " << distCoeffs.t();

            // 7. Calculate reprojection error | 7. 计算重投影误差
            double max_error = 0;
            for (size_t i = 0; i < imagePoints.size(); ++i)
            {
                std::vector<cv::Point2f> projected_points;

                switch (model_type)
                {
                case CameraModelType::PINHOLE:
                    cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i],
                                      cameraMatrix, distCoeffs, projected_points);
                    break;

                case CameraModelType::FISHEYE:
                    cv::fisheye::projectPoints(objectPoints[i], projected_points,
                                               rvecs[i], tvecs[i], cameraMatrix,
                                               distCoeffs);
                    break;

                default:
                    continue;
                }

                double error = cv::norm(cv::Mat(imagePoints[i]),
                                        cv::Mat(projected_points), cv::NORM_L2);
                error /= projected_points.size();
                max_error = std::max(max_error, error);
            }
            LOG_INFO_ZH << "- 最大每视图误差: " << max_error;
            LOG_INFO_EN << "- Maximum per-view error: " << max_error;

            return rms < 1.0; // For ideal images, RMS should be small | 对于理想图像，RMS应该很小
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 标定失败: " << e.what();
            LOG_ERROR_EN << "[MethodCalibrator] Calibration failed: " << e.what();
            return false;
        }
    }

    // Modify function declaration | 修改函数声明
    bool MethodCalibratorPlugin::ValidateCalibrationResults(
        const cv::Mat &cameraMatrix,
        const cv::Mat &distCoeffs,
        const cv::Size &imageSize, // Add imageSize parameter | 添加 imageSize 参数
        double rms)
    {

        // 1. Check if focal length is reasonable | 1. 检查焦距是否合理
        double fx = cameraMatrix.at<double>(0, 0);
        double fy = cameraMatrix.at<double>(1, 1);
        if (fx <= 0 || fy <= 0)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 无效焦距";
            LOG_ERROR_EN << "[MethodCalibrator] Invalid focal length";
            return false;
        }

        // 2. Check if principal point is near image center | 2. 检查主点位置是否在图像中心附近
        double cx = cameraMatrix.at<double>(0, 2);
        double cy = cameraMatrix.at<double>(1, 2);
        if (std::abs(cx - imageSize.width / 2) > imageSize.width / 4 ||
            std::abs(cy - imageSize.height / 2) > imageSize.height / 4)
        {
            LOG_ERROR_ZH << "[MethodCalibrator] 主点距离图像中心太远";
            LOG_ERROR_EN << "[MethodCalibrator] Principal point too far from image center";
            return false;
        }

        // 3. Check if RMS error is within acceptable range | 3. 检查RMS误差是否在可接受范围内
        if (rms > 2.0)
        { // Pixel error threshold | 像素误差阈值
            LOG_WARNING_ZH << "[MethodCalibrator] 警告! RMS误差太大: " << rms;
            LOG_WARNING_EN << "[MethodCalibrator] warning! RMS error too large: " << rms;
            return true;
        }

        // 4. Check if distortion coefficients are within reasonable range | 4. 检查畸变系数是否在合理范围内
        if (!distCoeffs.empty())
        {
            double k1 = distCoeffs.at<double>(0);
            if (std::abs(k1) > 1.0)
            {
                LOG_ERROR_ZH << "[MethodCalibrator] 径向畸变系数k1太大";
                LOG_ERROR_EN << "[MethodCalibrator] Radial distortion coefficient k1 too large";
                return false;
            }
        }

        return true;
    }

    // ---------------------- Note (Do not remove) -----------------------------
    // About distortion model selection in OpenCV camera calibration:
    // 关于OpenCV相机标定中的畸变模型选择:
    // 1. Default 5-parameter model:
    //    - Parameters: [k1, k2, p1, p2, k3]
    //    - k1,k2,k3 radial distortion
    //    - p1,p2 tangential distortion
    //    - Suitable for most common camera calibration scenarios
    // 1. 默认5参数模型:
    //    - 使用参数: [k1, k2, p1, p2, k3]
    //    - k1,k2,k3为径向畸变系数
    //    - p1,p2为切向畸变系数
    //    - 适用于大多数普通相机标定场景
    //
    // 2. CALIB_RATIONAL_MODEL (8-parameter model):
    //    - Parameters: [k1, k2, p1, p2, k3, k4, k5, k6]
    //    - Adds k4,k5,k6 higher-order radial distortion
    //    - Suitable for cameras with larger distortion
    //    - Requires more calibration images for stable results
    // 2. CALIB_RATIONAL_MODEL (8参数模型):
    //    - 使用参数: [k1, k2, p1, p2, k3, k4, k5, k6]
    //    - 增加了k4,k5,k6三个高阶径向畸变系数
    //    - 适用于较大畸变的相机
    //    - 需要更多标定图片以获得稳定结果
    //
    // 3. Notes:
    //    - If 5-parameter model is accurate enough, do not use more complex models
    //    - Ensure sufficient calibration images when using CALIB_RATIONAL_MODEL
    //    - Combining with CALIB_THIN_PRISM_MODEL or CALIB_TILTED_MODEL will further increase the number of distortion coefficients
    // 3. 注意事项:
    //    - 如果5参数模型精度足够，不建议使用更复杂的模型
    //    - 使用CALIB_RATIONAL_MODEL时需确保有足够的标定图片
    //    - 与CALIB_THIN_PRISM_MODEL或CALIB_TILTED_MODEL组合使用时
    //      会进一步增加畸变系数的数量
    // ----------------------------------------------------------------

    // ---------------------- Distortion model configuration description ---------------------------
    // 1. radial_k1 model:
    //    - Uses only one radial distortion coefficient k1
    //    - Distortion coefficients: [k1, 0, 0, 0, 0]
    //    - Suitable for cameras with small distortion
    //    - Set flags: CALIB_FIX_K2 | CALIB_FIX_K3 | CALIB_ZERO_TANGENT_DIST
    // 1. radial_k1模型:
    //    - 只使用一个径向畸变系数k1
    //    - 畸变系数: [k1, 0, 0, 0, 0]
    //    - 适用于畸变较小的相机
    //    - 设置标志位: CALIB_FIX_K2 | CALIB_FIX_K3 | CALIB_ZERO_TANGENT_DIST
    //
    // 2. radial_k3 model:
    //    - Uses three radial distortion coefficients k1,k2,k3
    //    - Distortion coefficients: [k1, k2, 0, 0, k3]
    //    - Suitable for cameras with larger distortion
    //    - Set flags: CALIB_ZERO_TANGENT_DIST
    // 2. radial_k3模型:
    //    - 使用三个径向畸变系数k1,k2,k3
    //    - 畸变系数: [k1, k2, 0, 0, k3]
    //    - 适用于畸变较大的相机
    //    - 设置标志位: CALIB_ZERO_TANGENT_DIST
    //
    // 3. Full model:
    //    - Uses all 5 distortion coefficients
    //    - Distortion coefficients: [k1, k2, p1, p2, k3]
    //    - Suitable for scenarios requiring precise modeling
    //    - No special flags set
    // 3. 完整模型:
    //    - 使用全部5个畸变系数
    //    - 畸变系数: [k1, k2, p1, p2, k3]
    //    - 适用于需要精确建模的场景
    //    - 不设置特殊标志位
    // ----------------------------------------------------------------

    int MethodCalibratorPlugin::GetCalibrationFlags()
    {
        int flags = 0;
        const std::string &camera_model = method_options_["camera_model"];
        const std::string &dist_model = method_options_["distortion_model"];

        // 1. Set flags based on camera model | 1. 基于相机模型设置标志位
        if (camera_model == "fisheye")
        {
            flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
            flags |= cv::fisheye::CALIB_CHECK_COND;
            flags |= cv::fisheye::CALIB_FIX_SKEW;
        }
        else
        { // pinhole
            flags |= cv::CALIB_USE_INTRINSIC_GUESS;
        }

        // 2. Set flags based on distortion model | 2. 基于畸变模型设置标志位
        if (dist_model == "none")
        {
            flags |= cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3;
            flags |= cv::CALIB_ZERO_TANGENT_DIST;
        }
        else if (dist_model == "radial_k1")
        {
            flags |= cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3;
            flags |= cv::CALIB_ZERO_TANGENT_DIST;
        }
        else if (dist_model == "radial_k3")
        {
            flags |= cv::CALIB_ZERO_TANGENT_DIST; // Only fix tangential distortion | 只固定切向畸变
        }
        else if (dist_model == "brown_conrady")
        {
            flags |= cv::CALIB_RATIONAL_MODEL; // Use 8-parameter model | 使用8参数模型
        }

        // 3. Other optional flags | 3. 其他可选标志位
        if (GetOptionAsBool("fix_principal_point", false))
        {
            flags |= cv::CALIB_FIX_PRINCIPAL_POINT;
        }
        if (GetOptionAsBool("fix_aspect_ratio", false))
        {
            flags |= cv::CALIB_FIX_ASPECT_RATIO;
        }
        if (GetOptionAsBool("zero_tangent_dist", true))
        {
            flags |= cv::CALIB_ZERO_TANGENT_DIST;
        }

        return flags;
    }

    DistortionType MethodCalibratorPlugin::GetDistortionType(
        const std::string &distortion_model_str)
    {

        if (distortion_model_str == "none")
        {
            return DistortionType::NO_DISTORTION;
        }
        else if (distortion_model_str == "radial_k1")
        {
            return DistortionType::RADIAL_K1;
        }
        else if (distortion_model_str == "radial_k3")
        {
            return DistortionType::RADIAL_K3;
        }
        else if (distortion_model_str == "brown_conrady")
        {
            return DistortionType::BROWN_CONRADY;
        }

        // Default to full Brown-Conrady model | 默认使用完整的Brown-Conrady模型
        LOG_WARNING_ZH << "[MethodCalibrator] 警告: 未知畸变模型 '" << distortion_model_str
                       << "', 使用Brown-Conrady模型";
        LOG_WARNING_EN << "[MethodCalibrator] Warning: Unknown distortion model '" << distortion_model_str
                       << "', using Brown-Conrady model";
        return DistortionType::BROWN_CONRADY;
    }

}

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PoSDKPlugin::MethodCalibratorPlugin)
