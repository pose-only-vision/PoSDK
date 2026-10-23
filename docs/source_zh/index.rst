.. PoMVG documentation master file, created by
   sphinx-quickstart on Sun May  4 15:38:07 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. only:: html

   欢迎来到 PoMVG 文档！
   ===========================

   PoMVG是一个面向 **空间视觉计算 (Spatial Visual Computing)** 的C++软件平台，可用于3D多视图几何处理，包括相机位姿估计、特征点云生成等。

   .. tip::

      **第一次使用？从快速开始入门。**

      - 还没安装：阅读 :doc:`v2.1.1 GUI 安装说明 <installation/gui_installation>`，下载 DMG 并安装。
      - 已经安装：打开 :doc:`快速开始 <installation/cli_gui_guide>`，按 ETH3D 示例完成数据关联、切换场景、运行和查看点云。

   **后续功能更新与用户文档以 GUI 版本为主。** v1.0.0 的 :doc:`安装说明 <installation/installation>` 保留为历史参考。

   本文档还介绍 PoMVG 架构、核心库和自定义插件开发。通过插件可集成并对比 OpenMVG、COLMAP、GLOMAP 等三维重建平台，见 :doc:`平台对比评测 <benchmark_comparison/index>`。



   纯位姿成像几何（Pose-only Imaging Geometry）
   ------------------------------------------------------------

   经典视觉几何方法在处理大规模场景时，常常面临"维度灾难"、计算效率低以及鲁棒性不足等挑战。

   **纯位姿成像几何 (Pose-only Imaging Geometry)** 理论方法另辟蹊径，给出了一种新的视觉表达及计算范式。它将高维 3D 场景信息无损地表达在低维位姿流形上，实现了相机位姿估计与三维场景重建的完全解耦，不需要进行位姿与场景特征的超高维空间联合优化。


   PoMVG 核心库基于纯位姿成像几何理论方法构建，提供了一套高效、稳健的工具箱，希望为轻量化、嵌入式视觉计算应用（如机器人导航、空间计算、混合现实等）提供底座引擎支撑。

   .. image:: _static/PoIG.png
      :alt: Pose-only Imaging Geometry Concept
      :align: center
      :width: 480px

   Cite Us
   -------

   如果您的研究工作受益于纯位姿理论方法或 PoMVG，请引用以下文献：

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
      
      @article{cai2024linear,
        title={Linear Relative Pose Estimation Founded on Pose-only Imaging Geometry},
        journal={arXiv preprint arXiv:2401.13357},
        year={2024}
      }

   License Compliance
   ------------------

   If you identify any open-source license compliance issues with PoMVG, please notify us immediately through `GitHub Issues <https://github.com/pose-only-vision/PoSDK/issues>`_. We are committed to maintaining compliance with all open-source licenses and will address any concerns promptly.

   Indices and Tables
   ------------------

   * :ref:`genindex`
   * :ref:`search`

.. toctree::
   :maxdepth: 2
   :hidden:

   快速开始 <installation/cli_gui_guide>
   installation/index
   introduction
   benchmark_comparison/index
   copyright_tracking
   basic_development/index
   advanced_development/index
   converters/index
   coords_and_poses/index
   version_history/index
   faq/index
   team/index
   appendices/index

