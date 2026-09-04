add_executable(dbms_tests 
    test_types.cpp 
    test_serializer.cpp
    test_storage_pipeline.cpp
    test_b_plus_tree.cpp
    test_index_manager.cpp
)

target_link_libraries(dbms_tests PRIVATE dbms_core gtest_main)

include(GoogleTest)
gtest_discover_tests(dbms_tests)

# На macOS каталог /usr/local/include просматривается раньше путей -isystem,
# поэтому системная копия gtest может перекрыть версию из FetchContent.
# Снимаем у целей gtest пометку SYSTEM, чтобы CMake выдавал -I вместо -isystem.
foreach(_gtest_target gtest gtest_main)
    if(TARGET ${_gtest_target})
        set_target_properties(${_gtest_target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "")
    endif()
endforeach()
