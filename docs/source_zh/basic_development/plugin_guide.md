# 插件开发、发布与安装指南

本指南涵盖 PoMVG 插件的完整生命周期：从使用 GUI 的 Developer 工具创建新插件，到使用 AI 辅助开发进行构建和测试，再到发布供他人使用，最后从市场安装插件。

---

## 1. 创建新插件

### 使用 Developer 按钮（推荐）

PoMVG GUI 提供了内置的脚手架工具，可以生成完整的插件项目。

1. 打开 **PoMVG GUI**
2. 点击工具栏中的绿色 **Developer** 按钮
3. 填写对话框：
   - **插件类型**：Method 或 Data
   - **插件名称**：使用 `snake_case` 格式（例如 `my_feature_matcher`）
   - **显示名称**：面向用户的默认名称，例如 `My Feature Matcher` 或
     `特征匹配`；该字段写入 Manifest
   - **类名**：自动生成为 PascalCase 格式（例如 `MyFeatureMatcher`）
   - **基类**：从 `MethodPresetProfiler`、`RobustEstimator` 或 `MethodPreset` 中选择
   - **输入/输出数据类型**：选择插件消费和产生的数据类型
   - **IDE**：选择 VS Code 或 Cursor（自动检测）
   - **生成 .claude/CLAUDE.md**：保持勾选以启用 AI 辅助开发
4. 点击 **Generate & Open**

这将在 `plugins/methods/MyFeatureMatcher/` 下创建一个可立即构建的插件目录：

```
MyFeatureMatcher/
├── CMakeLists.txt              # 构建配置（自动发现）
├── my_feature_matcher.hpp      # 插件头文件
├── my_feature_matcher.cpp      # 插件实现（包含性能分析、日志）
├── my_feature_matcher.ini      # 运行时配置
├── manifest.json               # .pospkg 导出的包元数据
├── README.md                   # 文档
└── .claude/
    └── CLAUDE.md               # AI 开发上下文和技能
```

IDE 会自动打开——您可以立即使用 Claude Code 或任何 AI 助手开始编码。

### 手动创建

如果您更喜欢手动创建插件，请参考 `plugins/methods/MethodExample/` 的结构。关键要求是：

- 使用 `add_posdk_plugin()` 的 `CMakeLists.txt`
- 声明继承自 PoMVG 基类的类的头文件
- 在末尾包含 `REGISTRATION_PLUGIN(PluginMethods::YourClass)` 的源文件
- `.ini` 配置文件（可选但推荐）

### 稳定 ID 与显示名称

PoMVG 将运行身份与界面名称严格分离：

- `name` / `plugin_name` 是 Factory、端口、Runner、配置路径、日志与包校验
  使用的稳定插件 ID。不要通过修改它来改变界面显示。
- 源码 Manifest 中的 `display_name` 是插件作者提供的默认显示名称。
- 在 Modules 中重命名得到的是本机用户别名，保存在应用设置中。
- 在 Behavior Workspace 中重命名得到的是节点实例名称，以 `displayName`
  写入 `.posdk`；它不会进入 Runner 协议或 INI。

显示名称支持空格、中文和常规标点，但必须去除首尾空白、采用 Unicode NFC，
最长为 80 个 UTF-16 单元 / 256 个 UTF-8 字节，且不能包含控制字符或格式控制字符。

---

## 2. 开发您的插件

PoMVG 插件工程要求 CMake 3.19 或更高版本。构建系统会先通过 CMake 的结构化
JSON 解析器校验源码 manifest，再生成运行时 sidecar；当前 CLI 与 GUI 脚手架会
自动声明这一最低版本。

### 项目结构

**Method 插件**使用命名空间 `PluginMethods` 并继承自以下之一：

| 基类 | 使用场景 |
|---|---|
| `MethodPresetProfiler` | 最常用——包含配置加载 + 性能分析 |
| `RobustEstimator` | 用于 RANSAC 风格的鲁棒估计算法 |
| `MethodPreset` | 轻量级——仅配置加载，无性能分析 |

