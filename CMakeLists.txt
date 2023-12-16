
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

# UIML 静态库部分
add_library(uiml_static OBJECT
    ${TOOLS_MODULE_SRC}
    ${SOFTBUS_MODULE_SRC}
    ${SYSTEM_MODULE_SRC}
    ${SERVICES_MODULE_SRC}
)

# 链接祖传的AHRS静态库
# 谁能把这玩意搞掉啊求求了
target_link_libraries(uiml_static PRIVATE ${CMAKE_SOURCE_DIR}/lib/AHRS.lib)

# 包含目录
target_include_directories(uiml_static PUBLIC
    ${TOOLS_MODULE_INCL}
    ${SOFTBUS_MODULE_INCL}
    ${SYSTEM_MODULE_INCL}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# 激发所有警告
target_compile_options(uiml_static PRIVATE -Wall)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
