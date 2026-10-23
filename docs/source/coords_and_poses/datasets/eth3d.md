# ETH3D Dataset

ETH3D provides high-resolution multi-view scenes with camera calibration. The PoMVG ETH3D DatasetLoader reads the undistorted DSLR images together with the intrinsics and poses in the matching calibration, for reconstruction and accuracy evaluation. Images and calibration are downloaded separately from the [ETH3D datasets page](https://www.eth3d.net/datasets).

## Prepare the Data

Each scene uses two official archives: `{scene}_dslr_undistorted.7z` for the undistorted images, and `{scene}_dslr_calibration_undistorted.7z` for the calibration. Extract both into the same scene folder:

```text
ETH3D/                         ← select this root
├── facade/
│   ├── images/
│   │   └── dslr_images_undistorted/
│   └── dslr_calibration_undistorted/
│       ├── cameras.txt
│       └── images.txt
└── meadow/
```

Do not select `images` or `dslr_images_undistorted` as the root. If the two archives are downloaded separately, extract both under the corresponding scene.

| Path | Contents and use |
| --- | --- |
| `images/dslr_images_undistorted/` | Undistorted DSLR images. This is the default `image_folder`. |
| `dslr_calibration_undistorted/cameras.txt` | COLMAP camera intrinsics. |
| `dslr_calibration_undistorted/images.txt` | Image names, quaternion poses, and translations. `gt_folder` points at this directory. |

The loader defaults are `specific_data=facade`, `device_type=dslr`, and `split=training`. Set `specific_data` to one scene folder under the root. Leave it empty only when `dataset_dir` already points at that scene; an empty value does not iterate every scene.

## Run in the GUI

1. Create an ETH3D reconstruction workflow from the workflow library.
2. Open **ETH3D DatasetLoader**, select the `ETH3D` root above, and click **Associate with node**.
3. Set the scene to an extracted folder, such as `facade` or `meadow`.
4. Keep the image path at `images/dslr_images_undistorted` and the ground-truth path at `dslr_calibration_undistorted`. Save and run.

Associating a directory, changing the scene, and looping over every scene under the root are described in the [quick start](../../installation/cli_gui_guide.md).

## Poses

The `qw qx qy qz` values and the following translation in `images.txt` are the COLMAP world-to-camera transform. The loader stores them as `PoseFormat::RwTc`. For a world point $X_w$, the camera coordinates are $X_c = R X_w + t$, where $R$ comes from that quaternion and $t$ is the translation in the file.

COLMAP `image_id` values are not necessarily contiguous from zero. The loader remaps them, in id order, to `0, 1, 2, ...`. With `compute_relative_poses=true`, relative poses are computed from these global poses.

See the [pose conventions](../conventions/posdk.md) and the [platform comparison](../../benchmark_comparison/index.rst).
