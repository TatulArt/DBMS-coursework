#include "record_manager.h"
#include "serializer.h"
#include <algorithm>

// ============================================================================
// СЕРИАЛИЗАЦИЯ СТРОКИ В БАЙТОВЫЙ МАССИВ
// Формат байт записи:
// [ NULL Bitmap (N байт) ] [ Value 1 ] [ Value 2 ] ...
// - Int: 4 байта (int32_t)
// - String: 2 байта длины (uint16_t) + символы строки
// ============================================================================
std::vector<uint8_t> RecordManager::serialize_record(const std::vector<Value>& fields, 
                                                     const std::vector<ColumnDef>& schema) {
    std::vector<uint8_t> buffer;
    
    size_t null_bitmap_bytes = calculate_null_bitmap_size(schema.size());
    std::vector<uint8_t> null_bitmap(null_bitmap_bytes, 0);

    // 1. Формируем NULL-битмаску
    for (size_t i = 0; i < fields.size(); ++i) {
        if (val::is_null(fields[i])) {
            null_bitmap[i / 8] |= (1 << (i % 8)); // Ставим 1, если NULL
        }
    }

    // Записываем NULL-битмаску в начало буфера
    buffer.insert(buffer.end(), null_bitmap.begin(), null_bitmap.end());

    // 2. Сериализуем сами значения
    for (size_t i = 0; i < fields.size(); ++i) {
        if (val::is_null(fields[i])) continue; // Пропускаем тела NULL-полей

        if (schema[i].type == ColType::INT) {
            int32_t val = val::get_int(fields[i]);
            uint8_t bytes[sizeof(int32_t)];
            std::memcpy(bytes, &val, sizeof(int32_t));
            buffer.insert(buffer.end(), bytes, bytes + sizeof(int32_t));
        } 
        else if (schema[i].type == ColType::STRING) {
            const std::string& str = val::get_string(fields[i]);
            uint16_t len = static_cast<uint16_t>(str.size());
            
            // Записываем 2 байта длины
            uint8_t len_bytes[sizeof(uint16_t)];
            std::memcpy(len_bytes, &len, sizeof(uint16_t));
            buffer.insert(buffer.end(), len_bytes, len_bytes + sizeof(uint16_t));
            
            // Записываем сами символы
            buffer.insert(buffer.end(), str.begin(), str.end());
        }
    }

    return buffer;
}

