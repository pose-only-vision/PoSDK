# ==============================================================================
# Copyright (c) 2024 PoSDK Project
# 文件: CMakeLists.txt
# 描述: PoMVG项目的主CMake构建配置文件
# 作者: Qi Cai
# 日期: 2024-01
# 功能: 配置项目的构建系统,设置编译选项,管理依赖关系
# ==============================================================================

# ------------------------------------------------------------------------------
# 基本配置
# ------------------------------------------------------------------------------
# 设置最低CMake版本要求
cmake_minimum_required(VERSION 3.15)

# 设置项目名称、版本号和语言
project(PoSDK
        VERSION 1.0.0.0
        LANGUAGES CXX)

# ------------------------------------------------------------------------------
# C++标准和编译器配置
# ------------------------------------------------------------------------------
# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 强制使用指定的C++标准
set(CMAKE_CXX_EXTENSIONS OFF)        # 禁用编译器特定扩展

# ------------------------------------------------------------------------------
# 构建路径配置
# ------------------------------------------------------------------------------

# 在文件开头添加，强制排除 Anaconda/Conda 路径
# Force exclude Anaconda/Conda paths at the beginning
# 注意：CMake 的 find_package 会搜索多个路径，需要显式排除
# Note: CMake's find_package searches multiple paths, need explicit exclusion

# 获取用户主目录（支持 ~ 展开）
# Get user home directory (support ~ expansion)
if(UNIX)
    set(USER_HOME $ENV{HOME})
elseif(WIN32)
    set(USER_HOME $ENV{USERPROFILE})
else()
    set(USER_HOME "")
endif()

# 构建 Anaconda/Conda 路径列表（如果存在）
# Build Anaconda/Conda path list (if exists)
set(ANACONDA_PATHS_TO_EXCLUDE "")
if(USER_HOME)
    set(POTENTIAL_ANACONDA_DIRS
        "${USER_HOME}/anaconda3"
        "${USER_HOME}/anaconda2"
        "${USER_HOME}/miniconda3"
        "${USER_HOME}/miniconda2"
    )
    
    # 添加 CONDA_PREFIX（如果存在且不重复）
    # Add CONDA_PREFIX (if exists and not duplicate)
    if(DEFINED ENV{CONDA_PREFIX} AND NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
        set(CONDA_PREFIX_PATH "$ENV{CONDA_PREFIX}")
        # 检查是否已在列表中（避免重复）
        # Check if already in list (avoid duplicates)
        list(FIND POTENTIAL_ANACONDA_DIRS "${CONDA_PREFIX_PATH}" CONDA_INDEX)
        if(CONDA_INDEX EQUAL -1)
            list(APPEND POTENTIAL_ANACONDA_DIRS "${CONDA_PREFIX_PATH}")
        endif()
    endif()
    
    foreach(ANACONDA_DIR ${POTENTIAL_ANACONDA_DIRS})
        if(ANACONDA_DIR AND EXISTS "${ANACONDA_DIR}")
            # 使用 list(APPEND) 并去重
            # Use list(APPEND) and remove duplicates
            list(APPEND ANACONDA_PATHS_TO_EXCLUDE "${ANACONDA_DIR}")
            list(APPEND ANACONDA_PATHS_TO_EXCLUDE "${ANACONDA_DIR}/lib")
            list(APPEND ANACONDA_PATHS_TO_EXCLUDE "${ANACONDA_DIR}/include")
            list(APPEND ANACONDA_PATHS_TO_EXCLUDE "${ANACONDA_DIR}/lib/cmake")
        endif()
    endforeach()
    
    # 去除重复路径
    # Remove duplicate paths
    if(ANACONDA_PATHS_TO_EXCLUDE)
        list(REMOVE_DUPLICATES ANACONDA_PATHS_TO_EXCLUDE)
    endif()
endif()

# 从 CMAKE_PREFIX_PATH 中移除 Anaconda 路径（如果存在）
# Remove Anaconda paths from CMAKE_PREFIX_PATH (if exists)
if(CMAKE_PREFIX_PATH AND ANACONDA_PATHS_TO_EXCLUDE)
    foreach(ANACONDA_PATH ${ANACONDA_PATHS_TO_EXCLUDE})
        list(REMOVE_ITEM CMAKE_PREFIX_PATH "${ANACONDA_PATH}")
    endforeach()
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (Anaconda excluded)" FORCE)
endif()

# 从 CMAKE_LIBRARY_PATH 中移除 Anaconda 路径（如果存在）
# Remove Anaconda paths from CMAKE_LIBRARY_PATH (if exists)
if(CMAKE_LIBRARY_PATH AND ANACONDA_PATHS_TO_EXCLUDE)
    foreach(ANACONDA_PATH ${ANACONDA_PATHS_TO_EXCLUDE})
        list(REMOVE_ITEM CMAKE_LIBRARY_PATH "${ANACONDA_PATH}")
    endforeach()
    set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "CMake library path (Anaconda excluded)" FORCE)
endif()

# 输出排除的路径（用于调试）
# Output excluded paths (for debugging)
if(ANACONDA_PATHS_TO_EXCLUDE)
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    message(STATUS "PoSDK: Excluding Anaconda/Conda paths from CMake search:")
    foreach(EXCLUDED_PATH ${ANACONDA_PATHS_TO_EXCLUDE})
        message(STATUS "  ✗ Excluded: ${EXCLUDED_PATH}")
    endforeach()
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
endif()

# 设置基础输出目录
set(OUTPUT_BASE_DIR "${CMAKE_BINARY_DIR}/output")

# 设置各类文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/lib")

# 设置其他输出目录
set(OUTPUT_BIN_DIR "${OUTPUT_BASE_DIR}/bin")
set(OUTPUT_LIB_DIR "${OUTPUT_BASE_DIR}/lib")

