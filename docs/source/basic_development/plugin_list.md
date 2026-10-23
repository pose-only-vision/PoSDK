# PoMVG Plugin List

This document lists selected PoMVG plugins. Input and output columns show the ordinary GUI ports declared in their source manifests.

## Plugin Function Table

### Two-View Estimators

| Plugin Name | Registration Name | Input Types | Plugin Output | Function |
| ----------- | ----------------- | ----------- | ------------- | -------- |
| BarathTwoViewEstimator | `barath_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Two-view pose estimation using MAGSAC and MAGSAC++ |
| DFETwoViewEstimator | `dfe_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Deep learning based two-view estimation using DFE network |
| DegensacTwoViewEstimator | `degensac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | C++ DEGENSAC for robust fundamental/essential matrix estimation |
| FiveptStewenius | `fivept_stewenius` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Stewenius five-point algorithm for essential matrix estimation |
| LiasEpipolarEstimator | `lias_epipolar_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | LIAS consistent epipolar estimator (CECME, ManifoldGN, LM) |
| NACNetTwoViewEstimator | `nacnet_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Deep learning based estimation using NACNet |
| NptEpipolarEstimator | `npt_pose_algorithm` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | NPT algorithm using SDP relaxation with SDPA solver |
| OpenCVTwoViewEstimator | `opencv_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | OpenCV-based two-view estimation (fundamental, essential, homography) with multiple RANSAC algorithms |
| OpenGVModelEstimator | `opengv_model_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | OpenGV-based relative pose estimation with various algorithms |
| PoseLibModelEstimator | `poselib_model_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | PoseLib-based relative pose estimation with refinement |
| PyDegensacTwoViewEstimator | `pydegensac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | PyDegensac Python library for robust fundamental matrix estimation |
| QDegsacTwoViewEstimator | `qdegsac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Independent paper-derived QDEGSAC reimplementation for quasi-degenerate robust estimation; not the authors' original code |
| TwoViewEstimator | `TwoViewEstimator` | `data_matches`, `data_features`, `data_camera_models` | `data_relative_poses`, `data_matches` | Multi-threaded two-view estimation with quality validation |

### Feature Processing

| Plugin Name | Registration Name | Input Types | Plugin Output | Function |
| ----------- | ----------------- | ----------- | ------------- | -------- |
| Img2Features | `method_img2features` | `data_images` | `data_features` | Feature extraction (SIFT, ORB, AKAZE, SuperPoint, etc.) |
| Img2Matches | `method_img2matches` | `data_features` | `data_features`, `data_matches` | Feature matching pipeline |
| LoFTR | `method_loftr` | `data_images` | `data_features`, `data_matches` | Detector-free matching using LoFTR deep learning model |
| MethodCalibrator | `method_calibrator` | `data_images` | `data_camera_models`, `data_global_poses`, `data_relative_poses`, `data_generic_scalib`, `data_features`, `data_matches`, `data_img_point_calib` | Camera calibration using OpenCV |

### Complete Pipelines

| Plugin Name | Registration Name | Input Types | Plugin Output | Function |
| ----------- | ----------------- | ----------- | ------------- | -------- |
| **GlobalSfMPipeline** | **`globalsfm_pipeline`** | `data_images` | `data_global_poses` | **PoMVG Complete SfM Pipeline**: End-to-end GlobalSfM reconstruction workflow, supports multiple preprocessors and algorithm comparison |
| OpenMVGPipeline | `openmvg_pipeline` | None | `data_images`, `data_features`, `data_matches`, `data_camera_models`, `data_relative_poses` | Complete OpenMVG preprocessing pipeline |
| GLOMAP | `glomap_pipeline` | `data_camera_models`, `data_features`, `data_matches`, `data_relative_poses` | `data_global_poses`, `data_points_3d`, `data_tracks`, `data_features`, `data_relative_poses`, `data_matches` | GLOMAP global SfM pipeline |

### Rotation Averaging

