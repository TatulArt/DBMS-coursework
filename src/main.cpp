#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#include "./types.h"
#include "./storage/page_manager.h"
#include "./storage/record_manager.h"
#include "./storage/serializer.h"

// Вспомогательная функция для красивой печати таблицы в консоль
void print_record(const Record& record) {
    std::cout << "| Slot ID: " << std::setw(2) << record.id.slot_id << " | ";
    for (const auto& field : record.fields) {
        std::cout << std::setw(15) << field.to_string() << " | ";
    }
    std::cout << "\n";
}

int main() {
    const std::string db_filename = "users_data.db";

    // 1. ОПРЕДЕЛЯЕМ СХЕМУ ТАБЛИЦЫ "users"
    // Колонки: id (INT, NOT NULL), name (STRING, NOT NULL), rating (INT, NULLABLE)
    std::vector<ColumnDef> schema = {
        {"id", ColumnType::Int, false, true},
        {"name", ColumnType::String, false, false},
        {"rating", ColumnType::Int, true, false}
    };

    std::cout << "=== ЭТАП 1: Создание файла и запись данных на диск ===\n";

    // Инициализируем менеджеры
    PageManager page_mgr_write(db_filename);
    
    // В этот момент open() создаст файл "users_data.db" на диске, если его нет
    if (!page_mgr_write.open().ok()) {
        std::cerr << "Ошибка при открытии/создании файла БД!\n";
        return 1;
    }

    RecordManager record_mgr_write(page_mgr_write);

    // Выделяем первую чистую страницу (4096 байт)
    PageId page_id;
    Page page;
    if (!page_mgr_write.allocate_page(page_id, page).ok()) {
        std::cerr << "Ошибка выделения страницы!\n";
        return 1;
    }
    std::cout << "Создана новая дисковая страница с ID: " << page_id << "\n";

    // Подготавливаем тестовые записи (строки)
    std::vector<std::vector<Value>> rows_to_insert = {
        { Value(1), Value("Alice Smith"), Value(100) },
        { Value(2), Value("Bob Marley"),  Value::Null() }, // Запись с NULL
        { Value(3), Value("Charlie Brown"), Value(85) }
    };

    // Вставляем строки и запоминаем их RecordId {page_id, slot_id}
    std::vector<RecordId> inserted_rids;
    for (const auto& row : rows_to_insert) {
        auto res = record_mgr_write.insert_record(page_id, row, schema);
        if (!res.ok()) {
            std::cerr << "Ошибка вставки строки: " << res.status().message << "\n";
            return 1;
        }
        inserted_rids.push_back(res.value());
        std::cout << "Запись закоммичена на страницу " << res.value().page_id 
                  << ", в слот #" << res.value().slot_id << "\n";
    }

    // Закрываем файл (данные физически сбрасываются из кэша RAM на диск)
    page_mgr_write.close();
    std::cout << "Файл " << db_filename << " успешно сохранен и закрыт.\n\n";

    // ========================================================================

    std::cout << "=== ЭТАП 2: Чтение данных с диска и десериализация ===\n";

    // Открываем файл заново (симулируем перезапуск СУБД)
    PageManager page_mgr_read(db_filename);
    if (!page_mgr_read.open().ok()) {
        std::cerr << "Ошибка при повторном открытии файла БД!\n";
        return 1;
    }

    RecordManager record_mgr_read(page_mgr_read);

    std::cout << "Вычитываем данные из файла " << db_filename << ":\n";
    std::cout << "-------------------------------------------------------------\n";
    std::cout << "|   Slot    |       id        |      name       |     rating    |\n";
    std::cout << "-------------------------------------------------------------\n";

    // Вычитываем каждую запись по сохраненному RecordId
    for (const auto& rid : inserted_rids) {
        auto record_res = record_mgr_read.get_record(rid, schema);
        if (!record_res.ok()) {
            std::cerr << "Ошибка чтения записи: " << record_res.status().message << "\n";
            continue;
        }

        Record rec = record_res.value();
        print_record(rec);
    }
    std::cout << "-------------------------------------------------------------\n";

    page_mgr_read.close();
    std::cout << "\nПрограмма успешно завершена!\n";

    return 0;
}