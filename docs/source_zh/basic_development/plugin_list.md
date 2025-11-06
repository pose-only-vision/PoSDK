# PoSDK 插件列表

本文档列出了PoSDK项目中的所有可用插件及其基本信息。

## 插件功能表

| 插件名                                 | 输入类型                                        | 插件输出                                                                                                 | 功能                                                                                           |
| -------------------------------------- | ----------------------------------------------- | -------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| `method_calibrator`                    | `data_images`                                   | `data_cameras` (CameraModels)                                                                            | 相机标定：使用OpenCV对针孔相机模型进行标定，支持棋盘格标定板                                   |
| `method_matches_visualizer`            | `data_matches`, `data_images`, `data_features`  | 无（可视化输出到文件）                                                                                   | 特征匹配可视化：将双视图匹配结果绘制在图像上，内点显示为绿色，外点显示为红色                   |
| `method_rotation_averaging_Chatterjee` | `data_relative_poses`, `data_global_poses`      | `data_global_poses` (GlobalPoses)                                                                        | 全局旋转平均：使用Chatterjee的IRLS算法进行全局旋转估计                                         |
| `method_rotation_averaging`            | `data_relative_poses`                           | `data_global_poses` (GlobalPoses)                                                                        | 全局旋转平均：使用GraphOptim工具进行全局旋转估计                                               |
| `opencv_two_view_estimator`            | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                    | 双视图位姿估计：使用OpenCV计算基础矩阵、本质矩阵或单应矩阵，支持多种RANSAC算法                 |
| `opengv_model_estimator`               | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                    | 双视图位姿估计：使用OpenGV库进行相对位姿估计，支持RANSAC鲁棒估计                               |
| `openmvg_pipeline`                     | `data_images`                                   | `data_images`, `data_features`, `data_matches`, `data_global_poses` (可选), `data_world_3dpoints` (可选) | OpenMVG完整管道：执行完整的SfM流程，包括特征提取、匹配、几何过滤、三维重建、点云着色和质量评估 |
| `poselib_model_estimator`              | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                    | 双视图位姿估计：使用PoseLib库进行相对位姿估计                                                  |
| `two_view_estimator`                   | `data_matches`, `data_features`, `data_cameras` | `data_relative_poses` (RelativePoses)                                                                    | 双视图位姿估计：集成PoSDK优化器的双视图位姿估计，支持多种估计器和精细优化                      |
| **`globalsfm_pipeline`**               | `data_images`                                   | `data_global_poses`, `data_world_3dpoints` (可选)                                                        | **PoSDK完整SfM管道**：基于PoSDK的端到端GlobalSfM重建流程，支持多种预处理器和算法比较           |

## 核心管道插件详解

### `globalsfm_pipeline` - PoSDK GlobalSfM管道 

PoSDK的核心SfM管道插件，提供全局式三维重建解决方案。

#### 基础配置参数

| 参数名              | 类型   | 默认值                              | 说明                                     |
| ------------------- | ------ | ----------------------------------- | ---------------------------------------- |
| `dataset_dir`       | string | 必须设置                            | 数据集根目录路径，支持`{exe_dir}`占位符  |
| `image_folder`      | string | `{dataset_dir}/images`              | 图像文件夹路径，支持嵌套占位符           |
| `work_dir`          | string | `{exe_dir}/globalsfm_pipeline_work` | 工作目录路径                             |
| `preprocess_type`   | string | `"posdk"`                           | 预处理类型：`openmvg`, `opencv`, `posdk` |
| `enable_evaluation` | bool   | `true`                              | 是否启用精度评估                         |

#### 管道控制选项

| 参数名                       | 类型   | 默认值      | 说明                                                          |
| ---------------------------- | ------ | ----------- | ------------------------------------------------------------- |
| `enable_3d_points_output`    | bool   | `true`      | 是否输出3D点云数据                                            |
| `enable_meshlab_export`      | bool   | `true`      | 是否导出Meshlab可视化工程文件                                 |
| `enable_posdk2colmap_export` | bool   | `true`      | 是否导出PoSDK重建结果到Colmap格式（相机、图像、3D点）         |
| `enable_csv_export`          | bool   | `true`      | 是否启用评估结果CSV导出                                       |
| `evaluation_print_mode`      | string | `"summary"` | 评估结果打印模式：`none`, `summary`, `detailed`, `comparison` |
| `compared_pipelines`         | string | `""`        | 对比算法列表，如`"openmvg,COLMAP"`                            |

#### 性能优化选项

| 参数名                   | 类型 | 默认值 | 说明             |
| ------------------------ | ---- | ------ | ---------------- |
| `enable_profiling`       | bool | `true` | 启用性能分析     |
| `enable_data_statistics` | bool | `true` | 启用数据统计功能 |

#### 配置文件示例

推荐在`{exe_dir}/../configs/methods/globalsfm_pipeline.ini`中配置：

```ini
[globalsfm_pipeline]
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

```bash
# 默认模式：使用配置文件参数
./PoSDK

# 自定义模式：命令行参数覆盖配置
./PoSDK --preset=custom --dataset_dir=/path/to/dataset
```

#### 输出结果

- **位姿数据**：`data_global_poses` - 全局相机位姿
- **3D点云**：`data_world_3dpoints` - 稀疏3D重建点（可选）
- **评估报告**：CSV格式的精度分析和算法对比
- **可视化文件**：Meshlab工程文件（.mlp）用于三维显示

详细参数说明请参考配置文件中的注释。

## 插件分类

### 相机标定类
- `method_calibrator`: 针孔相机模型标定

### 可视化类
- `method_matches_visualizer`: 特征匹配可视化

### 全局旋转估计类
- `method_rotation_averaging_Chatterjee`: Chatterjee算法
- `method_rotation_averaging`: GraphOptim工具

### 双视图位姿估计类
- `opencv_two_view_estimator`: OpenCV实现
- `opengv_model_estimator`: OpenGV实现
- `poselib_model_estimator`: PoseLib实现
- `two_view_estimator`: PoSDK集成实现

### 完整管道类
- `openmvg_pipeline`: OpenMVG完整SfM流程
- **`globalsfm_pipeline`**: PoSDK完整SfM流程（核心推荐）

## 使用说明

所有插件均通过PoSDK工厂模式创建和使用：

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
