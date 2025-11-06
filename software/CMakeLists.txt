# PoSDK GlobalSfM Pipeline 可执行程序
# 注意：这是主CMakeLists.txt的子目录，不需要重新设置项目和标准

# gflags依赖已经通过主CMakeLists.txt中的Ceres找到了，无需重复查找

# 添加宏定义 - 使用正确的项目源目录
add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# 创建PoSDK可执行程序
add_executable(PoSDK posdk_globalsfm.cpp)

# 链接库 - 使用父目录已经找到的po_core目标
# 注意：po_core现在会自动传播OpenCV依赖，无需显式链接${OpenCV_LIBS}
target_link_libraries(PoSDK 
    PoSDK::po_core      # 使用主CMakeLists.txt中找到的po_core目标（自动包含OpenCV依赖）
    gflags             # 通过Ceres找到的gflags目标
    Threads::Threads    # 使用标准的线程库目标
    # ${OpenCV_LIBS}    # 已移除：po_core现在会自动传播OpenCV依赖
)

# 处理CUDA依赖（如果OpenCV使用CUDA编译）
# 注意：OpenCV可能启用了CUDA支持，导致需要CUDA运行时库
find_package(CUDA QUIET)

if(CUDA_FOUND)
    message(STATUS "Found CUDA version: ${CUDA_VERSION}")
    message(STATUS "CUDA libraries: ${CUDA_LIBRARIES}")
    message(STATUS "CUDA library dirs: ${CUDA_LIBRARY_DIRS}")
    
    target_link_libraries(PoSDK ${CUDA_LIBRARIES})
    
    if(CUDA_LIBRARY_DIRS)
        get_target_property(CURRENT_RPATH PoSDK INSTALL_RPATH)
        if(CURRENT_RPATH)
            set_target_properties(PoSDK PROPERTIES
                INSTALL_RPATH "${CURRENT_RPATH}:${CUDA_LIBRARY_DIRS}"
            )
        else()
            set_target_properties(PoSDK PROPERTIES
                INSTALL_RPATH "$ORIGIN/../lib:${CUDA_LIBRARY_DIRS}"
            )
        endif()
    endif()
else()
    # 手动查找CUDA库（常见路径）
    message(WARNING "CUDA not found via find_package, trying manual search...")
    set(CUDA_SEARCH_PATHS
        "/usr/local/cuda"
        "/usr/local/cuda-12"
        "/usr/local/cuda-12.0"
        "/usr/local/cuda-11"
        "/opt/cuda"
        "/opt/cuda-12"
    )
    
    set(CUDA_FOUND_MANUAL FALSE)
    foreach(CUDA_PATH ${CUDA_SEARCH_PATHS})
        if(EXISTS "${CUDA_PATH}/lib64/libcudart.so" OR EXISTS "${CUDA_PATH}/lib/libcudart.so")
            set(CUDA_FOUND_MANUAL TRUE)
            if(EXISTS "${CUDA_PATH}/lib64/libcudart.so")
                set(CUDA_LIB_DIR "${CUDA_PATH}/lib64")
            else()
                set(CUDA_LIB_DIR "${CUDA_PATH}/lib")
            endif()
            message(STATUS "Found CUDA library directory: ${CUDA_LIB_DIR}")
            
            # 链接CUDA运行时库
            target_link_libraries(PoSDK
                ${CUDA_LIB_DIR}/libcudart.so
                ${CUDA_LIB_DIR}/libcusolver.so
                ${CUDA_LIB_DIR}/libcublas.so
                ${CUDA_LIB_DIR}/libcusparse.so
            )
            
            # 更新rpath
            get_target_property(CURRENT_RPATH PoSDK INSTALL_RPATH)
            if(CURRENT_RPATH)
                set_target_properties(PoSDK PROPERTIES
                    INSTALL_RPATH "${CURRENT_RPATH}:${CUDA_LIB_DIR}"
                )
            else()
                set_target_properties(PoSDK PROPERTIES
                    INSTALL_RPATH "$ORIGIN/../lib:${CUDA_LIB_DIR}"
                )
            endif()
            break()
        endif()
    endforeach()
    
    if(NOT CUDA_FOUND_MANUAL)
        message(WARNING 
            "CUDA libraries not found automatically. Required by OpenCV (compiled with CUDA support.\n"
            "  Solutions:\n"
            "  1. Install CUDA Toolkit 12.x from: https://developer.nvidia.com/cuda-downloads\n"
            "  2. Set LD_LIBRARY_PATH: export LD_LIBRARY_PATH=/path/to/cuda/lib64:$LD_LIBRARY_PATH\n"
            "  3. Rebuild OpenCV without CUDA: -DWITH_CUDA=OFF\n"
            "  4. Rebuild po_core on target system"
        )
        # 不致命错误，允许继续构建（运行时可能会失败）
    endif()
endif()

# 设置包含目录 - 使用父目录已经配置的路径
target_include_directories(PoSDK PRIVATE
    ${OUTPUT_INCLUDE_DIR}           # 来自主CMakeLists.txt的头文件目录
    ${CMAKE_SOURCE_DIR}             # 项目根目录
)

