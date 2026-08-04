# Интеграция Bison & Flex
BISON_TARGET(SqlParser parser/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp)
FLEX_TARGET(SqlLexer parser/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.flex.cpp)
ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Собираем ядро СУБД в библиотеку
add_library(dbms_core
    types.h
    ${BISON_SqlParser_OUTPUTS}
    ${FLEX_SqlLexer_OUTPUTS}
)

target_link_libraries(dbms_core PUBLIC nlohmann_json::nlohmann_json)

# Исполняемый файл CLI
add_executable(dbms_cli ./main.cpp)
target_link_libraries(dbms_cli PRIVATE dbms_core)