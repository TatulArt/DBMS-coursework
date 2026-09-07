# DBMS-coursework

Учебная СУБД на C++17. Вариант 2: индекс на основе **B+ дерева**.

## Сборка и запуск

```bash
cmake -S . -B build
cmake --build build -j
./build/src/dbms_cli      # демонстрация подсистемы индексации
cd build && ctest          # тесты
```

Требуются CMake ≥ 3.14, компилятор с C++17, Flex и Bison.
GoogleTest и nlohmann/json подтягиваются автоматически через FetchContent.

## Структура проекта

| Каталог | Назначение |
|---|---|
| `src/storage` | Страничное хранилище: `PageManager` (файл как массив страниц по 4 КБ), `RecordManager` (slotted pages), `Serializer` |
| `src/index`   | Подсистема индексации: `BPlusTree`, `IndexManager`, `TableIndexer` |
| `src/parser`  | Лексер и грамматика SQL-подобного языка (заготовка) |
| `tests`       | Тесты GoogleTest |

