#include <gtest/gtest.h>
#include <cstdio>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

#include "../src/index/b_plus_tree.h"

class BPlusTreeTest : public ::testing::Test {
protected:
    const std::string db_file = "test_btree.db";

    void SetUp() override {
        // Удаляем старый файл базы данных перед каждым тестом
        std::remove(db_file.c_str());
    }

    void TearDown() override {
        // Очищаем за собой тестовый файл после выполнения
        std::remove(db_file.c_str());
    }
};

// ----------------------------------------------------------------------------
// Тест 1: Базовая вставка и поиск единичной записи
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, InsertAndSearchSingle) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.create_database(db_file).ok()); // Создаем и инициализируем 0-ю страницу

    BPlusTree tree(pm);

    int32_t key = 42;
    RecordId rid{1, 5};

    Status st = tree.insert(key, rid);
    ASSERT_TRUE(st.ok()) << st.message;

    auto search_res = tree.search(key);
    ASSERT_TRUE(search_res.ok()) << search_res.status().message;
    
    EXPECT_EQ(search_res.value().page_id, rid.page_id);
    EXPECT_EQ(search_res.value().slot_id, rid.slot_id);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 2: Запрос несуществующего ключа
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, SearchNotFound) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    BPlusTree tree(pm);

    tree.insert(10, RecordId{1, 0});
    tree.insert(20, RecordId{1, 1});

    // Ищем ключ, которого точно нет
    auto search_res = tree.search(999);
    EXPECT_FALSE(search_res.ok());
    EXPECT_EQ(search_res.status().code, StatusCode::RecordNotFound);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 3: Вставка отсортированных последовательных данных (Проверка Split)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, SequentialInsertAndSplit) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    BPlusTree tree(pm);

    // Вставляем 500 элементов (Вместимость одного листа MAX_KEYS_LEAF = 200, 
    // поэтому гарантированно произойдут несколько расщеплений листа и корня)
    const int count = 500;
    for (int i = 1; i <= count; ++i) {
        RecordId rid{static_cast<PageId>(i / 10), static_cast<uint16_t>(i % 10)};
        Status st = tree.insert(i, rid);
        ASSERT_TRUE(st.ok()) << "Ошибка вставки ключа " << i << ": " << st.message;
    }

    // Проверяем, что абсолютно все 500 элементов корректно находятся
    for (int i = 1; i <= count; ++i) {
        auto res = tree.search(i);
        ASSERT_TRUE(res.ok()) << "Не найден ключ " << i << " после сплитов!";
        
        EXPECT_EQ(res.value().page_id, static_cast<PageId>(i / 10));
        EXPECT_EQ(res.value().slot_id, static_cast<uint16_t>(i % 10));
    }

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 4: Вставка элементов в случайном порядке (Unordered Split)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RandomInsertAndSearch) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());

    BPlusTree tree(pm);

    const int count = 600;
    std::vector<int32_t> keys(count);
    std::iota(keys.begin(), keys.end(), 1); // Заполняем 1, 2, 3 ... 600

    // Перемешиваем ключи случайным образом
    std::mt19937 g(1337); // Фиксированный seed для воспроизводимости
    std::shuffle(keys.begin(), keys.end(), g);

    // Вставляем элементы в случайном порядке
    for (int32_t key : keys) {
        RecordId rid{static_cast<PageId>(key), static_cast<uint16_t>(key % 50)};
        Status st = tree.insert(key, rid);
        ASSERT_TRUE(st.ok()) << "Ошибка вставки рандомного ключа " << key << ": " << st.message;
    }

    // Проверяем наличие всех элементов
    for (int32_t key : keys) {
        auto res = tree.search(key);
        ASSERT_TRUE(res.ok()) << "Не найден ключ " << key;
        EXPECT_EQ(res.value().page_id, static_cast<PageId>(key));
    }

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 5: Сохранение состояния и повторное чтение с диска
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, PersistenceTest) {
    PageId saved_root_id = INVALID_PAGE_ID;

    // ШАГ 1: Создаем дерево, вставляем данные и закрываем файл
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.open().ok());

        BPlusTree tree(pm);
        for (int i = 1; i <= 100; ++i) {
            tree.insert(i * 10, RecordId{static_cast<PageId>(i), 1});
        }

        saved_root_id = tree.get_root_page_id();
        ASSERT_NE(saved_root_id, INVALID_PAGE_ID);

        pm.close();
    }

    // ШАГ 2: Симулируем перезапуск СУБД, открываем тот же файл с сохраненным root_id
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.open().ok());

        // Инициализируем дерево с загруженным root_page_id
        BPlusTree tree(pm, saved_root_id);

        // Проверяем, что все данные вычитываются прямо с диска
        for (int i = 1; i <= 100; ++i) {
            auto res = tree.search(i * 10);
            ASSERT_TRUE(res.ok()) << "Ключ " << i * 10 << " не найден после перезапуска!";
            EXPECT_EQ(res.value().page_id, static_cast<PageId>(i));
        }

        pm.close();
    }
}

