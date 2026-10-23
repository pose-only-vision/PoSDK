# Quick Start

Not installed yet? Follow the [v2.1.1 GUI Installation Guide](gui_installation.md) to install the DMG, then return here.

Start with the **GUI**, the application you operate with a mouse, and run an existing workflow. Use the **CLI**, the command-line tool, when you want repeatable runs or server jobs. Both can use a saved `.posdk` workflow.

```{raw} html
<figure class="posdk-guide-video">
  <video controls playsinline preload="metadata" width="1320" height="720"
         poster="../_static/tutorials/quick-start-cropped.jpg"
         aria-label="PoSDK GUI quick-start walkthrough">
    <source src="../_static/tutorials/quick-start-cropped.mp4" type="video/mp4">
    Your browser does not support video playback. <a href="../_static/tutorials/quick-start-cropped.mp4">Download the video</a> to watch it.
  </video>
  <figcaption>PoSDK GUI quick-start walkthrough (2 min 19 sec) · <a href="../_static/tutorials/quick-start-cropped.mp4" download>Download video</a></figcaption>
</figure>
```

(gui-core-features)=
## Core GUI features

The GUI brings PoMVG's core capabilities into one workspace: run reconstruction, inspect accuracy and runtime, check algorithm attribution, and compare platforms.

| Core feature | What you can do | Where to find it |
| --- | --- | --- |
| **Pose-only Geometry Theory** | Estimate camera poses and generate a point cloud with a corresponding workflow, then inspect the reconstruction | **Workflow → Run → Visualization** |
| **Modular Plugin System** | Drag and connect loading, algorithm and output steps; replace modules or change settings | **Modules**, workflow canvas and the configuration panel |
| **Performance Profiling Tools** | Inspect recorded module timings, CPU and memory statistics, identify slow steps and export CSV data | Toolbar **Profiler** |
| **Automatic Evaluation** | Inspect recorded rotation, translation and other accuracy metrics, means, medians and charts; export data | Toolbar **Evaluator** |
| **Copyright Tracking** | Inspect authors, copyright declarations and dependencies; collect them in a report | Node **Information** tab; **Export → Copyright Information** |
| **GUI Installation** | v2.1.1 provides a macOS GUI; download the DMG and install it | [v2.1.1 GUI Installation Guide](gui_installation.md) |
| **Bilingual Logging** | Follow Chinese and English logs in Console and choose the interface language | **Console**; **Preferences → Language** |
| **Platform Integration and Comparison** | Process the same data with installed PoMVG, OpenMVG, COLMAP or GLOMAP modules and compare metrics and point clouds | Workflow library, **Evaluator**, **Visualization**; [benchmark results](../benchmark_comparison/index.rst) |

**Supporting tools:** 3D visualization provides split views and video export. Agent helps inspect or edit workflows through text, while IDE supports plugin development. Use the CLI described later for repeated runs or server jobs.

**You do not need to develop a plugin to run an existing workflow.** Steps also give the English button names.

Sign in through **Help → PoMVG Account** before running a workflow, including during the trial. GUI and CLI share the login saved in your system credential store. They restore it automatically and check account permissions online before each run or breakpoint resume; the distribution's trial/license checks also apply. A connection failure blocks that run but keeps the saved login for retry. Active use extends the login automatically; 30 days of inactivity or a revoked session requires signing in again.

An **Active** account also needs a valid trial or license for the current device. Workflows launched by the GUI check device permissions through the running native GUI process; standalone CLI runs restore and verify them locally. If the device identity or credential store is unavailable, unlock the login keychain, allow the installed PoMVG app to access it, and reopen the app.

## 1. Open a template from the workflow library

A workflow connects steps such as reading data, computing results and exporting files. Each box on the canvas is a **node**; connections carry data between nodes.

1. Open PoMVG GUI.
2. Click **Workflow** above the canvas, or choose **File → New from Workflow Library**.
3. Search by task or dataset name, select a suitable template, then click **Create and open workflow**.
4. Save your editable project, for example as `MyWorkflow.posdk`.

The template already connects the processing steps. Configure the input data, then check the parameters and output location. The following sections use **ETH3D reconstruction** as an example of data association, settings and execution.

```{figure} ../_static/user_guide/workflow-overview-imagegen-en-v2.png
:alt: PoMVG workflow: 01 open a workflow, 02 prepare data, 03 configure inputs, 04 check settings, 05 save and run, 06 review results.
:figclass: posdk-guide-workflow-image
:align: center
:width: 100%
```

