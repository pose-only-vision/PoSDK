# 内置数据类型

PoSDK提供了以下内置DataIO数据类型，用于管理SfM常用数据的存储和序列化：

| 内置数据类型               | 核心数据类型                                                                  | Save/Load支持格式  | 是否支持自动评估 |
| -------------------------- | ----------------------------------------------------------------------------- | ------------------ | ---------------- |
| `data_images`              | [`ImagePaths`](../appendices/appendix_a_types.md#image-paths)                 | folder             | ✗                |
| `data_camera_models`       | [`CameraModels`](../appendices/appendix_a_types.md#camera-models)             | .pb                | ✗                |
| `data_feature`             | [`ImageFeatureInfo*`](../appendices/appendix_a_types.md#image-feature-info)   | .pb                | ✗                |
| `data_features`            | [`FeaturesInfo`](../appendices/appendix_a_types.md#features-info)             | .pb                | ✗                |
| `data_matches`             | [`Matches`](../appendices/appendix_a_types.md#matches)                        | .pb                | ✗                |
| `data_bearing_pairs`       | [`BearingPairs`](../appendices/appendix_a_types.md#bearing-pairs)             | .pb                | ✗                |
| `data_tracks`              | [`Tracks`](../appendices/appendix_a_types.md#tracks)                          | .pb,.tracks        | ✗                |
| `data_relative_rotations`  | [`RelativeRotations`](../appendices/appendix_a_types.md#relative-rotations)   | -                  | ✗                |
| `data_relative_poses`      | [`RelativePoses`](../appendices/appendix_a_types.md#relative-poses)           | .g2o, .pb          | ✓                |
| `data_global_rotations`    | [`GlobalRotations`](../appendices/appendix_a_types.md#global-rotations)       | .rot               | ✗                |
| `data_global_translations` | [`GlobalTranslations`](../appendices/appendix_a_types.md#global-translations) | -                  | ✗                |
| `data_global_poses`        | [`GlobalPoses`](../appendices/appendix_a_types.md#global-poses)               | .po, .g2o, .rot    | ✓                |
| `data_points_3d`           | [`WorldPointInfo`](../appendices/appendix_a_types.md#world-point-info)        | .txt, .ply (+.ids) | ✗                |

```{note}
- **Save/Load支持格式**：
  - `.pb` - Protobuf二进制格式（高性能，跨平台）
  - `.txt` - 文本格式（人类可读）
  - `.ply` - PLY点云格式（仅用于3D点数据）
  - `+.ids` - PLY格式的使用状态辅助文件
- **自动评估**：标记为✓的数据类型重载了`Evaluate()`函数，支持与真值数据自动对比评估
- **核心数据类型详细说明**：请参阅 [附录A：PoSDK核心数据类型](../appendices/appendix_a_types.md)
```

## SfM常用数据类型

PoSDK的类型系统采用模块化设计，将常用的SfM数据类型分散在多个头文件中以便维护。这些类型包括：

- **基础类型**：标识符、矩阵向量、智能指针等
- **相机系统**：相机模型、内参、畸变参数
- **特征系统**：特征点、描述子、图像特征管理
- **匹配系统**：特征匹配、视线向量
- **轨迹系统**：观测信息、轨迹管理
- **位姿系统**：相对位姿、全局位姿、位姿格式
- **3D点云**：世界坐标点、点云管理

**详细说明**：所有SfM常用数据类型的完整定义、成员变量、可用方法和使用示例，请参阅 [附录A：PoSDK核心数据类型](../appendices/appendix_a_types.md)。