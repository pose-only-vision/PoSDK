Basic Plugin Development
=========================

This section introduces the fundamentals of PoMVG plugin development, including core interfaces, plugin registration, runtime manifests, platform compatibility declarations, GUI extension points, and detailed descriptions of built-in data and method plugins.

.. note::

   **PoMVG currently mainly supports the following plugin development**:

   - **DataIO Plugins**: For encapsulating various data types
   - **Method Plugins**: For implementing specific algorithm logic

   **Behavior Plugins** are temporarily not open for user development.

Quick Start
-----------

.. tip::

   **Recommended reading order for beginners**:

   1. First read :doc:`Plugin Development Quick Example <method_example>` to understand the basic plugin development workflow
   2. Then check :doc:`Data Plugin Interface Details <data_plugins>` and :doc:`Method Plugin Interface Details <method_plugins>` to gain deeper understanding of interface design
   3. Review :doc:`GUI Plugin Extensions <gui_extensions>` if your plugin ships custom QML
   4. Finally refer to :ref:`Built-in Type Reference <built-in-type-reference>` to learn about available data types and methods

Plugin Registration
-------------------

All plugins need to use the ``REGISTRATION_PLUGIN`` macro for registration at the end of the corresponding ``.cpp`` source file.

.. important::

   **Automatic Implementation of** ``GetType()`` **Function**

   The ``REGISTRATION_PLUGIN`` macro **automatically implements** the ``GetType()`` function:
   - Only **declare** ``const std::string& GetType() const override;`` in the header file
   - **Do not** manually implement ``GetType()`` in the source file
   - The macro automatically generates the implementation

**Plugin Registration Method**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Registration Method
     - Method 1
   * - **Usage**
     - Single-param + CMake
   * - **Syntax**
     - ``REGISTRATION_PLUGIN(MyClass)``
   * - **Plugin Name Source**
     - CMake auto-defined
   * - **Applicable Scenarios**
     - Any namespace
   * - **Recommendation**
     - **Highly Recommended**

.. tip::

   **Recommended Method (Single Source of Truth):**

   Define plugin name in **CMakeLists.txt**:

   .. code-block:: cmake

      add_posdk_plugin(my_method  # ← Plugin name defined only once here
          PLUGIN_TYPE methods
          SOURCES my_method.cpp
      )

   Use single-parameter mode in **C++ code** (auto-read):

   .. code-block:: cpp

      REGISTRATION_PLUGIN(MyNamespace::MyMethod)  // ← Automatically uses CMake-defined name

   **Advantages**:

   - **Single Source of Truth**: Plugin name only needs to be defined once in CMake
   - **Automatic Consistency**: File name, registration name, and CMake target automatically consistent
   - **Error Prevention**: No manual synchronization needed, reduces maintenance cost

.. note::

   **Important Notes**:

   - ``REGISTRATION_PLUGIN`` **must** be placed in **source file (.cpp)**, **cannot** be placed in header file (.hpp)
   - Ensure registered type string is unique within current plugin directory
   - Recommended to use **single-parameter + CMake auto-definition** mode for single source of truth management
   - Current builds also generate a runtime sidecar manifest ``posdk_plugin_<name>.posdk.json``

Platform Compatibility and Runtime Filtering
--------------------------------------------

``add_posdk_plugin(...)`` now supports optional deployment compatibility declarations:

.. code-block:: cmake

   add_posdk_plugin(my_method
       PLUGIN_TYPE methods
       SUPPORTED_SYSTEMS macos linux
       SUPPORTED_ARCHES arm64 x86_64
   )

Key behavior:

- Omitting both fields means the plugin is treated as cross-platform by default
- Build output generates ``posdk_plugin_<name>.posdk.json`` beside the library
- Compatible plugins load into ``Factory*`` and appear in ``Modules``
- Unsupported or invalid plugin binaries stay visible in diagnostics such as ``Plugin Paths`` but are filtered from runtime loading

**Example - Plugin Registration**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**CMakeLists.txt**:

