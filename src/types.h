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
// ИСКЛЮЧЕНИЯ И СИСТЕМА ТИПОВ
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

// ============================================================================
// КЛАСС VALUE (std::variant wrapper)
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

    // Безопасное извлечение значений
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

    // Преобразование в строку для вывода (для Человека B / JSON / CLI)
    std::string to_string() const {
        if (is_null()) return "NULL";
        if (std::holds_alternative<int32_t>(data)) {
            return std::to_string(std::get<int32_t>(data));
        }
        return std::get<std::string>(data);
    }

    // ========================================================================
    // ЛОГИКА СРАВНЕНИЯ (Для B+-дерева Человека A и WHERE-условий Человека B)
    // ========================================================================

    bool operator==(const Value& other) const {
        // Оба NULL -> равны на уровне внутренних структур данных
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
        // Правило: NULL считаются наименьшим возможным значением
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

// Оператор вывода в поток (для отладки std::cout)
inline std::ostream& operator<<(std::ostream& os, const Value& val) {
    os << val.to_string();
    return os;
}

#endif // TYPES_H