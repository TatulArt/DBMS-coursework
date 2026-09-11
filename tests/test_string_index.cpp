#include <gtest/gtest.h>
#include "test_db_path.h"
#include <cstdio>
#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../src/index/table_indexer.h"

// ============================================================================
// Индексация STRING-колонок: ключ StringKey, лексикографический порядок
// ============================================================================

// ----------------------------------------------------------------------------
// Сам тип ключа
// ----------------------------------------------------------------------------
TEST(StringKeyTest, ComparisonMatchesStdString) {
    const std::vector<std::string> samples = {
        "", "a", "ab", "abc", "b", "A", "Z", "aa", "aab",
        "user_1", "user_10", "user_2", "яблоко", "ягода", "Ёлка"
    };

    for (const std::string& lhs : samples) {
        for (const std::string& rhs : samples) {
            auto left = StringKey::from_string(lhs);
            auto right = StringKey::from_string(rhs);
            ASSERT_TRUE(left.ok() && right.ok());

            // Порядок ключей обязан совпадать с порядком std::string,
            // иначе диапазонные запросы по строкам вернут не то
            EXPECT_EQ(left.value() < right.value(), lhs < rhs)
                << "'" << lhs << "' vs '" << rhs << "'";
            EXPECT_EQ(left.value() == right.value(), lhs == rhs)
                << "'" << lhs << "' vs '" << rhs << "'";
        }
    }
}

TEST(StringKeyTest, RoundTripAndLengthLimit) {
    const std::string value = "какая-то строка";
    auto key = StringKey::from_string(value);
    ASSERT_TRUE(key.ok());
    EXPECT_EQ(key.value().to_std_string(), value);

    // Строка ровно по границе допустима
    auto max_key = StringKey::from_string(std::string(StringKey::MAX_LENGTH, 'x'));
    EXPECT_TRUE(max_key.ok());

    // Более длинная — отклоняется явной ошибкой, а не молча обрезается:
    // обрезание сделало бы разные значения одинаковыми ключами
    // и сломало бы уникальность INDEXED-колонки
    auto too_long = StringKey::from_string(std::string(StringKey::MAX_LENGTH + 1, 'x'));
    EXPECT_FALSE(too_long.ok());
    EXPECT_EQ(too_long.status().code, StatusCode::InvalidArgument);
}

// ----------------------------------------------------------------------------
// B+ дерево со строковыми ключами
// ----------------------------------------------------------------------------
class StringBPlusTreeTest : public ::testing::Test {
protected:
    std::string db_file;
    void SetUp() override {
        db_file = unique_db_file("test_string_btree");
        std::remove(db_file.c_str());
    }
    void TearDown() override { std::remove(db_file.c_str()); }

    static StringKey key(const std::string& s) {
        auto res = StringKey::from_string(s);
        EXPECT_TRUE(res.ok());
        return res.value();
    }
};

TEST_F(StringBPlusTreeTest, InsertSearchAndOrder) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    StringBPlusTree tree(pm);

    // Достаточно значений, чтобы дерево стало многоуровневым
    // (в лист помещается заметно меньше строковых ключей, чем целых)
    std::vector<std::string> values;
    for (int i = 0; i < 3000; ++i) {
        values.push_back("user_" + std::to_string(i));
    }
    std::mt19937 g(31337);
    std::shuffle(values.begin(), values.end(), g);

    for (size_t i = 0; i < values.size(); ++i) {
        ASSERT_TRUE(tree.insert(key(values[i]), RecordId{static_cast<PageId>(i), 0}).ok())
            << "Не вставлено значение " << values[i];
    }

    Status valid = tree.validate();
    ASSERT_TRUE(valid.ok()) << valid.message;

    for (const std::string& value : values) {
        EXPECT_TRUE(tree.search(key(value)).ok()) << "Не найдено значение " << value;
    }

    // Обход должен идти в лексикографическом порядке
    std::vector<std::string> scanned;
    const StringIndexIterator stop = tree.end();
    for (auto it = tree.begin(); it != stop; ++it) {
        scanned.push_back((*it).first.to_std_string());
    }

    std::vector<std::string> expected = values;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(scanned, expected);

    pm.close();
}

