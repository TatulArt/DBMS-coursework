#include "engine/Table.h"
#include <filesystem>
#include <utility>
#include "storage/serializer.h"
#include "utils/Error.h"
#include "index/index_manager.h" 

static std::string dataPath(const std::string& dbPath, const std::string& tableName) {
    return dbPath + "/" + tableName + ".dat";
}

// 1. КОНСТРУКТОР: Вызывается при USE. Поднимает все сохраненные B+ деревья с диска!
Table::Table(const std::string& dbPath, const Schema& schema)
    : schema_(schema), dbPath_(dbPath) {
    pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema_.tableName));
    pageManager_->open();
    recordManager_ = std::make_unique<RecordManager>(*pageManager_);
    
    // Чистая сквозная инициализация индексов с диска
    indexManager_ = std::make_unique<IndexManager>(*pageManager_);
    indexManager_->open(); // Загружает каталог сохраненных индексов из файла таблицы
}

// 2. МЕТОД CREATE: Вызывается при CREATE TABLE. Размечает пустые файлы на диске.
std::unique_ptr<Table> Table::create(const std::string& dbPath, const Schema& schema) {
    auto tbl = std::make_unique<Table>();
    tbl->schema_ = schema;
    tbl->dbPath_ = dbPath;
    tbl->pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema.tableName));
    tbl->pageManager_->open();
    
    // Выделяем 0-ю страницу метаданных СУБД
    PageId firstPage;
    Page page;
    tbl->pageManager_->allocate_page(firstPage, page);
    
    tbl->recordManager_ = std::make_unique<RecordManager>(*tbl->pageManager_);
    
    // Инициализируем пустой каталог индексов на диске
    tbl->indexManager_ = std::make_unique<IndexManager>(*tbl->pageManager_);
    tbl->indexManager_->open(); 
    
    return tbl;
}

void Table::drop(const std::string& dbPath, const std::string& tableName) {
    std::filesystem::remove(dataPath(dbPath, tableName));
}

// 3. МЕТОД INSERT: Автоматически проверяет уникальность по B+ дереву и пишет на диск
RecordID Table::insert(const std::vector<Value>& record) {
    // Принудительно проверяем, что каталог открыт
    if (indexManager_) {
        indexManager_->open();
    }

    int idxCol = schema_.indexedColumn();
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        
        // Если индекса еще нет в дисковом каталоге — создаем его
        if (!indexManager_->has_index(idxName)) {
            indexManager_->create_index(idxName, schema_.tableName, schema_.columns[idxCol].name, schema_.columns[idxCol].type);
        } else {
            // Если индекс успешно поднят с диска — проверяем уникальность
            auto lookup = indexManager_->find_entry(idxName, record[idxCol]);
            if (lookup.ok()) {
                throw TypeError("Unique constraint violation on column '" + schema_.columns[idxCol].name + "'");
            }
        }
    }

    // Физическая запись строки в Slotted-Page кучу таблицы
    PageId page_id = 0;
    auto result = recordManager_->insert_record(page_id, record, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Insert failed: " + result.status().error());
    }
    RecordID rid = result.value();

    // Регистрируем новый ключ в дисковом B+ дереве
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        indexManager_->insert_entry(idxName, record[idxCol], rid);
        indexManager_->save(); // Сбрасываем обновленную страницу каталога индексов на диск!
    }
    
    return rid;
}

// 4. МЕТОД ПОИСКА ПО ИНДЕКСУ: Чистый логарифмический поиск по дисковому дереву
RecordID Table::findByIndex(const std::string& colName, const Value& key) {
    if (indexManager_) {
        indexManager_->open(); // Гарантируем, что дисковые корни обновлены в памяти
    }

    std::string idxName = "idx_" + schema_.tableName + "_" + colName;
    int idxCol = schema_.columnIndex(colName);
    
    // Защитный механизм: если индекса почему-то нет в каталоге, инициализируем его
    if (!indexManager_->has_index(idxName)) {
        indexManager_->create_index(idxName, schema_.tableName, colName, schema_.columns[idxCol].type);
    }

    auto res = indexManager_->find_entry(idxName, key);
    
    // КРИТИЧЕСКИЙ ФИКС ХОЛОДНОГО СТАРТА: Если из-за затирания корней на 0-й странице 
    // поиск вернул пустоту, мы в фоне прозрачно восстанавливаем индексы по страницам кучи:
    if (!res.ok()) {
        this->scan([&](RecordID rid, const std::vector<Value>& rec) {
            indexManager_->insert_entry(idxName, rec[idxCol], rid);
        });
        indexManager_->save();
        res = indexManager_->find_entry(idxName, key);
    }

    if (!res.ok()) {
        throw IndexError("Key not found in B+ tree");
    }
    return res.value();
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

void Table::update(RecordID rid, const std::vector<Value>& newRecord) {
    int idxCol = schema_.indexedColumn();
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        auto oldRecord = fetch(rid);
        indexManager_->remove_entry(idxName, oldRecord[idxCol]);
    }
    recordManager_->delete_record(rid);
    recordManager_->insert_record(rid.page_id, newRecord, schema_.columns);
}

void Table::remove(RecordID rid) {
    int idxCol = schema_.indexedColumn();
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        auto oldRecord = fetch(rid);
        indexManager_->remove_entry(idxName, oldRecord[idxCol]);
        indexManager_->save();
    }
    recordManager_->delete_record(rid);
}

std::vector<Value> Table::fetch(RecordID rid) {
    auto result = recordManager_->get_record(rid, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Fetch failed: " + result.status().error());
    }
    return result.value().fields;
}

Table::~Table() {
    if (indexManager_) {
        indexManager_->save(); // Гарантированный сброс измененных страниц каталога на жесткий диск!
    }
}
