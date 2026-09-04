// ============================================================================
// Демонстрация подсистемы индексации (ветка feature/bplus-tree-indexing).
//
// Показывает полный путь фичи:
//   1. создание базы и таблицы со страницами данных;
//   2. автоматическое создание B+ индекса по колонке с модификатором INDEXED;
//   3. поддержание индекса при вставке и удалении записей;
//   4. выполнение выборок через индекс вместо полного скана;
//   5. сохранение индекса и каталога на диск между запусками.
// ============================================================================

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "./index/index_manager.h"
#include "./index/table_indexer.h"
#include "./storage/page_manager.h"
#include "./storage/record_manager.h"
#include "./types.h"

namespace {

using json = nlohmann::json;

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(72, '=') << "\n" << title << "\n"
              << std::string(72, '=') << "\n";
}

// Результат выборки печатается массивом JSON-объектов, как требует задание
json records_to_json(const TableSchema& schema, const std::vector<Record>& records) {
    json array = json::array();
    for (const Record& record : records) {
        json object;
        for (size_t i = 0; i < schema.columns.size(); ++i) {
            const std::string& name = schema.columns[i].name;
            const Value& value = record.fields[i];

            if (value.is_null()) {
                object[name] = nullptr;
            } else if (value.get_type() == ColumnType::Int) {
                object[name] = value.get_int();
            } else {
                object[name] = value.get_string();
            }
        }
        array.push_back(object);
    }
    return array;
}

void print_select(const std::string& query, const TableSchema& schema, const SelectResult& result) {
    std::cout << "\n> " << query << "\n";
    std::cout << "  план: " << access_method_to_string(result.method)
              << ", прочитано страниц данных: " << result.pages_examined
              << ", найдено записей: " << result.records.size() << "\n";

    // Длинные выборки печатаем частично, чтобы не засорять вывод
    std::vector<Record> shown = result.records;
    const bool truncated = shown.size() > 5;
    if (truncated) shown.resize(5);

    std::cout << records_to_json(schema, shown).dump(2) << "\n";
    if (truncated) {
        std::cout << "  ... ещё " << (result.records.size() - shown.size()) << " записей\n";
    }
}

int fail(const std::string& what, const Status& status) {
    std::cerr << "ОШИБКА: " << what << ": " << status.message << "\n";
    return 1;
}

} // namespace

