#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "./b_plus_tree.h"
#include "../storage/page_manager.h"
#include "../types.h"

struct IndexInfo {
    std::string index_name;
    std::string table_name;
    std::string column_name;
    PageId root_page_id{INVALID_PAGE_ID};
};

class IndexManager {
public:
    explicit IndexManager(PageManager& page_manager);
    ~IndexManager() = default;

    // Создание нового индекса
    Result<IndexInfo> create_index(const std::string& index_name, 
                                   const std::string& table_name, 
                                   const std::string& column_name);

    // Получение существующего B+ дерева по имени индекса
    Result<BPlusTree> get_index(const std::string& index_name);

    // Вставка ключа и RecordId в указанный индекс
    Status insert_entry(const std::string& index_name, int32_t key, const RecordId& rid);

    // Удаление индекса из каталога
    Status drop_index(const std::string& index_name);

    // Вспомогательный метод для получения метаданных (IndexInfo)
    Result<IndexInfo> get_index_info(const std::string& index_name) const;

private:
    PageManager& page_manager_;
    std::unordered_map<std::string, IndexInfo> index_catalog_;
};