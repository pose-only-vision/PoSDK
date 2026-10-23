# 快速开始

还没安装 PoMVG？先按 [v2.1.1 GUI 安装说明](gui_installation.md) 下载并安装 DMG，再回到这里操作。

第一次使用，建议先用 **GUI（图形界面）** 跑通一个已有工作流。熟悉之后，再用 **CLI（命令行）** 重复运行或在服务器上处理数据。两种方式都能使用保存的 `.posdk` 工作流。

```{raw} html
<figure class="posdk-guide-video">
  <video controls playsinline preload="metadata" width="1320" height="720"
         poster="../_static/tutorials/quick-start-cropped.jpg"
         aria-label="PoSDK GUI 快速开始演示">
    <source src="../_static/tutorials/quick-start-cropped.mp4" type="video/mp4">
    您的浏览器不支持视频播放，请<a href="../_static/tutorials/quick-start-cropped.mp4">下载视频</a>观看。
  </video>
  <figcaption>PoSDK GUI 快速开始演示（2 分 19 秒） · <a href="../_static/tutorials/quick-start-cropped.mp4" download>下载视频</a></figcaption>
</figure>
```

(gui-core-features)=
## GUI 的核心功能

GUI 将 PoMVG 的核心能力放在同一个工作环境中：不仅能运行重建，还能看精度、查耗时、了解算法来源、对比不同平台的结果。

| 核心特性 | 你可以做什么 | 从哪里使用 |
| --- | --- | --- |
| **纯位姿几何理论** | 使用相应工作流估计相机位姿并生成点云，再检查重建结果 | **Workflow → 运行 → 可视化** |
| **模块化插件系统** | 将数据读取、算法处理、输出步骤拖入画布并连线；按需要替换模块或修改配置 | 左侧 **模块**、工作流画布、右侧配置面板 |
| **性能分析工具** | 查看各模块已记录的耗时、CPU 和内存统计，找出慢的步骤，导出 CSV | 工具栏 **性能（Profiler）** |
| **自动评估功能** | 查看旋转、平移等已记录的精度指标及均值、中位数，查看图表并导出数据 | 工具栏 **评估（Evaluator）** |
| **版权跟踪** | 查看算法作者、版权声明和依赖信息，汇总成报告 | 节点 **信息** 页；**导出 → 版权信息** |
| **GUI 安装** | v2.1.1 提供 macOS GUI，下载 DMG 后安装即可 | [v2.1.1 GUI 安装说明](gui_installation.md) |
| **多语言日志** | 在控制台查看中英文运行日志；按需要切换界面语言 | **控制台**；**偏好设置 → Language（语言）** |
| **平台集成与对比** | 使用已安装的 PoMVG、OpenMVG、COLMAP、GLOMAP 等算法模块处理同一组数据，比较指标和点云 | 工作流库、**评估**、**可视化**；[平台对比结果](../benchmark_comparison/index.rst) |

**配套功能**：3D 可视化用于分屏看点云和导出视频；Agent 用文字辅助检查或调整工作流；IDE 用于编写插件。需要重复运行或在服务器上处理数据时，可以使用后文的 CLI。

**只运行已有工作流，不需要先开发插件。** 操作步骤同时给出英文按钮名。

运行工作流前，需要在 **帮助 → PoMVG 账户** 中登录。登录会安全保存在本机系统凭据库，GUI 与 CLI 共用；重启后自动恢复，每次运行或断点继续前联网检查账户权限。试用期内也需要登录，有效试用期或许可证的检查仍会保留。网络暂时不可用时，本次运行会被拦截，但会保留登录凭据供恢复连接后重试。持续使用会自动延长登录有效期，连续 30 天未使用或登录被撤销后需要重新登录。

账户的 **Active** 表示账户状态正常；运行时还会校验当前设备的试用期或许可证。GUI 发起的工作流通过正在运行的原生 GUI 进程完成设备权限校验，独立 CLI 则在本机恢复并校验。若提示设备身份或凭据存储不可用，请解锁登录钥匙串并允许已安装的 PoMVG 访问，再重新打开应用。

## 1. 从工作流库打开模板

