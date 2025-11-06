# PoSDK Plugin List

This document lists all available plugins in the PoSDK project along with their basic information.

## Plugin Function Table

| Plugin Name                            | Input Type                                      | Plugin Output                                                                                                    | Function                                                                                                                                                                                |
| -------------------------------------- | ----------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `method_calibrator`                    | `data_images`                                   | `data_cameras` (CameraModels)                                                                                    | Camera calibration: Uses OpenCV for pinhole camera model calibration, supports checkerboard calibration board                                                                           |
| `method_matches_visualizer`            | `data_matches`, `data_images`, `data_features`  | None (visualization output to file)                                                                              | Feature matching visualization: Draws two-view matching results on images, inliers shown in green, outliers in red                                                                      |
| `method_rotation_averaging_Chatterjee` | `data_relative_poses`, `data_global_poses`      | `data_global_poses` (GlobalPoses)                                                                                | Global rotation averaging: Uses Chatterjee's IRLS algorithm for global rotation estimation                                                                                              |
| `method_rotation_averaging`            | `data_relative_poses`                           | `data_global_poses` (GlobalPoses)                                                                                | Global rotation averaging: Uses GraphOptim tool for global rotation estimation                                                                                                          |
| `opencv_two_view_estimator`            | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                            | Two-view pose estimation: Uses OpenCV to compute fundamental matrix, essential matrix, or homography matrix, supports multiple RANSAC algorithms                                        |
| `opengv_model_estimator`               | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                            | Two-view pose estimation: Uses OpenGV library for relative pose estimation, supports RANSAC robust estimation                                                                           |
| `openmvg_pipeline`                     | `data_images`                                   | `data_images`, `data_features`, `data_matches`, `data_global_poses` (optional), `data_world_3dpoints` (optional) | OpenMVG complete pipeline: Executes complete SfM workflow, including feature extraction, matching, geometric filtering, 3D reconstruction, point cloud coloring, and quality assessment |
| `poselib_model_estimator`              | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                            | Two-view pose estimation: Uses PoseLib library for relative pose estimation                                                                                                             |
| `two_view_estimator`                   | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                            | Two-view pose estimation: Integrated PoSDK optimizer two-view pose estimation, supports multiple estimators and fine optimization                                                       |
| **`globalsfm_pipeline`**               | `data_images`                                   | `data_global_poses`, `data_world_3dpoints` (optional)                                                            | **PoSDK Complete SfM Pipeline**: End-to-end GlobalSfM reconstruction workflow based on PoSDK, supports multiple preprocessors and algorithm comparison                                  |

## Core Pipeline Plugin Details

### `globalsfm_pipeline` - PoSDK GlobalSfM Pipeline 

PoSDK's core SfM pipeline plugin, providing global 3D reconstruction solution.

#### Basic Configuration Parameters

| Parameter Name      | Type   | Default Value                       | Description                                                   |
| ------------------- | ------ | ----------------------------------- | ------------------------------------------------------------- |
| `dataset_dir`       | string | Must be set                         | Dataset root directory path, supports `{exe_dir}` placeholder |
| `image_folder`      | string | `{dataset_dir}/images`              | Image folder path, supports nested placeholders               |
| `work_dir`          | string | `{exe_dir}/globalsfm_pipeline_work` | Working directory path                                        |
| `preprocess_type`   | string | `"posdk"`                           | Preprocessing type: `openmvg`, `opencv`, `posdk`              |
| `enable_evaluation` | bool   | `true`                              | Whether to enable accuracy evaluation                         |

#### Pipeline Control Options

| Parameter Name               | Type   | Default Value | Description                                                                         |
| ---------------------------- | ------ | ------------- | ----------------------------------------------------------------------------------- |
| `enable_3d_points_output`    | bool   | `true`        | Whether to output 3D point cloud data                                               |
| `enable_meshlab_export`      | bool   | `true`        | Whether to export Meshlab visualization project files                               |
| `enable_posdk2colmap_export` | bool   | `true`        | Whether to export PoSDK reconstruction to Colmap format (cameras, images, points3D) |
| `enable_csv_export`          | bool   | `true`        | Whether to enable CSV export of evaluation results                                  |
| `evaluation_print_mode`      | string | `"summary"`   | Evaluation result print mode: `none`, `summary`, `detailed`, `comparison`           |
| `compared_pipelines`         | string | `""`          | Comparison algorithm list, e.g., `"openmvg,COLMAP"`                                 |

#### Performance Optimization Options

| Parameter Name           | Type | Default Value | Description                          |
| ------------------------ | ---- | ------------- | ------------------------------------ |
| `enable_profiling`       | bool | `true`        | Enable performance analysis          |
| `enable_data_statistics` | bool | `true`        | Enable data statistics functionality |

#### Configuration File Example

Recommended configuration in `{exe_dir}/../configs/methods/globalsfm_pipeline.ini`:

```ini
[globalsfm_pipeline]
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
- **3D Point Cloud**: `data_world_3dpoints` - Sparse 3D reconstruction points (optional)
- **Evaluation Reports**: CSV format accuracy analysis and algorithm comparison
- **Visualization Files**: Meshlab project files (.mlp) for 3D display

For detailed parameter descriptions, please refer to comments in the configuration file.

## Plugin Categories

### Camera Calibration Class
- `method_calibrator`: Pinhole camera model calibration

### Visualization Class
- `method_matches_visualizer`: Feature matching visualization

### Global Rotation Estimation Class
- `method_rotation_averaging_Chatterjee`: Chatterjee algorithm
- `method_rotation_averaging`: GraphOptim tool

### Two-View Pose Estimation Class
- `opencv_two_view_estimator`: OpenCV implementation
- `opengv_model_estimator`: OpenGV implementation
- `poselib_model_estimator`: PoseLib implementation
- `two_view_estimator`: PoSDK integrated implementation

### Complete Pipeline Class
- `openmvg_pipeline`: OpenMVG complete SfM workflow
- **`globalsfm_pipeline`**: PoSDK complete SfM workflow (core recommended)

## Usage Instructions

All plugins are created and used through PoSDK factory pattern:

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

---
