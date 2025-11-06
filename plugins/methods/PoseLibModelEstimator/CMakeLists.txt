# ==============================================================================
# PoseLibModelEstimator Plugin CMakeLists.txt
# 双视图位姿估计插件（PoseLib）构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# PoseLib库配置 (必需依赖)
# ------------------------------------------------------------------------------
# 设置PoseLib库路径（与dependencies/PoseLib目录结构一致）
set(POSELIB_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../dependencies/PoseLib/install_local")
set(POSELIB_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../dependencies/PoseLib/build_local")

# 检查PoseLib库是否存在（检查两个可能的位置）
if(EXISTS "${POSELIB_BUILD_DIR}/libPoseLib.a" OR EXISTS "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
    if(EXISTS "${POSELIB_BUILD_DIR}/libPoseLib.a")
        message(STATUS "Found PoseLib library at: ${POSELIB_BUILD_DIR}/libPoseLib.a")
        set(POSELIB_LIBRARIES "${POSELIB_BUILD_DIR}/libPoseLib.a")
        # 构建目录：头文件在源码根目录（这样才能用 #include <PoseLib/poselib.h>）
        set(POSELIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../dependencies/PoseLib")
    else()
        message(STATUS "Found PoseLib library at: ${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
        set(POSELIB_LIBRARIES "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
        # 安装后的头文件在include目录中
        set(POSELIB_INCLUDE_DIRS "${POSELIB_INSTALL_DIR}/include")
    endif()
else()
    message(FATAL_ERROR "PoseLib library not found. Please run dependencies/install_poselib.sh first.\n"
                        "Checked locations:\n"
                        "  - ${POSELIB_BUILD_DIR}/libPoseLib.a\n"
                        "  - ${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
endif()

# ------------------------------------------------------------------------------
# PoseLibModelEstimator插件
# ------------------------------------------------------------------------------
add_posdk_plugin(poselib_model_estimator
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        poselib_model_estimator.cpp
    HEADERS
        poselib_model_estimator.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        ${OpenCV_LIBS}
        ${POSELIB_LIBRARIES}
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
        ${POSELIB_INCLUDE_DIRS}
    COMPILE_DEFINITIONS
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "PoseLibModelEstimator Plugin Configuration:")
message(STATUS "  Plugin Name: poselib_model_estimator")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_poselib_model_estimator.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: poselib_model_estimator.cpp")
message(STATUS "  Headers: poselib_model_estimator.hpp")
message(STATUS "  Dependencies: OpenCV, PoseLib (required)")
message(STATUS "  PoseLib Library: ${POSELIB_LIBRARIES}")
message(STATUS "  PoseLib Include: ${POSELIB_INCLUDE_DIRS}")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building PoseLibModelEstimator plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "PoseLibModelEstimator plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "PoseLibModelEstimator plugin: Documentation enabled")
endif()