| Plugin Name | Registration Name | Input Types | Plugin Output | Function |
| ----------- | ----------------- | ----------- | ------------- | -------- |
| MethodRotationAveraging | `method_rotation_averaging` | `data_relative_poses` | `data_global_poses`, `data_relative_poses`, `data_relative_rotations`, `data_pose_graph_quality` | Global rotation estimation using GraphOptim |
| MethodRotationAveragingChatterjee | `method_rotation_averaging_chatterjee` | `data_relative_poses` | `data_global_poses` | Chatterjee algorithm with IRLS optimization for global rotation estimation |

### Utilities

| Plugin Name | Registration Name | Input Types | Plugin Output | Function |
| ----------- | ----------------- | ----------- | ------------- | -------- |
| DatasetLoader | Multiple loaders | N/A (loads from files) | `data_global_poses`, `data_relative_poses`, `data_camera_models` | Dataset loaders (Strecha, ETH3D, 1DSfM, ISPRS, Kitti, PMBA) |
| MethodExample | `method_example` | `data_tracks` | `data_tracks` | Example plugin for development reference |
| MethodMatchesVisualizer | `method_matches_visualizer` | `data_matches`, `data_images`, `data_features` | None | Visualization of feature matches, inliers in green, outliers in red |
| MethodMeshlabExporter | `method_meshlab_exporter` | `data_global_poses`, `data_camera_models`, `data_points_3d`, `data_images` | None | Export to MeshLab-compatible project files |

`method_example` is a development skeleton: its current manifest declares `data_tracks`, while `Run()` returns `data_map_string`. It is not a track-processing algorithm.

## Core Pipeline Plugin Details

### `globalsfm_pipeline` - PoMVG GlobalSfM Pipeline

PoMVG's core SfM pipeline plugin, providing global 3D reconstruction solution.

#### Basic Configuration Parameters

| Parameter Name      | Type   | Default Value                       | Description                                                   |
| ------------------- | ------ | ----------------------------------- | ------------------------------------------------------------- |
| `dataset_type` | string | `isprs` | Dataset source; use `custom` when providing an image folder directly |
| `dataset_dir`       | string | Must be set                         | Dataset root directory path, supports `{exe_dir}` placeholder |
| `image_folder`      | string | Empty | Image folder path, supports nested placeholders               |
| `work_dir`          | string | `{exe_dir}/globalsfm_pipeline_work` | Working directory path                                        |
| `preprocess_type`   | string | `"posdk"`                           | Preprocessing type: `openmvg`, `opencv`, `posdk`              |
| `enable_evaluation` | bool   | `true`                              | Whether to enable accuracy evaluation                         |

#### Pipeline Control Options

| Parameter Name               | Type   | Default Value | Description                                                                         |
| ---------------------------- | ------ | ------------- | ----------------------------------------------------------------------------------- |
| `enable_3d_points_output`    | bool   | `true`        | Whether to output 3D point cloud data                                               |
| `enable_meshlab_export`      | bool   | `true`        | Whether to export Meshlab visualization project files                               |
| `enable_posdk2colmap_export` | bool   | `true`        | Whether to export PoMVG reconstruction to Colmap format (cameras, images, points3D) |
| `enable_csv_export`          | bool   | `true`        | Whether to enable CSV export of evaluation results                                  |
| `evaluation_print_mode`      | string | `"none"`   | Evaluation result print mode: `none`, `summary`, `detailed`, `comparison`           |
| `compared_pipelines`         | string | `"openmvg,colmap"`          | Comparison algorithm list, e.g., `"openmvg,COLMAP"`                                 |

#### Performance Optimization Options

| Parameter Name           | Type | Default Value | Description                          |
| ------------------------ | ---- | ------------- | ------------------------------------ |
| `enable_profiling`       | bool | `true`        | Enable performance analysis          |
| `enable_data_statistics` | bool | `false`        | Enable data statistics functionality |

#### Configuration File Example

Recommended configuration in `{exe_dir}/../configs/methods/globalsfm_pipeline.ini`:

