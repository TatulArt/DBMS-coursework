#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include "../types.h"
#include "./index_key.h"
#include "../storage/page_manager.h"

// Тип узла B+ дерева
enum class BTreePageType : uint8_t {
    LEAF = 0,
    INTERNAL = 1
};

// ============================================================================
// Заголовок страницы B+ дерева (первые байты 4KB страницы).
//
// Размер заголовка сделан равным 16 байтам намеренно: массивы ключей,
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
template <typename KeyT>
class BPlusTreePageT {
public:
    using KeyType = KeyT;

    static constexpr size_t HEADER_SIZE = sizeof(BPlusTreeHeader);

    // Вместимость рассчитывается из размера страницы и размера ключа,
    // а не задаётся константой: лист хранит пары [Key + RecordId(8B)],
    // внутренний узел — N ключей и N+1 указателей [Key + PageId(4B)].
    static constexpr uint16_t MAX_KEYS_LEAF =
        static_cast<uint16_t>((PAGE_SIZE - HEADER_SIZE) / (sizeof(KeyT) + sizeof(RecordId)));

    static constexpr uint16_t MAX_KEYS_INTERNAL =
        static_cast<uint16_t>((PAGE_SIZE - HEADER_SIZE - sizeof(PageId)) / (sizeof(KeyT) + sizeof(PageId)));

    // Минимальная заполненность узла (для корня не действует)
    static constexpr uint16_t MIN_KEYS_LEAF = MAX_KEYS_LEAF / 2;
    static constexpr uint16_t MIN_KEYS_INTERNAL = MAX_KEYS_INTERNAL / 2;

    static void init_leaf_page(Page& page, PageId parent_id = INVALID_PAGE_ID);
    static void init_internal_page(Page& page, PageId parent_id = INVALID_PAGE_ID);

    static BPlusTreeHeader* get_header(Page& page);
    static const BPlusTreeHeader* get_header(const Page& page);

    // Доступ к массивам данных внутри страницы (смещение относительно заголовка)
    static KeyT* get_keys(Page& page);
    static const KeyT* get_keys(const Page& page);

    static RecordId* get_leaf_values(Page& page);         // Только для LEAF
    static const RecordId* get_leaf_values(const Page& page);

    static PageId* get_internal_values(Page& page);       // Только для INTERNAL
    static const PageId* get_internal_values(const Page& page);

    // Бинарный поиск первого ключа >= target
    static int find_key_index(const Page& page, const KeyT& key);

    // Минимальная заполненность для узла заданного типа
    static uint16_t min_keys_for(BTreePageType type);

    // Ключи лежат в странице подряд, сразу за ними — массив RecordId/PageId.
    // Чтобы этот массив оставался выровненным, размер ключа должен быть
    // кратен 4 байтам.
    static_assert(sizeof(KeyT) % 4 == 0, "Размер ключа должен быть кратен 4 байтам");
    static_assert(HEADER_SIZE % 4 == 0, "Заголовок страницы должен быть кратен 4 байтам");

    // Раскладка обязана помещаться в страницу
    static_assert(HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_LEAF
                      + sizeof(RecordId) * MAX_KEYS_LEAF <= PAGE_SIZE,
                  "Листовая страница не помещается в PAGE_SIZE");
    static_assert(HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_INTERNAL
                      + sizeof(PageId) * (MAX_KEYS_INTERNAL + 1) <= PAGE_SIZE,
                  "Внутренняя страница не помещается в PAGE_SIZE");

    // Слияние двух недозаполненных узлов должно помещаться в один узел.
    // Лист: (MIN-1) + MIN ключей. Внутренний узел: (MIN-1) + MIN плюс
    // опущенный из родителя ключ-разделитель, итого 2*MIN.
    static_assert(2 * MIN_KEYS_LEAF - 1 <= MAX_KEYS_LEAF,
                  "Слияние листьев не помещается в страницу");
    static_assert(2 * MIN_KEYS_INTERNAL <= MAX_KEYS_INTERNAL,
                  "Слияние внутренних узлов не помещается в страницу");

    // Узел должен вмещать хотя бы что-то осмысленное
    static_assert(MIN_KEYS_LEAF >= 1 && MIN_KEYS_INTERNAL >= 1,
                  "Ключ слишком велик: узел вмещает менее двух ключей");
};

