
file(GLOB UIML_TEST_SRC *.cpp)
add_executable(${PROJECT_NAME}_test
               ${UIML_TEST_SRC}
               ../system/yamlparser.cpp
               ../softbus/softbus.c
               ../softbus/softbus_cppapi.cpp
               ../tools/hasher/hasher.c
               ../tools/universal_vector/vector.c)

# target_link_libraries(${PROJECT_NAME}_test uiml_static)
target_include_directories(${PROJECT_NAME}_test PRIVATE
                           ./
                           ../inc
                           ../softbus
                           ../tools/include)

# Uncomment to enable YAML Testcase Debug printout
#target_compile_definitions(${PROJECT_NAME}_test PRIVATE -DUIML_TESTCASE)

add_test(RunTest ${PROJECT_NAME}_test)
set_tests_properties(RunTest PROPERTIES FAIL_REGULAR_EXPRESSION "^FAIL:")
