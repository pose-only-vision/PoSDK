安装说明
========

**后续功能更新与用户文档以 GUI 版本为主，新用户请从 v2.1.1 GUI 版开始。** v1.0.0 安装说明保留为历史参考。

.. list-table::
   :header-rows: 1
   :widths: 24 44 32

   * - 版本
     - 适合的用途
     - 安装说明
   * - **v2.1.1（GUI 版）**
     - 通过图形界面运行工作流、处理数据、查看点云
     - :doc:`下载 DMG 并安装 <gui_installation>`
   * - **v1.0.0（旧版源码）**
     - 查阅历史安装脚本与编译环境
     - :doc:`历史源码安装参考 <installation>`

.. tip::

   v2.1.1 的安装步骤是 **下载 DMG → 拖入 Applications → 打开 PoMVG**。
   安装后，直接阅读 :doc:`快速开始 <cli_gui_guide>`，按 ETH3D 示例跑通第一组数据。

分版本安装说明
--------------

.. toctree::
   :maxdepth: 1

   gui_installation
   installation

源码集成与编译参考
------------------

以下内容供需要集成核心库或准备源码编译环境的用户查阅。

.. toctree::
   :maxdepth: 1

   using_precompiled
   po_core_build_environment