## 2. Prepare and associate input data: ETH3D example

1. **Double-click ETH3D Data Reader** on the canvas. If its panel shows logs, switch to **Input Control** (输入控制).
2. If you need data, click **Download data** (下载数据). This opens the official ETH3D download page. Download the required scene in your browser and extract it on your computer.
3. Back in PoMVG, click **Choose local root** (选择本地根目录) and select the folder containing your scene folders.
4. Click **Associate with node** (关联到当前节点). Wait for the associated status. **Choosing a folder or typing a path alone does not complete association.**

A convenient folder layout is:

```text
ETH3D/                         ← choose this root
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

**Do not choose `images` or `dslr_images_undistorted` as the root.** If images and calibration were downloaded separately, extract them into their matching scene folders.

## 3. Confirm the scenes to run

**PoMVG — ETH3D** and **PoMVG — Strecha** already include a scene loop that visits the scene folders under the dataset root.

1. **Associate the dataset root** in the Loader.
2. Open **Variable Workspace** and check the previewed scenes and run count.
3. **Save and run**. Each scene runs once and its results are saved separately.

Runs are sequential, with loop acceleration disabled. To run one scene, remove the Enum Subdirs rule in Variable Workspace, then select that scene in the Loader.

## 4. Save, run and view the result

1. Save the workflow, close the module panel and click **Run** on the toolbar.
2. Watch **Console** at the bottom. If a run fails, read the last specific error, such as missing images, an unavailable folder or a plugin that could not load.
3. Inspect the output files from this run. To view 3D results, click **Visualization** and load the point cloud or camera files.
4. When viewing a point cloud, try **Reset View** if the result is not visible.

After clicking Run, **Workflow pre-run checks** shows the current step and plugin-check progress. It closes automatically when execution is ready. Click **Cancel run** to stop preparation.

Toggle **Preferences → General → Workflow → Pre-run checks** to disable the checks; they are enabled by default. Disabling skips extra pre-run checks. Modules are still validated and errors reported during execution.

| Viewing action | How |
| --- | --- |
| Rotate | Drag with the left mouse button in normal viewing mode |
| Pan | Drag with the middle mouse button |
| Zoom | Scroll the mouse wheel |
| Compare results | Load multiple files and use split views with linked cameras |
| Export a presentation video | Set a path in the camera-trajectory panel, then use video export |

### Inspect accuracy, performance and copyright reports

- **Accuracy:** enable evaluation in the node settings and connect the required reference data to the evaluation port. After running, open **Evaluator**, choose an algorithm and metric, and inspect statistics, charts or exported CSV data. Without reference data, you can still inspect the point cloud, but cannot obtain ground-truth pose errors from it.
- **Performance:** enable profiling for the nodes you want to measure. After running, open **Profiler** to inspect module timings, CPU and peak memory statistics before deciding which step to optimize.
- **Attribution:** click a node and open its **Information** tab to inspect authors and copyright declarations. Choose **Export → Copyright Information** for the combined report. The **Export** menu also provides **Evaluation Report** and **Performance Report** for existing records.

Inspect the outputs, then review evaluation metrics and module timings as needed. Keep the dataset and reference information consistent when comparing settings or platforms.

## 5. Create or adjust a workflow yourself

Use an existing template when it meets your needs. To learn how to build a workflow, open **Help → Tutorials → Create a workflow**.

The basic steps are:

1. Choose **File → New Project** for an empty canvas.
2. Find the required input module in **Modules**, drag it onto the canvas and double-click it to configure the inputs. For example, ETH3D reconstruction uses **method → SfM → DataLoader → ETH3D Data Reader** to associate a data root and select a scene.
3. Click a node's **output port**, then click a **compatible input port** on the next node. Alternatively, click the output and then empty canvas space to choose a compatible module; the GUI creates it and connects that port. Connect the other required inputs as well.
4. **Click a node** to view its configuration on the right. **Double-click it** to open its input-control/log panel.
5. Connect the processing steps for your task and supply each module's required inputs. For example, the ETH3D reconstruction template contains **data loading → features and matching → two-view estimation → rotation averaging / track building → PoseEngine → Fresh → PA → HAR reconstruction**. Follow its nodes and connections to supply the camera, feature and pose inputs required by each step.
6. Connect the results you want to save to **others → Output Control**. Open **Output Control** on the toolbar and check the output folder, file names and formats. Choose `.ply` for point clouds, and an available pose or trajectory format as needed.
7. **Save, then run**. Confirm completion in Console and inspect the exported files. Open 3D results in Visualization. Use **Auto layout** above the canvas to arrange nodes from left to right according to their dependencies.

In **Modules → Data**, right-click an item and choose **Rename**. Enter its Chinese and English names, then click **Save Alias**. Names follow the interface language and update data ports on existing and new nodes. Click **Restore Default** to remove the mapping.

A `.posdk` file saves the workflow. When moving a project, keep its companion configuration folder beside it so the settings remain available.

## 6. Develop and IDE: add your own plugin

Skip this section if you only want to run existing algorithms.

1. Click **Develop** and link your own writable Plugin SDK folder.
2. Choose **Method** for a processing step or **Data** for a data type.
3. Fill in the name and inputs/outputs. Under **Copyright & License → Edit Copyright / Choose License…**, enter the author and holder, choose a license, then preview and confirm the declaration.
4. Click **Generate** and open the plugin source in IDE.
5. Edit source in **IDE → Integrated**. Click **© Copyright** at the upper right to edit copyright information. Save, click **Rebuild**, and check the build output.
6. After a successful build, refresh the plugin and test it in **Plugin Test & Publish**. Return to Modules and place it in your workflow.

For a guided first plugin, use **Help → Tutorials → Create a development plugin**. See the [plugin guide](../basic_development/plugin_guide.md) for details.

For editing steps, see [Copyright Information](../copyright_tracking.rst).

## 7. Agent: ask for help in plain language

Choose the built-in Agent or combine Codex with PoSDK CLI to suit your task:

| How to work | What it helps with |
| --- | --- |
| **Built-in GUI Agent** | Click **Agent** on the toolbar to work with the current canvas: operate modules and connections, understand data flow, check inputs, analyze settings and explain runtime errors |
| **Codex + PoSDK CLI** | Build, adjust and run workflows locally or on a remote machine through SSH; change module settings, repeat runs, and analyze logs and results |

Use both together: check the workflow and settings in GUI, ask Codex to repeat runs through CLI locally or on a server, then inspect the results in GUI.

The accompanying **PoMVG Skill** provides workflow operations, module interfaces and development conventions. It helps with adjusting settings, developing Method/Data plugins and diagnosing problems.

```{figure} ../_static/user_guide/codex-posdk-cli-en-v1.png
:alt: Describe a task; Codex uses the PoMVG Skill and PoSDK CLI to build, configure and run local or SSH remote workflows. The built-in GUI Agent helps operate modules, understand workflows and analyze settings.
:figclass: posdk-concept-diagram
:align: center
:width: 100%