**Data 插件**使用命名空间 `PoSDKPlugin` 并继承自 `DataIO`。

### 关键模式

```cpp
// 构造函数——声明输入和输出
MyFeatureMatcher::MyFeatureMatcher()
{
    required_package_["data_images"] = nullptr;    // 必需输入
    output_types_ = {"data_features"};             // 输出类型
    InitializeDefaultConfigPath();                 // 加载 .ini 配置
}

// Run——您的算法在这里
DataPtr MyFeatureMatcher::Run()
{
    auto images = GetRequiredDataPtr<ImagePaths>("data_images");

    bool profiling = GetOptionAsBool("enable_profiling", true);
    POSDK_START(profiling);

    PROFILER_STAGE("feature_extraction");
    // ... 您的算法 ...

    PROFILER_END();
    PROFILER_PRINT_STATS(profiling);
    return result;
}
```

### 配置（INI）

```ini
[my_feature_matcher]

# @name: 启用性能分析
# @type: bool
enable_profiling=true

# @name: 最大特征数
# @type: int
# @range: 100, 50000
max_features=8000

# @name: 输出文件夹
# @type: path
output_folder=output/my_feature_matcher
```

在代码中读取值：
```cpp
int maxFeat = GetOptionAsIndexT("max_features", 8000);
std::string folder = GetOptionAsPath("output_folder");
```

### 双语日志

```cpp
LOG_INFO_ZH << "[MyFeatureMatcher] 检测到 " << count << " 个特征点";
LOG_INFO_EN << "[MyFeatureMatcher] Detected " << count << " features";
```

### AI 辅助开发

如果您生成了 `.claude/CLAUDE.md`，Claude Code（在 VS Code 或 Cursor 中）将自动理解：
- PoMVG 构建系统（`add_posdk_plugin()` 功能）
- 编码约定（命名空间、日志、性能分析宏）
- 关键参考文件和文档链接
- 统一的 `posdk` skill 入口用于 PoMVG 文档、INI / 日志元数据、QML 扩展与相机模型参考

---

## 3. 构建您的插件

### 从 PoMVG 项目根目录构建

```bash
cd /path/to/PoSDK
mkdir -p build && cd build
cmake ..
cmake --build . --target my_feature_matcher
```

具有自己 `CMakeLists.txt` 的 Method 插件会被 `auto_discover_plugin_directories()` **自动发现**——无需修改任何父 CMake 文件。

构建的插件出现在：
```
output/plugins/methods/posdk_plugin_my_feature_matcher.dylib   # macOS
output/plugins/methods/posdk_plugin_my_feature_matcher.so      # Linux
```

### 在“插件测试与发布”中验证

1. 打开 **Plugins → 插件测试与发布 → 测试**
2. 在 Method/Data 分组中选择插件，先运行 **Quick Test**；旧插件没有
   `posdk.tests.json` 时可使用 **创建并快速测试**
3. 查看任务级状态、可信 JSON/JUnit 报告和 owner-only 诊断日志
4. 需要进一步验证时运行 Standard、Benchmark、Security 或显式算法用例

测试页会固定当前不可变插件 generation，并校验实际执行集合、进程退出状态、
运行时契约和结构化证据。只有 exit code 为零而没有可信报告，或只有
`runtime_contract` 而没有算法用例，都不会被解释为算法已经正确。

---

## 4. 发布您的插件

### 4.1 使用 `posdk plugin` 创建 `.pospkg v3`

`.pospkg v3` 是严格、可签名且可复现的目标平台二进制发行格式。一个包只对应一个系统和架构；源码不是发布必需项。除动态库外，包中固定包含签名的 `install.sh`，并可按插件复杂度携带 `deps/`、INI、QML 和运行资源。唯一权威写入端是 `tools/posdk plugin package`，因此 GUI、CLI 和 CI 使用同一套验证规则。

