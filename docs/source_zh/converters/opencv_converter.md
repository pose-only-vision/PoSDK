# OpenCV 数据转换器
(opencv-converter)=

OpenCV数据转换器提供PoSDK与OpenCV之间的数据类型转换，支持特征点、描述子、匹配和相机模型的双向转换。

---

## 转换架构

### 三层转换层次
(opencv-conversion-hierarchy)=

```
Level 1: FeaturePoints ↔ KeyPoints (SOA批量转换)
    ↓
Level 2: ImageFeatureInfo ↔ CV Features (单图像转换)
    ↓
Level 3: FeaturesInfo ↔ CV Features (多图像批量转换)
```

- **Level 1**: 底层数据转换
- **Level 2**: 单图像特征信息转换
- **Level 3**: 多图像批量转换

---

## 特征点转换
(opencv-feature-conversion)=

### `CVKeyPoints2FeaturePoints`
(cv-keypoints-to-featurepoints)=

批量转换OpenCV关键点为PoSDK FeaturePoints

**函数签名**:
```cpp
static bool CVKeyPoints2FeaturePoints(
    const std::vector<cv::KeyPoint> &keypoints,
    FeaturePoints &feature_points);
```

**参数说明**:
| 参数             | 类型                               | 说明                          |
| ---------------- | ---------------------------------- | ----------------------------- |
| `keypoints`      | `const std::vector<cv::KeyPoint>&` | [输入] OpenCV关键点集合       |
| `feature_points` | `FeaturePoints&`                   | [输出] PoSDK特征点（SOA存储） |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
#include "converter_opencv.hpp"
using namespace PoSDK::Converter;

// OpenCV检测关键点
std::vector<cv::KeyPoint> cv_keypoints;
detector->detect(image, cv_keypoints);

// 转换为PoSDK格式（高性能）
types::FeaturePoints feature_points;
bool success = OpenCVConverter::CVKeyPoints2FeaturePoints(
    cv_keypoints, feature_points);

if (success) {
    // 访问转换后的数据（SOA格式）
    size_t num = feature_points.GetSize();
    const auto& coords = feature_points.GetCoordsConst();
    const auto& sizes = feature_points.GetSizesConst();
    const auto& angles = feature_points.GetAnglesConst();
}
```

### `FeaturePoints2CVKeyPoints`
(featurepoints-to-cv-keypoints)=

批量转换PoSDK FeaturePoints为OpenCV关键点

**函数签名**:
```cpp
static bool FeaturePoints2CVKeyPoints(
    const FeaturePoints &feature_points,
    std::vector<cv::KeyPoint> &keypoints);
```

**参数说明**:
| 参数             | 类型                         | 说明                          |
| ---------------- | ---------------------------- | ----------------------------- |
| `feature_points` | `const FeaturePoints&`       | [输入] PoSDK特征点（SOA存储） |
| `keypoints`      | `std::vector<cv::KeyPoint>&` | [输出] OpenCV关键点集合       |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
// PoSDK特征点
types::FeaturePoints feature_points = GetFeaturePoints();

// 转换为OpenCV格式
std::vector<cv::KeyPoint> cv_keypoints;
bool success = OpenCVConverter::FeaturePoints2CVKeyPoints(
    feature_points, cv_keypoints);

// 用于OpenCV匹配等操作
cv::BFMatcher matcher;
std::vector<cv::DMatch> matches;
matcher->match(descriptors1, descriptors2, matches);
```

---

## 描述子转换
(opencv-descriptor-conversion)=

### `CVDescriptors2DescsSOA`
(cv-descriptors-to-descs-soa)=

批量转换OpenCV描述子为DescriptorsSOA

**函数签名**:
```cpp
static bool CVDescriptors2DescsSOA(
    const cv::Mat &descriptors_cv,
    DescsSOA &descriptors_soa_out,
    const std::string &detector_type = "SIFT");
```

