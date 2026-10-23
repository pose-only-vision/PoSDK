# Plugin Development, Publishing & Installation Guide

This guide covers the current PoMVG plugin lifecycle: scaffold a plugin in the GUI, implement it with the `po_core` interfaces, build it with `add_posdk_plugin(...)`, and distribute it with the package manifest plus the runtime `.posdk.json` sidecar.

---

## 1. Creating a New Plugin

### Using the Develop Button

PoMVG GUI provides a built-in scaffolding flow through **DeveloperDialog**.

1. Open **PoMVG GUI**
2. Click the toolbar **Develop** button
3. Fill in the dialog:
   - **Plugin Type**: Method or Data
   - **Plugin Name**: `snake_case`, for example `my_feature_matcher`
   - **Display Name**: the human-facing default label, for example
     `My Feature Matcher` or `特征匹配`; this is written to Manifest
   - **Class Name**: auto-generated `PascalCase`, for example `MyFeatureMatcher`
   - **Base Class**: `MethodPresetProfiler`, `RobustEstimator`, `MethodPreset`, or `DataIO`
   - **Input / Output Data Types**: for method plugins
   - **All Platforms Supported** or explicit **Systems / Architectures**
   - **Generate `.claude/CLAUDE.md`** for AI-assisted development
   - Optional custom QML starter files such as `log_styles` or `input_controls`
4. Click **Generate & Open**

Typical method plugin output:

```text
plugins/methods/MyFeatureMatcher/
├── CMakeLists.txt
├── my_feature_matcher.hpp
├── my_feature_matcher.cpp
├── my_feature_matcher.ini
├── manifest.json
├── README.md
└── .claude/
    ├── CLAUDE.md
    └── commands/
        └── build.md
```

The runtime sidecar manifest is generated later by the build system:

```text
output/plugins/methods/posdk_plugin_my_feature_matcher.posdk.json
```

### Manual Creation

If you scaffold manually, keep these minimum requirements:

- A `CMakeLists.txt` that calls `add_posdk_plugin(...)`
- A class derived from a PoMVG method or data base class
- `REGISTRATION_PLUGIN(...)` at the end of the `.cpp`
- Optional method `.ini`, `log_styles`, `input_controls`, `eval_views`, or `evaluator_plots`

### Stable IDs and presentation names

PoMVG deliberately separates runtime identity from UI labels:

- `name` / `plugin_name` is the canonical plugin ID used by Factory, ports,
  Runner, configuration paths, logs, and package verification. Never rename it
  to change how a module looks in the GUI.
- `display_name` in the source Manifest is the plugin author's default label.
- `group` is the publisher's default Modules Group. It may be empty or a
  canonical slash-separated path such as `SfM/Estimators`; a local user Group
  assignment overrides it without changing the plugin package.
- A Modules rename is a local user alias stored in application settings.
- A Behavior Workspace rename is an instance label stored as `displayName` on
  that node in `.posdk`; it is never copied into the Runner contract or INI.

Display names may contain spaces, Chinese, and ordinary punctuation. They must
be trimmed, Unicode NFC-normalized, at most 80 UTF-16 code units / 256 UTF-8
bytes, and must not contain control or formatting characters.

---

## 2. Build and Registration Model

PoMVG plugin projects require CMake 3.19 or newer. The build validates source
manifests with CMake's structured JSON parser before generating runtime
sidecars; current CLI and GUI scaffolds declare this minimum automatically.

### `add_posdk_plugin(...)`

PoMVG plugins are built through `plugins/cmake/plugin-config.cmake`.

```cmake
add_posdk_plugin(my_feature_matcher
    PLUGIN_TYPE methods
    SOURCES my_feature_matcher.cpp
    HEADERS my_feature_matcher.hpp
    SUPPORTED_SYSTEMS macos linux
    SUPPORTED_ARCHES arm64 x86_64
)
```

Current build behavior:

- Output library name is `posdk_plugin_<plugin_name>.{dylib|so|dll}`
- A runtime sidecar manifest `posdk_plugin_<plugin_name>.posdk.json` is generated beside the library
- `PluginRegistration` and `PluginGetType` are validated with `nm` when the toolchain supports it
- `log_styles/`, `input_controls/`, `eval_views/`, and `evaluator_plots/` are copied automatically
  to the runtime plugin directory and installed with the plugin
- Omitted `SUPPORTED_SYSTEMS` / `SUPPORTED_ARCHES` means “support all known platforms”

See also: `plugins/cmake/PLUGIN_CMAKE_USAGE.md`

### `REGISTRATION_PLUGIN(...)`

Prefer the single-argument form so CMake remains the single source of truth:

```cpp
REGISTRATION_PLUGIN(PluginMethods::MyFeatureMatcher)
```

This macro:

- implements `GetType()` automatically
- exports `PluginRegistration`
- exports `PluginGetType`
- exports `PluginGetVersion`

### Runtime Filtering

At runtime, PoMVG does not trust filename matches alone and never loads a
library merely to discover its identity.

- Statically valid plugin + compatible platform: may be published to `Factory*`
  and appears in `Modules`; this is eligibility, not proof that the dynamic
  library and every dependency can be loaded
- Valid plugin + incompatible platform: appears in **Plugin Paths** diagnostics, but does not load
- Missing, malformed, or inconsistent runtime manifest: marked invalid and skipped
- Binary format or architecture not matching the host: marked unsupported or invalid and skipped