int main() {
    const std::string db_filename = "users_data.db";
    std::remove(db_filename.c_str());

    // Схема таблицы: id — INDEXED (уникален, не NULL), name — NOT_NULL, age — nullable
    TableSchema users;
    users.table_name = "users";
    users.columns = {
        {"id",   ColumnType::Int,    /*nullable*/ false, /*indexed*/ true},
        {"name", ColumnType::String, false, false},
        {"age",  ColumnType::Int,    true,  false}
    };

    // ------------------------------------------------------------------
    print_header("ЭТАП 1. Создание базы данных и индексов");
    // ------------------------------------------------------------------

    PageManager page_manager(db_filename);
    Status st = page_manager.open();
    if (!st.ok()) return fail("не удалось открыть файл БД", st);

    RecordManager record_manager(page_manager);
    IndexManager index_manager(page_manager);

    st = index_manager.open();
    if (!st.ok()) return fail("не удалось подключить каталог индексов", st);

    TableIndexer indexer(page_manager, record_manager, index_manager);

    st = indexer.create_indexes_for_table(users);
    if (!st.ok()) return fail("не удалось создать индексы таблицы", st);

    for (const IndexInfo& info : index_manager.list_indexes()) {
        std::cout << "Создан индекс " << info.index_name << " по колонке "
                  << info.table_name << "." << info.column_name
                  << " (корень дерева — страница " << info.root_page_id << ")\n";
    }

    std::cout << "\nПараметры B+ дерева:\n"
              << "  размер страницы .............. " << PAGE_SIZE << " байт\n"
              << "  ключей в листе ............... " << BPlusTreePage::MAX_KEYS_LEAF << "\n"
              << "  ключей во внутреннем узле .... " << BPlusTreePage::MAX_KEYS_INTERNAL << "\n";

    // ------------------------------------------------------------------
    print_header("ЭТАП 2. Вставка данных с автоматическим обновлением индекса");
    // ------------------------------------------------------------------

    const int row_count = 5000;
    for (int i = 1; i <= row_count; ++i) {
        std::vector<Value> row = {
            Value(i),
            Value("user_" + std::to_string(i)),
            (i % 7 == 0) ? Value::Null() : Value(18 + i % 60)
        };

        auto res = indexer.insert_row(users, row);
        if (!res.ok()) return fail("вставка строки " + std::to_string(i), res.status());
    }

    std::cout << "Вставлено записей: " << row_count << "\n"
              << "Занято страниц данных: " << users.data_pages.size() << "\n"
              << "Всего страниц в файле: " << page_manager.get_num_pages() << "\n";

    // ------------------------------------------------------------------
    print_header("ЭТАП 3. Ограничения целостности колонки INDEXED");
    // ------------------------------------------------------------------

    auto dup = indexer.insert_row(users, {Value(1), Value("двойник"), Value(30)});
    std::cout << "INSERT с существующим id=1        -> "
              << (dup.ok() ? "ПРИНЯТО (ошибка!)" : dup.status().message) << "\n";

    auto null_key = indexer.insert_row(users, {Value::Null(), Value("без id"), Value(30)});
    std::cout << "INSERT с NULL в INDEXED-колонке   -> "
              << (null_key.ok() ? "ПРИНЯТО (ошибка!)" : null_key.status().message) << "\n";

    auto bad_type = indexer.insert_row(users, {Value("строка"), Value("тип"), Value(30)});
    std::cout << "INSERT со строкой вместо INT      -> "
              << (bad_type.ok() ? "ПРИНЯТО (ошибка!)" : bad_type.status().message) << "\n";

    // ------------------------------------------------------------------
    print_header("ЭТАП 4. Выборка: индекс против полного скана");
    // ------------------------------------------------------------------

    auto by_id = indexer.select_equal(users, "id", Value(4321));
    if (!by_id.ok()) return fail("выборка по id", by_id.status());
    print_select("SELECT * FROM users WHERE id == 4321;", users, by_id.value());

    // По колонке age индекса нет — исполнителю приходится читать все страницы
    auto by_age = indexer.select_equal(users, "age", Value(42));
    if (!by_age.ok()) return fail("выборка по age", by_age.status());
    print_select("SELECT * FROM users WHERE age == 42;", users, by_age.value());

    auto range = indexer.select_range(users, "id", Value(100), Value(110), /*high_inclusive*/ false);
    if (!range.ok()) return fail("диапазонная выборка", range.status());
    print_select("SELECT * FROM users WHERE id BETWEEN 100 AND 110;", users, range.value());

    std::cout << "\nВыигрыш индекса: точечный поиск прочитал "
              << by_id.value().pages_examined << " страницу данных вместо "
              << by_age.value().pages_examined << ".\n";

    // ------------------------------------------------------------------
    print_header("ЭТАП 5. Удаление записи и чистка индекса");
    // ------------------------------------------------------------------

    if (!by_id.value().records.empty()) {
        const RecordId victim = by_id.value().records[0].id;
        st = indexer.delete_row(users, victim);
        if (!st.ok()) return fail("удаление записи", st);

        std::cout << "Удалена запись id=4321 (страница " << victim.page_id
                  << ", слот " << victim.slot_id << ")\n";

        auto after = indexer.select_equal(users, "id", Value(4321));
        if (!after.ok()) return fail("повторная выборка", after.status());
        std::cout << "Повторный поиск id=4321 нашёл записей: "
                  << after.value().records.size() << "\n";

        // Освободившийся ключ можно использовать снова
        auto reused = indexer.insert_row(users, {Value(4321), Value("новый_пользователь"), Value(33)});
        std::cout << "Повторная вставка id=4321        -> "
                  << (reused.ok() ? "успешно" : reused.status().message) << "\n";
    }

    const std::vector<PageId> data_pages = users.data_pages;
    page_manager.close();

    // ------------------------------------------------------------------
    print_header("ЭТАП 6. Перезапуск СУБД: индекс поднимается с диска");
    // ------------------------------------------------------------------

    PageManager reopened(db_filename);
    st = reopened.open();
    if (!st.ok()) return fail("повторное открытие БД", st);

    RecordManager reopened_records(reopened);
    IndexManager reopened_indexes(reopened);
    st = reopened_indexes.open();
    if (!st.ok()) return fail("загрузка каталога индексов", st);

    TableIndexer reopened_indexer(reopened, reopened_records, reopened_indexes);

    // Схему и список страниц таблицы восстанавливает каталог таблиц
    // (уровень парсера/исполнителя); индексы уже прочитаны с диска.
    TableSchema restored = users;
    restored.data_pages = data_pages;

    std::cout << "Индексов загружено из файла: " << reopened_indexes.size() << "\n";
    for (const IndexInfo& info : reopened_indexes.list_indexes()) {
        std::cout << "  " << info.index_name << " -> " << info.table_name << "."
                  << info.column_name << ", корень на странице " << info.root_page_id << "\n";
    }

    auto restored_res = reopened_indexer.select_equal(restored, "id", Value(4321));
    if (!restored_res.ok()) return fail("выборка после перезапуска", restored_res.status());
    print_select("SELECT * FROM users WHERE id == 4321;  -- после перезапуска",
                 restored, restored_res.value());

    // Проверяем, что индекс цел целиком, а не только в одной точке
    auto tree = reopened_indexes.get_index(TableIndexer::index_name_for("users", "id"));
    if (!tree.ok()) return fail("получение дерева индекса", tree.status());

    Status valid = tree.value().validate();
    std::cout << "\nСтруктурная проверка B+ дерева: "
              << (valid.ok() ? "дерево корректно" : "ПОВРЕЖДЕНО — " + valid.message) << "\n";

    size_t indexed_keys = 0;
    for (auto it = tree.value().begin(); it != tree.value().end(); ++it) {
        indexed_keys++;
    }
    std::cout << "Ключей в индексе после перезапуска: " << indexed_keys << "\n";

    reopened.close();
    std::cout << "\nГотово. Файл базы данных: " << db_filename << "\n";
    return 0;
}
