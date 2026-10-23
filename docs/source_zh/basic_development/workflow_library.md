# 工作流库与双视图估计基准

PoMVG GUI 将“插件自身是否能加载”与“多个模块组成的算法实验”分成两个工具：

- **Plugin Test Lab**：验证单个插件的 ABI、依赖、加载、接口契约和插件自带测试。
- **Workflow Library**：提供可复用的 `.posflow` 工作流，例如“仿真器 → 双视图估计 → Evaluator/Profiler”。

前者适合作为每次构建后的快速门禁；后者适合比较算法精度、噪声鲁棒性和运行时间趋势。工作流运行通过现有的独立 `posdk_runner` 子进程和精确插件快照完成，不会绕过 PluginPathManager。

## 打开内置工作流

有两个等价入口：

1. 在 **Behavior Workspace** 标题栏点击 **Workflow**。
2. 在 **File** 菜单选择 **New from Workflow Library…**。

切换到 **Test**，可按 `Visual Geometry`、`SfM` 或 `Algorithm Evaluation` 过滤。选择 **双视图估计算法基准测试** 后，右侧会明确列出：

- 测试目的和具体覆盖项；
- 所需插件及缺失原因；
- 预计运行次数；
- 包来源和版本。

点击 **Create editable project** 后，GUI 不会直接修改或运行只读内置包，而是先在 `Documents/PoSDK Workflows` 中创建普通 `.posdk` 工程。输出默认写入 `Documents/PoSDK Runs/<workflow-id>`，因此移动 `.app` 后仍然有效，也不会尝试写入 App Bundle。

### Combine：当前默认 v27

ETH3D、Strecha 和 1DSfM 三个入口当前默认使用 **v27.0.0**，由 `workflows/profiles/current-combined.json` 指定。ETH3D 与 Strecha 共用 HIGH、0.75 匹配比率、双向匹配、每图最多 30,000 特征和 8 线程；1DSfM 读取官方特征与匹配。已有工程不会自动迁移。

### Combine：历史 v23（认证 B／稀疏按需调度／PA20）

在 **通用工作流 → SfM** 中选择 **PoMVG Combine — ETH3D**、
**PoMVG Combine — Strecha** 或 **PoMVG Combine — 1DSfM**。
以下记录历史 **v23：认证 B／稀疏按需调度／PA20**，对应当时恢复的
`po_core 97ea607c`（求解源码等同 `e2dbfe57`）。历史版本保留在下拉框中，
不覆盖用户已有的 `.posdk` 工程；旧实验缺少配套原生选项时显示为仅归档。

点击 **创建可编辑工程**，然后双击最左侧 Loader，在其 InputControl 中关联
数据集共同根目录，并选择 `specific_data`：

| 入口 | 数据输入 | 默认场景 |
| --- | --- | --- |
| ETH3D | 含图像和标定的 ETH3D 场景共同根目录 | `meadow` |
| Strecha | 含 `images`、`gt_dense_cameras` 的场景共同根目录 | `castle-P30` |
| 1DSfM | 官方特征、匹配及标定元数据共同根目录；图像可选 | `Alamo` |

默认一次运行一个场景。选择其他场景时修改 `specific_data`；批量运行需显式为
场景建立 Loop。内置包不含本机数据路径，不需要旧实验目录或外部 checkpoint。
ETH3D/Strecha 会计算图像特征与匹配，1DSfM 使用官方提供的观测。
GT 只进入评估，不进入旋转平均、初始建轨或 PoseEngine 求解。

```text
Loader / 图像预处理 → 双视图估计 → RA
raw matches + features + 双视图认证种子 Prior → raw_candidates Tracks
  → Combine（LiGT / PoseLib P3P+PA2 / GroupSAC 分流）
  → Fresh5 → 全局 PA20 → HAR2 / G2 清理
```

双视图保持 Schedule ON，明确使用 `pair_scheduler_mode=demand`：最低匹配数为20，
优先选择匹配较多的候选边，按成功位姿图的连通、关联度数和闭环缺口分轮补边。
高匹配边也按需选取，不要求所有≥100匹配的图像对都估计。
估计后的几何、内点数量和比例门禁保持原配置；被调度估计不等于通过认证。
v21和v22历史包保留；v23相比v22只切换调度模式，后端求解流程不变。

raw 候选不直接成为优化或 HAR 的内点。v23 锁定认证版的观测状态、几何信息、
失活前 R 复核及边界检查，Core 保留至少 10 条独立有效 G3 内点。
Initial PA 关闭 G2 平移贡献，保留无窗口的逐批全图两步细化；末端 PA 为 20 次。
不启用后续的观测预算、PCG、阶段末二轮恢复或逐轨消点实验。

输出控制保存前端特征、raw/认证匹配、Rij、相对位姿、RA 和初始轨 checkpoint，
以及 Combine、Fresh、最终位姿、最终有效 Track、质量证书和 PLY 点云。
分别保留 C/R/U/A 状态，位姿评估使用 C/R/U。后续对比可复用已导出的前端
checkpoint；重算图像前端与复用旧 checkpoint 的输入来源不同，不能宣称结果逐字节复现。

