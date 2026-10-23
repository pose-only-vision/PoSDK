# GUI Plugin Extensions

PoMVG supports several plugin-provided QML extension points in the GUI. They are discovered at runtime and allow plugins to customize log rendering, data input controls, and evaluation views without changing the core GUI.

---

## 1. Extension Families

### `log_styles`

Method plugins can provide custom log viewers:

```text
plugins/methods/<PluginDir>/
└── log_styles/
    ├── log_view/
    │   └── <style_name>.qml
    └── match_view/
        └── <style_name>.qml
```

Used by: `ModuleLogDialog.qml`

### `input_controls`

Method and data plugins can provide custom editors for module parameters.
When a Method plugin provides one, the node details dialog exposes a compact
InputControl/LogViewer switch; ordinary Method plugins without one remain
LogViewer-only:

```text
plugins/{methods,data}/<PluginDir>/
└── input_controls/
    └── input_control.qml
```

Used by: `ModuleLogDialog.qml`

Every placed Data node also has a host-owned **Data Load / Save** page,
independent of whether it provides a custom InputControl. The page validates
loads before changing that node's `config[pluginName].associatedFilePath` and
exports copies through a fresh DataIO instance. Output formats come from the
active DataIO's `GetSupportedOutputTypes()`; asymmetric import formats are
handled separately (e.g. global poses import `.rot` but only export `.csv`).
Types without file serialization, and export-only diagnostic reports, are
identified rather than presented as generic Protobuf loaders.

InputControls are kept warm only within the same node context. Switching node
instances or resetting the project retires the previous editor and file
dialogs before injecting the new context. A repeated plugin name is never a
node identity. For `data_images`, selecting a directory associates it read-only;
it does not create or overwrite `data_images.imglist` in the source directory.
Preview, export, and workflow execution use the same ordered directory scan.
The existing `.imglist` association format remains supported.

### `eval_views`

Plugins can provide custom evaluation views for connection evaluation:

```text
plugins/data/<PluginDir>/
└── eval_views/
    └── data_<type>_eval.qml
```

Method plugins can also host `eval_views/` if they provide the evaluation UI for a target data type.

Used by: `EvalConnectionDialog.qml`

---

## 2. Runtime Discovery Rules

### Build and installation

All three directories are handled by `add_posdk_plugin(...)` in
`plugin-config.cmake` for method and data plugins.

- copied into the runtime plugin output directory at configure time and
  synchronized by an independent always-run target, so QML-only edits and
  deletions take effect without relinking the native plugin
- installed below
  `plugins/{methods,data}/<normalized-plugin-id>/<extension-family>/`
- resolved through the process-wide `PluginPathManager`
- copied into the same read-only, content-addressed runtime generation as the
  exact active native plugin and loaded only from that immutable copy

Here, runtime generation means the **active Factory generation** published by
the Manager. A Developer diagnostic may also report a **temporary helper probe
generation**; that helper-private path is short-lived telemetry and is never a
QML/config lookup root or a workflow runtime path.

`data_camera_models` and `data_global_poses` are po_core built-in data types,
not dynamic plugin targets. Their evaluation views are synchronized by the GUI
build into the relocatable built-in resource package at
`resources/plugin_resources/data/<id>/eval_views` (and into
`PoSDK_GUI.app/Contents/Resources/plugin_resources` on macOS). The
`PluginPathManager` resolves these relocatable resources directly. All
statically linked po_core method/data IDs are reserved: a dynamic candidate
with the same normalized ID is diagnosed as `blocked_by_core_builtin` and is
never published to the Factory registry.

### `eval_views`

`eval_views` are resolved through the same process-wide `PluginPathManager`.

- the GUI searches only the exact active plugin generation for
  `eval_views/<view_name>.qml`; a shadowed copy cannot contribute QML
- data and method IDs are resolved independently
- method plugins can provide evaluation views as well as data plugins
- the built-in fallback is `components/eval_views/default_eval.qml`

All relative resource paths are traversal-checked and symlink escapes are
rejected. The exact `<normalized-plugin-id>.ini` template selected for the
winning plugin is also content-bound and copied under `resources/configs/` in
that plugin's immutable generation; configuration resolution never reopens the
mutable source/package template after activation. Editable configuration never
writes into an app bundle or plugin package. The Manager can materialize the
read-only generation copy under
`~/.posdk/configs/{methods,data}` with a per-destination inter-process lock,
post-lock existence check, and atomic `QSaveFile` commit. That lock covers the
initial materialization race; it is not a general merge lock for later edits.

---

## 3. QML Contracts

### Log viewer contract

```qml
property var logMetadata: ({})
property var logData: ({})
property var preferenceModel: null
property bool _zh: false
property string rawLogContent: ""
```

Optional convenience properties:

```qml
property string moduleName: ""
property string moduleCategory: "method"
```

### Input control contract

```qml
property var configModel: null
property var datasetContext: null
property var uiTheme: ({})
property var preferenceModel: null
property bool _zh: false
property string moduleName: ""
property string moduleCategory: "method"
property string nodeId: ""

function hostContextReady() {
    // Perform the first config/status sync here. The host invokes this only
    // after every property above has been injected.
}
```

Method InputControls must also attest the v1 contract:

```qml
readonly property int extensionApiVersion: 1
readonly property string extensionKind: "input_control"
```

`configModel` is the editable item model for module settings. Typical fields include:

- `key`
- `value`
- `displayName`
- `details`
- `options`
- `readOnly`
- `hidden`

Use `configModel.setValue(row, value)` for edits.

