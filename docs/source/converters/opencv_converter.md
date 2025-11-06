# OpenCV Data Converter
(opencv-converter)=

The OpenCV data converter provides data type conversion between PoSDK and OpenCV, supporting bidirectional conversion of feature points, descriptors, matches, and camera models.

---

## Conversion Architecture

### Three-Level Conversion Hierarchy
(opencv-conversion-hierarchy)=

```
Level 1: FeaturePoints ↔ KeyPoints (SOA batch conversion)
    ↓
Level 2: ImageFeatureInfo ↔ CV Features (single image conversion)
    ↓
Level 3: FeaturesInfo ↔ CV Features (multi-image batch conversion)
```

- **Level 1**: Low-level data conversion
- **Level 2**: Single image feature information conversion
- **Level 3**: Multi-image batch conversion

---

## Feature Point Conversion
(opencv-feature-conversion)=

### `CVKeyPoints2FeaturePoints`
(cv-keypoints-to-featurepoints)=

Batch convert OpenCV keypoints to PoSDK FeaturePoints

**Function Signature**:
```cpp
static bool CVKeyPoints2FeaturePoints(
    const std::vector<cv::KeyPoint> &keypoints,
    FeaturePoints &feature_points);
```

**Parameter Description**:
| Parameter        | Type                               | Description                                 |
| ---------------- | ---------------------------------- | ------------------------------------------- |
| `keypoints`      | `const std::vector<cv::KeyPoint>&` | [Input] OpenCV keypoint collection          |
| `feature_points` | `FeaturePoints&`                   | [Output] PoSDK feature points (SOA storage) |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
#include "converter_opencv.hpp"
using namespace PoSDK::Converter;

// OpenCV keypoint detection
std::vector<cv::KeyPoint> cv_keypoints;
detector->detect(image, cv_keypoints);

// Convert to PoSDK format (high performance)
types::FeaturePoints feature_points;
bool success = OpenCVConverter::CVKeyPoints2FeaturePoints(
    cv_keypoints, feature_points);

if (success) {
    // Access converted data (SOA format)
    size_t num = feature_points.GetSize();
    const auto& coords = feature_points.GetCoordsConst();
    const auto& sizes = feature_points.GetSizesConst();
    const auto& angles = feature_points.GetAnglesConst();
}
```

### `FeaturePoints2CVKeyPoints`
(featurepoints-to-cv-keypoints)=

Batch convert PoSDK FeaturePoints to OpenCV keypoints

**Function Signature**:
```cpp
static bool FeaturePoints2CVKeyPoints(
    const FeaturePoints &feature_points,
    std::vector<cv::KeyPoint> &keypoints);
```

**Parameter Description**:
| Parameter        | Type                         | Description                                |
| ---------------- | ---------------------------- | ------------------------------------------ |
| `feature_points` | `const FeaturePoints&`       | [Input] PoSDK feature points (SOA storage) |
| `keypoints`      | `std::vector<cv::KeyPoint>&` | [Output] OpenCV keypoint collection        |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
// PoSDK feature points
types::FeaturePoints feature_points = GetFeaturePoints();

// Convert to OpenCV format
std::vector<cv::KeyPoint> cv_keypoints;
bool success = OpenCVConverter::FeaturePoints2CVKeyPoints(
    feature_points, cv_keypoints);

// Use for OpenCV matching, etc.
cv::BFMatcher matcher;
std::vector<cv::DMatch> matches;
matcher->match(descriptors1, descriptors2, matches);
```

---

## Descriptor Conversion
(opencv-descriptor-conversion)=

### `CVDescriptors2DescsSOA`
(cv-descriptors-to-descs-soa)=

Batch convert OpenCV descriptors to DescriptorsSOA

**Function Signature**:
```cpp
static bool CVDescriptors2DescsSOA(
    const cv::Mat &descriptors_cv,
    DescsSOA &descriptors_soa_out,
    const std::string &detector_type = "SIFT");
```

