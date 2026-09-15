#include <gtest/gtest.h>
#include "test_db_path.h"
#include <cstdio>
#include "../src/storage/page_manager.h"
#include "../src/storage/record_manager.h"
#include "../src/storage/serializer.h"

class StoragePipelineTest : public ::testing::Test {
protected:
    std::string test_db_file;
    std::vector<ColumnDef> schema;

    void SetUp() override {
        test_db_file = unique_db_file("test_pipeline");
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
// ============================================================================
// ТЕСТ: место удалённых записей возвращается в оборот.
//
// Раньше delete_record только помечал слот как пустой, и освободившиеся байты
// на странице больше никогда не использовались. Теперь insert_record при
// нехватке непрерывного места уплотняет страницу.
// ============================================================================
TEST_F(StoragePipelineTest, DeletedSpaceIsReusedAfterCompaction) {
    PageManager page_mgr(test_db_file);
    ASSERT_TRUE(page_mgr.open().ok());
    RecordManager record_mgr(page_mgr);

    PageId page_id = INVALID_PAGE_ID;
    Page page;
    ASSERT_TRUE(page_mgr.allocate_page(page_id, page).ok());
    ASSERT_TRUE(record_mgr.init_page(page_id).ok());

    // Забиваем страницу длинными строками, пока она не кончится
    const std::string filler(200, 'x');
    std::vector<RecordId> inserted;
    while (true) {
        auto res = record_mgr.insert_record(page_id, {Value(1), Value(filler), Value::Null()}, schema);
        if (!res.ok()) break;
        inserted.push_back(res.value());
    }
    ASSERT_GT(inserted.size(), 2u) << "Страница должна вмещать несколько записей";

    // Освобождаем половину записей
    const size_t to_delete = inserted.size() / 2;
    for (size_t i = 0; i < to_delete; ++i) {
        ASSERT_TRUE(record_mgr.delete_record(inserted[i]).ok());
    }

    // На освободившееся место должны влезть новые записи того же размера
    size_t reinserted = 0;
    for (size_t i = 0; i < to_delete; ++i) {
        auto res = record_mgr.insert_record(page_id, {Value(2), Value(filler), Value::Null()}, schema);
        if (!res.ok()) break;
        ++reinserted;
    }

    EXPECT_GT(reinserted, 0u) << "Место удалённых записей так и не переиспользовано";

    // Уцелевшие записи должны остаться читаемыми по прежним RecordId:
    // уплотнение не имеет права менять номера слотов
    for (size_t i = to_delete; i < inserted.size(); ++i) {
        auto res = record_mgr.get_record(inserted[i], schema);
        ASSERT_TRUE(res.ok()) << "Запись потеряна после уплотнения страницы";
        EXPECT_EQ(res.value().fields[0].get_int(), 1);
        EXPECT_EQ(res.value().fields[1].get_string(), filler);
    }
}

// ============================================================================
// ТЕСТ: страницы кучи связываются в цепочку
// ============================================================================
TEST_F(StoragePipelineTest, HeapPagesFormAChain) {
    PageManager page_mgr(test_db_file);
    ASSERT_TRUE(page_mgr.open().ok());
    RecordManager record_mgr(page_mgr);

    PageId first = INVALID_PAGE_ID;
    PageId second = INVALID_PAGE_ID;
    Page page;
    ASSERT_TRUE(page_mgr.allocate_page(first, page).ok());
    ASSERT_TRUE(record_mgr.init_page(first).ok());
    ASSERT_TRUE(page_mgr.allocate_page(second, page).ok());
    ASSERT_TRUE(record_mgr.init_page(second).ok());

    // Свежая страница — конец цепочки
    auto next = record_mgr.next_page(first);
    ASSERT_TRUE(next.ok());
    EXPECT_EQ(next.value(), INVALID_PAGE_ID);

    ASSERT_TRUE(record_mgr.set_next_page(first, second).ok());

    next = record_mgr.next_page(first);
    ASSERT_TRUE(next.ok());
    EXPECT_EQ(next.value(), second);

    // Ссылка страницы на саму себя зациклила бы обход — она запрещена
    EXPECT_FALSE(record_mgr.set_next_page(first, first).ok());

    // Запись данных не должна рвать цепочку
    ASSERT_TRUE(record_mgr.insert_record(first, {Value(1), Value("a"), Value::Null()}, schema).ok());
    next = record_mgr.next_page(first);
    ASSERT_TRUE(next.ok());
    EXPECT_EQ(next.value(), second);
}

// ============================================================================
// ТЕСТ: освобождённые страницы переиспользуются, а не теряются
// ============================================================================
TEST_F(StoragePipelineTest, FreedPagesAreReused) {
    PageManager page_mgr(test_db_file);
    ASSERT_TRUE(page_mgr.create_database(test_db_file).ok());

    PageId a = INVALID_PAGE_ID;
    PageId b = INVALID_PAGE_ID;
    Page page;
    ASSERT_TRUE(page_mgr.allocate_page(a, page).ok());
    ASSERT_TRUE(page_mgr.allocate_page(b, page).ok());
    const uint32_t pages_before = page_mgr.get_num_pages();

    ASSERT_TRUE(page_mgr.free_page(a).ok());
    ASSERT_TRUE(page_mgr.free_page(b).ok());

    auto freed = page_mgr.free_page_count();
    ASSERT_TRUE(freed.ok());
    EXPECT_EQ(freed.value(), 2u);

    // Следующие выделения должны вернуть уже освобождённые страницы,
    // а файл — не вырасти
    PageId reused_1 = INVALID_PAGE_ID;
    PageId reused_2 = INVALID_PAGE_ID;
    ASSERT_TRUE(page_mgr.allocate_page(reused_1, page).ok());
    ASSERT_TRUE(page_mgr.allocate_page(reused_2, page).ok());

    EXPECT_EQ(page_mgr.get_num_pages(), pages_before) << "Файл вырос вместо переиспользования страниц";
    EXPECT_NE(reused_1, reused_2);
    EXPECT_TRUE(reused_1 == a || reused_1 == b);
    EXPECT_TRUE(reused_2 == a || reused_2 == b);

    freed = page_mgr.free_page_count();
    ASSERT_TRUE(freed.ok());
    EXPECT_EQ(freed.value(), 0u);

    // Нулевую страницу с заголовком базы освободить нельзя
    EXPECT_FALSE(page_mgr.free_page(METADATA_PAGE_ID).ok());

    // Повторное освобождение не должно закольцевать список
    ASSERT_TRUE(page_mgr.free_page(reused_1).ok());
    ASSERT_TRUE(page_mgr.free_page(reused_1).ok());
    freed = page_mgr.free_page_count();
    ASSERT_TRUE(freed.ok());
    EXPECT_EQ(freed.value(), 1u);
}