日常开发优先使用 **Plugins → 插件测试与发布 → 发布**。开发者先选择发行模式并
运行模式化发布预检；只有当前插件最近一次可信测试和当前模式预检均通过，才可
点击 **正式检验并打包 .pospkg**。GUI 复用同一 CLI 后端，依次执行
`prepare-release → publisher-init → audit-release → release`，并显示完整日志、
错误和最终包路径。Developer 创建窗口不再提供另一条直接打包路径。

发布页的 **AI 协助打包**会调用 PoMVG 内部 Codex Agent，将权限限制到当前插件
源码目录，并传入所选模式与诊断日志。Agent 可以修复插件、依赖配方、
`install.sh`、测试和发布契约并运行构建/预检，但不能自动上传 GitHub Release，
也不能绕过可信测试、审计或签名校验。

```bash
tools/posdk plugin create --id my_feature_matcher --type method \
  --distribution-model closed_plugin_open_dependencies \
  --destination ./MyFeatureMatcher
tools/posdk plugin build --source ./MyFeatureMatcher --build-dir ./build \
  --sdk-root /path/to/plugin_sdk --target my_feature_matcher
tools/posdk plugin package --build-root ./build --id my_feature_matcher \
  --type method --version 1.0.0 --private-key publisher.pem \
  --installer-script ./MyFeatureMatcher/install.sh \
  --installer-config ./MyFeatureMatcher/posdk-installer.json \
  --distribution-model closed_plugin_open_dependencies \
  --output my_feature_matcher-1.0.0-macos-arm64.pospkg
tools/posdk plugin validate my_feature_matcher-1.0.0-macos-arm64.pospkg \
  --public-key publisher.pub.pem
```

Consumer 包必须使用 Ed25519 私钥签名。仅供 Developer 会话使用的未签名包必须显式增加 `--allow-unsigned`；这一例外不会写入 Consumer 策略。

**典型 `.pospkg v3` 内容：**

```
my_feature_matcher-1.0.0-macos-arm64.pospkg (zip)
├── manifest.json                                    # 身份、ABI、依赖、资源与每个文件的 SHA-256
├── signature.json                                   # Ed25519 签名
├── install.sh                                       # 签名的目标系统依赖安装程序
├── deps/                                            # 可选，签名的安装引导资源
├── plugins/methods/
│   ├── posdk_plugin_my_feature_matcher.dylib
│   ├── posdk_plugin_my_feature_matcher.posdk.json
│   ├── posdk_plugin_my_feature_matcher.manifest.json
│   └── posdk_plugin_my_feature_matcher.dependencies.json  # 可选
└── configs/methods/
    └── my_feature_matcher.ini
```

`install.sh` 负责当前目标系统上的 Homebrew、apt/dnf 或固定版本源码构建，并必须生成严格 JSON 安装报告；仅返回退出码 0 不算成功。需要系统包管理器权限时，开发者应在 `posdk-installer.json` 中声明 `privilege: "administrator"`，由操作系统弹出授权窗口，PoMVG 不读取密码。打包器还会跟随同 stem 的 `.runtime-files.json` 和 `.dependencies.json`，把声明的运行时文件与私有库一并纳入签名闭包。缺失文件、路径逃逸、符号链接、硬链接、重复路径或未声明的 generation 文件都会使打包失败。

开发者可在 **插件测试与发布 → 发布** 或 `--distribution-model` 中选择三种发布方式：

- `closed_plugin_closed_dependencies`：插件和依赖源码均不公开；仍须提供适用的许可证、再分发与 source-offer 说明，并把全部运行二进制纳入可移植签名闭包。
- `closed_plugin_open_dependencies`（OpenMVG、COLMAP、Python 包等封装插件的推荐方式）：插件动态库闭源，依赖库的源码地址、固定版本、哈希、许可证和构建方案公开。
- `open_plugin_open_dependencies`：插件源码和依赖构建方案全部提交到插件市场固定的公开 Git commit。

三种方式生成相同结构的目标平台签名 `.pospkg`，执行完全相同的签名、哈希、ABI、平台、依赖闭包、隔离安装和运行测试。开源方式在 Consumer 端仍下载签名二进制包，不会临时克隆源码编译；闭源方式也不会降低许可证或运行闭包要求。

