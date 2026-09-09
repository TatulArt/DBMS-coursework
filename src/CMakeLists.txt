cmake_minimum_required(VERSION 3.10)

# Поиск необходимых пакетов Flex и Bison в системе
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Интеграция генерации Flex и Bison (файлы создаются в бинарной директории)
BISON_TARGET(SqlParser parser/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp)
FLEX_TARGET(SqlLexer parser/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.flex.cpp)
ADD_FLEX_BISON_DEPENDENCY(SqlLexer SqlParser)

# Добавляем пути к заголовочным файлам, чтобы инклюды #include "types.h" работали корректно
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/parser)

# Сборка ВСЕХ модулей ядра СУБД (библиотека dbms_core)
add_library(dbms_core
    types.h
    
    # Модуль парсера и AST
    parser/AST.h
    parser/AST.cpp
    parser/SqlParser.h
    parser/SqlParser.cpp
    parser/SqlScanner.h
    
    # Модуль хранения (Storage)
    storage/page_manager.h
    storage/page_manager.cpp
    storage/record_manager.h
    storage/record_manager.cpp
    storage/serializer.h
    storage/serializer.cpp
    
    # Модуль индексов B+ дерева
    index/b_plus_tree.h
    index/b_plus_tree.cpp
    index/index_manager.h
    index/index_manager.cpp
    index/table_indexer.h
    index/table_indexer.cpp
    index/index_key.h
    
    # Движок и координация (Engine)
    engine/DBMS_Engine.h
    engine/DBMS_Engine.cpp
    engine/Executor.h
    engine/Executor.cpp
    engine/Database.h
    engine/Database.cpp
    engine/Table.h
    engine/Table.cpp
    engine/Schema.h
    
    # Системный каталог
    catalog/SystemCatalog.h
    catalog/SystemCatalog.cpp
    
    # Логирование (Задание №7 из допов)
    logger/AccessLogger.h
    logger/AccessLogger.cpp
    logger/LogRecord.h
    
    # Метрики и телеметрия (Задание №8 из допов)
    metrics/MetricsCollector.h
    metrics/MetricsCollector.cpp
    metrics/MetricsReporter.h
    metrics/MetricsReporter.cpp
    
    # Темпоральная персистентность (Задание №1 из допов)
    undo-log/UndoLogManager.h
    undo-log/UndoLogManager.cpp
    
    # Утилиты ошибок
    utils/Error.h
    
    # Сгенерированные Flex и Bison файлы
    ${BISON_SqlParser_OUTPUTS}
    ${FLEX_SqlLexer_OUTPUTS}
)

# Связываем ядро СУБД с JSON библиотекой nlohmann
target_link_libraries(dbms_core PUBLIC nlohmann_json::nlohmann_json)

# CLI Приложение (Исполняемый файл СУБД)
add_executable(dbms_cli ./main.cpp)
target_link_libraries(dbms_cli PRIVATE dbms_core)
