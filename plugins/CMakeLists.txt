# ==============================================================================
# 插件系统主配置
# ==============================================================================
set(PLUGIN_DIR "${OUTPUT_BASE_DIR}/plugins")
set(OUTPUT_PLUGINS_INCLUDE_DIR "${OUTPUT_INCLUDE_DIR}/plugins")
set(DATA_PLUGIN_DIR "${OUTPUT_PLUGINS_INCLUDE_DIR}/data")
set(METHOD_PLUGIN_DIR "${OUTPUT_PLUGINS_INCLUDE_DIR}/methods")

file(MAKE_DIRECTORY ${PLUGIN_DIR})
file(MAKE_DIRECTORY ${DATA_PLUGIN_DIR})
file(MAKE_DIRECTORY ${METHOD_PLUGIN_DIR})
file(MAKE_DIRECTORY ${OUTPUT_PLUGINS_INCLUDE_DIR})

# 包含插件通用配置
include(cmake/plugin-config.cmake)

# 添加子目录
add_subdirectory(data)
add_subdirectory(methods)
#add_subdirectory(behaviors)

# ==============================================================================
# 生成插件构建报告 | Generate Plugin Build Report
# ==============================================================================
# Call at the end to show summary of all plugins
# 在最后调用以显示所有插件的摘要
generate_plugin_build_report()