版本索引来自 App 内置的 `workflows/catalog-families.json`，以包 ID、版本和
manifest/project SHA-256 精确绑定。验证当前包使用
`python3 -B tools/generate_builtin_sfm_workflows.py --check --current-combined`；
版本索引检查使用 `python3 -B tools/generate_workflow_families.py`。
更新 GUI 后重新打开工作流库，新建当前默认版本工程；v23 作为历史版本保留，已有工程不会自动合并参数。

### Ours：PoseEngine stable-R2（Strecha）

通用工作流中的 **Ours：PoseEngine stable-R2 纯位姿重建（Strecha）** 固化

```text
Strecha loader → OpenMVG SIFT
  → OpenCV USAC-MAGSAC（旋转 reservoir）→ Chatterjee RA → Track=2
  → PoSDKPoseEngine(initial, backend_profile=stable_r2)   # stable-R2 + PA-1
  → Fresh5-1
  → PoSDKPoseEngine(expansion, backend_profile=stable_r2) # Frontier recovery + PA-2
  → Fresh5-2
  → posdk_reconstruction（HAR + attraction + clean）
```

两个 PoseEngine 节点的 INI 只有隔离的 `[PoSDKPoseEngine]` 节，通过
`backend_profile=stable_r2` 进入冻结的 historical stable-R2 后端；不会再嵌套
`[LiGTStable]` / `[method_PA]`，也不会走独立 G3-consensus Core。默认场景仍是
`castle-P30`。真值 Pose 只连 Evaluator 端口，不进入求解。

### Ours：PoseEngine stable-R2（1DSfM）

通用工作流中的 **Ours：PoseEngine stable-R2 纯位姿重建（1DSfM）** 与 ETH3D
Images 的 PoseEngine 后端相同，前端改为官方 1DSfM features/matches：

```text
1DSfM loader（Alamo features/matches）
  → OpenCV USAC-MAGSAC（旋转 reservoir）→ Chatterjee RA → Track=2
  → PoSDKPoseEngine(initial, backend_profile=stable_r2)   # stable-R2 + PA-1
  → Fresh5-1
  → PoSDKPoseEngine(expansion, backend_profile=stable_r2) # Frontier recovery + PA-2
  → Fresh5-2
  → posdk_reconstruction（HAR + attraction + clean）
```

没有 Preprocessor。Loader 自带的真值 Pose、Track 与点云只连 Evaluator。默认
场景是 `Alamo`。

### Ours：SfMEngine P1-full（ETH3D 图像）

通用工作流中的 **Ours：SfMEngine P1-full（ETH3D 图像输入）** 保持与 2026-09-10
P1-full 相同的图像前端，并把后端收成一个 `posdk_sfm_engine` 节点：

```text
ETH3D loader → OpenMVG SIFT（30k）
  → OpenCV USAC-MAGSAC（旋转 reservoir）→ Chatterjee RA → Track=2
  → posdk_sfm_engine   # 冻结 R2/PA、Fresh5、弱位姿、HAR+吸附+清理
```

公开 INI 只有 `backend_contract=p1_full`。子 LiGT/PA/Fresh/HAR 配置留在插件内。
默认场景是 `facade`。真值 Pose 只连 Evaluator。也可以从 Modules 把 **PoMVG SfM
Engine** 拖到已有 Track、RA Pose、相机、特征、raw matches 和 Rij 的工作流上。

## OpenCV-PoEngine 真实数据重建工作流

切换到 **通用工作流**，可以直接创建下列三套已知工作流：

- **OpenCV-PoEngine-Strecha 重建**；
- **OpenCV-PoEngine-ETH3D 重建**；
- **OpenCV-PoEngine-1DSfM 重建**。

另外提供两套与 OpenCV-PoEngine 严格区分、以 `Paper-` 为前缀的论文协议：

- **论文版 GNC-RANSAC-Gate-PoEngine-Strecha**：固定单线程
  `method_GNC_RANSAC_LiRP`，20 点采样、50k 假设预算、seed 31、严格大于
  50 个匹配、默认 `4e-4` 内点阈值、一次受 MSAC 评分保护的最终 LiRP 一致集
  重估和六步 PPO angle-2sin/Cauchy 精化；按论文复现实验要求显式增加无真值的
  `global_sync` 0.5 度图门禁；
- **论文版 Direct-Gate-PoEngine-ETH3D**：复现论文表格和临时实验报告中的
  LiRPFast Direct 工作点，要求每个视图对至少 201 个对应点，使用 100 个对应点
  的候选评分上限、weighted Rayleigh、六步 PPOopt/Cauchy，并显式固定
  `global_sync` 的 0.5 度旋转门禁。