工作流就是把“读取数据、计算、输出结果”等步骤连接起来。画布上的方框叫**节点**，节点之间的线表示数据从哪里传到哪里。

1. 打开 PoMVG GUI。
2. 点击画布上方的 **Workflow**，或者选择 **文件 → 从工作流库新建…**（File → New from Workflow Library）。
3. 按任务或数据集名称搜索，选择适合的模板，再点击 **创建并打开工作流**。
4. 将这个可编辑项目保存为自己的文件，例如 `MyWorkflow.posdk`。

模板已经接好处理步骤。先配置输入数据，再检查参数和输出位置。下文以 **ETH3D 重建工作流** 为例，介绍数据关联、参数设置和运行操作。

```{figure} ../_static/user_guide/workflow-overview-imagegen-zh-v2.png
:alt: PoMVG 工作流操作：01 打开工作流，02 准备数据，03 配置输入，04 检查设置，05 保存并运行，06 查看结果。
:figclass: posdk-guide-workflow-image
:align: center
:width: 100%
```

## 2. 准备并关联输入数据（以 ETH3D 为例）

1. 在画布上**双击 ETH3D 数据读取器**，打开模块面板；如果显示的是日志，切换到 **输入控制**（Input Control）。
2. 没有数据时，点击 **下载数据**（Download data）。按钮会打开 ETH3D 官方下载页；在浏览器中下载需要的场景，然后解压到电脑上。
3. 回到 PoMVG，点击 **选择本地根目录**（Choose local root），选择包含各个场景的文件夹。
4. 点击 **关联到当前节点**（Associate with node）。看到“已关联到当前节点”后，目录才算设置完成。**只选目录或填入路径，还没有完成关联。**

推荐把解压后的数据放成下面这样：

```text
ETH3D/                         ← 选择这个根目录
├── meadow/
│   ├── images/
│   │   └── dslr_images_undistorted/
│   └── dslr_calibration_undistorted/
│       ├── cameras.txt
│       └── images.txt
└── office/
    ├── images/
    │   └── dslr_images_undistorted/
    └── dslr_calibration_undistorted/
```

**不要把 `images` 或 `dslr_images_undistorted` 当成根目录。** 如果图像和标定分别下载，解压后也要放到对应场景下面。

## 3. 确认运行场景

**PoMVG — ETH3D** 和 **PoMVG — Strecha** 工作流已配置好场景循环，自动遍历数据根目录下的场景子文件夹。

1. 在数据读取器中**关联数据根目录**。
2. 打开**变量工作区**，确认预览中的场景列表和运行次数。
3. **保存并运行**。每个场景运行一次，结果分开保存。

默认串行运行，关闭循环加速。只跑一个场景时，在变量工作区删除“遍历子文件夹”规则，再在数据读取器中选择该场景。

## 4. 保存、运行、查看结果

1. 保存工作流，关闭模块面板，点击工具栏的 **运行**（Run）。
2. 在底部 **控制台**（Console）查看进度。报错时先看最后一条具体错误，例如缺少图像、目录不存在或插件未加载。
3. 检查本次生成的输出文件。需要查看三维结果时，点击 **可视化**（Visualization），加载点云或相机文件。
4. 查看点云时，如果画面中没有显示结果，先点 **重置视角**（Reset View）。

点击运行后，**工作流运行前检测**窗口显示当前步骤和插件检查进度，就绪后自动关闭并开始运行。需要停止时，点击 **取消运行**。

在 **偏好设置 → 通用 → 工作流 → 运行前检测** 中可以关闭检测，默认启用。关闭后会跳过额外的运行前检查；模块仍会在执行时验证并报告错误。

| 3D 查看操作 | 方法 |
| --- | --- |
| 旋转观察 | 在普通查看模式下按住鼠标左键拖动 |
| 平移画面 | 按住鼠标中键拖动 |
| 拉近／拉远 | 滚动滚轮 |
| 比较多个结果 | 加载多个结果文件，使用分屏和视角联动 |
| 导出展示视频 | 在相机轨迹面板中设置路径，再使用视频导出 |

### 查看精度、性能和版权报告

