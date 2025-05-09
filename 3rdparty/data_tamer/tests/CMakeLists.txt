
add_executable(datatamer_test
    dt_tests.cpp
    custom_types_tests.cpp
    parser_tests.cpp)

target_include_directories(datatamer_test
 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(datatamer_test data_tamer gtest_main pthread)

add_test(NAME datatamer_test COMMAND $<TARGET_FILE:datatamer_test>)
