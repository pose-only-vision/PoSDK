# Coordinates, Pose Conversion, and Evaluation

This section details the conversion methods and evaluation processes involved when PoSDK handles different coordinate systems, pose representations, and interactions with third-party libraries and standard datasets.

Understanding this content is crucial for correctly using PoSDK for pose estimation, interfacing with other vision libraries, and evaluating algorithm performance.

## [Pose Conventions and Conversions](conventions/index.md)
Detailed description of PoSDK's standard pose conventions and coordinate system conversion methods with mainstream vision libraries such as OpenMVG, OpenCV, OpenGV, etc.

## [Dataset Specifications](datasets/index.md)
Introduction to standard dataset formats supported by PoSDK, including data loading and usage methods for datasets such as 1DSfM, Strecha, RANSAC2020, etc.

## [Accuracy Evaluation System](evaluation/index.md)
Complete accuracy evaluation framework covering evaluation methods for relative poses, global poses, and global translations, providing standardized algorithm performance evaluation tools.

```{toctree}
:maxdepth: 2
:caption: Content Overview

conventions/index
datasets/index
evaluation/index
```