- **看精度**：在节点配置中启用评估，并将所需参考数据接入评估端口。运行后打开 **评估（Evaluator）**，选择算法和指标，查看统计、图表或导出 CSV。没有参考数据时，仍可查看点云，但不能据此得到真值位姿误差。
- **查耗时**：在需要分析的节点启用性能记录，运行后打开 **性能（Profiler）**，查看模块时间、CPU 和峰值内存统计，再决定要优化哪一步。
- **看算法来源**：单击节点，在右侧 **信息（Information）** 页查看作者和版权声明；通过 **导出 → 版权信息** 查看汇总。**导出** 菜单还提供 **评估报告** 和 **性能报告**，可将已有记录整理成报告。

运行后先检查输出，再按需查看评估指标和模块耗时。换参数或换平台后，使用相同数据与参考信息进行比较。

## 5. 想自己创建或调整工作流

已有模板能满足需求时，可以直接使用。想学习搭建过程，可打开 **帮助 → 教程 → 如何创建工作流**。

常用操作是：

1. **文件 → 新建项目**（File → New Project），打开空白画布。
2. 在左侧 **模块** 中找到所需的输入模块，拖到画布上并双击配置。例如，ETH3D 重建使用 **method → SfM → DataLoader → ETH3D 数据读取器**，关联数据根目录并选择场景。
3. 单击节点的**输出端口**，再单击下一个节点的**兼容输入端口**来连线。也可以单击输出端口，再单击画布空白处，从弹出的兼容模块列表中创建下一步，GUI 会自动接上这条线。每个节点需要的其他输入仍要接齐。
4. **单击节点**查看右侧配置；**双击节点**打开输入控制／日志面板。
5. 按任务接好计算步骤，接齐各模块的必需输入。例如，ETH3D 重建模板依次包含**数据读取 → 特征与匹配 → 双视图估计 → 旋转平均／建轨 → PoseEngine → Fresh → PA → HAR 重建**；照着预设的节点和连线调整，更容易接齐相机、特征、位姿等输入。
6. 将需要保存的结果连接到左侧 **others → 输出控制** 节点，打开工具栏的 **输出控制**，检查输出目录、文件名和格式。点云选择 `.ply`，位姿或轨迹按需要选择可用格式。
7. **保存，再运行**，在控制台确认完成，检查导出的文件；三维结果可在可视化中打开。整理画布时，点击画布上方的 **自动排版**；它按依赖关系从左到右排列节点。

在左侧 **模块 → Data** 中右键选择 **重命名**，分别填写中文和英文名称，点击 **保存别名**。名称随界面语言切换，并同步到已有和新建节点的数据端口；点击 **恢复默认** 可取消映射。

`.posdk` 是保存工作流的项目文件。移动项目时，也要带上它同目录下配套的配置文件夹，避免打开后丢失配置。

## 6. 开发与 IDE：加入自己的插件

如果只运行已有算法，可以跳过本节。

1. 点击工具栏 **开发**（Develop），先关联自己的可写 Plugin SDK 目录。
2. 选择插件类型：**Method** 用来增加一个处理步骤；**Data** 用来增加一种数据类型。
3. 填写名称和输入／输出。在 **版权与许可证 → 编辑版权 / 选择许可证…** 中填写作者、持有人并选择许可证；预览后确认使用。
4. 点击 **生成**（Generate），在 IDE 中打开插件源码。
5. 在 **IDE → 集成** 中编辑源码；点击右上角 **© 版权** 可修改版权信息。保存后点击 **重建**，查看构建输出。
6. 构建成功后，按提示刷新插件，并在 **插件测试与发布** 中测试；回到模块列表，将插件加入工作流。

第一次开发可按 **帮助 → 教程 → 如何创建开发插件** 操作。更详细的步骤见 [GUI 简明指南](../basic_development/gui_quick_start.md) 和 [插件开发指南](../basic_development/plugin_guide.md)。

版权编辑步骤见[版权信息](../copyright_tracking.rst)。

## 7. Agent：用文字让助手帮忙

可以根据任务选择内置 Agent，或让 Codex 配合 PoSDK CLI 工作：

