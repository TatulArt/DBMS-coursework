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

    // Поднимаем голову цепочки страниц кучи из заголовка файла.
    auto meta = pageManager_->read_metadata();
    if (meta.ok()) {
        firstDataPage_ = meta.value().first_data_page_id;
    } else {
        // Заголовка нет, хотя файл не пустой: так выглядят файлы, созданные
        // версией, которая писала записи прямо на нулевую страницу. Прочитать
        // их в новом формате нельзя.
        //
        // Ошибку запоминаем, но не бросаем: Executor поднимает СРАЗУ все базы
        // из каталога ./data, и исключение здесь уронило бы запуск СУБД
        // целиком из-за одного постороннего файла. Сообщение всплывёт при
        // первом же обращении именно к этой таблице.
        loadError_ = "Table file " + dataPath(dbPath, schema_.tableName) +
                     " has no database header: it was created by an older version. "
                     "Delete the file and create the table again.";
    }
}

void Table::requireUsable() const {
    if (!loadError_.empty()) {
        throw StorageError(loadError_);
    }
}

// 2. МЕТОД CREATE: Вызывается при CREATE TABLE. Размечает пустые файлы на диске.
std::unique_ptr<Table> Table::create(const std::string& dbPath, const Schema& schema) {
    auto tbl = std::make_unique<Table>();
    tbl->schema_ = schema;
    tbl->dbPath_ = dbPath;
    tbl->pageManager_ = std::make_unique<PageManager>(dataPath(dbPath, schema.tableName));
    tbl->pageManager_->open();

    // Размечаем 0-ю страницу как заголовок базы (magic + ссылки на кучу,
    // каталог индексов и список свободных страниц).
    //
    // Раньше здесь просто выделялась нулевая страница, а записи таблицы потом
    // писались прямо в неё: slotted-заголовок ложился поверх magic_number,
    // из-за чего каталог индексов вообще не мог подключиться к файлу.
    Status st = tbl->pageManager_->create_database(dataPath(dbPath, schema.tableName));
    if (!st.ok()) {
        throw StorageError("Cannot initialize table file: " + st.message);
    }

    tbl->recordManager_ = std::make_unique<RecordManager>(*tbl->pageManager_);

    // Инициализируем пустой каталог индексов на диске
    tbl->indexManager_ = std::make_unique<IndexManager>(*tbl->pageManager_);
    tbl->indexManager_->open(); 

    // Первая страница кучи — отдельная страница, а не нулевая
    tbl->ensureDataPage();

    return tbl;
}

void Table::drop(const std::string& dbPath, const std::string& tableName) {
    std::filesystem::remove(dataPath(dbPath, tableName));
}

// 3. МЕТОД INSERT: Автоматически проверяет уникальность по B+ дереву и пишет на диск
RecordID Table::insert(const std::vector<Value>& record) {
    requireUsable();

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

    // Физическая запись строки в Slotted-Page кучу таблицы.
    // Куча — цепочка страниц: если на текущих страницах места нет,
    // findPageForRecord() дописывает в цепочку новую.
    const size_t recordBytes = RecordManager::record_size(record, schema_.columns);
    const PageId page_id = findPageForRecord(recordBytes);

    auto result = recordManager_->insert_record(page_id, record, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Insert failed: " + result.status().message);
    }
    RecordID rid = result.value();

    // Регистрируем новый ключ в дисковом B+ дереве.
    //
    // Статус вставки в индекс обязательно проверяем: раньше он игнорировался,
    // и строка со слишком длинным ключом (STRING длиннее StringKey::MAX_LENGTH)
    // ложилась в кучу, но не попадала в дерево. SELECT по индексу такую строку
    // не находил, хотя COUNT(*) её считал.
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;

        Status st = indexManager_->insert_entry(idxName, record[idxCol], rid);
        if (!st.ok()) {
            // Откатываем запись, иначе куча и индекс разъедутся
            recordManager_->delete_record(rid);
            throw IndexError("Cannot index column '" + schema_.columns[idxCol].name +
                             "': " + st.message);
        }

        st = indexManager_->save(); // Сбрасываем страницу каталога индексов на диск
        if (!st.ok()) {
            throw StorageError("Cannot save index catalog: " + st.message);
        }
    }
    
    return rid;
}

