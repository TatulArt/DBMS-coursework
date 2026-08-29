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

// ============================================================================
// 1. СТАТУСЫ И ОШИБКИ (STATUS SYSTEM)
// ============================================================================

enum class StatusCode {
    OK = 0,
    DatabaseNotFound,
    DatabaseAlreadyExists,
    TableNotFound,
    TableAlreadyExists,
    ColumnNotFound,
    DuplicateColumn,
    NullConstraintViolation,
    UniqueConstraintViolation,
    TypeMismatch,
    IOError,
    CorruptedData,
    RecordNotFound,
    InvalidArgument
};

struct Status {
    StatusCode code{StatusCode::OK};
    std::string message;

    static Status OK() {
        return Status{StatusCode::OK, ""};
    }

    static Status Error(StatusCode code, const std::string& message) {
        return Status{code, message};
    }

    bool ok() const {
        return code == StatusCode::OK;
    }
};

template<typename T>
class Result {
private:
    Status status_;
    std::optional<T> value_;

public:
    Result(T value) : status_(Status::OK()), value_(std::move(value)) {}
    Result(Status status) : status_(std::move(status)), value_(std::nullopt) {}

    static Result<T> Error(StatusCode code, const std::string& message) {
        return Result<T>(Status::Error(code, message));
    }

    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }

    const T& value() const {
        if (!ok()) {
            throw std::runtime_error("Attempted to access value of unsuccessful Result: " + status_.message);
        }
        return *value_;
    }

    T& value() {
        if (!ok()) {
            throw std::runtime_error("Attempted to access value of unsuccessful Result: " + status_.message);
        }
        return *value_;
    }
};

// ============================================================================
// 2. ИСКЛЮЧЕНИЯ И ТИПЫ ДАННЫХ
// ============================================================================

class TypeError : public std::runtime_error {
public:
    explicit TypeError(const std::string& message) : std::runtime_error(message) {}
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

// Определение структуры колонки (Метаданные схемы)
struct ColumnDef {
    std::string name;
    ColumnType type;
    bool is_nullable{true};
    bool is_indexed{false};
};

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

    bool operator==(const RecordId& other) const {
        return page_id == other.page_id && slot_id == other.slot_id;
    }

    bool operator!=(const RecordId& other) const {
        return !(*this == other);
    }
};

// Запись в СУБД (Адрес + Набор полей)
struct Record {
    RecordId id;
    std::vector<Value> fields;
};

// ============================================================================
// 5. МЕТАДАННЫЕ БАЗЫ ДАННЫХ И СТРАНИЦЫ (DATABASE METADATA)
// ============================================================================

constexpr PageId METADATA_PAGE_ID = 0;
constexpr uint32_t DB_MAGIC_NUMBER = 0xBEEFCAFE;

#pragma pack(push, 1)
struct DatabaseMetadata {
    uint32_t magic_number;
    PageId root_page_id;
};
#pragma pack(pop)

#endif // TYPES_H