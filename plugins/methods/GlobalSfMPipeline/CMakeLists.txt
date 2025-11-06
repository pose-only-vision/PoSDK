# ==============================================================================
# GlobalSfMPipeline Plugin CMakeLists.txt
# 全局SfM处理流水线插件构建配置
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# GlobalSfMPipeline插件
# ------------------------------------------------------------------------------
add_posdk_plugin(globalsfm_pipeline
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        globalsfm_pipeline.cpp
        GlobalSfMPipelineParams.cpp
    HEADERS
        globalsfm_pipeline.hpp
        GlobalSfMPipelineParams.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
    COMPILE_DEFINITIONS
        #  删除 PLUGIN_NAME（由 add_posdk_plugin 自动添加）
        PLUGIN_VERSION="1.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------


# 插件信息摘要
message(STATUS "GlobalSfMPipeline Plugin Configuration:")
message(STATUS "  Plugin Name: globalsfm_pipeline")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_globalsfm_pipeline.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: globalsfm_pipeline.cpp, GlobalSfMPipelineParams.cpp")
message(STATUS "  Headers: globalsfm_pipeline.hpp, GlobalSfMPipelineParams.hpp")
message(STATUS "  Config: globalsfm_pipeline.ini")

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building GlobalSfMPipeline plugin in Debug mode")
endif()

# 测试支持（如果启用测试）
if(BUILD_TESTING)
    message(STATUS "GlobalSfMPipeline plugin: Testing enabled")
endif()

# 文档生成（如果启用文档）
if(BUILD_DOCUMENTATION)
    message(STATUS "GlobalSfMPipeline plugin: Documentation enabled")
endif()
