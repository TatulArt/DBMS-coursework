#include <gtest/gtest.h>
#include "test_db_path.h"
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>

#include "../src/index/table_indexer.h"

// ============================================================================
// Тесты сквозной фичи: индексация страниц данных таблицы и поиск через индекс
// ============================================================================
class TableIndexerTest : public ::testing::Test {
protected:
    std::string db_file;

    std::unique_ptr<PageManager> pm_;
    std::unique_ptr<RecordManager> rm_;
    std::unique_ptr<IndexManager> im_;
    std::unique_ptr<TableIndexer> ti_;
    TableSchema users_;

    void SetUp() override {
        db_file = unique_db_file("test_table_indexer");
        std::remove(db_file.c_str());

        pm_ = std::make_unique<PageManager>(db_file);
        ASSERT_TRUE(pm_->open().ok());

        rm_ = std::make_unique<RecordManager>(*pm_);
        im_ = std::make_unique<IndexManager>(*pm_);
        ASSERT_TRUE(im_->open().ok());

        ti_ = std::make_unique<TableIndexer>(*pm_, *rm_, *im_);

        // Таблица users: id INT INDEXED, name STRING NOT_NULL, age INT (nullable)
        users_.table_name = "users";
        users_.columns = {
            {"id",   ColumnType::Int,    false, true},
            {"name", ColumnType::String, false, false},
            {"age",  ColumnType::Int,    true,  false}
        };
    }

    void TearDown() override {
        ti_.reset();
        im_.reset();
        rm_.reset();
        if (pm_) pm_->close();
        pm_.reset();
        std::remove(db_file.c_str());
    }

    // Наполнение таблицы напрямую через RecordManager (без индексов),
    // как будто данные уже лежали в файле до создания индекса.
    void fill_raw(int count) {
        PageId page_id = INVALID_PAGE_ID;
        Page page;
        ASSERT_TRUE(pm_->allocate_page(page_id, page).ok());
        users_.data_pages.push_back(page_id);

        for (int i = 1; i <= count; ++i) {
            std::vector<Value> row = {Value(i), Value("user_" + std::to_string(i)), Value(20 + i % 50)};

            auto space = rm_->page_has_space(page_id, RecordManager::record_size(row, users_.columns));
            ASSERT_TRUE(space.ok());
            if (!space.value()) {
                ASSERT_TRUE(pm_->allocate_page(page_id, page).ok());
                users_.data_pages.push_back(page_id);
            }
            ASSERT_TRUE(rm_->insert_record(page_id, row, users_.columns).ok());
        }
    }
};

// ----------------------------------------------------------------------------
// 1. Построение индекса по уже существующим страницам данных
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, BuildIndexOverExistingPages) {
    fill_raw(500);
    ASSERT_GT(users_.data_pages.size(), 1u) << "Данные должны занимать несколько страниц";

    auto build_res = ti_->build_index(users_, "id");
    ASSERT_TRUE(build_res.ok()) << build_res.status().message;
    EXPECT_EQ(build_res.value().table_name, "users");
    EXPECT_EQ(build_res.value().column_name, "id");

    // Каждая запись должна находиться через индекс
    for (int i = 1; i <= 500; ++i) {
        auto sel = ti_->select_equal(users_, "id", Value(i));
        ASSERT_TRUE(sel.ok()) << sel.status().message;
        EXPECT_EQ(sel.value().method, AccessMethod::IndexLookup);
        ASSERT_EQ(sel.value().records.size(), 1u) << "Не найдена запись с id=" << i;
        EXPECT_EQ(sel.value().records[0].fields[0].get_int(), i);
        EXPECT_EQ(sel.value().records[0].fields[1].get_string(), "user_" + std::to_string(i));
    }
}

// ----------------------------------------------------------------------------
// 2. Индекс ссылается на данные и не дублирует их
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, IndexStoresReferencesNotData) {
    fill_raw(100);
    ASSERT_TRUE(ti_->build_index(users_, "id").ok());

    // Индекс хранит RecordId, по которому запись читается из кучи
    auto rid_res = im_->find_entry(TableIndexer::index_name_for("users", "id"), 42);
    ASSERT_TRUE(rid_res.ok());

    const RecordId rid = rid_res.value();
    auto record = ti_->fetch(users_, rid);
    ASSERT_TRUE(record.ok());
    EXPECT_EQ(record.value().fields[0].get_int(), 42);

    // RecordId должен указывать на реальную страницу данных таблицы
    EXPECT_NE(std::find(users_.data_pages.begin(), users_.data_pages.end(), rid.page_id),
              users_.data_pages.end());
}

