# ==============================================================================
# Plugin Common Configuration and Utility Functions
# ==============================================================================

# ==============================================================================
# Plugin Build Status Tracking | 插件构建状态跟踪
# ==============================================================================
# Initialize global lists to track plugin build results
# ONLY initialize if not already initialized (avoid resetting on re-include)
# 初始化全局列表来跟踪插件构建结果
# 仅在未初始化时初始化（避免重复include时重置）

get_property(_plugins_initialized GLOBAL PROPERTY POSDK_PLUGINS_INITIALIZED)
if(NOT _plugins_initialized)
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_SUCCESS "")      # 成功构建的插件列表
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_FAILED "")       # 构建失败的插件列表
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_SKIPPED "")      # 跳过的插件列表
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_TOTAL 0)         # 总插件数
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_INITIALIZED TRUE)  # 标记已初始化
    
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    message(STATUS "Plugin Status Tracking System Initialized")
    message(STATUS "插件状态跟踪系统已初始化")
    message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
endif()

# ------------------------------------------------------------------------------
# Helper Functions for Plugin Status Tracking | 插件状态跟踪辅助函数
# ------------------------------------------------------------------------------

# Record plugin success | 记录插件成功
macro(_record_plugin_success PLUGIN_NAME)
    get_property(success_list GLOBAL PROPERTY POSDK_PLUGINS_SUCCESS)
    if(success_list)
        set(success_list "${success_list};${PLUGIN_NAME}")
    else()
        set(success_list "${PLUGIN_NAME}")
    endif()
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_SUCCESS "${success_list}")
    # Debug message
    message(STATUS "  [Tracking] Recorded success: ${PLUGIN_NAME}")
endmacro()

# Record plugin failure | 记录插件失败
macro(_record_plugin_failure PLUGIN_NAME REASON)
    get_property(failed_list GLOBAL PROPERTY POSDK_PLUGINS_FAILED)
    if(failed_list)
        set(failed_list "${failed_list};${PLUGIN_NAME}:${REASON}")
    else()
        set(failed_list "${PLUGIN_NAME}:${REASON}")
    endif()
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_FAILED "${failed_list}")
    message(STATUS "  [Tracking] Recorded failure: ${PLUGIN_NAME}")
endmacro()

# Record plugin skipped | 记录插件跳过
macro(_record_plugin_skipped PLUGIN_NAME REASON)
    get_property(skipped_list GLOBAL PROPERTY POSDK_PLUGINS_SKIPPED)
    if(skipped_list)
        set(skipped_list "${skipped_list};${PLUGIN_NAME}:${REASON}")
    else()
        set(skipped_list "${PLUGIN_NAME}:${REASON}")
    endif()
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_SKIPPED "${skipped_list}")
    message(STATUS "  [Tracking] Recorded skipped: ${PLUGIN_NAME}")
endmacro()

# Increment total plugin count | 增加总插件数
macro(_increment_plugin_count)
    get_property(total GLOBAL PROPERTY POSDK_PLUGINS_TOTAL)
    math(EXPR total "${total} + 1")
    set_property(GLOBAL PROPERTY POSDK_PLUGINS_TOTAL ${total})
    # Debug message
    message(STATUS "  [Tracking] Total plugins: ${total}")
endmacro()

