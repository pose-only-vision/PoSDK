# ==============================================================================
# COLMAP 和 GLOMAP 预处理插件
# 文件: GLOMAP/CMakeLists.txt
# 描述: COLMAP和GLOMAP相关的预处理插件构建配置
# 作者: Qi Cai
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# 包含插件配置工具
include(../../cmake/plugin-config.cmake)

# 设置当前插件目录的特定配置
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# COLMAP预处理插件
# ------------------------------------------------------------------------------
# 方式1：显式指定源文件和头文件（适用于需要精确控制的场景）
add_posdk_plugin(colmap_pipeline
    PLUGIN_TYPE methods
    SOURCES colmap_pipeline.cpp
    HEADERS colmap_pipeline.hpp
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        PoSDK::pomvg_converter
    COMPILE_DEFINITIONS
        USE_COLMAP_PIPELINE=1
        $<$<CONFIG:Debug>:_DEBUG>
)

# ------------------------------------------------------------------------------
# GLOMAP预处理插件
# ------------------------------------------------------------------------------
add_posdk_plugin(glomap_pipeline
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES glomap_pipeline.cpp
    HEADERS glomap_pipeline.hpp
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        PoSDK::pomvg_converter
    COMPILE_DEFINITIONS
        USE_GLOMAP_PIPELINE=1
        USE_COLMAP_PIPELINE=1
        $<$<CONFIG:Debug>:_DEBUG>
)

# ------------------------------------------------------------------------------
# 插件特定配置
# ------------------------------------------------------------------------------

# 检查是否需要特殊的依赖配置
message(STATUS "GLOMAP plugin directory: Configuring COLMAP and GLOMAP preprocessing plugins")
message(STATUS "  - colmap_pipeline: COLMAP pipeline for incremental SfM")
message(STATUS "  - glomap_pipeline: GLOMAP pipeline for global SfM optimization")

# 为插件添加特殊的包含目录（如果需要）
if(TARGET colmap_pipeline)
    # 添加COLMAP特定的包含目录
    target_include_directories(colmap_pipeline PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(TARGET glomap_pipeline)
    # 添加GLOMAP特定的包含目录
    target_include_directories(glomap_pipeline PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# ------------------------------------------------------------------------------
# 安装和部署
# ------------------------------------------------------------------------------

# 安装README文件（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            DESTINATION docs/plugins/GLOMAP
            RENAME GLOMAP_README.md)
endif()

# 创建插件信息文件
set(PLUGIN_INFO_FILE "${CMAKE_CURRENT_BINARY_DIR}/plugin_info.txt")
file(WRITE ${PLUGIN_INFO_FILE}
"Plugin: GLOMAP Suite
Description: COLMAP and GLOMAP preprocessing plugins for SfM reconstruction
Author: Qi Cai, Xinrui Li
Plugins:
  - colmap_pipeline: COLMAP incremental SfM pipeline
  - glomap_pipeline: GLOMAP global SfM optimization pipeline
Dependencies:
  - Python3 with pycolmap environment
  - COLMAP binaries (via conda environment)
  - GLOMAP binaries (compiled from source)
  - OpenMVG binaries for image listing
")

install(FILES ${PLUGIN_INFO_FILE}
        DESTINATION docs/plugins/GLOMAP) 