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

// Заголовок Slotted Page (8 байт).
// next_page_id связывает страницы одной таблицы в цепочку: куча таблицы
// больше не ограничена одной страницей.
struct SlottedPageHeader {
    uint16_t slot_count{0};        // Кол-во слотов в массиве
    uint16_t free_space_offset{PAGE_SIZE}; // Указатель на свободу (растёт с 4096 вниз)
    PageId next_page_id{INVALID_PAGE_ID};  // Следующая страница кучи таблицы
};

static_assert(sizeof(SlottedPageHeader) == 8, "SlottedPageHeader должен занимать 8 байт");

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

    // Перечисление всех живых записей страницы (удалённые слоты пропускаются).
    // Нужен для полного скана таблицы и для построения индекса по колонке.
    Result<std::vector<Record>> scan_page(PageId page_id, const std::vector<ColumnDef>& schema);

    // Хватит ли на странице места под запись длиной record_size байт.
    // Учитывается и место, занятое удалёнными записями: insert_record
    // уплотнит страницу, если непрерывного куска не хватает.
    Result<bool> page_has_space(PageId page_id, size_t needed_bytes);

    // ------------------------------------------------------------------------
    // ЦЕПОЧКА СТРАНИЦ КУЧИ
    // ------------------------------------------------------------------------

    // Разметить страницу как пустую страницу кучи (обнуляет слоты и ссылку)
    Status init_page(PageId page_id);

    // Следующая страница цепочки (INVALID_PAGE_ID, если страница последняя)
    Result<PageId> next_page(PageId page_id);

    // Присоединить страницу next к концу страницы page_id
    Status set_next_page(PageId page_id, PageId next);

    // Уплотнить страницу: сдвинуть живые записи вплотную к концу страницы,
    // вернув место удалённых. Идентификаторы слотов при этом сохраняются,
    // поэтому ссылки RecordId из B+ деревьев остаются валидными.
    Status compact_page(PageId page_id);

    // Сколько байт занимает запись после сериализации
    static size_t record_size(const std::vector<Value>& fields,
                              const std::vector<ColumnDef>& schema);

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