Codex calls PoSDK CLI to carry out the task, using SSH for remote execution. The built-in Agent provides interactive help with the current GUI canvas.
```

### Set up the built-in Agent

For first-time setup, follow **Help → Tutorials → Use the Agent**, or use either method below to set up the local Codex CLI.

**Option 1: Ask in one sentence (recommended)**

Open Codex, Cursor, or another agent already on your computer and send:

> Help me set up the local Codex CLI.

**Option 2: Copy the full prompt**

Copy the following into that agent's chat:

```text
Help me set up the local Codex CLI for the Agent in PoMVG GUI.
1. Check whether it is installed. If installation is needed, use the official method for my operating system: https://developers.openai.com/codex/cli.
2. Configure the command path. If sign-in is needed, guide me through codex login.
3. Check codex --version and codex login status, then run a read-only request: codex exec --json --sandbox read-only --skip-git-repo-check "Reply with exactly OK."
4. After confirming the request succeeds, show me how to return to PoMVG GUI → Agent, click Recheck, and install the PoMVG Skill through Skills.
```

After setup, return to **PoMVG GUI → Agent** and click **Recheck**. Once the check passes, open **Skills**, click **Install All**, then enter and send your task.

For installation and sign-in steps, see the [official Codex CLI guide](https://developers.openai.com/codex/cli).

For example:

> Check this workflow. What inputs or settings are still missing?
>
> Change the scene from meadow to office and leave the other settings unchanged.
>
> Explain the last Console error and show me where to fix it.
>
> Use the PoMVG Skill to help me create a Method plugin and explain how to build and test it.

After an edit, inspect the loader panel or canvas, then save and run. Agent is optional; the earlier steps also work without it.

## 8. CLI: run the saved workflow from Terminal

CLI is useful for repeated runs and server tasks, using workflows saved in GUI.

The DMG contains the **PoSDK Plugin SDK** folder. Copy the entire folder to your **Documents** folder so its path is `~/Documents/PoSDK Plugin SDK`. The `posdk` command is located at `bin/posdk` inside that folder. The commands below store its full path in `POSDK_CLI` for reuse. Start by viewing the help:

```bash
POSDK_CLI="$HOME/Documents/PoSDK Plugin SDK/bin/posdk"
"$POSDK_CLI" --help
"$POSDK_CLI" workflow run --help
```

### Sign in and remember the account

CLI reuses a GUI login under the same OS user on the same computer. To sign in from Terminal:

```bash
"$POSDK_CLI" account login --build-root "/path/to/PoSDK_GUI.app"
"$POSDK_CLI" account status --build-root "/path/to/PoSDK_GUI.app"
"$POSDK_CLI" account logout --build-root "/path/to/PoSDK_GUI.app"
```

Confirm the code displayed in Terminal on the posdk.net browser authorization page. On a remote terminal, add `--no-browser` and open the displayed link in your own browser; no callback port forwarding is needed. Logout clears the shared GUI/CLI login for this OS user.

### Recommended: let Codex call PoSDK CLI

Open your working directory in [Codex](https://developers.openai.com/codex/cli), provide the SDK, project and dataset paths, and ask it to follow the PoMVG Skill. Describe your goal so it can build or adjust a workflow, change module settings, run it through PoSDK CLI, and analyze logs and results. For example:

```text
Follow the PoMVG Skill and use PoSDK CLI to adjust and run my ETH3D workflow.
SDK: ~/Documents/PoSDK Plugin SDK
Project: /path/to/MyWorkflow.posdk
Dataset: /path/to/ETH3D
Set the scene to office, check module connections and settings, save and run,
then analyze the logs and report the results directory.
```

For remote work, also provide the **SSH account and host**, plus the **SDK, runtime, project and dataset paths on the remote machine**. Codex can use SSH to build or adjust workflows, change module settings and run them there. Outputs stay on that machine and can be copied back for inspection in GUI.

### Check a project saved in GUI

First change Terminal's working directory to the folder containing `MyWorkflow.posdk`:

```bash
"$POSDK_CLI" workflow inspect --project ./MyWorkflow.posdk
"$POSDK_CLI" workflow validate-project --project ./MyWorkflow.posdk
```

`inspect` reports node, connection and configuration-file counts and required plugins. `validate-project` checks the project. Replace `MyWorkflow.posdk` with your saved project file.

### Run a selected scene: ETH3D example

```bash
"$POSDK_CLI" workflow run \
  --project ./MyWorkflow.posdk \
  --workspace ./eth3d-runs \
  --build-root "/path/to/PoSDK_GUI.app" \
  --dataset-root "loader=/path/to/ETH3D" \
  --set loader:eth3d_dataset_loader.specific_data=meadow
