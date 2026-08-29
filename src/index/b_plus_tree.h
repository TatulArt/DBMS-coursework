#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include "../types.h"
#include "../storage/page_manager.h"

// Тип узла B+ дерева
enum class BTreePageType : uint8_t {
    LEAF = 0,
    INTERNAL = 1
};

// Заголовок страницы B+ дерева (занимает первые байты 4KB страницы)
#pragma pack(push, 1)
struct BPlusTreeHeader {
    BTreePageType page_type; // LEAF или INTERNAL
    uint16_t num_keys;       // Текущее количество ключей в узле
    uint16_t max_keys;       // Максимальная вместимость ключей
    PageId parent_page_id;   // ID родительской страницы (INVALID_PAGE_ID если корень)
    PageId next_page_id;     // Ссылка на следующий лист (только для LEAF)
};
#pragma pack(pop)

// Класс итератора для обхода листовых страниц B+ Дерева
class IndexIterator {
public:
    IndexIterator(PageManager& page_manager, PageId current_page_id, uint16_t current_slot)
        : page_manager_(page_manager), current_page_id_(current_page_id), current_slot_(current_slot) {
        load_current_page();
    }

    ~IndexIterator() = default;

    // Проверка достижения конца итерации
    bool is_end() const {
        return current_page_id_ == INVALID_PAGE_ID;
    }

    // Получение текущей пары (Key, RecordId)
    std::pair<int32_t, RecordId> operator*();

    // Переход к следующему элементу
    IndexIterator& operator++();

    // Операторы сравнения итераторов
    bool operator==(const IndexIterator& other) const {
        return current_page_id_ == other.current_page_id_ && current_slot_ == other.current_slot_;
    }

    bool operator!=(const IndexIterator& other) const {
        return !(*this == other);
    }

private:
    void load_current_page();

    PageManager& page_manager_;
    PageId current_page_id_{INVALID_PAGE_ID};
    uint16_t current_slot_{0};
    
    // Кэш текущей загруженной листовой страницы
    Page current_page_{};
    bool page_loaded_{false};
};

// Вспомогательный класс для манипуляции байтами внутри страницы 4KB
class BPlusTreePage {
public:
    // Рассчитанная вместимость под размер 4KB (PAGE_SIZE)
    static constexpr uint16_t MAX_KEYS_LEAF = 200;     // Для листов: [Key(4B) + RecordId(8B)]
    static constexpr uint16_t MAX_KEYS_INTERNAL = 250; // Для внутренних: [Key(4B) + PageId(4B)]

    static void init_leaf_page(Page& page, PageId parent_id = INVALID_PAGE_ID);
    static void init_internal_page(Page& page, PageId parent_id = INVALID_PAGE_ID);

    static BPlusTreeHeader* get_header(Page& page);
    static const BPlusTreeHeader* get_header(const Page& page);

    // Доступ к массивам данных внутри страницы (смещение относительно заголовка)
    static int32_t* get_keys(Page& page);
    static const int32_t* get_keys(const Page& page);
    
    static RecordId* get_leaf_values(Page& page);   // Только для LEAF
    static PageId* get_internal_values(Page& page); // Только для INTERNAL

    // Бинарный поиск первого ключа >= target
    static int find_key_index(const Page& page, int32_t key);
};

// Главный класс B+ Дерева
class BPlusTree {
public:
    explicit BPlusTree(PageManager& page_manager, PageId root_page_id = INVALID_PAGE_ID);

    PageId get_root_page_id() const { return root_page_id_; }

    // Поиск записи по ключу
    Result<RecordId> search(int32_t key);

    // Вставка ключа и указателя на запись (RecordId)
    Status insert(int32_t key, const RecordId& rid);

    // Поиск диапазона ключей [low_key, high_key]
    Status scan_range(int32_t low_key, int32_t high_key, std::vector<RecordId>& result);

    // Удаление ключа из дерева
    Status remove(int32_t key);

    IndexIterator begin();

    // Итератор, обозначающий конец (INVALID_PAGE_ID)
    IndexIterator end();

    // Поиск итератора на первый элемент, который >= low_key
    IndexIterator lower_bound(int32_t low_key);

private:
    PageManager& page_manager_;
    PageId root_page_id_{INVALID_PAGE_ID};

    // 2. ДОБАВЬТЕ объявление метода flush_metadata в приватную секцию:
    Status flush_metadata();

    // Вспомогательные приватные методы
    Result<PageId> find_leaf_page(int32_t key);
    
    Status insert_into_leaf(PageId leaf_id, int32_t key, const RecordId& rid);
    Status split_leaf(PageId leaf_id);
    Status insert_into_parent(PageId left_child_id, int32_t key, PageId right_child_id);

    // Методы для удаления и перебалансировки
    Status remove_from_leaf(PageId leaf_id, int32_t key);
    Status coalesce_or_redistribute(PageId page_id);
    Status adjust_root(PageId root_id);

    Result<PageId> find_first_leaf_page();
};