**参数说明**:
| 参数                  | 类型                 | 说明                             |
| --------------------- | -------------------- | -------------------------------- |
| `descriptors_cv`      | `const cv::Mat&`     | [输入] OpenCV描述子矩阵 (N×D)    |
| `descriptors_soa_out` | `DescsSOA&`          | [输出] DescriptorsSOA（SOA格式） |
| `detector_type`       | `const std::string&` | [输入] 检测器类型（SIFT/ORB/等） |

**返回值**: `bool` - 转换是否成功

**支持的描述子类型**:
| 检测器 | OpenCV类型 | 维度   | PoSDK存储     |
| ------ | ---------- | ------ | ------------- |
| SIFT   | `CV_32F`   | 128    | `float` SOA   |
| SURF   | `CV_32F`   | 64/128 | `float` SOA   |
| ORB    | `CV_8U`    | 32     | `uint8_t` SOA |
| BRISK  | `CV_8U`    | 64     | `uint8_t` SOA |
| AKAZE  | `CV_8U`    | 61     | `uint8_t` SOA |

**使用示例**:
```cpp
// OpenCV特征提取
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
cv::Mat descriptors_cv;
detector->compute(image, keypoints, descriptors_cv);

// 转换为PoSDK SOA格式
types::DescsSOA descriptors_soa;
bool success = OpenCVConverter::CVDescriptors2DescsSOA(
    descriptors_cv, descriptors_soa, "SIFT");

if (success) {
    // SOA格式访问
    size_t num = descriptors_soa.GetSize();
    size_t dim = descriptors_soa.GetDim();
    const float* data = descriptors_soa.GetDataPtr();
    
    // data[i * dim + j] 访问第i个描述子的第j维
}
```

### `DescsSOA2CVDescriptors`
(descs-soa-to-cv-descriptors)=

批量转换DescriptorsSOA为OpenCV描述子

**函数签名**:
```cpp
static bool DescsSOA2CVDescriptors(
    const DescsSOA &descriptors_soa,
    cv::Mat &descriptors_cv_out,
    const std::string &detector_type = "SIFT");
```

**参数说明**:
| 参数                 | 类型                 | 说明                             |
| -------------------- | -------------------- | -------------------------------- |
| `descriptors_soa`    | `const DescsSOA&`    | [输入] DescriptorsSOA（SOA格式） |
| `descriptors_cv_out` | `cv::Mat&`           | [输出] OpenCV描述子矩阵          |
| `detector_type`      | `const std::string&` | [输入] 检测器类型                |

**返回值**: `bool` - 转换是否成功

**使用示例**:
```cpp
// PoSDK SOA描述子
types::DescsSOA descriptors_soa = GetDescriptorsSOA();

// 转换为OpenCV格式（用于匹配）
cv::Mat descriptors_cv;
bool success = OpenCVConverter::DescsSOA2CVDescriptors(
    descriptors_soa, descriptors_cv, "SIFT");

// OpenCV匹配
cv::BFMatcher matcher(cv::NORM_L2);
std::vector<cv::DMatch> matches;
matcher.match(descriptors_cv, descriptors_other, matches);
```

### 旧版AOS描述子转换（向后兼容）

#### `CVDescriptors2Descs`
批量转换OpenCV描述子为PoSDK Descs（AOS格式）

#### `Descs2CVDescriptors`
批量转换PoSDK Descs为OpenCV描述子（AOS格式）

---

## 图像特征信息转换
(opencv-image-feature-conversion)=

### `CVFeatures2ImageFeatureInfo`
(cv-features-to-image-feature-info)=

批量转换OpenCV特征为ImageFeatureInfo（不含描述子）

**函数签名**:
```cpp
static bool CVFeatures2ImageFeatureInfo(
    const std::vector<cv::KeyPoint> &keypoints,
    ImageFeatureInfo &image_features,
    const std::string &image_path = "");
```

