# ==============================================================================
# PoSDK 方法插件构建系统
# 作者: Qi Cai
# 版权: Copyright (c) 2021-2024
# 说明: 本CMakeLists.txt支持两种插件添加方式：
#   1. 新方式：独立插件目录，每个插件有自己的CMakeLists.txt
#   2. 旧方式：传统的单一CMakeLists.txt管理所有插件（向后兼容）
# ==============================================================================

cmake_minimum_required(VERSION 3.15)
project(method_plugins LANGUAGES CXX)

# 包含插件配置工具
include(../cmake/plugin-config.cmake)

# -----------------------------------------------------------------------------
# 自动发现插件目录
# -----------------------------------------------------------------------------
message(STATUS "=== Auto-discovering PoSDK Method Plugins ===")

# 自动发现所有包含CMakeLists.txt的插件子目录
auto_discover_plugin_directories("methods" ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "=== End of Auto-discovery ===")

# ==============================================================================
# 传统插件支持（向后兼容）
# ==============================================================================

# -----------------------------------------------------------------------------
# 传统插件列表定义
# -----------------------------------------------------------------------------
set(LEGACY_METHOD_PLUGINS
    # method_img2features_plugin            # 图像特征提取插件 (已迁移到独立插件目录: Img2Features/)
    # method_img2matches_plugin             # 特征匹配插件 (已迁移到独立插件目录: Img2Matches/)
    # method_calibrator_plugin              # 相机标定插件 (已迁移到独立插件目录: MethodCalibrator/)
    # method_matches_visualizer             # 匹配可视化插件 (已迁移到独立插件目录: MethodMatchesVisualizer/)
    # barath_two_view_estimator             # Barath两视图位姿估计插件 (已迁移到独立插件目录: BarathTwoViewEstimator/)
    # method_rotation_averaging_Chatterjee  # 旋转平均插件 (已迁移到独立插件目录: MethodRotationAveragingChatterjee/)
    # method_rotation_averaging             # 旋转平均插件 (已迁移到独立插件目录: MethodRotationAveraging/)
    # opencv_two_view_estimator             # 两视图位姿估计插件 (已迁移到独立插件目录: OpenCVTwoViewEstimator/)
    # opengv_model_estimator                # 双视图位姿估计插件 (已迁移到独立插件目录: OpenGVModelEstimator/)
    # openmvg_pipeline                      # OpenMVG预处理插件 (已迁移到独立插件目录: OpenMVGPipeline/)
    # poselib_model_estimator               # 双视图位姿估计插件 (已迁移到独立插件目录: PoseLibModelEstimator/)
    # two_view_estimator                     # 两视图位姿估计插件
    # method_rotation_refine_by_features     # 基于特征的旋转优化插件
)

# -----------------------------------------------------------------------------
# 特定依赖包配置（传统插件需要）
# -----------------------------------------------------------------------------
# OpenGL 相关配置已迁移到 MethodCalibrator/CMakeLists.txt
# find_package(OpenGL REQUIRED)
# find_package(glfw3 REQUIRED)

