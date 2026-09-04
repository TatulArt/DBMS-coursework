#pragma once

#include <string>
#include <vector>
#include "./index_manager.h"
#include "../storage/page_manager.h"
#include "../storage/record_manager.h"
#include "../types.h"

// ============================================================================
// Описание таблицы: схема колонок + список страниц, на которых лежат данные.
//
// Полноценный каталог таблиц (с сохранением схемы на диск) — задача уровня
// парсера/исполнителя запросов, поэтому здесь схема передаётся вызывающей
// стороной. Для индексов важны только имена и типы колонок и то, на каких
// страницах искать записи.
// ============================================================================
struct TableSchema {
    std::string table_name;
    std::vector<ColumnDef> columns;
    std::vector<PageId> data_pages;

    // Индекс колонки по имени, либо -1
    int column_index(const std::string& name) const;

    // Описание колонки по имени, либо nullptr
    const ColumnDef* column(const std::string& name) const;
};

// Как был выполнен запрос выборки — через индекс или полным сканом.
// Используется в тестах и в выводе плана запроса.
enum class AccessMethod {
    IndexLookup,  // Точечный поиск по B+ дереву
    IndexRange,   // Диапазонный поиск по B+ дереву
    FullScan      // Последовательный просмотр всех страниц данных
};

std::string access_method_to_string(AccessMethod method);

// Результат выборки: сами записи и способ, которым они были получены
struct SelectResult {
    std::vector<Record> records;
    AccessMethod method{AccessMethod::FullScan};

    // Сколько страниц данных было прочитано (наглядно показывает выигрыш индекса)
    size_t pages_examined{0};
};

// ============================================================================
// TableIndexer — связующее звено между хранилищем и индексами.
//
// Отвечает за то, ради чего писалась ветка feature/bplus-tree-indexing:
//   * построение B+ индекса по страницам данных таблицы;
//   * поддержание индексов в актуальном состоянии при вставке и удалении;
//   * выбор оптимального способа выполнения выборки (индекс vs полный скан).
//
// Индексы ссылаются на данные через RecordId {page_id, slot_id} и не хранят
// копий самих записей — это прямое требование задания.
// ============================================================================
class TableIndexer {
public:
    TableIndexer(PageManager& page_manager,
                 RecordManager& record_manager,
                 IndexManager& index_manager);

    // Соглашение об именовании индексов: idx_<таблица>_<колонка>
    static std::string index_name_for(const std::string& table_name,
                                      const std::string& column_name);

    // ------------------------------------------------------------------
    // Построение индексов
    // ------------------------------------------------------------------

    // Создать индексы для всех колонок, помеченных модификатором INDEXED
    Status create_indexes_for_table(const TableSchema& schema);

    // Создать (или перестроить) индекс по колонке: полный проход по страницам
    // данных таблицы с занесением каждой живой записи в B+ дерево.
    Result<IndexInfo> build_index(const TableSchema& schema, const std::string& column_name);

    // ------------------------------------------------------------------
    // Модификация данных с поддержкой индексов
    // ------------------------------------------------------------------

    // Вставка строки: проверяет ограничения, пишет запись на страницу данных
    // и обновляет все индексы таблицы. При нехватке места выделяет новую
    // страницу и добавляет её в schema.data_pages.
    Result<RecordId> insert_row(TableSchema& schema, const std::vector<Value>& fields);

    // Удаление строки по её физическому адресу с чисткой индексов
    Status delete_row(const TableSchema& schema, const RecordId& rid);

    // ------------------------------------------------------------------
    // Выборка
    // ------------------------------------------------------------------

    // Точное совпадение: column == value
    Result<SelectResult> select_equal(const TableSchema& schema,
                                      const std::string& column_name,
                                      const Value& value);

    // Диапазон [low, high] либо [low, high) — семантика BETWEEN из задания
    Result<SelectResult> select_range(const TableSchema& schema,
                                      const std::string& column_name,
                                      const Value& low,
                                      const Value& high,
                                      bool high_inclusive = true);

    // Последовательный просмотр всех страниц данных
    Result<SelectResult> full_scan(const TableSchema& schema);

    // Чтение одной записи по её адресу
    Result<Record> fetch(const TableSchema& schema, const RecordId& rid);

    // Проверка ограничений целостности строки (типы, NOT_NULL, INDEXED)
    Status validate_row(const TableSchema& schema, const std::vector<Value>& fields) const;

private:
    // Достать записи по списку RecordId
    Result<std::vector<Record>> fetch_all(const TableSchema& schema,
                                          const std::vector<RecordId>& rids);

    // Снять запись со всех индексов таблицы
    Status remove_from_indexes(const TableSchema& schema, const std::vector<Value>& fields);

    PageManager& page_manager_;
    RecordManager& record_manager_;
    IndexManager& index_manager_;
};
