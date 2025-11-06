# 坐标与位姿转换和评估

本部分详细介绍了PoSDK在处理不同坐标系、位姿表示以及与第三方库和标准数据集交互时涉及的转换方法和评估流程。

理解这些内容对于正确使用PoSDK进行位姿估计、与其他视觉库对接以及评估算法性能至关重要。

## [位姿约定与转换](conventions/index.md)
详细说明PoSDK的标准位姿约定，以及与OpenMVG、OpenCV、OpenGV等主流视觉库的坐标系转换方法。

## [数据集规范](datasets/index.md)  
介绍PoSDK支持的标准数据集格式，包括1DSfM、Strecha、RANSAC2020等数据集的加载和使用方法。

## [精度评估体系](evaluation/index.md)
完整的精度评估框架，涵盖相对位姿、全局位姿和全局位移的评估方法，提供标准化的算法性能评估工具。

```{toctree}
:maxdepth: 2
:caption: 内容概览

conventions/index
datasets/index
evaluation/index
``` 