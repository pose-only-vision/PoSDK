坐标与位姿转换和评估
====================

本部分详细介绍了PoMVG在处理不同坐标系、位姿表示以及与第三方库和标准数据集交互时涉及的转换方法和评估流程。

理解这些内容对于正确使用PoMVG进行位姿估计、与其他视觉库对接以及评估算法性能至关重要。

:doc:`位姿约定与转换 <conventions/index>`
   详细说明PoMVG的标准位姿约定，以及与OpenMVG、OpenCV、OpenGV等主流视觉库的坐标系转换方法。

:doc:`数据集规范 <datasets/index>`
   介绍PoMVG支持的标准数据集格式，包括 ETH3D、Strecha 和 1DSfM 的加载与使用。

   :doc:`ETH3D：准备数据、关联目录与位姿 <datasets/eth3d>`

   :doc:`1DSfM：准备数据、关联目录与运行 <datasets/1dsfm>`

:doc:`精度评估体系 <evaluation/index>`
   完整的精度评估框架，涵盖相对位姿、全局位姿和全局位移的评估方法，提供标准化的算法性能评估工具。

.. toctree::
   :maxdepth: 2
   :caption: 内容概览

   conventions/index
   datasets/index
   evaluation/index

