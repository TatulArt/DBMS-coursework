#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "../src/index/index_manager.h"
#include "../src/storage/page_manager.h"
#include "../src/types.h"

class IndexManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временный файл БД для тестирования
        std::string db_filename = "test_index_mgr.db";
        std::remove(db_filename.c_str());

        page_manager_ = std::make_unique<PageManager>(db_filename);
        ASSERT_TRUE(page_manager_->open().ok());

        index_manager_ = std::make_unique<IndexManager>(*page_manager_);
    }

    void TearDown() override {
        index_manager_.reset();
        page_manager_->close();
        page_manager_.reset();
        std::remove("test_index_mgr.db");
    }

    std::unique_ptr<PageManager> page_manager_;
    std::unique_ptr<IndexManager> index_manager_;
};

// 1. Тест создания нового индекса
TEST_F(IndexManagerTest, CreateIndexSuccess) {
    auto res = index_manager_->create_index("idx_user_id", "users", "id");
    ASSERT_TRUE(res.ok());

    IndexInfo info = res.value();
    EXPECT_EQ(info.index_name, "idx_user_id");
    EXPECT_EQ(info.table_name, "users");
    EXPECT_EQ(info.column_name, "id");
    EXPECT_NE(info.root_page_id, INVALID_PAGE_ID);

    // Проверяем получение информации об индексе из каталога
    auto info_res = index_manager_->get_index_info("idx_user_id");
    ASSERT_TRUE(info_res.ok());
    EXPECT_EQ(info_res.value().root_page_id, info.root_page_id);
}

// 2. Тест попытки создания дубликата индекса
TEST_F(IndexManagerTest, CreateDuplicateIndexFails) {
    auto first_res = index_manager_->create_index("idx_users_id", "users", "id");
    ASSERT_TRUE(first_res.ok());

    // Повторное создание с тем же именем должно вернуть ошибку
    auto second_res = index_manager_->create_index("idx_users_id", "users", "id");
    ASSERT_FALSE(second_res.ok());
}

// 3. Тест получение BPlusTree по имени индекса
TEST_F(IndexManagerTest, GetIndexSuccessAndFailure) {
    ASSERT_TRUE(index_manager_->create_index("idx_orders", "orders", "id").ok());

    // Успешное получение
    auto tree_res = index_manager_->get_index("idx_orders");
    ASSERT_TRUE(tree_res.ok());

    // Запрос несуществующего индекса
    auto invalid_res = index_manager_->get_index("non_existing_index");
    ASSERT_FALSE(invalid_res.ok());
}

// 4. Тест вставки элементов и корректного обновления root_page_id
TEST_F(IndexManagerTest, InsertEntriesAndRootUpdate) {
    std::string index_name = "idx_products_price";
    ASSERT_TRUE(index_manager_->create_index(index_name, "products", "price").ok());

    auto initial_info = index_manager_->get_index_info(index_name);
    ASSERT_TRUE(initial_info.ok());
    PageId initial_root_id = initial_info.value().root_page_id;

    // Вставляем 500 элементов для провоцирования сплита корня
    const int NUM_ENTRIES = 500;
    for (int i = 1; i <= NUM_ENTRIES; ++i) {
        RecordId rid{static_cast<uint32_t>(i / 10 + 1), static_cast<uint16_t>(i % 10)};
        Status st = index_manager_->insert_entry(index_name, i * 10, rid);
        ASSERT_TRUE(st.ok()) << "Failed to insert key: " << i * 10;
    }

    // Проверяем, что root_page_id обновился
    auto updated_info = index_manager_->get_index_info(index_name);
    ASSERT_TRUE(updated_info.ok());
    PageId new_root_id = updated_info.value().root_page_id;

    EXPECT_NE(new_root_id, initial_root_id);

    // Проверяем актуальность корня через get_index()
    auto tree_res = index_manager_->get_index(index_name);
    ASSERT_TRUE(tree_res.ok());
    EXPECT_EQ(tree_res.value().get_root_page_id(), new_root_id);
}

// 5. Тест вставки в несуществующий индекс
TEST_F(IndexManagerTest, InsertToNonExistingIndexFails) {
    RecordId rid{1, 0};
    Status st = index_manager_->insert_entry("unknown_index", 100, rid);
    ASSERT_FALSE(st.ok());
}