// ============================================================================
// Итератор для последовательного обхода листовых страниц B+ Дерева
// ============================================================================
template <typename KeyT>
class IndexIteratorT {
public:
    IndexIteratorT(PageManager& page_manager, PageId current_page_id, uint16_t current_slot)
        : page_manager_(&page_manager), current_page_id_(current_page_id), current_slot_(current_slot) {
        normalize();
    }

    ~IndexIteratorT() = default;

    // Проверка достижения конца итерации
    bool is_end() const {
        return current_page_id_ == INVALID_PAGE_ID;
    }

    // Получение текущей пары (Key, RecordId)
    std::pair<KeyT, RecordId> operator*();

    // Отдельные аксессоры (удобнее, чем распаковка пары)
    KeyT key();
    RecordId value();

    // Переход к следующему элементу
    IndexIteratorT& operator++();

    // Операторы сравнения итераторов
    bool operator==(const IndexIteratorT& other) const {
        return current_page_id_ == other.current_page_id_ && current_slot_ == other.current_slot_;
    }

    bool operator!=(const IndexIteratorT& other) const {
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
template <typename KeyT>
class BPlusTreeT {
public:
    // Callback, который вызывается при смене корня дерева.
    // Позволяет владельцу дерева (например IndexManager) сохранить новый
    // root_page_id в своём каталоге вместо записи в 0-ю страницу метаданных БД.
    using RootChangedCallback = std::function<Status(PageId)>;

    explicit BPlusTreeT(PageManager& page_manager, PageId root_page_id = INVALID_PAGE_ID);

    PageId get_root_page_id() const { return root_page_id_; }
    bool empty() const { return root_page_id_ == INVALID_PAGE_ID; }

    // Если listener задан, дерево не трогает 0-ю страницу метаданных БД
    void set_root_listener(RootChangedCallback callback) { root_listener_ = std::move(callback); }

    // Поиск записи по ключу
    Result<RecordId> search(const KeyT& key);

    // Вставка ключа и указателя на запись (RecordId).
    // Ключи уникальны: повторная вставка вернёт UniqueConstraintViolation.
    Status insert(const KeyT& key, const RecordId& rid);

    // Обновление RecordId у существующего ключа (запись переехала на другую страницу)
    Status update(const KeyT& key, const RecordId& rid);

    // Поиск диапазона ключей [low_key, high_key] (обе границы включительно)
    Status scan_range(const KeyT& low_key, const KeyT& high_key, std::vector<RecordId>& result);

    // Поиск диапазона [low_key, high_key) — семантика BETWEEN из задания
    Status scan_range_half_open(const KeyT& low_key, const KeyT& high_key, std::vector<RecordId>& result);

    // Удаление ключа из дерева
    Status remove(const KeyT& key);

    IndexIteratorT<KeyT> begin();

    // Итератор, обозначающий конец (INVALID_PAGE_ID)
    IndexIteratorT<KeyT> end();

    // Поиск итератора на первый элемент, который >= low_key
    IndexIteratorT<KeyT> lower_bound(const KeyT& low_key);

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
    Result<PageId> find_leaf_page(const KeyT& key);
    Result<PageId> find_first_leaf_page();

    Status insert_into_leaf(PageId leaf_id, const KeyT& key, const RecordId& rid);
    Status insert_into_parent(PageId left_child_id, const KeyT& key, PageId right_child_id);

    // Методы для удаления и перебалансировки
    Status remove_from_leaf(PageId leaf_id, const KeyT& key);
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
                            KeyT* prev_key, bool* has_prev, int depth, int* leaf_depth);
};

// ============================================================================
// Конкретные инстанцирования: индекс по INT-колонке и по STRING-колонке.
// Имена без суффикса сохранены ради совместимости с остальным кодом.
// ============================================================================
using BPlusTreePage  = BPlusTreePageT<int32_t>;
using IndexIterator  = IndexIteratorT<int32_t>;
using BPlusTree      = BPlusTreeT<int32_t>;

using StringBPlusTreePage = BPlusTreePageT<StringKey>;
using StringIndexIterator = IndexIteratorT<StringKey>;
using StringBPlusTree     = BPlusTreeT<StringKey>;

extern template class BPlusTreePageT<int32_t>;
extern template class IndexIteratorT<int32_t>;
extern template class BPlusTreeT<int32_t>;

extern template class BPlusTreePageT<StringKey>;
extern template class IndexIteratorT<StringKey>;
extern template class BPlusTreeT<StringKey>;
