#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <variant>
#include <string>
#include <iostream>

// ============================================================
// 1. ТИПЫ ДЛЯ ХРАНЕНИЯ НА ДИСКЕ
// ============================================================

using PageId = uint32_t;

struct RecordID {
    PageId page_id;
    uint16_t slot_id;
};

// Алиас для совместимости
using RecordId = RecordID;

// ============================================================
// 2. ТИПЫ ДЛЯ ДАННЫХ (Value и ColumnDef)
// ============================================================

enum class ColType { INT, STRING };

// Алиас для совместимости со storage
using ColumnType = ColType;

// Основной тип значения: NULL, int или string
using Value = std::optional<std::variant<int, std::string>>;

// Вспомогательные функции для работы с Value
namespace val {
    // camelCase (основные)
    inline bool isNull(const Value& v) { return !v.has_value(); }
    inline bool isInt(const Value& v) { return v.has_value() && std::holds_alternative<int>(*v); }
    inline bool isString(const Value& v) { return v.has_value() && std::holds_alternative<std::string>(*v); }

    inline int getInt(const Value& v) { return std::get<int>(*v); }
    inline const std::string& getString(const Value& v) { return std::get<std::string>(*v); }
    
    // snake_case (для совместимости со storage)
    inline bool is_null(const Value& v) { return isNull(v); }
    inline int get_int(const Value& v) { return getInt(v); }
    inline const std::string& get_string(const Value& v) { return getString(v); }
    
    // Для совместимости
    inline Value Null() { return std::nullopt; }
    
    inline std::string to_string(const Value& v) {
        if (isNull(v)) return "NULL";
        if (isInt(v)) return std::to_string(getInt(v));
        if (isString(v)) return getString(v);
        return "?";
    }
}

// Сравнение двух Value (для WHERE)
inline bool valueLess(const Value& a, const Value& b) {
    if (!a || !b) return false;
    if (val::isInt(a) && val::isInt(b)) return val::getInt(a) < val::getInt(b);
    if (val::isString(a) && val::isString(b)) return val::getString(a) < val::getString(b);
    return false;
}

inline bool valueEqual(const Value& a, const Value& b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return *a == *b;
}

// Оператор вывода для Value
inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    os << val::to_string(v);
    return os;
}

// ============================================================
// 3. ОПРЕДЕЛЕНИЕ КОЛОНКИ
// ============================================================

struct ColumnDef {
    std::string name;
    ColType type;
    bool notNull = false;
    bool indexed = false;
    Value defaultValue;
    bool is_nullable = true;  // для совместимости со storage
};

// ============================================================
// 4. СТАТУСЫ И РЕЗУЛЬТАТЫ
// ============================================================

enum class StatusCode {
    OK,
    IOError,
    InvalidArgument,
    NotFound,
    CorruptedData,
    RecordNotFound
};

struct Status {
    StatusCode code;
    std::string message;

    static Status OK() { return {StatusCode::OK, ""}; }
    static Status Error(StatusCode c, const std::string& msg) { return {c, msg}; }

    bool ok() const { return code == StatusCode::OK; }
    std::string error() const { return message; }
};

inline std::ostream& operator<<(std::ostream& os, const Value& val) {
    os << val.to_string();
    return os;
}

// ============================================================================
// 4. ФИЗИЧЕСКАЯ ИДЕНТИФИКАЦИЯ ЗАПИСЕЙ И ЗАПИСЬ (RECORD)
// ============================================================================

using PageId = uint32_t;
#ifndef INVALID_PAGE_ID
constexpr PageId INVALID_PAGE_ID = 0xFFFFFFFF;
#endif

// Уникальный адрес строки внутри СУБД (Номер страницы + Индекс слота)
struct RecordId {
    PageId page_id{0};
    uint16_t slot_id{0};

public:
    Result() : status_(Status::OK()), hasValue_(false) {}
    Result(const T& v) : status_(Status::OK()), value_(v), hasValue_(true) {}
    Result(Status s) : status_(s), hasValue_(false) {}

    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }
    const T& value() const { return value_; }
    T& value() { return value_; }
};

// ============================================================
// 5. ЗАПИСЬ
// ============================================================

struct Record {
    RecordID id;
    std::vector<Value> fields;
};

// ============================================================================
// 6. МЕТАДАННЫЕ БАЗЫ ДАННЫХ И СТРАНИЦЫ (DATABASE METADATA)
// ============================================================================

constexpr PageId METADATA_PAGE_ID = 0;
constexpr uint32_t DB_MAGIC_NUMBER = 0xBEEFCAFE;

#pragma pack(push, 1)
struct DatabaseMetadata {
    uint32_t magic_number;
    PageId root_page_id;          // Корень «главного» дерева файла
    PageId index_catalog_page_id; // Начало цепочки страниц каталога индексов
};
#pragma pack(pop)


// ============================================================
// 7. ДЛЯ СОВМЕСТИМОСТИ СО STORAGE
// ============================================================

// Функция-заглушка для Value::Null()
inline Value Value_Null() { return std::nullopt; }

#endif // TYPES_H
