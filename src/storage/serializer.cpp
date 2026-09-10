#include "serializer.h"

size_t Serializer::get_serialized_size(const std::vector<Value>& fields, 
                                       const std::vector<ColumnDef>& schema) {
    size_t total_size = get_null_bitmap_size(schema.size());

    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].is_null()) continue;

        if (schema[i].type == ColumnType::Int) {
            total_size += sizeof(int32_t);
        } else if (schema[i].type == ColumnType::String) {
            total_size += sizeof(uint16_t) + fields[i].get_string().size();
        }
    }
    return total_size;
}

std::vector<uint8_t> Serializer::serialize_fields(const std::vector<Value>& fields, 
                                                  const std::vector<ColumnDef>& schema) {
    if (fields.size() != schema.size()) {
        throw TypeError("Fields count does not match schema column count");
    }

    std::vector<uint8_t> buffer;
    size_t bitmap_size = get_null_bitmap_size(schema.size());
    std::vector<uint8_t> null_bitmap(bitmap_size, 0);

    // 1. Формируем битмаску NULL-значений
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].is_null()) {
            if (!schema[i].is_nullable) {
                throw TypeError("Column " + schema[i].name + " cannot be NULL");
            }
            null_bitmap[i / 8] |= (1 << (i % 8));
        }
    }

    // Записываем NULL-битмаску в начало буфера
    buffer.insert(buffer.end(), null_bitmap.begin(), null_bitmap.end());

    // 2. Сериализуем данные
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].is_null()) continue; // ТЕЛО NULL НЕ ПИШЕТСЯ В ДИСК

        if (schema[i].type == ColumnType::Int) {
            int32_t val = fields[i].get_int();
            uint8_t bytes[sizeof(int32_t)];
            std::memcpy(bytes, &val, sizeof(int32_t));
            buffer.insert(buffer.end(), bytes, bytes + sizeof(int32_t));
        } 
        else if (schema[i].type == ColumnType::String) {
            const std::string& str = fields[i].get_string();
            uint16_t len = static_cast<uint16_t>(str.size());
            
            // Префикс длины (2 байта)
            uint8_t len_bytes[sizeof(uint16_t)];
            std::memcpy(len_bytes, &len, sizeof(uint16_t));
            buffer.insert(buffer.end(), len_bytes, len_bytes + sizeof(uint16_t));
            
            // Символы строки
            buffer.insert(buffer.end(), str.begin(), str.end());
        }
    }

    return buffer;
}

Result<std::vector<Value>> Serializer::deserialize_fields(const uint8_t* data, 
                                                          size_t length, 
                                                          const std::vector<ColumnDef>& schema) {
    std::vector<Value> fields;
    size_t bitmap_size = get_null_bitmap_size(schema.size());
    
    if (length < bitmap_size) {
        return Status::Error(StatusCode::CorruptedData, "Buffer too small for NULL bitmap");
    }

    size_t offset = bitmap_size;

    for (size_t i = 0; i < schema.size(); ++i) {
        // Проверяем бит NULL
        bool is_null = (data[i / 8] & (1 << (i % 8))) != 0;

        if (is_null) {
            fields.push_back(Value::Null());
            continue;
        }

        if (schema[i].type == ColumnType::Int) {
            if (offset + sizeof(int32_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: end of INT reached unexpectedly");
            }
            int32_t val;
            std::memcpy(&val, data + offset, sizeof(int32_t));
            offset += sizeof(int32_t);
            fields.push_back(Value(val));
        } 
        else if (schema[i].type == ColumnType::String) {
            if (offset + sizeof(uint16_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: end of STRING length reached unexpectedly");
            }
            uint16_t str_len;
            std::memcpy(&str_len, data + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            if (offset + str_len > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: STRING content out of bounds");
            }

            std::string str(reinterpret_cast<const char*>(data + offset), str_len);
            offset += str_len;
            fields.push_back(Value(str));
        }
    }

    return fields;
}