两套论文工作流都为 Relative R/t 评估保留所有被门禁接受的连通分量，不能用
“只保留最大连通分量”改善精度或改变覆盖率；真值只连接 Evaluator，不进入在线
门禁。图门禁拒边仍同步到下游重建匹配。Relative R/t 结果必须与 accepted-pair
coverage 一起解读。当前论文表格公开的是相对旋转误差；相对平移误差由工作流
Evaluator 额外输出，不能冒充论文已有数值。

这两套论文工作流均把 Track 构建、RobustLiGT 最终可用性掩码和解析重建的最短长度
设为 2。解析重建仅对“恰好两个可用观测”的点启用额外景深门禁：最终共识点沿
两条观测射线的中值正景深除以稳健相机轨迹尺度，超过 20 倍即拒绝。该比例判定
对整体相似变换不敏感；已有的全 Track 远点过滤仍独立保留。OpenCV-PoEngine
三套工作流继续使用最短长度 3，作为保守基线，不会被新模板覆盖。

`OpenCV-PoEngine` 是算法身份，不只是标题前缀：三套模板都固定使用 OpenCV
`findEssentialMat_usac_magsac` 与当前稳定的 `PoRobustSfMEngine`。后续
Paper GNC-RANSAC/Direct 系列作为独立工作流存在，不能通过修改本模板的 estimator
名称来冒充，也不能覆盖这里的 OpenCV 参数。所有内置模板均已移除个人主机的
绝对 `configPath` 与 `outputBaseDir`，默认只运行 1 次，不会自动遍历整个数据集。

创建可编辑工程后，先双击最左侧 DatasetLoader 节点，在其 InputControl 中：

1. 选择包含多个场景目录的共同根目录，再点击 **关联到当前节点**；
2. Strecha 默认场景为 `castle-P30`，目录下应包含 `images` 与
   `gt_dense_cameras`；
3. ETH3D 默认场景为 `facade`，目录下应包含
   `images/dslr_images_undistorted` 与 `dslr_calibration_undistorted`；
4. 1DSfM 默认场景为 `Alamo`；双击 Loader 后先点击 **下载数据集** 打开官方
   获取页面，下载元数据后选择包含 `Alamo`、`Ellis_Island` 等目录的共同根目录；
   图像是单独的可选下载，几何重建不强制要求图像；
5. 如需其他场景，修改 `specific_data`；如需批量场景测试，再在 Var Workspace
   中为 `specific_data` 建立 Loop。

Strecha 的 Loader 先把图像送入 `posdk_preprocessor` 计算特征与匹配；ETH3D
与 1DSfM Loader 直接提供特征与匹配。后续流程一致：

```text
DatasetLoader / Preprocessor
  → OpenCV USAC-MAGSAC 50k（固定 seed=31、1 px）
  → PPO angle-2sin / Cauchy 六步精化
  → bounded weighted local-cycle LCC（保留最大连通分量并同步匹配内点）
  → Chatterjee 旋转平均 + Track 构建
  → PoRobustSfMEngine
       ├─ incremental robust LiGT / GNC-L0.5 / normal-PCG
       ├─ joint R+t PA / sparse normal Cholesky / Cauchy
       └─ consensus analytical reconstruction
          + min track 3 + m3=0.15 + far-point/structural filtering
  → MeshLab 工程与点云导出
```

相对位姿与全局位姿真值均连接到对应 Method 的 `gt_input`。MeshLab 输出目录为
工作流运行目录下的相对路径 `./meshlab_export`；Output Control 还会直接写出
`poengine_global_poses.g2o`、`poengine_points.ply` 与 `poengine_tracks.pb`。ETH3D
在 `PoRobustSfMEngine` 之后插入 Fresh5，再接入与 1DSfM-like Strecha 相同的
`posdk_reconstruction`（`har_attraction_clean`）。`matches2tracks` 保留无损
support-two Track 水库，Fresh 从该原始 Track 和 Loader 原始匹配召回观测；PoRobust
重建仍要求至少 3 个观测。MeshLab 导出清理后的 HAR 点云；Output Control 仍保留
PoRobust 的 G2O/PLY/PB 便于对照。本机数据
目录只绑定到实例化后的可编辑工程，不会写回只读 `.posflow`；对外发布带数据的
工作流时仍需使用已签名 `.posdata`，不能提交本机绝对路径。

### 棋盘格标定相机模型 → 独立图像重建

Developer Workflow Library 还提供 **棋盘格标定相机模型 → 独立图像重建**。
它用两个明确的 `data_images` 节点替换 Strecha Loader：在“棋盘格标定图片”节点
关联标定图，在“场景重建图片”节点关联另一组重建图。运行前双击
`method_calibrator`，按实物标定板设置水平/垂直内角点数、方格物理尺寸、相机模型
和畸变模型；模板默认值为 9×6 个内角点、25 mm 方格及 pinhole radial-K3。

