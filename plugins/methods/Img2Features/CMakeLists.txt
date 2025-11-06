# ==============================================================================
# Img2Features Plugin CMakeLists.txt
# Image feature extraction plugin build configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# Include plugin configuration tools
include(../../cmake/plugin-config.cmake)

# ------------------------------------------------------------------------------
# GLib 依赖检测（Linux 上 OpenCV + GTK-3 需要）
# GLib dependency detection (required by OpenCV + GTK-3 on Linux)
# ------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        # Find GLib using pkg-config (avoid Anaconda's GLib)
        # 使用 pkg-config 查找 GLib（避免 Anaconda 的 GLib）
        pkg_check_modules(GLIB QUIET glib-2.0)
        if(GLIB_FOUND)
            message(STATUS "Found GLib ${GLIB_VERSION} for GTK-3 compatibility")
            message(STATUS "  GLib libraries: ${GLIB_LIBRARIES}")
            message(STATUS "  GLib library dirs: ${GLIB_LIBRARY_DIRS}")
            
            # Verify GLib is from system, not Anaconda
            # 验证 GLib 来自系统，而非 Anaconda
            set(GLIB_FROM_SYSTEM TRUE)
            foreach(GLIB_LIB ${GLIB_LIBRARIES})
                if(GLIB_LIB MATCHES "anaconda" OR GLIB_LIB MATCHES "conda")
                    set(GLIB_FROM_SYSTEM FALSE)
                    message(WARNING "Warning: GLib found in Anaconda/Conda path: ${GLIB_LIB}")
                    message(WARNING "This may cause GTK-3 linking issues. Please ensure system GLib is used.")
                endif()
            endforeach()
            
            if(GLIB_FROM_SYSTEM)
                message(STATUS "  ✓ GLib is from system (not Anaconda)")
            else()
                message(WARNING "  ⚠ GLib may be from Anaconda - linking may fail")
            endif()
        else()
            message(WARNING "GLib not found via pkg-config. GTK-3 linking may fail if OpenCV uses GTK.")
            message(WARNING "Install with: sudo apt-get install libglib2.0-dev")
        endif()
    else()
        message(WARNING "pkg-config not found. Cannot detect GLib for GTK-3 compatibility.")
        message(WARNING "Install with: sudo apt-get install pkg-config libglib2.0-dev")
    endif()
endif()

# Set specific configuration for current plugin directory
set(CURRENT_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# Img2Features Plugin
# ------------------------------------------------------------------------------
# ✅ 插件名称改为 method_img2features（与注册名一致，实现单一信息源）
add_posdk_plugin(method_img2features
    PLUGIN_TYPE methods
    PLUGIN_FOLDER ${CURRENT_PLUGIN_DIR}
    SOURCES
        img2features_pipeline.cpp
    HEADERS
        img2features_pipeline.hpp
    LINK_LIBRARIES
        PoSDK::po_core
        PoSDK::pomvg_converter
        PoSDK::pomvg_common
        ${OpenCV_LIBS}
        # GLib 依赖（解决 GTK-3 链接问题，避免 Anaconda 版本冲突）
        # GLib dependency (fixes GTK-3 linking issues, avoids Anaconda version conflicts)
        $<$<BOOL:${GLIB_FOUND}>:${GLIB_LDFLAGS}>
    INCLUDE_DIRS
        ${OpenCV_INCLUDE_DIRS}
    COMPILE_DEFINITIONS
        # ✅ 删除 PLUGIN_NAME（由 add_posdk_plugin 自动添加）
        PLUGIN_VERSION="2.0.0"
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# ------------------------------------------------------------------------------
# Manual installation of shell scripts and requirements files
# ------------------------------------------------------------------------------
# Install environment configuration scripts
install(FILES 
    ${CURRENT_PLUGIN_DIR}/configure_lightglue_env.sh
    ${CURRENT_PLUGIN_DIR}/requirements.txt
    DESTINATION plugins/methods
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Copy to build output directory
configure_file(
    ${CURRENT_PLUGIN_DIR}/configure_lightglue_env.sh
    "${OUTPUT_BASE_DIR}/plugins/methods/configure_lightglue_env.sh"
    COPYONLY
)
configure_file(
    ${CURRENT_PLUGIN_DIR}/requirements.txt
    "${OUTPUT_BASE_DIR}/plugins/methods/requirements.txt"
    COPYONLY
)

# ------------------------------------------------------------------------------
# Plugin-specific configuration
# ------------------------------------------------------------------------------

# Plugin information summary
message(STATUS "Img2Features Plugin Configuration:")
message(STATUS "  Plugin Name: method_img2features")
message(STATUS "  Plugin Type: methods")
message(STATUS "  Plugin File: posdk_plugin_method_img2features.dylib/.so/.dll")
message(STATUS "  Plugin Folder: ${CURRENT_PLUGIN_DIR}")
message(STATUS "  Sources: img2features_pipeline.cpp")
message(STATUS "  Headers: img2features_pipeline.hpp")
message(STATUS "  Config: method_img2features.ini")
message(STATUS "  Python Scripts: method_img2features_plugin_superpoint.py")

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building Img2Features plugin in Debug mode")
endif()

# Test support (if testing is enabled)
if(BUILD_TESTING)
    message(STATUS "Img2Features plugin: Testing enabled")
endif()

# Documentation generation (if documentation is enabled)
if(BUILD_DOCUMENTATION)
    message(STATUS "Img2Features plugin: Documentation enabled")
endif()
