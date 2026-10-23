# ETH3D 数据集

ETH3D 提供带相机标定的高分辨率多视图场景。PoMVG 的 ETH3D DatasetLoader 读取去畸变 DSLR 图像，以及对应标定中的相机内参和位姿，用于重建和精度评估。图像与标定需要分别下载，入口见 [ETH3D 数据集页面](https://www.eth3d.net/datasets)。

## 准备数据

每个场景使用两份官方压缩包：`{场景}_dslr_undistorted.7z` 为去畸变图像，`{场景}_dslr_calibration_undistorted.7z` 为标定。解压后放在同一个场景目录中：

```text
ETH3D/                         ← 选择这个根目录
├── facade/
│   ├── images/
│   │   └── dslr_images_undistorted/
│   └── dslr_calibration_undistorted/
│       ├── cameras.txt
│       └── images.txt
└── meadow/
```

不要把 `images` 或 `dslr_images_undistorted` 选成根目录。图像和标定分开下载时，也要解压到对应场景下面。

| 路径 | 内容与用途 |
| --- | --- |
| `images/dslr_images_undistorted/` | 去畸变 DSLR 图像。加载器参数 `image_folder` 的默认值。 |
| `dslr_calibration_undistorted/cameras.txt` | COLMAP 相机内参。 |
| `dslr_calibration_undistorted/images.txt` | 图像名、四元数位姿和平移。加载器参数 `gt_folder` 指向这一层目录。 |

加载器默认 `specific_data=facade`、`device_type=dslr`、`split=training`。`specific_data` 填写根目录下的一个场景名。只有当 `dataset_dir` 已经直接指向该场景时才留空；留空不会自动遍历全部场景。

## 在 GUI 中运行

1. 在工作流库中创建 ETH3D 重建工作流。
2. 打开 **ETH3D DatasetLoader**，选择上面的 `ETH3D` 根目录，并点击 **关联到当前节点**。
3. 将场景名设为已解压的目录，例如 `facade` 或 `meadow`。
4. 确认图像相对目录为 `images/dslr_images_undistorted`，真值相对目录为 `dslr_calibration_undistorted`。保存后运行。

目录关联、切换场景，以及循环处理根目录下的全部场景，见 [快速开始](../../installation/cli_gui_guide.md)。

## 位姿

`images.txt` 中的 `qw qx qy qz` 和其后的平移是 COLMAP 的世界到相机变换。加载器按 `PoseFormat::RwTc` 保存。对世界点 $X_w$，相机坐标为 $X_c = R X_w + t$，其中 $R$ 由该四元数得到，$t$ 为文件中的平移。

COLMAP 的 `image_id` 不一定从 0 连续编号。加载器按编号将其重映射为 `0, 1, 2, ...`。`compute_relative_poses=true` 时，相对位姿由这些全局位姿计算。

位姿格式见 [位姿约定](../conventions/posdk.md)。已有场景结果见 [平台对比](../../benchmark_comparison/index.rst)。