两条分支之间只传递 `data_camera_models`。棋盘格位姿、角点特征和匹配均绑定标定图
的 ViewId，因此不会进入重建支路。场景图独立生成 SIFT 特征与匹配，并先用标定
相机检查观测有效域，再进入与 Strecha 相同的 USAC-MAGSAC、Chatterjee、Track、
`PoRobustSfMEngine` 和 MeshLab 主干；PA 固定内参、畸变和观测坐标。两组图必须来自
同一物理相机，并保持相同焦点/变焦、分辨率、裁剪和方向；本工作流不会对改变后的
图像几何自动缩放内参。Output Control 会同时保存棋盘格标定相机模型与重建的
位姿、Track 和点云。

针对 1DSfM，**GLOMAP式 PoRobust-1DSfM 重建**是升级后的正式配置。它保留
PoMVG 双视图估计器和 PoRobust 后端。USAC-MAGSAC 先执行已验证的 estimator-mask
`30 内点 / 0.25 内点率`安全门；显式标定 pre-RA 节点再以 1 像素阈值独立重计
支持，重新执行 `30 / 0.25`、保留最大连通分量并设置内点数边权。随后执行两轮
`solver_profile=glomap` 的 Chatterjee 旋转平均，每轮之后均执行同步 matches 的
10 度 post-RA 过滤。两次清图后的图以 10 像素 `merge_nearby` 同 view 冲突策略
构建长度 3--100 的核心 Track。RobustLiGT 启用相机跨度门禁；随后的全局 PA 对所有参与且
非规范锚定的视图联合优化 R+t，同时固定全部逐 view 映射的内参/畸变块及观测坐标，
并使用 0.25--4.0 解尺度门和
完整的局部 view-geometry gate。召回第 0 轮先按固定位姿清空并重建结构，第 1--3
轮围绕 3 次固定内参 pose 补全继续 complete、merge 与 refine；这些阶段重新验证
更宽的双视图 correspondence 池，而非 post-RA 核心图。Support-2 结构只在最后
一次 pose 补全后、通过双边邻近核心引导时加入，且不会回流 PnP。给定的 1DSfM
位姿仍只供 Evaluator 使用；每个相机消费者都直接读取 Loader 的显式逐 view
相机映射。

### Ours 两阶段 Pose-only Fresh5-吸附清理-HAR

内置 **Ours：两阶段纯位姿 Fresh5-吸附清理-HAR（1DSfM）** 固化当前 Ellis Island
可复现主链，默认选择 `Ellis_Island`：

```text
min-10 OpenCV 双视图 → Chatterjee RA → support-2 reservoir
  → 无最大长度限制的 Incremental LiGT（私有 Track>=4 控制池）
     + joint R/t pose-only PA1
  → 固定 Pose Fresh5（raw matches，Track>=3 + verified Track=2，5 px）
  → 固定骨架 bounded delta-t LiGT + 分级 joint R/t pose-only PA2
  → 固定 Pose Final Fresh5（召回 verified Track=2，5 px）
  → 固定 Pose HAR（冻结高质量骨架、吸附低质量骨架/其他点、清理空间孤点）
  → Evaluator / PLY / MeshLab
```

初始 Track reservoir 关闭 `method_matches2tracks` 的最大长度过滤；两轮 LiGT 的
求解 Track 最大长度均设为 0，两次 Fresh 的
`maximum_component_observations` 也设为 0，其中 0 是明确的无限制语义。第一阶段
保留 Ellis 已验证的私有 Track>=4 LiGT 控制池；`incremental_rank_irls` 使用较宽松
但 fail-closed 的全局相对秩阈值 `1e-8`，随后 PA1 仅联合优化相机 R+t。

首轮固定 Pose Fresh5 直接读取 Loader 的只读 raw matches 证据库，以 5 px 门禁
重建 Track>=3，同时已经写入验证通过且 feature ownership 唯一的 Track=2。因此
这些 support-two Track 会进入第二阶段，但权重仅为 `0.1`，不能主导骨架。Bounded
阶段固定高支持 core，只求解通过局部秩认证的弱视图 translation increment；局部
相对秩阈值为 `1e-6`，delta 先验权重为 `0.25`，单个光心修正硬界为稳健场景半径
的 `0.05`。LiGT 输出的 Pose hierarchy 在 PA2 中固定 core，只释放认证弱视图并
联合优化其 R+t；Track=2 的 PA 残差同样使用 `0.1` 权重。PA2 启用整阶段 solution
gate 与逐 View geometry gate，并关闭第二次输出 gauge normalization，因此第一轮
已经认证的骨架不会在 Engine 出口被再次整体 recenter/scale。