// 6. Тест удаления индекса (drop_index)
TEST_F(IndexManagerTest, DropIndexSuccess) {
    std::string index_name = "idx_to_remove";
    ASSERT_TRUE(index_manager_->create_index(index_name, "test_table", "col").ok());

    // Убеждаемся, что индекс существует
    ASSERT_TRUE(index_manager_->get_index_info(index_name).ok());

    // Удаляем индекс
    Status drop_st = index_manager_->drop_index(index_name);
    ASSERT_TRUE(drop_st.ok());

    // Проверяем, что индекс больше не находится в каталоге
    ASSERT_FALSE(index_manager_->get_index_info(index_name).ok());
    ASSERT_FALSE(index_manager_->get_index(index_name).ok());

    // Повторный drop должен возвращать ошибку
    ASSERT_FALSE(index_manager_->drop_index(index_name).ok());
}
// ============================================================================
// Тесты персистентности каталога индексов
// ============================================================================

class IndexCatalogPersistenceTest : public ::testing::Test {
protected:
    const std::string db_file = "test_index_catalog.db";

    void SetUp() override { std::remove(db_file.c_str()); }
    void TearDown() override { std::remove(db_file.c_str()); }
};

// 7. Каталог индексов переживает перезапуск СУБД
TEST_F(IndexCatalogPersistenceTest, CatalogSurvivesRestart) {
    PageId users_root = INVALID_PAGE_ID;

    // ШАГ 1: создаём индексы и наполняем один из них
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.open().ok());

        IndexManager im(pm);
        ASSERT_TRUE(im.open().ok());

        ASSERT_TRUE(im.create_index("idx_users_id", "users", "id").ok());
        ASSERT_TRUE(im.create_index("idx_orders_id", "orders", "id").ok());

        // Достаточно записей, чтобы корень дерева сменился после сплитов
        for (int i = 1; i <= 2000; ++i) {
            ASSERT_TRUE(im.insert_entry("idx_users_id", i, RecordId{static_cast<PageId>(i), 0}).ok());
        }

        users_root = im.get_index_info("idx_users_id").value().root_page_id;
        ASSERT_NE(users_root, INVALID_PAGE_ID);
        pm.close();
    }

    // ШАГ 2: открываем файл заново — каталог должен подняться с диска
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.open().ok());

        IndexManager im(pm);
        ASSERT_TRUE(im.open().ok());

        ASSERT_EQ(im.size(), 2u);
        ASSERT_TRUE(im.has_index("idx_users_id"));
        ASSERT_TRUE(im.has_index("idx_orders_id"));

        auto info = im.get_index_info("idx_users_id");
        ASSERT_TRUE(info.ok());
        EXPECT_EQ(info.value().root_page_id, users_root);
        EXPECT_EQ(info.value().table_name, "users");
        EXPECT_EQ(info.value().column_name, "id");

        // Данные индекса тоже должны быть на месте
        for (int i = 1; i <= 2000; ++i) {
            auto res = im.find_entry("idx_users_id", i);
            ASSERT_TRUE(res.ok()) << "Ключ " << i << " потерян после перезапуска";
            EXPECT_EQ(res.value().page_id, static_cast<PageId>(i));
        }
        pm.close();
    }
}

// 8. Несколько индексов в одном файле не затирают корни друг друга.
//    Раньше каждое дерево писало свой корень в DatabaseMetadata::root_page_id,
//    поэтому второй индекс ломал первый.
TEST_F(IndexCatalogPersistenceTest, MultipleIndexesDoNotOverwriteEachOther) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    IndexManager im(pm);
    ASSERT_TRUE(im.open().ok());

    ASSERT_TRUE(im.create_index("idx_a", "t", "a").ok());
    ASSERT_TRUE(im.create_index("idx_b", "t", "b").ok());
    ASSERT_TRUE(im.create_index("idx_c", "t", "c").ok());

    // Наполняем все три индекса вперемешку, провоцируя сплиты корней
    for (int i = 1; i <= 1500; ++i) {
        ASSERT_TRUE(im.insert_entry("idx_a", i, RecordId{static_cast<PageId>(i), 0}).ok());
        ASSERT_TRUE(im.insert_entry("idx_b", i * 2, RecordId{static_cast<PageId>(i), 1}).ok());
        ASSERT_TRUE(im.insert_entry("idx_c", -i, RecordId{static_cast<PageId>(i), 2}).ok());
    }

    // Корни трёх деревьев обязаны быть различными
    const PageId root_a = im.get_index_info("idx_a").value().root_page_id;
    const PageId root_b = im.get_index_info("idx_b").value().root_page_id;
    const PageId root_c = im.get_index_info("idx_c").value().root_page_id;
    EXPECT_NE(root_a, root_b);
    EXPECT_NE(root_b, root_c);
    EXPECT_NE(root_a, root_c);

    for (int i = 1; i <= 1500; ++i) {
        EXPECT_TRUE(im.find_entry("idx_a", i).ok()) << "idx_a потерял ключ " << i;
        EXPECT_TRUE(im.find_entry("idx_b", i * 2).ok()) << "idx_b потерял ключ " << i * 2;
        EXPECT_TRUE(im.find_entry("idx_c", -i).ok()) << "idx_c потерял ключ " << -i;
    }

    // Каждое дерево должно быть структурно корректным
    for (const char* name : {"idx_a", "idx_b", "idx_c"}) {
        auto tree = im.get_index(name);
        ASSERT_TRUE(tree.ok());
        EXPECT_TRUE(tree.value().validate().ok()) << name << ": " << tree.value().validate().message;
    }

    pm.close();
}

