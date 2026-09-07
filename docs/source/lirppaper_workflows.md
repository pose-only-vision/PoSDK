# Running the LiRPpaper Workflows for Testing

This guide explains how to test the two LiRPpaper workflows distributed with
the PoSDK 2.0.0 macOS arm64 DMG. The workflows are built into the application;
a source checkout, CMake, Qt, or a local plugin build is not required.

The two workflow definitions are version `1.1.0`:

| Workflow | Dataset loader | Default scene | Protocol |
| --- | --- | --- | --- |
| **LiRPpaper-Strecha** | `strecha_dataset_loader` | `castle-P30` | GNC-RANSAC LiRP, PPOopt, Chatterjee rotation averaging, PoRobustSfMEngine |
| **LiRPpaper-ETH3D** | `eth3d_dataset_loader` | `facade` | Direct LiRP, six-step PPOopt, Chatterjee rotation averaging, PoRobustSfMEngine |

The distributed workflows are fixed single-run test protocols. They do not
enable a parameter sweep by default. The instantiated project remains editable
if you want to inspect or change a parameter.

## 1. Download and install the DMG

1. Open the PoSDK 2.0.0 GitHub release that contains the LiRPpaper assets:
   <https://github.com/pose-only-vision/PoSDK/releases>.
2. Download `PoSDK-GUI-2.0.0-macos150-arm64.dmg` and its adjacent
   `.sha256` file.
3. Verify the download before opening it. For example:

   ```bash
   shasum -a 256 -c PoSDK-GUI-2.0.0-macos150-arm64.dmg.sha256
   ```

4. Open the DMG and drag `PoSDK GUI.app` to `/Applications`.
5. Launch the application. If macOS shows a first-launch security prompt,
   use **Open** only after confirming that the DMG came from the intended
   PoSDK release and that its checksum matches.

The DMG contains the application and the built-in workflow definitions. The
dataset loader controls provide the supported download and association path.
Dataset licenses and any restrictions imposed by the original providers still
apply.

## 2. Download and associate a dataset

You can download and associate the data from inside PoSDK:

1. In **Behavior Workspace**, double-click the relevant dataset-loader module
   (`Strecha Dataset Loader` or `ETH3D Dataset Loader`).
2. In the module's input panel, click **Download & auto-associate** for Strecha
   or **Download data** to open the official ETH3D download page.
3. After downloading and extracting ETH3D, enter or choose the common dataset
   root and click **Associate with node**. For Strecha, the automatic download
   flow can verify, extract, and associate the pinned package; you can also
   choose a local root and associate it manually.
4. Confirm that the panel reports the dataset as associated with the current
   node before running the workflow.

The loader's `dataset_dir` is a common parent directory. The default
`specific_data` value selects one scene below that parent. The expected layouts
for the default scenes are:

### Strecha (`castle-P30`)

```text
<strecha-root>/
└── castle-P30/
    ├── images/
    └── gt_dense_cameras/
```

The loader defaults are:

```text
dataset_dir   = <strecha-root>
specific_data = castle-P30
image_folder  = images
gt_folder     = gt_dense_cameras
```

### ETH3D (`facade`)

```text
<eth3d-root>/
└── facade/
    ├── images/
    │   └── dslr_images_undistorted/
    └── dslr_calibration_undistorted/
```

The loader defaults are:

```text
dataset_dir   = <eth3d-root>
specific_data = facade
image_folder  = images/dslr_images_undistorted
gt_folder     = dslr_calibration_undistorted
```

To run another supported scene, keep the same parent directory and change only
`specific_data` and any scene-specific loader values required by that dataset.
Do not claim that a different scene is the exact default paper replay unless it
has been recorded separately.

## 3. Create an editable workflow project

1. Open **PoSDK GUI**.
2. If necessary, open **Preferences → Language** and select **English**.
3. Open the workflow catalog from the **Workflow** button in Behavior
   Workspace, or use **File → New from Workflow Library**.
4. Select **LiRPpaper-Strecha** or **LiRPpaper-ETH3D**.
5. Click **Create & Open** (shown as **Create & Open Workflow** in some
   localized builds).

