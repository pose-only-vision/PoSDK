.. PoMVG documentation master file, created by
   sphinx-quickstart on Sun May  4 15:38:07 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. only:: html

   Welcome to PoMVG Documentation!
   ===========================================

   PoMVG is a C++ software platform for **Spatial Visual Computing**, designed for 3D multi-view geometry processing, including camera pose estimation, feature point cloud generation, and more.

   .. tip::

      **New to PoMVG? Start here.**

      - Not installed yet? Follow the :doc:`v2.1.1 GUI Installation Guide <installation/gui_installation>` to download and install the DMG.
      - Already installed? Open :doc:`Quick Start <installation/cli_gui_guide>` to associate ETH3D data, switch scenes, run a workflow and view the point cloud.

   **Future feature updates and user documentation will focus on the GUI edition.** The v1.0.0 :doc:`installation instructions <installation/installation>` are retained for historical reference.

   This documentation also covers the PoMVG architecture, core library and custom plugin development. Plugins can integrate and compare reconstruction platforms such as OpenMVG, COLMAP and GLOMAP; see :doc:`Platform Comparison <benchmark_comparison/index>`.



   Pose-only Imaging Geometry
   --------------------------

   Traditional visual geometry methods face challenges such as the "curse of dimensionality", low computational efficiency, and insufficient robustness when processing large-scale scenes.

   **Pose-only Imaging Geometry** theory provides a novel approach, offering a new visual representation and computational paradigm. It expresses high-dimensional 3D scene information losslessly on a low-dimensional pose manifold, achieving complete decoupling between camera pose estimation and 3D scene reconstruction, without requiring joint optimization of poses and scene features in ultra-high-dimensional space.

   PoMVG core library is built based on the Pose-only Imaging Geometry theory, providing an efficient and robust toolkit designed to serve as an engine foundation for lightweight, embedded visual computing applications (such as robot navigation, spatial computing, mixed reality, etc.).

   .. image:: _static/PoIG.png
      :alt: Pose-only Imaging Geometry Concept
      :align: center
      :width: 480px

   Cite Us
   -------

   If your research benefits from the Pose-only theory or PoMVG, please cite the following publications:

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

   Quick Start <installation/cli_gui_guide>
   installation/index
   introduction
   benchmark_comparison/index
   copyright_tracking
   using_precompiled
   basic_development/index
   advanced_development/index
   converters/index
   coords_and_poses/index
   version_history/index
   faq/index
   team/index
   appendices/index