**参数说明**:
| 参数             | 类型                               | 说明                |
| ---------------- | ---------------------------------- | ------------------- |
| `keypoints`      | `const std::vector<cv::KeyPoint>&` | [输入] OpenCV关键点 |
| `image_features` | `ImageFeatureInfo&`                | [输出] 图像特征信息 |
| `image_path`     | `const std::string&`               | [输入] 图像文件路径 |

**使用示例**:
```cpp
// 提取关键点
std::vector<cv::KeyPoint> keypoints;
detector->detect(image, keypoints);

// 转换为ImageFeatureInfo
types::ImageFeatureInfo image_features;
OpenCVConverter::CVFeatures2ImageFeatureInfo(
    keypoints, image_features, "image001.jpg");

// 访问特征信息
size_t num_features = image_features.GetNumFeatures();
const auto& feature_points = image_features.GetFeaturePointsConst();
```

### `CVFeatures2ImageFeatureInfoWithDesc`

批量转换OpenCV特征为ImageFeatureInfo（含描述子）

**函数签名**:
```cpp
static bool CVFeatures2ImageFeatureInfoWithDesc(
    const std::vector<cv::KeyPoint> &keypoints,
    const cv::Mat &descriptors_cv,
    ImageFeatureInfo &image_features,
    Descs &descriptors_out,
    const std::string &image_path = "",
    const std::string &detector_type = "SIFT");
```

**完整转换流程**:
```cpp
// 1. OpenCV特征提取
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

// 2. 转换为PoSDK格式
types::ImageFeatureInfo image_features;
types::Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs,
    "image001.jpg", "SIFT");

// 3. 添加到FeaturesInfo
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
features_info->push_back(image_features);
```

### `ImageFeatureInfo2CVFeatures`
(image-feature-info-to-cv-features)=

批量转换ImageFeatureInfo为OpenCV特征（不含描述子）

**函数签名**:
```cpp
static bool ImageFeatureInfo2CVFeatures(
    const ImageFeatureInfo &image_features,
    std::vector<cv::KeyPoint> &keypoints);
```

### `ImageFeatureInfo2CVFeaturesWithDesc`

批量转换ImageFeatureInfo为OpenCV特征（含描述子）

**函数签名**:
```cpp
static bool ImageFeatureInfo2CVFeaturesWithDesc(
    const ImageFeatureInfo &image_features,
    const Descs &descriptors,
    std::vector<cv::KeyPoint> &keypoints,
    cv::Mat &descriptors_out,
    const std::string &detector_type = "SIFT");
```

**错误处理**:
- 自动检测描述子数量不匹配
- 降级处理：仅转换关键点

---

## 批量特征转换

### `CVFeatures2FeaturesInfo`

批量转换OpenCV特征为FeaturesInfo（不含描述子）

**函数签名**:
```cpp
static bool CVFeatures2FeaturesInfo(
    const std::vector<cv::KeyPoint> &keypoints,
    FeaturesInfoPtr &features_info_ptr,
    const std::string &image_path);
```

### `CVFeatures2FeaturesInfoWithDesc`

批量转换OpenCV特征为FeaturesInfo（含描述子）

**函数签名**:
```cpp
static bool CVFeatures2FeaturesInfoWithDesc(
    const std::vector<cv::KeyPoint> &keypoints,
    const cv::Mat &descriptors,
    FeaturesInfoPtr &features_info_ptr,
    Descs &descriptors_out,
    const std::string &image_path,
    const std::string &detector_type = "SIFT");
```

**批量处理示例**:
```cpp
// 处理多张图像
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
types::Descs all_descriptors;

for (const auto& img_path : image_paths) {
    // 加载图像
    cv::Mat image = cv::imread(img_path);
    
    // 提取特征
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);
    
    // 添加到FeaturesInfo
    types::Descs descs;
    OpenCVConverter::CVFeatures2FeaturesInfoWithDesc(
        keypoints, descriptors, features_info, descs,
        img_path, "SIFT");
    
    // 合并描述子
    all_descriptors.insert(all_descriptors.end(), descs.begin(), descs.end());
}
```

