#include <gtest/gtest.h>
#include "../src/types.h"

// Тест 1: Инициализация типов и проверкa is_null
TEST(ValueTest, InitializationAndTypes) {
    Value val_null;
    Value val_int(42);
    Value val_str("hello");

    EXPECT_TRUE(val_null.is_null());
    EXPECT_FALSE(val_int.is_null());
    EXPECT_FALSE(val_str.is_null());

    EXPECT_EQ(val_int.get_type(), ColumnType::Int);
    EXPECT_EQ(val_str.get_type(), ColumnType::String);
    EXPECT_EQ(val_int.get_int(), 42);
    EXPECT_EQ(val_str.get_string(), "hello");
}

// Тест 2: Преобразование типов в строку для вывода
TEST(ValueTest, ToStringRepresentation) {
    EXPECT_EQ(Value().to_string(), "NULL");
    EXPECT_EQ(Value(100).to_string(), "100");
    EXPECT_EQ(Value("test_db").to_string(), "test_db");
}

// Тест 3: Сравнение одинаковых типов
TEST(ValueTest, SameTypeComparisons) {
    Value a(10);
    Value b(20);
    Value c(10);

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(b > a);

    Value str1("apple");
    Value str2("banana");

    EXPECT_TRUE(str1 < str2);
    EXPECT_TRUE(str1 != str2);
}

// Тест 4: Логика работы с NULL в B+-дереве
TEST(ValueTest, NullComparisons) {
    Value null_val;
    Value int_val(0); // Даже 0 должен быть больше NULL

    EXPECT_TRUE(null_val < int_val);
    EXPECT_FALSE(int_val < null_val);
    EXPECT_TRUE(null_val == Value::Null());
}

// Тест 5: Обработка ошибок несоответствия типов (Type Mismatch)
TEST(ValueTest, TypeMismatchExceptions) {
    Value val_int(42);
    Value val_str("42");

    // Запрещено извлекать не тот тип
    EXPECT_THROW(val_int.get_string(), TypeError);
    EXPECT_THROW(val_str.get_int(), TypeError);

    // Запрещено сравнивать разные типы (должен выбрасываться TypeError)
    EXPECT_THROW(void(val_int == val_str), TypeError);
    EXPECT_THROW(void(val_int < val_str), TypeError);
}