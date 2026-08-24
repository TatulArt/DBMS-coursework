add_executable(dbms_tests 
    test_types.cpp 
    test_serializer.cpp
    test_storage_pipeline.cpp
    test_b_plus_tree.cpp
)

target_link_libraries(dbms_tests PRIVATE dbms_core gtest_main)

include(GoogleTest)
gtest_discover_tests(dbms_tests)
