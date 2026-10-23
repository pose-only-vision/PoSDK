# Data 模块文件格式与 GUI 加载入口

核对日期：2026-09-07。口径：以 `po_core/src/internal/factory/factory_lookup.hpp` 的注册类型、内置 DataIO 的 `Load/Save` 实现，以及 `plugins/data` 和方法目录中注册的数据插件为准。不是根据扩展名、旧注释或 `GetSupportedOutputTypes()` 单独推断加载能力。

## 统计结论

本次源码共核对 **26 个 Data 类型**（21 个内置类型、5 个插件类型，含示例插件）。其中：

- **18 个**实现了可用于独立文件加载的 DataIO。
- **2 个**是仅支持 JSON 导出的诊断报告，尚未实现 `Load()`。
- **6 个**没有独立文件往返功能：2 个内存类型、3 个包容器、1 个占位示例。

这是源码统计，不代表每台机器已安装全部插件；GUI 以当前激活的 DataIO 实现为准，插件不可用时不会虚报文件支持。`.pb` 必须由对应 Data 类型写出；相同扩展名不代表不同类型的数据可以互换。

## 完整格式表

| Data 类型 | 双击面板可加载的文件 | 原生支持保存的格式 | 说明 / 源码依据 |
| --- | --- | --- | --- |
| `data_images` | `.imglist`；图片目录 | `.imglist` | `po_core/src/internal/data/data_images.cpp`；目录扫描支持 jpg/jpeg/png/bmp/tif/tiff/webp。图片原文件使用“选择图片/目录”入口，不是 Protobuf。 |
| `data_camera_models` | `.pb`, `.yaml`, `.yml` | `.pb`, `.yaml`, `.yml` | `data_camera_models.hpp`；YAML 为 PoMVG 相机模型结构，不是任意第三方标定 YAML。 |
| `data_feature` | `.pb` | `.pb` | `data_feature.hpp` + `PbDataIO`；单图特征。 |
| `data_features` | `.pb` | `.pb` | `data_features.hpp/.cpp` + `PbDataIO`；多图特征集合。 |
| `data_match` | `.pb` | `.pb` | `data_match.hpp` + `PbDataIO`；单图像对匹配。 |
| `data_matches` | `.pb` | `.pb` | `data_matches.hpp/.cpp` + `PbDataIO`；匹配集合。 |
| `data_bearing_pairs` | `.pb` | `.pb` | `data_bearing_pairs.hpp` + `PbDataIO`。 |
| `data_tracks` | `.pb`, `.tracks`, **`.icoords`** | `.pb`, `.tracks` | `data_tracks.cpp`；`.icoords` 为仅导入的像素观测文本，不能从输出格式声明推导。 |
| `data_relative_rotations` | `.pb` | `.pb` | `data_relative_rotations.cpp`；不是 `.rot` 文本类型。 |
| `data_relative_poses` | `.pb`, `.g2o` | `.g2o`, `.pb` | `data_relative_poses.hpp`；g2o 存储相对位姿边。 |
| `data_global_rotations` | `.pb`, **`.rot`** | `.pb` | `data_global_rotations.cpp` 的非 pb 分支调用 `file::SetupOrientation`。GUI 使用 `.rot` 作为该旧版文本格式的标准扩展名。 |
| `data_global_translations` | `.pb` | `.pb` | `data_global_translations.cpp`。 |
| `data_global_poses` | `.pb`, `.po`, `.g2o`, `.rot` | `.pb`, `.po`, `.g2o`, **`.csv`** | `data_global_poses.cpp`；`.rot` 只加载旋转，`.csv` 是状态报告导出，不能恢复位姿。 |
| `data_points_3d` | `.pb`, `.txt`, `.ply` | `.txt`, `.ply`, `.pb` | `data_points_3d.cpp`；PLY 的 `.ids` 是伴随文件，不是独立加载格式。TXT 首行为点数，后续各行为 x y z used_flag。 |
| `data_piecewise_surfaces` | `.pb` | `.pb` | `data_piecewise_surfaces.cpp`；加载包含结构与版本校验。 |
| `data_scene_blocks` | `.pb` | `.pb` | `data_scene_blocks.cpp`；加载包含场景块结构与版本校验。 |
| `data_pose_graph_quality` | 无 | 无 | `data_pose_graph_quality.hpp`：继承 `DataMap` 的临时内存诊断数据。 |
| `data_costs` | 无 | 无 | `internal/robust/data_cost.hpp`：没有实现文件 Load/Save。 |
| `data_package` | 无独立包文件入口 | 递归保存包内已有数据项 | `internal/data_package.cpp`；不是可直接读取的统一 package.pb。 |
| `dfeatures` | 无独立包文件入口 | 递归保存包内已有数据项 | `DataPackageVector` 别名；Load 依赖已存在的子对象和它们自己的保存路径。 |
| `dmatches` | 无独立包文件入口 | 递归保存包内已有数据项 | 同上。 |
| `data_img_point_calib` | `.txt` | `.txt` | `plugins/data/DataImgPointCalib/DataImgPointCalib.cpp`；四列空白分隔文本。 |
| `data_generic_scalib` | `.scalib`，embedded v3 | `.scalib` | `plugins/data/DataGenericScalib/DataGenericScalib.cpp`；旧版本原生 Load 会返回成功但不恢复完整数据，因此 GUI 拒绝将其视为成功加载。 |
| `data_reconstruction_quality` | 无（未实现 Load） | `.json` | `plugins/methods/MethodHierarchicalAnalyticalReconstruction/hierarchical_analytical_reconstruction.hpp/.cpp`；运行结果导出。 |
| `data_reconstruction_consistency_diagnostics` | 无（未实现 Load） | `.json` | `plugins/methods/MethodReconstructionConsistencyPA/reconstruction_consistency_pa.hpp/.cpp`；运行结果导出。 |
| `data_example` | 无实际实现 | 无实际实现 | `plugins/data/DataExample.hpp` 中 Load/Save 仅打印日志并返回 true，没有读写文件。 |

