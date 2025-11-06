# ==============================================================================
# TwoViewEstimator Plugin Configuration
# Two-view pose estimation with multi-threading support
# 双视图位姿估计（支持多线程）
# ==============================================================================

# Include plugin configuration utilities
include(../../cmake/plugin-config.cmake)

# ------------------------------------------------------------------------------
# TwoViewEstimator Plugin
# ------------------------------------------------------------------------------
add_posdk_plugin(TwoViewEstimator
    PLUGIN_TYPE methods
    LINK_LIBRARIES
        ${OpenCV_LIBS}
        PoSDK::pomvg_converter
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
    COMPILE_DEFINITIONS
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<BOOL:${OpenMP_CXX_FOUND}>:USE_OPENMP>
)

# ------------------------------------------------------------------------------
# Plugin information summary | 插件信息摘要
# ------------------------------------------------------------------------------
message(STATUS "TwoViewEstimator Plugin Configuration:")
message(STATUS "  Plugin Name: TwoViewEstimator")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_TwoViewEstimator.dylib/.so/.dll")
message(STATUS "  Features: Multi-threaded two-view pose estimation")
if(OpenMP_CXX_FOUND)
    message(STATUS "  OpenMP: Enabled for parallel view pair processing")
else()
    message(WARNING "  OpenMP: Not found, will run single-threaded")
endif()

