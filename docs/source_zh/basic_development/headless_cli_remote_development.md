# PoMVG CLI + Codex Skill 远程开发指南

这份指南面向课题组成员：成员在自己的电脑上使用 Codex CLI，通过 SSH/Tailscale SSH 进入 Mac Studio 自己的账户，在独立 workspace 中开发插件、修改 `.posdk` 工作流并运行。

## 1. 工作目录

每个人只使用自己的目录，不共享 CMake build 目录：

```text
~/PoSDKWorkspaces/my-project/
├── plugins/
├── workflows/
├── build/
└── runs/
```

SDK 可以由管理员提供只读副本；插件源码、构建目录、运行结果必须放在自己的 workspace 中。

## 2. 开始前检查

```bash
posdk plugin doctor --sdk-root <PoSDK-Plugin-SDK路径>
posdk capabilities --build-root <Developer构建目录>
```

`posdk` 位于源码仓库的 `tools/posdk`，也会随 Developer Plugin SDK
放入 `bin/posdk`。远程运行工作流时，Plugin SDK 还必须能够访问当前
Developer 构建中的 `posdk_runner` 和 `posdk_headless_snapshot`。
`--build-root` 可以指向 Developer preset 构建目录，也可以直接指向
同一台 Mac 上的 Developer `.app`；后一种方式会使用
`Contents/Helpers` 与 `Contents/PlugIns`。

当 PoMVG GUI 正在同一账户中运行时，可以使用受认证的实时控制命令。Module
目录直接来自当前进程的插件注册器以及现有 manifest `input_types` /
`output_types`、Data plugin datatype 和 INI `@Prior`，不要求插件开发者维护
额外的 Agent 描述：

```bash
posdk gui inspect
posdk gui modules
posdk gui describe-module --plugin eth3d_dataset_loader --category method
posdk gui add-node --plugin eth3d_dataset_loader --category method --x 180 --y 160
posdk gui compatible-targets --source-node <node-id> --source-port data_global_poses
posdk gui connect --source-node <node-id> --source-port data_global_poses \
  --target-node <target-id> --target-port data_global_poses
posdk gui replace-node --node <node-id> \
  --plugin eth3d_dataset_loader --category method
posdk gui remove-node --node <node-id>
```

`gui add-node` 返回实际 `nodeId` 和解析后的节点端口；后续命令必须使用该
返回值，不能猜测 ID。README 或 description 只用于理解模块用途，连线是否
合法始终由 PoMVG datatype 校验器决定。

替换画布上的 Module 时使用一次 `gui replace-node`，不要组合“删除、创建、猜测
新 ID、重新连线”等命令。该命令保留原 nodeId、位置、尺寸、自定义名称、组合
关系和所有 datatype 兼容的连线，并为新 Module 创建自己的配置。若存在不兼容
连线，默认拒绝整个替换；只有用户明确同意丢弃时才使用
`--drop-incompatible`。只读检查命令执行成功不代表替换已经完成。

如果使用远程 Mac Studio：

```bash
ssh <自己的账户>@<Mac-Studio地址>
cd ~/PoSDKWorkspaces/my-project
```

## 3. 检查和修改工作流

```bash
posdk workflow inspect --project workflows/demo.posdk
posdk workflow validate-project --project workflows/demo.posdk
```

Codex 需要先生成 action plan，再让用户确认：

```json
[
  {
    "type": "add_connection",
    "sourceNodeId": "loader",
    "sourcePort": "data_images",
    "targetNodeId": "features",
    "targetPort": "data_images"
  }
]
```

先检查，不写文件：

```bash
posdk workflow apply \
  --project workflows/demo.posdk \
  --plan actions.json \
  --check
```

确认无误后再应用：

```bash
posdk workflow apply \
  --project workflows/demo.posdk \
  --plan actions.json
```

CLI 会保留 `.posdk.bak`，并使用原子写入，避免修改中断导致项目损坏。

## 4. 运行工作流

```bash
posdk workflow run \
  --project workflows/demo.posdk \
  --workspace ~/PoSDKWorkspaces/my-project
```

如果工作流中的 Dataset Loader 需要本机数据目录，运行时显式绑定，不要把本机绝对路径写入可移植项目：

```bash
posdk workflow run \
  --project workflows/strecha.posdk \
  --workspace ~/PoSDKWorkspaces/my-project \
  --dataset-root strecha_loader=/path/to/Strecha
```

也可以覆盖一个普通 INI 参数：

```bash
posdk workflow run \
  --project workflows/demo.posdk \
  --workspace ~/PoSDKWorkspaces/my-project \
  --set estimator:two_view_estimator.num_threads=4
```

每次运行都会产生独立目录：

```text
runs/<run-id>/
├── workflow.json
├── plugin-runtime-snapshot.json
├── config/
├── output/
├── result/
├── console.log
├── events.jsonl
├── run-summary.json
└── artifacts.json
```

