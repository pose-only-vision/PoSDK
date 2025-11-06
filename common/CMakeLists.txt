# ==============================================================================
# Common library project configuration
# ==============================================================================
cmake_minimum_required(VERSION 3.10)
project(pomvg_common VERSION 1.0.0 LANGUAGES CXX)

# Set compile options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add submodules
add_subdirectory(converter)
add_subdirectory(image_viewer)

# Create aggregated library
add_library(pomvg_common SHARED pomvg_common.cpp)

# Link submodules and dependency libraries
target_link_libraries(pomvg_common
    PUBLIC
        pomvg_image_viewer
        $<$<BOOL:${POMVG_USE_EXTERNAL_OPENMVG}>:pomvg_converter>
        PoSDK::po_core
)

# Set include directories
target_include_directories(pomvg_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${OUTPUT_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Set compile options
target_compile_options(pomvg_common 
    PRIVATE 
        ${POMVG_COMPILE_OPTIONS}
        -fPIC
)

if(USE_SANITIZER)
    target_compile_options(pomvg_common 
        PRIVATE 
            -fsanitize=address 
            -fno-omit-frame-pointer
    )
    target_link_options(pomvg_common 
        PRIVATE 
            -fsanitize=address
    )
endif()

# Set output paths
set_target_properties(pomvg_common PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
)

# Collect and install header files
file(GLOB_RECURSE COMMON_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Copy header files to build directory
foreach(HEADER ${COMMON_HEADERS})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${HEADER})
    get_filename_component(HEADER_DIR ${REL_PATH} DIRECTORY)
    file(COPY ${HEADER} 
        DESTINATION "${OUTPUT_COMMON_INCLUDE_DIR}/${HEADER_DIR}")
endforeach()

# Create export target
add_library(PoSDK::pomvg_common ALIAS pomvg_common)

# Install rules
install(TARGETS pomvg_common
    EXPORT pomvg-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/common
    FILES_MATCHING 
        PATTERN "*.hpp"
        PATTERN "*.h"
)


