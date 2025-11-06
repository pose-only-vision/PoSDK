# ==============================================================================
# Copyright (c) 2024 PoSDK Project
# File: plugins/data/CMakeLists.txt
# Description: CMake build configuration file for data plugins
# Author: Qi Cai
# Date: 2024-01
# ==============================================================================

# Set minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Define project name and language
project(data_plugin LANGUAGES CXX)

# ==============================================================================
# Dependency Path Configuration
# ==============================================================================

# Find and load OpenCV dependency
# OpenCV is used for image processing and feature extraction functionalities
find_package(OpenCV REQUIRED)


# ------------------------------------------------------------------------------
# Abseil Library Configuration - No longer searching for Abseil independently here
# Depends on the global Abseil version set in PoSDK's top-level CMakeLists.txt
# Ensure absl::* targets are already provided by top-level find_package(absl)
# ------------------------------------------------------------------------------
if(NOT TARGET absl::base)
    message(FATAL_ERROR "Abseil (absl::base target) not found. Ensure PoSDK's main CMakeLists.txt has correctly found Abseil.")
endif()
message(STATUS "Data Plugin: Using Abseil version ${absl_VERSION} found by main PoSDK project at ${absl_DIR}")

# ==============================================================================
# Data Plugin Configuration
# ==============================================================================

# Define the list of data plugins to be built
# Each plugin will be compiled into an independent dynamic library (.so/.dll)
set(DATA_PLUGINS
    DataExample     # Data serialization example plugin, demonstrates PbDataIO usage
)

# Iterate through the plugin list and build each plugin
# add_pomvg_plugin is a custom function used to unify plugin build rules
# Parameter 1: Plugin name
# Parameter 2: Plugin type (data)
# Parameter 3+: Plugin source files
foreach(PLUGIN ${DATA_PLUGINS})
    add_pomvg_plugin(${PLUGIN} data
        ${PLUGIN}.cpp
        ${PLUGIN}.hpp
    )
    
    # DataExample plugin is simplified and doesn't need proto dependencies
    # Real feature data handling should use po_core's built-in DataFeatures class
    # DataExample 插件已简化，不需要 proto 依赖
    # 实际的特征数据处理应使用 po_core 内置的 DataFeatures 类
endforeach()
