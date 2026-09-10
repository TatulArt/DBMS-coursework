#include "engine/Table.h"
#include <filesystem>
#include <utility>
#include "storage/serializer.h"
#include "utils/Error.h"

static std::string dataPath(const std::string& dbPath, const std::string& tableName) {
    return dbPath + "/" + tableName + ".dat";
}

Table::Table(const std::string& dbPath, const Schema& schema)
    : schema_(schema), dbPath_(dbPath) {
    pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema_.tableName));
    pageManager_->open();
    recordManager_ = std::make_unique<RecordManager>(*pageManager_);
}

std::unique_ptr<Table> Table::create(const std::string& dbPath, const Schema& schema) {
    auto tbl = std::make_unique<Table>();
    tbl->schema_ = schema;
    tbl->dbPath_ = dbPath;
    tbl->pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema.tableName));
    tbl->pageManager_->open();
    
    // (строго одна страница для новой таблицы)
    PageId firstPage;
    Page page;
    tbl->pageManager_->allocate_page(firstPage, page);
    
    tbl->recordManager_ = std::make_unique<RecordManager>(*tbl->pageManager_);
    return tbl;
}

void Table::drop(const std::string& dbPath, const std::string& tableName) {
    std::filesystem::remove(dataPath(dbPath, tableName));
}

RecordId Table::insert(const std::vector<Value>& record) {
    // Вставляем на страницу 0
    PageId page_id = 0;
    auto result = recordManager_->insert_record(page_id, record, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Insert failed: " + result.status().message);
    }
    return result.value();
}

void Table::scan(std::function<void(RecordId, const std::vector<Value>&)> cb) const {
    // Обход страницы делает RecordManager: он пропускает удалённые слоты
    // и проверяет, что слот не выходит за границы страницы.
    auto records = recordManager_->scan_page(0, schema_.columns);
    if (!records.ok()) {
        throw StorageError("Scan failed: " + records.status().message);
    }

    for (const Record& record : records.value()) {
        cb(record.id, record.fields);
    }
}

RecordId Table::findByIndex(const std::string& colName, const Value& key) {
    throw IndexError("Index not implemented");
}

void Table::update(RecordId rid, const std::vector<Value>& newRecord) {
    recordManager_->delete_record(rid);
    recordManager_->insert_record(rid.page_id, newRecord, schema_.columns);
}

void Table::remove(RecordId rid) {
    recordManager_->delete_record(rid);
}

std::vector<Value> Table::fetch(RecordId rid) {
    auto result = recordManager_->get_record(rid, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Fetch failed: " + result.status().message);
    }
    return result.value().fields;
}