The runtime manifest is strict schema v1. Unknown fields, wrong JSON types,
non-canonical IDs, filename/ID mismatches, category/directory mismatches, and
duplicate or non-canonical platform values are rejected. The library itself is
also inspected as Mach-O, ELF, or PE; manifest architecture claims alone are
not sufficient.

Static validation never loads plugin code and therefore cannot prove that
`PluginGetType` and `PluginRegistration` are present or that the platform
loader can resolve every dependency. The build-time `nm` check catches missing
exports when the toolchain supports it. Otherwise, runtime verification occurs
only in the isolated Developer diagnostic probe or when `po_core` creates the
plugin. `Modules` must consequently be read as “statically eligible modules,”
not as a guarantee that every listed module has already been loaded.

Both Consumer and Developer registries require a valid strict runtime manifest;
a Developer probe of a legacy binary is diagnostic only and never makes that
binary eligible for Factory publication. Consumer distributions never
dynamically probe manifest-less legacy plugins. The Consumer executable neither
builds nor bundles `posdk_plugin_probe`; diagnostic probing is a Developer-only
capability and fails closed if invoked in Consumer mode.

In a signed macOS Consumer app, native plugin binaries and declared native
runtime targets are the only plugin-owned files under `Contents/PlugIns`.
Same-stem `.posdk.json`, `.manifest.json`, `.dependencies.json`, and
`.runtime-files.json` contracts plus declared runtime files/directories and QML
extension trees are staged under
`Contents/Resources/plugin_payloads/{methods,data}`. `PluginPathManager`
combines that read-only metadata root with the binary root before creating the
immutable generation. This split follows Apple's nested-code layout: data is
sealed by the outer app signature, while only native code receives an embedded
code signature.

Host-app signing and notarization are separate release gates. A macOS
`Distribution` build with notarization `Off` is explicitly
`signed-but-not-notarized` and is suitable only as an internal artifact. A
public Consumer release uses notarization `Required`; after Apple accepts and
the ticket is stapled, the build must pass
`spctl --assess --type execute`. Local Gatekeeper acceptance without
notarization never upgrades an internal artifact to public-distribution ready.

For a Developer diagnostic, the Manager performs only the request/path
preconditions before launching protocol v2. The short-lived
`posdk_plugin_probe` helper then performs the complete native probe pipeline:
static binary and manifest inspection, source-fingerprint verification,
dependency/resource closure discovery, immutable staging, generation-digest
verification, and finally dynamic load/export/type checks. Keeping all of that
work in the helper means a timeout can terminate inspection or staging as well
as a hung library load. The helper resolves but never calls
`PluginRegistration`.

The generation created by this helper is a **temporary diagnostic generation**
inside the request's isolated temporary environment. Its path is telemetry for
that probe result only: it is not inserted into a Factory registry, is not used
to run workflows, and is reclaimed with the helper request. The **active Factory
generation** published by the process-wide Manager is the only generation path
used for future runtime object creation.

The **Probe** action is available from each
Developer-mode Plugin Paths diagnostic row and reports timeout, crash, export,
and runtime-type results. The current ABI does not export a plugin category, so
the displayed probe category is the requested value, not independent runtime
verification. A successful legacy probe is diagnostic evidence, not permission
to register the plugin. Process separation, time/output/resource limits, a
sanitized environment, and process-group cleanup contain ordinary plugin
failures; the helper is not an OS security sandbox for hostile native code.
The asynchronous deadline covers the complete static inspection, immutable
staging, and helper-process probe pipeline. A result that finishes after that
deadline is discarded and cannot be reported as a successful probe. Windows
dynamic probing is currently disabled fail-closed with
`probe_containment_unavailable`; it will remain disabled until the helper is
placed in a resource-limited Job Object with kill-on-close descendant cleanup.

The **Scan Plugins** action is separate from that probe pipeline and runs on a
background immutable snapshot; the GUI thread only starts the request and
consumes its result. Refresh or any path/registry change invalidates the active
request, and a stale worker result is discarded. Only one scan worker can
exist at a time, including post-invalidation unwind, so repeated clicks cannot
queue unbounded hashing work. The synchronous `scanAllPluginDetails()` API is
retained for compatibility and tests, not for GUI use.

Production scan defaults are an aggregate 30-second deadline, 2,048 plugin
entries, 2 GiB of content inspection, and a bounded directory traversal. The
byte budget includes the main-binary static SHA-256 and the complete private
generation inspection for each row; per-generation hard limits (including 512
MiB per file and 1,024 files) still apply. Reaching an entry, directory, byte,
or time ceiling returns a visibly partial result with a specific diagnostic
code instead of continuing silently.

Probe-result reuse is bound to the complete generation digest: main binary,
same-stem manifests, dependency contract, and private dependency closure. The
main-binary SHA-256 remains a separate TOCTOU precondition passed to staging.
Path, size, mtime, and `probeToken` are telemetry only. If a complete
generation identity cannot be established, the row is explicitly marked
non-cacheable and its Probe action is disabled. Consequently, changing a
private dependency or `.dependencies.json` invalidates an old result even when
the main dylib bytes are unchanged. A pipeline timeout carries the requested
generation digest so it can still be displayed for the exact scanned row;
the Probe action remains busy until the expired worker has fully unwound.

### Runtime Generations

