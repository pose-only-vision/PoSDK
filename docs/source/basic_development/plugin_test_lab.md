# 插件测试与发布（Plugin Test & Release）

Plugin Test & Release 是 PoMVG Developer 发行版中的统一插件质量与发行工作台。
它分为两个页签：

- **测试**：保留 Plugin Test Lab 的可信测试能力，负责打包前预检测、运行时
  契约、算法用例、性能和鲁棒性证据。
- **发布**：选择三种发行模式之一，执行模式化发布预检，并在门禁通过后生成
  签名 `.pospkg`。

测试页的默认目标是：

```text
构建插件 → 选择插件 → Quick Test → 查看结果
```

开发者不需要手工管理插件 generation、运行时 snapshot 或测试报告路径。GUI
负责验证当前构建、固定不可变插件快照、隔离执行每个用例并生成 JSON/JUnit
报告。

Test Lab 的核心结论只分为两类，另加一道自动系统门禁：

| 类别 | 具体做什么 | 通过代表什么 | 不代表什么 |
|---|---|---|---|
| 系统前置门禁 | 校验 generation、manifest、ABI、arm64、依赖与可加载性 | 当前构建能被独立 runner 安全加载 | 没有执行算法 |
| 算法接入与运行 | 通过 Factory 创建输入，绑定必需数据，调用 `Build()`，检查非空输出及声明端口 | 数据确实进入所选 Method/Data 插件并产生约定输出 | 不自动证明精度 |
| 仿真与精度 | 使用固定 seed 和已知真值运行仿真，计算结构化精度指标并执行门限 | 报告中的样本、精度值和门限全部通过 | 不能外推到未声明的真实数据分布 |

性能基准和异常输入/安全鲁棒性仍保留为进阶测试，但不会混淆上述两个核心
开发闭环。若一个计划只有 `runtime_contract`，界面会明确显示“尚未执行算法”，
不会产生算法测试的假绿色。

## 1. 最快开始

在 `PoSDK_github/script` 下执行官方日常开发脚本：

```bash
./build_po_core_sync_plugin_dev.sh
```

脚本默认完成：

- 构建并同步 `po_core`
- 构建 PoMVG GUI 和第一方插件
- 构建已声明的插件测试 driver
- 检查 arm64 Mach-O、OpenCV 依赖闭包和 Runner/Factory ABI
- 启动 Developer GUI

打开 **Plugins → Plugin Test & Release**，进入 **Test / 测试** 页：

1. 在 Method/Data 分组树中选择 Developer 插件，或在 Behavior Workspace
   选中模块后点击 Configuration 顶部的测试按钮。
2. 选择 Quick、Standard、Benchmark 或 Security 测试档位。
3. 在测试任务表中查看每个任务的用途并勾选要运行的项目；默认全选。
4. 点击 **运行已选**，或直接点击 **Quick Test** 运行全部 Quick 任务。
5. 在任务表和结果区查看逐项状态、通过数、失败数、耗时、失败原因和可信报告。

任务表固定显示四列：

- 圆形勾选按钮：决定本次是否运行该 case。
- 测试任务：优先显示计划声明的可读标题，并直接标记“系统前置门禁”、
  “算法接入/运行”或“仿真/精度”；driver 保留在详细信息中。
- 测试内容：用普通语言说明该任务验证加载契约、算法断言、性能阈值还是异常输入，
  同时显示超时、repeat、seed 和实际 driver target。
- 状态：显示未选择、待运行、构建中、测试中、已通过或失败；通过项同时显示耗时。

多项勾选不是 GUI 侧的模拟队列。所选 case 会作为同一次严格选择传给
`posdk_test_runner`，按 `posdk.tests.json` 中的声明顺序执行，并由 GUI 对
完整 execution set、进程退出码及 JSON/JUnit 证据进行核验。取消勾选的任务
不会被标成通过。

Test Lab 标题栏可以拖动，完整外框始终可见。它仍是显式关闭的模态窗口：
点击主 GUI 不会关闭窗口或把窗口放到后面。

Developer App 内的插件副本只要与 linked build 和活动 runtime generation
内容一致，就属于同一次构建，不需要开发者再次手工激活路径。
如果源码已修改而活动插件仍是旧构建，Quick Test 和任务表中的 Run selected
都会调用 Developer 构建、激活新的不可变 generation，并按原 Profile/Case
自动重试一次；不需要退出 Test Lab。

Test Lab 是显式关闭的模态工作台。点击主 GUI 的空白区域不会关闭或把它藏到
主界面后面；测试或构建期间需要先停止，空闲时可使用 `Esc` 或右上角关闭按钮。
自动构建的 configure、测试目标发现和 compile 都使用异步子进程，不会占用
GUI 事件循环。

