# ==============================================================================
# OpenGVModelEstimator Plugin CMakeLists.txt
# 双视图位姿估计插件（OpenGV）构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# OpenGVModelEstimator插件
# ------------------------------------------------------------------------------
add_posdk_plugin(opengv_model_estimator
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        opengv_model_estimator.cpp
    HEADERS
        opengv_model_estimator.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        ${OpenCV_LIBS}
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
    COMPILE_DEFINITIONS
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "OpenGVModelEstimator Plugin Configuration:")
message(STATUS "  Plugin Name: opengv_model_estimator")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_opengv_model_estimator.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: opengv_model_estimator.cpp")
message(STATUS "  Headers: opengv_model_estimator.hpp")
message(STATUS "  Dependencies: OpenCV, OpenGV")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building OpenGVModelEstimator plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "OpenGVModelEstimator plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "OpenGVModelEstimator plugin: Documentation enabled")
endif()

