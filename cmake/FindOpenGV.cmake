# FindOpenGV.cmake
# Find OpenGV library and headers, prioritizing local installation

# 优先查找本地安装路径（CMAKE_PREFIX_PATH会被自动包含）
find_path(OpenGV_INCLUDE_DIR
    NAMES opengv/types.hpp
    PATHS
        # 本地安装路径会通过CMAKE_PREFIX_PATH自动包含
        ${CMAKE_PREFIX_PATH}/include
        # 系统路径
        /usr/local/include
        /usr/include
        /opt/homebrew/include  # macOS Homebrew (Apple Silicon)
        /usr/local/opt/opengv/include  # macOS Homebrew (Intel)
    PATH_SUFFIXES
        ""
)

find_library(OpenGV_LIBRARY
    NAMES opengv libopengv
    PATHS
        # 本地安装路径会通过CMAKE_PREFIX_PATH自动包含
        ${CMAKE_PREFIX_PATH}/lib
        # 系统路径
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /opt/homebrew/lib  # macOS Homebrew (Apple Silicon)
        /usr/local/opt/opengv/lib  # macOS Homebrew (Intel)
    PATH_SUFFIXES
        ""
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGV
    REQUIRED_VARS 
        OpenGV_LIBRARY 
        OpenGV_INCLUDE_DIR
)

if(OpenGV_FOUND)
    set(OpenGV_LIBRARIES ${OpenGV_LIBRARY})
    set(OpenGV_INCLUDE_DIRS ${OpenGV_INCLUDE_DIR})
    
    if(NOT TARGET OpenGV::OpenGV)
        add_library(OpenGV::OpenGV UNKNOWN IMPORTED)
        set_target_properties(OpenGV::OpenGV PROPERTIES
            IMPORTED_LOCATION "${OpenGV_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OpenGV_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(
    OpenGV_INCLUDE_DIR
    OpenGV_LIBRARY
)