| 使用方式 | 适合做什么 |
| --- | --- |
| **GUI 内置 Agent** | 点击工具栏 **Agent**，结合当前画布操作模块和连线、理解数据流、检查输入、分析参数配置与运行错误 |
| **Codex + PoSDK CLI** | 在本地或通过 SSH 在远端搭建、调整和运行工作流，修改模块配置、批量执行并分析日志与结果 |

两种方式可以配合使用：先在 GUI 中检查流程和参数，再让 Codex 通过 CLI 在本地或服务器上重复运行，最后回到 GUI 查看结果。

PoMVG 提供配套的 **PoMVG Skill**，帮助助手理解工作流操作、模块接口与开发规范，辅助修改配置、开发 Method／Data 插件和排查问题。

```{figure} ../_static/user_guide/codex-posdk-cli-zh-v1.png
:alt: 描述目标后，Codex 参考 PoMVG Skill 调用 PoSDK CLI，搭建、配置和运行本地或 SSH 远程工作流；GUI 内置 Agent 帮助操作模块、理解流程和分析参数。
:figclass: posdk-concept-diagram
:align: center
:width: 100%

Codex 调用 PoSDK CLI 完成任务，远程执行通过 SSH 连接目标机器；内置 Agent 结合当前 GUI 画布提供交互辅助。
```

### 配置内置 Agent

第一次使用，可按 **帮助 → 教程 → 如何使用 Agent 功能** 操作，或选择下面一种方式配置本机 Codex CLI。

**方式一：一句话配置（推荐）**

打开电脑上已有的 Codex、Cursor 等 Agent，在它的对话中发送：

> 帮我配置本地的 Codex CLI。

**方式二：复制完整提示词**

把下面的内容复制给 Codex、Cursor 等 Agent：

```text
请帮我配置本机的 Codex CLI，用于 PoMVG GUI 的 Agent。
1. 检查是否已安装；需要安装时，按 OpenAI 官方指南选择适合本机系统的方式：https://developers.openai.com/codex/cli。
2. 配置命令路径；如需登录，指导我完成 codex login。
3. 检查 codex --version 和 codex login status，并运行一次只读请求：codex exec --json --sandbox read-only --skip-git-repo-check "Reply with exactly OK."
4. 确认请求成功后，告诉我如何回到 PoMVG GUI → Agent 重新检测，并在 Skills 中一键安装 PoMVG Skill。
```

配置完成后，回到 **PoMVG GUI → Agent**，点击 **重新检测**。检测通过后，在 **Skills** 中点击 **一键安装全部**，再输入任务并发送。

