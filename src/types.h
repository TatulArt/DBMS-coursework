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
// 1. СТАТУСЫ И ОШИБКИ (STATUS SYSTEM)
// ============================================================================

#ifndef INVALID_PAGE_ID
constexpr PageId INVALID_PAGE_ID = 0xFFFFFFFF;
#endif

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

    friend bool operator==(const Value& lhs, const Value& rhs) {
        if (lhs.is_null() && rhs.is_null()) return true;
        if (lhs.is_null() || rhs.is_null()) return false;
        if (lhs.is_int() && rhs.is_int()) return lhs.get_int() == rhs.get_int();
        if (lhs.is_string() && rhs.is_string()) return lhs.get_string() == rhs.get_string();
        return false;
    }

    friend bool operator!=(const Value& lhs, const Value& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const Value& lhs, const Value& rhs) {
        if (lhs.is_null() || rhs.is_null()) return false;
        if (lhs.is_int() && rhs.is_int()) return lhs.get_int() < rhs.get_int();
        if (lhs.is_string() && rhs.is_string()) return lhs.get_string() < rhs.get_string();
        return false;
    }

    friend bool operator<=(const Value& lhs, const Value& rhs) {
        return (lhs < rhs) || (lhs == rhs);
    }

    friend bool operator>(const Value& lhs, const Value& rhs) {
        return rhs < lhs;
    }

    friend bool operator>=(const Value& lhs, const Value& rhs) {
        return (rhs < lhs) || (lhs == rhs);
    }
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
    ColType type;
    bool notNull = false;
    bool indexed = false;     
    Value defaultValue;
    
    bool is_nullable = true;  
    bool is_indexed = false; 

    // Метод для моментальной синхронизации двух флагов, чтобы не путаться
    void sync_flags() {
        if (indexed) is_indexed = true;
        if (is_indexed) indexed = true;
    }
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

enum class ColumnType {
    Int,
    String
};

inline std::string columnTypeToString(ColumnType type) {
    switch (type) {
        case ColumnType::Int: return "INT";
        case ColumnType::String: return "STRING";
    }
    return "UNKNOWN";
}

// ============================================================================
// 3. КЛАСС VALUE (std::variant wrapper)
// ============================================================================

// std::monostate представляет NULL в SQL
using RawValue = std::variant<std::monostate, int32_t, std::string>;

class Value {
public:
    RawValue data;

    // Конструкторы
    Value() : data(std::monostate{}) {}                             // NULL value
    Value(int32_t val) : data(val) {}                               // INT
    Value(const std::string& val) : data(val) {}                    // STRING
    Value(const char* val) : data(std::string(val)) {}              // C-string -> STRING

    // Фабричный метод для явного NULL
    static Value Null() {
        return Value();
    }

    // Проверки типа
    bool is_null() const {
        return std::holds_alternative<std::monostate>(data);
    }

    ColumnType get_type() const {
        if (std::holds_alternative<int32_t>(data)) return ColumnType::Int;
        if (std::holds_alternative<std::string>(data)) return ColumnType::String;
        throw TypeError("Cannot retrieve ColumnType for a NULL Value");
    }

    // Извлечение значений
    int32_t get_int() const {
        if (!std::holds_alternative<int32_t>(data)) {
            throw TypeError("Value is not an INT");
        }
        return std::get<int32_t>(data);
    }

    const std::string& get_string() const {
        if (!std::holds_alternative<std::string>(data)) {
            throw TypeError("Value is not a STRING");
        }
        return std::get<std::string>(data);
    }

    // Преобразование в строку для вывода
    std::string to_string() const {
        if (is_null()) return "NULL";
        if (std::holds_alternative<int32_t>(data)) {
            return std::to_string(std::get<int32_t>(data));
        }
        return std::get<std::string>(data);
    }

    // Логика сравнения
    bool operator==(const Value& other) const {
        if (is_null() && other.is_null()) return true;
        if (is_null() || other.is_null()) return false;
        
        if (data.index() != other.data.index()) {
            throw TypeError("Type mismatch in comparison: cannot compare different column types");
        }
        return data == other.data;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator<(const Value& other) const {
        if (is_null() && !other.is_null()) return true;
        if (!is_null() && other.is_null()) return false;
        if (is_null() && other.is_null()) return false;

        if (data.index() != other.data.index()) {
            throw TypeError("Type mismatch in comparison: cannot compare different column types");
        }
        return data < other.data;
    }

    bool operator<=(const Value& other) const {
        return (*this < other) || (*this == other);
    }

    bool operator>(const Value& other) const {
        return !(*this <= other);
    }

    bool operator>=(const Value& other) const {
        return !(*this < other);
    }
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