.. code-block:: cmake

   # Plugin name defined here (single definition point)
   add_posdk_plugin(opencv_two_view_estimator
       PLUGIN_TYPE methods
       SOURCES opencv_two_view_estimator.cpp
       HEADERS opencv_two_view_estimator.hpp
       LINK_LIBRARIES
           # Note: PoSDK::po_core, PoSDK::pomvg_converter, PoSDK::pomvg_common, and Eigen3::Eigen
           # are automatically linked by add_posdk_plugin function, no need to specify them here.
           # 注意：PoSDK::po_core、PoSDK::pomvg_converter、PoSDK::pomvg_common 和 Eigen3::Eigen
           # 已由 add_posdk_plugin 函数自动链接，无需在此指定。
           ${OpenCV_LIBS}  # Only specify additional libraries not auto-linked
       INCLUDE_DIRS
           ${OpenCV_INCLUDE_DIRS}
   )

**C++ Header File (opencv_two_view_estimator.hpp)**:

.. code-block:: cpp

   namespace PluginMethods {
       class OpenCVTwoViewEstimator : public MethodPresetProfiler {
       public:
           // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
           const std::string& GetType() const override;
           DataPtr Run() override;
           // ... (other methods) ...
       };
   }

**C++ Source File (opencv_two_view_estimator.cpp)**:

.. code-block:: cpp

   #include "opencv_two_view_estimator.hpp"

   namespace PluginMethods {

   // ... (Other function implementations for OpenCVTwoViewEstimator class) ...

   } // namespace PluginMethods

   // ✅ Single-parameter mode - automatically reads from CMake PLUGIN_NAME
   // Plugin type: "opencv_two_view_estimator" (from CMake)
   // File name: posdk_plugin_opencv_two_view_estimator.{so|dylib|dll}
   REGISTRATION_PLUGIN(PluginMethods::OpenCVTwoViewEstimator)

**Automatic Consistency**:
- CMake Target: ``opencv_two_view_estimator``
- Plugin file name: ``posdk_plugin_opencv_two_view_estimator.dylib``
- Registration type: ``"opencv_two_view_estimator"`` (auto-read from CMake)

Plugin Directory Management
----------------------------

PoMVG uses factory pattern to create and manage plugins. The system loads dynamic libraries (.so/.dll/.dylib) as plugins from specific directories. Each plugin type has a corresponding default directory:

.. important::

   Typical Developer output layout (relative to ``output/bin/``; the GUI plugin path manager resolves actual package locations):
   - Data plugins: ``../plugins/data/``
   - Method plugins: ``../plugins/methods/``

Plugin Loading Mechanism
-------------------------

Factories initialize lazily when used directly: the first ``Create()`` or ``DispPluginTypes()`` call can trigger a directory scan, and ``ManagePlugins()`` manages directories explicitly. The GUI plugin path manager validates and publishes the available registry before objects are created on demand. Factory object creation proceeds as follows:

1. Check if plugin of corresponding type has been loaded
2. If not loaded, attempt to load all matching dynamic libraries from default plugin directory
3. When user requests object creation, prioritize creation from built-in factory; if no matching type, attempt to create from plugin

.. tip::

   If you encounter plugin loading issues, please check:
   1. Whether plugin directory exists and has read permissions
   2. Whether plugin files are compatible with current system (Linux .so, Windows .dll, macOS .dylib)
   3. Whether plugin correctly registered the type
   4. Whether all plugin dependency libraries are correctly loaded

Manually Specify Plugin Directory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can manually specify plugin directory through factory class static method ``ManagePlugins``:

.. code-block:: cpp

   PoSDK::FactoryData::ManagePlugins("/path/to/your/data/plugins");
   PoSDK::FactoryMethod::ManagePlugins("/path/to/your/method/plugins");
   PoSDK::FactoryBehavior::ManagePlugins("/path/to/your/behavior/plugins");

Get Available Plugin Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use the following functions to display available plugin types in command line:

.. code-block:: cpp

   // Only display types provided by plugins
   PoSDK::FactoryData::DispPluginTypes();
   PoSDK::FactoryMethod::DispPluginTypes();
   PoSDK::FactoryBehavior::DispPluginTypes();

Plugin Development Guide
-------------------------

This section provides complete plugin development tutorials and interface specifications.

.. toctree::
   :maxdepth: 1
   :caption: Plugin Development

   plugin_guide
   workflow_library
   method_example
   data_plugins
   method_plugins
   gui_extensions
   prior_and_gt

.. _built-in-type-reference:

Built-in Type Reference
-----------------------

This section provides a list of preset data and method plugins in PoMVG core library along with brief descriptions.

.. toctree::
   :maxdepth: 1
   :caption: Built-in Types

   data_types
   plugin_list
