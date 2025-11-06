# ==============================================================================
# MethodRotationAveraging Plugin CMakeLists.txt
# 旋转平均插件构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# MethodRotationAveraging插件
# ------------------------------------------------------------------------------
add_posdk_plugin(method_rotation_averaging
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        method_rotation_averaging.cpp
    HEADERS
        method_rotation_averaging.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        method_rotation_averaging_chatterjee  # ✅ 依赖 Chatterjee 算法插件
    COMPILE_DEFINITIONS
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 插件信息摘要
message(STATUS "MethodRotationAveraging Plugin Configuration:")
message(STATUS "  Plugin Name: method_rotation_averaging")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_method_rotation_averaging.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: method_rotation_averaging.cpp")
message(STATUS "  Headers: method_rotation_averaging.hpp")
message(STATUS "  Dependencies: method_rotation_averaging_chatterjee")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building MethodRotationAveraging plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "MethodRotationAveraging plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "MethodRotationAveraging plugin: Documentation enabled")
endif()