**Parameter Description**:
| Parameter             | Type                 | Description                            |
| --------------------- | -------------------- | -------------------------------------- |
| `descriptors_cv`      | `const cv::Mat&`     | [Input] OpenCV descriptor matrix (N×D) |
| `descriptors_soa_out` | `DescsSOA&`          | [Output] DescriptorsSOA (SOA format)   |
| `detector_type`       | `const std::string&` | [Input] Detector type (SIFT/ORB/etc.)  |

**Return Value**: `bool` - Whether conversion succeeded

**Supported Descriptor Types**:
| Detector | OpenCV Type | Dimension | PoSDK Storage |
| -------- | ----------- | --------- | ------------- |
| SIFT     | `CV_32F`    | 128       | `float` SOA   |
| SURF     | `CV_32F`    | 64/128    | `float` SOA   |
| ORB      | `CV_8U`     | 32        | `uint8_t` SOA |
| BRISK    | `CV_8U`     | 64        | `uint8_t` SOA |
| AKAZE    | `CV_8U`     | 61        | `uint8_t` SOA |

**Usage Example**:
```cpp
// OpenCV feature extraction
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
cv::Mat descriptors_cv;
detector->compute(image, keypoints, descriptors_cv);

// Convert to PoSDK SOA format
types::DescsSOA descriptors_soa;
bool success = OpenCVConverter::CVDescriptors2DescsSOA(
    descriptors_cv, descriptors_soa, "SIFT");

if (success) {
    // SOA format access
    size_t num = descriptors_soa.GetSize();
    size_t dim = descriptors_soa.GetDim();
    const float* data = descriptors_soa.GetDataPtr();
    
    // data[i * dim + j] accesses j-th dimension of i-th descriptor
}
```

### `DescsSOA2CVDescriptors`
(descs-soa-to-cv-descriptors)=

Batch convert DescriptorsSOA to OpenCV descriptors

**Function Signature**:
```cpp
static bool DescsSOA2CVDescriptors(
    const DescsSOA &descriptors_soa,
    cv::Mat &descriptors_cv_out,
    const std::string &detector_type = "SIFT");
```

**Parameter Description**:
| Parameter            | Type                 | Description                         |
| -------------------- | -------------------- | ----------------------------------- |
| `descriptors_soa`    | `const DescsSOA&`    | [Input] DescriptorsSOA (SOA format) |
| `descriptors_cv_out` | `cv::Mat&`           | [Output] OpenCV descriptor matrix   |
| `detector_type`      | `const std::string&` | [Input] Detector type               |

**Return Value**: `bool` - Whether conversion succeeded

**Usage Example**:
```cpp
// PoSDK SOA descriptors
types::DescsSOA descriptors_soa = GetDescriptorsSOA();

// Convert to OpenCV format (for matching)
cv::Mat descriptors_cv;
bool success = OpenCVConverter::DescsSOA2CVDescriptors(
    descriptors_soa, descriptors_cv, "SIFT");

// OpenCV matching
cv::BFMatcher matcher(cv::NORM_L2);
std::vector<cv::DMatch> matches;
matcher.match(descriptors_cv, descriptors_other, matches);
```

### Legacy AOS Descriptor Conversion (Backward Compatibility)

#### `CVDescriptors2Descs`
Batch convert OpenCV descriptors to PoSDK Descs (AOS format)

#### `Descs2CVDescriptors`
Batch convert PoSDK Descs to OpenCV descriptors (AOS format)

---

## Image Feature Information Conversion
(opencv-image-feature-conversion)=

### `CVFeatures2ImageFeatureInfo`
(cv-features-to-image-feature-info)=

Batch convert OpenCV features to ImageFeatureInfo (without descriptors)

**Function Signature**:
```cpp
static bool CVFeatures2ImageFeatureInfo(
    const std::vector<cv::KeyPoint> &keypoints,
    ImageFeatureInfo &image_features,
    const std::string &image_path = "");
```