Final Fresh 在固定 Pose 下同样采用 5 px 验证门，并再次保留 ownership-safe
Track=2。两轮 Fresh5 都在私有 staging 中完成所有权审计，并要求候选
至少保留输入的 `50%` 总 Track、`80%` Track>=3 及 `90%` 原有 View；未通过时整
容器回滚到输入 reservoir，不能把异常的小候选原子提交。HAR 随后固定全部 Pose、逐 view CameraModel 映射、内参、畸变、Feature
像素与 Track topology 来重建结构。两次 PA 均没有点参数，Engine 的旧解析重建
始终关闭。HAR 关闭曲面与 Rij/tij 边。高质量骨架要求 Track>=3，并联合通过条件
协方差、视差、留一稳定性和重投影门；其坐标不可移动且不能被 clean。协方差的信息
矩阵对每个 inlier 累加 `J_i^T w_i J_i`，因此 Track length 已自然参与不确定度估计；
这里把冻结的硬观测数下限由 5 调整为 3，使几何与协方差良好的 Track=3/4 不再被
先验排除。20 px strong ray-tube 以加权 HDI95 簇边缘和逐 CameraModel 的 0.5/1 px
全内点可行域，允许低质量骨架及其他 Track>=2 点吸附。终末 clean 只删除吸附后仍未
解决且为空间孤点的低质量目标，高质量骨架无条件保留。1DSfM 给定 Pose、Track 和
Points 仍只供 Evaluator。

## LiGT 全局平移精度工作流

在 **Test** 中，**ETH3D LiGT 全局平移精度**用于一个本地关联的 ETH3D
场景，**仿真 LiGT 全局平移精度**用于可复现的合成实验矩阵。两者都终止于
`method_LiGT`，但前端有意采用不同设置。ETH3D 评估完整的 Direct-gate 链：

```text
ETH3D 相机 + 特征 + 匹配
  -> two_view_estimator（LiRPFast + PPOopt + Direct 旋转图门禁）
  -> 估计相对位姿 -> Chatterjee 全局旋转平均
  -> 同步后的内点匹配 + Loader 特征 -> method_matches2tracks
干净 Track + 相机 + 平均后的 global R -> method_LiGT
ETH3D 全局位姿 GT -> method_LiGT gt_input
```

估计器要求每个视图对至少含 201 个对应点。旋转图门禁还会在
`method_matches2tracks` 运行前只保留最大连通分量，并将其他视图对的匹配全部
清为 outlier。最大连通分量筛选是全局重建专用步骤：不同于论文的 pairwise
统计协议，Chatterjee 与 LiGT 必须在一个连通分量内定义单一全局坐标系。因此
LiGT 不使用 Loader 预建的 `data_tracks`，而只使用由 Direct gate 保留且与相对
位姿图一致的估计器输出所重建的 Track。

仿真工作流仍用于隔离评估 LiGT 的平移估计：

```text
仿真器相对位姿 GT -> Chatterjee 全局旋转平均
仿真 Track + 相机 + 平均后的 global R -> method_LiGT
仿真器全局位姿 GT -> method_LiGT gt_input
```

LiGT 后不再运行 `PoGlobalSfMEngine`、位置平均或束调整，因此被评估的是 LiGT
本身的平移输出。Evaluator 先将估计位姿与真值统一转换为相机光心，再做 7-DoF
相似变换对齐以消除单目尺度与坐标系自由度，逐相机记录 `translation_error` 和
`rotation_error_deg`。Output Control 同时在每次运行的输出目录中写出
`LiGT_global_poses.g2o`。

ETH3D 模板默认运行 `facade` 一次。仿真模板使用 12 个视图和 1000 个点，将像素
噪声 `0, 0.5, 1.0, 2.0` 与固定种子 `41, 42, 43` 交叉组合，共 12 次运行。
仿真模板将精确相对旋转送入旋转平均，使噪声曲线集中度量 LiGT 从带噪 Track
估计平移的精度，不混入额外双视图估计器；ETH3D 模板则度量所选 LiRPFast +
PPOopt Direct-gate 相对位姿及内点匹配前端所产生的下游全局平移精度。

## 默认双视图实验

内置工程连接：

```text
visual_simulator
  ├─ data_camera_models ─┐
  ├─ data_features ──────┼─> two_view_estimator
  ├─ data_matches ───────┤
  └─ data_relative_poses ──> gt_input
```

默认实验矩阵使用两个独立场景轴：

| 实验轴 | 默认值 | 目的 |
|---|---|---|
| `noise_level` | `0, 0.5, 1.0, 2.0` px | 噪声鲁棒性 |
| `num_points` | `100, 300` | 数据规模与耗时趋势 |

并使用绑定变体保证 estimator 与 algorithm 不会被错误地做笛卡尔积：

- LiRP
- LiRP Fast
- LiRP Consistent
- OpenGV five-point Stewenius
- OpenCV Essential RANSAC

因此默认共运行 `5 × 4 × 2 = 40` 个场景。可在 Loop/Experiment Matrix 对话框中勾选算法变体、修改场景轴或增加重复次数。

### 在 Var Workspace 中选择绑定变体

打开 **Var Workspace Editor** 后，上方的 **绑定算法变体** 不是普通 Loop，
而是一个可由用户定义的绑定维度。内置工作流先提供经过验证的预设；每个预设
是一组共同取值的参数。通常只需要绑定 `estimator + algorithm`，以避免产生
不兼容组合。`refinement`、优化开关或评估选项不必因为它们是 option 就自动
绑定；只有它们必须随某个算法共同变化时才应加入变体。