# 设置输出目录 - 使用父目录的输出目录配置
set_target_properties(PoSDK PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
)

# 修复 macOS rpath 配置，确保能找到 po_core 库
if(APPLE)
    # 根据是否使用集成模式配置不同的 rpath
    if(USE_INTEGRATED_PO_CORE)
        # 集成模式：库在构建输出目录中
        set_target_properties(PoSDK PROPERTIES
            INSTALL_RPATH "@loader_path/../lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    else()
        # 非集成模式：添加外部 po_core 库路径
        get_target_property(PO_CORE_LOCATION PoSDK::po_core IMPORTED_LOCATION)
        if(PO_CORE_LOCATION)
            get_filename_component(PO_CORE_LIB_DIR "${PO_CORE_LOCATION}" DIRECTORY)
            set_target_properties(PoSDK PROPERTIES
                INSTALL_RPATH "@loader_path/../lib;${PO_CORE_LIB_DIR}"
                BUILD_WITH_INSTALL_RPATH TRUE
            )
        else()
            # 回退：使用配置的 po_core 外部路径
            set_target_properties(PoSDK PROPERTIES
                INSTALL_RPATH "@loader_path/../lib;${po_core_external_folder}/lib"
                BUILD_WITH_INSTALL_RPATH TRUE
            )
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux: 设置 rpath（包含 OpenCV 和 po_core 路径）
    # Linux: Set rpath (include OpenCV and po_core paths)
    
    # 收集所有需要的库路径
    # Collect all required library paths
    set(RPATH_PATHS "$ORIGIN/../lib")
    
    # 添加 po_core 库路径
    # Add po_core library path
    if(USE_INTEGRATED_PO_CORE)
        # 集成模式：库在构建输出目录中（已在 $ORIGIN/../lib 中）
        # Integrated mode: libraries in build output directory (already in $ORIGIN/../lib)
    else()
        get_target_property(PO_CORE_LOCATION PoSDK::po_core IMPORTED_LOCATION)
        if(PO_CORE_LOCATION)
            get_filename_component(PO_CORE_LIB_DIR "${PO_CORE_LOCATION}" DIRECTORY)
            list(APPEND RPATH_PATHS "${PO_CORE_LIB_DIR}")
        else()
            list(APPEND RPATH_PATHS "${po_core_external_folder}/lib")
        endif()
    endif()
    
    # 添加 OpenCV 库路径（如果存在）
    # Add OpenCV library path (if exists)
    # 优先使用相对路径，如果无法确定相对路径则使用绝对路径
    # Prefer relative paths, use absolute path if relative path cannot be determined
    
    set(OPENCV_LIB_DIR "")
    
    if(DEFINED OPENCV_INSTALL_PREFIX AND EXISTS "${OPENCV_INSTALL_PREFIX}")
        set(OPENCV_LIB_DIR "${OPENCV_INSTALL_PREFIX}/lib")
    elseif(OpenCV_DIR)
        get_filename_component(OPENCV_LIB_DIR "${OpenCV_DIR}/../../lib" ABSOLUTE)
    endif()
    
    if(OPENCV_LIB_DIR AND EXISTS "${OPENCV_LIB_DIR}")
        # 尝试使用相对路径（从可执行文件到 OpenCV 库）
        # Try to use relative path (from executable to OpenCV library)
        # PoSDK 可执行文件位置: build/output/bin/PoSDK
        # OpenCV 库位置: dependencies/opencv/install_local/lib
        
        # 计算从 build/output/bin/ 到 dependencies/opencv/install_local/lib/ 的相对路径
        # Calculate relative path from build/output/bin/ to dependencies/opencv/install_local/lib/
        set(OPENCV_RELATIVE_PATH "\$ORIGIN/../../../dependencies/opencv/install_local/lib")
        
        # 同时添加相对路径和绝对路径（相对路径优先）
        # Add both relative and absolute paths (relative path takes priority)
        list(APPEND RPATH_PATHS "${OPENCV_RELATIVE_PATH}")
        list(APPEND RPATH_PATHS "${OPENCV_LIB_DIR}")
        
        message(STATUS "PoSDK executable: Adding OpenCV to RPATH:")
        message(STATUS "  Relative path: ${OPENCV_RELATIVE_PATH}")
        message(STATUS "  Absolute path: ${OPENCV_LIB_DIR}")
    endif()
    
    # 合并所有 RPATH 路径
    # Merge all RPATH paths
    list(REMOVE_DUPLICATES RPATH_PATHS)
    string(REPLACE ";" ":" RPATH_STRING "${RPATH_PATHS}")
    
    set_target_properties(PoSDK PROPERTIES
        INSTALL_RPATH "${RPATH_STRING}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
    message(STATUS "PoSDK executable INSTALL_RPATH: ${RPATH_STRING}")
endif()

# 安装规则
install(TARGETS PoSDK
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "PoSDK 可执行程序配置:")
message(STATUS "  - C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - 使用 po_core 目标: PoSDK::po_core")
message(STATUS "  - 输出目录: ${OUTPUT_BIN_DIR}")
message(STATUS "  - 包含目录: ${OUTPUT_INCLUDE_DIR}")
