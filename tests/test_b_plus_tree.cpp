#include <gtest/gtest.h>
#include "test_db_path.h"
#include <cstdio>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <map>

#include "../src/index/b_plus_tree.h"

class BPlusTreeTest : public ::testing::Test {
protected:
    std::string db_file;

    void SetUp() override {
        db_file = unique_db_file("test_btree");
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
// ----------------------------------------------------------------------------
// Тест 11: Уникальность ключей (модификатор INDEXED из задания)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, DuplicateKeyIsRejected) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    ASSERT_TRUE(tree.insert(10, RecordId{1, 0}).ok());

    Status dup = tree.insert(10, RecordId{2, 0});
    EXPECT_FALSE(dup.ok());
    EXPECT_EQ(dup.code, StatusCode::UniqueConstraintViolation);

    // Исходная ссылка не должна быть перезаписана
    auto res = tree.search(10);
    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.value().page_id, 1u);

    // Дубликат в дереве с несколькими листьями
    for (int i = 100; i < 1000; ++i) {
        ASSERT_TRUE(tree.insert(i, RecordId{static_cast<PageId>(i), 0}).ok());
    }
    EXPECT_EQ(tree.insert(500, RecordId{9, 9}).code, StatusCode::UniqueConstraintViolation);
    EXPECT_TRUE(tree.validate().ok()) << tree.validate().message;

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 12: update() переставляет ссылку, не меняя структуру дерева
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, UpdateExistingKey) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    for (int i = 1; i <= 1000; ++i) {
        ASSERT_TRUE(tree.insert(i, RecordId{static_cast<PageId>(i), 0}).ok());
    }

    ASSERT_TRUE(tree.update(777, RecordId{4242, 7}).ok());

    auto res = tree.search(777);
    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.value().page_id, 4242u);
    EXPECT_EQ(res.value().slot_id, 7);

    EXPECT_FALSE(tree.update(999999, RecordId{1, 1}).ok());
    EXPECT_TRUE(tree.validate().ok());

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 13: Многоуровневое дерево остаётся корректным (validate)
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, StructureStaysValidAfterManyInserts) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    const int count = 20000; // Гарантированно несколько уровней дерева
    std::vector<int32_t> keys(count);
    std::iota(keys.begin(), keys.end(), 1);
    std::mt19937 g(2024);
    std::shuffle(keys.begin(), keys.end(), g);

    for (int32_t key : keys) {
        ASSERT_TRUE(tree.insert(key, RecordId{static_cast<PageId>(key), 0}).ok());
    }

    Status valid = tree.validate();
    EXPECT_TRUE(valid.ok()) << valid.message;

    // Обход итератором должен вернуть все ключи по возрастанию
    int expected = 1;
    int seen = 0;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        EXPECT_EQ((*it).first, expected++);
        seen++;
    }
    EXPECT_EQ(seen, count);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 14: Удаление с реальным слиянием и перераспределением узлов
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RemoveWithMergeAndRedistribute) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    // 5000 ключей — это много листов, поэтому удаление вызовет
    // и заимствование у соседа, и слияние узлов, и понижение высоты дерева.
    const int count = 5000;
    for (int i = 1; i <= count; ++i) {
        ASSERT_TRUE(tree.insert(i, RecordId{static_cast<PageId>(i), 1}).ok());
    }

    for (int i = 2; i <= count; i += 2) {
        Status st = tree.remove(i);
        ASSERT_TRUE(st.ok()) << "Ошибка удаления ключа " << i << ": " << st.message;
    }

    Status valid = tree.validate();
    ASSERT_TRUE(valid.ok()) << "Дерево повреждено после удалений: " << valid.message;

    for (int i = 1; i <= count; ++i) {
        auto res = tree.search(i);
        if (i % 2 == 1) {
            ASSERT_TRUE(res.ok()) << "Нечётный ключ " << i << " должен существовать";
            EXPECT_EQ(res.value().page_id, static_cast<PageId>(i));
        } else {
            EXPECT_FALSE(res.ok()) << "Чётный ключ " << i << " не должен находиться";
        }
    }

    // Связный список листьев не должен быть порван: итератор обязан
    // вернуть ровно оставшиеся 2500 ключей по возрастанию.
    std::vector<int32_t> scanned;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        scanned.push_back((*it).first);
    }
    ASSERT_EQ(scanned.size(), static_cast<size_t>(count / 2));
    EXPECT_TRUE(std::is_sorted(scanned.begin(), scanned.end()));
    EXPECT_EQ(scanned.front(), 1);
    EXPECT_EQ(scanned.back(), count - 1);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 15: Полное опустошение дерева и повторное наполнение
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RemoveAllThenReinsert) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    const int count = 3000;
    std::vector<int32_t> keys(count);
    std::iota(keys.begin(), keys.end(), 1);
    std::mt19937 g(777);
    std::shuffle(keys.begin(), keys.end(), g);

    for (int32_t key : keys) {
        ASSERT_TRUE(tree.insert(key, RecordId{static_cast<PageId>(key), 0}).ok());
    }

    std::shuffle(keys.begin(), keys.end(), g);
    for (int32_t key : keys) {
        Status st = tree.remove(key);
        ASSERT_TRUE(st.ok()) << "Не удалось удалить ключ " << key << ": " << st.message;
        ASSERT_TRUE(tree.validate().ok()) << "Дерево повреждено после удаления " << key;
    }

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.get_root_page_id(), INVALID_PAGE_ID);
    EXPECT_TRUE(tree.begin() == tree.end());

    // Дерево должно быть готово принимать данные заново
    ASSERT_TRUE(tree.insert(42, RecordId{1, 1}).ok());
    EXPECT_TRUE(tree.search(42).ok());

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 16: Удаление отсутствующего ключа не ломает дерево
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RemoveMissingKeyIsSafe) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    // Удаление из пустого дерева
    EXPECT_FALSE(tree.remove(1).ok());

    for (int i = 0; i < 1000; i += 2) {
        ASSERT_TRUE(tree.insert(i, RecordId{static_cast<PageId>(i), 0}).ok());
    }

    EXPECT_EQ(tree.remove(1).code, StatusCode::RecordNotFound);
    EXPECT_EQ(tree.remove(-5).code, StatusCode::RecordNotFound);
    EXPECT_EQ(tree.remove(100000).code, StatusCode::RecordNotFound);

    EXPECT_TRUE(tree.validate().ok());
    EXPECT_TRUE(tree.search(500).ok());

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 17: Границы диапазонного поиска
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RangeScanBoundaries) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    for (int i = 1; i <= 2000; ++i) {
        ASSERT_TRUE(tree.insert(i * 2, RecordId{static_cast<PageId>(i), 0}).ok()); // чётные 2..4000
    }

    std::vector<RecordId> res;

    // Обе границы включительно
    ASSERT_TRUE(tree.scan_range(10, 20, res).ok());
    ASSERT_EQ(res.size(), 6u); // 10,12,14,16,18,20

    // Полуоткрытый интервал [10, 20) — семантика BETWEEN из задания
    ASSERT_TRUE(tree.scan_range_half_open(10, 20, res).ok());
    ASSERT_EQ(res.size(), 5u); // 10,12,14,16,18

    // Границы не совпадают с существующими ключами
    ASSERT_TRUE(tree.scan_range(11, 19, res).ok());
    EXPECT_EQ(res.size(), 4u); // 12,14,16,18

    // Пустой и перевёрнутый диапазоны
    ASSERT_TRUE(tree.scan_range(5001, 6000, res).ok());
    EXPECT_TRUE(res.empty());
    ASSERT_TRUE(tree.scan_range(100, 50, res).ok());
    EXPECT_TRUE(res.empty());

    // Диапазон, покрывающий всё дерево
    ASSERT_TRUE(tree.scan_range(-1000, 100000, res).ok());
    EXPECT_EQ(res.size(), 2000u);

    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 18: Отрицательные и граничные значения ключей
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, NegativeAndExtremeKeys) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    std::vector<int32_t> keys = {0, -1, 1, INT32_MIN, INT32_MAX, -1000000, 1000000};
    for (int32_t key : keys) {
        ASSERT_TRUE(tree.insert(key, RecordId{static_cast<PageId>(key & 0xFFFF), 0}).ok())
            << "Ключ " << key;
    }

    for (int32_t key : keys) {
        EXPECT_TRUE(tree.search(key).ok()) << "Ключ " << key << " не найден";
    }

    std::vector<int32_t> scanned;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        scanned.push_back((*it).first);
    }
    std::vector<int32_t> expected = keys;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(scanned, expected);

    EXPECT_TRUE(tree.validate().ok());
    pm.close();
}

