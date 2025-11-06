# ==============================================================================
# BarathTwoViewEstimator Plugin CMakeLists.txt
# Barath两视图位姿估计插件构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# ------------------------------------------------------------------------------
# GLib 依赖检测（Linux 上 OpenCV + GTK-3 需要）
# ------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLIB QUIET glib-2.0)
        if(GLIB_FOUND)
            message(STATUS "Found GLib ${GLIB_VERSION} for GTK-3 compatibility")
        endif()
    endif()
endif()

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# BarathTwoViewEstimator插件
# ------------------------------------------------------------------------------
add_posdk_plugin(barath_two_view_estimator
    PLUGIN_TYPE methods
    OPTIONAL  # ← 标记为可选，构建失败不影响其他插件
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        barath_two_view_estimator.cpp
    HEADERS
        barath_two_view_estimator.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        ${OpenCV_LIBS}
        # MAGSAC 和 GraphCutRANSAC 静态库
        ${CMAKE_SOURCE_DIR}/dependencies/magsac-master/build_local/libMAGSAC.a
        ${CMAKE_SOURCE_DIR}/dependencies/magsac-master/build_local/libGraphCutRANSAC.a
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
        # GLib 依赖（解决 GTK-3 链接问题，Ubuntu 24.04+）
        $<$<BOOL:${GLIB_FOUND}>:${GLIB_LDFLAGS}>
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/dependencies/magsac-master/src/pymagsac/include
        ${CMAKE_SOURCE_DIR}/dependencies/magsac-master/graph-cut-ransac/src/pygcransac/include
        ${CMAKE_SOURCE_DIR}/dependencies/superansac-main/include
    COMPILE_DEFINITIONS
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
        $<$<BOOL:${OpenMP_CXX_FOUND}>:USE_OPENMP>
        $<$<BOOL:${MAGSAC_FOUND}>:HAVE_MAGSAC_BUNDLE_ADJUSTMENT>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "BarathTwoViewEstimator Plugin Configuration:")
message(STATUS "  Plugin Name: barath_two_view_estimator")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_barath_two_view_estimator.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: barath_two_view_estimator.cpp")
message(STATUS "  Headers: barath_two_view_estimator.hpp")
message(STATUS "  Dependencies: OpenCV, MAGSAC, GraphCutRANSAC, SupeRANSAC")

# OpenMP 支持信息
if(OpenMP_CXX_FOUND)
    message(STATUS "  OpenMP: Enabled for multi-threaded RANSAC")
else()
    message(WARNING "  OpenMP: Not found, MAGSAC functionality may be limited")
endif()

# MAGSAC Bundle Adjustment 支持信息
if(MAGSAC_FOUND)
    message(STATUS "  Bundle Adjustment: Enabled via MAGSAC")
else()
    message(WARNING "  Bundle Adjustment: Disabled (MAGSAC library not found)")
endif()

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building BarathTwoViewEstimator plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "BarathTwoViewEstimator plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "BarathTwoViewEstimator plugin: Documentation enabled")
endif()

