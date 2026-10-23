# 1DSfM Dataset

1DSfM supplies image features, feature tracks, pairwise geometry, and reference reconstructions for Internet landmark photo collections. Download the numerical data and optional photos from the [official dataset page](https://www.cs.cornell.edu/projects/1dsfm/).

## Prepare the Data

Extract the numerical data, keeping each scene in its own folder:

```text
1DSfM/
└── Alamo/
    ├── cc.txt
    ├── EGs.txt
    ├── coords.txt
    ├── tracks.txt
    ├── list.txt
    ├── gt_bundle.out
    └── images/          # Optional: separately downloaded photos
```

| File | Contents and use |
| --- | --- |
| `cc.txt` | Original image IDs in the connected component. |
| `EGs.txt` | Pairwise rotations and translation directions. |
| `coords.txt` | Feature coordinates, focal lengths, principal points, and feature colors. |
| `tracks.txt` | Correspondences recorded as image IDs and feature IDs. |
| `list.txt` | Image names and focal lengths; image IDs follow the file's ordering. |
| `gt_bundle.out` / `bundle.out` | A Bundler-format reference reconstruction. The loader defaults to `gt_bundle.out`. If your file is named `bundle.out`, set `bundle_file=bundle.out`. |

See the authors' [SfM_Init data description](https://github.com/wilsonkl/SfM_Init#data) for format details. Keep the original IDs and file contents; do not reorder image lists or tracks independently.

## Run in the GUI

1. Click **Workflow**, search for **1DSfM**, and create a reconstruction workflow that reads the official metadata.
2. Open **1DSfM DatasetLoader**, click **Choose root**, select the local `1DSfM` folder, and click **Associate with node**.
3. Choose a scene, such as **Alamo**. If you selected the `Alamo` folder itself, leave **Scene** empty.
4. Check **Metadata Folder**. Leave it empty when the listed files are directly inside the scene; otherwise enter their subfolder name.
5. Check the reference Bundle filename, save the workflow, and run. Open **Visualization** to inspect the cloud and cameras, **Evaluator** for accuracy, and **Profiler** for timings.

For reconstruction from the supplied features and tracks, photos are optional; keep `require_images=false`. To inspect photos or matching overlays, place the images in `images/` or change **Image Folder**.

## Input and Coordinate Handling

The loader remaps original image IDs consistently, reads calibration, and converts official SIFT pixel coordinates to the platform's zero-based convention. Keep `convert_sift_to_zero_based_pixels=true`; do not convert the input coordinates a second time.

By default, candidate matches are generated only for pairs listed in `EGs.txt`. The workflow then performs two-view verification, rotation averaging, track construction, and pose recovery. Track construction should consume the **verified matches output** of the two-view estimator.

## Inspect the Results

- Inspect point clouds, camera placement, and view recovery in the 3D viewer.
- Compare reference-aligned pose errors using the same evaluation view set.
- The supplied Bundle is a reference reconstruction, rather than independently surveyed ground truth. The platform comparison reports reprojection errors, recovered views, and point counts for 1DSfM.

See the [platform comparison](../../benchmark_comparison/index.rst) and [pose conventions](../conventions/posdk.md).