需要离开 SSH 后继续运行时，使用内置 detached 模式，不需要保持
终端或 tmux 会话：

```bash
posdk workflow run \
  --project workflows/demo.posdk \
  --workspace ~/PoSDKWorkspaces/my-project \
  --detach
```

## 5. 查看日志和结果

```bash
posdk run status \
  --workspace ~/PoSDKWorkspaces/my-project \
  --run <run-id>

posdk run logs \
  --workspace ~/PoSDKWorkspaces/my-project \
  --run <run-id> \
  --follow

posdk run artifacts \
  --workspace ~/PoSDKWorkspaces/my-project \
  --run <run-id>

posdk run cancel \
  --workspace ~/PoSDKWorkspaces/my-project \
  --run <run-id>
```

`console.log` 用于阅读；最终是否成功应以 `run-summary.json`、
`events.jsonl` 中的 `run_finished` 终态和 `artifacts.json` 为准。
即使子进程返回 0，只要缺少合法终态事件，CLI 也会把运行标记为失败。

VNC 只用于检查 Behavior Workspace 的布局、QML 控件和可视化结果；普通构建和运行不需要 VNC。

PoMVG GUI 不负责发起 SSH。正确方向是：成员先从自己的电脑通过 SSH/Tailscale
登录 Mac Studio 的个人账户，再在该账户终端中执行上面的 `posdk` 命令。
需要检查 Behavior Workspace、QML 或可视化结果时，再通过 VNC 进入同一个
服务器账户打开 PoMVG GUI；不要通过 SSHFS 把服务器插件目录挂到另一台 GUI。

## 6. 通过 SSH 发布插件

先在 **插件测试与发布 → 发布** 或 CLI `plugin release` 中生成通过正式检验的
签名 `.pospkg`。然后在远端插件目录执行：

```bash
export POSDK_PLUGIN_SDK_ROOT="$HOME/PoSDK/PluginSDK"
cd "$HOME/PoSDKWorkspaces/my-plugin"
./publish.sh
```

若远端账户已经能直接运行 `posdk`，无需再设置 `POSDK_PLUGIN_SDK_ROOT`。

按提示输入 GitHub 的 `owner/repository`。如果 `gh` 尚未登录，终端会显示 GitHub
官方设备授权提示；在自己的浏览器完成授权即可，不要把密码或 Token 粘贴到
PoMVG。核对终端显示的新建/追加/更新计划后输入 `yes`。GitHub Release 上传完成
后，终端会显示 posdk.net 链接、12 位授权码和插件 ID；在任意浏览器登录
posdk.net，核对三项信息并批准。批准只允许当前插件发布一次，成功后立即失效。
SSH 无需 VNC，也无需把 GitHub 或 posdk.net 密码保存在服务器。

## 7. Codex Skill

成员电脑上安装 PoMVG Skill 后，Codex 会知道：

- manifest、INI、端口和 Prior 规则；
- 插件创建、构建、测试和 snapshot；
- `.posdk` 节点、连接和参数修改；
- 普通端口、Prior 与 Evaluation 连线的类型检查；
- SSH workspace 和运行日志位置；
- 运行结果的正确判定方法。

Skill 不替代 macOS 权限。不要访问其他成员目录，不要把绝对主机路径写入可移植 `.posdk`，不要未经确认执行删除或覆盖操作。

如果成员只拿到 Developer Plugin SDK，没有完整源码仓库，可以直接从 SDK 安装主 Skill：

```bash
cd "/path/to/PoSDK Plugin SDK"
bin/posdk skill install
bin/posdk skill doctor
```

默认安装位置是 `~/.codex/skills/posdk/SKILL.md`。如果目标文件已经存在且内容不同，CLI 会停止并要求显式加 `--force`，避免覆盖成员自己的修改。每位成员只在自己的电脑上安装一次；通过 SSH 登录 Mac Studio 时，远端账户也应在自己的账户中运行 `bin/posdk skill doctor`。

也可以在 PoMVG GUI 的 Agent → Skill 窗口点击 **下载压缩包**。GUI 会把
`PoSDK-Skill-*.zip` 导出到下载目录，包内包含 macOS/Linux 与 Windows
安装脚本、SHA-256 校验、清单和远程控制说明。压缩包不会写入服务器地址、
账户、SSH 私钥、激活码或 GUI 控制描述符。应当在运行 Codex 的客户端安装
Skill，再通过 SSH 登录远端；`posdk gui status/inspect/apply` 必须在运行
PoMVG GUI 的同一远端账户下执行。


## 版权元数据导入

按[版权信息](../copyright_tracking.rst)中的步骤生成 JSON 模板、预览并导入，完成后重编译并刷新插件。