// ----------------------------------------------------------------------------
// Тест 6: Последовательный обход дерева через итератор (Full Scan)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, IteratorFullScan) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    // Вставляем элементы не по порядку
    std::vector<int32_t> keys = {50, 10, 30, 20, 40, 60, 70};
    for (int key : keys) {
        tree.insert(key, RecordId{static_cast<PageId>(key), 1});
    }

    // Проверяем, что итератор обходит элементы строго по возрастанию
    std::vector<int32_t> expected = {10, 20, 30, 40, 50, 60, 70};
    std::vector<int32_t> actual;

    for (auto it = tree.begin(); it != tree.end(); ++it) {
        auto [key, rid] = *it;
        actual.push_back(key);
    }

    EXPECT_EQ(actual, expected);
    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 7: Сканирование диапазона ключей (Range Scan с lower_bound)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, IteratorRangeScan) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    for (int i = 1; i <= 100; ++i) {
        tree.insert(i * 10, RecordId{static_cast<PageId>(i), 1}); // 10, 20, 30 ... 1000
    }

    // Ищем диапазон от 250 до 500 (должны найти: 250 -> 260, 270, ..., 500)
    int32_t low_key = 255;
    int32_t high_key = 300;

    std::vector<int32_t> range_results;
    for (auto it = tree.lower_bound(low_key); it != tree.end(); ++it) {
        auto [key, rid] = *it;
        if (key > high_key) break; // Выходим из сканирования при превышении верхней границы
        range_results.push_back(key);
    }

    std::vector<int32_t> expected = {260, 270, 280, 290, 300};
    EXPECT_EQ(range_results, expected);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 8: Автоматическая персистентность метаданных на диске
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, AutoMetadataPersistenceTest) {
    // ШАГ 1: Создаем файл базы данных и заполняем дерево
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.create_database(db_file).ok());

        Page meta_page;
        ASSERT_TRUE(pm.read_page(METADATA_PAGE_ID, meta_page).ok());
        auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);

        BPlusTree tree(pm, meta->root_page_id);

        // Вставляем достаточно ключей, чтобы корень сменился несколько раз
        for (int32_t i = 1; i <= 400; ++i) {
            ASSERT_TRUE(tree.insert(i, RecordId{static_cast<PageId>(i), 0}).ok());
        }

        pm.close();
    }

    // ШАГ 2: Переоткрываем БД без явного сохранения root_id во внешней переменной
    {
        PageManager pm(db_file);
        ASSERT_TRUE(pm.open().ok());

        // Читаем root_page_id напрямую из заголовка метаданных БД
        Page meta_page;
        ASSERT_TRUE(pm.read_page(METADATA_PAGE_ID, meta_page).ok());
        auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);

        EXPECT_NE(meta->root_page_id, INVALID_PAGE_ID);

        BPlusTree tree(pm, meta->root_page_id);

        // Проверяем сохраненность данных
        for (int32_t i = 1; i <= 400; ++i) {
            auto res = tree.search(i);
            ASSERT_TRUE(res.ok()) << "Ключ " << i << " не найден из авто-метаданных!";
        }

        pm.close();
    }
}

// ----------------------------------------------------------------------------
// Тест 9: Диапазонный поиск через метод scan_range
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, ScanRangeMethod) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    for (int i = 1; i <= 50; ++i) {
        tree.insert(i * 5, RecordId{static_cast<PageId>(i * 5), static_cast<uint16_t>(i)});
    }

    std::vector<RecordId> results;
    // Ищем диапазон [22, 58] (должны попасть: 25, 30, 35, 40, 45, 50, 55)
    Status st = tree.scan_range(22, 58, results);
    ASSERT_TRUE(st.ok()) << st.message;

    ASSERT_EQ(results.size(), 7);
    EXPECT_EQ(results[0].page_id, 25);
    EXPECT_EQ(results[3].page_id, 40);
    EXPECT_EQ(results[6].page_id, 55);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 10: Удаление элементов и балансировка дерева
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RemoveAndRebalance) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    const int count = 200;
    for (int i = 1; i <= count; ++i) {
        tree.insert(i, RecordId{static_cast<PageId>(i), 1});
    }

    // Удаляем все четные числа
    for (int i = 2; i <= count; i += 2) {
        Status st = tree.remove(i);
        ASSERT_TRUE(st.ok()) << "Ошибка удаления ключа " << i << ": " << st.message;
    }

    // Проверяем отсутствие удаленных и наличие оставшихся элементов
    for (int i = 1; i <= count; ++i) {
        auto res = tree.search(i);
        if (i % 2 == 1) {
            EXPECT_TRUE(res.ok()) << "Нечетный ключ " << i << " должен существовать";
        } else {
            EXPECT_FALSE(res.ok()) << "Четный ключ " << i << " не должен находиться после удаления";
        }
    }

    pm.close();
}