The GUI publishes method and data registries atomically. Every accepted binary,
its same-stem runtime/port manifests, plugin-private runtime dependency closure,
its selected exact `<normalized-plugin-id>.ini` configuration template, and its
`log_styles`, `input_controls`, `eval_views`, and `evaluator_plots` trees are copied to a private,
content-addressed, process-lifetime generation before the path is given to
`po_core`. Resource and configuration roots follow the same precedence as the
plugin; a shadowed package cannot contribute either one. Symlinks, package
escapes, non-regular files, traversal, and staging-budget violations are
rejected.
Unreferenced sibling libraries are not copied. On macOS, Mach-O load commands
are followed recursively inside the plugin package; app/OS/SDK dependencies
remain explicit host ABI inputs. Safe in-package dependency symlink aliases are
materialized as ordinary generation files; an alias escaping the package is
rejected. The complete generation is validated and made read-only before
atomic publication. A published generation is never repaired, overwritten, or
deleted during the process lifetime.

Workflow handoff uses strict runtime snapshot schema v2. The runner owns the
distribution-mode policy: a Consumer runner accepts only a Consumer snapshot,
so a caller cannot replay Developer roots or legacy compatibility rules into a
shipped app. Before applying any snapshot or running a diagnostic probe, each
helper compares the public Factory runtime ABI epoch compiled into the helper
with the value exported by the loaded `libpomvg_factory`; a stale SDK or bundle
library fails before registry use.

Do not confuse the source/package path or a probe helper's temporary path with
the runtime mapping. In **Plugin Paths**, **Active Factory generation** (also
reported as `factoryPath`/the resolved runtime binary) is the immutable path
actually present in the current Factory registry. **Temporary helper probe
generation** is evidence produced by one diagnostic request and has no runtime
authority.

Private libraries loaded dynamically rather than named by the platform loader
must be declared by a dependency contract. In the plugin source directory,
use `<plugin_name>.dependencies.json` (preferred), its lowercase equivalent,
`dependencies.<plugin_name>.json`, or `dependencies.json` for a one-plugin
source directory. `add_posdk_plugin()` copies and installs it under the exact
runtime name `<plugin-binary>.dependencies.json`. The strict schema is:

Plugin-owned native libraries can be staged explicitly with
`add_posdk_plugin(PRIVATE_DEPENDENCIES
deps/<platform>/libfoo.dylib::<plugin-id>/deps/libfoo.dylib)`. The source must
be a regular non-symlink file inside the plugin directory and the same-stem
dependency contract is mandatory. Host ABI libraries such as Qt, po_core, and
the App-pinned OpenCV runtime remain single-copy host dependencies; do not
duplicate them in every plugin. See `plugins/spec/plugin-private-dependencies-v1.md`
and `plugins/spec/plugin-install-v1.md` for Marketplace repositories.

```json
{
  "schema_version": 1,
  "private_dependencies": ["lib/my_plugin_support.dylib"],
  "external_dependencies": ["@rpath/libpo_core.0.dylib"]
}
```

Private entries are portable paths relative to the plugin binary directory;
absolute paths, `..`, symlink-directory traversal, duplicates, missing files,
and package escapes are rejected. `external_dependencies` declares expected
loader inputs supplied by the app, operating system, or SDK; the runtime still
classifies and verifies the actual native loader closure. On macOS, a non-system
host dependency is bound to its canonical path, SHA-256, and host generation,
then pinned for the process lifetime before the Factory registry is published.
An unresolved or mismatched dependency fails closed. The dependency manifest
itself is included in the generation digest.

The sidecar is not proof that all ELF or PE imports were enumerated. Until the
runtime has native ELF and PE loader-closure parsers with equivalent host
binding, strict Consumer mode rejects dynamic native plugins on Linux and
Windows even when this sidecar is present. Developer-compatible diagnostics may
still use the declaration to inspect a package, but that result is not Consumer
eligibility. A legacy package without the contract is reported as
`legacy-unverified-loader-closure` and is never published.

When an active ID is rebuilt, replaced, overridden, or removed:

- future `Factory::Create()` calls use the newly published immutable generation,
  or no mapping after removal;
- objects that already exist retain their previous generation and library handle;
- **Plugin Paths** raises `restartRequired`; restarting the app is required to
  retire every previous instance, not to make the new registry mapping visible.

The flag is intentionally sticky for the rest of the process lifetime because
an old object or library handle can still exist after the path change; the
compatibility `clearRestartRequired()` entry point does not clear it. New,
previously unseen IDs can be published without a restart warning.

---

## 3. 插件测试与发布（Developer）

### 技术路线与可信性边界

插件测试是 Developer 工具链，不是 Method 模块。`posdk_test_runner` 只校验
测试计划、施加进程资源边界、验证认证事件并生成报告；它不会在自身或 GUI
进程中加载待测 dylib。每个 case 都在新的受约束子进程中运行：

- `snapshot` 和 `workflow` 复用正式 `posdk_runner`；
- `workflow` 使用正式 runner 的认证事件协议，并验证目标插件完成次数以及声明
  为必需的 evaluator/profiler 结果；
- `gtest` 只执行 `--build-root` 内计划明确声明的可执行文件，并从私有 XML
  报告核对真实执行数量。

