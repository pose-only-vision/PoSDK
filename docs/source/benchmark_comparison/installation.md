---
tocdepth: 1

---

# Platform Installation

:::{container} platform-section-tabs
**Platform Installation**
[Evaluation Results](results.rst)
:::

This page describes the **Install, associate and test** panel opened by double-clicking a **COLMAP Pipeline, OpenMVG Pipeline or GLOMAP Pipeline** node in PoSDK GUI. Associate an existing compatible executable first; install from the panel if none is available.

:::{figure} ../_static/benchmark_comparison/platform-installation-options-en-v1.png
:alt: Two alternatives: download supported source and build it yourself, or use Download and Install in the panel. Both continue with Scan again, verified-program selection, association, and the sample test.
:width: 100%
:figclass: posdk-installation-options
:align: center

Choose either a manual build or panel-assisted installation. After installation, scan, verify and associate the programs with the current node.
:::

## Required programs

| Workflow node | Supported version | Programs to associate |
| --- | --- | --- |
| **COLMAP Pipeline** | COLMAP 3.13.0.dev0 | **COLMAP**: select `bin/colmap`. This node uses COLMAP's own image reader and does not require OpenMVG. |
| **OpenMVG Pipeline** | OpenMVG 2.1.0 with the required PoSDK extensions | **OpenMVG**: select `bin/openMVG_main_SfMInit_ImageListing`. Keep the companion OpenMVG executables in the same `bin` directory. |
| **GLOMAP Pipeline** | GLOMAP 1.1.0 with the required PoSDK extensions | **GLOMAP + COLMAP + OpenMVG**: select `bin/glomap`, `bin/colmap` and `bin/openMVG_main_SfMInit_ImageListing` respectively. COLMAP extracts and matches features; OpenMVG provides image-listing and other helpers. |

These are the versions supported by the current plugins. The panel checks startup, version and required capabilities before association. A program with the expected name must still pass these checks. Installation from the panel uses the corresponding pinned versions and required extensions.

## 1. Open the panel and associate local programs

1. **Double-click the platform node** in your workflow to open its **Install, associate and test** panel.
2. Under **Associate local programs**, click **Scan again** and wait for candidate checks to finish.
3. If an existing installation is missing, click **Choose search directory…** and select its installation directory or a Plugin SDK directory to scan. Alternatively, click **Choose manually…** and select the executable listed in the table above.
4. Select a candidate marked **Verified**, then click **Associate selected**. Association is complete when the program shows **Associated**.
5. For a GLOMAP node, complete association for all three programs: GLOMAP, COLMAP and OpenMVG.

Scanning and selecting a candidate do not change the configuration. Clicking the association button saves the program path to the **current node**. Check other nodes separately. After moving a project to another computer, associate programs installed on that computer.

## 2. Install from the panel when needed

Automatic installation currently supports **Apple Silicon macOS** and requires an Internet connection and **Xcode Command Line Tools**. If the command-line tools are missing, enter this command in macOS Terminal and complete the system installation dialog:

```bash
xcode-select --install
```

:::{note}
The panel's Download / Install attempts to download pinned source and build it locally. Network access, dependencies and the local toolchain can cause failures, so **success is not guaranteed**. If it fails, download and build the supported source revisions, including the required PoSDK extensions, yourself. Then follow step 1 to scan, verify and associate the executables in `bin`.
:::

Return to the platform node's panel:

1. Under **Download and install (optional)**, click **Download source and install**.
2. Wait for download, compilation, installation and executable checks. The progress display and **Operation log** show the current step.
3. Under **Associate local programs**, click **Scan again**, select a newly installed **Verified** program and click **Associate selected**. This step is still required after installation.
4. Run the included sample described below to check the installation.

The COLMAP node installs COLMAP only. The OpenMVG node installs OpenMVG only. The GLOMAP node installs GLOMAP and its required COLMAP and OpenMVG tools. Installation uses separate local directories without requiring manual replacement of system dependency libraries.

If all source files have been downloaded but compilation was interrupted, click **Install downloaded source**. If automatic downloading fails, expand **Manual download: links and git commands**, download the pinned revisions to the indicated directories, then click **Download source and install**. Use **Export install.sh and sample** to retain the installation materials or run installation manually.

## 3. Run the included sample

1. Confirm that every program required by this node shows **Associated**.
2. Under **Run the included sample**, click **Run sample**.
3. Wait for completion and check the recovered-view and 3D-point counts in the **Operation log**.
4. Click **Open test results** to inspect the output files.

The sample contains **nine synthetic images, about 1 MB**, for checking installation and association. Results go to a separate test directory. The sample does not replace the workflow's dataset and is not used for the formal accuracy comparisons.

## 4. Use your images and workflow

The installation panel manages executable installation, association and sample tests. Configure inputs in the workflow to process an actual dataset:

1. Associate a dataset and select a scene in its loader. Connect the **images and camera intrinsics** to the platform node. Keep the image set and intrinsics consistent across platforms. See [Quick Start](../installation/cli_gui_guide.md) for dataset association.
2. Give each platform a separate **working directory** (`work_dir`), distinct from the original images and dataset. Running with work-directory clearing enabled deletes the contents of this directory.
3. Choose the reconstruction mode. For full OpenMVG reconstruction, enable **SfM reconstruction** (`enable_sfm_reconstruction=true`) and select `INCREMENTAL`, `INCREMENTALV2`, `GLOBAL` or `STELLAR` through `sfm_engine`. When reconstruction is disabled, the node runs preprocessing only.
4. Save the project, run the workflow and check the node log for completion and output locations. Native models are stored in each platform's working directory. Connect an appropriate **Output Control** node for results you want the GUI to save.
5. Inspect the point cloud and the **Evaluator** and **Profiler** records. Ground-truth pose errors require the corresponding reference data; with images alone, inspect reconstruction structure and recovered-view counts.

See [Evaluation Results](results.rst) for the input conditions, accuracy, recovered views, runtime and reconstruction files of existing comparisons.

## Troubleshooting

| Message or symptom | What to do |
| --- | --- |
| No local program found | Use **Choose search directory…** for the installation directory, or **Choose manually…** for the actual executable. If none is available, install it in step 2. |
| Startup failure, missing library or incompatible dependency | Click **Show check details** to see the library name or error. Select a verified program, or install the supported version from the panel. An executable without its required libraries cannot be used. |
| Unsupported version or missing capabilities | Install the pinned version offered by the panel. GLOMAP and OpenMVG also require the corresponding PoSDK extensions; a matching version number alone does not ensure compatibility. |
| Sample asks to associate every required executable | Return to step 1 and confirm that every required entry in this node shows **Associated**, especially the three programs required by GLOMAP. |
| Programs are associated but the workflow produces no reconstruction | Check dataset inputs, the working directory and the node log. For OpenMVG, also check that **SfM reconstruction** is enabled. A passing sample checks the tools; the actual dataset still needs to be configured. |
