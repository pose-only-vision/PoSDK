# ==============================================================================
# Copyright (c) 2024 PoSDK Project
# ==============================================================================

# Find OpenGV package
find_package(OpenGV REQUIRED)

if(OpenGV_FOUND)
    message(STATUS "Found OpenGV: ${OpenGV_DIR}")
    message(STATUS "OpenGV include dirs: ${OpenGV_INCLUDE_DIRS}")
    message(STATUS "OpenGV libraries: ${OpenGV_LIBRARIES}")
else()
    message(FATAL_ERROR "OpenGV not found! Searched in ${OpenGV_DIR}")
endif()

# Find nlohmann_json library
# 查找 nlohmann_json 库

# Step 1: Try local dependencies directory first (with full CMake support)
# 步骤1：优先尝试本地 dependencies 目录（完整 CMake 支持）
set(LOCAL_NLOHMANN_DIR "${CMAKE_SOURCE_DIR}/dependencies/nlohmann")
set(LOCAL_NLOHMANN_INSTALL_DIR "${LOCAL_NLOHMANN_DIR}/install_local")

# Check if locally built nlohmann/json exists
# 检查是否存在本地构建的 nlohmann/json
if(EXISTS "${LOCAL_NLOHMANN_INSTALL_DIR}/share/cmake/nlohmann_json/nlohmann_jsonConfig.cmake")
    message(STATUS "Found locally built nlohmann/json at: ${LOCAL_NLOHMANN_INSTALL_DIR}")
    message(STATUS "找到本地构建的 nlohmann/json：${LOCAL_NLOHMANN_INSTALL_DIR}")
    
    # Use the locally built installation
    # 使用本地构建的安装
    set(nlohmann_json_DIR "${LOCAL_NLOHMANN_INSTALL_DIR}/share/cmake/nlohmann_json")
    find_package(nlohmann_json REQUIRED)
    
    message(STATUS "✓ Using locally built nlohmann_json")
    message(STATUS "✓ 使用本地构建的 nlohmann_json")
elseif(EXISTS "${LOCAL_NLOHMANN_DIR}/CMakeLists.txt")
    message(STATUS "Found local nlohmann/json source with CMake support at: ${LOCAL_NLOHMANN_DIR}")
    message(STATUS "在本地找到带 CMake 支持的 nlohmann/json 源码：${LOCAL_NLOHMANN_DIR}")
    
    # Use the local nlohmann/json source with CMake config
    # 使用本地的 nlohmann/json 源码 CMake 配置
    set(nlohmann_json_DIR "${LOCAL_NLOHMANN_DIR}")
    find_package(nlohmann_json REQUIRED)
    
    message(STATUS "✓ Using local nlohmann_json source with CMake config")
    message(STATUS "✓ 使用本地 nlohmann_json 源码 CMake 配置")
else()
    # Step 2: Try system-wide find_package
    # 步骤2：尝试系统级 find_package
    find_package(nlohmann_json QUIET)

    if(nlohmann_json_FOUND)
        message(STATUS "Found nlohmann_json via system find_package")
        message(STATUS "通过系统 find_package 找到 nlohmann_json")
    else()
        # Step 3: Check if header file exists (manually installed or single-header version)
        # 步骤3：检查头文件是否存在（手动安装或单头文件版本）
        
        # First check local dependencies (single-header version)
        # 首先检查本地 dependencies（单头文件版本）
        find_path(NLOHMANN_JSON_INCLUDE_DIR
            NAMES nlohmann/json.hpp
            PATHS
                "${CMAKE_SOURCE_DIR}/dependencies/nlohmann/single_include"  # Complete source
                "${CMAKE_SOURCE_DIR}/dependencies/nlohmann/include"         # Complete source
                "${CMAKE_SOURCE_DIR}/dependencies/nlohmann"                 # Single header
                /usr/local/include
                /usr/include
            NO_DEFAULT_PATH
        )
        
        if(NLOHMANN_JSON_INCLUDE_DIR)
            message(STATUS "Found nlohmann_json header at: ${NLOHMANN_JSON_INCLUDE_DIR}")
            message(STATUS "在以下位置找到 nlohmann_json 头文件：${NLOHMANN_JSON_INCLUDE_DIR}")
            
            # Create an interface library for nlohmann_json
            # 为 nlohmann_json 创建接口库
            add_library(nlohmann_json INTERFACE)
            target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
            add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
            
            message(STATUS "✓ Created interface library for nlohmann_json")
            message(STATUS "✓ 为 nlohmann_json 创建了接口库")
        else()
            message(FATAL_ERROR "nlohmann_json not found! Please run install.sh to install dependencies first.")
            message(FATAL_ERROR "未找到 nlohmann_json！请先运行 install.sh 安装依赖。")
        endif()
    endif()
