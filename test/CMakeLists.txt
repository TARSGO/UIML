
file(GLOB UIML_TEST_SRC *.cpp)
add_executable(${PROJECT_NAME}_test
               ${UIML_TEST_SRC}
               ../conf/yamlparser.cpp
               ../softbus/softbus.c
               ../tools/hasher/hasher.c
               ../tools/universal_vector/vector.c)

# target_link_libraries(${PROJECT_NAME}_test uiml_static)
target_include_directories(${PROJECT_NAME}_test PRIVATE
                           ./
                           ../softbus
                           ../tools/include)

add_test(RunTest ${PROJECT_NAME}_test)
set_tests_properties(RunTest PROPERTIES FAIL_REGULAR_EXPRESSION "^FAIL:")
