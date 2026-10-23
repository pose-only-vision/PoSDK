# 1DSfM 数据集

1DSfM 提供互联网地标图像的特征坐标、匹配轨迹、双视图几何及参考重建，可用于测试无序图像的相机位姿恢复和三维重建。数据与原始图像分开下载，入口见 [1DSfM 官方页面](https://www.cs.cornell.edu/projects/1dsfm/)。

## 准备数据

下载并解压官方数值数据包，将每个场景保留为独立文件夹，例如：

```text
1DSfM/
└── Alamo/
    ├── cc.txt
    ├── EGs.txt
    ├── coords.txt
    ├── tracks.txt
    ├── list.txt
    ├── gt_bundle.out
    └── images/          # 可选：单独下载的原图
```

| 文件 | 内容与用途 |
| --- | --- |
| `cc.txt` | 当前连通分量中的原始图像编号。 |
| `EGs.txt` | 图像对的相对旋转与平移方向。 |
| `coords.txt` | 各图像的特征坐标、焦距、主点及特征颜色。 |
| `tracks.txt` | 跨图像的特征对应关系；每条轨迹记录图像编号与特征编号。 |
| `list.txt` | 图像名称及焦距；图像编号对应文件中的顺序。 |
| `gt_bundle.out` / `bundle.out` | Bundler 格式的参考重建。加载器默认查找 `gt_bundle.out`；文件名为 `bundle.out` 时，将 `bundle_file` 改为 `bundle.out`。 |

文件格式的完整定义见作者的 [SfM_Init 数据说明](https://github.com/wilsonkl/SfM_Init#data)。保留官方编号和文件内容，不要单独重排图像列表或轨迹。

## 在 GUI 中运行

1. 点击顶部 **Workflow**，在工作流库搜索 **1DSfM**，创建读取官方元数据的重建工作流。
2. 打开 **1DSfM DatasetLoader**，点击 **选择根目录**，选择本地 `1DSfM` 文件夹，再点击 **关联到当前节点**。
3. 选择场景，例如 **Alamo**。如果直接选择了 `Alamo` 场景文件夹，将 **Scene / 场景** 留空。
4. 检查 **Metadata Folder / 元数据子目录**：上表文件直接位于场景下时留空；位于子文件夹时填写该子文件夹名称。
5. 检查参考重建文件名，保存工作流并运行。结束后打开 **可视化** 查看点云与相机，打开 **Evaluator（评估）** 和 **Profiler（性能）** 查看指标与耗时。

只使用已提供的特征和轨迹进行重建时，可以不下载原图，保持 `require_images=false`。需要查看原图或匹配叠加时，将图像放入 `images/`，或修改 **Image Folder / 图像子目录**。

## 输入与坐标处理

加载器会统一映射原始图像编号，读取相机标定，并将官方 SIFT 像素坐标转换为平台使用的零基像素坐标。保持 `convert_sift_to_zero_based_pixels=true`，不要在导入前重复转换坐标。

默认只为 `EGs.txt` 中存在的图像对生成候选匹配。重建工作流继续进行双视图验证、旋转平均、建轨和位姿恢复；建轨应使用**双视图验证后的匹配输出**。

## 查看结果

- 在三维视图中检查点云、相机分布和视图恢复情况。
- 在评估面板中查看参考位姿对齐后的误差，比较时使用相同的视图集合。
- 官方 Bundle 是参考重建，不是独立测量的真值；平台对比页的 1DSfM 结果使用重投影误差，并同时列出恢复视图和点数。

已有场景结果见 [平台对比](../../benchmark_comparison/index.rst)，坐标表示见 [位姿约定](../conventions/posdk.md)。
