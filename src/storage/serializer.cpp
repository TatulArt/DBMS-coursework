#include "serializer.h"
#include <cstring>
#include <iostream>  // ← для отладки
#include "utils/Error.h"

size_t Serializer::get_serialized_size(const std::vector<Value>& fields, 
                                       const std::vector<ColumnDef>& schema) {
    size_t total_size = get_null_bitmap_size(schema.size());

    for (size_t i = 0; i < fields.size(); ++i) {
        if (val::is_null(fields[i])) continue;

        if (schema[i].type == ColType::INT) {
            total_size += sizeof(int32_t);
        } else if (schema[i].type == ColType::STRING) {
            total_size += sizeof(uint16_t) + val::get_string(fields[i]).size();
        }
    }
    return total_size;
}

std::vector<uint8_t> Serializer::serialize_fields(const std::vector<Value>& fields, 
                                                  const std::vector<ColumnDef>& schema) {
    if (fields.size() != schema.size()) {
        throw TypeError("Fields count does not match schema column count");
    }

    std::cout << "DEBUG serialize_fields: fields.size()=" << fields.size() 
              << ", schema.size()=" << schema.size() << std::endl;

    std::vector<uint8_t> buffer;
    size_t bitmap_size = get_null_bitmap_size(schema.size());
    std::vector<uint8_t> null_bitmap(bitmap_size, 0);

    for (size_t i = 0; i < fields.size(); ++i) {
        std::cout << "DEBUG serialize_fields: i=" << i 
                  << ", isNull=" << val::is_null(fields[i]) << std::endl;
        
        if (val::is_null(fields[i])) {
            if (schema[i].notNull) {
                throw TypeError("Column " + schema[i].name + " cannot be NULL");
            }
            null_bitmap[i / 8] |= (1 << (i % 8));
        }
    }

    buffer.insert(buffer.end(), null_bitmap.begin(), null_bitmap.end());

    for (size_t i = 0; i < fields.size(); ++i) {
        if (val::is_null(fields[i])) continue;

        std::cout << "DEBUG serialize_fields: writing field " << i 
                  << ", type=" << (schema[i].type == ColType::INT ? "INT" : "STRING") << std::endl;

        if (schema[i].type == ColType::INT) {
            int32_t val = val::get_int(fields[i]);
            std::cout << "DEBUG serialize_fields: int value=" << val << std::endl;
            uint8_t bytes[sizeof(int32_t)];
            std::memcpy(bytes, &val, sizeof(int32_t));
            buffer.insert(buffer.end(), bytes, bytes + sizeof(int32_t));
        } 
        else if (schema[i].type == ColType::STRING) {
            const std::string& str = val::get_string(fields[i]);
            std::cout << "DEBUG serialize_fields: string value=\"" << str << "\"" << std::endl;
            uint16_t len = static_cast<uint16_t>(str.size());
            
            uint8_t len_bytes[sizeof(uint16_t)];
            std::memcpy(len_bytes, &len, sizeof(uint16_t));
            buffer.insert(buffer.end(), len_bytes, len_bytes + sizeof(uint16_t));
            
            buffer.insert(buffer.end(), str.begin(), str.end());
        }
    }

    std::cout << "DEBUG serialize_fields: final buffer size=" << buffer.size() << std::endl;
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
        bool is_null = (data[i / 8] & (1 << (i % 8))) != 0;

        if (is_null) {
            fields.push_back(std::nullopt);
            continue;
        }

        if (schema[i].type == ColType::INT) {
            if (offset + sizeof(int32_t) > length) {
                return Status::Error(StatusCode::CorruptedData, "Corrupted record: end of INT reached unexpectedly");
            }
            int32_t val;
            std::memcpy(&val, data + offset, sizeof(int32_t));
            offset += sizeof(int32_t);
            fields.push_back(Value(val));
        } 
        else if (schema[i].type == ColType::STRING) {
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