// 9. Удаление ключей через каталог и поиск индекса по колонке
TEST_F(IndexCatalogPersistenceTest, RemoveEntriesAndLookupByColumn) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    IndexManager im(pm);
    ASSERT_TRUE(im.open().ok());
    ASSERT_TRUE(im.create_index("idx_users_id", "users", "id").ok());

    for (int i = 1; i <= 1000; ++i) {
        ASSERT_TRUE(im.insert_entry("idx_users_id", i, RecordId{static_cast<PageId>(i), 0}).ok());
    }

    // Оптимизатор должен уметь находить индекс по паре (таблица, колонка)
    auto by_column = im.find_index_for_column("users", "id");
    ASSERT_TRUE(by_column.ok());
    EXPECT_EQ(by_column.value().index_name, "idx_users_id");
    EXPECT_FALSE(im.find_index_for_column("users", "name").ok());

    for (int i = 1; i <= 500; ++i) {
        ASSERT_TRUE(im.remove_entry("idx_users_id", i).ok()) << "Не удалён ключ " << i;
    }

    for (int i = 1; i <= 1000; ++i) {
        EXPECT_EQ(im.find_entry("idx_users_id", i).ok(), i > 500) << "Ключ " << i;
    }

    std::vector<RecordId> range;
    ASSERT_TRUE(im.range_scan("idx_users_id", 600, 700, range).ok());
    EXPECT_EQ(range.size(), 101u);

    std::vector<RecordId> all;
    ASSERT_TRUE(im.full_scan("idx_users_id", all).ok());
    EXPECT_EQ(all.size(), 500u);

    pm.close();
}

// 10. Повторный индекс по той же колонке запрещён, список индексов таблицы
TEST_F(IndexCatalogPersistenceTest, DuplicateColumnIndexRejectedAndListing) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    IndexManager im(pm);
    ASSERT_TRUE(im.open().ok());

    ASSERT_TRUE(im.create_index("idx_users_id", "users", "id").ok());

    // Другое имя, но та же колонка — бессмысленный дубликат
    auto dup = im.create_index("idx_users_id_2", "users", "id");
    EXPECT_FALSE(dup.ok());

    ASSERT_TRUE(im.create_index("idx_users_age", "users", "age").ok());
    ASSERT_TRUE(im.create_index("idx_orders_id", "orders", "id").ok());

    EXPECT_EQ(im.list_indexes().size(), 3u);
    EXPECT_EQ(im.indexes_for_table("users").size(), 2u);
    EXPECT_EQ(im.indexes_for_table("orders").size(), 1u);
    EXPECT_EQ(im.indexes_for_table("unknown").size(), 0u);

    // Индексы по строковым колонкам пока не поддерживаются — ошибка должна быть явной
    auto str_index = im.create_index("idx_users_name", "users", "name", ColumnType::String);
    EXPECT_FALSE(str_index.ok());
    EXPECT_EQ(str_index.status().code, StatusCode::TypeMismatch);

    // Удаление индекса тоже должно сохраняться на диск
    ASSERT_TRUE(im.drop_index("idx_users_age").ok());
    ASSERT_TRUE(im.reload().ok());
    EXPECT_EQ(im.size(), 2u);
    EXPECT_FALSE(im.has_index("idx_users_age"));

    pm.close();
}
