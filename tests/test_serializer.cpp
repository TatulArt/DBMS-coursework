#include <gtest/gtest.h>
#include "../src/storage/serializer.h"

class SerializerTest : public ::testing::Test {
protected:
    std::vector<ColumnDef> schema;

    void SetUp() override {
        // Задаём тестовую схему: id (INT), name (STRING), age (INT, Nullable)
        schema = {
            {"id", ColumnType::Int, false, true},
            {"name", ColumnType::String, false, false},
            {"age", ColumnType::Int, true, false}
        };
    }
};

// ============================================================================
// ТЕСТ 1: Базовая сериализация и десериализация корректных данных
// ============================================================================
TEST_F(SerializerTest, BasicEncodeDecode) {
    std::vector<Value> original_fields = {
        Value(101),
        Value("Database Engine"),
        Value(25)
    };

    // 1. Проверяем вычисление размера в байтах:
    // 1 байт (bitmap) + 4 байта (int) + (2 байта len + 15 байт str) + 4 байта (int) = 26 байт
    size_t expected_size = 1 + 4 + (2 + 15) + 4;
    EXPECT_EQ(Serializer::get_serialized_size(original_fields, schema), expected_size);

    // 2. Сериализация
    std::vector<uint8_t> bytes = Serializer::serialize_fields(original_fields, schema);
    EXPECT_EQ(bytes.size(), expected_size);

    // 3. Десериализация
    auto result = Serializer::deserialize_fields(bytes.data(), bytes.size(), schema);
    ASSERT_TRUE(result.ok());

    auto decoded_fields = result.value();
    EXPECT_EQ(decoded_fields.size(), 3);
    EXPECT_EQ(decoded_fields[0].get_int(), 101);
    EXPECT_EQ(decoded_fields[1].get_string(), "Database Engine");
    EXPECT_EQ(decoded_fields[2].get_int(), 25);
}

// ============================================================================
// ТЕСТ 2: Проверка работы NULL-полей и битмаски
// ============================================================================
TEST_F(SerializerTest, NullValuesBitmapHandling) {
    std::vector<Value> original_fields = {
        Value(42),
        Value("Alice"),
        Value::Null() // Третье поле = NULL
    };

    // Сериализуем
    auto bytes = Serializer::serialize_fields(original_fields, schema);

    // Проверяем 1-й байт (NULL bitmap): бит #2 (1 << 2 = 4) должен быть взведён
    EXPECT_EQ(bytes[0] & (1 << 2), 4);

    // Десериализуем
    auto result = Serializer::deserialize_fields(bytes.data(), bytes.size(), schema);
    ASSERT_TRUE(result.ok());

    auto decoded_fields = result.value();
    EXPECT_FALSE(decoded_fields[0].is_null());
    EXPECT_FALSE(decoded_fields[1].is_null());
    EXPECT_TRUE(decoded_fields[2].is_null()); // Поле восстановилось как Value::Null()
}

// ============================================================================
// ТЕСТ 3: Проверка пустых строк
// ============================================================================
TEST_F(SerializerTest, EmptyStringHandling) {
    std::vector<Value> original_fields = {
        Value(0),
        Value(""), // Пустая строка
        Value::Null()
    };

    auto bytes = Serializer::serialize_fields(original_fields, schema);
    auto result = Serializer::deserialize_fields(bytes.data(), bytes.size(), schema);
    ASSERT_TRUE(result.ok());

    EXPECT_EQ(result.value()[1].get_string(), "");
}

// ============================================================================
// ТЕСТ 4: Обработка ошибок (битые буферы данных)
// ============================================================================
TEST_F(SerializerTest, CorruptedDataHandling) {
    std::vector<Value> original_fields = { Value(1), Value("Test"), Value(10) };
    auto bytes = Serializer::serialize_fields(original_fields, schema);

    // Имитируем обрезку данных (передаём размер на 2 байта меньше, чем нужно)
    auto result = Serializer::deserialize_fields(bytes.data(), bytes.size() - 2, schema);
    
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code, StatusCode::CorruptedData);
}