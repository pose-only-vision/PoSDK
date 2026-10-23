# 内置数据类型

## 在 GUI 中查看与重命名

1. 在左侧 **Modules → Data** 中选中数据模块，在右侧 **Configuration** 点击 **数据结构说明**，查看用途、所含数据和使用方式。也可以右键模块，选择 **数据结构说明…**。
2. 需要进一步解释时，点击说明窗口中的 **询问 Agent**，检查输入框中的问题后发送。
3. 右键数据模块，选择 **重命名模块…**，分别填写中文名称和英文名称。重名会显示冲突插件；更换名称后保存。留空使用默认名称。

名称随界面语言切换，模块列表、画布节点与数据端口同步更新。单独为画布节点设置的名称保留。

单项用名称，多个同类项用“集”，数据容器用“数据包”。

| 数据标识 | 中文名称 | 英文名称 |
| --- | --- | --- |
| `data_images` | 图像集 | Image set |
| `data_feature` | 特征 | Image features |
| `data_features` | 特征集 | Feature set |
| `data_match` | 匹配 | Image-pair matches |
| `data_matches` | 匹配集 | Match set |
| `data_relative_poses` | 相对位姿集 | Relative pose set |
| `data_global_poses` | 全局位姿集 | Global pose set |
| `data_package` | 数据包 | Data package |
| `dfeatures` | 特征数据包 | Feature data package |
| `dmatches` | 匹配数据包 | Match data package |

## 文件格式

PoMVG提供了以下内置DataIO数据类型，用于管理SfM常用数据的存储和序列化：

加载与保存是不同的能力；完整的 26 类统计、插件类型和限制见 [DataIO 文件格式表](dataio_file_formats.md)。

| 内置数据类型 | 核心数据类型 | 加载格式 | 保存格式 |
| --- | --- | --- | --- |
| `data_images` | `ImagePaths` | folder, .imglist | .imglist |
| `data_camera_models` | `CameraModels` | .pb, .yaml, .yml | .pb, .yaml, .yml |
| `data_feature` | `ImageFeatureInfo*` | .pb | .pb |
| `data_features` | `FeaturesInfo` | .pb | .pb |
| `data_match` | `ViewPair + IdMatches` | .pb | .pb |
| `data_matches` | `Matches` | .pb | .pb |
| `data_bearing_pairs` | `BearingPairs` | .pb | .pb |
| `data_tracks` | `Tracks` | .pb, .tracks, .icoords | .pb, .tracks |
| `data_relative_rotations` | `RelativeRotations` | .pb | .pb |
| `data_relative_poses` | `RelativePoses` | .g2o, .pb | .g2o, .pb |
| `data_global_rotations` | `GlobalRotations` | .pb, .rot | .pb |
| `data_global_translations` | `GlobalTranslations` | .pb | .pb |
| `data_global_poses` | `GlobalPoses` | .pb, .po, .g2o, .rot | .pb, .po, .g2o, .csv |
| `data_points_3d` | `WorldPointInfo` | .txt, .ply (+.ids), .pb | .txt, .ply (+.ids), .pb |
| `data_piecewise_surfaces` | `PiecewiseSurfaces` | .pb | .pb |
| `data_scene_blocks` | `SceneBlockMap` | .pb | .pb |

```{note}
- **Save/Load支持格式**：
  - `.pb` - Protobuf二进制格式（高性能，跨平台）
  - `.txt` - 文本格式（人类可读）
  - `.ply` - PLY点云格式（仅用于3D点数据）
  - `+.ids` - PLY格式的使用状态辅助文件
- **核心数据类型详细说明**：请参阅 [附录A：PoMVG核心数据类型](../appendices/appendix_a_types.md)
```

## SfM常用数据类型

PoMVG的类型系统采用模块化设计，将常用的SfM数据类型分散在多个头文件中以便维护。这些类型包括：

- **基础类型**：标识符、矩阵向量、智能指针等
- **相机系统**：相机模型、内参、畸变参数
- **特征系统**：特征点、描述子、图像特征管理
- **匹配系统**：特征匹配、视线向量
- **轨迹系统**：观测信息、轨迹管理
- **位姿系统**：相对位姿、全局位姿、位姿格式
- **3D点云**：世界坐标点、点云管理

**详细说明**：所有SfM常用数据类型的完整定义、成员变量、可用方法和使用示例，请参阅 [附录A：PoMVG核心数据类型](../appendices/appendix_a_types.md)。
