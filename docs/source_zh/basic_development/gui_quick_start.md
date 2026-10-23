# PoMVG GUI 简明使用指南

只想运行 ETH3D、选择场景和查看点云？请先阅读 [快速开始](../installation/cli_gui_guide.md)。运行已有工作流不需要先准备插件开发目录；本页后续说明用于 SDK 关联和开发环境设置。

这份指南只覆盖第一次使用 PoMVG GUI 时最常见的动作：关联自己的插件开发目录、打开或创建工作流，以及启用 Agent、PoMVG CLI 和 PoMVG Skill。

## 1. 先准备自己的 Plugin SDK

不要多人共用同一个 `build/` 目录。管理员可以把一份干净模板放到 Mac Studio 的共享目录：

```text
/Users/Shared/PoSDK/Plugin SDK Template
```

如果还没有共享模板，Developer 版首次启动后也会在当前账户生成一份个人原件：

```text
~/Library/Application Support/PoSDK/PoSDK GUI Developer/DeveloperWorkspace
```

在 Finder 中按 `Shift + Command + G`，打开上面的任一路径。将整个 `DeveloperWorkspace` 目录复制到自己的：

```text
~/Documents/PoSDK Plugin SDK
```

每个人只在自己的 Documents 副本中编译和修改插件。

## 2. 关联插件开发目录

1. 打开 PoMVG GUI。
2. 点击工具栏 **开发**（Developer）。
3. 在 **插件 SDK 关联** 区域点击 **浏览...**。
4. 选择 `~/Documents/PoSDK Plugin SDK`，确认后等待状态变为“自定义 SDK 路径”。
5. 现在可以在 Developer 中创建插件、构建和调试。

也可以点击 **帮助 → 教程**。教程列表分别提供 **如何创建开发插件**、**如何使用 Agent 功能**、**如何打开已有工作流** 和 **如何创建工作流**；选中一个教程并确认后，界面会让其他区域变暗，并用与目标控件边框对齐的脉冲边框指向下一步要点击的按钮。

## 3. 打开或创建工作流

- 加载预设工作流：可以点击 **文件 → 从工作流库新建...**，也可以在 **Behavior Workspace** 顶部点击 **Workflow**；两者都会打开 Workflow Library，选择预设卡片后点击“创建并打开工作流”。
- 创建新项目：点击 **文件 → 从工作流库新建...**，选择模板并创建一个可编辑副本。

## 4. 搭建和运行工作流

1. 在左侧 **Modules** 中找到需要的 Method 或 Data。
2. 将模块拖到中间的 Workflow Canvas。
3. 从输出端口拖到兼容的输入端口，完成连接。
4. 双击模块，在右侧 **Configuration** 中填写参数。
5. 点击保存，再点击工具栏 **运行**。底部 Console 会显示执行状态和错误信息。

## 5. 使用 Agent 与 PoMVG Skill（可选）

Agent 只调用本机的 Codex CLI。PoMVG GUI 不再提供 Claude、Anthropic、HTTP Provider、Base URL 或 API Key 配置；模型和登录状态由 Codex CLI 自己管理。必须先在终端确认 `codex` 能够独立完成一次真实请求，然后再回到 PoMVG GUI。

### 5.1 安装 Codex CLI

首次使用前在终端执行：

```bash
npm install -g @openai/codex

# 方案 A：使用 ChatGPT 登录
codex login
codex login status
codex --version
```

无论使用哪种认证方式，都必须运行一次 smoke test：

```bash
codex exec --json --sandbox read-only --skip-git-repo-check \
  "Reply with exactly OK."
```

只有 smoke test 成功后，PoMVG Agent 才具备可用的 Codex CLI 运行环境。GUI 的“重新检测”主要检查 CLI 可执行文件和登录状态，不能替代这次真实请求验证。API-key 模式可能不提供依赖 ChatGPT 工作区的 Apps/远程插件目录；这不影响 PoMVG Agent 调用本机 Codex CLI 和 PoMVG Skill。

然后：

1. 在 PoMVG GUI 点击工具栏 **Agent**。
2. 点击 Agent 侧边栏的 **Skills** 图标，打开临时的“安装 PoMVG Skill”窗口。
3. 检查窗口中的 Codex CLI 和 PoMVG Skill 状态。
4. 点击 **一键安装全部**。窗口会显示准备、安装和完成状态，GUI 会把 Skill 写入 `~/.codex/skills/posdk/SKILL.md`；若目标目录已有非 PoMVG 文件，GUI 会停止并提示，不会静默覆盖。
5. 之后可以让 Agent 查看当前 `.posdk` 项目、解释模块连接，并在你确认后协助修改配置或搭建工作流。

如果 Agent 显示未检测到 Codex CLI，先在终端完成上述安装、认证和 smoke test，再回到 GUI 点击侧边栏的 **重新检测**。如果终端可以使用、GUI 仍然报 401，检查 PoMVG GUI 启动的 CLI 是否与终端中的 `codex` 是同一个可执行文件，以及 GUI 进程是否能够读取同一份 `CODEX_HOME`/认证缓存；macOS GUI 通常不会自动读取 `.zshrc` 中临时 `export` 的环境变量。

可用下面的命令确认 PoMVG Skill 已安装：

```bash
test -f ~/.codex/skills/posdk/SKILL.md && echo "PoSDK Skill ready"
```

### 5.2 使用 PoMVG CLI（开发者可选）

PoMVG CLI 是源码仓库中的 `tools/posdk_cli.py`，不是独立的 pip/npm 包。普通 DMG 用户使用 GUI 或 Agent 时不需要安装它；开发插件、检查 Plugin SDK 或打包资源时才需要使用。

```bash
cd /path/to/PoSDK_github
python3 tools/posdk_cli.py --help
python3 tools/posdk_cli.py plugin doctor \
  --sdk-root "$HOME/Documents/PoSDK Plugin SDK"
```

将 `/path/to/PoSDK_github` 替换为自己的 PoMVG 源码目录。`plugin doctor` 会检查 Python、CMake、编译工具链及 Plugin SDK 是否可用。

## 6. 常见问题

**为什么不直接关联 Library 原件？** Library 原件可能由安装包或 Developer 自动更新；多人直接在其中编译也会互相覆盖 `build/`。因此只关联自己的 Documents 副本。

**为什么插件没有出现在 Modules？** 先确认关联的是 SDK 根目录（里面应有 `cmake/`、`plugins/`、`dependencies/` 等），然后在 Developer 中完成一次构建，并刷新插件路径。

**教程在哪里？** 工具栏 **帮助 → 教程**。按 `×` 可退出，之后可以随时重新打开。