// 4. МЕТОД ПОИСКА ПО ИНДЕКСУ: Чистый логарифмический поиск по дисковому дереву
RecordID Table::findByIndex(const std::string& colName, const Value& key) {
    requireUsable();

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

    // Раньше здесь при каждом промахе индекс перестраивался полным сканом
    // таблицы: каталог индексов не сохранялся на диск, потому что записи
    // затирали заголовок базы на 0-й странице. Куча переехала на собственные
    // страницы, каталог переживает перезапуск — перестройка больше не нужна.
    if (!res.ok()) {
        throw IndexError("Key not found in B+ tree");
    }
    return res.value();
}

void Table::scan(std::function<void(RecordId, const std::vector<Value>&)> cb) const {
    requireUsable();

    // Идём по цепочке страниц кучи. Обход отдельной страницы делает
    // RecordManager: он пропускает удалённые слоты и проверяет, что слот
    // не выходит за границы страницы.
    PageId page_id = firstDataPage_;
    uint32_t visited = 0;
    const uint32_t page_limit = pageManager_->get_num_pages();

    while (page_id != INVALID_PAGE_ID) {
        auto records = recordManager_->scan_page(page_id, schema_.columns);
        if (!records.ok()) {
            throw StorageError("Scan failed: " + records.status().message);
        }

        for (const Record& record : records.value()) {
            cb(record.id, record.fields);
        }

        auto next = recordManager_->next_page(page_id);
        if (!next.ok()) {
            throw StorageError("Scan failed: " + next.status().message);
        }
        page_id = next.value();

        // Страховка от испорченной цепочки: страниц в ней не может быть
        // больше, чем страниц в файле
        if (++visited > page_limit) {
            throw StorageError("Scan failed: heap page chain of table " +
                               schema_.tableName + " is cyclic");
        }
    }
}

void Table::update(RecordID rid, const std::vector<Value>& newRecord) {
    requireUsable();

    int idxCol = schema_.indexedColumn();
    const bool indexed = (idxCol != -1 && indexManager_);

    std::vector<Value> oldRecord;
    std::string idxName;
    if (indexed) {
        idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        oldRecord = fetch(rid);
        indexManager_->remove_entry(idxName, oldRecord[idxCol]);
    }

    // Новая версия строки может оказаться длиннее старой и не поместиться
    // на прежнюю страницу, поэтому место под неё ищем заново.
    recordManager_->delete_record(rid);

    const size_t recordBytes = RecordManager::record_size(newRecord, schema_.columns);
    PageId target = rid.page_id;

    auto space = recordManager_->page_has_space(target, recordBytes);
    if (!space.ok() || !space.value()) {
        target = findPageForRecord(recordBytes);
    }

    auto inserted = recordManager_->insert_record(target, newRecord, schema_.columns);
    if (!inserted.ok()) {
        // Откат: возвращаем ключ старой строки в индекс, чтобы он не разъехался
        // с содержимым кучи
        if (indexed) {
            indexManager_->insert_entry(idxName, oldRecord[idxCol], rid);
            indexManager_->save();
        }
        throw StorageError("Update failed: " + inserted.status().message);
    }

    // Запись получила новый слот — индекс обязан указывать на него.
    // Без этого после UPDATE поиск по индексу возвращал старый адрес.
    if (indexed) {
        indexManager_->insert_entry(idxName, newRecord[idxCol], inserted.value());
        indexManager_->save();
    }
}

void Table::remove(RecordID rid) {
    requireUsable();

    int idxCol = schema_.indexedColumn();
    if (idxCol != -1 && indexManager_) {
        std::string idxName = "idx_" + schema_.tableName + "_" + schema_.columns[idxCol].name;
        auto oldRecord = fetch(rid);
        indexManager_->remove_entry(idxName, oldRecord[idxCol]);
        indexManager_->save();
    }
    recordManager_->delete_record(rid);
}

