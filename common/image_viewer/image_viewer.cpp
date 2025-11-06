#include "image_viewer.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <iomanip>

namespace common
{

    ImageViewer &ImageViewer::Instance()
    {
        static ImageViewer instance;
        return instance;
    }

    ImageViewer::ImageViewer()
    {
        // Initialize display options
        display_options_ = DisplayOptions();
    }

    ImageViewer::~ImageViewer()
    {
        CloseAllWindows();
    }

    void ImageViewer::ShowImage(const cv::Mat &image,
                                const std::vector<cv::KeyPoint> &keypoints,
                                const std::string &window_name)
    {
        auto &instance = Instance();
        instance.current_window_ = window_name; // Record current window
        instance.ShowImageImpl(image, keypoints, window_name);
    }

    void ImageViewer::WaitKey(int delay)
    {
        cv::waitKey(delay);
    }

    void ImageViewer::ShowImageImpl(const cv::Mat &image,
                                    const std::vector<cv::KeyPoint> &keypoints,
                                    const std::string &window_name)
    {
        CreateWindowIfNeeded(window_name);

        // Store current image and keypoints
        auto &window_info = windows_[window_name];
        window_info.current_image = image.clone();
        window_info.current_keypoints = keypoints;

        // Prepare display image
        cv::Mat display_image;
        if (image.channels() == 1)
        {
            cv::cvtColor(image, display_image, cv::COLOR_GRAY2BGR);
        }
        else
        {
            display_image = image.clone();
        }

        // Apply scaling
        if (display_options_.scale_factor != 1.0f)
        {
            cv::resize(display_image, display_image,
                       cv::Size(),
                       display_options_.scale_factor,
                       display_options_.scale_factor);
        }

        // Draw keypoints
        for (const auto &kp : keypoints)
        {
            cv::Point2f pt(kp.pt.x * display_options_.scale_factor,
                           kp.pt.y * display_options_.scale_factor);

            // Draw keypoint
            cv::circle(display_image, pt,
                       display_options_.keypoint_size,
                       display_options_.keypoint_color,
                       -1);

            // Draw orientation
            if (display_options_.show_orientation)
            {
                float angle = kp.angle * CV_PI / 180.0f;
                cv::Point2f dir(std::cos(angle), std::sin(angle));
                cv::line(display_image, pt,
                         pt + dir * (kp.size * 0.5f) * display_options_.scale_factor,
                         display_options_.keypoint_color,
                         1);
            }

            // Draw scale
            if (display_options_.show_scale)
            {
                cv::circle(display_image, pt,
                           kp.size * 0.5f * display_options_.scale_factor,
                           display_options_.keypoint_color,
                           1);
            }
        }

        cv::imshow(window_name, display_image);

        // Add window control
        if (display_options_.enable_window_control)
        {
            // Create control panel
            cv::createTrackbar("Scale", window_name, nullptr, 50, [](int pos, void *userdata)
                               {
                auto viewer = static_cast<ImageViewer*>(userdata);
                viewer->display_options_.scale_factor = pos / 100.0f;
                viewer->UpdateWindow(viewer->current_window_); }, this);
            // Set initial value
            cv::setTrackbarPos("Scale", window_name,
                               static_cast<int>(display_options_.scale_factor * 100));

            cv::createTrackbar("Point Size", window_name, nullptr, 20, [](int pos, void *userdata)
                               {
                auto viewer = static_cast<ImageViewer*>(userdata);
                viewer->display_options_.keypoint_size = pos;
                viewer->UpdateWindow(viewer->current_window_); }, this);
            // Set initial value
            cv::setTrackbarPos("Point Size", window_name,
                               display_options_.keypoint_size);
        }

        // Auto wait processing
        if (display_options_.auto_wait)
        {
            WaitKey(display_options_.wait_time);
        }
    }

    void ImageViewer::SetWindowProperty(const std::string &window_name,
                                        int property_id,
                                        double value)
    {
        cv::setWindowProperty(window_name, property_id, value);
    }

    void ImageViewer::ResizeWindow(const std::string &window_name,
                                   int width,
                                   int height)
    {
        cv::resizeWindow(window_name, width, height);
    }

    void ImageViewer::MoveWindow(const std::string &window_name,
                                 int x,
                                 int y)
    {
        cv::moveWindow(window_name, x, y);
    }

    void ImageViewer::SaveWindowImage(const std::string &window_name,
                                      const std::string &filename)
    {
        auto it = windows_.find(window_name);
        if (it != windows_.end() && !it->second.current_image.empty())
        {
            cv::imwrite(filename, it->second.current_image);
        }
    }

    void ImageViewer::CloseWindow(const std::string &window_name)
    {
        cv::destroyWindow(window_name);
        windows_.erase(window_name);
    }

