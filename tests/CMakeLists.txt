add_executable(dbms_tests test_types.cpp)
target_link_libraries(dbms_tests PRIVATE dbms_core gtest_main)

include(GoogleTest)
gtest_discover_tests(dbms_tests)