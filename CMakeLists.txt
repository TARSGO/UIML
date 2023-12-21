
cmake_minimum_required(VERSION 3.21)
project(UIML)

if(MSVC)
    add_compile_options("/utf-8")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

# 子模块添加
add_subdirectory(tools)
add_subdirectory(services)
add_subdirectory(softbus)
add_subdirectory(system)

# Embedded Template Library
add_subdirectory(lib/etl)

# UIML 静态库部分
add_library(uiml_static OBJECT
    ${TOOLS_MODULE_SRC}
    ${SOFTBUS_MODULE_SRC}
    ${SYSTEM_MODULE_SRC}
    ${SERVICES_MODULE_SRC}
)

# 链接祖传的AHRS静态库
# 谁能把这玩意搞掉啊求求了
target_link_libraries(uiml_static PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/AHRS.lib)
target_link_libraries(uiml_static PUBLIC etl::etl) # 链接ETL

# 包含目录
target_include_directories(uiml_static PUBLIC
    ${TOOLS_MODULE_INCL}
    ${SOFTBUS_MODULE_INCL}
    ${SYSTEM_MODULE_INCL}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# 激发所有警告
# FIXME: 由于UIML Softbus在C API中使用的部分宏会导致报出missing brace警告（如按手册编写无实际影响）
# 故取消该警告，但留一个FIXME供检索
target_compile_options(uiml_static PRIVATE -Wall -Wno-missing-braces)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