TEST_F(StringBPlusTreeTest, UniqueKeysAndRangeScan) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    StringBPlusTree tree(pm);

    const std::vector<std::string> fruits = {
        "апельсин", "банан", "вишня", "груша", "дыня", "ежевика", "яблоко"
    };
    for (size_t i = 0; i < fruits.size(); ++i) {
        ASSERT_TRUE(tree.insert(key(fruits[i]), RecordId{static_cast<PageId>(i), 0}).ok());
    }

    // Повтор значения запрещён (семантика INDEXED)
    EXPECT_EQ(tree.insert(key("вишня"), RecordId{99, 0}).code,
              StatusCode::UniqueConstraintViolation);

    // Диапазон по строкам — включительно с обеих сторон
    std::vector<RecordId> range;
    ASSERT_TRUE(tree.scan_range(key("банан"), key("груша"), range).ok());
    EXPECT_EQ(range.size(), 3u); // банан, вишня, груша

    // Полуоткрытый диапазон — семантика BETWEEN
    ASSERT_TRUE(tree.scan_range_half_open(key("банан"), key("груша"), range).ok());
    EXPECT_EQ(range.size(), 2u); // банан, вишня

    // Префиксный поиск: всё, что начинается на "в"
    ASSERT_TRUE(tree.scan_range_half_open(key("в"), key("г"), range).ok());
    EXPECT_EQ(range.size(), 1u); // вишня

    pm.close();
}

TEST_F(StringBPlusTreeTest, RemoveWithMergeKeepsLeafChain) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    StringBPlusTree tree(pm);

    std::map<std::string, RecordId> reference;
    for (int i = 0; i < 2000; ++i) {
        const std::string value = "key_" + std::to_string(100000 + i);
        const RecordId rid{static_cast<PageId>(i), 0};
        ASSERT_TRUE(tree.insert(key(value), rid).ok());
        reference[value] = rid;
    }

    // Удаляем каждое второе значение — это вызывает слияния узлов
    int step = 0;
    for (auto it = reference.begin(); it != reference.end();) {
        if (step++ % 2 == 0) {
            ASSERT_TRUE(tree.remove(key(it->first)).ok()) << "Не удалено " << it->first;
            it = reference.erase(it);
        } else {
            ++it;
        }
    }

    Status valid = tree.validate();
    ASSERT_TRUE(valid.ok()) << valid.message;

    // Связный список листьев не должен потерять записи
    std::vector<std::string> scanned;
    const StringIndexIterator stop = tree.end();
    for (auto it = tree.begin(); it != stop; ++it) {
        scanned.push_back((*it).first.to_std_string());
    }

    std::vector<std::string> expected;
    for (const auto& kv : reference) expected.push_back(kv.first);

    ASSERT_EQ(scanned.size(), expected.size());
    EXPECT_EQ(scanned, expected);

    pm.close();
}

// ----------------------------------------------------------------------------
// Сквозной сценарий: INDEXED STRING-колонка таблицы
// ----------------------------------------------------------------------------
class StringTableIndexTest : public ::testing::Test {
protected:
    std::string db_file;

    std::unique_ptr<PageManager> pm_;
    std::unique_ptr<RecordManager> rm_;
    std::unique_ptr<IndexManager> im_;
    std::unique_ptr<TableIndexer> ti_;
    TableSchema accounts_;

    void SetUp() override {
        db_file = unique_db_file("test_string_table");
        std::remove(db_file.c_str());
        pm_ = std::make_unique<PageManager>(db_file);
        ASSERT_TRUE(pm_->open().ok());
        rm_ = std::make_unique<RecordManager>(*pm_);
        im_ = std::make_unique<IndexManager>(*pm_);
        ASSERT_TRUE(im_->open().ok());
        ti_ = std::make_unique<TableIndexer>(*pm_, *rm_, *im_);

        // login STRING INDEXED — уникален и не может быть NULL
        accounts_.table_name = "accounts";
        accounts_.columns = {
            {"login", ColumnType::String, false, true},
            {"email", ColumnType::String, false, false},
            {"karma", ColumnType::Int,    true,  false}
        };
    }

    void TearDown() override {
        ti_.reset(); im_.reset(); rm_.reset();
        if (pm_) pm_->close();
        pm_.reset();
        std::remove(db_file.c_str());
    }
};