三类 driver 都绑定同一份不可变 `PluginRuntimeSnapshot v2`。这保证测试的是
GUI 当前激活的精确插件 generation，而不是碰巧同名的旧构建或另一条搜索路径。
零测试、全部 skipped、异常退出、超时、输出超限、认证事件不完整或预期结果
缺失均为失败。`benchmark` 还必须生成结构化指标并通过声明阈值；`security`
必须包含绑定到精确 `Suite.Test` 的 robustness evidence，控制器会从 GTest
XML 核验该 testcase 确实执行并通过；普通 smoke 不能只贴标签冒充这两类门禁。

进程隔离能够控制普通插件崩溃、死循环、输出洪泛和后代进程，但它不是敌对原生
代码的 macOS 沙箱。对来源不可信插件做模糊测试、恶意输入或攻击性安全测试时，
应使用可恢复快照、可直接丢弃的 Parallels macOS 虚拟机，不应在开发主机执行。

### GUI 使用

运行官方 `script/build_po_core_sync_plugin_dev.sh` 后，打开
**Plugins → 插件测试与发布… → 测试**，选择插件并点击 **Quick Test**。Developer
App 内复制的插件与 linked build、活动 runtime generation 内容一致时属于同一
构建，不需要再次手工激活路径。GUI 负责固定 snapshot、调用 Developer bundle
中的 `posdk_test_runner`，并展示通过数、失败数、耗时、具体原因和报告入口；
同一私有结果目录生成 JSON 与 JUnit。停止操作终止当前测试请求，不把状态写入
Consumer 设置。

新脚手架自动生成 `posdk.tests.json`。旧插件缺少计划时，Test Lab 显示
**Create & Quick Test / 创建并快速测试**，以加锁、原子且不覆盖已有文件的方式
创建只含 `runtime_contract` 的 starter plan，然后立即执行 Quick。已有但损坏的
计划不会被覆盖。starter plan 只证明 ID、ABI、架构、依赖和加载契约，不证明
算法正确；算法插件仍应增加确定性 workflow 或
`posdk_add_plugin_gtest()` 用例。

默认界面隐藏 profile、case、generation 和报告协议细节；展开“高级测试”后才
选择 Standard、Benchmark、Security 或单个 case。完整上手和故障排查见
[`plugin_test_lab.md`](plugin_test_lab.md)。

切换到 **发布 / Release** 页后，开发者可以选择
`closed_plugin_open_dependencies`、`closed_plugin_closed_dependencies` 或
`open_plugin_open_dependencies`。GUI 显示源码/当前平台构建、当前插件最近一次
可信测试、模式化发布预检和签名包四道门禁。只有当前插件测试通过且所选模式的
预检通过，才会启用正式打包。正式流程统一执行
`prepare-release → publisher-init → audit-release → release`，并把每次预检或
打包的 owner-only 日志、完整诊断和最终 `.pospkg` 路径保留在同一窗口。

**AI 协助打包**会把当前插件源码根、所选发行模式和诊断日志交给 PoMVG 内部
Codex Agent。Agent 的修改与执行权限限定在该插件源码范围，可以修复测试、依赖、
`install.sh`、链接闭包和发行契约，但不会自动上传 GitHub Release。AI 修复不能
绕过任何测试、审计或签名门禁。

`quick` 是日常构建后的默认入口。它应保持快速、固定 seed、无网络且可重复；
算法实现至少应提供一个有明确输出断言的确定性 workflow 或 GTest smoke case。
所有 workflow、配置和 fixture 必须在根级 `resources` 中声明
`path/size_bytes/sha256`。控制器会固定资源并让子进程只看到私有副本；配置中的
路径使用 `posdk-resource://...`，宿主机绝对路径会被拒绝。

Developer GUI 的 Build 会先增量 configure 并启用
`POSDK_BUILD_PLUGIN_TESTS`。使用 `posdk_add_plugin_gtest()` 声明的测试会自动
汇总到稳定 target `posdk_plugin_tests_<sanitized_plugin_id>`；发现该 target
时，Build 同时编译插件及其全部 GTest，任何一个测试 target 编译失败都不会发布
新的插件 generation。没有声明 GTest 的旧插件仍按原路径只构建插件。SDK 对
`plugins/methods` 和 `plugins/data` 使用受 CMake 跟踪的目录发现，新建插件后
无需开发者手工删除 build 目录或重新运行 configure。

### CLI 与 CI

CLI 是 GUI 的薄封装，二者执行同一控制器和同一计划：

```bash
tools/posdk plugin test \
  --plan /absolute/path/to/posdk.tests.json \
  --profile quick \
  --plugin-binary /absolute/path/to/posdk_plugin_my_estimator.dylib \
  --build-dir /absolute/path/to/build \
  --results-dir /absolute/path/to/private-results
```

`--plugin-binary` 是默认的单插件入口：Developer-only
`posdk_test_snapshot` 使用 GUI 同源的严格 manifest/ABI/架构校验和
`PluginRuntimeStaging`，将二进制、同 stem sidecar、可重复的
`--plugin-resource-root` 以及可选 `--plugin-config-template` 固定到不可变
generation；该 helper 在 `posdk_test_runner` 结束前一直存活，因此 lease 不会
在长时间 benchmark 中提前失效。CLI 会优先发现官方
`build/presets/macos-arm64-developer` 的 runner/helper。

此模式只覆盖目标插件的 `snapshot` 与 GTest case。若所选 profile/case 含
`workflow` driver，helper 会 fail-fast，因为工作流可能依赖其它 Method/Data
插件。应从 Developer GUI 导出当前完整活动图，并使用：

