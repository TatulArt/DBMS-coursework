#ifndef RECORD_MANAGER_H
#define RECORD_MANAGER_H

#include "../types.h"
#include "page_manager.h"
#include <vector>
#include <cstdint>
#include <cstring>

// Структура слота в начале страницы (4 байта на запись)
struct Slot {
    uint16_t offset{0}; // Смещение данных записи внутри 4KB страницы
    uint16_t length{0}; // Длина данных (length == 0 означает, что запись была удалена)
};

// Заголовок Slotted Page (4 байта)
struct SlottedPageHeader {
    uint16_t slot_count{0};        // Кол-во слотов в массиве
    uint16_t free_space_offset{PAGE_SIZE}; // Указатель на свободу (растёт с 4096 вниз)
};

class RecordManager {
private:
    PageManager& page_manager_;

public:
    explicit RecordManager(PageManager& page_manager) 
        : page_manager_(page_manager) {}

    // ------------------------------------------------------------------------
    // CRUD ОПЕРАЦИИ НАД ЗАПИСЯМИ
    // ------------------------------------------------------------------------

    // Вставка строки в конкретную таблицу (с использованием заданной схемы)
    Result<RecordId> insert_record(PageId page_id, 
                                   const std::vector<Value>& fields, 
                                   const std::vector<ColumnDef>& schema);

    // Чтение строки по ее RecordId {page_id, slot_id}
    Result<Record> get_record(RecordId id, const std::vector<ColumnDef>& schema);

    // Удаление записи (помечает слот как удалённый)
    Status delete_record(RecordId id);

    // ------------------------------------------------------------------------
    // СЕРИАЛИЗАЦИЯИ ДЕСЕРИАЛИЗАЦИЯ (Record <-> raw bytes)
    // ------------------------------------------------------------------------

    static std::vector<uint8_t> serialize_record(const std::vector<Value>& fields, 
                                                 const std::vector<ColumnDef>& schema);

    static Result<std::vector<Value>> deserialize_record(const uint8_t* data, 
                                                         size_t length, 
                                                         const std::vector<ColumnDef>& schema);

private:
    static size_t calculate_null_bitmap_size(size_t column_count) {
        return (column_count + 7) / 8; // Кол-во байт для битовой маски NULL-полей
    }
};

#endif // RECORD_MANAGER_H