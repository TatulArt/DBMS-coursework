#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include "utils/Error.h"

// ============================================================================
// 1. БАЗОВЫЕ ТИПЫ И ОШИБКИ
// ============================================================================

// Номер страницы в файле БД. Объявляется первым: на него опираются
// INVALID_PAGE_ID, RecordID, METADATA_PAGE_ID и DatabaseMetadata ниже.
using PageId = uint32_t;

#ifndef INVALID_PAGE_ID
constexpr PageId INVALID_PAGE_ID = 0xFFFFFFFF;
#endif

// Ошибка обращения к значению не того типа. Раньше объявлялась в
// utils/Error.h, сейчас её там нет, а Value::get_int() и get_string()
// её бросают — поэтому объявление живёт здесь.
class TypeError : public std::runtime_error {
public:
    explicit TypeError(const std::string& message) : std::runtime_error(message) {}
};

struct RecordID {
    PageId page_id{0};
    uint16_t slot_id{0};
};

using RecordId = RecordID;

// ============================================================
// 2. ТИПЫ ДЛЯ ДАННЫХ (Идеальный мост-адаптер)
// ============================================================

enum class ColType { 
    INT, 
    STRING,
    Int = INT,       
    String = STRING  
};

using ColumnType = ColType; 

// Делаем класс Value наследником std::optional
class Value : public std::optional<std::variant<int, std::string>> {
public:
    using std::optional<std::variant<int, std::string>>::optional;
    using std::optional<std::variant<int, std::string>>::operator=;

    Value(const char* val) : std::optional<std::variant<int, std::string>>(std::string(val)) {}
    static Value Null() { return Value(); }
    // Методы, которые ищут table_indexer.cpp и index_manager.cpp
    bool is_null() const { return !this->has_value(); }
    bool is_int() const { return this->has_value() && std::holds_alternative<int>(**this); }
    bool is_string() const { return this->has_value() && std::holds_alternative<std::string>(**this); }

    int get_int() const { 
        if (!is_int()) throw TypeError("Value is not an INT");
        return std::get<int>(**this); 
    }
    
    const std::string& get_string() const { 
        if (!is_string()) throw TypeError("Value is not a STRING");
        return std::get<std::string>(**this); 
    }

    ColumnType get_type() const {
        if (is_int()) return ColumnType::INT;
        return ColumnType::STRING;
    }

    std::string to_string() const {
        if (is_null()) return "NULL";
        if (is_int()) return std::to_string(get_int());
        return get_string();
    }

    // NULL меньше любого значения, а сравнение значений разных типов —
    // семантическая ошибка: DBMS_Engine ловит её и показывает пользователю,
    // как требует задание. Тот же контракт проверяет tests/test_types.cpp.
    friend bool operator==(const Value& lhs, const Value& rhs) {
        if (lhs.is_null() && rhs.is_null()) return true;
        if (lhs.is_null() || rhs.is_null()) return false;
        if (lhs.is_int() != rhs.is_int()) {
            throw TypeError("Type mismatch in comparison: cannot compare different column types");
        }
        return lhs.is_int() ? lhs.get_int() == rhs.get_int()
                            : lhs.get_string() == rhs.get_string();
    }

    friend bool operator!=(const Value& lhs, const Value& rhs) { return !(lhs == rhs); }

    friend bool operator<(const Value& lhs, const Value& rhs) {
        if (lhs.is_null() && !rhs.is_null()) return true;
        if (rhs.is_null()) return false;   // включая случай "оба NULL"
        if (lhs.is_int() != rhs.is_int()) {
            throw TypeError("Type mismatch in comparison: cannot compare different column types");
        }
        return lhs.is_int() ? lhs.get_int() < rhs.get_int()
                            : lhs.get_string() < rhs.get_string();
    }

    friend bool operator<=(const Value& lhs, const Value& rhs) { return (lhs < rhs) || (lhs == rhs); }
    friend bool operator>(const Value& lhs, const Value& rhs)  { return !(lhs <= rhs); }
    friend bool operator>=(const Value& lhs, const Value& rhs) { return !(lhs < rhs); }
};