std::vector<Value> Table::fetch(RecordId rid) {
    requireUsable();

    auto result = recordManager_->get_record(rid, schema_.columns);
    if (!result.ok()) {
        throw std::runtime_error("Fetch failed: " + result.status().message);
    }
    return result.value().fields;
}

// ----------------------------------------------------------------------------
// ЦЕПОЧКА СТРАНИЦ КУЧИ
// ----------------------------------------------------------------------------

// Возвращает голову цепочки, создавая её при первом обращении
PageId Table::ensureDataPage() {
    if (firstDataPage_ != INVALID_PAGE_ID) {
        return firstDataPage_;
    }

    Page page;
    PageId new_page_id = INVALID_PAGE_ID;
    Status st = pageManager_->allocate_page(new_page_id, page);
    if (!st.ok()) {
        throw StorageError("Cannot allocate data page: " + st.message);
    }

    st = recordManager_->init_page(new_page_id);
    if (!st.ok()) {
        throw StorageError("Cannot initialize data page: " + st.message);
    }

    // Голову цепочки запоминаем в заголовке файла таблицы
    auto meta = pageManager_->read_metadata();
    if (!meta.ok()) {
        throw StorageError("Cannot read table header: " + meta.status().message);
    }
    DatabaseMetadata updated = meta.value();
    updated.first_data_page_id = new_page_id;

    st = pageManager_->write_metadata(updated);
    if (!st.ok()) {
        throw StorageError("Cannot save table header: " + st.message);
    }

    firstDataPage_ = new_page_id;
    insertHintPage_ = new_page_id;
    return firstDataPage_;
}

PageId Table::findPageForRecord(size_t recordBytes) {
    const PageId head = ensureDataPage();

    // 1. Быстрый путь: страница, куда клали прошлую запись
    if (insertHintPage_ != INVALID_PAGE_ID) {
        auto space = recordManager_->page_has_space(insertHintPage_, recordBytes);
        if (space.ok() && space.value()) {
            return insertHintPage_;
        }
    }

    // 2. Полный обход цепочки: ищем «дырку», освободившуюся после DELETE,
    // и заодно находим хвост, к которому можно прицепить новую страницу
    PageId page_id = head;
    PageId tail = head;
    uint32_t visited = 0;
    const uint32_t page_limit = pageManager_->get_num_pages();

    while (page_id != INVALID_PAGE_ID) {
        tail = page_id;

        auto space = recordManager_->page_has_space(page_id, recordBytes);
        if (!space.ok()) {
            throw StorageError("Cannot inspect data page: " + space.status().message);
        }
        if (space.value()) {
            insertHintPage_ = page_id;
            return page_id;
        }

        auto next = recordManager_->next_page(page_id);
        if (!next.ok()) {
            throw StorageError("Cannot walk data pages: " + next.status().message);
        }
        page_id = next.value();

        if (++visited > page_limit) {
            throw StorageError("Heap page chain of table " + schema_.tableName + " is cyclic");
        }
    }

    // 3. Места нет нигде — расширяем таблицу новой страницей
    Page fresh;
    PageId new_page_id = INVALID_PAGE_ID;
    Status st = pageManager_->allocate_page(new_page_id, fresh);
    if (!st.ok()) {
        throw StorageError("Cannot allocate data page: " + st.message);
    }

    st = recordManager_->init_page(new_page_id);
    if (!st.ok()) {
        throw StorageError("Cannot initialize data page: " + st.message);
    }

    st = recordManager_->set_next_page(tail, new_page_id);
    if (!st.ok()) {
        throw StorageError("Cannot link data page: " + st.message);
    }

    // Запись обязана помещаться хотя бы на пустую страницу
    auto space = recordManager_->page_has_space(new_page_id, recordBytes);
    if (!space.ok() || !space.value()) {
        throw StorageError("Record of " + std::to_string(recordBytes) +
                           " bytes does not fit into an empty " +
                           std::to_string(PAGE_SIZE) + "-byte page");
    }

    insertHintPage_ = new_page_id;
    return new_page_id;
}

Table::~Table() {
    if (indexManager_) {
        indexManager_->save(); // Гарантированный сброс измененных страниц каталога на жесткий диск!
    }
}