安装与登录步骤见 [Codex CLI 官方指南](https://developers.openai.com/codex/cli)。

可以直接这样提问：

> 请检查当前工作流，还缺哪些输入或设置？
>
> 把这次处理的场景从 meadow 改成 office，其他设置保持不变。
>
> 解释控制台最后这条错误，并告诉我应该在哪里修改。
>
> 参考 PoMVG Skill，帮我创建一个 Method 插件，并说明如何编译和测试。

助手修改后，回到 DatasetLoader 面板或画布确认变化，再保存并运行。Agent 是可选功能；不用 Agent 也能完成前面的操作。

## 8. CLI：在终端运行同一份工作流

CLI 适合重复运行或服务器任务，可运行 GUI 中保存的工作流。

DMG 中提供了 **PoSDK Plugin SDK** 文件夹。请将整个文件夹复制到 **Documents（文稿）** 中，放置为 `~/Documents/PoSDK Plugin SDK`；`posdk` 命令位于其中的 `bin/posdk`。下面用 `POSDK_CLI` 保存命令的完整路径，供后面的命令复用。先查看帮助：

```bash
POSDK_CLI="$HOME/Documents/PoSDK Plugin SDK/bin/posdk"
"$POSDK_CLI" --help
"$POSDK_CLI" workflow run --help
```

### 登录与记住账户

如果已经在同一台电脑、同一系统用户的 GUI 中登录，CLI 会直接复用。也可以在终端登录：

```bash
"$POSDK_CLI" account login --build-root "/path/to/PoSDK_GUI.app"
"$POSDK_CLI" account status --build-root "/path/to/PoSDK_GUI.app"
"$POSDK_CLI" account logout --build-root "/path/to/PoSDK_GUI.app"
```

首次登录会打开 posdk.net 浏览器授权页，核对终端显示的授权码后确认即可。远端终端可加 `--no-browser`，将显示的链接复制到自己的浏览器完成授权，不需要转发本地回调端口。退出会清除这台电脑当前系统用户的 GUI/CLI 共享登录。

### 推荐：让 Codex 调用 PoSDK CLI

在 [Codex](https://developers.openai.com/codex/cli) 中打开工作目录，说明 SDK、项目和数据的位置，并让它参考 PoMVG Skill。你可以直接描述目标，让它通过 PoSDK CLI 搭建或调整工作流、修改模块配置、运行并分析日志与结果，例如：

```text
请参考 PoMVG Skill，通过 PoSDK CLI 调整并运行我的 ETH3D 工作流。
SDK：~/Documents/PoSDK Plugin SDK
项目：/path/to/MyWorkflow.posdk
数据：/path/to/ETH3D
将场景设为 office，检查模块连线与参数，保存后运行，并分析日志、给出结果目录。
```

远程任务再补充 **SSH 账户和主机**，以及**远端的 SDK、运行程序、项目与数据路径**。Codex 可以通过 SSH 在目标机器上搭建或调整工作流、修改模块配置并运行，输出保存在远端，可按需取回 GUI 查看。具体步骤见 [CLI 远程开发指南](../basic_development/headless_cli_remote_development.md)。

### 先检查 GUI 中保存的项目

先在终端中进入保存 `MyWorkflow.posdk` 的文件夹，再执行：

```bash
"$POSDK_CLI" workflow inspect --project ./MyWorkflow.posdk
"$POSDK_CLI" workflow validate-project --project ./MyWorkflow.posdk
```

`inspect` 显示节点、连线和配置文件的数量，以及所需插件；`validate-project` 检查项目是否完整。这里的 `MyWorkflow.posdk` 是你自己保存的项目。

### 指定目录与场景后运行

```bash
"$POSDK_CLI" workflow run \
  --project ./MyWorkflow.posdk \
  --workspace ./eth3d-runs \
  --build-root "/path/to/PoSDK_GUI.app" \
  --dataset-root "loader=/path/to/ETH3D" \
  --set loader:eth3d_dataset_loader.specific_data=meadow
```

把两个 `/path/to/...` 换成这台电脑上的实际路径。`--build-root` 指向配套的 PoMVG 应用或构建目录。例子中的 **`loader` 是 DatasetLoader 的节点 ID**；在 GUI 中单击这个节点，可在右侧配置面板查看节点 ID。如果不是 `loader`，就把命令中的两处 `loader` 一起替换。

`--dataset-root` 指定数据根目录，`--set` 指定本次处理的场景。运行结束后会显示运行编号及结果目录。

### 查看状态、日志和输出文件

把下面的 `RUN_ID` 换成上一步返回的运行编号：

```bash
"$POSDK_CLI" run status --workspace ./eth3d-runs --run RUN_ID
"$POSDK_CLI" run logs --workspace ./eth3d-runs --run RUN_ID --follow
"$POSDK_CLI" run artifacts --workspace ./eth3d-runs --run RUN_ID
```

输出文件可以回到 GUI 的可视化中查看。需要离开终端后继续运行时，在运行命令末尾加上 `--detach`。

## 遇到问题先检查这里

| 现象 | 先检查什么 |
| --- | --- |
| 已选目录，但仍提示未关联 | 是否点了 **关联到当前节点**，并等到完成提示 |
| 找不到图像或标定 | 根目录是否包含场景目录；图像和标定是否都已解压到对应场景 |
| 修改场景后没有生效 | 场景名称是否与目录名一致；输入后是否按 Tab／点到其他位置，并保存 |
| 不知道下一步点哪里 | 打开 **帮助 → 教程**，按高亮提示操作 |
| 插件无法编译 | 检查 Plugin SDK 目录是否已关联，并查看构建输出中的具体错误 |
| 运行完却看不到点云 | 在可视化中选中本次输出的 PLY，并点击 **重置视角** |