#### 自动绑定安装产物与插件配置

凡是由 `install.sh` 安装或编译的外部可执行程序，都不应再要求用户手动查找路径。安装器应把插件私有工具安装到 `POSDK_INSTALL_STATE_ROOT` 下，并在签名的 `installer.configuration_bindings` 中预先声明允许写入的配置目标：

```json
{
  "configuration_bindings": [
    {
      "id": "graphoptim_bin",
      "config_path": "configs/methods/method_rotation_averaging.ini",
      "section": "method_rotation_averaging",
      "key": "graphoptim_bin_folder",
      "kind": "directory",
      "required_executable": "rotation_estimator"
    }
  ]
}
```

`install.sh` 的严格 JSON 报告只返回已声明的绑定 ID 和相对于安装状态目录的路径：

```json
{
  "bindings": [
    {"id": "graphoptim_bin", "relative_path": "bin"}
  ]
}
```

PoMVG 会解析并校验最终目录，拒绝符号链接和路径逃逸，确认所需可执行文件存在且可执行。随后由 PoMVG 原子维护固定入口 `<state-root>/plugin-bin/<plugin-id>/<binding-id>`；该入口是指向版本化安装产物的相对软链接，INI 只写入这个不含版本号的稳定绝对路径。空值、旧的 PoMVG installer-state 路径或旧稳定绑定会自动更新；用户明确设置的非空外部路径会被保留。GUI 安装完成后还会刷新当前工作流中匹配节点的实例配置，新建节点则继承更新后的插件配置模板。`install.sh` 本身不得任意修改 PoMVG 配置文件。

真实安装目录与对外稳定入口分别承担隔离和易用性：

```text
<state-root>/installer/<plugin>/<version>/<system>-<arch>/bin/tool
<state-root>/plugin-bin/<plugin>/<binding> -> ../../installer/.../bin
```

插件升级时只原子切换软链接，用户配置和工作流无需变化；卸载时稳定入口同步失效。开发者和用户可通过 `PoMVG plugin paths --id <plugin>` 查询 Plugin Store、版本化目录、全部稳定 binding 和可执行文件。PoMVG GUI 的“已安装插件包”页也提供路径查看、打开和复制。插件运行时应优先解析稳定 binding，仅为旧版 PoMVG 已安装包保留带版本号的兼容查找。

稳定路径只是在当前电脑上的固定入口，不等于二进制可以直接复制。换电脑时必须在目标电脑重新安装对应系统与架构的签名 `.pospkg`，不要复制旧电脑的 `plugin-bin/`、`installer/` 或单个 bin。源码构建型 `install.sh` 必须固定源码与依赖版本，显式设置并验证目标架构和最低系统版本，拒绝构建机绝对路径，并在目标电脑完成真实 smoke test；arm64 与 x86_64 应分别发布签名包。旧工作流若携带另一台电脑的无效绝对路径，运行时只能在验证本机稳定 binding 后回退；仍然有效的用户自定义外部路径保持最高优先级。

#### 每个插件独立维护 Python 与模型环境

Python 插件不得激活或修改用户现有的 Conda 环境，也不得与其他插件共用一个可变
环境。`install.sh` 应在 `POSDK_INSTALL_STATE_ROOT` 下创建当前插件版本和目标平台
专属的 prefix；可以优先使用系统已有的 Conda/Mamba/Micromamba 作为环境创建器，
没有时再下载由发布者固定版本并校验 SHA-256 的 Micromamba。Conda 包使用 explicit
lock，pip 使用 `--require-hashes`，模型与源码归档固定不可变 revision 和 SHA-256。
安装报告通过签名的 `python_bin` binding 返回私有 `env/bin` 目录，由 PoMVG 自动
回写经过校验的 `python_bin_folder`；插件只执行该目录中固定名称的 Python
可执行文件，不能执行安装器返回的任意命令字符串。最终门禁必须同时验证 import、
架构和准确版本、真实模型/后端 smoke test，以及安装后原生插件的 runtime contract。