`datasetContext` is an optional, node-scoped DatasetLoader facade. It exposes
only `status(datasetId)`, asynchronous `refresh()`, `associate(datasetId)`,
`registerLocalAndAssociate(datasetId, folderUrl)`, and the bounded asynchronous
`scanDirectory(datasetId)`. Observe `busy`, `operationFinished`,
`refreshFinished`, and `directoryScanFinished`; a `true` return from an
asynchronous request means accepted, not completed. Root association is checked
against the exact node's `@dataset_binding` declaration and updates only that
node-owned instance INI. It does not expose Marketplace credentials, package
installation, arbitrary node selection, or raw workflow mutation.

While `busy` is true, `operation` is one of `refresh`, `associate`, `rollback`,
or `scan`, allowing the InputControl to show an accurate non-blocking status.

`directoryEntries` contains at most the host safety limit and reports directory
name, root-relative path, depth, child-directory count, and file count. The
scan ignores symlinks, stays inside the canonical linked root, and never gives
extension QML a general filesystem API.

PoMVG no longer exposes the old **File → Dataset Manager** window. Dataset
layout and acquisition guidance belong to each DatasetLoader's
`input_control.qml`, because scene/sequence and ground-truth conventions are
plugin-specific. The process-level DatasetManager remains an internal host
service for local references, signed `.posdata` installation, Marketplace
transactions, and portable workflow materialization; plugin QML receives only
the node-scoped facade above.

The bundled DatasetLoaders share the same root-association workflow while
preserving their real on-disk contracts:

| Loader | Associated root | Per-run selector | Important specialization |
| --- | --- | --- | --- |
| Strecha | common scene root | `specific_data` scene | `images` and camera-GT folders |
| ETH3D | common scene root | `specific_data` scene | split/device and COLMAP GT layout |
| KITTI Odometry | `sequences/` | two-digit sequence | image/P camera pair is updated together; poses normally live in sibling `poses/` |
| ISPRS | common sub-dataset root | Toronto/Vaihingen-style sub-dataset | `cam.txt` may override camera and GT-file fallbacks |
| PMBA | common scene root | PMBA scene | Cam/cal/Feature files are scene-local; virtual image paths are expected |

Do not replace these controls with a global layout switch. The reusable
contract is status, safe association, bounded tree inspection, and exact-node
configuration; dataset layout remains owned and validated by the Loader.

`uiTheme` contains read-only color tokens (`panel`, `surface`, `card`,
`control`, `hover`, `border`, `text`, `subtext`, `accent`, `accentText`,
`success`, `warning`, and `danger`) so disk-loaded QML can match PoMVG without
importing the private `PoSDK_UI` module. Use `accentText` for labels and
selection text rendered on an `accent` background; it follows the active
PoMVG theme's contrast choice.

### Visual design contract

All `input_controls`, `log_styles`, and `eval_views` are part of the PoMVG GUI
product surface. Match the nearest host dialog, keep a flat and modern visual
hierarchy, and do not introduce a plugin-specific chrome.

- Use `uiTheme` tokens only; do not hard-code a dark-only palette or use
  translucent white/black chrome that fails in other themes.
- Prefer 7-10 px rounded cards/controls, 34 px standard controls, 28 px
  compact actions, 12-16 px card padding, and one clear primary action.
  Avoid gradients, bevels, excessive shadows, thick borders, oversized logos,
  and wide saturated selection fills.
- Implement hover, pressed, keyboard-focus, disabled, selected, and result
  states. Keep focus visible and use `accentText` over `accent` fills.
- Use compact headers/status pills and grouped cards for InputControls. Keep
  LogViewer layouts readable and dense; do not duplicate the host title bar,
  navigation, or outer frame.
- Use asynchronous status/progress for operations that can wait. Before
  delivery, run `qmlformat`, the applicable QML contract test, and inspect a
  dark plus a light PoMVG theme.

### Eval view contract

```qml
property var evalHelper: null
```

`evalHelper` is injected by `EvalConnectionDialog` and exposes:

- setup / path resolution for result and GT files
- evaluation status
- evaluation result rows
- helper methods such as `runDefaultEvaluate()` and `getDataSummary()`

---

## 4. Naming and Loading Tips

- QML files are loaded dynamically from disk, so do not `import PoSDK_UI`.
- Use `input_controls/input_control.qml` as the v1 conventional entrypoint.
- Consumer builds accept only the fixed `input_control.qml` Method entrypoint;
  the Developer build may diagnose an older first-file layout for migration.
- InputControl QML is loaded asynchronously from the exact active immutable
  plugin generation. PoMVG never falls back to a shadowed plugin or mutable
  source directory.
- Dynamic Loader construction completes before ordinary property injection.
  Put the first `configModel`/`datasetContext` refresh in
  `hostContextReady()` and also react to later property/signal changes; do not
  assume `Component.onCompleted` already has host context.
- Plugin QML may open an official dataset page in the browser, but must not
  download/extract archives or create `${dataset:...}` itself. Automatic data
  installation remains restricted to signed Marketplace `.posdata` entries.
- Keep file names stable because metadata such as `@style` and view names map directly to QML basenames.
- For method logs, `@style` should match the QML file basename without `.qml`.
- For eval views, the recommended basename is `<data_type>_eval.qml`, for example `data_global_poses_eval.qml`.

---

## 5. Related Docs

- `docs/Plugin_QML_Style_Guide.md`
- `docs/INI_Configuration_Specification.md`
- `docs/Log_Format_Developer_Guide.md`
- `plugins/cmake/PLUGIN_CMAKE_USAGE.md`
