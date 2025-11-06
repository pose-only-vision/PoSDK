# Platform Comparison Test Results

This document presents performance and accuracy comparison test results between PoSDK and current mainstream 3D reconstruction platforms on standard datasets.


```{figure} ../_static/pipeline.png
:alt: Pose-only platform comparison framework with other platforms
:align: center
:width: 90%

Pose-only platform comparison framework with other platforms
```

## Platform Introduction

### PoSDK
- **Project Homepage**: [PoSDK Documentation](https://posdk.readthedocs.io/)
- **Repository**: [https://github.com/pose-only-vision/PoSDK](https://github.com/pose-only-vision/PoSDK)
- **Technical Features**: Efficient pose estimation platform based on pose-only imaging geometry theory
- **Core Algorithm**: GlobalSfM pipeline, supporting multiple preprocessors and algorithm comparison
- **Configuration Details**: See [GlobalSfM Pipeline Plugin Configuration](../basic_development/plugin_list.md#globalsfm-pipeline-posdk-globalsfm管道-⭐)
- **Development Team**: [Shanghai Jiao Tong University VINF Research Group](https://isn.sjtu.edu.cn/web/personal-page/yxwu)
- **License**: cc-by-sa-4.0 (Creative Commons Attribution Share Alike 4.0 International)

```{tip}
**PoSDK Runtime Parameter Configuration**

PoSDK executes comparison tests through the `globalsfm_pipeline` plugin with main parameters:

- **Preprocessing Type**: `preprocess_type=posdk` (also supports openmvg, opencv)
- **Evaluation Mode**: `evaluation_print_mode=comparison`
- **Comparison Algorithms**: `compared_pipelines=openmvg,COLMAP,GLOMAP`
- **Performance Analysis**: `enable_profiling=true`

For complete parameter descriptions, refer to [Plugin Configuration Documentation](../basic_development/plugin_list.md#基础配置参数).
```

### OpenMVG (Open Multiple View Geometry)
- **Project Homepage**: [https://github.com/openMVG/openMVG](https://github.com/openMVG/openMVG)
- **Repository**: [https://github.com/openMVG/openMVG](https://github.com/openMVG/openMVG)
- **Technical Features**:
  - Open-source multi-view geometry library
  - Provides complete SfM (Structure from Motion) solution
  - Supports both incremental and global reconstruction methods
- **Development Team**: Pierre Moulon and open-source community contributors
- **License**: MPL2 (Mozilla Public License 2.0)

### COLMAP
- **Project Homepage**: [https://COLMAP.github.io/](https://COLMAP.github.io/)
- **Repository**: [https://github.com/COLMAP/COLMAP](https://github.com/COLMAP/COLMAP)
- **Technical Features**:
  - Industry-leading 3D reconstruction system
  - Supports incremental SfM, dense reconstruction, and MVS
  - Provides graphical interface and command-line tools
  - High-precision pose estimation and point cloud reconstruction
- **Development Team**: Johannes Schönberger, ETH team
- **License**: BSD License

### GLOMAP (Global Mapping)
- **Project Homepage**: [https://github.com/COLMAP/GLOMAP](https://github.com/COLMAP/GLOMAP)
- **Repository**: [https://github.com/COLMAP/GLOMAP](https://github.com/COLMAP/GLOMAP)
- **Technical Features**:
  - Next-generation global SfM system
  - Focuses on rapid reconstruction of large-scale scenes
  - Pose estimation method based on global optimization
  - COLMAP-compatible data format
- **Development Team**: ETH team
- **License**: BSD License

## Test Environment

- **Operating System**: Ubuntu 24.04 LTS
- **Processor**: Intel/AMD x86_64
- **Test Dataset**: Strecha standard dataset
  - fountain-P11 (11 images)
  - castle-P19/P30 (19/30 images)
  - entry-P10 (10 images)
  - Herz-Jesus-P8/P25 (8/25 images)

---

## Sparse Point Cloud Visualization

The following shows sparse point cloud reconstruction result comparisons from different platforms on the Strecha (castle-P30) dataset:

```{raw} html
<table style="width: 100%; border-collapse: collapse;">
  <tr>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/COLMAP.gif" alt="COLMAP sparse point cloud reconstruction result" style="width: 100%; max-width: 500px;">
      <p><strong>COLMAP</strong></p>
    </td>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/GLOMAP.gif" alt="GLOMAP sparse point cloud reconstruction result" style="width: 100%; max-width: 500px;">
      <p><strong>GLOMAP</strong></p>
    </td>
  </tr>
  <tr>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/openmvg.gif" alt="OpenMVG sparse point cloud reconstruction result" style="width: 100%; max-width: 500px;">
      <p><strong>OpenMVG</strong></p>
    </td>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/ours.gif" alt="PoSDK sparse point cloud reconstruction result" style="width: 100%; max-width: 500px;">
      <p><strong>PoSDK (Ours)</strong></p>
    </td>
  </tr>
</table>
```

**Notes**:
- All point clouds are compared and displayed under the same dataset
- Point cloud colors represent RGB information of feature points
- GIF animations show point cloud rotation views for easy observation of reconstruction quality


```{note}
Table data comes from platform test results. In the tables:
- <span style="color: #d32f2f; font-weight: bold;">Red bold</span>: Best result for the same dataset
- **Black bold**: Second-best result for the same dataset
```

## Total Runtime Comparison

The following table shows total runtime (unit: milliseconds) on different datasets:

```{raw} html
:file: processed/performance_comparison.html
```
---

## Accuracy Comparison Results

### 1. Global Pose Rotation Error

Global pose rotation error statistics (unit: degrees):

```{raw} html
:file: processed/global_rotation_error.html
```


### 2. Global Pose Translation Error

Global pose translation error statistics (unit: normalized distance):

```{raw} html
:file: processed/global_translation_error.html
```


### 3. Relative Pose Rotation Error

Relative pose rotation error statistics (unit: degrees):

```{raw} html
:file: processed/relative_rotation_error.html
```



---

## Dataset Details

### Strecha Dataset

The Strecha dataset is a classic multi-view stereo (MVS) evaluation dataset provided by École Polytechnique Fédérale de Lausanne (EPFL), containing:

- **fountain**: Fountain scene with complex geometric structures and rich textures
- **castle**: Castle scene with large viewing angle span and challenging occluded regions
- **entry**: Entry scene of medium scale with rich building details
- **Herz-Jesus**: Church scene with fine architectural details and lighting variations

Each dataset provides:
- High-resolution color images
- Calibrated camera intrinsic matrices
- Ground truth camera poses
- Ground truth 3D point clouds (for accuracy evaluation)

---


**References**:

1. Strecha, C., et al. "On Benchmarking Camera Calibration and Multi-View Stereo for High Resolution Imagery." CVPR 2008.
2. Moulon, P., et al. "OpenMVG: Open Multiple View Geometry." ICCV 2013 Workshop.
3. Cai, Q., et al. "A pose-only solution to visual reconstruction and navigation." TPAMI 2023.