### 4.2 发布前编辑 manifest.json

在分发之前，更新脚手架生成的 `manifest.json`。对方法插件来说，
`input_types` / `output_types` 也是 GUI 静态加载普通端口的唯一来源；
没有普通输入或输出时必须显式写成 `[]`，不能省略。

同一个源码目录如果构建多个方法插件，请使用插件专属 manifest，查找优先级为：

1. `manifest.<plugin_name>.json`
2. `<plugin_name>.manifest.json`
3. `manifest.json`

`.ini` 中的 `# @Prior:` 只声明底部 Prior 入口可接受的数据类型；
`# @output:` 只标记条件输出状态，不会新增普通端口。

```json
{
  "format_version": 1,
  "name": "my_feature_matcher",
  "display_name": "My Feature Matcher",
  "version": "1.0.0",
  "author": "Your Name",
  "description": "使用学习描述符的高性能特征匹配",
  "plugin_type": "method",
  "plugin_files": ["plugins/methods/posdk_plugin_my_feature_matcher.dylib"],
  "config_files": ["configs/methods/my_feature_matcher.ini"],
  "input_types": ["data_images"],
  "output_types": ["data_features"],
  "base_class": "MethodPresetProfiler",
  "min_posdk_version": "1.0.0",
  "tags": ["features", "matching", "deep-learning"],
  "homepage": "https://github.com/yourname/my-feature-matcher",
  "license": "MPL-2.0"
}
```

### 4.3 发布到 GitHub（市场）

PoMVG GUI 的插件市场使用 **GitHub Releases** 作为二进制分发后端，posdk.net 只保存元数据、Logo、评分、下载量和已核验的 Release 链接。闭源模式的仓库只需公开 `posdk-marketplace.json`、`manifest.json`、`publisher-key.pem`、供审核的 `install.sh`、可选 README，以及每个平台的 Release 资产；全开源模式则使用完整插件源码仓库。

推荐先在 GUI 的 **插件测试与发布 → 发布** 中完成可信测试、模式化预检和
**正式检验并打包 .pospkg**。发布页使用本机 GitHub CLI 的浏览器授权，PoMVG
不读取或保存 GitHub 密码与 Token。选择已有或新建多插件仓库后，先执行
**检查仓库**，确认这是新建、追加还是更新，再显式点击 **发布到 GitHub**。
在本机终端或 SSH 中，使用插件目录自带的相同发布向导：

```bash
export POSDK_PLUGIN_SDK_ROOT="/path/to/PoSDK Plugin SDK"
cd /path/to/MyFeatureMatcher
./publish.sh
```

如果 `posdk` 已加入 `PATH`，可以省略 `POSDK_PLUGIN_SDK_ROOT`。

脚本会依次提示输入 `owner/repository`、通过 GitHub 官方浏览器/设备码授权、
核对新建/追加/更新计划、上传签名 `.pospkg` 与 SHA-256，然后打印 posdk.net
一次性授权链接。用户在任意浏览器登录 posdk.net，核对 12 位授权码和插件 ID
后批准即可。短时令牌只绑定当前插件，发布成功后立即失效；PoMVG 不读取或保存
GitHub/posdk.net 密码。也可预填参数，但仍保留两次明确确认：

```bash
./publish.sh --repository owner/PoSDK-plugins \
  --package ./dist/my_feature_matcher-1.2.3-macos-arm64.pospkg
```

以下命令保留给诊断或自动化拆分，不是普通用户的首选流程：

```bash
tools/posdk plugin prepare-release --source ./MyFeatureMatcher \
  --id my_feature_matcher --type method \
  --distribution-model closed_plugin_open_dependencies
tools/posdk plugin publisher-init --source ./MyFeatureMatcher
tools/posdk plugin audit-release --source ./MyFeatureMatcher \
  --build-root ./build --id my_feature_matcher --type method
tools/posdk plugin release --source ./MyFeatureMatcher \
  --existing-build-root ./build --output-dir ./MyFeatureMatcher/dist
tools/posdk plugin github-plan --source ./MyFeatureMatcher \
  --repository owner/PoSDK-plugins
tools/posdk plugin github-publish --source ./MyFeatureMatcher \
  --package ./MyFeatureMatcher/dist/my_feature_matcher-1.2.3-macos-arm64.pospkg \
  --repository owner/PoSDK-plugins \
  --confirm-plugin-id my_feature_matcher
```