每次操作都会保留独立的诊断日志：

- 测试预检和隔离运行生成 `plugin-test-lab/*.log`
- 自动 Developer 构建生成 `developer-builds/*.log`

结果区会显示本次实际日志路径，并提供 **显示诊断日志** 按钮。日志包含插件
身份、Profile/Case、当前构建根、CMake 输出、generation 哈希诊断、runner
退出状态和可信报告路径。发送问题反馈时应同时附上该 `.log`，不必只截取红色
错误条。

测试结束后点击 **查看报告 / View report**，GUI 会从已经完成严格 schema、
身份、execution set、证据和进程退出码校验的 `report.json` 生成自包含
`report.html`，并直接在默认浏览器打开。该页面离线工作、没有 JavaScript，
也不会启动监听局域网的 HTTP 服务；它首先显示本次实际覆盖与未覆盖的类别，
再为每个 case 说明测试动作、通过含义、结论边界、耗时与证据。运行环境、哈希、
资源限制和原始 artifact 收在可展开的技术证据区，避免内部元数据压过测试结论。
**在 Finder 中显示 / Show in Finder** 则继续用于取得原始 `report.json`、
`junit.xml` 和同目录 artifacts。HTML 只是可信 JSON 的只读视图，不参与测试
结论判定。

## 2. 旧插件没有测试计划

新建插件已经自动包含 `posdk.tests.json`。旧插件若缺少该文件，界面会显示
**Create & Quick Test / 创建并快速测试**。

点击后，GUI 在插件源码目录中创建一个不覆盖现有文件的 starter plan：

```json
{
  "schema_version": 1,
  "plugin": {
    "id": "my_plugin",
    "kind": "method"
  },
  "resources": [],
  "profiles": {
    "quick": ["runtime_contract"],
    "standard": ["runtime_contract"]
  },
  "cases": [
    {
      "id": "runtime_contract",
      "title": "Runtime compatibility gate | 运行兼容性门禁",
      "description": "Pins and validates the active generation; it does not execute the algorithm. | 固定并验证活动 generation；不会执行算法。",
      "driver": "snapshot",
      "timeout_seconds": 15,
      "repeat": 1,
      "seed": 42
    }
  ]
}
```

`runtime_contract` 验证插件 ID、ABI、架构、manifest、依赖闭包和加载能力。
它不代表算法精度已经得到验证。Test Lab 会把这一点明确显示在引导中，避免
“能够加载”被误解为“算法正确”。

建议每个 case 都填写 `title` 和 `description`，并采用
`English | 中文`。这两项会经过严格计划校验、固定进可信报告，并直接显示在
任务表和离线 HTML 中。`description` 应说明实际输入、关键断言和门限；不要只
重复 case ID。

## 3. 增加算法测试

在插件 CMake 中通过 Developer SDK 声明 GTest：

```cmake
if(POSDK_BUILD_PLUGIN_TESTS)
    posdk_add_plugin_gtest(
        TARGET test_my_plugin_smoke
        PLUGIN_ID my_plugin
        PLUGIN_KIND method
        PLUGIN_TARGET my_plugin
        SOURCES tests/test_my_plugin_smoke.cpp
        LINK_LIBRARIES PoSDK::po_core
    )
endif()
```

不要链接 `gtest_main`。PoMVG 提供的 attested main 会应用与 Test Lab 相同的
精确 runtime snapshot，并把执行身份发送给测试控制器。

先加入一个接入/运行用例。它应检查输入创建和绑定、`Build()` 确实返回、输出
`DataPackage` 非空以及声明的输出端口存在：

```json
{
  "id": "algorithm_integration",
  "title": "Algorithm integration and run | 算法接入与运行",
  "description": "Binds all required inputs, calls Build(), and checks the declared output ports. | 绑定全部必需输入，调用 Build()，并检查声明的输出端口。",
  "driver": "gtest",
  "timeout_seconds": 60,
  "repeat": 1,
  "seed": 42,
  "executable": "output/bin/plugin-tests/test_my_plugin_smoke",
  "arguments": [
    "--gtest_filter=MyPluginTest.AlgorithmInputOutputIntegration"
  ],
  "evidence": {
    "kind": "functional"
  }
}
```

将 `algorithm_integration` 加入 `quick` profile。再次点击 Quick Test 时，
GUI 会同时执行系统门禁和算法接入/运行。

有仿真真值后，再加入 `evidence.kind=accuracy`。测试程序必须在
`POSDK_TEST_RESULT_DIR` 下写出 `metrics_file`；GUI 只有在可信 GTest XML、
指标文件、样本数和全部门限一致时才显示精度通过：

