# PoMVG 插件列表

本文档列出 PoMVG 的部分插件。表中的输入与输出对应源码 manifest 声明的 GUI 普通端口。

## 插件功能表

### 两视图估计器

| 插件名 | 注册名 | 输入类型 | 插件输出 | 功能 |
| ------ | ------ | -------- | -------- | ---- |
| BarathTwoViewEstimator | `barath_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 双视图位姿估计：使用MAGSAC与MAGSAC++进行鲁棒估计 |
| DFETwoViewEstimator | `dfe_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 基于深度学习的双视图估计：使用DFE网络 |
| DegensacTwoViewEstimator | `degensac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | C++ DEGENSAC：鲁棒基础矩阵/本质矩阵估计 |
| FiveptStewenius | `fivept_stewenius` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | Stewenius五点算法：本质矩阵估计 |
| LiasEpipolarEstimator | `lias_epipolar_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | LIAS一致性对极估计器（CECME、ManifoldGN、LM） |
| NACNetTwoViewEstimator | `nacnet_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 基于深度学习的双视图估计：使用NACNet |
| NptEpipolarEstimator | `npt_pose_algorithm` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | NPT算法：使用SDPA求解器的SDP松弛方法 |
| OpenCVTwoViewEstimator | `opencv_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 使用OpenCV计算基础矩阵、本质矩阵或单应矩阵，支持多种RANSAC算法 |
| OpenGVModelEstimator | `opengv_model_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 使用OpenGV库进行相对位姿估计，支持多种算法 |
| PoseLibModelEstimator | `poselib_model_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 使用PoseLib库进行相对位姿估计，支持精细优化 |
| PyDegensacTwoViewEstimator | `pydegensac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | PyDegensac Python库：鲁棒基础矩阵估计 |
| QDegsacTwoViewEstimator | `qdegsac_two_view_estimator` | `data_sample`, `data_features`, `data_camera_models` | `data_relative_pose` | 基于论文独立复现的 QDEGSAC 准退化鲁棒估计（非作者原始代码） |
| TwoViewEstimator | `TwoViewEstimator` | `data_matches`, `data_features`, `data_camera_models` | `data_relative_poses`, `data_matches` | 多线程双视图估计：集成质量验证的位姿估计 |

### 特征处理

| 插件名 | 注册名 | 输入类型 | 插件输出 | 功能 |
| ------ | ------ | -------- | -------- | ---- |
| Img2Features | `method_img2features` | `data_images` | `data_features` | 特征提取（SIFT、ORB、AKAZE、SuperPoint等） |
| Img2Matches | `method_img2matches` | `data_features` | `data_features`, `data_matches` | 特征匹配管道 |
| LoFTR | `method_loftr` | `data_images` | `data_features`, `data_matches` | 无检测器匹配：使用LoFTR深度学习模型 |
| MethodCalibrator | `method_calibrator` | `data_images` | `data_camera_models`, `data_global_poses`, `data_relative_poses`, `data_generic_scalib`, `data_features`, `data_matches`, `data_img_point_calib` | 相机标定：使用OpenCV对针孔相机模型进行标定 |

### 完整流水线

| 插件名 | 注册名 | 输入类型 | 插件输出 | 功能 |
| ------ | ------ | -------- | -------- | ---- |
| **GlobalSfMPipeline** | **`globalsfm_pipeline`** | `data_images` | `data_global_poses` | **PoMVG完整SfM管道**：基于PoMVG的端到端GlobalSfM重建流程，支持多种预处理器和算法比较 |
| OpenMVGPipeline | `openmvg_pipeline` | 无 | `data_images`, `data_features`, `data_matches`, `data_camera_models`, `data_relative_poses` | OpenMVG完整预处理管道 |
| GLOMAP | `glomap_pipeline` | `data_camera_models`, `data_features`, `data_matches`, `data_relative_poses` | `data_global_poses`, `data_points_3d`, `data_tracks`, `data_features`, `data_relative_poses`, `data_matches` | GLOMAP 全局 SfM 管道 |

### 旋转平均

| 插件名 | 注册名 | 输入类型 | 插件输出 | 功能 |
| ------ | ------ | -------- | -------- | ---- |
| MethodRotationAveraging | `method_rotation_averaging` | `data_relative_poses` | `data_global_poses`, `data_relative_poses`, `data_relative_rotations`, `data_pose_graph_quality` | 全局旋转估计：使用GraphOptim工具 |
| MethodRotationAveragingChatterjee | `method_rotation_averaging_chatterjee` | `data_relative_poses` | `data_global_poses` | Chatterjee算法：使用IRLS优化进行全局旋转估计 |

### 工具类

| 插件名 | 注册名 | 输入类型 | 插件输出 | 功能 |
| ------ | ------ | -------- | -------- | ---- |
| DatasetLoader | 多种加载器 | 无（从文件加载） | `data_global_poses`, `data_relative_poses`, `data_camera_models` | 数据集加载器（Strecha、ETH3D、1DSfM、ISPRS、Kitti、PMBA） |
| MethodExample | `method_example` | `data_tracks` | `data_tracks` | 示例插件：开发参考 |
| MethodMatchesVisualizer | `method_matches_visualizer` | `data_matches`, `data_images`, `data_features` | 无 | 特征匹配可视化：内点绿色、外点红色 |
| MethodMeshlabExporter | `method_meshlab_exporter` | `data_global_poses`, `data_camera_models`, `data_points_3d`, `data_images` | 无 | 导出MeshLab兼容的工程文件 |

`method_example` 是开发骨架示例：当前 manifest 声明输出 `data_tracks`，但其 `Run()` 实际返回 `data_map_string`，不能作为轨迹处理算法使用。

## 核心管道插件详解

### `globalsfm_pipeline` - PoMVG GlobalSfM管道

PoMVG的核心SfM管道插件，提供全局式三维重建解决方案。

#### 基础配置参数

| 参数名              | 类型   | 默认值                              | 说明                                     |
| ------------------- | ------ | ----------------------------------- | ---------------------------------------- |
| `dataset_type` | string | `isprs` | 数据源类型；直接指定图像目录时设为 `custom` |
| `dataset_dir`       | string | 必须设置                            | 数据集根目录路径，支持`{exe_dir}`占位符  |
| `image_folder`      | string | 空 | 图像文件夹路径，支持嵌套占位符           |
| `work_dir`          | string | `{exe_dir}/globalsfm_pipeline_work` | 工作目录路径                             |
| `preprocess_type`   | string | `"posdk"`                           | 预处理类型：`openmvg`, `opencv`, `posdk` |
| `enable_evaluation` | bool   | `true`                              | 是否启用精度评估                         |

#### 管道控制选项

| 参数名                       | 类型   | 默认值      | 说明                                                          |
| ---------------------------- | ------ | ----------- | ------------------------------------------------------------- |
| `enable_3d_points_output`    | bool   | `true`      | 是否输出3D点云数据                                            |
| `enable_meshlab_export`      | bool   | `true`      | 是否导出Meshlab可视化工程文件                                 |
| `enable_posdk2colmap_export` | bool   | `true`      | 是否导出PoMVG重建结果到Colmap格式（相机、图像、3D点）         |
| `enable_csv_export`          | bool   | `true`      | 是否启用评估结果CSV导出                                       |
| `evaluation_print_mode`      | string | `"none"` | 评估结果打印模式：`none`, `summary`, `detailed`, `comparison` |
| `compared_pipelines`         | string | `"openmvg,colmap"`        | 对比算法列表，如`"openmvg,COLMAP"`                            |

#### 性能优化选项

| 参数名                   | 类型 | 默认值 | 说明             |
| ------------------------ | ---- | ------ | ---------------- |
| `enable_profiling`       | bool | `true` | 启用性能分析     |
| `enable_data_statistics` | bool | `false` | 启用数据统计功能 |

#### 配置文件示例

推荐在`{exe_dir}/../configs/methods/globalsfm_pipeline.ini`中配置：

```ini
[globalsfm_pipeline]
dataset_type=custom
# 基础参数 - 使用占位符自动适配
dataset_dir={exe_dir}/tests/Strecha
image_folder={dataset_dir}/castle-P19/images
work_dir={exe_dir}/globalsfm_pipeline_work

