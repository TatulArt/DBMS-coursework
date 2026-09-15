#include "record_manager.h"
#include "serializer.h"
#include <algorithm>

namespace {

// Страница, только что выделенная allocate_page(), заполнена нулями.
// Нулевой заголовок означает не «страница с 0 слотов и 0 свободного места»,
// а «страница ещё не размечена»: приводим его к корректному виду.
void normalize_fresh_header(SlottedPageHeader& header) {
    if (header.free_space_offset == 0) {
        header.free_space_offset = PAGE_SIZE;

        // У неразмеченной страницы next_page_id тоже нулевой, а 0 — это
        // страница метаданных. Без этой поправки обход цепочки зациклился бы.
        if (header.slot_count == 0) {
            header.next_page_id = INVALID_PAGE_ID;
        }
    }
}

// Сколько байт на странице реально заняты живыми записями
uint32_t live_bytes_of(const Page& page, const SlottedPageHeader& header) {
    uint32_t used = 0;
    for (uint16_t i = 0; i < header.slot_count; ++i) {
        Slot slot;
        std::memcpy(&slot, page.data + sizeof(SlottedPageHeader) + i * sizeof(Slot), sizeof(Slot));
        used += slot.length;
    }
    return used;
}

} // namespace

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
        if (fields[i].is_null()) {
            null_bitmap[i / 8] |= (1 << (i % 8)); // Ставим 1, если NULL
        }
    }

    // Записываем NULL-битмаску в начало буфера
    buffer.insert(buffer.end(), null_bitmap.begin(), null_bitmap.end());

    // 2. Сериализуем сами значения
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].is_null()) continue; // Пропускаем тела NULL-полей

        if (schema[i].type == ColumnType::Int) {
            int32_t val = fields[i].get_int();
            uint8_t bytes[sizeof(int32_t)];
            std::memcpy(bytes, &val, sizeof(int32_t));
            buffer.insert(buffer.end(), bytes, bytes + sizeof(int32_t));
        } 
        else if (schema[i].type == ColumnType::String) {
            const std::string& str = fields[i].get_string();
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
            fields.push_back(Value::Null());
            continue;
        }

        if (schema[i].type == ColumnType::Int) {
            if (offset + sizeof(int32_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: unexpected end of INT field");
            }
            int32_t val;
            std::memcpy(&val, data + offset, sizeof(int32_t));
            offset += sizeof(int32_t);
            fields.push_back(Value(val));
        } 
        else if (schema[i].type == ColumnType::String) {
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
    normalize_fresh_header(header);

    // Вычисляем объём требуемого свободного места
    size_t needed_space = record_len + sizeof(Slot);
    size_t current_slot_array_end = sizeof(SlottedPageHeader) + header.slot_count * sizeof(Slot);

    const bool fits = header.free_space_offset >= current_slot_array_end &&
                      (header.free_space_offset - current_slot_array_end) >= needed_space;

    if (!fits) {
        // Непрерывного куска не хватает, но на странице могут быть «дыры»
        // от удалённых записей. Уплотняем страницу и пробуем ещё раз.
        std::memcpy(page.data, &header, sizeof(SlottedPageHeader));
        st = page_manager_.write_page(page_id, page);
        if (!st.ok()) return st;

        st = compact_page(page_id);
        if (!st.ok()) return st;

        st = page_manager_.read_page(page_id, page);
        if (!st.ok()) return st;
        std::memcpy(&header, page.data, sizeof(SlottedPageHeader));

        current_slot_array_end = sizeof(SlottedPageHeader) + header.slot_count * sizeof(Slot);
        if (header.free_space_offset < current_slot_array_end ||
            (header.free_space_offset - current_slot_array_end) < needed_space) {
            return Status::Error(StatusCode::IOError, "Page " + std::to_string(page_id) + " is full");
        }
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

    // Слот, указывающий за пределы страницы, означает повреждение данных.
    // Без этой проверки deserialize_record читал бы память за буфером страницы.
    if (static_cast<size_t>(slot.offset) + slot.length > PAGE_SIZE) {
        return Status::Error(StatusCode::CorruptedData,
                             "Slot " + std::to_string(id.slot_id) + " points outside of page " +
                             std::to_string(id.page_id));
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

// ============================================================================
// ПЕРЕЧИСЛЕНИЕ ЗАПИСЕЙ СТРАНИЦЫ (нужно для полного скана и построения индекса)
// ============================================================================
Result<std::vector<Record>> RecordManager::scan_page(PageId page_id,
                                                     const std::vector<ColumnDef>& schema) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));

    std::vector<Record> records;

    // Массив слотов не может выходить за пределы страницы
    const size_t max_slots = (PAGE_SIZE - sizeof(SlottedPageHeader)) / sizeof(Slot);
    if (header.slot_count > max_slots) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " reports impossible slot count");
    }

    for (uint16_t slot_idx = 0; slot_idx < header.slot_count; ++slot_idx) {
        Slot slot;
        std::memcpy(&slot, page.data + sizeof(SlottedPageHeader) + slot_idx * sizeof(Slot), sizeof(Slot));

        if (slot.length == 0) {
            continue; // Запись удалена
        }
        if (static_cast<size_t>(slot.offset) + slot.length > PAGE_SIZE) {
            return Status::Error(StatusCode::CorruptedData,
                                 "Slot " + std::to_string(slot_idx) + " points outside of page " +
                                 std::to_string(page_id));
        }

        auto fields_res = deserialize_record(page.data + slot.offset, slot.length, schema);
        if (!fields_res.ok()) return fields_res.status();

        records.push_back(Record{RecordId{page_id, slot_idx}, fields_res.value()});
    }

    return records;
}