---

## 匹配转换
(opencv-match-conversion)=

### `CVDMatch2IdMatch`

转换OpenCV DMatch到PoSDK IdMatch（单个匹配）

**函数签名**:
```cpp
static void CVDMatch2IdMatch(const cv::DMatch &cv_match, IdMatch &match);
```

### `CVDMatch2IdMatches`
(cv-dmatch-to-id-matches)=

转换OpenCV DMatch到PoSDK IdMatches（批量转换）

**函数签名**:
```cpp
static bool CVDMatch2IdMatches(const std::vector<cv::DMatch> &cv_matches,
                               IdMatches &matches);
```

**使用示例**:
```cpp
// OpenCV匹配
cv::BFMatcher matcher;
std::vector<cv::DMatch> cv_matches;
matcher.match(descriptors1, descriptors2, cv_matches);

// 转换为PoSDK格式
types::IdMatches id_matches;
OpenCVConverter::CVDMatch2IdMatches(cv_matches, id_matches);
```

### `CVDMatch2Matches`

转换OpenCV DMatch到PoSDK Matches（带视图对）

**函数签名**:
```cpp
static bool CVDMatch2Matches(const std::vector<cv::DMatch> &cv_matches,
                             const IndexT view_id1,
                             const IndexT view_id2,
                             MatchesPtr &matches_ptr);
```

### `IdMatch2CVDMatch`

转换PoSDK IdMatch到OpenCV DMatch（单个匹配）

### `IdMatches2CVPoints`
(id-matches-to-cv-points)=

将PoSDK IdMatches转换为OpenCV点对

**函数签名**:
```cpp
static bool IdMatches2CVPoints(
    const IdMatches &matches,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    std::vector<cv::Point2f> &points1,
    std::vector<cv::Point2f> &points2);
```

**使用场景**:
- 用于OpenCV的`findEssentialMat`
- 用于OpenCV的`findFundamentalMat`
- 用于OpenCV的`recoverPose`

**使用示例**:
```cpp
// PoSDK匹配数据
types::IdMatches matches;
types::FeaturesInfo features_info;
types::CameraModels camera_models;
types::ViewPair view_pair(0, 1);

// 转换为OpenCV点对
std::vector<cv::Point2f> points1, points2;
OpenCVConverter::IdMatches2CVPoints(
    matches, features_info, camera_models, view_pair,
    points1, points2);

// 使用OpenCV计算本质矩阵
cv::Mat E = cv::findEssentialMat(points1, points2, focal_length, 
                                  principal_point, cv::RANSAC);
```

### `MatchesDataPtr2CVPoints`

将PoSDK匹配DataPtr转换为OpenCV点对

**函数签名**:
```cpp
static bool MatchesDataPtr2CVPoints(
    const DataPtr &matches_data_ptr,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    std::vector<cv::Point2f> &points1,
    std::vector<cv::Point2f> &points2);
```

---

## 相机模型转换
(opencv-camera-conversion)=

### `CVCalibration2CameraModel`
(cv-calibration-to-camera-model)=

转换OpenCV相机标定到PoSDK CameraModel

**函数签名**:
```cpp
static bool CVCalibration2CameraModel(
    const cv::Mat &camera_matrix,
    const cv::Mat &dist_coeffs,
    const cv::Size &image_size,
    CameraModel &camera_model,
    DistortionType distortion_type = DistortionType::BROWN_CONRADY);
```

**参数说明**:
| 参数              | 类型              | 说明                        |
| ----------------- | ----------------- | --------------------------- |
| `camera_matrix`   | `const cv::Mat&`  | [输入] OpenCV内参矩阵 (3×3) |
| `dist_coeffs`     | `const cv::Mat&`  | [输入] OpenCV畸变系数       |
| `image_size`      | `const cv::Size&` | [输入] 图像尺寸             |
| `camera_model`    | `CameraModel&`    | [输出] PoSDK相机模型        |
| `distortion_type` | `DistortionType`  | [输入] 畸变类型             |

