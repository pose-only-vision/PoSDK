#pragma once

#include <po_core/po_logger.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace common
{

    class ImageViewer
    {
    public:
        // Singleton access | 单例访问
        static ImageViewer &Instance();

        // Disable copy and assignment | 禁用拷贝和赋值
        ImageViewer(const ImageViewer &) = delete;
        ImageViewer &operator=(const ImageViewer &) = delete;

        // Keep original static interfaces unchanged | 原有的静态接口保持不变
        static void ShowImage(const cv::Mat &image,
                              const std::vector<cv::KeyPoint> &keypoints,
                              const std::string &window_name);
        static void WaitKey(int delay = 0);

        // New feature interfaces | 新增功能接口
        void SetWindowProperty(const std::string &window_name,
                               int property_id,
                               double value);

        void ResizeWindow(const std::string &window_name,
                          int width,
                          int height);

        void MoveWindow(const std::string &window_name,
                        int x,
                        int y);

        void SaveWindowImage(const std::string &window_name,
                             const std::string &filename);

        void CloseWindow(const std::string &window_name);
        void CloseAllWindows();

        // Display options configuration | 显示选项配置
        struct DisplayOptions
        {
            // Keypoint color | 特征点颜色
            cv::Scalar keypoint_color = cv::Scalar(0, 255, 0);
            // Keypoint size | 特征点大小
            int keypoint_size = 3;
            // Whether to show orientation | 是否显示方向
            bool show_orientation = true;
            // Whether to show scale | 是否显示尺度
            bool show_scale = true;
            // Display scale factor | 显示缩放因子
            float scale_factor = 1.0f;

            // New display control options | 新增显示控制选项
            // Whether to auto wait | 是否自动等待
            bool auto_wait = true;
            // Wait time in milliseconds, 0 means wait forever | 等待时间(ms)，0表示永久等待
            int wait_time = 0;
            // Whether to enable window control | 是否启用窗口控制
            bool enable_window_control = true;

            // Match line style | 匹配线条样式
            // Default yellow color | 默认黄色
            cv::Scalar match_color = cv::Scalar(0, 255, 255);
            // Line thickness | 线条宽度
            int match_thickness = 1;
            // Transparency | 透明度
            float match_alpha = 0.7f;
            // Whether to use random colors | 是否使用随机颜色
            bool use_random_colors = false;
            // Distance percentile lower bound | 距离百分比下限
            float min_distance_percentile = 0.0f;
            // Distance percentile upper bound | 距离百分比上限
            float max_distance_percentile = 1.0f;

            // Match display related options | 匹配显示相关选项
            // Initial window width | 初始窗口宽度
            int initial_window_width = 1600;
            // Initial window height | 初始窗口高度
            int initial_window_height = 800;
            // Whether to show separator | 是否显示分割线
            bool show_separator = true;
            // Separator width | 分割线宽度
            int separator_width = 2;
            // Separator color | 分割线颜色
            cv::Scalar separator_color = cv::Scalar(200, 200, 200);

            // Match line style | 匹配线条样式
            // Whether to use quality colormap | 是否使用质量颜色映射
            bool use_quality_colormap = true;
            // Colormap scheme | 颜色映射方案
            int colormap_type = cv::COLORMAP_JET;
            // Line transparency | 线条透明度
            float line_transparency = 0.7;
            // Line thickness | 线条粗细
            int line_thickness = 2;
            // Whether to use antialiasing | 是否使用抗锯齿
            bool use_antialiasing = true;
        };

        void SetDisplayOptions(const DisplayOptions &options);
        const DisplayOptions &GetDisplayOptions() const;

        // Add new display interface | 添加新的显示接口
        void ShowMatches(const cv::Mat &img1,
                         const cv::Mat &img2,
                         const std::vector<cv::KeyPoint> &keypoints1,
                         const std::vector<cv::KeyPoint> &keypoints2,
                         const std::vector<cv::DMatch> &matches,
                         const std::string &window_name);

        // Batch save interface | 批量保存接口
        void SaveMatchVisualization(const std::string &prefix,
                                    const std::vector<std::pair<int, int>> &image_pairs,
                                    const std::vector<cv::Mat> &images,
                                    const std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
                                    const std::vector<std::vector<cv::DMatch>> &all_matches);

    private:
        // Private constructor | 私有构造函数
        ImageViewer();
        ~ImageViewer();

        // Actual display implementation | 实际的显示实现
        void ShowImageImpl(const cv::Mat &image,
                           const std::vector<cv::KeyPoint> &keypoints,
                           const std::string &window_name);

        struct WindowInfo
        {
            cv::Mat current_image;
            std::vector<cv::KeyPoint> current_keypoints;
            bool is_visible;
        };

        std::unordered_map<std::string, WindowInfo> windows_;
        DisplayOptions display_options_;

        // Window management | 窗口管理
        void CreateWindowIfNeeded(const std::string &window_name);
        void UpdateWindow(const std::string &window_name);

        // Add current window name member | 添加当前窗口名称成员
        std::string current_window_;
    };

} // namespace common