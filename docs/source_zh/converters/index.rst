PoMVG 数据转换器
================

本文档详细介绍了PoMVG提供的各种数据转换器，用于实现PoMVG与其他主流计算机视觉库/工具之间的数据格式转换。

----

转换器模块结构
--------------

PoMVG转换器采用模块化设计，每个转换器负责与特定的外部库或工具进行数据交互。

文件组织
~~~~~~~~

.. _converter-file-organization:

.. code-block:: text

   common/converter/
   +-- converter_base.hpp         # 转换器基类
   +-- converter_opencv.hpp/cpp    # OpenCV数据转换器
   +-- converter_openmvg_file.hpp/cpp # OpenMVG文件转换器
   +-- converter_opengv.hpp/cpp    # OpenGV数据转换器
   +-- converter_colmap_file.hpp/cpp  # COLMAP文件转换器
   -- CMakeLists.txt             # 构建配置

模块职责分工
~~~~~~~~~~~~

.. _converter-module-responsibilities:

.. list-table::
   :header-rows: 1
   :widths: 20 50 30

   * - 模块
     - 主要功能
     - 文档链接
   * - ``OpenCVConverter``
     - 特征点转换、描述子转换、匹配转换、相机模型转换
     - :doc:`OpenCV 转换器 <opencv_converter>`
   * - ``OpenMVGFileConverter``
     - SfM数据加载、特征文件转换、匹配文件转换
     - :doc:`OpenMVG 转换器 <openmvg_converter>`
   * - ``OpenGVConverter``
     - Bearing向量转换、位姿转换、相机参数转换
     - :doc:`OpenGV 转换器 <opengv_converter>`
   * - ``ColmapConverter``
     - SfM数据转换、匹配数据转换、数据导出
     - :doc:`COLMAP 转换器 <colmap_converter>`

----

转换器功能对比
--------------

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 20 20

   * - 转换功能
     - :doc:`OpenCV <opencv_converter>`
     - :doc:`OpenMVG <openmvg_converter>`
     - :doc:`OpenGV <opengv_converter>`
     - :doc:`COLMAP <colmap_converter>`
   * - **特征数据**
     -
     -
     -
     -
   * - 特征点
     - Yes 双向转换
     - Yes 文件读取
     - \-
     - \-
   * - 描述子
     - Yes 双向转换
     - \-
     - \-
     - \-
   * - 匹配
     - Yes 双向转换
     - Yes 文件读取
     - Yes 匹配转Bearing
     - Yes 文件读取
   * - **几何数据**
     -
     -
     -
     -
   * - Bearing向量
     - \-
     - \-
     - Yes 像素/匹配转换
     - \-
   * - 相机模型
     - Yes 标定转换
     - Yes SfM文件读取
     - Yes 内参提取
     - Yes 文件读取/导出
   * - 相对位姿
     - \-
     - \-
     - Yes 算法结果转换
     - \-
   * - 全局位姿
     - \-
     - Yes SfM文件读取
     - \-
     - Yes 文件读取
   * - **3D数据**
     -
     -
     -
     -
   * - 3D点云
     - \-
     - Yes SfM文件读取
     - \-
     - Yes 文件读取/导出
   * - **其他数据**
     -
     -
     -
     -
   * - 图像路径
     - \-
     - Yes SfM文件读取
     - \-
     - \-
   * - PLY导出
     - \-
     - \-
     - \-
     - Yes 点云/场景导出

**图例说明**:

- **Yes** : 支持该功能
- ``-`` : 不支持

----

详细文档
--------

转换器使用说明、API参考和示例代码请参阅各子文档：

.. toctree::
   :maxdepth: 2

   opencv_converter
   openmvg_converter
   opengv_converter
   colmap_converter

----

快速入门示例
------------

OpenCV特征提取与转换
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <opencv2/features2d.hpp>
   #include "converter_opencv.hpp"

   using namespace PoSDK;
   using namespace PoSDK::Converter;

   // OpenCV特征提取
   cv::Ptr<cv::SIFT> detector = cv::SIFT::create();
   std::vector<cv::KeyPoint> keypoints;
   cv::Mat descriptors;
   detector->detectAndCompute(image, cv::noArray(), keypoints, descriptors);

   // 转换为PoSDK格式
   ImageFeatureInfo image_features;
   Descs descs;
   OpenCVConverter::CVFeatures2ImageFeatureInfoWithDesc(
       keypoints, descriptors, image_features, descs, 
       image_path, "SIFT");

OpenMVG数据加载
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_openmvg_file.hpp"

   using namespace PoSDK::Converter;

   // 加载OpenMVG SfM结果
   Interface::DataPtr global_poses_data;
   OpenMVGFileConverter::ToDataGlobalPoses(
       "sfm_data.json", global_poses_data);

OpenGV位姿估计
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_opengv.hpp"

   using namespace PoSDK::Converter;

   // 转换匹配为Bearing向量
   opengv::bearingVectors_t bearings1, bearings2;
   OpenGVConverter::MatchesToBearingVectors(
       matches, features_info, camera_models, view_pair,
       bearings1, bearings2);

COLMAP数据加载
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_colmap_file.hpp"

   using namespace PoSDK::Converter::COLMAP;

   // 1. 创建文件名到ID的映射（从OpenMVG SfM文件）
   std::map<std::string, int> file_name_to_id;
   SfMFileToIdMap("sfm_data.json", file_name_to_id);

   // 2. 加载全局位姿（从COLMAP images.txt文件）
   types::GlobalPoses global_poses;
   Interface::DataPtr global_poses_data = 
       std::make_shared<Interface::DataMap<types::GlobalPoses>>(global_poses);
   ToDataGlobalPoses("images.txt", global_poses_data, file_name_to_id);

   // 3. 加载匹配数据（从COLMAP匹配文件夹）
   types::Matches matches;
   Interface::DataPtr matches_data = 
       std::make_shared<Interface::DataMap<types::Matches>>(matches);
   ToDataMatches("matches/", matches_data, file_name_to_id);

导出为COLMAP数据格式
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "converter_colmap_file.hpp"

   using namespace PoSDK::Converter::COLMAP;

   // 导出PoSDK结果为COLMAP格式
   OutputPoSDK2Colmap(output_path, global_poses, camera_models,
                      features, tracks, pts3d);

----

**相关文档**:

- :doc:`核心数据类型 <../appendices/appendix_a_types>` - 了解PoMVG数据类型定义
- :doc:`插件开发 <../basic_development/index>` - 学习如何开发自定义插件
- :doc:`高级功能 <../advanced_development/index>` - 探索高级开发特性