发布资产必须使用 `<id>-<version>-<system>-<arch>.pospkg` 命名，并同时上传 `.sha256`。macOS、Linux 或不同架构应分别生成包；包内只携带该目标的动态库和依赖安装方案。`publish.sh` 会在 GitHub 上传完成后继续完成 posdk.net 发布，无需再手工复制仓库链接。

同一发布者可以维护统一的多插件仓库。每个插件固定占用
`<仓库>/plugins/<plugin-id>`，不能创建嵌套 Git 仓库；向 posdk.net 提交默认
分支的 `tree/main/plugins/<plugin-id>` 链接。每个插件使用独立的
`posdk-<plugin-id>-v<version>` Release Tag，因此相同版本的不同插件不会冲突。
新增/更新依据插件 ID、发布者公钥和仓库目录判断，不按显示名称判断。同版本
签名资产的 SHA-256 不允许覆盖；内容变化时必须提升版本。

发布页或 **帮助 → PoMVG 账户…** 可以连接 PoMVG 账户：GUI 在系统浏览器中通过 PKCE 授权，再用一次性
`posdk://account/connect` 回调返回；访问 Token 只存系统钥匙串，网站只存哈希。
GitHub 发布完成后点击 **发布到 posdk.net**，网站仍会重新检查仓库身份、Group、
平台、公钥、Release 与 SHA-256。GitHub 身份和 PoMVG 身份彼此独立，不共享凭据。

当前 v3 的签名 `install.sh` 执行器正式支持 macOS 和 Linux。Windows Marketplace 先保持失败关闭，等签名 PowerShell 安装协议完成后再开放；不能把需要 `/bin/sh` 的包错误标记为 Windows 可安装。

> **闭源提示**：不要把裸 `.dylib`、`.so` 或 `.dll` 作为 Marketplace 下载链接。将它放入签名 `.pospkg v3`；这样系统/架构/ABI、Group、`install.sh`、私有依赖、QML 和配置都由同一个签名清单约束。

---

## 5. 安装插件

### 5.1 从 .pospkg 文件安装

如果有人直接与您分享 `.pospkg` 文件：

1. 打开 **PoMVG GUI**
2. 在 Modules 面板中点击**齿轮图标**（安装与管理插件 / Install & Manage Plugins）
3. 在 **已安装插件包（Installed Packages）** 标签中，点击 **从文件安装...（Install from File...）**
4. 选择 `.pospkg` 文件
5. 如果签名密钥尚未受信任，请从发布者的独立可信渠道取得 Ed25519 公钥；可选择“仅本次使用”或“信任并安装”
6. Consumer 仅安装可信签名包；Marketplace 强制使用带包内安装程序的 v3，Developer 对未签名或 legacy 包需要两次风险确认
7. 安装器先验证平台、架构、ABI、哈希和签名，再运行包内 `install.sh`；如声明管理员权限，由操作系统单独请求授权
8. 严格安装报告通过后，PoMVG 才以带锁事务写入 Plugin Store，并刷新对应的 Data/Method Group
9. 新增插件可立即使用；替换已激活插件时界面会提示重启

### 5.2 从市场安装

要从 posdk.net 浏览和安装插件：

1. 打开 **安装与管理插件（Install & Manage Plugins）**（齿轮图标）
2. 切换到 **插件市场（Marketplace）** 标签
3. 按 Method/Data、Group 或关键词筛选
4. 点击目标插件的 **安装（Install）**，确认发布者和目标平台信息
5. PoMVG 直接下载 posdk.net 已核验的当前系统 `.pospkg` Release 资产；无需 GitHub CLI，也不会 clone 或编译源码
6. 在安装状态列表查看下载、签名验证、依赖准备、Plugin Store 提交和加载检查结果

