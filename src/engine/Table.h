#ifndef DBMS_TABLE_H
#define DBMS_TABLE_H

#include <functional>
#include <memory>
#include <string>
#include "Schema.h"
#include "index/index_manager.h"
#include "storage/page_manager.h" 
#include "storage/record_manager.h"


class Table {
public:
    Table() = default;
    Table(const std::string& dbPath, const Schema& schema);
    
    // Создать новую таблицу
    static std::unique_ptr<Table> create(const std::string& dbPath, const Schema& schema);

    // Удалить таблицу (файлы с диска)
    static void drop(const std::string& dbPath, const std::string& tableName);

    [[nodiscard]] const Schema& schema() const { return schema_; }

    // DML (data manipulation language)
    RecordID insert(const std::vector<Value>& record);

    void scan(std::function<void(RecordID, const std::vector<Value>&)> cb) const;

    // Поиск по индексу — вернёт RecordID или бросит если нет индекса
    RecordID findByIndex(const std::string& colName, const Value& key);
    void update(RecordID rid, const std::vector<Value>& newRecord);
    void remove(RecordID rid);
    std::vector<Value> fetch(RecordID rid);

    ~Table();

private:
    // Голова цепочки страниц кучи. Хранится в метаданных файла таблицы,
    // поэтому переживает перезапуск СУБД.
    PageId firstDataPage_{INVALID_PAGE_ID};

    // Подсказка «куда класть следующую запись»: последняя страница цепочки.
    // Нужна, чтобы обычная вставка не проходила всю цепочку целиком.
    PageId insertHintPage_{INVALID_PAGE_ID};

    // Файл таблицы нечитаем (создан версией без заголовка базы на 0-й
    // странице). Пустая строка означает, что с таблицей всё в порядке.
    std::string loadError_;

    // Бросает StorageError, если файл таблицы не удалось прочитать
    void requireUsable() const;

    // Загрузить/создать голову цепочки страниц данных
    PageId ensureDataPage();

    // Найти страницу, на которую влезет запись нужного размера,
    // при необходимости дописав в цепочку новую страницу
    PageId findPageForRecord(size_t recordBytes);

    Schema schema_;
    std::string dbPath_;
    std::unique_ptr<PageManager> pageManager_;
    std::unique_ptr<RecordManager> recordManager_;
    std::unique_ptr<IndexManager> indexManager_;
};


#endif // DBMS_TABLE_H