```bash
tools/posdk plugin test \
  --plan /absolute/path/to/posdk.tests.json \
  --profile standard \
  --plugin-snapshot /absolute/path/to/plugin-runtime-snapshot.json \
  --build-dir /absolute/path/to/build
```

`--plugin-snapshot` 是完整多插件工作流的高级/首选入口，且必须来自当前
Developer GUI 或可信测试夹具；CLI 不会根据源码目录猜测其它插件。它与
`--plugin-binary` 二选一。只有校验并列出选择项时两者都省略：

```bash
tools/posdk plugin test --plan posdk.tests.json --profile all --list
```

CLI/CI 可用同一稳定 target 构建某个插件的全部测试：

```bash
tools/posdk plugin build \
  --source /absolute/path/to/MyEstimator \
  --build-dir /absolute/path/to/build \
  --sdk-root /absolute/path/to/plugin_sdk \
  --tests --id my_estimator --config Release
```

`--tests` 会在 configure 阶段强制启用 `POSDK_BUILD_PLUGIN_TESTS`，然后构建
`posdk_plugin_tests_<sanitized_plugin_id>`；同时传入 `--target` 时会先后构建
显式 target 与测试聚合 target。根 `manifest.json.name` 已是规范插件 ID 时可
省略 `--id`，未指定 `--tests` 时保持普通构建语义。

`--case <id>` 可从所选 profile 中运行单个 case。`--runner`、
`--test-runner` 和 `--snapshot-helper` 仅用于非标准布局下显式覆盖可信
Developer helper；通常无需填写。默认会查找官方 preset、当前源码 build 输出
或 Developer app 的 `Contents/Helpers`。包含 `gtest` driver 时必须提供
`--build-dir`。每次运行默认在 owner-only 结果目录中生成 `report.json` 和
`junit.xml`，便于本地查看和 CI 归档。

### `posdk.tests.json` profile

`posdk plugin create` 会生成含 `snapshot` runtime-contract case 的严格
`posdk.tests.json`。它可立即被校验和选择，实际执行可用目标
`--plugin-binary` 自动生成 snapshot；该默认 case 只证明 ABI、平台、架构、
哈希、依赖和插件身份匹配，不证明算法正确。

- `quick`：每次构建的加载契约和单场景确定性 smoke；
- `standard`：多场景、多固定 seed、噪声和外点矩阵；
- `security`：损坏输入、边界值、超时/崩溃和输出限制，每个 robustness
  scenario 均绑定到精确 GTest identity；Host 结果只表示 Developer
  robustness，未知插件须在一次性 VM；
- `benchmark`：显式性能测试，不进入默认快速门禁；必须输出结构化 metrics
  并通过至少一个阈值。跨机器比较时，CI 应把机器/系统信息与 JSON/JUnit
  报告一起归档。

计划只能引用相对、非符号链接的 fixture/可执行文件；解析器拒绝未知字段、重复
key、路径逃逸和超限输入。完整格式见
`plugins/spec/plugin-tests-v1.md`。

### Consumer / Developer 边界

Consumer app 只允许运行已安装插件和普通工作流，不包含
`posdk_test_runner`、GTest driver、Plugin SDK 或测试源码。Developer app
才提供插件创建、构建、override、插件测试与发布工作台和 CLI/CI 测试工具。普通用户需要
开发插件时，应安装独立 Developer 发行物；不要通过开关让 Consumer app 暴露
测试和编译能力。

---

## 4. AI-Assisted Development

If you generate `.claude/CLAUDE.md`, the generated context should point assistants to the single PoMVG skill entry and the local development docs.

Recommended reference set:

- `docs/source/basic_development/plugin_guide.md`
- `docs/source/basic_development/method_plugins.md`
- `docs/source/basic_development/data_plugins.md`
- `docs/source/basic_development/gui_extensions.md`
- `docs/INI_Configuration_Specification.md`
- `docs/Log_Format_Developer_Guide.md`

Use the unified `posdk` skill entry rather than older split skill names.

---

## 5. GUI Extension Points

PoMVG currently supports four plugin-side GUI resource families:

- `log_styles/` for method log rendering
- `input_controls/input_control.qml` for method/data custom editors; Method
  controls use the v1 handshake and may receive a node-scoped
  `datasetContext`
- `eval_views/` for evaluation connection dialogs
- `evaluator_plots/` for metric visualization descriptors/renderers

See: [GUI Plugin Extensions](gui_extensions.md)

All four resource families are synchronized and installed by
`add_posdk_plugin(...)`. At runtime, the GUI resolves them only from the exact
active plugin's immutable generation.

---

## 6. Packaging and Publishing

PoMVG uses two runtime metadata concepts:

- `manifest.json`: package metadata for `.pospkg` and method static port metadata
- `posdk_plugin_<name>.posdk.json`: runtime metadata generated by the build

For method plugins, `manifest.json` must explicitly contain `input_types` and
`output_types`. These arrays are the only source for ordinary workflow ports in
the GUI static loading path. Empty ordinary inputs or outputs must be written as
`[]`, not omitted.

