.. PoSDK documentation master file, created by
   sphinx-quickstart on Sun May  4 15:38:07 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to PoSDK Development Documentation!
===========================================

PoSDK is a C++ software platform for **Spatial Visual Computing**, designed for 3D multi-view geometry processing, including camera pose estimation, feature point cloud generation, and more.

This guide introduces the PoSDK architecture, core library usage, and custom plugin development (through plugins, you can integrate and benchmark mainstream 3D reconstruction platforms such as OpenMVG, COLMAP, GLOMAP, etc. :doc:`Benchmark Comparison Results <benchmark_comparison/index>`).

Pose-only Imaging Geometry
--------------------------

Traditional visual geometry methods face challenges such as the "curse of dimensionality", low computational efficiency, and insufficient robustness when processing large-scale scenes.

**Pose-only Imaging Geometry** theory provides a novel approach, offering a new visual representation and computational paradigm. It expresses high-dimensional 3D scene information losslessly on a low-dimensional pose manifold, achieving complete decoupling between camera pose estimation and 3D scene reconstruction, without requiring joint optimization of poses and scene features in ultra-high-dimensional space.

PoSDK core library is built based on the Pose-only Imaging Geometry theory, providing an efficient and robust toolkit designed to serve as an engine foundation for lightweight, embedded visual computing applications (such as robot navigation, spatial computing, mixed reality, etc.).

.. image:: _static/PoIG.png
   :alt: Pose-only Imaging Geometry Concept
   :align: center

Cite Us
--------------------

If your research benefits from the Pose-only theory or PoSDK, please cite the following publications:

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
   version_history
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