TEST_F(StringTableIndexTest, IndexedStringColumnEndToEnd) {
    ASSERT_TRUE(ti_->create_indexes_for_table(accounts_).ok());

    auto info = im_->get_index_info(TableIndexer::index_name_for("accounts", "login"));
    ASSERT_TRUE(info.ok());
    EXPECT_EQ(info.value().key_type, ColumnType::String);

    for (int i = 0; i < 1200; ++i) {
        const std::string login = "user_" + std::to_string(1000 + i);
        auto res = ti_->insert_row(accounts_, {Value(login), Value(login + "@mail.ru"), Value(i)});
        ASSERT_TRUE(res.ok()) << login << ": " << res.status().message;
    }

    // Точечный поиск идёт через индекс
    auto found = ti_->select_equal(accounts_, "login", Value(std::string("user_1500")));
    ASSERT_TRUE(found.ok());
    EXPECT_EQ(found.value().method, AccessMethod::IndexLookup);
    ASSERT_EQ(found.value().records.size(), 1u);
    EXPECT_EQ(found.value().records[0].fields[1].get_string(), "user_1500@mail.ru");

    // Несуществующее значение
    auto missing = ti_->select_equal(accounts_, "login", Value(std::string("нет_такого")));
    ASSERT_TRUE(missing.ok());
    EXPECT_TRUE(missing.value().records.empty());

    // По неиндексированной строковой колонке — полный скан
    auto by_email = ti_->select_equal(accounts_, "email", Value(std::string("user_1500@mail.ru")));
    ASSERT_TRUE(by_email.ok());
    EXPECT_EQ(by_email.value().method, AccessMethod::FullScan);
    ASSERT_EQ(by_email.value().records.size(), 1u);

    // Индексный поиск читает меньше страниц, чем полный скан
    EXPECT_LT(found.value().pages_examined, by_email.value().pages_examined);
}

TEST_F(StringTableIndexTest, StringRangeIsLexicographic) {
    ASSERT_TRUE(ti_->create_indexes_for_table(accounts_).ok());

    const std::vector<std::string> logins = {
        "anna", "anton", "boris", "dmitry", "egor", "ivan", "zoya"
    };
    for (const std::string& login : logins) {
        ASSERT_TRUE(ti_->insert_row(accounts_, {Value(login), Value(login + "@x.ru"), Value(1)}).ok());
    }

    // Диапазон [anna, dmitry) — семантика BETWEEN из задания
    auto range = ti_->select_range(accounts_, "login",
                                   Value(std::string("anna")), Value(std::string("dmitry")), false);
    ASSERT_TRUE(range.ok());
    EXPECT_EQ(range.value().method, AccessMethod::IndexRange);

    std::vector<std::string> got;
    for (const Record& r : range.value().records) got.push_back(r.fields[0].get_string());
    EXPECT_EQ(got, (std::vector<std::string>{"anna", "anton", "boris"}));

    // Результат должен быть отсортирован лексикографически
    EXPECT_TRUE(std::is_sorted(got.begin(), got.end()));

    // Префиксная выборка "все логины на an"
    auto prefix = ti_->select_range(accounts_, "login",
                                    Value(std::string("an")), Value(std::string("ao")), false);
    ASSERT_TRUE(prefix.ok());
    EXPECT_EQ(prefix.value().records.size(), 2u); // anna, anton
}

TEST_F(StringTableIndexTest, ConstraintsOnIndexedStringColumn) {
    ASSERT_TRUE(ti_->create_indexes_for_table(accounts_).ok());

    ASSERT_TRUE(ti_->insert_row(accounts_, {Value("ivan"), Value("ivan@x.ru"), Value(10)}).ok());

    // Дубликат логина
    auto dup = ti_->insert_row(accounts_, {Value("ivan"), Value("other@x.ru"), Value(20)});
    EXPECT_FALSE(dup.ok());
    EXPECT_EQ(dup.status().code, StatusCode::UniqueConstraintViolation);

    // NULL в INDEXED-колонке
    auto null_login = ti_->insert_row(accounts_, {Value::Null(), Value("a@x.ru"), Value(1)});
    EXPECT_FALSE(null_login.ok());
    EXPECT_EQ(null_login.status().code, StatusCode::NullConstraintViolation);

    // Число вместо строки
    auto wrong_type = ti_->insert_row(accounts_, {Value(42), Value("b@x.ru"), Value(1)});
    EXPECT_FALSE(wrong_type.ok());
    EXPECT_EQ(wrong_type.status().code, StatusCode::TypeMismatch);

    // Слишком длинное значение для строкового ключа отклоняется,
    // а запись при этом не должна остаться в таблице
    const std::string huge(StringKey::MAX_LENGTH + 10, 'x');
    auto too_long = ti_->insert_row(accounts_, {Value(huge), Value("c@x.ru"), Value(1)});
    EXPECT_FALSE(too_long.ok());

    auto all = ti_->full_scan(accounts_);
    ASSERT_TRUE(all.ok());
    EXPECT_EQ(all.value().records.size(), 1u);
}

