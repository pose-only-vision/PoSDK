#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <cfloat>


// ----------------- PatternSizeDetector -----------------
/**
 * @brief 检测标定板尺寸
 * @param image 输入图像
 * @param patternSize 标定板尺寸
 * @param centers 中心点
 * @param isCirclesGrid 是否为圆形网格
 * @return 是否检测到标定板
 * @note 参考: https://github.com/opencv/opencv/blob/master/samples/cpp/calibration.cpp   
 */
// --------------------------------------------------------------------------

class CirclesPatternDetector {
public:
    static bool detectPattern(const cv::Mat& image, cv::Size& patternSize) {
        cv::Mat gray;
        if(image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }

        // 1. 使用blob检测器找圆点
        std::vector<cv::KeyPoint> keypoints;
        auto blobDetector = createBlobDetector();
        blobDetector->detect(gray, keypoints);

        // 转换keypoints到centers
        std::vector<cv::Point2f> centers;
        centers.clear();
        for(const auto& kp : keypoints) {
            centers.push_back(kp.pt);
        }

        if(centers.size() < 9) { // 至少需要3x3的pattern
            return false;
        }

        // 2. 分析点的空间分布
        if(!analyzePointDistribution(centers, patternSize)) {
            return false;
        }

        // 3. 使用找到的尺寸进行精确检测
        std::vector<cv::Point2f> refined_centers;
        bool found = cv::findCirclesGrid(gray, patternSize, refined_centers,
            cv::CALIB_CB_SYMMETRIC_GRID + cv::CALIB_CB_CLUSTERING,
            blobDetector);

        if(found) {
            return true;
        }

        return false;
    }

private:
    static cv::Ptr<cv::SimpleBlobDetector> createBlobDetector() {
        cv::SimpleBlobDetector::Params params;
        
        // 改变阈值
        params.minThreshold = 10;
        params.maxThreshold = 220;
        params.thresholdStep = 10;
        
        // 根据面积过滤
        params.filterByArea = true;
        params.minArea = 20;
        params.maxArea = 5000;
        
        // 根据圆度过滤
        params.filterByCircularity = true;
        params.minCircularity = 0.8;
        
        // 根据凸度过滤
        params.filterByConvexity = true;
        params.minConvexity = 0.87;
        
        // 根据惯性比过滤
        params.filterByInertia = true;
        params.minInertiaRatio = 0.01;

        return cv::SimpleBlobDetector::create(params);
    }

    static bool analyzePointDistribution(const std::vector<cv::Point2f>& points, cv::Size& size) {
        if(points.empty()) return false;

        // 1. 计算最近邻距离
        std::vector<float> minDistances;
        for(size_t i = 0; i < points.size(); i++) {
            float minDist = FLT_MAX;
            for(size_t j = 0; j < points.size(); j++) {
                if(i != j) {
                    float dist = cv::norm(points[i] - points[j]);
                    minDist = std::min(minDist, dist);
                }
            }
            if(minDist < FLT_MAX) {
                minDistances.push_back(minDist);
            }
        }

        if(minDistances.empty()) return false;

        // 2. 计算平均最小距离
        float avgMinDist = 0;
        for(float dist : minDistances) {
            avgMinDist += dist;
        }
        avgMinDist /= minDistances.size();

        // 3. 对点进行聚类分析
        std::vector<float> x_coords, y_coords;
        for(const auto& p : points) {
            x_coords.push_back(p.x);
            y_coords.push_back(p.y);
        }
        std::sort(x_coords.begin(), x_coords.end());
        std::sort(y_coords.begin(), y_coords.end());

        // 4. 分析行列
        std::vector<int> x_clusters, y_clusters;
        float threshold = avgMinDist * 0.7f;  // 可调整的阈值

        // X方向聚类
        for(size_t i = 1; i < x_coords.size(); i++) {
            if(x_coords[i] - x_coords[i-1] > threshold) {
                x_clusters.push_back(i);
            }
        }
        x_clusters.push_back(x_coords.size());

        // Y方向聚类
        for(size_t i = 1; i < y_coords.size(); i++) {
            if(y_coords[i] - y_coords[i-1] > threshold) {
                y_clusters.push_back(i);
            }
        }
        y_clusters.push_back(y_coords.size());

        // 5. 确定行列数
        size.width = x_clusters.size();
        size.height = y_clusters.size();

        // 6. 验证结果
        if(size.width * size.height != points.size()) {
            // 如果检测到的行列数与点数不匹配，尝试其他组合
            int total_points = points.size();
            for(int w = 2; w <= sqrt(total_points * 2); w++) {
                if(total_points % w == 0) {
                    int h = total_points / w;
                    if(abs(w - h) < abs(size.width - size.height)) {
                        size.width = w;
                        size.height = h;
                    }
                }
            }
        }

        return (size.width >= 2 && size.height >= 2);
    }
};