# 预处理和评估
preprocess_type=posdk
enable_evaluation=true

# 输出控制
enable_3d_points_output=true
enable_meshlab_export=true
enable_posdk2colmap_export=true
enable_csv_export=true
evaluation_print_mode=summary

# 性能选项
enable_profiling=true
```

#### 占位符系统

配置支持灵活的占位符替换：

- `{exe_dir}`: 可执行文件所在目录
- `{root_dir}`: 项目根目录
- `{dataset_dir}`: 数据集根目录（动态替换）
- `{key_name}`: 引用其他配置键值

#### 运行模式

先在 GUI 中保存包含 `globalsfm_pipeline` 节点的 `globalsfm.posdk` 工程，再使用当前 CLI：

```bash
posdk workflow inspect --project globalsfm.posdk
posdk workflow run --project globalsfm.posdk --workspace ./workspace
```

#### 输出结果

- **位姿数据**：`data_global_poses` - 全局相机位姿
- **3D点云**：`data_points_3d` - 稀疏3D重建点（可选）
- **评估报告**：CSV格式的精度分析和算法对比
- **可视化文件**：Meshlab工程文件（.mlp）用于三维显示

详细参数说明请参考配置文件中的注释。

## 插件分类

### 两视图估计器
- `barath_two_view_estimator`: MAGSAC/MAGSAC++鲁棒估计
- `dfe_two_view_estimator`: DFE深度学习双视图估计
- `degensac_two_view_estimator`: C++ DEGENSAC鲁棒估计
- `fivept_stewenius`: Stewenius五点算法
- `lias_epipolar_estimator`: LIAS一致性对极估计器
- `nacnet_two_view_estimator`: NACNet深度学习估计
- `npt_pose_algorithm`: NPT算法（SDP松弛）
- `opencv_two_view_estimator`: OpenCV实现
- `opengv_model_estimator`: OpenGV实现
- `poselib_model_estimator`: PoseLib实现
- `pydegensac_two_view_estimator`: PyDegensac鲁棒估计
- `qdegsac_two_view_estimator`: 基于论文复现的QDEGSAC（非作者原始代码）
- `TwoViewEstimator`: 多线程集成双视图估计

### 特征处理
- `method_img2features`: 特征提取（SIFT、ORB、AKAZE、SuperPoint等）
- `method_img2matches`: 特征匹配管道
- `method_loftr`: LoFTR无检测器匹配
- `method_calibrator`: 相机标定

### 完整流水线
- **`globalsfm_pipeline`**: PoMVG完整SfM流程（核心推荐）
- `openmvg_pipeline`: OpenMVG完整预处理管道
- `glomap_pipeline`: GLOMAP全局SfM预处理管道

### 旋转平均
- `method_rotation_averaging`: GraphOptim全局旋转估计
- `method_rotation_averaging_chatterjee`: Chatterjee IRLS算法

### 工具类
- DatasetLoader: 数据集加载器（Strecha、ETH3D、1DSfM、ISPRS、Kitti、PMBA）
- `method_example`: 示例插件
- `method_matches_visualizer`: 特征匹配可视化
- `method_meshlab_exporter`: MeshLab工程文件导出

## 使用说明

所有插件均通过PoMVG工厂模式创建和使用：

```cpp
// 创建插件实例
auto plugin = FactoryMethod::Create("plugin_name");

// 准备输入数据
DataPackagePtr input_package = std::make_shared<DataPackage>();
input_package->AddData("data_type", data_ptr);

// 运行插件
DataPtr output = plugin->Build(input_package);
```

详细功能和参数配置请参考各插件源码。

---

*最后更新: 2025年*