The copied same-stem `<plugin-binary>.manifest.json` is validated as strict
package-manifest schema v1 before Workflow consumes it. The file must be a
regular non-symlink file of at most 1 MiB; its root must be an object containing
`format_version: 1`, a matching `name`, and an exact `plugin_type` of `method`
or `data`. Unknown fields, wrong field types, identity/category mismatches,
duplicate port IDs, and non-canonical port IDs are rejected. Existing v1
package metadata fields such as `author`, `base_class`, `plugin_files`, and
`config_files` remain allowed and type-checked. `group`,
`supported_systems`, and `supported_arches` are also validated and carried by
the signed package port manifest.

An omitted `input_types` or `output_types` means the port contract is
incomplete. An explicit `[]` means the plugin has been confirmed to have no
ordinary ports in that direction. Workflow publishes no partial ordinary-port
set from an incomplete or invalid manifest.

The strict runtime `.posdk.json` mirrors `input_types` / `output_types` when the
source manifest provides them, but it does not silently replace a missing
same-stem port manifest. Keeping the two roles separate preserves the missing
versus explicit-empty diagnostic contract.

When a source port manifest declares the strict
`capabilities.two_view_estimator` v1 object, `add_posdk_plugin()` validates its
complete key set, enum values, and required method ports with CMake's native
JSON parser, then writes the same values to `.posdk.json` in deterministic key
order. Static binary inspection compares both same-stem sidecars without
loading plugin code. A capability present on only one side, an unknown value,
or any field mismatch fails closed. Plugins that do not declare this optional
capability remain backward-compatible. Strict source-manifest generation
requires CMake 3.19 or newer.

If one source directory builds multiple plugins, use plugin-specific manifests.
The lookup order is:

1. `manifest.<plugin_name>.json`
2. `<plugin_name>.manifest.json`
3. `manifest.json`

INI metadata stays separate: `# @Prior:` declares types accepted by the bottom
Prior port, and `# @output:` only marks conditional output state. Neither tag
adds ordinary workflow ports.

### `.pospkg`

Exported packages typically contain:

```text
my_feature_matcher.pospkg
├── manifest.json
├── signature.json
├── plugins/methods/
│   ├── posdk_plugin_my_feature_matcher.dylib
│   ├── posdk_plugin_my_feature_matcher.posdk.json
│   └── posdk_plugin_my_feature_matcher.dependencies.json  # optional
└── configs/methods/
    └── my_feature_matcher.ini
```

`.pospkg` v3 is the default strict distribution format. Its manifest binds plugin
identity, SemVer, platform/architecture and Factory/plugin ABI epochs, direct
plugin dependencies, every QML/config/runtime resource, each payload size and
SHA-256 digest, the signed package `install.sh` contract, and the Ed25519 signature policy. Consumer validation requires
a trusted signature; Developer validation or installation accepts an unsigned
package only with an explicit per-command `--allow-unsigned` switch.
Existing `.pospkg` v2 archives remain supported. New Marketplace releases use
v3 so a developer can publish only target-specific dynamic libraries: source
code is optional, while `deps/`, QML, configs, and other resources remain
optional according to plugin complexity.

Use the same command surface in local development and CI:

```bash
tools/posdk plugin create --id my_feature_matcher --type method \
  --group SfM/Matching \
  --distribution-model closed_plugin_open_dependencies \
  --destination ./MyFeatureMatcher
tools/posdk plugin build --source ./MyFeatureMatcher --build-dir ./build \
  --sdk-root /path/to/plugin_sdk --target my_feature_matcher
tools/posdk plugin package --build-root ./build --id my_feature_matcher \
  --type method --version 1.0.0 --private-key publisher.pem \
  --installer-script ./MyFeatureMatcher/install.sh \
  --installer-config ./MyFeatureMatcher/posdk-installer.json \
  --distribution-model closed_plugin_open_dependencies \
  --output my_feature_matcher.pospkg
tools/posdk plugin validate my_feature_matcher.pospkg \
  --public-key publisher.pub.pem
tools/posdk plugin install my_feature_matcher.pospkg \
  --public-key publisher.pub.pem
```

For an independent Marketplace repository, the scaffold also writes
`posdk-marketplace.json`, `deps/README.md`, `install.sh`,
`posdk-installer.json`, and `publish.sh`. The install script is embedded in and
signed with each platform package; PoMVG GUI never executes the repository
copy directly.
The common release path is:

```bash
export POSDK_PLUGIN_SDK_ROOT=/path/to/PoSDK\ Plugin\ SDK
cd /path/to/my-plugin
./publish.sh
```

If the SDK's `posdk` executable is already on `PATH`, the export is optional.

First create the signed `.pospkg` in **Plugin Test & Release → Release**. The
script explains the flow, asks for `owner/repository`, and uses the local
GitHub CLI's official browser/device authorization. It detects whether the
repository already exists, displays an exact create/append/update plan, and
waits for explicit confirmation before changing GitHub. It then commits only
`plugins/<plugin-id>/`, verifies and uploads the signed package plus SHA-256 to
the plugin-scoped Release, and prints a one-time posdk.net authorization URL.
Sign in to posdk.net in any browser and verify the displayed 12-character code
and plugin ID. The scoped token is consumed after that one Marketplace publish.
This flow works in a local terminal or over SSH; PoMVG never receives, reads,
or stores a GitHub or posdk.net password.

To prefill the target or package while retaining the same confirmations:

```bash
./publish.sh --repository owner/PoSDK-plugins \
  --package ./dist/my_plugin-1.2.3-macos-arm64.pospkg
```