```json
{
  "id": "simulation_accuracy",
  "title": "Simulation and accuracy | 仿真与精度",
  "description": "Runs fixed ground-truth simulation and reports pose errors. | 运行固定真值仿真并报告位姿误差。",
  "driver": "gtest",
  "timeout_seconds": 120,
  "repeat": 1,
  "seed": 42,
  "executable": "output/bin/plugin-tests/test_my_plugin_smoke",
  "arguments": [
    "--gtest_filter=MyPluginTest.SimulationAccuracy"
  ],
  "evidence": {
    "kind": "accuracy",
    "metrics_file": "accuracy-metrics.json",
    "thresholds": [
      {
        "metric": "max_rotation_error_deg",
        "unit": "deg",
        "maximum": 2.9
      }
    ]
  }
}
```

离线报告会把 `accuracy-metrics.json` 渲染为“指标、实测结果、门限、样本数、
状态”精度表，而不是只给一个模糊的绿色结论。

## 4. 四个测试档位

### Quick

- 每次构建后运行
- 固定 seed、无网络、可重复
- runtime contract
- 一个具有明确数值/结构断言的算法 smoke
- 建议控制在数秒到一分钟内

### Standard

- 提交或合并前运行
- 多场景、多固定 seed
- 噪声、外点率、数据规模矩阵
- 精度和完整输出断言

### Benchmark

- 显式性能测试
- 输出结构化 metrics JSON
- 至少包含一个最小值或最大值阈值
- 跨机器比较时同时归档机器、系统和 ABI 身份

### Security

- 缺失、损坏和边界输入
- 崩溃、超时、输出/内存/进程限制
- 每个 robustness scenario 必须绑定到唯一、实际执行并通过的
  `Suite.Test`
- Host 结果只表示可信 Developer 插件的鲁棒性，不是恶意代码安全认证

## 5. 测试资源

workflow、配置、模型和 fixture 必须作为相对资源声明：

```json
{
  "path": "tests/fixtures/smoke.json",
  "size_bytes": 1024,
  "sha256": "<64 lowercase hex>"
}
```

不要在计划中使用开发机绝对路径。控制器会在运行前验证大小和 SHA-256，并把
资源复制到本次私有结果目录。工作流配置通过
`posdk-resource://tests/fixtures/...` 引用这些资源。

## 6. 结果判定

以下情况都会失败，不能显示绿色：

- 没有实际执行用例
- 全部 skipped
- 进程崩溃、超时或资源超限
- snapshot/generation attestation 不匹配
- GTest XML 缺失、损坏或执行数量不符
- 计划声明的 case/repeat/seed 与报告不一致
- Benchmark 没有结构化指标或未通过阈值
- Accuracy 没有结构化精度指标、样本数为零或未通过阈值
- Security scenario 没有对应的已执行 GTest

GUI 的摘要用于快速阅读；最终可信结果由进程退出码、严格 JSON 报告和 JUnit
共同判定。

在 macOS 上，隔离测试会把 fixture、临时文件和结果保留在本次运行的私有目录，
但 GTest 认证通道使用 `/private/tmp` 下每次运行唯一的、权限为 `0700` 的短
Unix socket 目录。这是为了避开 Darwin 的 socket 路径长度限制，不会放宽测试
文件的隔离范围；通道仍要求随机 token 认证，并在运行结束后删除。若认证通道
创建失败，诊断日志会记录 endpoint、字节长度、Qt 错误码和底层错误信息。

## 7. 常见问题

### 当前构建不匹配

说明插件源码或 linked build 已变化，而活动 generation 仍是旧内容。默认
Quick Test 与 Run selected 都会自动构建并重试一次。若自动构建失败，应查看
结果区的 **运行输出** 和 **显示诊断日志**；Test Lab 不会继续运行旧插件。
自动构建必须使用当前运行 Developer App 对应的 CMake preset build root，
不能回退到另一个陈旧的 `build/` 缓存。仅仅因为 Developer App 把相同二进制
复制进 `.app` 不会触发此错误。

### Profile 为空

测试计划缺失或不合法。缺失时使用 **Create & Quick Test**；已有但不合法时，
Test Lab 不会覆盖文件，应根据具体诊断修复。

### GTest 找不到

确认：

- CMake 使用 `posdk_add_plugin_gtest()`
- `POSDK_BUILD_PLUGIN_TESTS=ON`
- `executable` 位于当前 build root 的 `output/bin/plugin-tests`
- `posdk.tests.json` 中使用相对路径

### Python/模型插件出现假通过

