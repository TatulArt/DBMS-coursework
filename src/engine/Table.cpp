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

RecordID Table::insert(const std::vector<Value>& record) {
    // Вставляем на страницу 0
    std::cout << "DEBUG: Table::insert - schema.columns.size() = " << schema_.columns.size() << std::endl;
    std::cout << "DEBUG: Table::insert - record.size() = " << record.size() << std::endl;
    PageId page_id = 0;
    auto result = recordManager_->insert_record(page_id, record, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Insert failed: " + result.status().error());
    }
    return result.value();
}

void Table::scan(std::function<void(RecordID, const std::vector<Value>&)> cb) const {
    Page page;
    pageManager_->read_page(0, page);
    
    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    
    for (uint16_t i = 0; i < header.slot_count; ++i) {
        Slot slot;
        std::memcpy(&slot, page.data + sizeof(SlottedPageHeader) + i * sizeof(Slot), sizeof(Slot));
        if (slot.length == 0) continue;
        
        auto fields_res = Serializer::deserialize_fields(page.data + slot.offset, slot.length, schema_.columns);
        if (fields_res.ok()) {
            cb(RecordID{0, i}, fields_res.value());
        }
    }
}

RecordID Table::findByIndex(const std::string& colName, const Value& key) {
    throw IndexError("Index not implemented");
}

void Table::update(RecordID rid, const std::vector<Value>& newRecord) {
    recordManager_->delete_record(rid);
    recordManager_->insert_record(rid.page_id, newRecord, schema_.columns);
}

void Table::remove(RecordID rid) {
    recordManager_->delete_record(rid);
}

std::vector<Value> Table::fetch(RecordID rid) {
    auto result = recordManager_->get_record(rid, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Fetch failed: " + result.status().error());
    }
    return result.value().fields;
}