使用顺序如下：

1. 从预设开始；点击 **编辑** 可增删绑定参数和修改值，也可新增或复制变体。
2. 用圆形勾选按钮选择本次要比较的变体，右侧显示“已选/全部”。
3. 在下方只为相互独立的场景参数添加 Loop，例如 `noise_level` 和
   `num_points`。若确实需要扫描某个优化开关，也把它作为普通 Loop。
4. 设置 `Parallel`、`Jobs` 和 `Repeat`，并核对标题栏中的总配置版本数。
5. 点击 **OK**，再运行工作流。每个启用变体会与所有独立场景轴组合。

运行数计算为：

```text
coupled_var × non_coupled_var1 × non_coupled_var2 × … × Repeat
```

界面中的 **? 指南** 会就地显示上述说明、当前总运行数和一个
`5 × 4 × 2 × 1 = 40` 的示例。

变体内部不是笛卡尔积。例如一个 OpenCV 变体内
`(estimator=opencv_two_view_estimator, algorithm=ESSENTIAL_RANSAC)` 是一次
完整选择，而不是两个实验轴。这个 coupled 维度在外部再与 `noise_level`、
`num_points` 等独立维度做笛卡尔积。

绑定关系保存在实例化后 `.posdk` 工程的 `experimentVariants` 中。编辑器只
列出当前 Method 配置中真实存在的参数，并在保存时检查重复绑定与参数存在性。
内置 `.posflow` 预设仍保持只读且受签名保护，所以用户对当前项目的调整不会
污染模板。未绑定选项保持当前 Configuration 值；只有希望比较多个值时才需要
添加普通 Loop。

## 结果怎么看

这套内置工作流同时回答三类问题：

| 结果 | 具体证明什么 | 不证明什么 |
|---|---|---|
| 工作流成功、输出 `data_relative_poses` | 数据成功进入 Method，算法运行到输出接口 | 不代表精度正确 |
| Evaluator 精度表 | 相对仿真真值的旋转误差、平移方向误差 | 不代表真实数据泛化能力 |
| Profiler 统计 | Method 与核心算法在当前场景下的耗时趋势 | 少量样本不等于稳定微基准 |

Var Workspace 会按界面顺序把实验轴保存为结构化维度。默认双视图表格因此包含
`Algorithm`、`variant`、`noise_level`、`num_points` 和原始 `EvalCommit` 等独立列；
Profiler 的 Method/Core Time 也使用相同维度聚合，所以不同 `num_points` 不会混算。
`ProfileCommit` 继续表示 Method 自己定义的实验提交说明，不再承载 loop 参数。
旧字段 `tagProfileCommit` 仅为工程兼容和运行目录可读性保留。

默认 40 次运行用于快速、可视化比较。若要做论文级运行时间统计，应增加重复次数、固定随机种子、保持温控和电源状态，并继续运行 Developer 测试中的长时 GTest benchmark。GUI 工作流用于系统级数据流与场景比较，不替代 `test_time.cpp` 的大量进程内重复微基准。

## 鲁棒双视图外点基准

**鲁棒双视图外点基准测试**来自历史 `test_gnc_ransac.cpp` 的实验语义，
但以可直接运行、可编辑且结果结构化的 built-in workflow 提供。它不是把标题换成
“鲁棒测试”的普通噪声模板，而是使用真正支持 `outlier_fraction` 的
`opengv_simulator`：

该完整矩阵只在不受限的源码 Developer 构建中内置。Barath、DEGENSAC 与基于
论文复现的 QDEGSAC 后端均为实验性原生插件，尚未全部通过 DMG 发行认证或形成
已验证的可安装 `.pospkg`。Consumer 与 Developer Kit 构建都不会展示一个无法
满足精确依赖闭包的内置工作流。

```text
opengv_simulator
  ├─ data_camera_models ─┐
  ├─ data_features ──────┼─> two_view_estimator
  ├─ data_matches ───────┤
  └─ data_relative_poses ──> gt_input
```

默认绑定变体为：

- GNC-RANSAC LiRP，`sample_size=20`；
- GNC-RANSAC LiRP，`sample_size=30`；
- OpenGV Stewenius 五点 RANSAC；
- OpenCV Essential LMedS；
- OpenCV USAC Default；
- OpenCV USAC MAGSAC；
- Barath MAGSAC；
- Barath MAGSAC++；
- DEGENSAC（原生）；
- QDEGSAC（基于论文复现）。

DEGENSAC 与 QDEGSAC 变体分别直接选择 `degensac_two_view_estimator` 和
`qdegsac_two_view_estimator`，并把用于结果展示的 `algorithm` 标签显式设为
`DEGENSAC` 与 `QDEGSAC`，避免 Evaluator 混合两者或继承基础配置的 `fast` 标签。

