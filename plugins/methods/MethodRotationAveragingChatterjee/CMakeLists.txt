# ==============================================================================
# MethodRotationAveragingChatterjee Plugin CMakeLists.txt
# 旋转平均插件（Chatterjee算法）构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# MethodRotationAveragingChatterjee插件
# ------------------------------------------------------------------------------
add_posdk_plugin(method_rotation_averaging_chatterjee
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        method_rotation_averaging_Chatterjee.cpp
    HEADERS
        method_rotation_averaging_Chatterjee.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
    INCLUDE_DIRS
        ${CURRENT_PLUGIN_DIR}/chatterjee  # ✅ 包含 chatterjee 子目录
    COMPILE_DEFINITIONS
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "MethodRotationAveragingChatterjee Plugin Configuration:")
message(STATUS "  Plugin Name: method_rotation_averaging_chatterjee")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_method_rotation_averaging_chatterjee.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: method_rotation_averaging_Chatterjee.cpp")
message(STATUS "  Headers: method_rotation_averaging_Chatterjee.hpp")
message(STATUS "  Sub-directory: chatterjee/ (algorithm implementation)")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building MethodRotationAveragingChatterjee plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "MethodRotationAveragingChatterjee plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "MethodRotationAveragingChatterjee plugin: Documentation enabled")
endif()