```

Replace both `/path/to/...` values with real paths on this computer. `--build-root` selects the matching PoMVG application or build directory. **`loader` is the DatasetLoader node ID in this example.** Click that node in GUI and read its node ID in the configuration panel on the right. If it differs, replace both occurrences of `loader` in the command.

`--dataset-root` supplies the root folder and `--set` selects this run's scene. The command reports a run ID and its output directory.

### Read status, logs and output files

Replace `RUN_ID` with the ID returned by the run:

```bash
"$POSDK_CLI" run status --workspace ./eth3d-runs --run RUN_ID
"$POSDK_CLI" run logs --workspace ./eth3d-runs --run RUN_ID --follow
"$POSDK_CLI" run artifacts --workspace ./eth3d-runs --run RUN_ID
```

Open the output files in GUI Visualization when you want to inspect them. Add `--detach` to the run command if the job should continue after you leave the terminal. See [Workflow Library](../basic_development/workflow_library.md) for more about saved workflows.

## Quick fixes

| What happened | Check this first |
| --- | --- |
| A folder is selected, but the loader is not associated | Click **Associate with node** and wait for completion |
| Images or calibration cannot be found | Choose the parent of the scene folders; extract images and calibration into the matching scene |
| A scene change did not take effect | Match the folder name, press Tab or leave the field, then save |
| You do not know what to click next | Open **Help → Tutorials** and follow the highlights |
| A plugin will not compile | Check that the Plugin SDK folder is linked and read the specific error in Build Output |
| The run finished, but the cloud is not visible | Select the output PLY in Visualization and click **Reset View** |
