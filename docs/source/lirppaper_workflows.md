# Running the LiRPpaper Workflows for Testing

This guide explains how to test the two LiRPpaper workflows distributed with
the PoSDK 2.0.0 macOS arm64 DMG. The workflows are built into the application;
a source checkout, CMake, Qt, or a local plugin build is not required.

The release contains two ready-to-run workflow definitions:

| Workflow | Dataset loader | Default scene |
| --- | --- | --- |
| **LiRPpaper-Strecha** | `strecha_dataset_loader` | `castle-P30` |
| **LiRPpaper-ETH3D** | `eth3d_dataset_loader` | `facade` |

The workflow project is editable after it is created, but no parameter changes
are needed for the standard test.

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

The loader validates the selected directory before the run starts. Leave the
other values unchanged for the standard test.

## 5. Run the workflow

1. Save the project when prompted.
2. Click the green **Run** button in the top toolbar (`▶ Run`).
3. Keep the application open until the run finishes.
4. Read progress and any warnings in the **Console** panel.

The default output root created by the workflow catalog is:

```text
~/Documents/PoSDK Runs/<workflow-id>/
```

The exact path is shown in **Output Control**. Do not put output inside the
application bundle or inside the read-only DMG.

## 6. Inspect the results

After a successful run, use the toolbar viewers:

- **Evaluator**: inspect the generated evaluation summary.
- **Profiler**: inspect the generated timing summary.
- **Output Control** or Finder: inspect the files written by the workflow.

Typical result locations include:

```text
<run-root>/storage/evaluator_results/
<run-root>/storage/profiler/
<run-root>/output/
```

The exact filenames can vary with the PoSDK release and the selected output
options. Preserve the complete run directory if you need to share the test
result.

## 7. Troubleshooting

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

### The run ends with warnings

Open the **Console** and retain the warning/error lines. If the workflow does
not finish, verify the dataset association and repeat the test with the
released workflow values unchanged.