namespace val {
    inline bool isNull(const Value& v) { return v.is_null(); }
    inline bool isInt(const Value& v) { return v.is_int(); }
    inline bool isString(const Value& v) { return v.is_string(); }

    inline int getInt(const Value& v) { return v.get_int(); }
    inline const std::string& getString(const Value& v) { return v.get_string(); }
    
    inline bool is_null(const Value& v) { return v.is_null(); }
    inline int get_int(const Value& v) { return v.get_int(); }
    inline const std::string& get_string(const Value& v) { return v.get_string(); }
    
    inline Value Null() { return Value(); }
    
    inline std::string to_string(const Value& v) { return v.to_string(); }
}

inline std::string columnTypeToString(ColType type) {
    return (type == ColType::INT) ? "INT" : "STRING";
}

// Сравнения объектов Value
inline bool valueLess(const Value& a, const Value& b) {
    if (a.is_null() || b.is_null()) return false;
    if (a.is_int() && b.is_int()) return a.get_int() < b.get_int();
    if (a.is_string() && b.is_string()) return a.get_string() < b.get_string();
    return false;
}

inline bool valueEqual(const Value& a, const Value& b) {
    if (a.is_null() && b.is_null()) return true;
    if (a.is_null() || b.is_null()) return false;
    if (a.is_int() && b.is_int()) return a.get_int() == b.get_int();
    if (a.is_string() && b.is_string()) return a.get_string() == b.get_string();
    return false;
}

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    os << v.to_string();
    return os;
}

// ============================================================
// 3. ОПРЕДЕЛЕНИЕ СТРУКТУР СУБД
// ============================================================

struct ColumnDef {
    std::string name;
    ColType type{ColType::INT};

    // Ровно по одному полю на свойство. Раньше здесь было три пары имён
    // (indexed/is_indexed, notNull/is_nullable, defaultValue/default_value)
    // с раздельным хранением: запись шла в одно имя, чтение из другого,
    // и признак молча терялся.
    bool is_nullable{true};
    bool is_indexed{false};
    Value default_value;
};



struct Record {
    RecordID id;
    std::vector<Value> fields;
};

// ============================================================
// 4. СТАТУСЫ И ШАБЛОН РЕЗУЛЬТАТА (Result)
// ============================================================

enum class StatusCode {
    OK, IOError, InvalidArgument, NotFound, CorruptedData, RecordNotFound,
    UniqueConstraintViolation, NullConstraintViolation, TypeMismatch, ColumnNotFound = NotFound
};

// Результат операции: код и человекочитаемое описание.
struct Status {
    StatusCode code{StatusCode::OK};
    std::string message;

    static Status OK() { return Status{StatusCode::OK, ""}; }
    static Status Error(StatusCode code, const std::string& message) { return Status{code, message}; }

    bool ok() const { return code == StatusCode::OK; }
};

template <typename T>
class Result {
private:
    Status status_;
    std::optional<T> value_;

public:
    Result() : status_(Status::OK()), value_(std::nullopt) {}
    Result(const T& v) : status_(Status::OK()), value_(v) {}
    Result(Status s) : status_(s), value_(std::nullopt) {}

    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }
    
    const T& value() const { return *value_; }
    T& value() { return *value_; }
};

// ============================================================================
// 5. МЕТАДАННЫЕ БАЗЫ ДАННЫХ
// ============================================================================

constexpr PageId METADATA_PAGE_ID = 0;
constexpr uint32_t DB_MAGIC_NUMBER = 0xBEEFCAFE;

#pragma pack(push, 1)
struct DatabaseMetadata {
    uint32_t magic_number;
    PageId root_page_id;          
    PageId index_catalog_page_id; 
};
#pragma pack(pop)

inline Value Value_Null() { return Value(); }

#endif // TYPES_H