// ============================================================================
// ДЕСЕРИАЛИЗАЦИЯ БАЙТОВОГО МАССИВА В RECORD
// ============================================================================
Result<std::vector<Value>> RecordManager::deserialize_record(const uint8_t* data, 
                                                             size_t length, 
                                                             const std::vector<ColumnDef>& schema) {
    std::vector<Value> fields;
    size_t null_bitmap_bytes = calculate_null_bitmap_size(schema.size());
    
    if (length < null_bitmap_bytes) {
        return Status::Error(StatusCode::CorruptedData, "Record byte buffer is too short for NULL bitmap");
    }

    size_t offset = null_bitmap_bytes;

    for (size_t i = 0; i < schema.size(); ++i) {
        // Проверяем бит в NULL-маске
        bool is_null = (data[i / 8] & (1 << (i % 8))) != 0;

        if (is_null) {
            fields.push_back(val::Null());
            continue;
        }

        if (schema[i].type == ColType::INT) {
            if (offset + sizeof(int32_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: unexpected end of INT field");
            }
            int32_t val;
            std::memcpy(&val, data + offset, sizeof(int32_t));
            offset += sizeof(int32_t);
            fields.push_back(Value(val));
        } 
        else if (schema[i].type == ColType::STRING) {
            if (offset + sizeof(uint16_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: unexpected end of STRING length");
            }
            uint16_t str_len;
            std::memcpy(&str_len, data + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            if (offset + str_len > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: string boundary out of bounds");
            }

            std::string str(reinterpret_cast<const char*>(data + offset), str_len);
            offset += str_len;
            fields.push_back(Value(str));
        }
    }

    return fields;
}

// ============================================================================
// ВСТАВКА ЗАПИСИ НА СТРАНИЦУ (SLOTTED PAGE)
// ============================================================================

Result<RecordId> RecordManager::insert_record(PageId page_id, 
                                              const std::vector<Value>& fields, 
                                              const std::vector<ColumnDef>& schema) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    std::vector<uint8_t> record_bytes = Serializer::serialize_fields(fields, schema);
    uint16_t record_len = static_cast<uint16_t>(record_bytes.size());

    // Читаем заголовок страницы
    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    std::cout << "DEBUG: record_len = " << record_len << std::endl;

    // ИСПРАВЛЕНИЕ: Если страница абсолютно новая (free_space_offset == 0),
    // инициализируем указатель свободной памяти на конец страницы (4096)
    if (header.free_space_offset == 0) {
        header.free_space_offset = PAGE_SIZE;
    }

    // Вычисляем объём требуемого свободного места
    size_t needed_space = record_len + sizeof(Slot);
    size_t current_slot_array_end = sizeof(SlottedPageHeader) + header.slot_count * sizeof(Slot);

    if (header.free_space_offset < current_slot_array_end || 
        (header.free_space_offset - current_slot_array_end) < needed_space) {
        return Status::Error(StatusCode::IOError, "Page " + std::to_string(page_id) + " is full");
    }

    // Записываем тело записи в конец свободного места страницы
    uint16_t new_data_offset = header.free_space_offset - record_len;
    std::memcpy(page.data + new_data_offset, record_bytes.data(), record_len);

    // Заполняем новый слот
    Slot new_slot;
    new_slot.offset = new_data_offset;
    new_slot.length = record_len;

    uint16_t slot_idx = header.slot_count;
    std::memcpy(page.data + sizeof(SlottedPageHeader) + slot_idx * sizeof(Slot), &new_slot, sizeof(Slot));

    // Обновляем заголовок страницы
    header.slot_count++;
    header.free_space_offset = new_data_offset;
    std::memcpy(page.data, &header, sizeof(SlottedPageHeader));

    // Сбрасываем обновлённую страницу обратно на диск
    st = page_manager_.write_page(page_id, page);
    if (!st.ok()) return st;

    return RecordId{page_id, slot_idx};
}

// ============================================================================
// ЧТЕНИЕ ЗАПИСИ
// ============================================================================
Result<Record> RecordManager::get_record(RecordId id, const std::vector<ColumnDef>& schema) {
    Page page;
    Status st = page_manager_.read_page(id.page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));

    if (id.slot_id >= header.slot_count) {
        return Status::Error(StatusCode::InvalidArgument, "Invalid slot_id " + std::to_string(id.slot_id));
    }

    Slot slot;
    std::memcpy(&slot, page.data + sizeof(SlottedPageHeader) + id.slot_id * sizeof(Slot), sizeof(Slot));

    if (slot.length == 0) {
        return Status::Error(StatusCode::RecordNotFound, "Record was deleted");
    }

    auto fields_res = deserialize_record(page.data + slot.offset, slot.length, schema);
    if (!fields_res.ok()) return fields_res.status();

    return Record{id, fields_res.value()};
}

// ============================================================================
// УДАЛЕНИЕ ЗАПИСИ
// ============================================================================
Status RecordManager::delete_record(RecordId id) {
    Page page;
    Status st = page_manager_.read_page(id.page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));

    if (id.slot_id >= header.slot_count) {
        return Status::Error(StatusCode::InvalidArgument, "Invalid slot_id");
    }

    Slot slot;
    size_t slot_offset = sizeof(SlottedPageHeader) + id.slot_id * sizeof(Slot);
    std::memcpy(&slot, page.data + slot_offset, sizeof(Slot));

    // Помечаем слот как удалённый (length = 0)
    slot.length = 0;
    std::memcpy(page.data + slot_offset, &slot, sizeof(Slot));

    return page_manager_.write_page(id.page_id, page);
}