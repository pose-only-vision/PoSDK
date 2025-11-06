# ==============================================================================
# MethodCalibrator Plugin CMakeLists.txt
# 相机标定插件构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# OpenGL 相关配置
# ------------------------------------------------------------------------------
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ------------------------------------------------------------------------------
# MethodCalibrator插件
# ------------------------------------------------------------------------------
add_posdk_plugin(method_calibrator
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        method_calibrator_plugin.cpp
    HEADERS
        method_calibrator_plugin.hpp
        CirclesPatternDetector.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        method_img2features  # 依赖特征提取插件
        ${CBDETECT_LIBRARY}
        ${OpenCV_LIBS}
        ${OPENGL_LIBRARIES}
        glfw
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
message(STATUS "MethodCalibrator Plugin Configuration:")
message(STATUS "  Plugin Name: method_calibrator")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_method_calibrator.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: method_calibrator_plugin.cpp")
message(STATUS "  Headers: method_calibrator_plugin.hpp, CirclesPatternDetector.hpp")
message(STATUS "  Dependencies: OpenCV, OpenGL, glfw, CBDetect, Img2Features")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building MethodCalibrator plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "MethodCalibrator plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "MethodCalibrator plugin: Documentation enabled")
endif()

