# ==============================================================================
# Img2Matches Plugin CMakeLists.txt
# 图像特征匹配插件构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# ------------------------------------------------------------------------------
# GLib 依赖检测（Linux 上 OpenCV + GTK-3 需要）
# GLib dependency detection (required by OpenCV + GTK-3 on Linux)
# ------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        # Find GLib using pkg-config (avoid Anaconda's GLib)
        # 使用 pkg-config 查找 GLib（避免 Anaconda 的 GLib）
        pkg_check_modules(GLIB QUIET glib-2.0)
        if(GLIB_FOUND)
            message(STATUS "Found GLib ${GLIB_VERSION} for GTK-3 compatibility")
            message(STATUS "  GLib libraries: ${GLIB_LIBRARIES}")
            message(STATUS "  GLib library dirs: ${GLIB_LIBRARY_DIRS}")
            
            # Verify GLib is from system, not Anaconda
            # 验证 GLib 来自系统，而非 Anaconda
            set(GLIB_FROM_SYSTEM TRUE)
            foreach(GLIB_LIB ${GLIB_LIBRARIES})
                if(GLIB_LIB MATCHES "anaconda" OR GLIB_LIB MATCHES "conda")
                    set(GLIB_FROM_SYSTEM FALSE)
                    message(WARNING "Warning: GLib found in Anaconda/Conda path: ${GLIB_LIB}")
                    message(WARNING "This may cause GTK-3 linking issues. Please ensure system GLib is used.")
                endif()
            endforeach()
            
            if(GLIB_FROM_SYSTEM)
                message(STATUS "  ✓ GLib is from system (not Anaconda)")
            else()
                message(WARNING "  ⚠ GLib may be from Anaconda - linking may fail")
            endif()
        else()
            message(WARNING "GLib not found via pkg-config. GTK-3 linking may fail if OpenCV uses GTK.")
            message(WARNING "Install with: sudo apt-get install libglib2.0-dev")
        endif()
    else()
        message(WARNING "pkg-config not found. Cannot detect GLib for GTK-3 compatibility.")
        message(WARNING "Install with: sudo apt-get install pkg-config libglib2.0-dev")
    endif()
endif()

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})


# ------------------------------------------------------------------------------
# Img2Matches插件
# ------------------------------------------------------------------------------
# ✅ 插件名称改为 method_img2matches（与注册名一致，实现单一信息源）
# Plugin name changed to method_img2matches (consistent with registration name, single source of truth)
add_posdk_plugin(method_img2matches
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        img2matches_pipeline.cpp
        img2matches_fast_mode.cpp
        img2matches_viewer_mode.cpp
        Img2MatchesParams.cpp
        FASTCASCADEHASHINGL2.cpp
        LightGlueMatcher.cpp
    HEADERS
        img2matches_pipeline.hpp
        Img2MatchesParams.hpp
        FASTCASCADEHASHINGL2.hpp
        LightGlueMatcher.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        method_img2features
        ${OpenCV_LIBS}
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
        $<$<BOOL:${GLIB_FOUND}>:${GLIB_LDFLAGS}>
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
    COMPILE_DEFINITIONS
        # ✅ 删除 PLUGIN_NAME（由 add_posdk_plugin 自动添加）
        # PLUGIN_NAME is now automatically defined by add_posdk_plugin
        PLUGIN_VERSION="2.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
        $<$<BOOL:${OpenMP_CXX_FOUND}>:USE_OPENMP>
)

# ------------------------------------------------------------------------------
# 依赖关系配置
# ------------------------------------------------------------------------------
# 确保Img2Features先构建
add_dependencies(method_img2matches method_img2features)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "Img2Matches Plugin Configuration:")
message(STATUS "  Plugin Name: method_img2matches")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_method_img2matches.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: img2matches_pipeline.cpp, img2matches_fast_mode.cpp, img2matches_viewer_mode.cpp, Img2MatchesParams.cpp, FASTCASCADEHASHINGL2.cpp")
message(STATUS "  Headers: img2matches_pipeline.hpp, Img2MatchesParams.hpp, FASTCASCADEHASHINGL2.hpp")
message(STATUS "  Config: method_img2matches.ini")
if(OpenMP_CXX_FOUND)
    message(STATUS "  OpenMP: Enabled for multi-threaded feature extraction")
else()
    message(WARNING "  OpenMP: Not found, feature extraction will run single-threaded")
endif()

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building Img2Matches plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "Img2Matches plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "Img2Matches plugin: Documentation enabled")
endif()
