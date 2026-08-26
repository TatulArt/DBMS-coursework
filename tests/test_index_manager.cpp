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