**使用示例**:
```cpp
// OpenCV相机标定结果
cv::Mat camera_matrix, dist_coeffs;
cv::Size image_size(640, 480);
cv::calibrateCamera(object_points, image_points, image_size,
                    camera_matrix, dist_coeffs, rvecs, tvecs);

// 转换为PoSDK格式
types::CameraModel camera_model;
OpenCVConverter::CVCalibration2CameraModel(
    camera_matrix, dist_coeffs, image_size, camera_model,
    types::DistortionType::BROWN_CONRADY);

// 使用PoSDK相机模型
Vector2d normalized = camera_model.PixelToNormalized(pixel);
```

### `CameraModel2CVCalibration`
(camera-model-to-cv-calibration)=

转换PoSDK CameraModel到OpenCV相机标定

**函数签名**:
```cpp
static bool CameraModel2CVCalibration(const CameraModel &camera_model,
                                      cv::Mat &camera_matrix,
                                      cv::Mat &dist_coeffs);
```

**使用示例**:
```cpp
// PoSDK相机模型
types::CameraModel camera_model = GetCameraModel();

// 转换为OpenCV格式
cv::Mat camera_matrix, dist_coeffs;
OpenCVConverter::CameraModel2CVCalibration(
    camera_model, camera_matrix, dist_coeffs);

// 使用OpenCV函数
cv::undistort(image_distorted, image_undistorted, 
              camera_matrix, dist_coeffs);
```

---

---

## 错误处理和调试

### 类型检查
```cpp
if (descriptors_cv.type() != CV_32F) {
    LOG_ERROR_ZH << "描述子类型错误，期望CV_32F";
    return false;
}
```

### 维度验证
```cpp
if (descriptors_cv.rows != keypoints.size()) {
    LOG_ERROR_ZH << "描述子数量(" << descriptors_cv.rows 
                 << ")与关键点数量(" << keypoints.size() << ")不匹配";
    return false;
}
```

### 空指针检查
```cpp
const CameraModel* camera = camera_models[view_id];
if (!camera) {
    LOG_ERROR_ZH << "无效的相机模型，view_id=" << view_id;
    return false;
}
```

---

## 完整工作流示例

### OpenCV特征提取 + PoSDK处理
```cpp
#include <opencv2/features2d.hpp>
#include "converter_opencv.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// 1. OpenCV特征提取
cv::Mat image = cv::imread("image.jpg");
cv::Ptr<cv::SIFT> detector = cv::SIFT::create(2000);

std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

LOG_INFO_ZH << "检测到 " << keypoints.size() << " 个特征点";

// 2. 转换为PoSDK格式
types::ImageFeatureInfo image_features;
types::Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs,
    "image.jpg", "SIFT");

// 3. 使用PoSDK处理（例如：鲁棒匹配）
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
features_info->push_back(image_features);

// 4. 转换回OpenCV（如需要）
std::vector<cv::KeyPoint> cv_keypoints_back;
cv::Mat cv_descriptors_back;
OpenCVConverter::ImageFeatureInfo2CVFeaturesWithDesc(
    image_features, descs, cv_keypoints_back, cv_descriptors_back, "SIFT");

// 5. 使用OpenCV可视化
cv::Mat output;
cv::drawKeypoints(image, cv_keypoints_back, output);
cv::imshow("Features", output);
```

---

**相关链接**:
- [核心数据类型 - FeaturePoints](../appendices/appendix_a_types.md#feature-point)
- [核心数据类型 - ImageFeatureInfo](../appendices/appendix_a_types.md#image-feature-info)
- [核心数据类型 - CameraModel](../appendices/appendix_a_types.md#camera-model)
- [转换器总览](index.md)



