.. PoSDK documentation master file, created by
   sphinx-quickstart on Sun May  4 15:38:07 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

欢迎来到 PoSDK 开发文档！
===========================

PoSDK是一个面向 **空间视觉计算 (Spatial Visual Computing)** 的C++软件平台，可用于3D多视图几何处理，包括相机位姿估计、特征点云生成等。

本指南将介绍 PoSDK 架构、核心库的使用以及自定义插件的开发(通过插件可以集成及对比评测OpenMVG、COLMAP、GLOMAP等主流三维重建平台 :doc:`平台对比测试结果 <benchmark_comparison/index>`）

纯位姿成像几何（Pose-only Imaging Geometry）
--------------------------

经典视觉几何方法在处理大规模场景时，常常面临"维度灾难"、计算效率低以及鲁棒性不足等挑战。

**纯位姿成像几何 (Pose-only Imaging Geometry)** 理论方法另辟蹊径，给出了一种新的视觉表达及计算范式。它将高维 3D 场景信息无损地表达在低维位姿流形上，实现了相机位姿估计与三维场景重建的完全解耦，不需要进行位姿与场景特征的超高维空间联合优化。


PoSDK 核心库基于纯位姿成像几何理论方法构建，提供了一套高效、稳健的工具箱，希望为轻量化、嵌入式视觉计算应用（如机器人导航、空间计算、混合现实等）提供底座引擎支撑。

.. image:: _static/PoIG.png
   :alt: Pose-only Imaging Geometry Concept
   :align: center

Cite Us
--------------------

如果您的研究工作受益于纯位姿理论方法或 PoSDK，请引用以下文献：

.. code-block:: bibtex

   @article{cai2019equivalent,
     title={Equivalent Constraints for Two-View Geometry: Pose Solution/Pure Rotation Identification and 3D Reconstruction},
     journal={International Journal of Computer Vision},
     volume={127},
     pages={163--180},
     year={2019},
     publisher={Springer},
     doi={10.1007/s11263-018-1136-9}
   }

   @article{cai2023pose,
     title={A Pose-only Solution to Visual Reconstruction and Navigation},
     journal={IEEE Transactions on Pattern Analysis and Machine Intelligence},
     volume={45},
     number={1},
     pages={73--86},
     year={2023},
     publisher={IEEE},
     doi={10.1109/TPAMI.2021.3139681}
   }
   


.. toctree::
   :maxdepth: 2
   :hidden:

   introduction
   benchmark_comparison/index
   copyright_tracking
   installation/index
   using_precompiled
   basic_development/index
   advanced_development/index
   converters/index
   coords_and_poses/index
   # task_list/index
   version_history
   # version_log/index
   faq/index
   team
   appendices/index

License Compliance
==================

If you identify any open-source license compliance issues with PoSDK, please notify us immediately through `GitHub Issues <https://github.com/pose-only-vision/PoSDK/issues>`_. We are committed to maintaining compliance with all open-source licenses and will address any concerns promptly.

Indices and Tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