One publisher repository may contain many plugins. Every plugin owns exactly
`plugins/<plugin-id>/`, and its GitHub Release tag is
`posdk-<plugin-id>-v<version>`. Stable identity uses the plugin ID, publisher
key, and repository path—not the display name. A same-version Release cannot
be replaced by different signed assets; increase the plugin version instead.

PoMVG GUI can alternatively connect the developer's posdk.net account from the Release
tab (or **Help → PoMVG Account…**). Authorization happens in the browser with PKCE and returns through a
one-time `posdk://account/connect` link. The access token is stored only in the
system keychain. After GitHub publication, click **Publish to posdk.net**; the
Portal rechecks repository identity, Group, supported targets, public key,
Release assets, and SHA-256 before creating or updating the Marketplace entry.
GitHub and PoMVG accounts remain independent and never share credentials.

Choose one source-publication model in **Plugin Test & Release → Release** or with
`--distribution-model`:

- `closed_plugin_closed_dependencies`: neither plugin nor dependency source is
  public. The package must still carry a portable binary closure and the
  publisher must provide every applicable redistribution, license, and source-
  offer notice.
- `closed_plugin_open_dependencies` (recommended for wrappers around OpenMVG,
  COLMAP, Python packages, and similar projects): the plugin binary stays
  closed while dependency source URLs, versions, hashes, licenses, and build
  recipes remain public and reviewable.
- `open_plugin_open_dependencies`: the complete plugin source and dependency
  recipes are committed to the public repository at the pinned Marketplace
  commit. Consumer installation still downloads the signed binary `.pospkg`
  instead of compiling repository source.

All three models use the same package shape and have identical signature,
installer, ABI, platform, dependency-closure, isolated-installation, and
runtime-test requirements. Source openness is a distribution choice, not a
verification shortcut.

#### Automatically bind installed tools to plugin configuration

Do not require users to locate an executable installed or compiled by
``install.sh``. Install private tools below ``POSDK_INSTALL_STATE_ROOT`` and
declare every configuration target in the signed
``installer.configuration_bindings`` array. The installer report returns only
the declared binding ID and a state-root-relative directory:

.. code-block:: json

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

.. code-block:: json

   {
     "bindings": [
       {"id": "graphoptim_bin", "relative_path": "bin"}
     ]
   }

PoMVG resolves and validates the reported directory, rejects symlinks and path
escape, and verifies the required executable. It then atomically maintains the
fixed entry point
``<state-root>/plugin-bin/<plugin-id>/<binding-id>`` as a relative symlink to
the versioned installer output and writes this stable absolute path only to the
signed INI target. An empty value, an older PoMVG installer-state path, or an
older stable binding is updated automatically; a non-empty external path
explicitly selected by the user is preserved. GUI installation also refreshes matching live workflow-node
configuration, while newly created nodes inherit the updated plugin template.
The installer must never edit arbitrary PoMVG configuration files itself.

The versioned physical output and stable public path therefore have distinct
purposes:

.. code-block:: text

   <state-root>/installer/<plugin>/<version>/<system>-<arch>/bin/tool
   <state-root>/plugin-bin/<plugin>/<binding> -> ../../installer/.../bin

Developers and users can query all paths with
``PoMVG plugin paths --id <plugin>``. The Installed Packages page also exposes
every stable binding with Open and Copy actions. Plugins must resolve the
stable binding first and may retain a versioned lookup only for packages
installed by older PoMVG releases.

The stable binding is local to one host; it is not a promise that one compiled
executable can be copied between Macs. Install the signed target-specific
``.pospkg`` again on the destination computer. A source-building installer
must pin its source and dependency contract, explicitly set and verify the
target architecture and deployment baseline, reject build-host paths, and run
the real smoke test on that host. Publish arm64 and x86_64 as separate signed
assets. Never migrate ``plugin-bin/``, ``installer/``, or an individual bin.

#### Isolate Python and model runtimes per plugin

Python-backed plugins must not activate or modify an ambient Conda environment
and must not share one mutable environment with another plugin. Create one
version- and target-specific prefix below `POSDK_INSTALL_STATE_ROOT`, prefer an
existing Conda/Mamba/Micromamba executable only as the environment creator, and
fall back to a publisher-pinned, SHA-256-verified Micromamba bootstrap when no
runner is installed. Use an explicit Conda package lock and pip
`--require-hashes`; pin model/source archives by immutable revision and
SHA-256. Report the private `env/bin` directory through a signed
`configuration_bindings` entry such as `python_bin`, so PoMVG writes the
verified `python_bin_folder` automatically. The plugin must execute only a
fixed executable name inside that directory, never an installer-provided shell
command. Installation is not complete until imports, architecture, exact
versions, the real model/backend smoke test, and the installed native plugin's
runtime contract all pass.

For a legacy plugin that is still built by the PoMVG monorepo, do not force its
subdirectory through a second, incomplete standalone CMake configure. Prepare
and audit the repository contract, then consume the exact completed platform
build:

```bash
tools/posdk plugin prepare-release --source plugins/methods/MyPlugin \
  --id my_plugin --type method
tools/posdk plugin publisher-init --source plugins/methods/MyPlugin
tools/posdk plugin audit-release --source plugins/methods/MyPlugin \
  --build-root build/presets/macos-arm64-developer/output \
  --id my_plugin --type method
tools/posdk plugin release --source plugins/methods/MyPlugin \
  --existing-build-root build/presets/macos-arm64-developer/output \
  --output-dir plugins/methods/MyPlugin/dist
```