测试必须断言实际 backend 和模型身份。若插件会回退到 OpenCV 或其他实现，
仅检查“返回成功”是不充分的；报告必须能够区分真实算法与 fallback。

## 8. CLI/CI

GUI 与 CLI 使用同一份测试计划和控制器：

```bash
tools/posdk plugin build \
  --source /absolute/path/to/MyPlugin \
  --build-dir /absolute/path/to/build \
  --sdk-root /absolute/path/to/plugin_sdk \
  --tests --id my_plugin --config Release

tools/posdk plugin test \
  --plan /absolute/path/to/posdk.tests.json \
  --profile quick \
  --case runtime_contract \
  --case deterministic_smoke \
  --plugin-binary /absolute/path/to/posdk_plugin_my_plugin.dylib \
  --build-dir /absolute/path/to/build
```

`--case` 可以重复，用于运行显式子集；实际执行顺序始终使用测试计划中的声明
顺序，而不是命令行参数顺序。不提供 `--case` 时运行整个 profile。

CI 应在每次提交运行 Quick，合并前运行 Standard/Security，并在固定机器上运行
Benchmark。ASan、UBSan、TSan 属于独立构建通道，不应伪装成普通 profile。

## 9. 发布页

发布页把过去散落在 Developer 创建窗口和命令行中的发布动作收拢为一条可审计
流程。Developer 创建窗口只负责生成插件工程；已有插件或刚创建的插件都从
**Plugins → Plugin Test & Release → Release / 发布** 进入正式发行。

### 9.1 三种发行模式

- `closed_plugin_open_dependencies`：插件动态库闭源，依赖源码、固定版本、哈希、
  许可证和构建配方公开；适合外部程序、Python/模型和原生库包装器，也是默认
  推荐模式。
- `closed_plugin_closed_dependencies`：插件与依赖源码均不公开；仍须满足完整
  运行闭包、再分发许可、第三方声明和适用的源码提供义务。
- `open_plugin_open_dependencies`：插件源码和依赖配方均公开；预检还要求源码、
  CMake、Git 仓库和已提交的干净工作树。

三种模式只决定源码与依赖材料如何公开，不改变签名、ABI、平台、架构、依赖
闭包、隔离安装和运行测试门禁。

### 9.2 发布门禁

正式打包按钮同时要求：

1. 已定位当前插件的源码、当前平台构建和 PoMVG CLI；
2. 当前所选插件最近一次可信测试通过；
3. 当前所选发行模式的 **Run release preflight / 运行发布预检** 通过。

正式操作按统一后端顺序执行：

```text
prepare-release → publisher-init → audit-release → release
```

`prepare-release` 只补齐或更新发布契约；`publisher-init` 准备受保护的 Ed25519
密钥；`audit-release` 重新执行模式化审计；`release` 生成并自检目标系统与架构
的签名 `.pospkg`。GUI 不会自动上传 GitHub Release。

每次发布预检和正式打包都写入独立的 owner-only 日志。发布页显示日志路径、
完整输出和最终安装包路径，并提供 Finder 操作，不能只依赖一条短错误提示。

### 9.3 AI 协助打包

点击 **AI packaging help / AI 协助打包** 会调用 PoMVG 内部 Codex Agent，并将
所选发行模式、当前插件源码根、构建根、日志路径和诊断输出作为上下文。授权
范围只覆盖当前插件源码目录；Agent 可以修改插件、依赖配方、`install.sh`、
测试和发行契约，也可以运行构建、测试与发布预检，但不得自动上传 GitHub
Release。修复完成后仍必须重新通过可信测试、模式化预检和正式签名包校验。

### 9.4 GitHub 与 posdk.net 发布指南

发布页在正式打包操作下方提供两个只读指南入口：

- **发布到 GitHub**：GUI 通过本机 `gh auth login --web` 连接 GitHub，不接触
  密码或 Token。输入已有或新建仓库后先检查 `create_repository / append /
  update` 计划；用户确认后，GUI 只提交规范的 `plugins/<plugin-id>` 目录，并将
  已签名 `.pospkg` 与 `.sha256` 上传到
  `posdk-<plugin-id>-v<version>` Release。同版本不同 SHA-256 必须提升版本。
- **发布到 posdk.net**：GUI 通过浏览器 PKCE 和一次性 Deep Link 连接 PoMVG
  账户，Token 只保存在系统钥匙串。GitHub 发布成功后可直接登记或更新市场；
  网站仍会检查身份、Group、平台、公开材料、公钥、Release 与 SHA-256。

GitHub 与 PoMVG 账户互不共享凭据。所有外部写入都需要用户点击对应发布按钮；
“浏览器发布”保留为 posdk.net 手工登记的兜底入口。