独立场景轴为 `outlier_fraction = 0.1, 0.3, 0.5, 0.7` 和
`random_seed = 41, 42, 43`，固定 `num_points=300`、`noise_level=1.0`，
所以默认运行 `10 × 4 × 3 = 120` 次。seed 是显式、可复现的实验列，不依赖系统
时间；若希望做历史论文级长时实验，可把外点轴扩展为 0.1 至 0.7、增加 seed，
但不应把固定 seed 下的同一场景简单重复 50 次并误认为 50 个独立场景。
`random_seed` 通过一条 `assign_value` 变量规则同时写入仿真器与
`two_view_estimator`，因此不仅场景固定，GNC-RANSAC、OpenGV RANSAC、OpenCV
USAC Default/MAGSAC、默认 `sampler_id=0` 的 Barath、原生 DEGENSAC 与基于论文
复现的 QDEGSAC 采样序列也固定；该规则不产生额外笛卡尔积维度。OpenCV LMedS
的公开接口不提供单次seed，
因此使用其固定内部序列。
OpenGV 使用 `ransac_max_iterations` 原生键，并把统一的像素阈值
按两台相机的焦距转换为其射线残差阈值，避免直接把 `1.0 px` 错当成归一化阈值。

`qdegsac_two_view_estimator` 是根据 Frahm 与 Pollefeys（CVPR 2006）独立复现的
实现，既不是作者原始代码，也不是更名后的 DEGENSAC 后端。当前工作流的一般三维
场景用于检查接入、外点鲁棒性、AUC 与耗时，不能单独证明 QDEGSAC 的准退化优势；
后者由插件的主平面专项测试验证。

LMedS 只有在内点超过 50% 时才有稳健性保证，因此 70% 外点场景的失败或低精度是
需要呈现的算法局限，不能补成零误差。RHO 未混入此矩阵：OpenCV 正式只为
`findHomography` 提供 RHO，而当前是一般三维双视图场景；把未受支持的
`findFundamentalMat + RHO` 标成论文 RHO 会形成假基准。

为保证耗时比较含义一致，默认统一 `max_iterations=50000`；QDEGSAC 将其作为完整
模型估计、分层选择与模型补全共享的总假设预算，而不是每个秩各运行 50000 次。
同时关闭仅 MAGSAC 执行的 Bundle Adjustment 和自适应内点后处理。Evaluator 表按
`variant`、`outlier_fraction`、`random_seed` 展示旋转误差与平移方向误差；Profiler 使用相同
维度，因而不会把不同外点比例或 seed 的耗时混在一起。运行失败表示该算法在对应
场景未产生合法输出，不应被填成零误差或“通过”。

GNC-RANSAC 还显式采用原生方法的质量门禁：内部最大可接受残差 `1e-3`、最终最大
平均残差 `5e-3`、最终最小内点率 `0.2` 和 GNC 指标 `10.0`。这些值来自
`method_GNC_RANSAC_LiRP` 本身的默认契约；包装层不再暗中改用更严格的 `2e-4`
阈值，否则 1 px 噪声下已找到有效内点集合的结果会被误判为失败。

仿真器输出的 GT 会从 OpenGV 相机位姿约定显式转换到 PoMVG 的
`x_j = R_ij x_i + t_ij` 约定。若省略这一步，工作流仍可能完成，但 Evaluator
会把互逆位姿比较成数十度甚至接近 180° 的“伪误差”。

## `.posflow v2` 安装与 Marketplace

### DatasetLoader InputControl 与数据集服务

将对应 DatasetLoader 拖入 Behavior Workspace 后，双击当前节点。提供
`input_controls/input_control.qml` 的 Loader 会在 LogViewer 旁显示 InputControl；
数据集说明、官方来源链接、scene/sequence 参数和预期目录树由插件负责呈现，
App 只通过绑定到当前 nodeId 的 `datasetContext` 执行特权操作。全局
**File → Dataset Manager…** 窗口不再作为用户入口。

选择本地根目录并点击 **关联到当前节点**。兼容性只来自 Loader INI 紧邻路径参数声明的
`@type: path + @path_mode: folder + @dataset_binding`，不会按插件名称猜测。操作只
更新当前节点的 instance INI，不改插件模板。签名 `.posdata` 同时写入可移植
`${dataset:...}` binding；外部本地目录只供本机运行，发布前必须先封装并签名为
`.posdata`。解除本地路径登记后，残留在 INI 或 `.bak` 中的绝对路径仍会被最终
发布验证拒绝，不会因登记记录被删除而泄漏到工作流包。

删除全局窗口并不删除数据集安全后端：工作流 Marketplace 仍会从已验证 index
下载 `.posdata`，展示下载/校验/许可证状态，并在失败时执行事务回滚。不能再分发
或需要登录的数据集只允许插件打开官方页面，再由用户选择本地目录。

数据集 Store 按 Consumer/Developer 分离。安装时先核对 archive、manifest、
发布者 Key ID、许可证、确认策略、所需 binding 名称与 `directory` 类型，再写入
内容寻址对象；同版本不同发布者或同 SemVer 优先级的 build 变体不会被当作幂等
安装。已安装小中型对象会在刷新时复核 payload hash，超大对象刷新时执行
manifest、文件集合与大小校验，并在重复采用已有对象前执行完整 hash 校验。