// ============================================================================
// ПРОВЕРКА СВОБОДНОГО МЕСТА НА СТРАНИЦЕ
// ============================================================================
Result<bool> RecordManager::page_has_space(PageId page_id, size_t needed_bytes) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    normalize_fresh_header(header);

    // Новая запись всегда получает новый слот: переиспользовать слот удалённой
    // записи нельзя, иначе «висячая» ссылка RecordId из индекса стала бы
    // указывать на чужую строку.
    const size_t slot_array_end =
        sizeof(SlottedPageHeader) + (static_cast<size_t>(header.slot_count) + 1) * sizeof(Slot);

    // Считаем место, которое будет доступно ПОСЛЕ уплотнения страницы:
    // байты удалённых записей вернутся в оборот
    const size_t used = live_bytes_of(page, header);
    if (slot_array_end + used > PAGE_SIZE) {
        return false;
    }

    return (PAGE_SIZE - slot_array_end - used) >= needed_bytes;
}

size_t RecordManager::record_size(const std::vector<Value>& fields,
                                  const std::vector<ColumnDef>& schema) {
    return Serializer::get_serialized_size(fields, schema);
}

// ============================================================================
// ЦЕПОЧКА СТРАНИЦ КУЧИ
// ============================================================================

Status RecordManager::init_page(PageId page_id) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    page.clear();
    page.id = page_id;

    SlottedPageHeader header;
    header.slot_count = 0;
    header.free_space_offset = PAGE_SIZE;
    header.next_page_id = INVALID_PAGE_ID;
    std::memcpy(page.data, &header, sizeof(SlottedPageHeader));

    return page_manager_.write_page(page_id, page);
}

Result<PageId> RecordManager::next_page(PageId page_id) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return Result<PageId>(st);

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    normalize_fresh_header(header);

    // Ссылка на саму себя означала бы бесконечный обход цепочки
    if (header.next_page_id == page_id) {
        return Result<PageId>(Status::Error(
            StatusCode::CorruptedData,
            "Heap page " + std::to_string(page_id) + " points at itself"));
    }

    return Result<PageId>(header.next_page_id);
}

Status RecordManager::set_next_page(PageId page_id, PageId next) {
    if (next == page_id) {
        return Status::Error(StatusCode::InvalidArgument,
                             "Heap page cannot follow itself");
    }

    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    normalize_fresh_header(header);

    header.next_page_id = next;
    std::memcpy(page.data, &header, sizeof(SlottedPageHeader));

    return page_manager_.write_page(page_id, page);
}

// ============================================================================
// УПЛОТНЕНИЕ СТРАНИЦЫ
//
// Записи лежат в конце страницы и растут вниз, к массиву слотов. После
// удаления между ними остаются «дыры». Здесь живые записи переписываются
// подряд заново, а free_space_offset возвращается к фактической границе.
// Номера слотов не меняются: RecordId, сохранённые в B+ деревьях, остаются
// корректными.
// ============================================================================
Status RecordManager::compact_page(PageId page_id) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    SlottedPageHeader header;
    std::memcpy(&header, page.data, sizeof(SlottedPageHeader));
    normalize_fresh_header(header);

    const size_t max_slots = (PAGE_SIZE - sizeof(SlottedPageHeader)) / sizeof(Slot);
    if (header.slot_count > max_slots) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " reports impossible slot count");
    }

    // Собираем живые записи во временный буфер, затем раскладываем обратно
    std::vector<Slot> slots(header.slot_count);
    for (uint16_t i = 0; i < header.slot_count; ++i) {
        std::memcpy(&slots[i], page.data + sizeof(SlottedPageHeader) + i * sizeof(Slot), sizeof(Slot));
        if (slots[i].length != 0 &&
            static_cast<size_t>(slots[i].offset) + slots[i].length > PAGE_SIZE) {
            return Status::Error(StatusCode::CorruptedData,
                                 "Slot " + std::to_string(i) + " points outside of page " +
                                 std::to_string(page_id));
        }
    }

    std::vector<uint8_t> region(PAGE_SIZE, 0);
    uint16_t write_offset = PAGE_SIZE;

    // Идём от последнего слота к первому, укладывая записи от конца страницы:
    // так сохраняется исходный порядок данных внутри страницы
    for (uint16_t i = header.slot_count; i-- > 0;) {
        if (slots[i].length == 0) continue;

        write_offset = static_cast<uint16_t>(write_offset - slots[i].length);
        std::memcpy(region.data() + write_offset, page.data + slots[i].offset, slots[i].length);
        slots[i].offset = write_offset;
    }

    const size_t slot_array_end = sizeof(SlottedPageHeader) + header.slot_count * sizeof(Slot);
    if (write_offset < slot_array_end) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " does not fit its own live records");
    }

    // Область данных перезаписываем целиком: старые «хвосты» затираются нулями
    std::memcpy(page.data + slot_array_end, region.data() + slot_array_end, PAGE_SIZE - slot_array_end);

    for (uint16_t i = 0; i < header.slot_count; ++i) {
        std::memcpy(page.data + sizeof(SlottedPageHeader) + i * sizeof(Slot), &slots[i], sizeof(Slot));
    }

    header.free_space_offset = write_offset;
    std::memcpy(page.data, &header, sizeof(SlottedPageHeader));

    return page_manager_.write_page(page_id, page);
}
