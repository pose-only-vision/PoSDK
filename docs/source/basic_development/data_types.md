# Built-in Data Types

## View and rename data in the GUI

1. Select a module in **Modules → Data**, then click **Data Structure** in **Configuration** to read its purpose, contents and usage. You can also right-click the module and choose **Data Structure…**.
2. For more help, click **Ask Agent**, review the question in the composer, then send it.
3. Right-click the module and choose **Rename Module…**. Enter Chinese and English names. If a name conflicts, change it before saving. Leave a field blank to use its default.

Names follow the interface language and update the module list, canvas nodes and data ports. Names assigned to individual canvas nodes are retained.

Use a direct name for one item, “set” for a collection, and “data package” for a container.

| Data identifier | Chinese name | English name |
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

## File formats

PoMVG provides the following built-in DataIO data types for managing storage and serialization of common SfM data:

Loading and saving are distinct capabilities.

| Built-in data type | Core payload | Load formats | Save formats |
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
- **Save/Load Supported Formats**:
  - `.pb` - Protobuf binary format (high performance, cross-platform)
  - `.txt` - Text format (human-readable)
  - `.ply` - PLY point cloud format (only for 3D point data)
  - `+.ids` - PLY format usage status auxiliary file
- **Core Data Type Detailed Description**: Please refer to [Appendix A: PoMVG Core Data Types](../appendices/appendix_a_types.md)
```

## Common SfM Data Types

PoMVG's type system adopts modular design, distributing common SfM data types across multiple header files for easier maintenance. These types include:

- **Basic Types**: Identifiers, matrix vectors, smart pointers, etc.
- **Camera System**: Camera models, intrinsics, distortion parameters
- **Feature System**: Feature points, descriptors, image feature management
- **Match System**: Feature matching, viewing vectors
- **Track System**: Observation information, track management
- **Pose System**: Relative poses, global poses, pose formats
- **3D Point Cloud**: World coordinate points, point cloud management

**Detailed Description**: For complete definitions, member variables, available methods, and usage examples of all common SfM data types, please refer to [Appendix A: PoMVG Core Data Types](../appendices/appendix_a_types.md).