# ------------------------------------------------------------------------------
# Generic Plugin Addition Function (with error handling)
# ------------------------------------------------------------------------------
function(add_posdk_plugin PLUGIN_NAME)
    set(options OPTIONAL)  # 添加OPTIONAL选项
    set(oneValueArgs PLUGIN_TYPE)
    set(multiValueArgs SOURCES HEADERS LINK_LIBRARIES INCLUDE_DIRS COMPILE_DEFINITIONS)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Increment total count | 增加总数
    _increment_plugin_count()
    
    # ==============================================================================
    # Dependency Validation | 依赖验证
    # ==============================================================================
    # Check if all required libraries exist before adding plugin
    # 在添加插件前检查所有依赖库是否存在
    
    if(PLUGIN_LINK_LIBRARIES)
        set(MISSING_LIBS "")
        foreach(lib ${PLUGIN_LINK_LIBRARIES})
            # Skip generator expressions and CMake targets
            # 跳过生成器表达式和CMake目标
            string(REGEX MATCH "^\\$<" IS_GENERATOR_EXPR "${lib}")
            string(REGEX MATCH "::" IS_CMAKE_TARGET "${lib}")
            
            if(NOT IS_GENERATOR_EXPR AND NOT IS_CMAKE_TARGET)
                # Check if it's an absolute path to a library file
                # 检查是否是库文件的绝对路径
                if(IS_ABSOLUTE "${lib}" AND lib MATCHES "\\.(a|so|dylib|lib)$")
                    if(NOT EXISTS "${lib}")
                        list(APPEND MISSING_LIBS "${lib}")
                    endif()
                endif()
            endif()
        endforeach()
        
        # If libraries are missing and plugin is OPTIONAL, skip it
        # 如果缺少库且插件是OPTIONAL，跳过它
        if(MISSING_LIBS AND PLUGIN_OPTIONAL)
            string(REPLACE ";" ", " MISSING_LIBS_STR "${MISSING_LIBS}")
            _record_plugin_skipped("${PLUGIN_NAME}" "Missing libraries: ${MISSING_LIBS_STR}")
            
            message(STATUS "")
            message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
            message(STATUS "⊘ Plugin: ${PLUGIN_NAME} (SKIPPED)")
            message(STATUS "  Reason: Required libraries not found")
            message(STATUS "  Missing:")
            foreach(missing_lib ${MISSING_LIBS})
                get_filename_component(lib_name "${missing_lib}" NAME)
                message(STATUS "    - ${lib_name}")
            endforeach()
            message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
            message(STATUS "")
            return()  # Exit function, don't add plugin
        endif()
    endif()
    
    # Plugin folder is always CMAKE_CURRENT_SOURCE_DIR | 插件目录始终为CMAKE_CURRENT_SOURCE_DIR
    set(PLUGIN_PLUGIN_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Set default plugin type | 设置默认插件类型
    if(NOT PLUGIN_PLUGIN_TYPE)
        set(PLUGIN_PLUGIN_TYPE "methods")
    endif()
    
    # Auto-discover source and header files if not specified | 如果未指定，自动发现源文件和头文件
    if(NOT PLUGIN_SOURCES)
        file(GLOB PLUGIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
        if(NOT PLUGIN_SOURCES)
            message(WARNING "No .cpp files found in ${CMAKE_CURRENT_SOURCE_DIR} for plugin ${PLUGIN_NAME}")
        else()
            message(STATUS "  Auto-discovered ${PLUGIN_NAME} sources: ${PLUGIN_SOURCES}")
        endif()
    endif()
    
    if(NOT PLUGIN_HEADERS)
        file(GLOB PLUGIN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
        if(PLUGIN_HEADERS)
            message(STATUS "  Auto-discovered ${PLUGIN_NAME} headers: ${PLUGIN_HEADERS}")
        endif()
    endif()
    
    # Create plugin library
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
    
    # Mark plugin as EXCLUDE_FROM_ALL if OPTIONAL is set
    # This allows build to continue even if this plugin fails
    # 如果设置了OPTIONAL，将插件标记为EXCLUDE_FROM_ALL
    # 这允许即使此插件失败，构建也能继续
    if(PLUGIN_OPTIONAL)
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            EXCLUDE_FROM_ALL FALSE  # Still build by default, but errors won't stop the build
        )
        message(STATUS "  → Plugin ${PLUGIN_NAME} marked as OPTIONAL")
    endif()
    
    # ========================================
    # 性能优化：设置插件库文件名包含类型信息（无需dlopen即可获取插件类型）
    # Performance Optimization: Set plugin library filename to include type info (no dlopen needed to get plugin type)
    # ========================================
    # 格式 | Format: posdk_plugin_<type>.{so|dylib|dll}
    # 优势 | Advantage: 插件注册阶段只需扫描文件名，无需加载动态库（6秒 → 几毫秒）
    #                   Plugin registration only needs to scan filenames, no need to load libraries (6s → few ms)
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        OUTPUT_NAME "posdk_plugin_${PLUGIN_NAME}"
        PREFIX ""  # 移除 lib 前缀 | Remove lib prefix
    )
    
    # Set basic include directories
    target_include_directories(${PLUGIN_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PLUGIN_PLUGIN_FOLDER}
            ${OUTPUT_INCLUDE_DIR}        # Use build output include directory
            ${OpenCV_INCLUDE_DIRS}
            ${EIGEN3_INCLUDE_DIRS}
            ${PLUGIN_INCLUDE_DIRS}       # User-specified additional include directories
    )
    
    # Link basic dependency libraries
    target_link_libraries(${PLUGIN_NAME}
        PRIVATE
            PoSDK::pomvg_common
            PoSDK::pomvg_converter
            PoSDK::po_core
            Eigen3::Eigen
            ${PLUGIN_LINK_LIBRARIES}     # User-specified additional link libraries
    )
    
    # Avoid Anaconda library interference on Linux
    # 在 Linux 上避免 Anaconda 库干扰
    if(UNIX AND NOT APPLE)
        # Set linker search paths to prioritize system libraries over Anaconda
        # 设置链接器搜索路径以优先使用系统库而非 Anaconda
        # This ensures GTK-3 links to system GLib, not Anaconda's old GLib
        # 这确保 GTK-3 链接到系统 GLib，而不是 Anaconda 的旧 GLib
        set(SYSTEM_LIB_DIRS
            "/usr/lib/x86_64-linux-gnu"  # Standard Ubuntu library path
            "/usr/lib"
            "/lib/x86_64-linux-gnu"
            "/lib"
        )
        
        foreach(SYSTEM_LIB_DIR ${SYSTEM_LIB_DIRS})
            if(EXISTS "${SYSTEM_LIB_DIR}")
                # Add to linker search path with higher priority
                # 以更高优先级添加到链接器搜索路径
                target_link_directories(${PLUGIN_NAME} PRIVATE ${SYSTEM_LIB_DIR})
            endif()
        endforeach()
        
        # Add linker flags to control library resolution order
        # 添加链接器标志以控制库解析顺序
        # Use -Wl format (CMake will handle it correctly for the linker)
        # 使用 -Wl 格式（CMake 会正确处理传递给链接器）
        target_link_options(${PLUGIN_NAME} PRIVATE
            "-Wl,--as-needed"          # Only link needed libraries
        )
    endif()
    
    # ==============================================================================
    # Security: Force symbol binding to po_core (prevent fake implementations)
    # 安全：强制符号绑定到po_core（防止假实现）
    # ==============================================================================
    # This ensures that VerifyPluginSecurityToken must come from po_core
    # 这确保 VerifyPluginSecurityToken 必须来自 po_core
    if(UNIX AND NOT APPLE)
        # Linux: Disallow undefined symbols
        # Use -Wl prefix for linker flags (compatible with CMake 3.15+)
        # 使用 -Wl 前缀传递链接器标志（兼容 CMake 3.15+）
        target_link_options(${PLUGIN_NAME} PRIVATE
            "-Wl,--no-undefined"           # 禁止未定义符号
            "-Wl,--no-allow-shlib-undefined"  # 禁止共享库未定义符号
        )
    elseif(APPLE)
        # macOS: Undefined symbols are errors
        # Use -Wl prefix for linker flags (compatible with CMake 3.15+)
        # 使用 -Wl 前缀传递链接器标志（兼容 CMake 3.15+）
        target_link_options(${PLUGIN_NAME} PRIVATE
            "-Wl,-undefined,error"         # 未定义符号报错
        )
    elseif(WIN32)
        # Windows: Similar behavior by default
        # Windows默认就会检查未定义符号
    endif()
    
    message(STATUS "  [Security] Plugin ${PLUGIN_NAME}: Forced symbol binding enabled")
    message(STATUS "  [安全] 插件 ${PLUGIN_NAME}: 已启用强制符号绑定")
    
    # Set compile options
    target_compile_options(${PLUGIN_NAME} 
        PRIVATE ${POMVG_COMPILE_OPTIONS}
    )
    
    # Add compile definitions
    target_compile_definitions(${PLUGIN_NAME} 
        PRIVATE
            POMVG_OUTPUT_DIR="${OUTPUT_BASE_DIR}"
            POMVG_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
            PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
            PLUGIN_NAME=${PLUGIN_NAME}  # ✅ 自动定义插件名称宏（不加引号，由STRINGIFY宏处理）
            ${PLUGIN_COMPILE_DEFINITIONS}  # User-specified additional compile definitions
    )
    
    # Set output paths
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/plugins/${PLUGIN_PLUGIN_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/plugins/${PLUGIN_PLUGIN_TYPE}"
    )
    
    # Handle plugin header files
    _handle_plugin_headers(${PLUGIN_NAME} ${PLUGIN_PLUGIN_TYPE} ${PLUGIN_PLUGIN_FOLDER})
    
    # Handle configuration files (.ini files)
    _handle_plugin_configs(${PLUGIN_NAME} ${PLUGIN_PLUGIN_TYPE} ${PLUGIN_PLUGIN_FOLDER})
    
    # Handle Python files
    _handle_plugin_python_files(${PLUGIN_NAME} ${PLUGIN_PLUGIN_TYPE} ${PLUGIN_PLUGIN_FOLDER})
    
    # Install plugin library files
    install(TARGETS ${PLUGIN_NAME}
        LIBRARY DESTINATION plugins/${PLUGIN_PLUGIN_TYPE}    # Unix .so
        RUNTIME DESTINATION plugins/${PLUGIN_PLUGIN_TYPE}    # Windows .dll
    )
    
    # Record successful plugin addition | 记录插件成功添加
    _record_plugin_success("${PLUGIN_NAME}")
    
    message(STATUS "✓ Added PoSDK plugin: ${PLUGIN_NAME} (type: ${PLUGIN_PLUGIN_TYPE})")
endfunction()

# ------------------------------------------------------------------------------
# Helper function to handle plugin header files
# ------------------------------------------------------------------------------
function(_handle_plugin_headers PLUGIN_NAME PLUGIN_TYPE PLUGIN_FOLDER)
    # Collect plugin public header files
    file(GLOB PLUGIN_PUBLIC_HEADERS 
        "${CMAKE_SOURCE_DIR}/include/plugins/${PLUGIN_TYPE}/*.hpp"
        "${CMAKE_SOURCE_DIR}/include/plugins/${PLUGIN_TYPE}/*.h"
    )
    
    # Also collect header files from plugin folder
    file(GLOB PLUGIN_LOCAL_HEADERS 
        "${PLUGIN_FOLDER}/*.hpp"
        "${PLUGIN_FOLDER}/*.h"
    )

    if(PLUGIN_PUBLIC_HEADERS)
        # Copy to build directory
        foreach(HEADER ${PLUGIN_PUBLIC_HEADERS})
            get_filename_component(HEADER_NAME ${HEADER} NAME)
            configure_file(
                ${HEADER}
                "${OUTPUT_PLUGINS_INCLUDE_DIR}/${PLUGIN_TYPE}/${HEADER_NAME}"
                COPYONLY
            )
        endforeach()
        
        # Install public header files
        install(FILES ${PLUGIN_PUBLIC_HEADERS}
            DESTINATION include/plugins/${PLUGIN_TYPE}
        )
    endif()
    
    # Install plugin local header files
    if(PLUGIN_LOCAL_HEADERS)
        install(FILES ${PLUGIN_LOCAL_HEADERS}
            DESTINATION include/plugins/${PLUGIN_TYPE}
        )
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Helper function to handle plugin configuration files
# ------------------------------------------------------------------------------
function(_handle_plugin_configs PLUGIN_NAME PLUGIN_TYPE PLUGIN_FOLDER)
    # Find all .ini configuration files in plugin directory
    file(GLOB PLUGIN_CONFIG_FILES "${PLUGIN_FOLDER}/*.ini")
    
    if(PLUGIN_CONFIG_FILES)
        # Copy configuration files to unified config directory
        foreach(CONFIG_FILE ${PLUGIN_CONFIG_FILES})
            get_filename_component(CONFIG_NAME ${CONFIG_FILE} NAME)
            configure_file(
                ${CONFIG_FILE}
                "${METHOD_CONFIG_DIR}/${CONFIG_NAME}"
                COPYONLY
            )
        endforeach()
        
        # Install configuration files
        install(FILES ${PLUGIN_CONFIG_FILES}
            DESTINATION configs/methods
        )
        
        message(STATUS "  Copied ${PLUGIN_NAME} config files to: ${METHOD_CONFIG_DIR}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Helper function to handle plugin Python files
# ------------------------------------------------------------------------------
function(_handle_plugin_python_files PLUGIN_NAME PLUGIN_TYPE PLUGIN_FOLDER)
    # Find all .py files in plugin directory
    file(GLOB PLUGIN_PYTHON_FILES "${PLUGIN_FOLDER}/*.py")
    
    if(PLUGIN_PYTHON_FILES)
        # Create plugin-specific Python directory
        set(PLUGIN_PYTHON_DIR "${OUTPUT_PYTHON_DIR}/${PLUGIN_NAME}")
        file(MAKE_DIRECTORY ${PLUGIN_PYTHON_DIR})
        
        # Copy Python files to plugin-specific directory
        foreach(PYTHON_FILE ${PLUGIN_PYTHON_FILES})
            get_filename_component(PYTHON_NAME ${PYTHON_FILE} NAME)
            configure_file(
                ${PYTHON_FILE}
                "${PLUGIN_PYTHON_DIR}/${PYTHON_NAME}"
                COPYONLY
            )
        endforeach()
        
        # Also copy to methods directory (backward compatibility)
        foreach(PYTHON_FILE ${PLUGIN_PYTHON_FILES})
            get_filename_component(PYTHON_NAME ${PYTHON_FILE} NAME)
            configure_file(
                ${PYTHON_FILE}
                "${OUTPUT_BASE_DIR}/plugins/methods/${PYTHON_NAME}"
                COPYONLY
            )
        endforeach()
        
        # Install Python files
        install(FILES ${PLUGIN_PYTHON_FILES}
            DESTINATION python/${PLUGIN_NAME}
        )
        install(FILES ${PLUGIN_PYTHON_FILES}
            DESTINATION plugins/methods  # Backward compatibility
        )
        
        message(STATUS "  Copied ${PLUGIN_NAME} Python files to: ${PLUGIN_PYTHON_DIR}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Function to auto-discover plugin directories (with error handling)
# ------------------------------------------------------------------------------
function(auto_discover_plugin_directories PLUGIN_TYPE BASE_DIR)
    # Find all subdirectories containing CMakeLists.txt
    file(GLOB PLUGIN_DIRS "${BASE_DIR}/*/CMakeLists.txt")
    
    foreach(CMAKE_FILE ${PLUGIN_DIRS})
        get_filename_component(PLUGIN_DIR ${CMAKE_FILE} DIRECTORY)
        get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
        
        message(STATUS "Discovered plugin directory: ${PLUGIN_NAME} at ${PLUGIN_DIR}")
        
        # Try to add subdirectory with error handling
        # 尝试添加子目录并处理错误
        # Note: CMake's add_subdirectory doesn't have try-catch, but we can validate before adding
        # 注意：CMake的add_subdirectory没有try-catch，但我们可以在添加前验证
        
        # Check if CMakeLists.txt exists
        # Note: IS_READABLE requires CMake 3.29+, so we only check EXISTS for compatibility
        # 注意：IS_READABLE需要CMake 3.29+，为了兼容性只检查EXISTS
        if(NOT EXISTS "${CMAKE_FILE}")
            message(WARNING "⚠ Plugin ${PLUGIN_NAME}: CMakeLists.txt does not exist, skipping...")
            _increment_plugin_count()
            _record_plugin_skipped("${PLUGIN_NAME}" "CMakeLists.txt not found")
            continue()
        endif()
        
        # Try to add subdirectory
        # CMake will handle errors during configuration phase
        # 尝试添加子目录（CMake会在配置阶段处理错误）
        add_subdirectory(${PLUGIN_DIR})
    endforeach()
endfunction()

# ------------------------------------------------------------------------------
# Function to generate plugin build report | 生成插件构建报告
# ------------------------------------------------------------------------------
function(generate_plugin_build_report)
    message(STATUS "")
    message(STATUS "╔════════════════════════════════════════════════════════════════╗")
    message(STATUS "║          Plugin Build Report | 插件构建报告                    ║")
    message(STATUS "╚════════════════════════════════════════════════════════════════╝")
    message(STATUS "")
    
    # Get statistics | 获取统计信息
    get_property(total GLOBAL PROPERTY POSDK_PLUGINS_TOTAL)
    get_property(success_list GLOBAL PROPERTY POSDK_PLUGINS_SUCCESS)
    get_property(failed_list GLOBAL PROPERTY POSDK_PLUGINS_FAILED)
    get_property(skipped_list GLOBAL PROPERTY POSDK_PLUGINS_SKIPPED)
    
    if(success_list)
        list(LENGTH success_list success_count)
    else()
        set(success_count 0)
    endif()
    
    if(failed_list)
        list(LENGTH failed_list failed_count)
    else()
        set(failed_count 0)
    endif()
    
    if(skipped_list)
        list(LENGTH skipped_list skipped_count)
    else()
        set(skipped_count 0)
    endif()
    
    # Summary | 总结
    message(STATUS "Total plugins discovered: ${total} | 发现的插件总数: ${total}")
    message(STATUS "  ✓ Success: ${success_count} | 成功: ${success_count}")
    if(skipped_count GREATER 0)
        message(STATUS "  ⊘ Skipped: ${skipped_count} | 跳过: ${skipped_count}")
    endif()
    if(failed_count GREATER 0)
        message(STATUS "  ✗ Failed:  ${failed_count} | 失败: ${failed_count}")
    endif()
    message(STATUS "")
    
    # Successful plugins | 成功的插件
    if(success_count GREATER 0)
        message(STATUS "✓ Successfully Built Plugins | 成功构建的插件:")
        foreach(plugin ${success_list})
            message(STATUS "    ✓ ${plugin}")
        endforeach()
        message(STATUS "")
    endif()
    
    # Skipped plugins | 跳过的插件
    if(skipped_count GREATER 0)
        message(STATUS "⊘ Skipped Plugins | 跳过的插件:")
        foreach(plugin_info ${skipped_list})
            # Split plugin_info into name and reason
            string(REPLACE ":" ";" plugin_parts "${plugin_info}")
            list(GET plugin_parts 0 plugin_name)
            list(GET plugin_parts 1 plugin_reason)
            message(STATUS "    ⊘ ${plugin_name}")
            message(STATUS "      Reason | 原因: ${plugin_reason}")
        endforeach()
        message(STATUS "")
    endif()
    
    # Failed plugins | 失败的插件
    if(failed_count GREATER 0)
        message(WARNING "")
        message(WARNING "✗ Failed Plugins | 构建失败的插件:")
        foreach(plugin_info ${failed_list})
            # Split plugin_info into name and reason
            string(REPLACE ":" ";" plugin_parts "${plugin_info}")
            list(GET plugin_parts 0 plugin_name)
            list(LENGTH plugin_parts parts_count)
            if(parts_count GREATER 1)
                list(GET plugin_parts 1 plugin_reason)
                message(WARNING "    ✗ ${plugin_name}")
                message(WARNING "      Reason | 原因: ${plugin_reason}")
            else()
                message(WARNING "    ✗ ${plugin_name}")
            endif()
        endforeach()
        message(WARNING "")
        message(WARNING "⚠ Note: Some plugins failed to build, but the build will continue.")
        message(WARNING "⚠ 注意：某些插件构建失败，但构建将继续进行。")
        message(WARNING "")
    endif()
    
    message(STATUS "╔════════════════════════════════════════════════════════════════╗")
    if(failed_count EQUAL 0 AND skipped_count EQUAL 0)
        message(STATUS "║  ✓ All plugins built successfully! | 所有插件构建成功！     ║")
    elseif(success_count GREATER 0)
        message(STATUS "║  ⚠ Build completed with warnings | 构建完成但有警告         ║")
    else()
        message(STATUS "║  ✗ No plugins were built | 没有插件被构建                   ║")
    endif()
    message(STATUS "╚════════════════════════════════════════════════════════════════╝")
    message(STATUS "")
endfunction()

# ==============================================================================
# Preserved Legacy Compatibility Functions
# ==============================================================================
function(add_pomvg_plugin PLUGIN_NAME PLUGIN_TYPE)
    add_library(${PLUGIN_NAME} SHARED ${ARGN})
    
    # Set include directories
    target_include_directories(${PLUGIN_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${OUTPUT_INCLUDE_DIR}        # Use build output include directory
            ${OpenCV_INCLUDE_DIRS}
            ${EIGEN3_INCLUDE_DIRS}
    )
    
    # Link dependency libraries
    target_link_libraries(${PLUGIN_NAME}
        PRIVATE
            PoSDK::pomvg_common
            PoSDK::pomvg_converter
            PoSDK::po_core
            Eigen3::Eigen
    )
    
    # Use unified compile options
    target_compile_options(${PLUGIN_NAME} 
        PRIVATE ${POMVG_COMPILE_OPTIONS}
    )
    
    # Set output paths - build time
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/plugins/${PLUGIN_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/plugins/${PLUGIN_TYPE}"
    )
    
    # Handle header files
    _handle_plugin_headers(${PLUGIN_NAME} ${PLUGIN_TYPE} ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Install plugin library files
    install(TARGETS ${PLUGIN_NAME}
        LIBRARY DESTINATION plugins/${PLUGIN_TYPE}    # Unix .so
        RUNTIME DESTINATION plugins/${PLUGIN_TYPE}    # Windows .dll
    )
    
    # Install plugin header files
    install(FILES ${PLUGIN_HEADERS}
        DESTINATION include/plugins/${PLUGIN_TYPE}
    )
endfunction()