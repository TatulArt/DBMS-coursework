#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>
#include "../types.h"

// ============================================================================
// Типы ключей B+ дерева.
//
// Дерево шаблонное по типу ключа. Требования к типу ключа:
//   * тривиально копируемый и с выравниванием, кратным 4 (ключи лежат прямо
//     в байтах страницы, следом за ними идёт массив RecordId/PageId);
//   * полный порядок через операторы < == > >= ;
//   * функция key_to_string() для сообщений об ошибках.
//
// Инстанцируются два ключа: int32_t для колонок INT и StringKey для STRING.
// ============================================================================

// --- Ключ для колонок типа INT -------------------------------------------

inline std::string key_to_string(int32_t key) {
    return std::to_string(key);
}

// --- Ключ для колонок типа STRING ----------------------------------------
//
// Строка фиксированной длины, хранимая прямо в странице. Сравнение
// лексикографическое и совпадает с порядком std::string, как требует
// задание («сравнение строк — лексикографическое»).
//
// Длина ограничена: в узел дерева должно помещаться разумное число ключей.
// Строки длиннее MAX_LENGTH индексировать нельзя — обрезание сделало бы
// разные значения одинаковыми ключами и сломало бы проверку уникальности
// INDEXED-колонки, поэтому такое значение отклоняется явной ошибкой.
#pragma pack(push, 1)
struct StringKey {
    static constexpr size_t MAX_LENGTH = 126;

    uint8_t length{0};
    uint8_t reserved{0};
    char data[MAX_LENGTH]{};

    StringKey() = default;

    std::string to_std_string() const {
        return std::string(data, length);
    }

    // Построение ключа из значения колонки.
    // Возвращает ошибку, если строка не помещается в ключ.
    static Result<StringKey> from_string(const std::string& value) {
        if (value.size() > MAX_LENGTH) {
            return Result<StringKey>(Status::Error(
                StatusCode::InvalidArgument,
                "Value is too long for a string index: " + std::to_string(value.size()) +
                    " bytes, limit is " + std::to_string(MAX_LENGTH)));
        }

        StringKey key;
        key.length = static_cast<uint8_t>(value.size());
        // Хвост обнуляем, чтобы одинаковые строки давали одинаковые байты
        std::memset(key.data, 0, MAX_LENGTH);
        std::memcpy(key.data, value.data(), value.size());
        return Result<StringKey>(key);
    }

    int compare(const StringKey& other) const {
        const size_t common = std::min<size_t>(length, other.length);
        // memcmp сравнивает байты как unsigned char — ровно то же,
        // что делает std::string::compare
        const int diff = std::memcmp(data, other.data, common);
        if (diff != 0) return diff;
        if (length == other.length) return 0;
        return (length < other.length) ? -1 : 1;
    }

    bool operator==(const StringKey& other) const { return compare(other) == 0; }
    bool operator!=(const StringKey& other) const { return compare(other) != 0; }
    bool operator<(const StringKey& other) const  { return compare(other) < 0; }
    bool operator<=(const StringKey& other) const { return compare(other) <= 0; }
    bool operator>(const StringKey& other) const  { return compare(other) > 0; }
    bool operator>=(const StringKey& other) const { return compare(other) >= 0; }
};
#pragma pack(pop)

// Размер кратен 4, поэтому массив RecordId/PageId, лежащий сразу за
// массивом ключей, остаётся выровненным
static_assert(sizeof(StringKey) == 128, "StringKey должен занимать 128 байт");
static_assert(sizeof(StringKey) % 4 == 0, "Размер ключа должен быть кратен 4");

inline std::string key_to_string(const StringKey& key) {
    return key.to_std_string();
}
