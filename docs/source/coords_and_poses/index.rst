Coordinates, Pose Conversion, and Evaluation
============================================

This section details the conversion methods and evaluation processes involved when PoMVG handles different coordinate systems, pose representations, and interactions with third-party libraries and standard datasets.

Understanding this content is crucial for correctly using PoMVG for pose estimation, interfacing with other vision libraries, and evaluating algorithm performance.

:doc:`Pose Conventions and Conversions <conventions/index>`
   Detailed description of PoMVG's standard pose conventions and coordinate system conversion methods with mainstream vision libraries such as OpenMVG, OpenCV, OpenGV, etc.

:doc:`Dataset Specifications <datasets/index>`
   Introduction to the standard dataset formats supported by PoMVG, including loading and use of ETH3D, Strecha, and 1DSfM.

   :doc:`ETH3D: prepare data, associate folders, and poses <datasets/eth3d>`

   :doc:`1DSfM: prepare data, associate folders, and run <datasets/1dsfm>`

:doc:`Accuracy Evaluation System <evaluation/index>`
   Complete accuracy evaluation framework covering evaluation methods for relative poses, global poses, and global translations, providing standardized algorithm performance evaluation tools.

.. toctree::
   :maxdepth: 2
   :caption: Content Overview

   conventions/index
   datasets/index
   evaluation/index

