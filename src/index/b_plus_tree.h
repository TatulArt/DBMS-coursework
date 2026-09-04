#pragma once

#include <cstdint>
#include <functional>
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

// ============================================================================
// Заголовок страницы B+ дерева (первые байты 4KB страницы).
//
// Размер заголовка сделан равным 16 байтам намеренно: массивы ключей (int32_t),
// RecordId и PageId, которые лежат сразу за заголовком, должны быть выровнены
// по границе 4 байт. При «плотном» заголовке в 13 байт обращение к ним было
// невыровненным (undefined behavior и падения на платформах со строгим
// выравниванием).
// ============================================================================
#pragma pack(push, 1)
struct BPlusTreeHeader {
    BTreePageType page_type; // LEAF или INTERNAL
    uint8_t  reserved_0{0};  // Выравнивание
    uint16_t num_keys;       // Текущее количество ключей в узле
    uint16_t max_keys;       // Максимальная вместимость ключей
    uint16_t reserved_1{0};  // Выравнивание
    PageId parent_page_id;   // ID родительской страницы (INVALID_PAGE_ID если корень)
    PageId next_page_id;     // Ссылка на следующий лист (только для LEAF)
};
#pragma pack(pop)

static_assert(sizeof(BPlusTreeHeader) == 16, "BPlusTreeHeader должен занимать 16 байт");

// ============================================================================
// Вспомогательный класс для манипуляции байтами внутри страницы 4KB
// ============================================================================
class BPlusTreePage {
public:
    static constexpr size_t HEADER_SIZE = sizeof(BPlusTreeHeader);

    // Вместимость рассчитывается из размера страницы, а не задаётся константой:
    // лист хранит пары [Key(4B) + RecordId(8B)], внутренний узел — N ключей и
    // N+1 указателей на дочерние страницы [Key(4B) + PageId(4B)].
    static constexpr uint16_t MAX_KEYS_LEAF =
        static_cast<uint16_t>((PAGE_SIZE - HEADER_SIZE) / (sizeof(int32_t) + sizeof(RecordId)));

    static constexpr uint16_t MAX_KEYS_INTERNAL =
        static_cast<uint16_t>((PAGE_SIZE - HEADER_SIZE - sizeof(PageId)) / (sizeof(int32_t) + sizeof(PageId)));

    // Минимальная заполненность узла (для корня не действует)
    static constexpr uint16_t MIN_KEYS_LEAF = MAX_KEYS_LEAF / 2;
    static constexpr uint16_t MIN_KEYS_INTERNAL = MAX_KEYS_INTERNAL / 2;

    static void init_leaf_page(Page& page, PageId parent_id = INVALID_PAGE_ID);
    static void init_internal_page(Page& page, PageId parent_id = INVALID_PAGE_ID);

    static BPlusTreeHeader* get_header(Page& page);
    static const BPlusTreeHeader* get_header(const Page& page);

    // Доступ к массивам данных внутри страницы (смещение относительно заголовка)
    static int32_t* get_keys(Page& page);
    static const int32_t* get_keys(const Page& page);

    static RecordId* get_leaf_values(Page& page);         // Только для LEAF
    static const RecordId* get_leaf_values(const Page& page);

    static PageId* get_internal_values(Page& page);       // Только для INTERNAL
    static const PageId* get_internal_values(const Page& page);

    // Бинарный поиск первого ключа >= target
    static int find_key_index(const Page& page, int32_t key);

    // Минимальная заполненность для узла заданного типа
    static uint16_t min_keys_for(BTreePageType type);
};

// Проверяем, что раскладка гарантированно помещается в страницу
static_assert(BPlusTreePage::HEADER_SIZE
                  + sizeof(int32_t) * BPlusTreePage::MAX_KEYS_LEAF
                  + sizeof(RecordId) * BPlusTreePage::MAX_KEYS_LEAF <= PAGE_SIZE,
              "Листовая страница не помещается в PAGE_SIZE");

static_assert(BPlusTreePage::HEADER_SIZE
                  + sizeof(int32_t) * BPlusTreePage::MAX_KEYS_INTERNAL
                  + sizeof(PageId) * (BPlusTreePage::MAX_KEYS_INTERNAL + 1) <= PAGE_SIZE,
              "Внутренняя страница не помещается в PAGE_SIZE");

// ============================================================================
// Итератор для последовательного обхода листовых страниц B+ Дерева
// ============================================================================
class IndexIterator {
public:
    IndexIterator(PageManager& page_manager, PageId current_page_id, uint16_t current_slot)
        : page_manager_(&page_manager), current_page_id_(current_page_id), current_slot_(current_slot) {
        normalize();
    }

