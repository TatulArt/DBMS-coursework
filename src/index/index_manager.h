#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "./b_plus_tree.h"
#include "../storage/page_manager.h"
#include "../types.h"

// Метаданные одного индекса
struct IndexInfo {
    std::string index_name;
    std::string table_name;
    std::string column_name;
    PageId root_page_id{INVALID_PAGE_ID};

    // Тип индексируемой колонки. B+ дерево работает с ключами int32_t,
    // поэтому пока поддерживается только ColumnType::Int.
    ColumnType key_type{ColumnType::Int};
};

// ============================================================================
// Каталог индексов базы данных.
//
// Отвечает за:
//   * создание и удаление индексов (B+ деревьев);
//   * хранение соответствия «индекс -> корневая страница дерева»;
//   * сохранение каталога на диск, чтобы индексы переживали перезапуск СУБД;
//   * маршрутизацию операций поиска/вставки/удаления в нужное дерево.
//
// Каталог хранится в цепочке страниц; ID первой страницы лежит в метаданных
// БД (поле DatabaseMetadata::index_catalog_page_id на 0-й странице).
// ============================================================================
class IndexManager {
public:
    explicit IndexManager(PageManager& page_manager);
    ~IndexManager() = default;

    // Подключение к файлу БД: инициализирует метаданные (если файл пустой)
    // и загружает каталог индексов с диска.
    // Пока метод не вызван, каталог живёт только в оперативной памяти.
    Status open();

    // Явное сохранение каталога на диск (вызывается автоматически при
    // создании/удалении индекса и при смене корня дерева).
    Status save();

    // Перечитать каталог с диска, отбросив состояние в памяти
    Status reload();

    // ------------------------------------------------------------------
    // Управление индексами
    // ------------------------------------------------------------------

    // Создание нового индекса
    Result<IndexInfo> create_index(const std::string& index_name,
                                   const std::string& table_name,
                                   const std::string& column_name,
                                   ColumnType key_type = ColumnType::Int);

    // Удаление индекса из каталога
    Status drop_index(const std::string& index_name);

    // Получение существующего B+ дерева по имени индекса.
    // У дерева уже настроен listener, который сохранит новый корень в каталоге.
    Result<BPlusTree> get_index(const std::string& index_name);

    // ------------------------------------------------------------------
    // Операции над содержимым индекса
    // ------------------------------------------------------------------

    // Вставка пары (ключ -> ссылка на запись)
    Status insert_entry(const std::string& index_name, int32_t key, const RecordId& rid);

    // Удаление ключа
    Status remove_entry(const std::string& index_name, int32_t key);

    // Переустановка ссылки для существующего ключа
    Status update_entry(const std::string& index_name, int32_t key, const RecordId& rid);

    // Точечный поиск: ключ -> RecordId
    Result<RecordId> find_entry(const std::string& index_name, int32_t key);

    // Диапазонный поиск [low_key, high_key]
    Status range_scan(const std::string& index_name, int32_t low_key, int32_t high_key,
                      std::vector<RecordId>& result);

    // Полный обход индекса в порядке возрастания ключей
    Status full_scan(const std::string& index_name, std::vector<RecordId>& result);

    // ------------------------------------------------------------------
    // Справочные методы
    // ------------------------------------------------------------------

    Result<IndexInfo> get_index_info(const std::string& index_name) const;

    // Поиск индекса, построенного по конкретной колонке таблицы.
    // Именно этот метод использует оптимизатор запросов, чтобы понять,
    // можно ли выполнить WHERE через индекс вместо полного скана.
    Result<IndexInfo> find_index_for_column(const std::string& table_name,
                                            const std::string& column_name) const;

    bool has_index(const std::string& index_name) const;

    std::vector<IndexInfo> list_indexes() const;
    std::vector<IndexInfo> indexes_for_table(const std::string& table_name) const;

    size_t size() const { return index_catalog_.size(); }

private:
    // Найти запись каталога или вернуть ошибку
    Result<IndexInfo*> lookup(const std::string& index_name);

    // Собрать дерево для записи каталога и подписать его на смену корня
    BPlusTree make_tree(IndexInfo& info);

    // Сериализация/десериализация каталога
    std::vector<uint8_t> serialize_catalog() const;
    Status deserialize_catalog(const std::vector<uint8_t>& bytes);

    Status write_catalog_pages(const std::vector<uint8_t>& bytes);
    Result<std::vector<uint8_t>> read_catalog_pages(PageId first_page_id) const;

    PageManager& page_manager_;
    std::unordered_map<std::string, IndexInfo> index_catalog_;

    // true, если каталог привязан к файлу БД и должен сохраняться на диск
    bool attached_{false};
};