`prepare-release` creates only missing files. `audit-release` is a hard gate:
it reports repository-contract omissions, runtime sidecar failures,
and non-portable loader dependencies. Dependency class is not a blacklist:
Python/model environments, external executables, remote services, solvers, and
private native libraries are all publishable when the signed target package,
installer, declared package dependencies, and final runtime closure implement
their actual contract. An optional `deps/posdk-release-lock.json` strictly pins
bundled installer/build inputs when a repository uses them. The
**Plugin Test & Release** Release tab exposes the same sequence through
**Audit & package .pospkg** and writes the signed result under the plugin's
`dist/` directory. The old direct package action is intentionally absent from
the plugin-creation dialog so testing, model-aware audit, signing, and AI
diagnostics cannot diverge into separate release paths.

Packaging is deterministic: member order, timestamps, JSON encoding, and Unix
modes are stable. Installation pins and validates one archive inode, stages
under a private directory, and publishes through the same locked schema-v2
registry and recovery journal as the GUI. Registry entries retain normalized
dependency contracts, so an install or upgrade is rejected if the prospective
registry would break a required dependency. The packager also follows the
same-stem `.runtime-files.json` and `.dependencies.json` contracts and includes
every declared non-native runtime file and private library. A missing,
symlinked, hard-linked, escaping, or undeclared generation file fails packaging;
the dependency contract must enumerate the complete private native closure.
See
`plugins/spec/pospkg-v3.md` in the SDK source tree for the full
trust, path, resource, and archive-limit contract.

### Marketplace Notes

When building for distribution:

- build one package per target platform as needed
- publish v3 `install.sh` packages for macOS or Linux only; Windows remains
  fail-closed until the signed PowerShell installer contract is available
- sign Consumer packages with the protected publisher key; commit only
  `publisher-key.pem`
- keep plugin-private runtime libraries in the package's declared runtime
  closure; list optional installer bootstrap files in `posdk-installer.json`
- make `install.sh` install and report the target system's Homebrew, apt/dnf,
  user-local, or source-built dependencies; declare administrator privilege
  before signing when the operating system package manager requires it
- publish `<id>-<version>-<system>-<arch>.pospkg` plus `.sha256` for every
  declared target before submitting the repository to posdk.net
- keep `manifest.json` metadata accurate
- keep `manifest.json` and `posdk-marketplace.json` identity, version, Group,
  and support arrays identical
- keep method `input_types` / `output_types` accurate for GUI static port loading
- declare platform restrictions in CMake if the plugin depends on CUDA, DirectX, Metal-only code, or platform-specific SDKs

---

## 7. Installation and Diagnostics

After installation, use these two surfaces together:

- **Modules**: statically validated, platform-compatible candidates appear here;
  actual dynamic loading is verified when the module is created
- **Plugin Paths**: shows compatible, unsupported, invalid, mismatch, or conflict states

If a plugin does not appear in **Modules**, check:

1. whether the library exports `PluginRegistration` and `PluginGetType`
2. whether the runtime `.posdk.json` exists and matches the host platform
3. whether the plugin type and package metadata are consistent
4. whether its dependency libraries can be resolved on the current machine

### Path Settings and Stable-ID Actions

Custom roots are stored as strict schema v2 in
`~/.posdk/plugin_paths.json`. Each record contains its stable `id`, absolute
`path`, `kind`, `enabled` state, and `priority`. The settings file and its lock
must remain regular non-symlink files under the trusted user root.

Reads and writes use an inter-process lock. A write additionally compares the
last-read SHA-256 revision before committing with `QSaveFile`; if another
process changed the settings, the operation fails with a conflict and the user
must refresh and retry. Malformed, unsafe, oversized, or schema-invalid files
are quarantined rather than partially accepted. Legacy bare arrays are read
only through the narrow migration path and become schema v2 on the next
successful edit.

Plugin Paths mutations are addressed by stable entry ID, not by display path:

- enable/disable, priority movement, and removal apply only to persisted custom
  entries;
- revoking a Developer session override uses that override's stable session ID;
- built-in and other protected rows remain read-only;
- path-only methods are compatibility wrappers and reject ambiguous matches.

For editable plugin configuration, the Manager's
`writableConfigPath(..., materializeTemplate=true)` operation serializes the
first template copy with a per-destination inter-process lock, rechecks the
destination after acquiring the lock, and commits with `QSaveFile`. This lock
protects initial materialization; it is not a general multi-writer merge
protocol for later INI edits. App-bundle and plugin-generation templates remain
read-only, while edits target `~/.posdk/configs/{methods,data}`. The Manager
selects one exact `<normalized-plugin-id>.ini` from the winning plugin's
authorized source/package or packaged distribution tier, captures it inside
the same content-addressed generation, and includes it in the generation
digest. Custom and user-installed winners do not borrow a same-ID template
from the Developer runtime's central config directory.

---

## 8. Recommended Reading Order

1. [Plugin Development Quick Example](method_example.md)
2. [Method Plugin Interface Details](method_plugins.md)
3. [Data Plugin Interface Details](data_plugins.md)
4. [GUI Plugin Extensions](gui_extensions.md)
5. [Appendix A: PoMVG Core Data Types](../appendices/appendix_a_types.md)