# -----------------------------------------------------------------------------
# MAGSAC和SupeRANSAC库配置
# -----------------------------------------------------------------------------
# 设置MAGSAC库路径
set(MAGSAC_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/magsac-master/install_local")
set(MAGSAC_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/magsac-master/build_local")

# 设置SupeRANSAC库路径
set(SUPERANSAC_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/superansac-main/install_local")
set(SUPERANSAC_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/superansac-main/build_local")
set(SUPERANSAC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/superansac-main")

# -----------------------------------------------------------------------------
# OpenMP配置（继承自根目录配置）
# -----------------------------------------------------------------------------
# OpenMP配置已在根目录CMakeLists.txt中统一处理
# 这里只需要检查OpenMP是否可用
if(OpenMP_CXX_FOUND)
    message(STATUS "Methods plugins: OpenMP support available")
else()
    message(STATUS "Methods plugins: OpenMP not available, multi-threading disabled")
endif()

# 检查MAGSAC库是否存在
if(EXISTS "${MAGSAC_BUILD_DIR}/libMAGSAC.a")
    message(STATUS "Found MAGSAC library at: ${MAGSAC_BUILD_DIR}/libMAGSAC.a")
    set(MAGSAC_LIBRARIES "${MAGSAC_BUILD_DIR}/libMAGSAC.a" "${MAGSAC_BUILD_DIR}/libGraphCutRANSAC.a")
    set(MAGSAC_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/magsac-master/src/pymagsac/include")
    set(MAGSAC_FOUND TRUE)
else()
    message(WARNING "MAGSAC library not found. Please run install_magsac.sh first.")
    set(MAGSAC_FOUND FALSE)
endif()

# 检查SupeRANSAC库是否存在
if(EXISTS "${SUPERANSAC_BUILD_DIR}/src/libSupeRANSAC.dylib")
    message(STATUS "Found SupeRANSAC library at: ${SUPERANSAC_BUILD_DIR}/src/libSupeRANSAC.dylib")
    set(SUPERANSAC_LIBRARIES "${SUPERANSAC_BUILD_DIR}/src/libSupeRANSAC.dylib")
    set(SUPERANSAC_INCLUDE_DIRS "${SUPERANSAC_SRC_DIR}/include")
else()
    message(WARNING "SupeRANSAC library not found. Please run install_superansac.sh first.")
endif()

# -----------------------------------------------------------------------------
# PoseLib库配置 (参考MAGSAC的配置模式)
# -----------------------------------------------------------------------------
# 设置PoseLib库路径 (与dependencies/PoseLib目录结构一致)
set(POSELIB_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/PoseLib/install_local")
set(POSELIB_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/PoseLib/build_local")

# 检查PoseLib库是否存在
if(EXISTS "${POSELIB_BUILD_DIR}/libPoseLib.a" OR EXISTS "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
    if(EXISTS "${POSELIB_BUILD_DIR}/libPoseLib.a")
        message(STATUS "Found PoseLib library at: ${POSELIB_BUILD_DIR}/libPoseLib.a")
        set(POSELIB_LIBRARIES "${POSELIB_BUILD_DIR}/libPoseLib.a")
        # 头文件在源码目录的PoseLib子目录中
        set(POSELIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/PoseLib")
    else()
        message(STATUS "Found PoseLib library at: ${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
        set(POSELIB_LIBRARIES "${POSELIB_INSTALL_DIR}/lib/libPoseLib.a")
        # 安装后的头文件在include目录中
        set(POSELIB_INCLUDE_DIRS "${POSELIB_INSTALL_DIR}/include")
    endif()
    set(POSELIB_FOUND TRUE)
else()
    message(WARNING "PoseLib library not found. Please run src/dependencies/install_poselib.sh first.")
    set(POSELIB_FOUND FALSE)
endif()

# -----------------------------------------------------------------------------
# 传统插件构建（仅构建存在源文件的插件）
# -----------------------------------------------------------------------------
message(STATUS "=== Building Legacy Method Plugins ===")

foreach(PLUGIN ${LEGACY_METHOD_PLUGINS})
    # 检查源文件是否存在
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}.cpp")
        message(STATUS "Building legacy plugin: ${PLUGIN}")
        add_pomvg_plugin(${PLUGIN} methods
            ${PLUGIN}.cpp
            ${PLUGIN}.hpp
        )
    else()
        message(STATUS "Skipping legacy plugin (source not found): ${PLUGIN}")
    endif()
endforeach()

# -----------------------------------------------------------------------------
# 传统插件特定依赖配置
# -----------------------------------------------------------------------------
# 特征提取插件依赖 (已迁移到独立插件目录: Img2Features/)
# if(TARGET method_img2features_plugin)
#     target_link_libraries(method_img2features_plugin PRIVATE ${OpenCV_LIBS})
# endif()

# 特征匹配插件依赖 (已迁移到独立插件目录，由其自身的CMakeLists.txt管理)
# if(TARGET method_img2matches_plugin)
#     target_link_libraries(method_img2matches_plugin
#         PRIVATE
#             method_img2features_plugin
#             ${OpenCV_LIBS}
#     )
# endif()

# 新的独立目录插件的依赖将由各自的 CMakeLists.txt 管理
# - Img2Matches/CMakeLists.txt: method_img2matches
# - Img2Features/CMakeLists.txt: method_img2features
# - MethodCalibrator/CMakeLists.txt: method_calibrator
# - MethodMatchesVisualizer/CMakeLists.txt: method_matches_visualizer
# - BarathTwoViewEstimator/CMakeLists.txt: barath_two_view_estimator
# - MethodRotationAveragingChatterjee/CMakeLists.txt: method_rotation_averaging_chatterjee
# - MethodRotationAveraging/CMakeLists.txt: method_rotation_averaging
# - OpenCVTwoViewEstimator/CMakeLists.txt: opencv_two_view_estimator
# - OpenGVModelEstimator/CMakeLists.txt: opengv_model_estimator
# - OpenMVGPipeline/CMakeLists.txt: openmvg_pipeline
# - PoseLibModelEstimator/CMakeLists.txt: poselib_model_estimator

# openmvg_pipeline 插件依赖已迁移到 OpenMVGPipeline/CMakeLists.txt
# poselib_model_estimator 插件依赖已迁移到 PoseLibModelEstimator/CMakeLists.txt
# method_rotation_averaging_Chatterjee 插件依赖已迁移到 MethodRotationAveragingChatterjee/CMakeLists.txt
# method_rotation_averaging 插件依赖已迁移到 MethodRotationAveraging/CMakeLists.txt

# two_view_estimator 插件OpenMP支持
if(TARGET two_view_estimator)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(two_view_estimator PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(two_view_estimator PRIVATE USE_OPENMP)
        message(STATUS "two_view_estimator: OpenMP support enabled")
    else()
        message(STATUS "two_view_estimator: OpenMP not found, using single-threaded execution")
    endif()
endif()

# barath_two_view_estimator 插件依赖已迁移到 BarathTwoViewEstimator/CMakeLists.txt

# method_rotation_refine_by_features 插件依赖
if(TARGET method_rotation_refine_by_features)
    target_link_libraries(method_rotation_refine_by_features
        PRIVATE
            ${OpenCV_LIBS}
    )
    message(STATUS "method_rotation_refine_by_features: OpenCV support enabled")
endif()

if(TARGET superansac)
    include_directories(${CMAKE_SOURCE_DIR}/dependencies/superansac-main/include)
    link_directories(${CMAKE_SOURCE_DIR}/dependencies/superansac-main/build_local/src)
endif()

# -----------------------------------------------------------------------------
# 传统配置文件处理
# -----------------------------------------------------------------------------
# 复制所有配置文件到构建目录
file(GLOB METHOD_CONFIG_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/*.ini"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ini"
)
foreach(CONFIG_FILE ${METHOD_CONFIG_FILES})
    get_filename_component(CONFIG_NAME ${CONFIG_FILE} NAME)
    configure_file(
        ${CONFIG_FILE}
        "${METHOD_CONFIG_DIR}/${CONFIG_NAME}"
        COPYONLY
    )
endforeach()

# -----------------------------------------------------------------------------
# 传统编译定义配置
# -----------------------------------------------------------------------------
foreach(PLUGIN ${LEGACY_METHOD_PLUGINS})
    if(TARGET ${PLUGIN})
        target_compile_definitions(${PLUGIN} 
            PRIVATE
                POMVG_OUTPUT_DIR="${OUTPUT_BASE_DIR}"
                POMVG_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        )
    endif()
endforeach()

# -----------------------------------------------------------------------------
# 安装配置
# -----------------------------------------------------------------------------
if(METHOD_CONFIG_FILES)
    install(FILES ${METHOD_CONFIG_FILES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins/methods/configs
    )
endif()

# -----------------------------------------------------------------------------
# 构建总结
# -----------------------------------------------------------------------------
message(STATUS "=== PoSDK Method Plugins Build Summary ===")
message(STATUS "Plugin discovery completed.")
message(STATUS "Legacy plugins processed.")
message(STATUS "============================================")