// ----------------------------------------------------------------------------
// Тест 19: Стресс-тест — случайные вставки и удаления против эталонного std::map
//
// Самая ценная проверка структуры: после каждой операции дерево сверяется
// с эталоном и проходит структурную валидацию. Именно такой сценарий ловил
// расхождение между search() и обходом по связному списку листьев.
// ----------------------------------------------------------------------------
TEST_F(BPlusTreeTest, RandomizedStressAgainstReferenceMap) {
    PageManager pm(db_file);
    ASSERT_TRUE(pm.open().ok());
    BPlusTree tree(pm);

    std::map<int32_t, RecordId> reference;
    std::mt19937 rng(20260904);
    std::uniform_int_distribution<int32_t> key_dist(-3000, 3000);

    const int operations = 6000;
    for (int step = 0; step < operations; ++step) {
        const int32_t key = key_dist(rng);
        // 60% вставок, 40% удалений — дерево то растёт, то сжимается
        const bool do_insert = (rng() % 100) < 60;

        if (do_insert) {
            const RecordId rid{static_cast<PageId>(std::abs(key) + 1), static_cast<uint16_t>(step % 100)};
            Status st = tree.insert(key, rid);

            if (reference.count(key)) {
                ASSERT_EQ(st.code, StatusCode::UniqueConstraintViolation)
                    << "Дубликат ключа " << key << " должен быть отклонён (шаг " << step << ")";
            } else {
                ASSERT_TRUE(st.ok()) << "Вставка " << key << " на шаге " << step << ": " << st.message;
                reference[key] = rid;
            }
        } else {
            Status st = tree.remove(key);
            if (reference.count(key)) {
                ASSERT_TRUE(st.ok()) << "Удаление " << key << " на шаге " << step << ": " << st.message;
                reference.erase(key);
            } else {
                ASSERT_EQ(st.code, StatusCode::RecordNotFound)
                    << "Удаление отсутствующего ключа " << key << " на шаге " << step;
            }
        }

        // Периодическая полная сверка (на каждом шаге это было бы слишком долго)
        if (step % 250 == 0) {
            Status valid = tree.validate();
            ASSERT_TRUE(valid.ok()) << "Шаг " << step << ": " << valid.message;
        }
    }

    ASSERT_TRUE(tree.validate().ok()) << tree.validate().message;

    // 1) Каждый ключ эталона находится точечным поиском
    for (const auto& kv : reference) {
        auto res = tree.search(kv.first);
        ASSERT_TRUE(res.ok()) << "Ключ " << kv.first << " потерян";
        EXPECT_EQ(res.value().page_id, kv.second.page_id);
        EXPECT_EQ(res.value().slot_id, kv.second.slot_id);
    }

    // 2) Обход по связному списку листьев даёт ровно те же ключи в том же порядке.
    //    Именно это расхождение и означало «тихую» потерю данных при слиянии узлов.
    std::vector<int32_t> scanned;
    const IndexIterator stop = tree.end();
    for (auto it = tree.begin(); it != stop; ++it) {
        scanned.push_back((*it).first);
    }

    std::vector<int32_t> expected;
    expected.reserve(reference.size());
    for (const auto& kv : reference) expected.push_back(kv.first);

    ASSERT_EQ(scanned.size(), expected.size())
        << "Обход листьев видит не все записи: " << scanned.size()
        << " вместо " << expected.size();
    EXPECT_EQ(scanned, expected);

    // 3) Диапазонный поиск согласован с эталоном
    std::vector<RecordId> range;
    ASSERT_TRUE(tree.scan_range(-500, 500, range).ok());
    const size_t expected_in_range = static_cast<size_t>(
        std::distance(reference.lower_bound(-500), reference.upper_bound(500)));
    EXPECT_EQ(range.size(), expected_in_range);

    pm.close();
}
