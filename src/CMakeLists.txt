# Интеграция генерации Flex и Bison
BISON_TARGET(SqlParser parser/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp)
FLEX_TARGET(SqlLexer parser/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.flex.cpp)
ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Сборка всех модулей ядра СУБД
add_library(dbms_core
    types.h
    storage/page_manager.h
    storage/page_manager.cpp
    storage/record_manager.h
    storage/record_manager.cpp
    storage/serializer.h
    storage/serializer.cpp
    ${BISON_SqlParser_OUTPUTS}
    ${FLEX_SqlLexer_OUTPUTS}
)

target_link_libraries(dbms_core PUBLIC nlohmann_json::nlohmann_json)

# CLI Приложение
add_executable(dbms_cli ./main.cpp)
target_link_libraries(dbms_cli PRIVATE dbms_core)