set(OUTPUT_INCLUDE_DIR "${OUTPUT_BASE_DIR}/include")
set(OUTPUT_PO_CORE_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/po_core")

# PoSDK 项目版本头文件生成
set(OUTPUT_POMVG_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/pomvg")
file(MAKE_DIRECTORY ${OUTPUT_POMVG_INCLUDE_DIR})
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${OUTPUT_POMVG_INCLUDE_DIR}/version.hpp"
    @ONLY
)
# 子项目可以通过 target_include_directories(target PRIVATE ${OUTPUT_POMVG_INCLUDE_DIR}) 来包含此头文件
# 或者，如果 pomvg_common 已经将 OUTPUT_INCLUDE_DIR 作为 PUBLIC 包含目录，
# 那么链接了 pomvg_common 的目标将能够通过 #include "pomvg/version.hpp" 来使用它。

set(CONFIG_DIR "${OUTPUT_BASE_DIR}/configs")
set(METHOD_CONFIG_DIR "${CONFIG_DIR}/methods")

# 新增 common 相关目录
set(OUTPUT_COMMON_DIR "${OUTPUT_BASE_DIR}/common")
set(OUTPUT_COMMON_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/common")

# 新增 Python 脚本目录
set(OUTPUT_PYTHON_DIR "${OUTPUT_BASE_DIR}/python")

# 创建必要的目录
file(MAKE_DIRECTORY ${OUTPUT_BIN_DIR})
file(MAKE_DIRECTORY ${OUTPUT_LIB_DIR})

file(MAKE_DIRECTORY ${OUTPUT_PO_CORE_INCLUDE_DIR})

file(MAKE_DIRECTORY ${CONFIG_DIR})
file(MAKE_DIRECTORY ${METHOD_CONFIG_DIR})
# 新增 common 目录创建
file(MAKE_DIRECTORY ${OUTPUT_COMMON_DIR})
file(MAKE_DIRECTORY ${OUTPUT_COMMON_INCLUDE_DIR})
# 新增 Python 目录创建
file(MAKE_DIRECTORY ${OUTPUT_PYTHON_DIR})

#  新增测试数据目录
set(OUTPUT_TESTS_DIR  "${OUTPUT_BASE_DIR}/tests")
file(MAKE_DIRECTORY ${OUTPUT_TESTS_DIR})

# 测试数据源目录
set(SOURCE_TESTS_DIR  "${CMAKE_SOURCE_DIR}/tests")

# 添加 cmake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ------------------------------------------------------------------------------
# 平台特定配置
# ------------------------------------------------------------------------------
if(APPLE)
    # macOS平台配置
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        # Apple Silicon (M1/M2) 架构优化
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1")
    endif()
elseif(WIN32)
    # Windows平台配置
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)  # 启用符号导出
    add_definitions(-D_WIN32_WINNT=0x0601)    # 设置Windows目标平台版本(Win7)
else()
    # Linux平台配置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")  # 启用本地CPU优化
    
    # 设置链接器标志以优先系统库（避免 Anaconda 库冲突）
    # Set linker flags to prioritize system libraries (avoid Anaconda library conflicts)
    # 注意：这些设置会在构建时生效，但运行时仍可能受到 LD_LIBRARY_PATH 影响
    # Note: These settings take effect at build time, but runtime may still be affected by LD_LIBRARY_PATH
    
    # 优先系统库路径（用于链接时库搜索）
    # Prioritize system library paths (for link-time library search)
    set(SYSTEM_LIB_PATHS
        "/usr/lib/x86_64-linux-gnu"
        "/usr/lib"
        "/lib/x86_64-linux-gnu"
        "/lib"
    )
    
    # 构建链接器搜索路径参数
    # Build linker search path arguments
    set(LINKER_LIBRARY_PATH_FLAGS "")
    foreach(SYSTEM_PATH ${SYSTEM_LIB_PATHS})
        if(EXISTS "${SYSTEM_PATH}")
            set(LINKER_LIBRARY_PATH_FLAGS "${LINKER_LIBRARY_PATH_FLAGS} -Wl,-rpath-link,${SYSTEM_PATH}")
endif()
    endforeach()
    
    # 添加链接器标志：只链接需要的库，优先系统库路径
    # Add linker flags: only link needed libraries, prioritize system library paths
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed ${LINKER_LIBRARY_PATH_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed ${LINKER_LIBRARY_PATH_FLAGS}")
    
    # 设置系统库搜索路径优先级（用于 CMake 的库查找）
    # Set system library search path priority (for CMake library finding)
    # 注意：这不会完全消除警告，但会确保链接时优先使用系统库
    # Note: This won't completely eliminate warnings, but ensures system libraries are prioritized during linking
    list(APPEND CMAKE_SYSTEM_LIBRARY_PATH ${SYSTEM_LIB_PATHS})
endif()


# ------------------------------------------------------------------------------
# Abseil库配置
# ------------------------------------------------------------------------------

set(ABSEIL_MIN_VERSION "20250814")

# 优先查找项目内部安装的 Abseil
set(LOCAL_ABSEIL_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/abseil-cpp/install_local")
set(LOCAL_ABSEIL_CMAKE_DIR "${LOCAL_ABSEIL_INSTALL_DIR}/lib/cmake/absl")

if(EXISTS "${LOCAL_ABSEIL_CMAKE_DIR}/abslConfig.cmake")
    message(STATUS "Attempting to use local Abseil installation from: ${LOCAL_ABSEIL_INSTALL_DIR}")

    # 强制使用本地安装的Abseil，避免系统版本干扰
    set(absl_DIR "${LOCAL_ABSEIL_CMAKE_DIR}")
    message(STATUS "Explicitly setting absl_DIR to: ${absl_DIR}")

    # 也添加到干净的CMAKE_PREFIX_PATH（排除Anaconda）作为备选
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_ABSEIL_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "Added local Abseil to clean CMAKE_PREFIX_PATH: ${LOCAL_ABSEIL_INSTALL_DIR}")
else()
    message(STATUS "Local Abseil installation not found at ${LOCAL_ABSEIL_CMAKE_DIR}/abslConfig.cmake")
    message(STATUS "Please run install_absl.sh to build and install Abseil locally.")
    message(STATUS "Will attempt to use system Abseil installation...")
endif()

# find_package 现在会优先使用 absl_DIR 指定的路径
find_package(absl CONFIG) # 移除 REQUIRED，后续做检查

# 检查版本
if(absl_FOUND)
    message(STATUS "Found Abseil version: ${absl_VERSION} at ${absl_DIR}")
    if(absl_VERSION VERSION_LESS ABSEIL_MIN_VERSION)
        message(FATAL_ERROR "Found Abseil version ${absl_VERSION} at ${absl_DIR} is older than required version ${ABSEIL_MIN_VERSION}. Please ensure the correct version is built in dependencies/abseil-cpp.")
    else()
        message(STATUS "Using Abseil version: ${absl_VERSION} from ${absl_DIR}")
    endif()
else()
    # 如果没有找到本地安装的，并且也没有设置其他搜索路径，这里就会报错
    message(FATAL_ERROR "Abseil not found. Please ensure it is built and installed correctly in dependencies/abseil-cpp/install_local by running install_absl.sh, or provide a valid system installation.")
endif()

# ------------------------------------------------------------------------------
# Protocol Buffers库配置
# ------------------------------------------------------------------------------

set(PROTOBUF_MIN_VERSION "33.0")

# 优先查找项目内部安装的 Protobuf
set(LOCAL_PROTOBUF_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/protobuf/install_local")
set(LOCAL_PROTOBUF_CMAKE_DIR "${LOCAL_PROTOBUF_INSTALL_DIR}/lib/cmake/protobuf")

if(EXISTS "${LOCAL_PROTOBUF_CMAKE_DIR}/protobuf-config.cmake")
    message(STATUS "Attempting to use local Protobuf installation from: ${LOCAL_PROTOBUF_INSTALL_DIR}")

    # 强制使用本地安装的Protobuf，避免系统版本干扰
    set(Protobuf_DIR "${LOCAL_PROTOBUF_CMAKE_DIR}")
    message(STATUS "Explicitly setting Protobuf_DIR to: ${Protobuf_DIR}")

    # 也添加到干净的CMAKE_PREFIX_PATH（排除Anaconda）作为备选
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_PROTOBUF_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "Added local Protobuf to clean CMAKE_PREFIX_PATH: ${LOCAL_PROTOBUF_INSTALL_DIR}")
else()
    message(STATUS "Local Protobuf installation not found at ${LOCAL_PROTOBUF_CMAKE_DIR}/protobuf-config.cmake")
    message(STATUS "Please run install_protobuf.sh to build and install Protobuf locally.")
    message(STATUS "Will attempt to use system Protobuf installation...")
endif()

# find_package 现在会优先使用 Protobuf_DIR 指定的路径
find_package(Protobuf CONFIG) # 移除 REQUIRED，后续做检查

# 检查版本
if(Protobuf_FOUND)
    message(STATUS "Found Protobuf version: ${Protobuf_VERSION} at ${Protobuf_DIR}")
    if(Protobuf_VERSION VERSION_LESS PROTOBUF_MIN_VERSION)
        message(FATAL_ERROR "Found Protobuf version ${Protobuf_VERSION} at ${Protobuf_DIR} is older than required version ${PROTOBUF_MIN_VERSION}. Please ensure the correct version is built in dependencies/protobuf.")
    else()
        message(STATUS "Using Protobuf version: ${Protobuf_VERSION} from ${Protobuf_DIR}")
    endif()
else()
    # 如果没有找到本地安装的，尝试使用系统安装，但如果仍然失败则报错
    message(FATAL_ERROR "Protobuf not found. Please ensure it is built and installed correctly in dependencies/protobuf/install_local by running install_protobuf.sh, or provide a valid system installation.")
endif()


# ------------------------------------------------------------------------------
# Boost 配置 - 必须在 find_package(po_core) 之前
# Boost Configuration - Must be done before find_package(po_core)
# ------------------------------------------------------------------------------
# 清理 CMAKE_PREFIX_PATH 以排除 Anaconda | Clean CMAKE_PREFIX_PATH to exclude Anaconda
# 注意：CMake 不能直接修改环境变量，但我们可以显式构建干净的 CMAKE_PREFIX_PATH
# Note: CMake cannot directly modify environment variables, but we can explicitly build a clean CMAKE_PREFIX_PATH
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "PoSDK: Cleaning CMAKE_PREFIX_PATH and CMAKE_LIBRARY_PATH to exclude Anaconda paths")
message(STATUS "Original CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "Original CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")

# 保存原始 CMAKE_PREFIX_PATH 用于调试
# Save original CMAKE_PREFIX_PATH for debugging
set(ORIGINAL_CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")

# 从 CMAKE_PREFIX_PATH 中移除 Anaconda/Conda 路径
# Remove Anaconda/Conda paths from CMAKE_PREFIX_PATH
set(CLEAN_CMAKE_PREFIX_PATH "")
if(CMAKE_PREFIX_PATH)
    foreach(PATH_ITEM ${CMAKE_PREFIX_PATH})
        string(TOLOWER "${PATH_ITEM}" PATH_ITEM_LOWER)
        if(NOT PATH_ITEM_LOWER MATCHES "anaconda" AND NOT PATH_ITEM_LOWER MATCHES "conda")
            list(APPEND CLEAN_CMAKE_PREFIX_PATH "${PATH_ITEM}")
        else()
            message(STATUS "  Removed Anaconda/Conda path from CMAKE_PREFIX_PATH: ${PATH_ITEM}")
        endif()
    endforeach()
endif()

# 从 CMAKE_LIBRARY_PATH 中移除 Anaconda/Conda 路径
# Remove Anaconda/Conda paths from CMAKE_LIBRARY_PATH
set(CLEAN_CMAKE_LIBRARY_PATH "")
if(CMAKE_LIBRARY_PATH)
    foreach(PATH_ITEM ${CMAKE_LIBRARY_PATH})
        string(TOLOWER "${PATH_ITEM}" PATH_ITEM_LOWER)
        if(NOT PATH_ITEM_LOWER MATCHES "anaconda" AND NOT PATH_ITEM_LOWER MATCHES "conda")
            list(APPEND CLEAN_CMAKE_LIBRARY_PATH "${PATH_ITEM}")
        else()
            message(STATUS "  Removed Anaconda/Conda path from CMAKE_LIBRARY_PATH: ${PATH_ITEM}")
        endif()
    endforeach()
endif()

# 设置清理后的 CMAKE_PREFIX_PATH（只包含本地依赖路径）
# Set cleaned CMAKE_PREFIX_PATH (only include local dependency paths)
# 本地依赖将显式添加到 CLEAN_CMAKE_PREFIX_PATH
# Local dependencies will be explicitly added to CLEAN_CMAKE_PREFIX_PATH
set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (Anaconda paths excluded)" FORCE)

# 设置清理后的 CMAKE_LIBRARY_PATH（只包含系统库路径）
# Set cleaned CMAKE_LIBRARY_PATH (only include system library paths)
set(CMAKE_LIBRARY_PATH "${CLEAN_CMAKE_LIBRARY_PATH}" CACHE PATH "CMake library path (Anaconda paths excluded)" FORCE)

message(STATUS "Cleaned CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "Cleaned CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Note: CMake warnings about library conflicts are informational.")
message(STATUS "      They indicate potential runtime issues, not build failures.")
message(STATUS "      To minimize them, run cmake with clean environment:")
message(STATUS "      unset LD_LIBRARY_PATH && export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

# 设置CMake策略以支持Boost查找 | Set CMake policies for Boost finding
# CMP0074: 允许 find_package 使用 <PackageName>_ROOT 变量（如 Boost_ROOT）
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)  # 启用Boost_ROOT变量支持 | Enable Boost_ROOT variable support
    message(STATUS "PoSDK: Set CMP0074=NEW (Boost_ROOT variable support)")
endif()

# CMP0144: 注意：CGAL需要OLD策略来使用BOOST_ROOT
# CMP0144: Note: CGAL needs OLD policy to use BOOST_ROOT
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 OLD)  # CGAL兼容性：允许使用BOOST_ROOT | CGAL compatibility: allow BOOST_ROOT
    message(STATUS "PoSDK: Set CMP0144=OLD (CGAL compatibility with BOOST_ROOT)")
endif()

# CMP0167: CGAL需要OLD策略来使用旧的Boost查找方式
# CMP0167: CGAL needs OLD policy to use old Boost finding method
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)  # CGAL兼容性：使用旧的Boost查找方式 | CGAL compatibility: use old Boost finding method
    message(STATUS "PoSDK: Set CMP0167=OLD (CGAL compatibility)")
endif()

# 优先查找本地构建的Boost | Prefer local built Boost
set(BOOST_MIN_VERSION "1.85.0")
set(LOCAL_BOOST_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/boost_1_85_0/install_local")

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Checking for local Boost installation:")
message(STATUS "  Expected directory: ${LOCAL_BOOST_INSTALL_DIR}")
message(STATUS "  Expected version file: ${LOCAL_BOOST_INSTALL_DIR}/include/boost/version.hpp")

if(EXISTS "${LOCAL_BOOST_INSTALL_DIR}/include/boost/version.hpp")
    message(STATUS "  ✓ Local Boost ${BOOST_MIN_VERSION} found")
    
    # 强制使用本地Boost，避免系统版本干扰 | Force local Boost, avoid system version interference
    # 1. 设置Boost路径变量 | Set Boost path variables
    set(BOOST_ROOT "${LOCAL_BOOST_INSTALL_DIR}" CACHE PATH "Boost root directory" FORCE)
    set(Boost_DIR "${LOCAL_BOOST_INSTALL_DIR}/lib/cmake/Boost-${BOOST_MIN_VERSION}" CACHE PATH "Boost CMake config directory" FORCE)
    set(BOOST_INCLUDEDIR "${LOCAL_BOOST_INSTALL_DIR}/include" CACHE PATH "Boost include directory" FORCE)
    set(BOOST_LIBRARYDIR "${LOCAL_BOOST_INSTALL_DIR}/lib" CACHE PATH "Boost library directory" FORCE)
    set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not search system for Boost" FORCE)
    
    message(STATUS "PoSDK: Set BOOST_ROOT=${BOOST_ROOT}")
    message(STATUS "PoSDK: Set Boost_DIR=${Boost_DIR}")
    message(STATUS "PoSDK: Set Boost_NO_SYSTEM_PATHS=ON")
    
    # 2. 显式构建干净的CMAKE_PREFIX_PATH（只包含本地依赖，排除Anaconda）
    # Build clean CMAKE_PREFIX_PATH (only local dependencies, exclude Anaconda)
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_BOOST_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "PoSDK: Added local Boost to clean CMAKE_PREFIX_PATH: ${LOCAL_BOOST_INSTALL_DIR}")
    message(STATUS "PoSDK: Clean CMAKE_PREFIX_PATH (Anaconda excluded): ${CMAKE_PREFIX_PATH}")
    
    # 3. 使用CONFIG模式和显式路径查找Boost，严格限制搜索路径以避免Anaconda干扰
    # Use CONFIG mode with explicit path, strictly limit search paths to avoid Anaconda interference
    find_package(Boost ${BOOST_MIN_VERSION} REQUIRED CONFIG
        PATHS "${LOCAL_BOOST_INSTALL_DIR}"
        PATH_SUFFIXES "lib/cmake/Boost-${BOOST_MIN_VERSION}" "lib/cmake/Boost"
        NO_DEFAULT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_PACKAGE_REGISTRY
        NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
    )
    
    if(Boost_FOUND)
        message(STATUS "PoSDK: ✓ Using local Boost version: ${Boost_VERSION}")
        message(STATUS "PoSDK: Boost include dirs: ${Boost_INCLUDE_DIRS}")
        message(STATUS "PoSDK: Boost library dirs: ${Boost_LIBRARY_DIRS}")
        
        # 锁定Boost路径，确保传播给po_core-config.cmake | Lock Boost paths for po_core-config.cmake
        set(BOOST_ROOT "${LOCAL_BOOST_INSTALL_DIR}" CACHE PATH "Boost root directory" FORCE)
        set(Boost_DIR "${LOCAL_BOOST_INSTALL_DIR}/lib/cmake/Boost-${Boost_VERSION}" CACHE PATH "Boost CMake config directory" FORCE)
        message(STATUS "PoSDK: Boost paths locked (propagated to po_core)")
    else()
        message(FATAL_ERROR "Failed to load local Boost from ${LOCAL_BOOST_INSTALL_DIR}")
    endif()
else()
    message(STATUS "  ✗ Local Boost not found")
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    message(FATAL_ERROR 
        "Local Boost ${BOOST_MIN_VERSION} installation not found at:\n"
        "  ${LOCAL_BOOST_INSTALL_DIR}\n"
        "\n"
        "Boost ${BOOST_MIN_VERSION}+ is REQUIRED for po_core compatibility.\n"
        "System Boost (if available) may have incompatible version/ABI.\n"
        "\n"
        "To install Boost ${BOOST_MIN_VERSION} locally:\n"
        "  cd dependencies\n"
        "  ./install_boost.sh\n"
        "\n"
        "Or run the full dependency installation:\n"
        "  ./install.sh\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    )
endif()
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

# ------------------------------------------------------------------------------
# Ceres Solver 配置 - 必须在 find_package(po_core) 之前
# ------------------------------------------------------------------------------
# 优先查找本地构建的Ceres Solver | Prefer local built Ceres Solver
# Check for local Ceres installation in dependencies directory
set(CERES_MIN_VERSION "2.2.0")
set(LOCAL_CERES_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/ceres-solver-2.2.0/install_local")
set(LOCAL_CERES_CMAKE_DIR "${LOCAL_CERES_INSTALL_DIR}/lib/cmake/Ceres")

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Checking for local Ceres installation:")
message(STATUS "  Expected directory: ${LOCAL_CERES_INSTALL_DIR}")
message(STATUS "  Expected CMake config: ${LOCAL_CERES_CMAKE_DIR}/CeresConfig.cmake")
if(EXISTS "${LOCAL_CERES_CMAKE_DIR}/CeresConfig.cmake")
    message(STATUS "  Config file status: ✓ EXISTS")
else()
    message(STATUS "  Config file status: ✗ NOT FOUND")
endif()
if(EXISTS "${LOCAL_CERES_INSTALL_DIR}")
    message(STATUS "  ✓ Install directory exists")
    if(EXISTS "${LOCAL_CERES_CMAKE_DIR}")
        message(STATUS "  ✓ CMake directory exists")
        # List files in CMake directory
        file(GLOB CERES_CMAKE_FILES "${LOCAL_CERES_CMAKE_DIR}/*")
        message(STATUS "  Files in CMake directory:")
        foreach(FILE ${CERES_CMAKE_FILES})
            get_filename_component(FILENAME ${FILE} NAME)
            message(STATUS "    - ${FILENAME}")
        endforeach()
    else()
        message(STATUS "  ✗ CMake directory missing: ${LOCAL_CERES_CMAKE_DIR}")
    endif()
else()
    message(STATUS "  ✗ Install directory missing: ${LOCAL_CERES_INSTALL_DIR}")
endif()
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

if(EXISTS "${LOCAL_CERES_CMAKE_DIR}/CeresConfig.cmake")
    message(STATUS "PoSDK: Found local Ceres installation at: ${LOCAL_CERES_INSTALL_DIR}")
    
    # 强制使用本地 Ceres，避免系统版本干扰
    # 1. 清除可能存在的 Ceres 缓存变量
    unset(Ceres_DIR CACHE)
    unset(Ceres_FOUND CACHE)
    
    # 2. 显式设置 Ceres_DIR
    set(Ceres_DIR "${LOCAL_CERES_CMAKE_DIR}" CACHE PATH "Path to Ceres CMake config" FORCE)
    message(STATUS "PoSDK: Forcing Ceres_DIR to: ${Ceres_DIR}")
    
    # 3. 添加本地 Ceres 到干净的 CMAKE_PREFIX_PATH（排除Anaconda）
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_CERES_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "PoSDK: Added local Ceres to clean CMAKE_PREFIX_PATH: ${LOCAL_CERES_INSTALL_DIR}")
    
    # 4. 使用严格的路径限制，避免Anaconda干扰
    find_package(Ceres ${CERES_MIN_VERSION} REQUIRED CONFIG
        PATHS "${LOCAL_CERES_CMAKE_DIR}"
        NO_DEFAULT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_PACKAGE_REGISTRY
        NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
    )
    
    if(Ceres_FOUND)
        message(STATUS "PoSDK: ✓ Using local Ceres version: ${Ceres_VERSION} from ${Ceres_DIR}")
        if(Ceres_VERSION VERSION_LESS CERES_MIN_VERSION)
            message(FATAL_ERROR "Local Ceres version ${Ceres_VERSION} is older than required ${CERES_MIN_VERSION}")
        endif()
        
        # 再次强制设置 Ceres_DIR，确保传播给 po_core-config.cmake 的 find_dependency
        # 这样 po_core-config.cmake 中的 find_dependency(Ceres REQUIRED) 会使用这个路径
        set(Ceres_DIR "${LOCAL_CERES_CMAKE_DIR}" CACHE PATH "Path to Ceres CMake config" FORCE)
        message(STATUS "PoSDK: Ceres_DIR locked to: ${Ceres_DIR} (propagated to po_core)")
    else()
        message(FATAL_ERROR "Failed to load local Ceres from ${LOCAL_CERES_CMAKE_DIR}")
    endif()
else()
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    message(FATAL_ERROR 
        "Local Ceres ${CERES_MIN_VERSION} installation not found at:\n"
        "  ${LOCAL_CERES_CMAKE_DIR}\n"
        "\n"
        "Ceres Solver ${CERES_MIN_VERSION}+ is REQUIRED for po_core compatibility.\n"
        "System Ceres (if available) has incompatible version/ABI.\n"
        "\n"
        "To install Ceres ${CERES_MIN_VERSION} locally:\n"
        "  cd dependencies\n"
        "  ./install_ceres.sh\n"
        "\n"
        "Or run the full dependency installation:\n"
        "  ./install.sh\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    )
endif()


# ------------------------------------------------------------------------------
# OpenCV库配置 - 必须在 find_package(po_core) 之前
# ------------------------------------------------------------------------------
# 重要：po_core 依赖 OpenCV，必须先找到 OpenCV 才能正确配置 po_core 的依赖传播
# Important: po_core depends on OpenCV, must find OpenCV first for correct dependency propagation

# 优先查找项目内部安装的 OpenCV (标准模块)
# 统一使用 install_local 路径结构
set(LOCAL_OPENCV_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/opencv/install_local")

# 如果 PoSDK 本地没有，尝试使用 po_core 的 OpenCV
if(NOT EXISTS "${LOCAL_OPENCV_INSTALL_DIR}/include/opencv4/opencv2/opencv.hpp")
    set(PO_CORE_OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/po_core_lib/dependencies/opencv/install_local")
    if(EXISTS "${PO_CORE_OPENCV_DIR}/include/opencv4/opencv2/opencv.hpp")
        set(LOCAL_OPENCV_INSTALL_DIR "${PO_CORE_OPENCV_DIR}")
        message(STATUS "PoSDK local OpenCV not found, using po_core's OpenCV: ${LOCAL_OPENCV_INSTALL_DIR}")
    endif()
endif()

# 保存 OpenCV 安装路径（用于后续 RPATH 设置）
# Save OpenCV installation path (for later RPATH setup)
set(OPENCV_INSTALL_PREFIX_FOR_RPATH "")

if(EXISTS "${LOCAL_OPENCV_INSTALL_DIR}/include/opencv4/opencv2/opencv.hpp")
    message(STATUS "Attempting to use local OpenCV installation from: ${LOCAL_OPENCV_INSTALL_DIR}")
    
    # 保存安装路径用于 RPATH 设置
    # Save installation path for RPATH setup
    set(OPENCV_INSTALL_PREFIX_FOR_RPATH "${LOCAL_OPENCV_INSTALL_DIR}")
    
    # 强制使用本地安装的OpenCV，避免系统版本和Anaconda版本干扰
    # 将本地安装目录添加到干净的 CMAKE_PREFIX_PATH（排除Anaconda）
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_OPENCV_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "Added local OpenCV to clean CMAKE_PREFIX_PATH: ${LOCAL_OPENCV_INSTALL_DIR}")
    message(STATUS "Clean CMAKE_PREFIX_PATH (Anaconda excluded): ${CMAKE_PREFIX_PATH}")
    
    # 如果存在CMake配置文件，也设置OpenCV_DIR
    if(EXISTS "${LOCAL_OPENCV_INSTALL_DIR}/lib/cmake/opencv4/OpenCVConfig.cmake")
        set(OpenCV_DIR "${LOCAL_OPENCV_INSTALL_DIR}/lib/cmake/opencv4")
        message(STATUS "Explicitly setting OpenCV_DIR to: ${OpenCV_DIR}")
    endif()
else()
    message(STATUS "Local OpenCV installation not found at ${LOCAL_OPENCV_INSTALL_DIR}")
    message(STATUS "Please run install_opencv.sh to build and install OpenCV locally.")
    message(STATUS "Expected path: dependencies/opencv/install_local")
    message(STATUS "Will attempt to use system OpenCV installation...")
endif()

find_package(Eigen3 3.3 REQUIRED NO_MODULE)  # Eigen3线性代数库

# 查找 OpenCV，使用严格的路径限制以避免 Anaconda 干扰
# Find OpenCV with strict path restrictions to avoid Anaconda interference
find_package(OpenCV REQUIRED
    NO_CMAKE_SYSTEM_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)

# 验证 OpenCV 版本和模块
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")

# 诊断：检查 OpenCV 来源（本地安装 vs 系统）
# Diagnosis: Check OpenCV source (local installation vs system)
# 优先使用 OpenCV_DIR 来判断来源（最可靠）
# Prefer using OpenCV_DIR to determine source (most reliable)
set(OPENCV_SOURCE_DETECTED FALSE)

if(OpenCV_DIR)
    message(STATUS "Diagnosing OpenCV source from OpenCV_DIR: ${OpenCV_DIR}")
    
    # 从 OpenCV_DIR 推断安装位置
    # Infer installation location from OpenCV_DIR
    if(OpenCV_DIR MATCHES "install_local" OR OpenCV_DIR MATCHES "dependencies/opencv")
        message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        message(STATUS "✓ OpenCV Configuration: Using LOCAL installation")
        message(STATUS "  Version: ${OpenCV_VERSION}")
        message(STATUS "  CMake config: ${OpenCV_DIR}")
        message(STATUS "  Include dirs: ${OpenCV_INCLUDE_DIRS}")
        # 从 OpenCV_DIR 推断库目录 (OpenCV_DIR 通常是 .../lib/cmake/opencv4)
        # Infer library directory from OpenCV_DIR (usually .../lib/cmake/opencv4)
        get_filename_component(OPENCV_LIB_DIR "${OpenCV_DIR}/../.." ABSOLUTE)
        if(EXISTS "${OPENCV_LIB_DIR}")
            message(STATUS "  Library directory: ${OPENCV_LIB_DIR}")
        endif()
        message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        set(OPENCV_SOURCE_DETECTED TRUE)
    elseif(OpenCV_DIR MATCHES "/usr" OR OpenCV_DIR MATCHES "/opt")
        message(WARNING "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        message(WARNING "⚠ OpenCV Configuration: Using SYSTEM installation")
        message(WARNING "  This may cause compatibility issues with local dependencies")
        message(WARNING "  Consider building OpenCV locally with install_opencv.sh")
        message(WARNING "  CMake config: ${OpenCV_DIR}")
        message(WARNING "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        set(OPENCV_SOURCE_DETECTED TRUE)
    elseif(OpenCV_DIR MATCHES "anaconda" OR OpenCV_DIR MATCHES "conda")
        message(FATAL_ERROR "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        message(FATAL_ERROR "✗ OpenCV Configuration: Anaconda/Conda OpenCV detected!")
        message(FATAL_ERROR "  This will cause serious build/runtime issues")
        message(FATAL_ERROR "  CMake config: ${OpenCV_DIR}")
        message(FATAL_ERROR "  Please build OpenCV locally with install_opencv.sh")
        message(FATAL_ERROR "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    endif()
endif()

# 如果从 OpenCV_DIR 无法判断，尝试从 include 目录判断
# If cannot determine from OpenCV_DIR, try from include directories
if(NOT OPENCV_SOURCE_DETECTED AND OpenCV_INCLUDE_DIRS)
    message(STATUS "Diagnosing OpenCV source from include dirs: ${OpenCV_INCLUDE_DIRS}")
    
    if(OpenCV_INCLUDE_DIRS MATCHES "install_local" OR OpenCV_INCLUDE_DIRS MATCHES "dependencies/opencv")
        message(STATUS "✓ OpenCV source: LOCAL installation (from include dirs)")
        set(OPENCV_SOURCE_DETECTED TRUE)
    elseif(OpenCV_INCLUDE_DIRS MATCHES "/usr")
        message(WARNING "⚠ OpenCV source: SYSTEM installation (from include dirs)")
        set(OPENCV_SOURCE_DETECTED TRUE)
    elseif(OpenCV_INCLUDE_DIRS MATCHES "anaconda" OR OpenCV_INCLUDE_DIRS MATCHES "conda")
        message(FATAL_ERROR "✗ OpenCV source: Anaconda/Conda (from include dirs) - This will cause issues!")
    endif()
endif()

if(NOT OPENCV_SOURCE_DETECTED)
    message(WARNING "⚠ Cannot determine OpenCV source from OpenCV_DIR or include dirs")
    message(WARNING "  OpenCV_DIR: ${OpenCV_DIR}")
    message(WARNING "  Include dirs: ${OpenCV_INCLUDE_DIRS}")
endif()

message(STATUS "Note: This build uses standard OpenCV modules only (no opencv_contrib)")

# 保存 OpenCV 安装路径（find_package 后确保路径可用）
# Save OpenCV installation path (ensure path is available after find_package)
if(OPENCV_INSTALL_PREFIX_FOR_RPATH AND EXISTS "${OPENCV_INSTALL_PREFIX_FOR_RPATH}")
    # 使用之前保存的路径（优先）
    # Use previously saved path (priority)
    set(OPENCV_INSTALL_PREFIX "${OPENCV_INSTALL_PREFIX_FOR_RPATH}")
elseif(OpenCV_DIR)
    # 从 OpenCV_DIR 提取安装前缀
    # Extract installation prefix from OpenCV_DIR
    get_filename_component(OPENCV_INSTALL_PREFIX "${OpenCV_DIR}/../.." ABSOLUTE)
elseif(OpenCV_LIBS)
    # 从 OpenCV_LIBS 中提取库路径
    # Extract library path from OpenCV_LIBS
    list(GET OpenCV_LIBS 0 OPENCV_FIRST_LIB)
    if(OPENCV_FIRST_LIB)
        get_filename_component(OPENCV_FIRST_LIB_PATH "${OPENCV_FIRST_LIB}" DIRECTORY)
        get_filename_component(OPENCV_INSTALL_PREFIX "${OPENCV_FIRST_LIB_PATH}/.." ABSOLUTE)
    endif()
elseif(EXISTS "${LOCAL_OPENCV_INSTALL_DIR}")
    # 回退到已知的本地安装目录
    # Fallback to known local installation directory
    set(OPENCV_INSTALL_PREFIX "${LOCAL_OPENCV_INSTALL_DIR}")
endif()

# ------------------------------------------------------------------------------
# 设置 RPATH 以支持运行时库查找 | Set RPATH for runtime library search
# ------------------------------------------------------------------------------
# 确保可执行文件能在运行时找到 OpenCV 和其他依赖库
# Ensure executables can find OpenCV and other dependency libraries at runtime
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# 添加 OpenCV 库路径到 RPATH | Add OpenCV library path to RPATH
if(OPENCV_INSTALL_PREFIX AND EXISTS "${OPENCV_INSTALL_PREFIX}")
    set(OPENCV_LIB_DIR "${OPENCV_INSTALL_PREFIX}/lib")
    if(EXISTS "${OPENCV_LIB_DIR}")
        message(STATUS "Adding OpenCV library path to RPATH: ${OPENCV_LIB_DIR}")
        list(APPEND CMAKE_INSTALL_RPATH "${OPENCV_LIB_DIR}")
        list(APPEND CMAKE_BUILD_RPATH "${OPENCV_LIB_DIR}")
    else()
        message(WARNING "OpenCV library directory not found: ${OPENCV_LIB_DIR}")
    endif()
else()
    message(WARNING "OpenCV installation prefix not found")
    message(WARNING "  OpenCV_DIR: ${OpenCV_DIR}")
    message(WARNING "  OpenCV_LIBS: ${OpenCV_LIBS}")
    message(WARNING "  LOCAL_OPENCV_INSTALL_DIR: ${LOCAL_OPENCV_INSTALL_DIR}")
endif()

# 添加 po_core 库路径到 RPATH | Add po_core library path to RPATH
if(po_core_external_folder)
    set(PO_CORE_LIB_DIR "${po_core_external_folder}/lib")
    if(EXISTS "${PO_CORE_LIB_DIR}")
        message(STATUS "Adding po_core library path to RPATH: ${PO_CORE_LIB_DIR}")
        list(APPEND CMAKE_INSTALL_RPATH "${PO_CORE_LIB_DIR}")
        list(APPEND CMAKE_BUILD_RPATH "${PO_CORE_LIB_DIR}")
    endif()
endif()

# 使用 $ORIGIN 相对路径（Linux）或 @executable_path（macOS）
# Use $ORIGIN relative path (Linux) or @executable_path (macOS)
if(APPLE)
    list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../lib")
    list(APPEND CMAKE_BUILD_RPATH "@executable_path/../lib")
else()
    list(APPEND CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
    list(APPEND CMAKE_BUILD_RPATH "\$ORIGIN/../lib")
endif()

message(STATUS "CMAKE_BUILD_RPATH: ${CMAKE_BUILD_RPATH}")
message(STATUS "CMAKE_INSTALL_RPATH: ${CMAKE_INSTALL_RPATH}")


# ------------------------------------------------------------------------------
# po_core 配置选项 | po_core Configuration Options
# ------------------------------------------------------------------------------
option(USE_INTEGRATED_PO_CORE "Use integrated po_core instead of installed version" OFF)

# 主要选项：使用预编译 po_core (推荐)
# Main option: Use precompiled po_core (Recommended)
option(USE_PRECOMPILED_PO_CORE "Use precompiled po_core from dependencies directory" ON)

# po_core路径检测 | po_core path detection
# Define search paths in priority order (highest priority first)
# 定义搜索路径列表（优先级从高到低）
set(PO_CORE_SEARCH_PATHS
    "${CMAKE_SOURCE_DIR}/../po_core/build/po_core_lib"
    "${CMAKE_SOURCE_DIR}/dependencies/po_core_lib"
)

# Search for po_core in PO_CORE_SEARCH_PATHS | 在PO_CORE_SEARCH_PATHS中查找po_core
set(PO_CORE_FOUND FALSE)
foreach(SEARCH_PATH ${PO_CORE_SEARCH_PATHS})
    if(EXISTS "${SEARCH_PATH}/lib/cmake/po_core/po_core-config.cmake")
        set(po_core_external_folder "${SEARCH_PATH}")
        set(PO_CORE_FOUND TRUE)
        message(STATUS "Found po_core at: ${po_core_external_folder}")
        break()
    endif()
endforeach()

# Report error if not found | 如果未找到则报错
if(NOT PO_CORE_FOUND)
    message(FATAL_ERROR "po_core not found at any of the following locations:\n"
        "  ${PO_CORE_SEARCH_PATHS}\n"
        "Please ensure po_core is built and available at one of these paths.")
endif()


# ------------------------------------------------------------------------------
# po_core 集成配置
# ------------------------------------------------------------------------------

if(USE_INTEGRATED_PO_CORE)
    message(STATUS "Using integrated mode, copying po_core files...")
    set(po_core_integrated_root "${OUTPUT_BASE_DIR}") # 定义集成的根目录
    set(po_core_integrated_cmake_dir "${po_core_integrated_root}/lib/cmake/po_core")

    # 检查 po_core_external_folder 是否存在
    if(NOT EXISTS "${po_core_external_folder}")
        message(FATAL_ERROR "External po_core folder not found at: ${po_core_external_folder}\n"
            "Please specify correct po_core_external_folder path")
    endif()

    # 完整复制 po_core_external_folder 到 po_core_integrated_root
    # 注意: 这会复制整个目录结构，包括 include, lib, cmake 等
    # 如果 po_core_external_folder 很大或包含不必要的文件，这可能不是最高效的
    # 但它符合您"将po_core_lib目录内所有文件和子文件夹拷贝"的描述
    message(STATUS "Copying all contents from ${po_core_external_folder} to ${po_core_integrated_root}")
    file(COPY "${po_core_external_folder}/" DESTINATION "${po_core_integrated_root}/") # 确保路径末尾的斜杠
    message(STATUS "Copied po_core to: ${po_core_integrated_root}")

    # 设置 CMAKE_PREFIX_PATH 或 po_core_DIR 以便 find_package 找到集成版本
    # 清除可能存在的旧路径，优先使用集成路径（同时排除Anaconda）
    list(REMOVE_ITEM CLEAN_CMAKE_PREFIX_PATH "${po_core_external_folder}/lib/cmake")
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${po_core_integrated_cmake_dir}") # Prepend 更安全
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    # 或者直接设置 po_core_DIR
    # set(po_core_DIR "${po_core_integrated_cmake_dir}")

    message(STATUS "Successfully configured integrated po_core. Searching for it at ${po_core_integrated_cmake_dir}")
else()
    message(STATUS "Using non-integrated mode, po_core will be used from: ${po_core_external_folder}")
    # 检查 po_core_external_folder 是否存在
    if(NOT EXISTS "${po_core_external_folder}")
        message(FATAL_ERROR "External po_core folder not found at: ${po_core_external_folder}\n"
            "Please specify correct po_core_external_folder path")
    endif()
    list(APPEND CLEAN_CMAKE_PREFIX_PATH "${po_core_external_folder}/lib/cmake")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
endif()


# 现在调用 find_package(po_core)
# po_core-config.cmake 会检测到父项目已找到的 OpenCV 并自动配置依赖传播
find_package(po_core REQUIRED)

# ------------------------------------------------------------------------------
# OpenGV库配置
# ------------------------------------------------------------------------------

# 优先查找项目内部安装的 OpenGV
set(LOCAL_OPENGV_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/opengv/install_local")

if(EXISTS "${LOCAL_OPENGV_INSTALL_DIR}/include/opengv/types.hpp")
    message(STATUS "Attempting to use local OpenGV installation from: ${LOCAL_OPENGV_INSTALL_DIR}")
    
    # 强制使用本地安装的OpenGV，避免系统版本和Anaconda版本干扰
    # 1. 清除可能存在的 OpenGV 缓存变量
    unset(OpenGV_DIR CACHE)
    unset(OpenGV_FOUND CACHE)
    
    # 2. 如果存在CMake配置文件，显式设置OpenGV_DIR
    if(EXISTS "${LOCAL_OPENGV_INSTALL_DIR}/lib/cmake/opengv/opengvConfig.cmake")
        set(OpenGV_DIR "${LOCAL_OPENGV_INSTALL_DIR}/lib/cmake/opengv" CACHE PATH "Path to OpenGV CMake config" FORCE)
        message(STATUS "PoSDK: Forcing OpenGV_DIR to: ${OpenGV_DIR}")
    endif()
    
    # 3. 将本地安装目录添加到干净的 CMAKE_PREFIX_PATH（排除Anaconda）
    list(PREPEND CLEAN_CMAKE_PREFIX_PATH "${LOCAL_OPENGV_INSTALL_DIR}")
    set(CMAKE_PREFIX_PATH "${CLEAN_CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path (clean, no Anaconda)" FORCE)
    message(STATUS "Added local OpenGV to clean CMAKE_PREFIX_PATH: ${LOCAL_OPENGV_INSTALL_DIR}")
else()
    message(STATUS "Local OpenGV installation not found at ${LOCAL_OPENGV_INSTALL_DIR}/include/opengv/types.hpp")
    message(STATUS "Please run install_opengv.sh to build and install OpenGV locally.")
    message(STATUS "Will attempt to use system OpenGV installation...")
endif()

# ------------------------------------------------------------------------------
# 依赖项配置
# ------------------------------------------------------------------------------
# 查找并加载必要的外部依赖

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 查找 GTest (兼容 Ubuntu 18.04/24.04 和 macOS)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 策略：
# 1. 显式使用 MODULE 模式，这是跨平台最兼容的方式
# 2. 为 FindGTest.cmake 提供明确的路径提示（Ubuntu 18.04 需要）
# 3. 如果失败，提供清晰的、针对不同系统的修复指南
#
# 这避免了 find_package 静默失败后回退到 CONFIG 模式，从而产生误导性的错误信息。
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Set hint variables for FindGTest.cmake (critical for Ubuntu 18.04)
# Ubuntu 18.04's FindGTest.cmake may not search /usr/local by default
if(EXISTS "/usr/local/include/gtest/gtest.h")
    set(GTEST_ROOT "/usr/local" CACHE PATH "GTest root directory (for FindGTest.cmake)")
    set(GTEST_INCLUDE_DIR "/usr/local/include" CACHE PATH "GTest include directory")
    message(STATUS "Setting GTest hint paths for FindGTest.cmake: /usr/local")
endif()

# Explicitly use MODULE mode to prevent CONFIG mode fallback
find_package(GTest MODULE)

# 如果上面的命令失败，尝试手动诊断（Ubuntu 18.04 兼容性回退）
if(NOT GTest_FOUND)
    message(STATUS "find_package(GTest MODULE) failed, checking for manual installation...")
    
    # Check common installation locations
    set(GTEST_MANUAL_PATHS
        "/usr/local/include/gtest/gtest.h"
        "/usr/include/gtest/gtest.h"
    )
    
    set(GTEST_HEADER_FOUND "")
    foreach(HEADER_PATH ${GTEST_MANUAL_PATHS})
        if(EXISTS "${HEADER_PATH}")
            get_filename_component(GTEST_INCLUDE_DIR "${HEADER_PATH}" DIRECTORY)
            get_filename_component(GTEST_ROOT_DIR "${GTEST_INCLUDE_DIR}" DIRECTORY)
            set(GTEST_HEADER_FOUND "${HEADER_PATH}")
            break()
        endif()
    endforeach()
    
    if(GTEST_HEADER_FOUND)
        message(STATUS "Found GTest header at: ${GTEST_HEADER_FOUND}")
        message(STATUS "Attempting to find libraries manually...")
        
        # Find library files
        set(GTEST_LIB_PATHS
            "${GTEST_ROOT_DIR}/lib"
            "/usr/local/lib"
            "/usr/lib"
        )
        
        set(GTEST_LIBRARY "")
        set(GTEST_MAIN_LIBRARY "")
        
        foreach(LIB_DIR ${GTEST_LIB_PATHS})
            if(EXISTS "${LIB_DIR}/libgtest.a" AND NOT GTEST_LIBRARY)
                set(GTEST_LIBRARY "${LIB_DIR}/libgtest.a")
            endif()
            if(EXISTS "${LIB_DIR}/libgtest_main.a" AND NOT GTEST_MAIN_LIBRARY)
                set(GTEST_MAIN_LIBRARY "${LIB_DIR}/libgtest_main.a")
            endif()
        endforeach()
        
        if(GTEST_LIBRARY AND GTEST_MAIN_LIBRARY)
            message(STATUS "✓ Found GTest libraries manually (Ubuntu 18.04 compatibility mode)")
            message(STATUS "  Library: ${GTEST_LIBRARY}")
            message(STATUS "  Main: ${GTEST_MAIN_LIBRARY}")
            message(STATUS "  Include: ${GTEST_INCLUDE_DIR}")
            
            # Manually create imported targets
            add_library(GTest::GTest STATIC IMPORTED)
            set_target_properties(GTest::GTest PROPERTIES
                IMPORTED_LOCATION "${GTEST_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            )
            
            add_library(GTest::Main STATIC IMPORTED)
            set_target_properties(GTest::Main PROPERTIES
                IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES GTest::GTest
            )
            
            # Create aliases for compatibility
            if(NOT TARGET GTest::gtest)
                add_library(GTest::gtest ALIAS GTest::GTest)
            endif()
            if(NOT TARGET GTest::gtest_main)
                add_library(GTest::gtest_main ALIAS GTest::Main)
            endif()
            
            set(GTest_FOUND TRUE)
            set(GTEST_FOUND TRUE)
            message(STATUS "✓ GTest manually configured")
        endif()
    endif()
endif()

if(NOT GTest_FOUND)
    message(FATAL_ERROR 
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "✗ GTest not found!\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "\n"
        "For Ubuntu 24.04 / macOS (PRIMARY):\n"
        "  Ubuntu: sudo apt-get install -y libgtest-dev googletest\n"
        "  macOS:  brew install googletest\n"
        "\n"
        "For Ubuntu 18.04/20.04 (COMPATIBILITY):\n"
        "  Run './install.sh' which will compile and install GTest to /usr/local.\n"
        "\n"
        "Diagnostics:\n"
        "  Check headers: ls -la /usr/local/include/gtest/gtest.h\n"
        "  Check libraries: ls -la /usr/local/lib/libgtest*.a\n"
        "  If files exist but CMake still fails, delete 'build' directory and retry.\n"
        "\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    )
else()
    message(STATUS "✓ GTest found successfully")
endif()

# find_package 现在会优先考虑干净的 CMAKE_PREFIX_PATH 中的本地安装（排除Anaconda）
# 如果设置了 OpenGV_DIR，使用显式路径查找（类似 Ceres 的方式）
if(OpenGV_DIR AND EXISTS "${OpenGV_DIR}/opengvConfig.cmake")
    # 使用显式路径查找，使用 CONFIG 模式和严格路径限制
    find_package(OpenGV CONFIG
        PATHS "${OpenGV_DIR}"
        NO_DEFAULT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_PACKAGE_REGISTRY
        NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
    )
else()
    # 使用普通的 find_package（会使用 CMAKE_PREFIX_PATH，已清理过 Anaconda）
    # find_package 会自动尝试 CONFIG 模式，如果没有配置文件则回退到 MODULE 模式
find_package(OpenGV) # 移除 REQUIRED，后续做检查
endif()

# 检查OpenGV是否找到
if(OpenGV_FOUND)
    message(STATUS "Found OpenGV at ${OpenGV_DIR}")
    if(OpenGV_INCLUDE_DIRS)
        message(STATUS "OpenGV include dirs: ${OpenGV_INCLUDE_DIRS}")
    endif()
    if(OpenGV_LIBRARIES)
        message(STATUS "OpenGV libraries: ${OpenGV_LIBRARIES}")
    endif()
else()
    # 如果没有找到本地安装的，尝试手动设置系统路径
    message(STATUS "OpenGV not found via find_package, trying manual detection...")
    
    # 手动设置OpenGV路径
    set(OpenGV_INCLUDE_DIRS "/usr/local/include")
    set(OpenGV_LIBRARIES "/usr/local/lib/libopengv.a")
    if(EXISTS "${OpenGV_LIBRARIES}" AND IS_DIRECTORY "${OpenGV_INCLUDE_DIRS}/opengv")
        set(OpenGV_FOUND TRUE)
        message(STATUS "Manually set OpenGV_LIBRARIES: ${OpenGV_LIBRARIES}")
        message(STATUS "Manually set OpenGV_INCLUDE_DIRS: ${OpenGV_INCLUDE_DIRS}")
    else()
        message(FATAL_ERROR "OpenGV not found. Please ensure it is built and installed correctly in dependencies/opengv/install_local by running install_opengv.sh, or provide a valid system installation.")
    endif()
endif()


# ------------------------------------------------------------------------------
# dependencies 配置
# ------------------------------------------------------------------------------

# ==============================================================================
# Lemon
# ==============================================================================
# - internal by default (Lemon),
# - external if LEMON_INCLUDE_DIR_HINTS and a valid Lemon setup is found
# ==============================================================================
# 设置Lemon库路径
set(LEMON_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/dependencies/lemon)

option(USE_OPENMVG "Use OpenMVG library" OFF)
if(USE_OPENMVG)
    set(OpenMVG_BIN_DIR "" CACHE PATH "Path to OpenMVG binary directory")
    if(NOT OpenMVG_BIN_DIR)
        message(WARNING "OpenMVG_BIN_DIR not specified. Please set -DOpenMVG_BIN_DIR=/path/to/OpenMVG/bin")
    else()
        message(STATUS "Using OpenMVG binaries from: ${OpenMVG_BIN_DIR}")
    endif()

    find_package(cereal QUIET CONFIG)
    if (cereal_FOUND)
        message(STATUS "Found external cereal library")
    else()
        message(STATUS "Using internal cereal library")
        add_library(cereal INTERFACE)
        target_include_directories(cereal
                INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cereal/include>
        )
        install(TARGETS cereal EXPORT openMVG-targets)
    endif()

    if (TARGET cereal::cereal)
        set(cereal_TARGET "cereal::cereal")
        message(STATUS "Using cereal::cereal target")
    endif()
    if (TARGET cereal)
        set(cereal_TARGET "cereal")
        message(STATUS "Using cereal target")
    endif()
endif()

# 复制cereal整个目录到output/include
file(COPY "${CMAKE_SOURCE_DIR}/dependencies/cereal/include/cereal"
     DESTINATION "${OUTPUT_INCLUDE_DIR}")

# 复制lemon库到output/include
file(COPY "${CMAKE_SOURCE_DIR}/dependencies/lemon/lemon"
     DESTINATION "${OUTPUT_INCLUDE_DIR}")

# 确保lemon的config.h文件被正确复制
configure_file(
    "${CMAKE_SOURCE_DIR}/dependencies/lemon/lemon/config.h"
    "${OUTPUT_INCLUDE_DIR}/lemon/config.h"
    COPYONLY
)

# ==============================================================================
# PoSDK Python Drawer 配置
# ==============================================================================
# 从po_core集成的Python目录拷贝到output/python目录
set(DRAWER_SOURCE_DIR "${po_core_external_folder}/python")

if(EXISTS "${DRAWER_SOURCE_DIR}")
    # 获取集成python目录中的各类文件
    file(GLOB PYTHON_SCRIPTS "${DRAWER_SOURCE_DIR}/*.py")
    file(GLOB LAUNCHER_SCRIPTS "${DRAWER_SOURCE_DIR}/*.sh" "${DRAWER_SOURCE_DIR}/*.bat")
    file(GLOB DRAWER_DOCS "${DRAWER_SOURCE_DIR}/*.md")
    file(GLOB DRAWER_CONFIGS "${DRAWER_SOURCE_DIR}/*.txt")

    # 拷贝Python脚本
if(PYTHON_SCRIPTS)
    file(COPY ${PYTHON_SCRIPTS} DESTINATION ${OUTPUT_PYTHON_DIR})
        message(STATUS "Copied Python scripts from po_core to: ${OUTPUT_PYTHON_DIR}")
else()
        message(WARNING "No Python scripts found in ${DRAWER_SOURCE_DIR}")
endif()

    # 拷贝启动脚本
if(LAUNCHER_SCRIPTS)
    file(COPY ${LAUNCHER_SCRIPTS} DESTINATION ${OUTPUT_PYTHON_DIR})
        message(STATUS "Copied launcher scripts from po_core to: ${OUTPUT_PYTHON_DIR}")
    
    # 在Unix系统上给shell脚本添加执行权限
    if(UNIX)
        file(GLOB SHELL_SCRIPTS "${OUTPUT_PYTHON_DIR}/*.sh")
        foreach(SCRIPT ${SHELL_SCRIPTS})
            execute_process(COMMAND chmod +x "${SCRIPT}")
        endforeach()
        message(STATUS "Set executable permissions for shell scripts")
    endif()
endif()

    # 拷贝文档文件
if(DRAWER_DOCS)
    file(COPY ${DRAWER_DOCS} DESTINATION ${OUTPUT_PYTHON_DIR})
        message(STATUS "Copied drawer documentation from po_core to: ${OUTPUT_PYTHON_DIR}")
endif()

    # 拷贝配置文件
if(DRAWER_CONFIGS)
    file(COPY ${DRAWER_CONFIGS} DESTINATION ${OUTPUT_PYTHON_DIR})
        message(STATUS "Copied drawer config files from po_core to: ${OUTPUT_PYTHON_DIR}")
    endif()
    
    # 拷贝子目录（如conda_env等）
    file(GLOB DRAWER_SUBDIRS "${DRAWER_SOURCE_DIR}/*/")
    if(DRAWER_SUBDIRS)
        foreach(SUBDIR ${DRAWER_SUBDIRS})
            get_filename_component(SUBDIR_NAME ${SUBDIR} NAME)
            file(COPY ${SUBDIR} DESTINATION ${OUTPUT_PYTHON_DIR})
            message(STATUS "Copied drawer subdirectory ${SUBDIR_NAME} from po_core to: ${OUTPUT_PYTHON_DIR}")
        endforeach()
    endif()
    
    message(STATUS "PoSDK Python Drawer integration from po_core completed successfully")
else()
    message(WARNING "po_core Python drawer directory not found at: ${DRAWER_SOURCE_DIR}")
    message(WARNING "Please ensure po_core has been built with drawer integration enabled")
endif()


# ------------------------------------------------------------------------------
# OpenMP 配置（统一的多系统支持）
# ------------------------------------------------------------------------------
# 确保线程库可用
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 检测OpenMP支持，参考install_magsac.sh的逻辑
if(APPLE)
    # macOS: 首先尝试标准的OpenMP检测
    find_package(OpenMP QUIET)
    
    if(NOT OpenMP_CXX_FOUND)
        # 如果标准检测失败，尝试检查Homebrew LLVM和libomp
        # 使用固定路径，因为Homebrew在Apple Silicon Mac上的标准路径
        set(POTENTIAL_LLVM_PATHS
            "/opt/homebrew/opt/llvm"  # Apple Silicon Macs
            "/usr/local/opt/llvm"     # Intel Macs
        )
        
        set(POTENTIAL_LIBOMP_PATHS
            "/opt/homebrew/opt/libomp"  # Apple Silicon Macs
            "/usr/local/opt/libomp"     # Intel Macs
        )
        
        # 查找LLVM编译器
        foreach(LLVM_PATH ${POTENTIAL_LLVM_PATHS})
            if(EXISTS "${LLVM_PATH}/bin/clang++")
                set(LLVM_PREFIX "${LLVM_PATH}")
                break()
            endif()
        endforeach()
        
        # 查找libomp库
        foreach(LIBOMP_PATH ${POTENTIAL_LIBOMP_PATHS})
            if(EXISTS "${LIBOMP_PATH}/lib/libomp.dylib")
                set(LIBOMP_PREFIX "${LIBOMP_PATH}")
                break()
            endif()
        endforeach()
        
        if(LLVM_PREFIX AND LIBOMP_PREFIX)
            message(STATUS "Found Homebrew LLVM at: ${LLVM_PREFIX}")
            message(STATUS "Found Homebrew libomp at: ${LIBOMP_PREFIX}")
            # 设置OpenMP相关变量
            set(OpenMP_C_FLAGS "-Xpreprocessor;-fopenmp;-I${LIBOMP_PREFIX}/include")
            set(OpenMP_CXX_FLAGS "-Xpreprocessor;-fopenmp;-I${LIBOMP_PREFIX}/include")
            set(OpenMP_C_LIB_NAMES "libomp")
            set(OpenMP_CXX_LIB_NAMES "libomp")
            set(OpenMP_libomp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
            
            # 手动设置OpenMP为找到状态
            set(OpenMP_FOUND TRUE)
            set(OpenMP_CXX_FOUND TRUE)
            
            # 创建OpenMP::OpenMP_CXX目标（如果不存在）
            if(NOT TARGET OpenMP::OpenMP_CXX)
                add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
                set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_COMPILE_OPTIONS "-Xpreprocessor" "-fopenmp")
                set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LIBOMP_PREFIX}/include")
                set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_LINK_LIBRARIES ${OpenMP_libomp_LIBRARY})
            endif()
            
            message(STATUS "OpenMP configured for macOS with Homebrew LLVM and libomp")
        else()
            message(WARNING "Homebrew LLVM or libomp not found. OpenMP may not be available.")
        endif()
    endif()
elseif(WIN32)
    # Windows: 使用标准的FindOpenMP，通常与MSVC一起工作
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP found on Windows")
    else()
        message(WARNING "OpenMP not found on Windows. Consider installing Visual Studio with OpenMP support or using Intel Compiler.")
    endif()
else()
    # Linux/Unix: 使用标准的FindOpenMP
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP found on Linux/Unix")
    else()
        message(WARNING "OpenMP not found on Linux/Unix. Install with: sudo apt-get install libomp-dev (Ubuntu) or equivalent.")
    endif()
endif()

# 设置全局OpenMP状态信息，供子项目使用
if(OpenMP_CXX_FOUND)
    message(STATUS "✓ OpenMP is available for multi-threading support")
    # 可选：设置全局编译标志（如果需要）
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message(WARNING "❌ OpenMP not available. Multi-threading features will be disabled.")
endif()


# 输出依赖项检测状态
message(STATUS "Found po_core: ${po_core_FOUND}")
message(STATUS "Found absl: ${absl_FOUND}")
message(STATUS "Found Protobuf: ${Protobuf_FOUND}")
message(STATUS "Found GTest: ${GTEST_FOUND}")
message(STATUS "Found OpenGV: ${OpenGV_FOUND}")
message(STATUS "Found Eigen3: ${Eigen3_FOUND}")
message(STATUS "Found OpenCV: ${OpenCV_FOUND}")
message(STATUS "Using external OpenMVG: ${POMVG_USE_EXTERNAL_OPENMVG}")
if(Ceres_FOUND)
    message(STATUS "Found Ceres: ${Ceres_FOUND} (Version: ${Ceres_VERSION})")
else()
    message(STATUS "Found Ceres: ${Ceres_FOUND}")
endif()

# ------------------------------------------------------------------------------
# 子项目配置
# ------------------------------------------------------------------------------
add_subdirectory(common)  # 新增 common 子目录
add_subdirectory(plugins)

# ------------------------------------------------------------------------------
# 创建聚合动态库 PoEngine::posdk
# ------------------------------------------------------------------------------
add_library(posdk_agg SHARED dummy_posdk_agg.cpp) # 添加虚拟源文件

# 设置别名 PoEngine::posdk
add_library(PoEngine::posdk ALIAS posdk_agg)

target_link_libraries(posdk_agg PUBLIC
    PoSDK::po_core      # 来自 find_package(po_core)
    PoSDK::pomvg_proto  # 假设也来自 find_package(po_core) 或其依赖
    pomvg_common        # 来自 add_subdirectory(common)
    pomvg_converter     # 来自 add_subdirectory(common)
)

# 确保聚合库的输出目录与项目中其他库一致
set_target_properties(posdk_agg PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# 传递包含目录
# 当 PUBLIC 链接时，依赖项的 INTERFACE_INCLUDE_DIRECTORIES 会自动传递。
target_include_directories(posdk_agg PUBLIC
    ${OUTPUT_INCLUDE_DIR}         # po_core 和可能的其他头文件
    ${OUTPUT_COMMON_INCLUDE_DIR}  # common 库的头文件
    # PoSDK::po_core 目标应该已经有正确的 INTERFACE_INCLUDE_DIRECTORIES
    # common 库目标 (pomvg_common, pomvg_converter) 也应该有
) 

# add_subdirectory(tests)
add_subdirectory(software) 

# ------------------------------------------------------------------------------
# Test Data Configuration | 测试数据配置
#  ------------------------------------------------------------------------------
#  自动拷贝测试数据到输出目录，使ini配置可以使用相对路径
# Copy test data to output directory automatically, allowing ini configs  to use relative paths

set(SOURCE_STRECHA_DIR  "${CMAKE_SOURCE_DIR}/tests/Strecha")
set(TARGET_TESTS_DIR "${OUTPUT_TESTS_DIR}")

# 检查源测试数据目录是否存在
#  Check if source  Strecha directory exists
if(EXISTS "${SOURCE_STRECHA_DIR}")
    message(STATUS "Found Strecha  test data source directory: ${SOURCE_STRECHA_DIR}")

    # 拷贝Strecha目录到输出目录
     # Copy entire  tests directory to output directory
     file(COPY  "${SOURCE_STRECHA_DIR}" DESTINATION "${TARGET_TESTS_DIR}")
     message(STATUS  "✓ Copied Strecha test data  to: ${TARGET_TESTS_DIR}/Strecha")
     message(STATUS "  Now ini files can  use: dataset_dir={exe_dir}/../tests/Strecha")

    #  检查Strecha数据是否存在
     # Check  if Strecha data exists
     if(EXISTS  "${TARGET_TESTS_DIR}/Strecha")
         message(STATUS "✓ Strecha test dataset available at: {exe_dir}/../tests/Strecha")

         # 列出可用的数据集
         #  List available datasets
          file(GLOB STRECHA_DATASETS "${TARGET_TESTS_DIR}/Strecha/*")
         if(STRECHA_DATASETS)
              message(STATUS "  Available Strecha datasets:")
             foreach(DATASET ${STRECHA_DATASETS})
                   if(IS_DIRECTORY ${DATASET})
                      get_filename_component(DATASET_NAME ${DATASET} NAME)
                      message(STATUS  "    - ${DATASET_NAME}")
                endif()
             endforeach()
        endif()
    else()
         message(WARNING "Strecha test data not found in copied directory")
    endif()
else()
    message(WARNING  "Strecha test data directory not found: ${SOURCE_STRECHA_DIR}")
    message(WARNING "Please ensure Strecha test data is available at src/tests/Strecha for automatic copying")
    message(WARNING "Manual setup: copy Strecha data to ${TARGET_TESTS_DIR}")
endif()