PoSDK copies the immutable built-in definition to an editable `.posdk` project
under the user's PoSDK workflow directory. Editing that copy does not modify
the built-in release asset.

## 4. Confirm the dataset configuration

Open the dataset-loader module again and confirm the associated root and scene
fields in its input panel. If you entered the path manually, associate it from
that panel before running.

- For **LiRPpaper-Strecha**, set `dataset_dir` to the directory containing
  `castle-P30` (or the parent of the scene selected in `specific_data`).
- For **LiRPpaper-ETH3D**, set `dataset_dir` to the directory containing
  `facade` (or the parent of the scene selected in `specific_data`).

Keep the remaining protocol parameters at their released values for a stable
test. The loader validates the selected directory before the run starts.

## 5. Run the workflow

1. Save the project when prompted.
2. Click the green **Run** button in the top toolbar (`▶ Run`).
3. Keep the application open while feature extraction, matching, relative-pose
   estimation, rotation averaging, track construction, global reconstruction,
   and export are running. The Strecha and ETH3D workflows are intentionally
   serial single-run protocols.
4. Read progress and warnings in the **Console** panel. Warnings and errors
   for individual degenerate image pairs are retained; a completed workflow
   status means that the runner reached its terminal completion event and wrote
   the declared outputs, not that every individual pair was numerically valid.

The default output root created by the workflow catalog is:

```text
~/Documents/PoSDK Runs/<workflow-id>/
```

The exact path is shown in **Output Control**. Do not put output inside the
application bundle or inside the read-only DMG.

## 6. Inspect the results

After a successful run, use the toolbar viewers:

- **Evaluator**: inspect global and relative rotation/translation accuracy
  tables and the JSON result snapshot.
- **Profiler**: inspect method/core timing summaries and CSV exports.
- **Output Control** or Finder: inspect exported PLY/MLP files, pose graphs,
  tracks, logs, and per-run artifacts.

Typical result locations include:

```text
<run-root>/storage/evaluator_results/
<run-root>/storage/profiler/
<run-root>/output/
```

The exact filenames can vary with the PoSDK release and the selected output
options. Preserve the complete run directory when submitting reproducibility
materials; do not report only the final point cloud without the configuration,
Evaluator JSON, and Profiler CSV.

## 7. Reproducibility notes

- The built-in LiRPpaper workflows currently have no ordinary Loop rules and
  `repeatCount=1`; they are not an automatic multi-scene sweep.
- To compare algorithms or sample sizes, first create an editable `.posdk`
  project. Use **Var Workspace** only for an explicitly designed experiment
  matrix; bind parameters that must change together as one experiment variant.
- Keep the original project copy unchanged when reporting the released
  protocol. Save modified experiments under a separate project name and record
  every changed parameter.
- A run with a completed runner status can still contain genuine per-pair
  degeneracy warnings. These should remain visible in the submitted console
  log and should be discussed as part of the result quality.

## 8. Troubleshooting

### The workflow is not listed

Confirm that the downloaded DMG is the LiRPpaper release asset and that the
application was not replaced by an older copy in `/Applications`. The workflow
cards are embedded in the application; they are not loaded from the source
checkout.

### The workflow card reports missing plugins

Use the exact PoSDK release DMG that was published with the workflow. Do not
mix a workflow definition from one release with an application from another
release. If the release provides both Consumer and Developer products, use the
product named by the release notes; both products must contain the dependency
closure declared by the workflow manifest.

### The loader cannot find the data

Check that `dataset_dir` is the common parent and that `specific_data` names an
existing child directory. For ETH3D, do not select
`dslr_images_undistorted` itself as `dataset_dir`; select the parent containing
`facade`, `courtyard`, and other scenes.

### The run ends with warnings or rejected pairs

Open the Console and retain the warning/error lines. Some image pairs can be
rejected by LiRP quality checks or geometric degeneracy checks while the
workflow still completes and writes aggregate outputs. Treat those lines as
test diagnostics and record any configuration changes.