TEST_F(StringTableIndexTest, StringIndexSurvivesRestart) {
    ASSERT_TRUE(ti_->create_indexes_for_table(accounts_).ok());
    for (int i = 0; i < 800; ++i) {
        const std::string login = "acc_" + std::to_string(10000 + i);
        ASSERT_TRUE(ti_->insert_row(accounts_, {Value(login), Value(login + "@x.ru"), Value(i)}).ok());
    }
    const std::vector<PageId> data_pages = accounts_.data_pages;

    ti_.reset(); im_.reset(); rm_.reset();
    pm_->close();
    pm_.reset();

    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    RecordManager rm(pm);
    IndexManager im(pm);
    ASSERT_TRUE(im.open().ok());
    TableIndexer ti(pm, rm, im);

    TableSchema schema = accounts_;
    schema.data_pages = data_pages;

    // Тип ключа должен восстановиться из каталога
    auto info = im.get_index_info(TableIndexer::index_name_for("accounts", "login"));
    ASSERT_TRUE(info.ok());
    EXPECT_EQ(info.value().key_type, ColumnType::String);

    for (int i = 0; i < 800; i += 37) {
        const std::string login = "acc_" + std::to_string(10000 + i);
        auto res = ti.select_equal(schema, "login", Value(login));
        ASSERT_TRUE(res.ok());
        EXPECT_EQ(res.value().method, AccessMethod::IndexLookup);
        ASSERT_EQ(res.value().records.size(), 1u) << login << " потерян после перезапуска";
    }

    auto tree = im.get_string_index(TableIndexer::index_name_for("accounts", "login"));
    ASSERT_TRUE(tree.ok());
    EXPECT_TRUE(tree.value().validate().ok()) << tree.value().validate().message;

    pm.close();
    pm_ = std::make_unique<PageManager>(db_file);
}

// ----------------------------------------------------------------------------
// Смешанный случай: в одной таблице индексы и по INT, и по STRING
// ----------------------------------------------------------------------------
TEST_F(StringTableIndexTest, IntAndStringIndexesCoexist) {
    TableSchema mixed;
    mixed.table_name = "mixed";
    mixed.columns = {
        {"id",    ColumnType::Int,    false, true},
        {"login", ColumnType::String, false, true},
        {"note",  ColumnType::String, true,  false}
    };

    ASSERT_TRUE(ti_->create_indexes_for_table(mixed).ok());
    EXPECT_EQ(im_->indexes_for_table("mixed").size(), 2u);

    for (int i = 1; i <= 700; ++i) {
        ASSERT_TRUE(ti_->insert_row(mixed, {Value(i), Value("login_" + std::to_string(i)), Value::Null()}).ok())
            << "строка " << i;
    }

    // Оба индекса работают независимо и оба через дерево
    auto by_id = ti_->select_equal(mixed, "id", Value(555));
    ASSERT_TRUE(by_id.ok());
    EXPECT_EQ(by_id.value().method, AccessMethod::IndexLookup);
    ASSERT_EQ(by_id.value().records.size(), 1u);
    EXPECT_EQ(by_id.value().records[0].fields[1].get_string(), "login_555");

    auto by_login = ti_->select_equal(mixed, "login", Value(std::string("login_555")));
    ASSERT_TRUE(by_login.ok());
    EXPECT_EQ(by_login.value().method, AccessMethod::IndexLookup);
    ASSERT_EQ(by_login.value().records.size(), 1u);
    EXPECT_EQ(by_login.value().records[0].fields[0].get_int(), 555);

    // Удаление строки чистит оба индекса
    ASSERT_TRUE(ti_->delete_row(mixed, by_id.value().records[0].id).ok());
    EXPECT_TRUE(ti_->select_equal(mixed, "id", Value(555)).value().records.empty());
    EXPECT_TRUE(ti_->select_equal(mixed, "login", Value(std::string("login_555"))).value().records.empty());
}
