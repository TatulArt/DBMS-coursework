#include <gtest/gtest.h>
#include <cstdio>
#include "../src/storage/page_manager.h"
#include "../src/storage/record_manager.h"
#include "../src/storage/serializer.h"

class StoragePipelineTest : public ::testing::Test {
protected:
    std::string test_db_file = "test_pipeline.db";
    std::vector<ColumnDef> schema;

    void SetUp() override {
        // Удаляем тестовый файл, если он остался от прошлых запусков
        std::remove(test_db_file.c_str());

        // Задаём тестовую схему таблицы: id (INT), name (STRING), score (INT, Nullable)
        schema = {
            {"id", ColumnType::Int, false, true},
            {"name", ColumnType::String, false, false},
            {"score", ColumnType::Int, true, false}
        };
    }

    void TearDown() override {
        // Очищаем файл после завершения теста
        std::remove(test_db_file.c_str());
    }
};

// ============================================================================
// ТЕСТ 1: Сквозная запись и чтение строк через весь storage-пайплайн
// ============================================================================
TEST_F(StoragePipelineTest, InsertAndReadRecordPipeline) {
    PageManager page_mgr(test_db_file);
    ASSERT_TRUE(page_mgr.open().ok());

    RecordManager record_mgr(page_mgr);

    // Выделяем первую страницу под таблицу
    PageId page_id;
    Page initial_page;
    ASSERT_TRUE(page_mgr.allocate_page(page_id, initial_page).ok());
    EXPECT_EQ(page_id, 0);

    // Подготавливаем тестовые данные
    std::vector<Value> row1 = { Value(1), Value("Alice"), Value(100) };
    std::vector<Value> row2 = { Value(2), Value("Bob"), Value::Null() }; // row2 с NULL-полем

    // 1. Вставка первой строки
    auto res1 = record_mgr.insert_record(page_id, row1, schema);
    ASSERT_TRUE(res1.ok());
    RecordId rid1 = res1.value();
    EXPECT_EQ(rid1.page_id, 0);
    EXPECT_EQ(rid1.slot_id, 0);

    // 2. Вставка второй строки
    auto res2 = record_mgr.insert_record(page_id, row2, schema);
    ASSERT_TRUE(res2.ok());
    RecordId rid2 = res2.value();
    EXPECT_EQ(rid2.page_id, 0);
    EXPECT_EQ(rid2.slot_id, 1);

    // 3. Вычитываем первую запись и проверяем точное совпадение полей
    auto rec1_res = record_mgr.get_record(rid1, schema);
    ASSERT_TRUE(rec1_res.ok());
    Record rec1 = rec1_res.value();
    EXPECT_EQ(rec1.fields[0].get_int(), 1);
    EXPECT_EQ(rec1.fields[1].get_string(), "Alice");
    EXPECT_EQ(rec1.fields[2].get_int(), 100);

    // 4. Вычитываем вторую запись и проверяем NULL
    auto rec2_res = record_mgr.get_record(rid2, schema);
    ASSERT_TRUE(rec2_res.ok());
    Record rec2 = rec2_res.value();
    EXPECT_EQ(rec2.fields[0].get_int(), 2);
    EXPECT_EQ(rec2.fields[1].get_string(), "Bob");
    EXPECT_TRUE(rec2.fields[2].is_null());

    page_mgr.close();
}

// ============================================================================
// ТЕСТ 2: Проверка персистентности (переоткрытие файла базы данных с диска)
// ============================================================================
TEST_F(StoragePipelineTest, DiskPersistence) {
    RecordId saved_rid;

    // Секция 1: Записываем данные и жестко закрываем файл
    {
        PageManager page_mgr(test_db_file);
        ASSERT_TRUE(page_mgr.open().ok());
        RecordManager record_mgr(page_mgr);

        PageId page_id;
        Page page;
        page_mgr.allocate_page(page_id, page);

        std::vector<Value> row = { Value(42), Value("Persistent Data"), Value(999) };
        auto insert_res = record_mgr.insert_record(page_id, row, schema);
        ASSERT_TRUE(insert_res.ok());
        saved_rid = insert_res.value();
        
        page_mgr.close(); // Имитируем завершение работы СУБД
    }

    // Секция 2: Открываем файл заново с диска и читаем запись
    {
        PageManager page_mgr(test_db_file);
        ASSERT_TRUE(page_mgr.open().ok());
        RecordManager record_mgr(page_mgr);

        auto read_res = record_mgr.get_record(saved_rid, schema);
        ASSERT_TRUE(read_res.ok());
        
        Record rec = read_res.value();
        EXPECT_EQ(rec.fields[0].get_int(), 42);
        EXPECT_EQ(rec.fields[1].get_string(), "Persistent Data");
        EXPECT_EQ(rec.fields[2].get_int(), 999);

        page_mgr.close();
    }
}

// ============================================================================
// ТЕСТ 3: Проверка логики удаления (Delete Record)
// ============================================================================
TEST_F(StoragePipelineTest, DeleteRecordLogic) {
    PageManager page_mgr(test_db_file);
    ASSERT_TRUE(page_mgr.open().ok());
    RecordManager record_mgr(page_mgr);

    PageId page_id;
    Page page;
    page_mgr.allocate_page(page_id, page);

    std::vector<Value> row = { Value(777), Value("To Be Deleted"), Value(0) };
    auto insert_res = record_mgr.insert_record(page_id, row, schema);
    ASSERT_TRUE(insert_res.ok());
    RecordId rid = insert_res.value();

    // Проверяем, что запись существует
    EXPECT_TRUE(record_mgr.get_record(rid, schema).ok());

    // Удаляем запись
    ASSERT_TRUE(record_mgr.delete_record(rid).ok());

    // Попытка прочитать удалённую запись должна вернуть ошибку RecordNotFound
    auto read_res = record_mgr.get_record(rid, schema);
    EXPECT_FALSE(read_res.ok());
    EXPECT_EQ(read_res.status().code, StatusCode::RecordNotFound);

    page_mgr.close();
}