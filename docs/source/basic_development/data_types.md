# Built-in Data Types

PoSDK provides the following built-in DataIO data types for managing storage and serialization of common SfM data:

| Built-in Data Type         | Core Data Type                                                                | Save/Load Supported Formats | Supports Auto Evaluation |
| -------------------------- | ----------------------------------------------------------------------------- | --------------------------- | ------------------------ |
| `data_images`              | [`ImagePaths`](../appendices/appendix_a_types.md#image-paths)                 | folder                      | ✗                        |
| `data_camera_models`       | [`CameraModels`](../appendices/appendix_a_types.md#camera-models)             | .pb                         | ✗                        |
| `data_feature`             | [`ImageFeatureInfo*`](../appendices/appendix_a_types.md#image-feature-info)   | .pb                         | ✗                        |
| `data_features`            | [`FeaturesInfo`](../appendices/appendix_a_types.md#features-info)             | .pb                         | ✗                        |
| `data_matches`             | [`Matches`](../appendices/appendix_a_types.md#matches)                        | .pb                         | ✗                        |
| `data_bearing_pairs`       | [`BearingPairs`](../appendices/appendix_a_types.md#bearing-pairs)             | .pb                         | ✗                        |
| `data_tracks`              | [`Tracks`](../appendices/appendix_a_types.md#tracks)                          | .pb,.tracks                 | ✗                        |
| `data_relative_rotations`  | [`RelativeRotations`](../appendices/appendix_a_types.md#relative-rotations)   | -                           | ✗                        |
| `data_relative_poses`      | [`RelativePoses`](../appendices/appendix_a_types.md#relative-poses)           | .g2o, .pb                   | ✓                        |
| `data_global_rotations`    | [`GlobalRotations`](../appendices/appendix_a_types.md#global-rotations)       | .rot                        | ✗                        |
| `data_global_translations` | [`GlobalTranslations`](../appendices/appendix_a_types.md#global-translations) | -                           | ✗                        |
| `data_global_poses`        | [`GlobalPoses`](../appendices/appendix_a_types.md#global-poses)               | .po, .g2o, .rot             | ✓                        |
| `data_points_3d`           | [`WorldPointInfo`](../appendices/appendix_a_types.md#world-point-info)        | .txt, .ply (+.ids)          | ✗                        |

```{note}
- **Save/Load Supported Formats**:
  - `.pb` - Protobuf binary format (high performance, cross-platform)
  - `.txt` - Text format (human-readable)
  - `.ply` - PLY point cloud format (only for 3D point data)
  - `+.ids` - PLY format usage status auxiliary file
- **Auto Evaluation**: Data types marked with ✓ have overloaded `Evaluate()` function, supporting automatic comparison evaluation with ground truth data
- **Core Data Type Detailed Description**: Please refer to [Appendix A: PoSDK Core Data Types](../appendices/appendix_a_types.md)
```

## Common SfM Data Types

PoSDK's type system adopts modular design, distributing common SfM data types across multiple header files for easier maintenance. These types include:

- **Basic Types**: Identifiers, matrix vectors, smart pointers, etc.
- **Camera System**: Camera models, intrinsics, distortion parameters
- **Feature System**: Feature points, descriptors, image feature management
- **Match System**: Feature matching, viewing vectors
- **Track System**: Observation information, track management
- **Pose System**: Relative poses, global poses, pose formats
- **3D Point Cloud**: World coordinate points, point cloud management

**Detailed Description**: For complete definitions, member variables, available methods, and usage examples of all common SfM data types, please refer to [Appendix A: PoSDK Core Data Types](../appendices/appendix_a_types.md).