表中未带目录的内置类型源文件均位于 `po_core/src/internal/data/`。公共二进制序列化逻辑位于 `po_core/src/internal/pb_dataio.hpp`。

## 双击后的操作

1. 在 Behavior Workspace 双击任意 Data 节点。
2. 没有专用输入界面的类型直接显示“加载文件 / 另存为”。有专用界面的类型（如 images、标定点）点击窗口顶部的“数据加载 / 保存”。
3. 文件对话框默认显示“全部支持的格式”，也可以按单一扩展名筛选。加载由该类型实际的 DataIO 执行，不是只记录文件名。
4. 加载成功后仅更新当前节点的关联；失败保留原关联。预览、异步结果和保存操作仍按节点隔离。
5. “另存为”从已关联来源导出副本；仅导出型报告不能从 JSON 恢复，因此不能在此假装完成 Load→Save 往返。运行中的报告数据可由现有 Output Control 导出。

## 格式限制

- `.rot`：首个值为视图数，随后每个视图 9 个旋转矩阵数值，按列顺序读取；不含平移。
- `.icoords`：文件头为 num_view num_pts num_obs，之后为 view_id pts_id x y；原生加载器对 x/y 取负，并标记为非归一化观测，且 feature_id 未知。不要把它当作 `.tracks` 的等价无损存储。
- `.tracks`：归一化轨迹文本。原生保存逻辑可能按当前归一化状态进行格式处理；`.pb` 更适合保留 PoMVG 内部完整状态。
- 标定点 `.txt`：source_image_index point_index_1based x_px y_px，支持 `#` 元数据注释和 NaN。原生实现忽略 file_type，但并不解析逗号分隔 CSV；不能因为 `.csv` 后缀曾出现在旧界面就宣称支持通用 CSV。
- 个别原生函数会忽略扩展名或把未知扩展名回退为 Protobuf；这些不是新增格式，不计入格式统计。

GUI 实现：`GUI/backend/DataIOModel.cpp`、`GUI/PoSDK_UI/PoSDK_UIContent/styles/input_control/data_io.qml`、`components/ModuleLogDialog.qml`。回归测试：`GUI/tests/DataIOModelTest.cpp`。
