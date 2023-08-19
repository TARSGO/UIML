
cmake_minimum_required(VERSION 3.21)
project(UIML)

if(MSVC)
    add_compile_options("/utf-8")
endif()

# 子模块添加
add_subdirectory(tools)
add_subdirectory(softbus)
add_subdirectory(conf)

# UIML 静态库部分
add_library(uiml_static OBJECT
    ${TOOLS_MODULE_SRC}
    ${SOFTBUS_MODULE_SRC}
    ${CONFIG_MODULE_SRC}
)

target_include_directories(uiml_static PUBLIC
    ${TOOLS_MODULE_INCL}
    ${SOFTBUS_MODULE_INCL}
    ${CONFIG_MODULE_INCL}
)

if(PROJECT_IS_TOP_LEVEL)
    target_include_directories(uiml_static PRIVATE test)

    file(GLOB UIML_TEST_SRC test/*.cpp)
    add_executable(${PROJECT_NAME}_test ${UIML_TEST_SRC} conf/yamlparser.cpp tools/hasher/hasher.c tools/universal_vector/vector.c)
    # target_link_libraries(${PROJECT_NAME}_test uiml_static)
    target_include_directories(${PROJECT_NAME}_test PRIVATE test tools/include)
    add_test(RunTest ${PROJECT_NAME}_test)
    set_tests_properties(RunTest PROPERTIES FAIL_REGULAR_EXPRESSION "^FAIL:")
endif()