`.posdk` 是用户可编辑工程；`.posflow v2` 是不可变分发单元。App 不直接运行包内工程，而是依次执行：

```text
验证 Marketplace index 签名
  → 下载到仅当前用户可读写的临时目录
  → 校验 archive SHA-256
  → 校验包签名、manifest、lock 与平台兼容性
  → 下载并校验锁定的插件/数据集
  → 用户确认数据集许可证
  → 在一个可回滚事务中安装依赖和工作流
  → 从只读 Store 创建普通 .posdk 工程
```

包内固定包含 `manifest.json`、`project.posdk`、`lock.json`、可选预览图和 `signature.json`。禁止目录 ZIP 成员、符号链接、重复或大小写冲突路径、未知文件、脚本、宿主机绝对路径与未声明网络入口。`lock.json` 固定插件/数据集的版本、manifest SHA-256 和发布者 Key ID；实际下载 URL 只能来自独立签名的 Marketplace index。Index 的 `keys` 表由 Marketplace 根签名授权，并提供与 Key ID 严格匹配的发布者公钥，因此新机器无需预装每个包发布者密钥。

在工作流库中点击“加载签名市场…”可以载入 `index.json`。Consumer 只接受 App 内置 Marketplace 根公钥；Developer 可以为本次会话显式选择测试根公钥。选择 Marketplace 工作流后，界面会自动下载所需 `.pospkg`/`.posdata`，列出安装、升级、已满足和许可证确认项。安装和哈希计算在后台执行，不阻塞主界面。

深链接只允许以下身份格式，不携带任意 URL 或脚本：

```text
posdk://workflow/<publisher>/<workflow-id>@<semver>
```

打开深链接前必须已经载入对应的签名 index；App 会按 `publisher + id + version` 精确定位并准备安装，不进行模糊匹配。

## Developer 发布流程

Developer 模式在 File 菜单提供“发布当前工作流”。向导按四步执行：

1. 从当前工程导出不含宿主机路径的 portable `project.posdk`，创建发布工作区；
2. 使用 Marketplace 根公钥验证 index，再解析并写入精确依赖 lock；
3. 使用工作流发布者 Ed25519 私钥生成签名 `.posflow`；
4. 把工作流发布者公钥授权写入 index，使用独立的 Marketplace 根私钥更新并签名 index。根密钥与工作流发布者密钥必须分开保管。

第 1 步会把当前进程真正激活的插件 generation 写入 portable project：App 内置
模块标记为 `builtin`，动态插件记录精确 SemVer 和不可变 generation SHA-256。
因此新 GUI 创建的依赖使用精确版本约束，不会把本机测试过的 `1.x` 静默锁到
Marketplace 的 `2.x`。旧工程没有 generation 证明时仍可作为兼容输入读取，但
不具备这项发布复现保证，应先在当前 GUI 中重新导出。

工作流所依赖的签名 `.pospkg` 和可再分发 `.posdata` 必须先进入同一个
Marketplace index；否则第 2 步会因无法生成可验证 lock 而停止。对应命令为：

```bash
posdk plugin publish estimator.pospkg --index index.json --publisher posdk \
  --base-url https://example.org/releases --license MPL-2.0 \
  --private-key marketplace-private.pem --public-key plugin-public.pem
posdk dataset publish benchmark.posdata --index index.json --publisher posdk \
  --base-url https://example.org/releases \
  --private-key marketplace-private.pem --public-key dataset-public.pem
```

等价 CLI 为：

```bash
posdk workflow create ...
posdk workflow lock --workspace flow.posflow-workspace --index index.json \
  --public-key marketplace-root-public.pem
posdk workflow validate flow.posflow --public-key publisher-public.pem
posdk workflow package --workspace flow.posflow-workspace \
  --private-key publisher-private.pem --output flow.posflow
posdk workflow publish flow.posflow --index index.json --publisher posdk \
  --base-url https://example.org/releases \
  --private-key marketplace-private.pem \
  --public-key publisher-public.pem
```

`workflow lock` 还会把 index 中经过签名验证的真实数据集许可证写回发布工作区，
替换创建阶段的 `REQUIRED` 占位符。`publish` 负责严格校验包、授权包发布者公钥、更新并重新签名本地 index，
不代替网站上传。最后由 HTTPS 静态发布流水线同时部署包文件、`index.json` 和
`index.json.signature.json`；只有三者都来自同一轮发布，用户端才能完成下载和验证。

普通用户得到的始终是签名包和可编辑工程副本，不会看到源码或开发构建路径。完整机器规范见 `workflows/spec/posflow-v2.md`、`posflow-v2.schema.json` 与 `marketplace-index-v1.md`；旧 v1 目录包仅用于 App 内置模板兼容，不作为第三方发行格式。