    void ImageViewer::CloseAllWindows()
    {
        cv::destroyAllWindows();
        windows_.clear();
    }

    void ImageViewer::SetDisplayOptions(const DisplayOptions &options)
    {
        display_options_ = options;
        // Update all windows
        for (const auto &window : windows_)
        {
            if (window.second.is_visible)
            {
                ShowImageImpl(window.second.current_image,
                              window.second.current_keypoints,
                              window.first);
            }
        }
    }

    const ImageViewer::DisplayOptions &ImageViewer::GetDisplayOptions() const
    {
        return display_options_;
    }

    void ImageViewer::CreateWindowIfNeeded(const std::string &window_name)
    {
        auto it = windows_.find(window_name);
        if (it == windows_.end())
        {
            cv::namedWindow(window_name, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
            windows_[window_name].is_visible = true;
        }
    }

    void ImageViewer::UpdateWindow(const std::string &window_name)
    {
        auto it = windows_.find(window_name);
        if (it != windows_.end())
        {
            ShowImageImpl(it->second.current_image,
                          it->second.current_keypoints,
                          window_name);
        }
    }

    void ImageViewer::ShowMatches(const cv::Mat &img1, const cv::Mat &img2,
                                  const std::vector<cv::KeyPoint> &keypoints1,
                                  const std::vector<cv::KeyPoint> &keypoints2,
                                  const std::vector<cv::DMatch> &matches,
                                  const std::string &window_name)
    {
        // Create window and set initial size
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name, display_options_.initial_window_width,
                         display_options_.initial_window_height);

        // Calculate display image size
        double aspect_ratio = std::max(
            static_cast<double>(img1.rows) / img1.cols,
            static_cast<double>(img2.rows) / img2.cols);

        int display_width = display_options_.initial_window_width / 2;
        int display_height = static_cast<int>(display_width * aspect_ratio);

        // Resize images
        cv::Mat img1_resized, img2_resized;
        cv::resize(img1, img1_resized, cv::Size(display_width, display_height),
                   0, 0, cv::INTER_AREA);
        cv::resize(img2, img2_resized, cv::Size(display_width, display_height),
                   0, 0, cv::INTER_AREA);

        // Create display image
        int total_width = display_width * 2;
        if (display_options_.show_separator)
        {
            total_width += display_options_.separator_width;
        }

        cv::Mat display_image(display_height, total_width, CV_8UC3);

        // Copy images
        cv::Mat left_roi(display_image, cv::Rect(0, 0, display_width, display_height));
        cv::Mat right_roi(display_image, cv::Rect(
                                             display_width + (display_options_.show_separator ? display_options_.separator_width : 0),
                                             0, display_width, display_height));

        if (img1_resized.channels() == 1)
        {
            cv::cvtColor(img1_resized, left_roi, cv::COLOR_GRAY2BGR);
        }
        else
        {
            img1_resized.copyTo(left_roi);
        }

        if (img2_resized.channels() == 1)
        {
            cv::cvtColor(img2_resized, right_roi, cv::COLOR_GRAY2BGR);
        }
        else
        {
            img2_resized.copyTo(right_roi);
        }

        // Draw separator line
        if (display_options_.show_separator)
        {
            cv::line(display_image,
                     cv::Point(display_width, 0),
                     cv::Point(display_width, display_height),
                     display_options_.separator_color,
                     display_options_.separator_width);
        }

        // Calculate match quality range
        std::vector<float> distances;
        distances.reserve(matches.size());
        for (const auto &m : matches)
        {
            distances.push_back(m.distance);
        }

        float min_dist = *std::min_element(distances.begin(), distances.end());
        float max_dist = *std::max_element(distances.begin(), distances.end());
        float dist_range = max_dist - min_dist;

        // Create overlay for drawing match lines
        cv::Mat overlay = display_image.clone();

        // Draw match lines
        for (const auto &match : matches)
        {
            // Calculate scaled keypoint positions
            cv::Point2f pt1 = keypoints1[match.queryIdx].pt *
                              (static_cast<float>(display_width) / img1.cols);
            cv::Point2f pt2 = keypoints2[match.trainIdx].pt *
                              (static_cast<float>(display_width) / img2.cols);
            pt2.x += display_width + display_options_.separator_width;

            // Calculate match quality color
            cv::Scalar color;
            if (display_options_.use_quality_colormap)
            {
                float normalized_dist = 1.0f - (match.distance - min_dist) / dist_range;
                cv::Mat colormap_value(1, 1, CV_8UC3);
                cv::Mat in_value(1, 1, CV_8UC1);
                in_value.at<uchar>(0, 0) = static_cast<uchar>(normalized_dist * 255);
                cv::applyColorMap(in_value, colormap_value, display_options_.colormap_type);
                color = cv::Scalar(
                    colormap_value.at<cv::Vec3b>(0, 0)[0],
                    colormap_value.at<cv::Vec3b>(0, 0)[1],
                    colormap_value.at<cv::Vec3b>(0, 0)[2]);
            }
            else
            {
                color = display_options_.match_color;
            }

            // Draw line on overlay
            cv::line(overlay, pt1, pt2, color,
                     display_options_.line_thickness,
                     display_options_.use_antialiasing ? cv::LINE_AA : cv::LINE_8);
        }

        // Apply transparency
        cv::addWeighted(overlay, display_options_.line_transparency,
                        display_image, 1.0 - display_options_.line_transparency,
                        0, display_image);

        // Add color bar legend
        if (display_options_.use_quality_colormap)
        {
            int legend_height = 30;
            int legend_margin = 10;
            cv::Mat legend(legend_height, display_image.cols, CV_8UC3);

            for (int x = 0; x < legend.cols; ++x)
            {
                float normalized_x = static_cast<float>(x) / legend.cols;
                cv::Mat colormap_value(1, 1, CV_8UC3);
                cv::Mat in_value(1, 1, CV_8UC1);
                in_value.at<uchar>(0, 0) = static_cast<uchar>(normalized_x * 255);
                cv::applyColorMap(in_value, colormap_value, display_options_.colormap_type);
                cv::line(legend,
                         cv::Point(x, 0),
                         cv::Point(x, legend_height),
                         cv::Scalar(colormap_value.at<cv::Vec3b>(0, 0)),
                         1);
            }

            cv::Mat final_image(display_image.rows + legend_height + legend_margin,
                                display_image.cols, CV_8UC3, cv::Scalar(255, 255, 255));
            display_image.copyTo(final_image(cv::Rect(0, 0, display_image.cols, display_image.rows)));
            legend.copyTo(final_image(cv::Rect(0, display_image.rows + legend_margin,
                                               legend.cols, legend_height)));

            // Add text labels
            cv::putText(final_image, "Match Quality",
                        cv::Point(10, display_image.rows + legend_margin + legend_height - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            cv::putText(final_image, "Low",
                        cv::Point(10, display_image.rows + legend_margin + 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            cv::putText(final_image, "High",
                        cv::Point(final_image.cols - 50, display_image.rows + legend_margin + 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

            display_image = final_image;
        }

        // Calculate and add statistics to status bar
        std::stringstream status;
        status << "Images: " << img1.size() << " & " << img2.size()
               << " | Keypoints: " << keypoints1.size() << " & " << keypoints2.size()
               << " | Matches: " << matches.size();

        if (!matches.empty())
        {
            // Calculate match distance statistics
            std::vector<float> distances;
            distances.reserve(matches.size());
            for (const auto &m : matches)
            {
                distances.push_back(m.distance);
            }

            float min_dist = *std::min_element(distances.begin(), distances.end());
            float max_dist = *std::max_element(distances.begin(), distances.end());
            float avg_dist = std::accumulate(distances.begin(), distances.end(), 0.0f) / distances.size();

            status << " | Distances - Min: " << std::fixed << std::setprecision(2) << min_dist
                   << " Max: " << max_dist
                   << " Avg: " << avg_dist;
        }

        // Add status bar at bottom of image
        int status_height = 30;
        cv::Mat final_image(display_image.rows + status_height,
                            display_image.cols, CV_8UC3, cv::Scalar(240, 240, 240));

        // Copy main image
        display_image.copyTo(final_image(cv::Rect(0, 0, display_image.cols, display_image.rows)));

        // Add status information
        cv::putText(final_image, status.str(),
                    cv::Point(10, display_image.rows + status_height - 8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

        // Display result
        cv::imshow(window_name, final_image);

        // Save current display information
        windows_[window_name].current_image = final_image.clone();
        current_window_ = window_name;

        // Remove code related to creating control window
    }

    void ImageViewer::SaveMatchVisualization(
        const std::string &prefix,
        const std::vector<std::pair<int, int>> &image_pairs,
        const std::vector<cv::Mat> &images,
        const std::vector<std::vector<cv::KeyPoint>> &all_keypoints,
        const std::vector<std::vector<cv::DMatch>> &all_matches)
    {

        for (size_t i = 0; i < image_pairs.size(); ++i)
        {
            const auto &pair = image_pairs[i];
            const auto &matches = all_matches[i];

            // Create filename
            std::string filename = prefix + "_match_" +
                                   std::to_string(pair.first) + "_" +
                                   std::to_string(pair.second) + ".png";

            // Show matches
            ShowMatches(images[pair.first], images[pair.second],
                        all_keypoints[pair.first], all_keypoints[pair.second],
                        matches, "temp_window");

            // Save image
            SaveWindowImage("temp_window", filename);
        }

        // Clean up temporary window
        CloseWindow("temp_window");
    }

} // namespace common