### 5.3 卸载插件

1. 打开 **安装与管理插件（Install & Manage Plugins）**
2. 在 **已安装插件包（Installed Packages）** 标签中，点击插件旁边的 **卸载（Uninstall）**
3. 插件文件从 `~/.posdk/plugins/` 中删除，注册表更新

---

## 6. 插件目录结构

已安装的插件位于用户目录中，与内置插件分开：

```
~/.posdk/
├── plugins/
│   ├── methods/           # 已安装的 method 插件 .dylib/.so 文件
│   └── data/              # 已安装的 data 插件 .dylib/.so 文件
├── configs/
│   ├── methods/           # 可编辑的 method .ini 配置文件
│   └── data/              # data 配置文件
├── trusted_keys/          # 用户显式信任的 Ed25519 公钥
├── registry.json          # schema v2；记录版本、文件所有权和依赖
├── plugin_paths.json      # schema v2；自定义路径的稳定 ID、启停和优先级
└── sources.json           # 市场源仓库
```

所有路径、配置模板和 QML 资源都由进程级 `PluginPathManager` 解析。Consumer 优先级为 App 内置、自定义路径、用户安装；Developer 只在显式会话中增加构建输出或 override。实际执行使用只读、内容寻址的 generation，而不是直接加载可写安装目录。

在已签名的 macOS Consumer App 中，`Contents/PlugIns` 只保存插件 dylib
和显式声明的原生 runtime target。与二进制同 stem 的 `.posdk.json`、
`.manifest.json`、`.dependencies.json`、`.runtime-files.json`，以及
`RUNTIME_FILES`、`RUNTIME_DIRECTORIES` 和 `log_styles` / `input_controls` /
`eval_views`、`evaluator_plots`，统一放在
`Contents/Resources/plugin_payloads/{methods,data}`。`PluginPathManager`
在创建不可变 generation 前组合只读 metadata root 与 binary root。这样普通资源由
App 外层签名的 CodeResources 封存，只有原生代码获得嵌入式代码签名，符合 macOS
nested-code 布局要求。

宿主 App 的 Developer ID 签名与公证是两个独立发布门槛。macOS
`Distribution` 构建若将公证设为 `Off`，状态只能是
`signed-but-not-notarized`，仅适合作为内部产物。公开发布的 Consumer App
必须使用 `Required` 公证；Apple 接受并完成 ticket staple 后，构建还必须通过
`spctl --assess --type execute`。本机 Gatekeeper 偶然接受未公证 App，并不能把
内部产物提升为可公开分发状态。

---

## 快速参考

| 任务 | 方法 |
|---|---|
| 创建新插件 | 工具栏 → **Developer** 按钮 |
| 构建插件 | `cmake --build . --target <name>` |
| 在 GUI 中测试 | Plugins → **插件测试与发布** → **测试** |
| 创建签名 `.pospkg v3` | Plugins → **插件测试与发布** → **发布** → **正式检验并打包 .pospkg**，或 `tools/posdk plugin release ...` |
| 创建 Developer 未签名包 | `tools/posdk plugin package ... --allow-unsigned` |
| 发布到 GitHub + posdk.net | 插件测试与发布中的发布向导，或在插件目录运行 `./publish.sh` |
| 安装 .pospkg | 安装与管理插件 → 已安装插件包 → **从文件安装...** |
| 从市场安装 | 安装与管理插件 → 插件市场 → **安装** |
| 卸载 | 安装与管理插件 → 已安装插件包 → **卸载** |

---

## 进一步阅读

- [插件开发快速示例](method_example.md)
- [Method 插件接口详解](method_plugins.md)
- [Data 插件接口详解](data_plugins.md)
- [配置文件（INI）](../advanced_development/configuration_files.md)
- [性能分析](../advanced_development/profiler.md)
- `plugins/cmake/PLUGIN_CMAKE_USAGE.md`
- [在线文档](https://posdk.readthedocs.io/en/latest/)
