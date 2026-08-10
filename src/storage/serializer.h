#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "../types.h"
#include <vector>
#include <cstdint>
#include <cstring>

class Serializer {
public:
    // ========================================================================
    // Сериализация: Преобразование вектора Value в вектор байт
    // ========================================================================
    static std::vector<uint8_t> serialize_fields(const std::vector<Value>& fields, 
                                                 const std::vector<ColumnDef>& schema);

    // ========================================================================
    // Десериализация: Восстановление вектора Value из сырого массива байт
    // ========================================================================
    static Result<std::vector<Value>> deserialize_fields(const uint8_t* data, 
                                                         size_t length, 
                                                         const std::vector<ColumnDef>& schema);

    // Вычисление размера NULL-битмаски в байтах
    static size_t get_null_bitmap_size(size_t column_count) {
        return (column_count + 7) / 8;
    }

    // Вычисление точного размера записи в байтах (для проверок свободного места)
    static size_t get_serialized_size(const std::vector<Value>& fields, 
                                      const std::vector<ColumnDef>& schema);
};

#endif // SERIALIZER_H