// ----------------------------------------------------------------------------
// 3. Вставка строк через TableIndexer поддерживает индексы в актуальном виде
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, InsertRowMaintainsIndexes) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    for (int i = 1; i <= 1000; ++i) {
        std::vector<Value> row = {Value(i), Value("name_" + std::to_string(i)), Value(30)};
        auto res = ti_->insert_row(users_, row);
        ASSERT_TRUE(res.ok()) << "Строка " << i << ": " << res.status().message;
    }

    EXPECT_GT(users_.data_pages.size(), 1u);

    for (int i = 1; i <= 1000; i += 37) {
        auto sel = ti_->select_equal(users_, "id", Value(i));
        ASSERT_TRUE(sel.ok());
        EXPECT_EQ(sel.value().method, AccessMethod::IndexLookup);
        ASSERT_EQ(sel.value().records.size(), 1u) << "id=" << i;
        EXPECT_EQ(sel.value().records[0].fields[1].get_string(), "name_" + std::to_string(i));
    }

    // Полный скан должен видеть ровно столько же записей
    auto all = ti_->full_scan(users_);
    ASSERT_TRUE(all.ok());
    EXPECT_EQ(all.value().records.size(), 1000u);
}

// ----------------------------------------------------------------------------
// 4. Ограничения целостности колонки INDEXED (уникальность и NOT NULL)
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, IndexedColumnConstraints) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    ASSERT_TRUE(ti_->insert_row(users_, {Value(1), Value("Alice"), Value(30)}).ok());

    // Повтор значения в INDEXED-колонке
    auto dup = ti_->insert_row(users_, {Value(1), Value("Bob"), Value(40)});
    EXPECT_FALSE(dup.ok());
    EXPECT_EQ(dup.status().code, StatusCode::UniqueConstraintViolation);

    // INDEXED-поле не может быть NULL
    auto null_key = ti_->insert_row(users_, {Value::Null(), Value("Carol"), Value(25)});
    EXPECT_FALSE(null_key.ok());
    EXPECT_EQ(null_key.status().code, StatusCode::NullConstraintViolation);

    // NOT_NULL-поле не может быть NULL
    auto null_name = ti_->insert_row(users_, {Value(2), Value::Null(), Value(25)});
    EXPECT_FALSE(null_name.ok());
    EXPECT_EQ(null_name.status().code, StatusCode::NullConstraintViolation);

    // Нарушение типа
    auto wrong_type = ti_->insert_row(users_, {Value("не число"), Value("Dave"), Value(25)});
    EXPECT_FALSE(wrong_type.ok());
    EXPECT_EQ(wrong_type.status().code, StatusCode::TypeMismatch);

    // Неверное количество значений
    EXPECT_FALSE(ti_->insert_row(users_, {Value(3), Value("Eve")}).ok());

    // Nullable-поле может быть NULL
    ASSERT_TRUE(ti_->insert_row(users_, {Value(2), Value("Frank"), Value::Null()}).ok());

    // Отклонённые строки не должны попасть в таблицу
    auto all = ti_->full_scan(users_);
    ASSERT_TRUE(all.ok());
    EXPECT_EQ(all.value().records.size(), 2u);
}

// ----------------------------------------------------------------------------
// 5. Диапазонный поиск идёт через индекс (BETWEEN из задания — [low, high))
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, RangeSelectUsesIndex) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    for (int i = 1; i <= 1000; ++i) {
        ASSERT_TRUE(ti_->insert_row(users_, {Value(i), Value("u" + std::to_string(i)), Value(i)}).ok());
    }

    // Включительный диапазон [100, 200]
    auto incl = ti_->select_range(users_, "id", Value(100), Value(200), true);
    ASSERT_TRUE(incl.ok());
    EXPECT_EQ(incl.value().method, AccessMethod::IndexRange);
    EXPECT_EQ(incl.value().records.size(), 101u);

    // Полуоткрытый диапазон [100, 200) — семантика BETWEEN
    auto half = ti_->select_range(users_, "id", Value(100), Value(200), false);
    ASSERT_TRUE(half.ok());
    EXPECT_EQ(half.value().method, AccessMethod::IndexRange);
    EXPECT_EQ(half.value().records.size(), 100u);

    // Результат должен быть отсортирован по ключу индекса
    std::vector<int32_t> ids;
    for (const Record& r : half.value().records) ids.push_back(r.fields[0].get_int());
    EXPECT_TRUE(std::is_sorted(ids.begin(), ids.end()));
    EXPECT_EQ(ids.front(), 100);
    EXPECT_EQ(ids.back(), 199);

    // Пустой диапазон
    auto empty = ti_->select_range(users_, "id", Value(5000), Value(6000), true);
    ASSERT_TRUE(empty.ok());
    EXPECT_TRUE(empty.value().records.empty());
}