```ini
[globalsfm_pipeline]
dataset_type=custom
# Basic parameters - use placeholders for automatic adaptation
dataset_dir={exe_dir}/tests/Strecha
image_folder={dataset_dir}/castle-P19/images
work_dir={exe_dir}/globalsfm_pipeline_work

# Preprocessing and evaluation
preprocess_type=posdk
enable_evaluation=true

# Output control
enable_3d_points_output=true
enable_meshlab_export=true
enable_posdk2colmap_export=true
enable_csv_export=true
evaluation_print_mode=summary

# Performance options
enable_profiling=true
```

#### Placeholder System

Configuration supports flexible placeholder replacement:

- `{exe_dir}`: Executable file directory
- `{root_dir}`: Project root directory
- `{dataset_dir}`: Dataset root directory (dynamic replacement)
- `{key_name}`: Reference other configuration key values

#### Running Modes

```bash
# Default mode: use configuration file parameters
./PoSDK

# Custom mode: command-line parameters override configuration
./PoSDK --preset=custom --dataset_dir=/path/to/dataset
```

#### Output Results

- **Pose Data**: `data_global_poses` - Global camera poses
- **3D Point Cloud**: `data_points_3d` - Sparse 3D reconstruction points (optional)
- **Evaluation Reports**: CSV format accuracy analysis and algorithm comparison
- **Visualization Files**: Meshlab project files (.mlp) for 3D display

For detailed parameter descriptions, please refer to comments in the configuration file.

## Plugin Categories

### Two-View Estimators
- `barath_two_view_estimator`: MAGSAC/MAGSAC++ robust estimation
- `dfe_two_view_estimator`: DFE deep learning two-view estimation
- `degensac_two_view_estimator`: C++ DEGENSAC robust estimation
- `fivept_stewenius`: Stewenius five-point algorithm
- `lias_epipolar_estimator`: LIAS consistent epipolar estimator
- `nacnet_two_view_estimator`: NACNet deep learning estimation
- `npt_pose_algorithm`: NPT algorithm (SDP relaxation)
- `opencv_two_view_estimator`: OpenCV implementation
- `opengv_model_estimator`: OpenGV implementation
- `poselib_model_estimator`: PoseLib implementation
- `pydegensac_two_view_estimator`: PyDegensac robust estimation
- `qdegsac_two_view_estimator`: Paper-derived QDEGSAC reimplementation (not the authors' original code)
- `TwoViewEstimator`: Multi-threaded integrated two-view estimation

### Feature Processing
- `method_img2features`: Feature extraction (SIFT, ORB, AKAZE, SuperPoint, etc.)
- `method_img2matches`: Feature matching pipeline
- `method_loftr`: LoFTR detector-free matching
- `method_calibrator`: Camera calibration

### Complete Pipelines
- **`globalsfm_pipeline`**: PoMVG complete SfM workflow (core recommended)
- `openmvg_pipeline`: OpenMVG complete preprocessing pipeline
- `glomap_pipeline`: GLOMAP global SfM preprocessing pipeline

### Rotation Averaging
- `method_rotation_averaging`: GraphOptim global rotation estimation
- `method_rotation_averaging_chatterjee`: Chatterjee IRLS algorithm

### Utilities
- DatasetLoader: Dataset loaders (Strecha, ETH3D, 1DSfM, ISPRS, Kitti, PMBA)
- `method_example`: Example plugin
- `method_matches_visualizer`: Feature matching visualization
- `method_meshlab_exporter`: MeshLab project file export

## Usage Instructions

All plugins are created and used through PoMVG factory pattern:

```cpp
// Create plugin instance
auto plugin = FactoryMethod::Create("plugin_name");

// Prepare input data
DataPackagePtr input_package = std::make_shared<DataPackage>();
input_package->AddData("data_type", data_ptr);

// Run plugin
DataPtr output = plugin->Build(input_package);
```

For detailed functionality and parameter configuration, please refer to each plugin's source code.