endif()

# ==============================================================================
# Converter library build configuration
# ==============================================================================

# Base source file list
set(CONVERTER_SOURCES
    converter_base.hpp
    converter_opencv.hpp
    converter_opencv.cpp
    converter_opengv.hpp
    converter_opengv.cpp
    converter_openmvg_file.hpp
    converter_openmvg_file.cpp
    converter_openmvg_file.hpp
    converter_openmvg_file.cpp
    converter_colmap_file.hpp
    converter_colmap_file.cpp
)


# Create converter library
add_library(pomvg_converter SHARED ${CONVERTER_SOURCES})

# ------------------------------------------------------------------------------
# Header file include configuration
# ------------------------------------------------------------------------------
target_include_directories(pomvg_converter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${OUTPUT_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/dependencies/cereal/include
)

# If there is no import target, manually add include directory
if(NOT TARGET OpenGV::OpenGV)
    target_include_directories(pomvg_converter
        PUBLIC ${OpenGV_INCLUDE_DIRS}
    )
endif()

# ------------------------------------------------------------------------------
# Dependency library linking configuration
# ------------------------------------------------------------------------------
target_link_libraries(pomvg_converter
    PUBLIC
        PoSDK::po_core
        Eigen3::Eigen
        nlohmann_json::nlohmann_json
    PRIVATE
        ${OpenCV_LIBS}       # OpenCV is used internally only
)

# Check if OpenGV import target exists, prefer using import target
if(TARGET OpenGV::OpenGV)
    target_link_libraries(pomvg_converter
        PUBLIC OpenGV::OpenGV
    )
    message(STATUS "Using OpenGV import target for pomvg_converter")
else()
    target_link_libraries(pomvg_converter
        PUBLIC ${OpenGV_LIBRARIES}
    )
    message(STATUS "Using OpenGV variables for pomvg_converter")
endif()

# ------------------------------------------------------------------------------
# Compile options configuration
# ------------------------------------------------------------------------------
target_compile_options(pomvg_converter PRIVATE ${POMVG_COMPILE_OPTIONS})
target_compile_definitions(pomvg_converter PRIVATE ${POMVG_COMPILE_DEFINITIONS})

target_compile_options(pomvg_converter PRIVATE -fPIC)

if(USE_SANITIZER)
    target_compile_options(pomvg_converter PRIVATE 
        -fsanitize=address 
        -fno-omit-frame-pointer
    )
    target_link_options(pomvg_converter PRIVATE 
        -fsanitize=address
    )
endif()

# ------------------------------------------------------------------------------
# Output configuration
# ------------------------------------------------------------------------------
set_target_properties(pomvg_converter PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    POSITION_INDEPENDENT_CODE ON
)

# Copy header files to build directory
file(GLOB CONVERTER_HEADERS "*.hpp" "*.h")

foreach(HEADER ${CONVERTER_HEADERS})
    file(COPY ${HEADER} 
        DESTINATION "${OUTPUT_COMMON_INCLUDE_DIR}/converter")
endforeach()

# Create export target
add_library(PoSDK::pomvg_converter ALIAS pomvg_converter)