**Parameter Description**:
| Parameter        | Type                               | Description                 |
| ---------------- | ---------------------------------- | --------------------------- |
| `keypoints`      | `const std::vector<cv::KeyPoint>&` | [Input] OpenCV keypoints    |
| `image_features` | `ImageFeatureInfo&`                | [Output] Image feature info |
| `image_path`     | `const std::string&`               | [Input] Image file path     |

**Usage Example**:
```cpp
// Extract keypoints
std::vector<cv::KeyPoint> keypoints;
detector->detect(image, keypoints);

// Convert to ImageFeatureInfo
types::ImageFeatureInfo image_features;
OpenCVConverter::CVFeatures2ImageFeatureInfo(
    keypoints, image_features, "image001.jpg");

// Access feature information
size_t num_features = image_features.GetNumFeatures();
const auto& feature_points = image_features.GetFeaturePointsConst();
```

### `CVFeatures2ImageFeatureInfoWithDesc`

Batch convert OpenCV features to ImageFeatureInfo (with descriptors)

**Function Signature**:
```cpp
static bool CVFeatures2ImageFeatureInfoWithDesc(
    const std::vector<cv::KeyPoint> &keypoints,
    const cv::Mat &descriptors_cv,
    ImageFeatureInfo &image_features,
    Descs &descriptors_out,
    const std::string &image_path = "",
    const std::string &detector_type = "SIFT");
```

**Complete Conversion Workflow**:
```cpp
// 1. OpenCV feature extraction
cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

// 2. Convert to PoSDK format
types::ImageFeatureInfo image_features;
types::Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs,
    "image001.jpg", "SIFT");

// 3. Add to FeaturesInfo
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
features_info->push_back(image_features);
```

### `ImageFeatureInfo2CVFeatures`
(image-feature-info-to-cv-features)=

Batch convert ImageFeatureInfo to OpenCV features (without descriptors)

**Function Signature**:
```cpp
static bool ImageFeatureInfo2CVFeatures(
    const ImageFeatureInfo &image_features,
    std::vector<cv::KeyPoint> &keypoints);
```

### `ImageFeatureInfo2CVFeaturesWithDesc`

Batch convert ImageFeatureInfo to OpenCV features (with descriptors)

**Function Signature**:
```cpp
static bool ImageFeatureInfo2CVFeaturesWithDesc(
    const ImageFeatureInfo &image_features,
    const Descs &descriptors,
    std::vector<cv::KeyPoint> &keypoints,
    cv::Mat &descriptors_out,
    const std::string &detector_type = "SIFT");
```

**Error Handling**:
- Automatically detects descriptor count mismatches
- Graceful degradation: converts only keypoints

---

## Batch Feature Conversion

### `CVFeatures2FeaturesInfo`

Batch convert OpenCV features to FeaturesInfo (without descriptors)

**Function Signature**:
```cpp
static bool CVFeatures2FeaturesInfo(
    const std::vector<cv::KeyPoint> &keypoints,
    FeaturesInfoPtr &features_info_ptr,
    const std::string &image_path);
```

### `CVFeatures2FeaturesInfoWithDesc`

Batch convert OpenCV features to FeaturesInfo (with descriptors)

**Function Signature**:
```cpp
static bool CVFeatures2FeaturesInfoWithDesc(
    const std::vector<cv::KeyPoint> &keypoints,
    const cv::Mat &descriptors,
    FeaturesInfoPtr &features_info_ptr,
    Descs &descriptors_out,
    const std::string &image_path,
    const std::string &detector_type = "SIFT");
```

**Batch Processing Example**:
```cpp
// Process multiple images
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
types::Descs all_descriptors;

for (const auto& img_path : image_paths) {
    // Load image
    cv::Mat image = cv::imread(img_path);
    
    // Extract features
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);
    
    // Add to FeaturesInfo
    types::Descs descs;
    OpenCVConverter::CVFeatures2FeaturesInfoWithDesc(
        keypoints, descriptors, features_info, descs,
        img_path, "SIFT");
    
    // Merge descriptors
    all_descriptors.insert(all_descriptors.end(), descs.begin(), descs.end());
}
```