    ~IndexIterator() = default;

    // Проверка достижения конца итерации
    bool is_end() const {
        return current_page_id_ == INVALID_PAGE_ID;
    }

    // Получение текущей пары (Key, RecordId)
    std::pair<int32_t, RecordId> operator*();

    // Отдельные аксессоры (удобнее, чем распаковка пары)
    int32_t key();
    RecordId value();

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

    // Приводит итератор к корректному состоянию: пропускает пустые листы и
    // переходит на следующую страницу, если слот вышел за границу текущей.
    void normalize();

    PageManager* page_manager_{nullptr};
    PageId current_page_id_{INVALID_PAGE_ID};
    uint16_t current_slot_{0};

    // Кэш текущей загруженной листовой страницы
    Page current_page_{};
    bool page_loaded_{false};
};

// ============================================================================
// Главный класс B+ Дерева
// ============================================================================
class BPlusTree {
public:
    // Callback, который вызывается при смене корня дерева.
    // Позволяет владельцу дерева (например IndexManager) сохранить новый
    // root_page_id в своём каталоге вместо записи в 0-ю страницу метаданных БД.
    using RootChangedCallback = std::function<Status(PageId)>;

    explicit BPlusTree(PageManager& page_manager, PageId root_page_id = INVALID_PAGE_ID);

    PageId get_root_page_id() const { return root_page_id_; }
    bool empty() const { return root_page_id_ == INVALID_PAGE_ID; }

    // Если listener задан, дерево не трогает 0-ю страницу метаданных БД
    void set_root_listener(RootChangedCallback callback) { root_listener_ = std::move(callback); }

    // Поиск записи по ключу
    Result<RecordId> search(int32_t key);

    // Вставка ключа и указателя на запись (RecordId).
    // Ключи уникальны: повторная вставка вернёт UniqueConstraintViolation.
    Status insert(int32_t key, const RecordId& rid);

    // Обновление RecordId у существующего ключа (запись переехала на другую страницу)
    Status update(int32_t key, const RecordId& rid);

    // Поиск диапазона ключей [low_key, high_key] (обе границы включительно)
    Status scan_range(int32_t low_key, int32_t high_key, std::vector<RecordId>& result);

    // Поиск диапазона [low_key, high_key) — семантика BETWEEN из задания
    Status scan_range_half_open(int32_t low_key, int32_t high_key, std::vector<RecordId>& result);

    // Удаление ключа из дерева
    Status remove(int32_t key);

    IndexIterator begin();

    // Итератор, обозначающий конец (INVALID_PAGE_ID)
    IndexIterator end();

    // Поиск итератора на первый элемент, который >= low_key
    IndexIterator lower_bound(int32_t low_key);

    // Проверка структурной целостности дерева (используется в тестах)
    Status validate();

private:
    PageManager& page_manager_;
    PageId root_page_id_{INVALID_PAGE_ID};
    RootChangedCallback root_listener_{};

    // Сохранение нового корня: через listener либо в метаданные БД
    Status notify_root_changed();
    Status flush_metadata();

    // Вспомогательные приватные методы
    Result<PageId> find_leaf_page(int32_t key);
    Result<PageId> find_first_leaf_page();

    Status insert_into_leaf(PageId leaf_id, int32_t key, const RecordId& rid);
    Status insert_into_parent(PageId left_child_id, int32_t key, PageId right_child_id);

    // Методы для удаления и перебалансировки
    Status remove_from_leaf(PageId leaf_id, int32_t key);
    Status coalesce_or_redistribute(PageId page_id);
    Status adjust_root(PageId root_id);

    // Заимствование элемента у соседа
    Status redistribute(Page& page, PageId page_id,
                        Page& sibling, PageId sibling_id,
                        Page& parent, PageId parent_id,
                        uint32_t child_idx, bool sibling_is_left);

    // Слияние узла с соседом
    Status coalesce(Page& page, PageId page_id,
                    Page& sibling, PageId sibling_id,
                    Page& parent, PageId parent_id,
                    uint32_t child_idx, bool sibling_is_left);

    // Проставить parent_page_id дочерней странице
    Status set_parent(PageId child_id, PageId parent_id);

    Status validate_subtree(PageId page_id, PageId expected_parent,
                            int32_t* prev_key, bool* has_prev, int depth, int* leaf_depth);
};