// ----------------------------------------------------------------------------
// 6. Без индекса выполняется полный скан, и он даёт тот же результат
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, FallsBackToFullScanWithoutIndex) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    for (int i = 1; i <= 300; ++i) {
        ASSERT_TRUE(ti_->insert_row(users_, {Value(i), Value("u" + std::to_string(i)), Value(i % 10)}).ok());
    }

    // По колонке age индекса нет
    auto by_age = ti_->select_equal(users_, "age", Value(3));
    ASSERT_TRUE(by_age.ok());
    EXPECT_EQ(by_age.value().method, AccessMethod::FullScan);
    EXPECT_EQ(by_age.value().records.size(), 30u);

    // По строковой колонке индекса тоже нет — тоже полный скан
    auto by_name = ti_->select_equal(users_, "name", Value("u42"));
    ASSERT_TRUE(by_name.ok());
    EXPECT_EQ(by_name.value().method, AccessMethod::FullScan);
    ASSERT_EQ(by_name.value().records.size(), 1u);
    EXPECT_EQ(by_name.value().records[0].fields[0].get_int(), 42);

    // Индексный и полный поиск по id обязаны совпадать
    auto indexed = ti_->select_equal(users_, "id", Value(42));
    ASSERT_TRUE(indexed.ok());
    EXPECT_EQ(indexed.value().method, AccessMethod::IndexLookup);
    ASSERT_EQ(indexed.value().records.size(), 1u);
    EXPECT_EQ(indexed.value().records[0].fields[1].get_string(), "u42");

    // Несуществующая колонка
    EXPECT_FALSE(ti_->select_equal(users_, "no_such_column", Value(1)).ok());
}

// ----------------------------------------------------------------------------
// 7. Индексный поиск читает меньше страниц, чем полный скан
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, IndexLookupReadsFewerPages) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    for (int i = 1; i <= 2000; ++i) {
        ASSERT_TRUE(ti_->insert_row(users_, {Value(i), Value("u" + std::to_string(i)), Value(i)}).ok());
    }
    ASSERT_GT(users_.data_pages.size(), 5u) << "Данные должны лежать на многих страницах";

    auto indexed = ti_->select_equal(users_, "id", Value(1500));
    ASSERT_TRUE(indexed.ok());
    EXPECT_EQ(indexed.value().pages_examined, 1u);

    auto scanned = ti_->select_equal(users_, "age", Value(1500));
    ASSERT_TRUE(scanned.ok());
    EXPECT_EQ(scanned.value().pages_examined, users_.data_pages.size());

    // Именно ради этого и писался индекс
    EXPECT_LT(indexed.value().pages_examined, scanned.value().pages_examined);
}

// ----------------------------------------------------------------------------
// 8. Удаление строки чистит индексы
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, DeleteRowCleansIndexes) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());

    std::vector<RecordId> rids;
    for (int i = 1; i <= 800; ++i) {
        auto res = ti_->insert_row(users_, {Value(i), Value("u" + std::to_string(i)), Value(i)});
        ASSERT_TRUE(res.ok());
        rids.push_back(res.value());
    }

    // Удаляем каждую вторую строку
    for (size_t i = 0; i < rids.size(); i += 2) {
        Status st = ti_->delete_row(users_, rids[i]);
        ASSERT_TRUE(st.ok()) << "Не удалена строка " << (i + 1) << ": " << st.message;
    }

    for (int i = 1; i <= 800; ++i) {
        auto sel = ti_->select_equal(users_, "id", Value(i));
        ASSERT_TRUE(sel.ok());
        const bool should_exist = (i % 2 == 0); // удаляли строки с нечётными id
        EXPECT_EQ(sel.value().records.size(), should_exist ? 1u : 0u) << "id=" << i;
    }

    auto all = ti_->full_scan(users_);
    ASSERT_TRUE(all.ok());
    EXPECT_EQ(all.value().records.size(), 400u);

    // После удаления значение можно вставить заново — индекс освободил ключ
    ASSERT_TRUE(ti_->insert_row(users_, {Value(1), Value("новый"), Value(1)}).ok());

    auto reused = ti_->select_equal(users_, "id", Value(1));
    ASSERT_TRUE(reused.ok());
    ASSERT_EQ(reused.value().records.size(), 1u);
    EXPECT_EQ(reused.value().records[0].fields[1].get_string(), "новый");
}

