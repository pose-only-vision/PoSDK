PoMVG Data Converters
=====================

This document details various data converters provided by PoMVG for data format conversion between PoMVG and other mainstream computer vision libraries/tools.

----

Converter Module Structure
--------------------------

PoMVG converters adopt a modular design, with each converter responsible for data interaction with specific external libraries or tools.

File Organization
~~~~~~~~~~~~~~~~~

.. _converter-file-organization:

.. code-block:: text

   common/converter/
   +-- converter_base.hpp         # Converter base class
   +-- converter_opencv.hpp/cpp    # OpenCV data converter
   +-- converter_openmvg_file.hpp/cpp # OpenMVG file converter
   +-- converter_opengv.hpp/cpp    # OpenGV data converter
   +-- converter_colmap_file.hpp/cpp  # COLMAP file converter
   -- CMakeLists.txt             # Build configuration

Module Responsibilities
~~~~~~~~~~~~~~~~~~~~~~~

.. _converter-module-responsibilities:

.. list-table::
   :header-rows: 1
   :widths: 20 50 30

   * - Module
     - Main Functions
     - Documentation Link
   * - ``OpenCVConverter``
     - Feature point conversion, Descriptor conversion, Match conversion, Camera model conversion
     - :doc:`OpenCV Converter <opencv_converter>`
   * - ``OpenMVGFileConverter``
     - SfM data loading, Feature file conversion, Match file conversion
     - :doc:`OpenMVG Converter <openmvg_converter>`
   * - ``OpenGVConverter``
     - Bearing vector conversion, Pose conversion, Camera parameter conversion
     - :doc:`OpenGV Converter <opengv_converter>`
   * - ``ColmapConverter``
     - SfM data conversion, Match data conversion, Data export
     - :doc:`COLMAP Converter <colmap_converter>`

----

Converter Feature Comparison
-----------------------------

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 20 20

   * - Conversion Feature
     - :doc:`OpenCV <opencv_converter>`
     - :doc:`OpenMVG <openmvg_converter>`
     - :doc:`OpenGV <opengv_converter>`
     - :doc:`COLMAP <colmap_converter>`
   * - **Feature Data**
     -
     -
     -
     -
   * - Feature points
     - Yes Bidirectional conversion
     - Yes File reading
     - \-
     - \-
   * - Descriptors
     - Yes Bidirectional conversion
     - \-
     - \-
     - \-
   * - Matches
     - Yes Bidirectional conversion
     - Yes File reading
     - Yes Match to Bearing
     - Yes File reading
   * - **Geometric Data**
     -
     -
     -
     -
   * - Bearing vectors
     - \-
     - \-
     - Yes Pixel/match conversion
     - \-
   * - Camera models
     - Yes Calibration conversion
     - Yes SfM file reading
     - Yes Intrinsic extraction
     - Yes File reading/export
   * - Relative poses
     - \-
     - \-
     - Yes Algorithm result conversion
     - \-
   * - Global poses
     - \-
     - Yes SfM file reading
     - \-
     - Yes File reading
   * - **3D Data**
     -
     -
     -
     -
   * - 3D point clouds
     - \-
     - Yes SfM file reading
     - \-
     - Yes File reading/export
   * - **Other Data**
     -
     -
     -
     -
   * - Image paths
     - \-
     - Yes SfM file reading
     - \-
     - \-
   * - PLY export
     - \-
     - \-
     - \-
     - Yes Point cloud/scene export

**Legend**:

- **Yes** : Feature supported
- ``-`` : Not supported

----

Detailed Documentation
----------------------

For converter usage instructions, API reference, and example code, please refer to the following sub-documents:

.. toctree::
   :maxdepth: 2

   opencv_converter
   openmvg_converter
   opengv_converter
   colmap_converter

----

Quick Start Examples
---------------------

OpenCV Feature Extraction and Conversion
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <opencv2/features2d.hpp>
   #include "converter_opencv.hpp"

   using namespace PoSDK;
   using namespace PoSDK::Converter;

   // OpenCV feature extraction
   cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
   std::vector<cv::KeyPoint> keypoints;
   cv::Mat descriptors;
   detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

   // Convert to PoSDK format
   ImageFeatureInfo image_features;
   Descs descs;
   OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
       keypoints, descriptors, image_features, descs, 
       image_path, "SIFT");

OpenMVG Data Loading
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_openmvg_file.hpp"

   using namespace PoSDK::Converter;

   // Load OpenMVG SfM results
   Interface::DataPtr global_poses_data;
   OpenMVGFileConverter::ToDataGlobalPoses(
       "sfm_data.json", global_poses_data);

OpenGV Pose Estimation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_opengv.hpp"

   using namespace PoSDK::Converter;

   // Convert matches to Bearing vectors
   opengv::bearingVectors_t bearings1, bearings2;
   OpenGVConverter::MatchesToBearingVectors(
       matches, features_info, camera_models, view_pair,
       bearings1, bearings2);

COLMAP Data Loading
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_colmap_file.hpp"

   using namespace PoSDK::Converter::COLMAP;

   // 1. Create file name to ID mapping (from OpenMVG SfM file)
   std::map<std::string, int> file_name_to_id;
   SfMFileToIdMap("sfm_data.json", file_name_to_id);

   // 2. Load global poses (from COLMAP images.txt file)
   types::GlobalPoses global_poses;
   Interface::DataPtr global_poses_data = 
       std::make_shared<Interface::DataMap<types::GlobalPoses>>(global_poses);
   ToDataGlobalPoses("images.txt", global_poses_data, file_name_to_id);

   // 3. Load match data (from COLMAP matches folder)
   types::Matches matches;
   Interface::DataPtr matches_data = 
       std::make_shared<Interface::DataMap<types::Matches>>(matches);
   ToDataMatches("matches/", matches_data, file_name_to_id);

Export to COLMAP Data Format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_colmap_file.hpp"

   using namespace PoSDK::Converter::COLMAP;

   // Export PoSDK results to COLMAP format
   OutputPoSDK2Colmap(output_path, global_poses, camera_models,
                      features, tracks, pts3d);

----

**Related Documentation**:

- :doc:`Core Data Types <../appendices/appendix_a_types>` - Learn PoMVG data type definitions
- :doc:`Plugin Development <../basic_development/index>` - Learn how to develop custom plugins
- :doc:`Advanced Features <../advanced_development/index>` - Explore advanced development features