---

## Match Conversion
(opencv-match-conversion)=

### `CVDMatch2IdMatch`

Convert OpenCV DMatch to PoSDK IdMatch (single match)

**Function Signature**:
```cpp
static void CVDMatch2IdMatch(const cv::DMatch &cv_match, IdMatch &match);
```

### `CVDMatch2IdMatches`
(cv-dmatch-to-id-matches)=

Convert OpenCV DMatch to PoSDK IdMatches (batch conversion)

**Function Signature**:
```cpp
static bool CVDMatch2IdMatches(const std::vector<cv::DMatch> &cv_matches,
                               IdMatches &matches);
```

**Usage Example**:
```cpp
// OpenCV matching
cv::BFMatcher matcher;
std::vector<cv::DMatch> cv_matches;
matcher.match(descriptors1, descriptors2, cv_matches);

// Convert to PoSDK format
types::IdMatches id_matches;
OpenCVConverter::CVDMatch2IdMatches(cv_matches, id_matches);
```

### `CVDMatch2Matches`

Convert OpenCV DMatch to PoSDK Matches (with view pair)

**Function Signature**:
```cpp
static bool CVDMatch2Matches(const std::vector<cv::DMatch> &cv_matches,
                             const IndexT view_id1,
                             const IndexT view_id2,
                             MatchesPtr &matches_ptr);
```

### `IdMatch2CVDMatch`

Convert PoSDK IdMatch to OpenCV DMatch (single match)

### `IdMatches2CVPoints`
(id-matches-to-cv-points)=

Convert PoSDK IdMatches to OpenCV point pairs

**Function Signature**:
```cpp
static bool IdMatches2CVPoints(
    const IdMatches &matches,
    const FeaturesInfo &features_info,
    const CameraModels &camera_models,
    const ViewPair &view_pair,
    std::vector<cv::Point2f> &points1,
    std::vector<cv::Point2f> &points2);
```

**Use Cases**:
- For OpenCV's `findEssentialMat`
- For OpenCV's `findFundamentalMat`
- For OpenCV's `recoverPose`

**Usage Example**:
```cpp
// PoSDK match data
types::IdMatches matches;
types::FeaturesInfo features_info;
types::CameraModels camera_models;
types::ViewPair view_pair(0, 1);

// Convert to OpenCV point pairs
std::vector<cv::Point2f> points1, points2;
OpenCVConverter::IdMatches2CVPoints(
    matches, features_info, camera_models, view_pair,
    points1, points2);

// Use OpenCV to compute essential matrix
cv::Mat E = cv::findEssentialMat(points1, points2, focal_length, 
                                  principal_point, cv::RANSAC);
```

### `MatchesDataPtr2CVPoints`

Convert PoSDK match DataPtr to OpenCV point pairs

**Function Signature**:
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

## Camera Model Conversion
(opencv-camera-conversion)=

### `CVCalibration2CameraModel`
(cv-calibration-to-camera-model)=

Convert OpenCV camera calibration to PoSDK CameraModel

**Function Signature**:
```cpp
static bool CVCalibration2CameraModel(
    const cv::Mat &camera_matrix,
    const cv::Mat &dist_coeffs,
    const cv::Size &image_size,
    CameraModel &camera_model,
    DistortionType distortion_type = DistortionType::BROWN_CONRADY);
```

**Parameter Description**:
| Parameter         | Type              | Description                            |
| ----------------- | ----------------- | -------------------------------------- |
| `camera_matrix`   | `const cv::Mat&`  | [Input] OpenCV intrinsic matrix (3×3)  |
| `dist_coeffs`     | `const cv::Mat&`  | [Input] OpenCV distortion coefficients |
| `image_size`      | `const cv::Size&` | [Input] Image size                     |
| `camera_model`    | `CameraModel&`    | [Output] PoSDK camera model            |
| `distortion_type` | `DistortionType`  | [Input] Distortion type                |

