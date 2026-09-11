add_executable(dbms_tests 
    test_types.cpp 
    test_serializer.cpp
    test_storage_pipeline.cpp
    test_b_plus_tree.cpp
    test_index_manager.cpp
    test_table_indexer.cpp
    test_string_index.cpp
    test_cli_sql.cpp
)

target_link_libraries(dbms_tests PRIVATE dbms_core gtest_main)

# Интеграционные тесты (test_cli_sql.cpp) запускают настоящий бинарник СУБД и
# подают ему SQL в стандартный ввод. Путь к нему подставляет CMake, поэтому
# тесты одинаково работают и локально, и в devcontainer.
target_compile_definitions(dbms_tests PRIVATE
    DBMS_CLI_PATH="$<TARGET_FILE:dbms_cli>")

# Без этого dbms_cli может быть ещё не собран к моменту запуска тестов
add_dependencies(dbms_tests dbms_cli)

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
