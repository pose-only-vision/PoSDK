# ==============================================================================
# Copyright (c) 2024 PoSDK Project
# ==============================================================================

# Find OpenCV package
find_package(OpenCV REQUIRED)

# ==============================================================================
# Image viewer library build configuration
# ==============================================================================
add_library(pomvg_image_viewer SHARED
    image_viewer.cpp
    image_viewer.hpp
)

# ------------------------------------------------------------------------------
# Header file include configuration
# ------------------------------------------------------------------------------
target_include_directories(pomvg_image_viewer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${OUTPUT_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
)

# ------------------------------------------------------------------------------
# Dependency library linking configuration
# ------------------------------------------------------------------------------
target_link_libraries(pomvg_image_viewer
    PUBLIC
        ${OpenCV_LIBS}
        PoSDK::po_core
)

# ------------------------------------------------------------------------------
# Compile options configuration
# ------------------------------------------------------------------------------
target_compile_options(pomvg_image_viewer PRIVATE ${POMVG_COMPILE_OPTIONS})
target_compile_definitions(pomvg_image_viewer PRIVATE ${POMVG_COMPILE_DEFINITIONS})

target_compile_options(pomvg_image_viewer PRIVATE -fPIC)

if(USE_SANITIZER)
    target_compile_options(pomvg_image_viewer PRIVATE 
        -fsanitize=address 
        -fno-omit-frame-pointer
    )
    target_link_options(pomvg_image_viewer PRIVATE 
        -fsanitize=address
    )
endif()

# ------------------------------------------------------------------------------
# Output configuration
# ------------------------------------------------------------------------------
set_target_properties(pomvg_image_viewer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_COMMON_DIR}"
    POSITION_INDEPENDENT_CODE ON
)

# Copy header files to build directory
file(GLOB IMAGE_VIEWER_HEADERS "*.hpp" "*.h")
foreach(HEADER ${IMAGE_VIEWER_HEADERS})
    file(COPY ${HEADER} 
        DESTINATION "${OUTPUT_COMMON_INCLUDE_DIR}/image_viewer")
endforeach()

# Create export target
add_library(PoSDK::pomvg_image_viewer ALIAS pomvg_image_viewer)