**Usage Example**:
```cpp
// OpenCV camera calibration results
cv::Mat camera_matrix, dist_coeffs;
cv::Size image_size(640, 480);
cv::calibrateCamera(object_points, image_points, image_size,
                    camera_matrix, dist_coeffs, rvecs, tvecs);

// Convert to PoSDK format
types::CameraModel camera_model;
OpenCVConverter::CVCalibration2CameraModel(
    camera_matrix, dist_coeffs, image_size, camera_model,
    types::DistortionType::BROWN_CONRADY);

// Use PoSDK camera model
Vector2d normalized = camera_model.PixelToNormalized(pixel);
```

### `CameraModel2CVCalibration`
(camera-model-to-cv-calibration)=

Convert PoSDK CameraModel to OpenCV camera calibration

**Function Signature**:
```cpp
static bool CameraModel2CVCalibration(const CameraModel &camera_model,
                                      cv::Mat &camera_matrix,
                                      cv::Mat &dist_coeffs);
```

**Usage Example**:
```cpp
// PoSDK camera model
types::CameraModel camera_model = GetCameraModel();

// Convert to OpenCV format
cv::Mat camera_matrix, dist_coeffs;
OpenCVConverter::CameraModel2CVCalibration(
    camera_model, camera_matrix, dist_coeffs);

// Use OpenCV functions
cv::undistort(image_distorted, image_undistorted, 
              camera_matrix, dist_coeffs);
```

---

---

## Error Handling and Debugging

### Type Checking
```cpp
if (descriptors_cv.type() != CV_32F) {
    LOG_ERROR_ZH << "Descriptor type error, expected CV_32F";
    return false;
}
```

### Dimension Validation
```cpp
if (descriptors_cv.rows != keypoints.size()) {
    LOG_ERROR_ZH << "Descriptor count(" << descriptors_cv.rows 
                 << ") does not match keypoint count(" << keypoints.size() << ")";
    return false;
}
```

### Null Pointer Check
```cpp
const CameraModel* camera = camera_models[view_id];
if (!camera) {
    LOG_ERROR_ZH << "Invalid camera model, view_id=" << view_id;
    return false;
}
```

---

## Complete Workflow Example

### OpenCV Feature Extraction + PoSDK Processing
```cpp
#include <opencv2/features2d.hpp>
#include "converter_opencv.hpp"
#include <po_core.hpp>

using namespace PoSDK;
using namespace PoSDK::Converter;

// 1. OpenCV feature extraction
cv::Mat image = cv::imread("image.jpg");
cv::Ptr<cv::SIFT> detector = cv::SIFT::create(2000);

std::vector<cv::KeyPoint> keypoints;
cv::Mat descriptors;
detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

LOG_INFO_ZH << "Detected " << keypoints.size() << " feature points";

// 2. Convert to PoSDK format
types::ImageFeatureInfo image_features;
types::Descs descs;
OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
    keypoints, descriptors, image_features, descs,
    "image.jpg", "SIFT");

// 3. Use PoSDK processing (e.g., robust matching)
types::FeaturesInfoPtr features_info = std::make_shared<types::FeaturesInfo>();
features_info->push_back(image_features);

// 4. Convert back to OpenCV (if needed)
std::vector<cv::KeyPoint> cv_keypoints_back;
cv::Mat cv_descriptors_back;
OpenCVConverter::ImageFeatureInfo2CVFeaturesWithDesc(
    image_features, descs, cv_keypoints_back, cv_descriptors_back, "SIFT");

// 5. Use OpenCV visualization
cv::Mat output;
cv::drawKeypoints(image, cv_keypoints_back, output);
cv::imshow("Features", output);
```

---

**Related Links**:
- [Core Data Types - FeaturePoints](../appendices/appendix_a_types.md#feature-point)
- [Core Data Types - ImageFeatureInfo](../appendices/appendix_a_types.md#image-feature-info)
- [Core Data Types - CameraModel](../appendices/appendix_a_types.md#camera-model)
- [Converter Overview](index.md)