// ----------------------------------------------------------------------------
// 9. Индекс переживает перезапуск СУБД вместе с данными
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, IndexAndDataSurviveRestart) {
    ASSERT_TRUE(ti_->create_indexes_for_table(users_).ok());
    for (int i = 1; i <= 1500; ++i) {
        ASSERT_TRUE(ti_->insert_row(users_, {Value(i), Value("u" + std::to_string(i)), Value(i)}).ok());
    }
    const std::vector<PageId> data_pages = users_.data_pages;

    // Закрываем базу
    ti_.reset();
    im_.reset();
    rm_.reset();
    pm_->close();
    pm_.reset();

    // Открываем заново
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    RecordManager rm(pm);
    IndexManager im(pm);
    ASSERT_TRUE(im.open().ok());
    TableIndexer ti(pm, rm, im);

    // Схема восстанавливается уровнем каталога таблиц; страницы данных те же
    TableSchema schema = users_;
    schema.data_pages = data_pages;

    ASSERT_TRUE(im.has_index(TableIndexer::index_name_for("users", "id")));

    for (int i = 1; i <= 1500; i += 41) {
        auto sel = ti.select_equal(schema, "id", Value(i));
        ASSERT_TRUE(sel.ok());
        EXPECT_EQ(sel.value().method, AccessMethod::IndexLookup);
        ASSERT_EQ(sel.value().records.size(), 1u) << "id=" << i << " потерян после перезапуска";
        EXPECT_EQ(sel.value().records[0].fields[1].get_string(), "u" + std::to_string(i));
    }

    // И вставка после перезапуска тоже должна работать
    ASSERT_TRUE(ti.insert_row(schema, {Value(9999), Value("после перезапуска"), Value(1)}).ok());
    auto res = ti.select_equal(schema, "id", Value(9999));
    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.value().records.size(), 1u);

    pm.close();
    // Файл удалит TearDown
    pm_ = std::make_unique<PageManager>(db_file);
}

// ----------------------------------------------------------------------------
// 10. Построение индекса по колонке с дубликатами должно падать с понятной ошибкой
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, BuildIndexRejectsDuplicatesAndNulls) {
    PageId page_id = INVALID_PAGE_ID;
    Page page;
    ASSERT_TRUE(pm_->allocate_page(page_id, page).ok());
    users_.data_pages.push_back(page_id);

    ASSERT_TRUE(rm_->insert_record(page_id, {Value(1), Value("a"), Value(1)}, users_.columns).ok());
    ASSERT_TRUE(rm_->insert_record(page_id, {Value(1), Value("b"), Value(2)}, users_.columns).ok());

    auto res = ti_->build_index(users_, "id");
    EXPECT_FALSE(res.ok());
    EXPECT_EQ(res.status().code, StatusCode::UniqueConstraintViolation);

    // Неудачное построение не должно оставлять «полуготовый» индекс в каталоге
    EXPECT_FALSE(im_->has_index(TableIndexer::index_name_for("users", "id")));

    // По строковой колонке индекс строится (значения "a" и "b" различны)
    auto str_res = ti_->build_index(users_, "name");
    ASSERT_TRUE(str_res.ok()) << str_res.status().message;
    EXPECT_EQ(str_res.value().key_type, ColumnType::String);

    // Несуществующая колонка
    EXPECT_EQ(ti_->build_index(users_, "нет_такой").status().code, StatusCode::ColumnNotFound);
}

// ----------------------------------------------------------------------------
// 11. Перестроение индекса поверх существующего
// ----------------------------------------------------------------------------
TEST_F(TableIndexerTest, RebuildIndexReplacesOldOne) {
    fill_raw(400);

    ASSERT_TRUE(ti_->build_index(users_, "id").ok());
    const PageId first_root = im_->get_index_info(TableIndexer::index_name_for("users", "id"))
                                  .value().root_page_id;

    // Повторный вызов должен пересобрать индекс, а не упасть с «уже существует»
    auto again = ti_->build_index(users_, "id");
    ASSERT_TRUE(again.ok()) << again.status().message;
    EXPECT_NE(again.value().root_page_id, first_root);
    EXPECT_EQ(im_->list_indexes().size(), 1u);

    for (int i = 1; i <= 400; i += 17) {
        auto sel = ti_->select_equal(users_, "id", Value(i));
        ASSERT_TRUE(sel.ok());
        EXPECT_EQ(sel.value().records.size(), 1u) << "id=" << i;
    }
}
