#include "./b_plus_tree.h"
#include <algorithm>
#include <cstring>

namespace {
// Защита от зацикливания на повреждённых данных: дерево такой глубины
// не может существовать физически (fanout > 300).
constexpr int MAX_TREE_DEPTH = 64;
} // namespace

// ============================================================================
// Реализация вспомогательного класса BPlusTreePage
// ============================================================================

template <typename KeyT>
void BPlusTreePageT<KeyT>::init_leaf_page(Page& page, PageId parent_id) {
    std::memset(page.data, 0, PAGE_SIZE);
    auto* header = get_header(page);
    header->page_type = BTreePageType::LEAF;
    header->reserved_0 = 0;
    header->reserved_1 = 0;
    header->num_keys = 0;
    header->max_keys = MAX_KEYS_LEAF;
    header->parent_page_id = parent_id;
    header->next_page_id = INVALID_PAGE_ID;
}

template <typename KeyT>
void BPlusTreePageT<KeyT>::init_internal_page(Page& page, PageId parent_id) {
    std::memset(page.data, 0, PAGE_SIZE);
    auto* header = get_header(page);
    header->page_type = BTreePageType::INTERNAL;
    header->reserved_0 = 0;
    header->reserved_1 = 0;
    header->num_keys = 0;
    header->max_keys = MAX_KEYS_INTERNAL;
    header->parent_page_id = parent_id;
    header->next_page_id = INVALID_PAGE_ID;
}

template <typename KeyT>
BPlusTreeHeader* BPlusTreePageT<KeyT>::get_header(Page& page) {
    return reinterpret_cast<BPlusTreeHeader*>(page.data);
}

template <typename KeyT>
const BPlusTreeHeader* BPlusTreePageT<KeyT>::get_header(const Page& page) {
    return reinterpret_cast<const BPlusTreeHeader*>(page.data);
}

template <typename KeyT>
KeyT* BPlusTreePageT<KeyT>::get_keys(Page& page) {
    return reinterpret_cast<KeyT*>(page.data + HEADER_SIZE);
}

template <typename KeyT>
const KeyT* BPlusTreePageT<KeyT>::get_keys(const Page& page) {
    return reinterpret_cast<const KeyT*>(page.data + HEADER_SIZE);
}

template <typename KeyT>
RecordId* BPlusTreePageT<KeyT>::get_leaf_values(Page& page) {
    return reinterpret_cast<RecordId*>(
        page.data + HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_LEAF
    );
}

template <typename KeyT>
const RecordId* BPlusTreePageT<KeyT>::get_leaf_values(const Page& page) {
    return reinterpret_cast<const RecordId*>(
        page.data + HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_LEAF
    );
}

template <typename KeyT>
PageId* BPlusTreePageT<KeyT>::get_internal_values(Page& page) {
    return reinterpret_cast<PageId*>(
        page.data + HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_INTERNAL
    );
}

template <typename KeyT>
const PageId* BPlusTreePageT<KeyT>::get_internal_values(const Page& page) {
    return reinterpret_cast<const PageId*>(
        page.data + HEADER_SIZE + sizeof(KeyT) * MAX_KEYS_INTERNAL
    );
}

template <typename KeyT>
uint16_t BPlusTreePageT<KeyT>::min_keys_for(BTreePageType type) {
    return (type == BTreePageType::LEAF) ? MIN_KEYS_LEAF : MIN_KEYS_INTERNAL;
}

template <typename KeyT>
int BPlusTreePageT<KeyT>::find_key_index(const Page& page, const KeyT& key) {
    const auto* header = get_header(page);
    const auto* keys = get_keys(page);

    int low = 0, high = static_cast<int>(header->num_keys) - 1;
    int idx = header->num_keys;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (keys[mid] >= key) {
            idx = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return idx;
}

// ============================================================================
// Реализация IndexIterator
// ============================================================================

template <typename KeyT>
void IndexIteratorT<KeyT>::load_current_page() {
    if (current_page_id_ == INVALID_PAGE_ID || page_manager_ == nullptr) {
        page_loaded_ = false;
        return;
    }
    Status st = page_manager_->read_page(current_page_id_, current_page_);
    page_loaded_ = st.ok();
}

template <typename KeyT>
void IndexIteratorT<KeyT>::normalize() {
    // Пропускаем пустые листовые страницы и переходим на следующую,
    // если текущий слот вышел за границу заполненности.
    while (current_page_id_ != INVALID_PAGE_ID) {
        load_current_page();
        if (!page_loaded_) {
            break;
        }

        const auto* header = BPlusTreePageT<KeyT>::get_header(current_page_);
        if (header->page_type != BTreePageType::LEAF) {
            break; // Итератор должен ходить только по листьям
        }
        if (current_slot_ < header->num_keys) {
            return; // Позиция корректна
        }

        current_page_id_ = header->next_page_id;
        current_slot_ = 0;
    }

    current_page_id_ = INVALID_PAGE_ID;
    current_slot_ = 0;
    page_loaded_ = false;
}

template <typename KeyT>
std::pair<KeyT, RecordId> IndexIteratorT<KeyT>::operator*() {
    if (is_end()) {
        return {KeyT{}, RecordId{INVALID_PAGE_ID, 0}};
    }
    if (!page_loaded_) {
        load_current_page();
        if (!page_loaded_) {
            return {KeyT{}, RecordId{INVALID_PAGE_ID, 0}};
        }
    }
    const auto* keys = BPlusTreePageT<KeyT>::get_keys(current_page_);
    const auto* values = BPlusTreePageT<KeyT>::get_leaf_values(current_page_);
    return {keys[current_slot_], values[current_slot_]};
}

template <typename KeyT>
KeyT IndexIteratorT<KeyT>::key() {
    return (**this).first;
}

template <typename KeyT>
RecordId IndexIteratorT<KeyT>::value() {
    return (**this).second;
}

template <typename KeyT>
IndexIteratorT<KeyT>& IndexIteratorT<KeyT>::operator++() {
    if (is_end()) {
        return *this;
    }

    ++current_slot_;
    normalize();
    return *this;
}

// ============================================================================
// Реализация BPlusTree
// ============================================================================

template <typename KeyT>
BPlusTreeT<KeyT>::BPlusTreeT(PageManager& page_manager, PageId root_page_id)
    : page_manager_(page_manager), root_page_id_(root_page_id) {

    // Если корень не задан явно, пробуем загрузить его из метаданных (0-я страница)
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page meta_page;
        if (page_manager_.read_page(METADATA_PAGE_ID, meta_page).ok()) {
            const auto* meta = reinterpret_cast<const DatabaseMetadata*>(meta_page.data);
            if (meta->magic_number == DB_MAGIC_NUMBER) {
                root_page_id_ = meta->root_page_id;
            }
        }
    }
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::notify_root_changed() {
    // Если у дерева есть владелец (IndexManager), корень сохраняет он.
    // Иначе — используем 0-ю страницу метаданных БД.
    if (root_listener_) {
        return root_listener_(root_page_id_);
    }
    return flush_metadata();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::flush_metadata() {
    Page meta_page;
    Status st = page_manager_.read_page(METADATA_PAGE_ID, meta_page);
    if (!st.ok()) {
        // 0-й страницы нет (дерево живёт в изолированном файле) — ничего не пишем.
        return Status::OK();
    }

    auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);
    // Обновляем метаданные ТОЛЬКО если 0-я страница — действительно метаданные базы
    if (meta->magic_number == DB_MAGIC_NUMBER) {
        meta->root_page_id = root_page_id_;
        return page_manager_.write_page(METADATA_PAGE_ID, meta_page);
    }

    return Status::OK();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::set_parent(PageId child_id, PageId parent_id) {
    Page child_page;
    Status st = page_manager_.read_page(child_id, child_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePageT<KeyT>::get_header(child_page);
    if (header->parent_page_id == parent_id) {
        return Status::OK(); // Лишняя запись на диск не нужна
    }
    header->parent_page_id = parent_id;
    return page_manager_.write_page(child_id, child_page);
}

template <typename KeyT>
Result<PageId> BPlusTreeT<KeyT>::find_first_leaf_page() {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Tree is empty"));
    }

    PageId current_id = root_page_id_;
    for (int depth = 0; depth < MAX_TREE_DEPTH && current_id != INVALID_PAGE_ID; ++depth) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) return Result<PageId>(st);

        const auto* header = BPlusTreePageT<KeyT>::get_header(page);
        if (header->page_type == BTreePageType::LEAF) {
            return Result<PageId>(current_id);
        }
        if (header->num_keys == 0) {
            return Result<PageId>(Status::Error(StatusCode::CorruptedData,
                                                "Internal node without children"));
        }

        const auto* children = BPlusTreePageT<KeyT>::get_internal_values(page);
        current_id = children[0];
    }
    return Result<PageId>(Status::Error(StatusCode::CorruptedData, "Tree traversal depth exceeded"));
}

template <typename KeyT>
Result<PageId> BPlusTreeT<KeyT>::find_leaf_page(const KeyT& key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Tree is empty"));
    }

    PageId current_id = root_page_id_;
    for (int depth = 0; depth < MAX_TREE_DEPTH && current_id != INVALID_PAGE_ID; ++depth) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) return Result<PageId>(st);

        const auto* header = BPlusTreePageT<KeyT>::get_header(page);
        if (header->page_type == BTreePageType::LEAF) {
            return Result<PageId>(current_id);
        }

        const auto* keys = BPlusTreePageT<KeyT>::get_keys(page);
        const auto* children = BPlusTreePageT<KeyT>::get_internal_values(page);

        // Бинарный поиск вместо линейного: первый ключ > key задаёт нужного потомка
        int low = 0, high = static_cast<int>(header->num_keys) - 1;
        int child_idx = header->num_keys;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (keys[mid] > key) {
                child_idx = mid;
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        current_id = children[child_idx];
    }
    return Result<PageId>(Status::Error(StatusCode::CorruptedData, "Tree traversal depth exceeded"));
}

template <typename KeyT>
IndexIteratorT<KeyT> BPlusTreeT<KeyT>::begin() {
    auto res = find_first_leaf_page();
    if (!res.ok()) {
        return end();
    }
    return IndexIteratorT<KeyT>(page_manager_, res.value(), 0);
}

template <typename KeyT>
IndexIteratorT<KeyT> BPlusTreeT<KeyT>::end() {
    return IndexIteratorT<KeyT>(page_manager_, INVALID_PAGE_ID, 0);
}

template <typename KeyT>
IndexIteratorT<KeyT> BPlusTreeT<KeyT>::lower_bound(const KeyT& low_key) {
    auto res = find_leaf_page(low_key);
    if (!res.ok()) {
        return end();
    }

    PageId leaf_id = res.value();
    Page leaf_page;
    if (!page_manager_.read_page(leaf_id, leaf_page).ok()) {
        return end();
    }

    int slot = BPlusTreePageT<KeyT>::find_key_index(leaf_page, low_key);

    // Конструктор итератора сам перейдёт на следующую страницу,
    // если slot вышел за пределы текущего листа.
    return IndexIteratorT<KeyT>(page_manager_, leaf_id, static_cast<uint16_t>(slot));
}

// ----------------------------------------------------------------------------
// Вставка
// ----------------------------------------------------------------------------

template <typename KeyT>
Status BPlusTreeT<KeyT>::insert(const KeyT& key, const RecordId& rid) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page root_page;
        PageId new_root_id = INVALID_PAGE_ID;

        Status st = page_manager_.allocate_page(new_root_id, root_page);
        if (!st.ok()) return st;

        BPlusTreePageT<KeyT>::init_leaf_page(root_page, INVALID_PAGE_ID);

        auto* keys = BPlusTreePageT<KeyT>::get_keys(root_page);
        auto* values = BPlusTreePageT<KeyT>::get_leaf_values(root_page);
        auto* header = BPlusTreePageT<KeyT>::get_header(root_page);

        keys[0] = key;
        values[0] = rid;
        header->num_keys = 1;

        st = page_manager_.write_page(new_root_id, root_page);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        return notify_root_changed();
    }

    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) return leaf_res.status();

    return insert_into_leaf(leaf_res.value(), key, rid);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::insert_into_leaf(PageId leaf_id, const KeyT& key, const RecordId& rid) {
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePageT<KeyT>::get_header(leaf_page);
    auto* keys = BPlusTreePageT<KeyT>::get_keys(leaf_page);
    auto* values = BPlusTreePageT<KeyT>::get_leaf_values(leaf_page);

    const uint32_t insert_idx = static_cast<uint32_t>(BPlusTreePageT<KeyT>::find_key_index(leaf_page, key));

    // Индексируемые поля уникальны (модификатор INDEXED в задании),
    // поэтому дубликат ключа — ошибка ограничения целостности.
    if (insert_idx < header->num_keys && keys[insert_idx] == key) {
        return Status::Error(StatusCode::UniqueConstraintViolation,
                             "Duplicate key in unique index: " + key_to_string(key));
    }

    // Если место есть — просто вставляем
    if (header->num_keys < BPlusTreePageT<KeyT>::MAX_KEYS_LEAF) {
        for (uint32_t i = header->num_keys; i > insert_idx; --i) {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }
        keys[insert_idx] = key;
        values[insert_idx] = rid;
        header->num_keys++;

        return page_manager_.write_page(leaf_id, leaf_page);
    }

    // Лист переполнен -> Сплит.
    // Собираем N+1 элементов во временный буфер, затем делим пополам.
    const uint32_t total_keys = BPlusTreePageT<KeyT>::MAX_KEYS_LEAF + 1;
    std::vector<KeyT> temp_keys(total_keys);
    std::vector<RecordId> temp_values(total_keys);

    for (uint32_t i = 0; i < insert_idx; ++i) {
        temp_keys[i] = keys[i];
        temp_values[i] = values[i];
    }
    temp_keys[insert_idx] = key;
    temp_values[insert_idx] = rid;
    for (uint32_t i = insert_idx; i < header->num_keys; ++i) {
        temp_keys[i + 1] = keys[i];
        temp_values[i + 1] = values[i];
    }

    Page new_leaf_page;
    PageId new_leaf_id = INVALID_PAGE_ID;
    st = page_manager_.allocate_page(new_leaf_id, new_leaf_page);
    if (!st.ok()) return st;

    // ВНИМАНИЕ: allocate_page могла расширить файл, но leaf_page у нас уже в
    // памяти — указатели header/keys/values остаются валидными.
    BPlusTreePageT<KeyT>::init_leaf_page(new_leaf_page, header->parent_page_id);

    auto* new_header = BPlusTreePageT<KeyT>::get_header(new_leaf_page);
    auto* new_keys = BPlusTreePageT<KeyT>::get_keys(new_leaf_page);
    auto* new_values = BPlusTreePageT<KeyT>::get_leaf_values(new_leaf_page);

    const uint32_t left_count = total_keys / 2;
    const uint32_t right_count = total_keys - left_count;

    header->num_keys = static_cast<uint16_t>(left_count);
    for (uint32_t i = 0; i < left_count; ++i) {
        keys[i] = temp_keys[i];
        values[i] = temp_values[i];
    }

    new_header->num_keys = static_cast<uint16_t>(right_count);
    for (uint32_t i = 0; i < right_count; ++i) {
        new_keys[i] = temp_keys[left_count + i];
        new_values[i] = temp_values[left_count + i];
    }

    // Поддерживаем связный список листьев
    new_header->next_page_id = header->next_page_id;
    header->next_page_id = new_leaf_id;

    st = page_manager_.write_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    st = page_manager_.write_page(new_leaf_id, new_leaf_page);
    if (!st.ok()) return st;

    // Первый ключ правого листа поднимается в родителя как разделитель
    const KeyT split_key = new_keys[0];
    return insert_into_parent(leaf_id, split_key, new_leaf_id);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::insert_into_parent(PageId left_id, const KeyT& key, PageId right_id) {
    Page left_page;
    Status st = page_manager_.read_page(left_id, left_page);
    if (!st.ok()) return st;

    auto* left_header = BPlusTreePageT<KeyT>::get_header(left_page);
    const PageId parent_id = left_header->parent_page_id;

    // 1. Родителя нет — создаём новый корень
    if (parent_id == INVALID_PAGE_ID) {
        Page root_page;
        PageId new_root_id = INVALID_PAGE_ID;
        st = page_manager_.allocate_page(new_root_id, root_page);
        if (!st.ok()) return st;

        BPlusTreePageT<KeyT>::init_internal_page(root_page, INVALID_PAGE_ID);

        auto* root_header = BPlusTreePageT<KeyT>::get_header(root_page);
        auto* root_keys = BPlusTreePageT<KeyT>::get_keys(root_page);
        auto* root_children = BPlusTreePageT<KeyT>::get_internal_values(root_page);

        root_header->num_keys = 1;
        root_keys[0] = key;
        root_children[0] = left_id;
        root_children[1] = right_id;

        st = page_manager_.write_page(new_root_id, root_page);
        if (!st.ok()) return st;

        // Обновляем parent_page_id у потомков
        left_header->parent_page_id = new_root_id;
        st = page_manager_.write_page(left_id, left_page);
        if (!st.ok()) return st;

        st = set_parent(right_id, new_root_id);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        return notify_root_changed();
    }

    // 2. Родитель существует — читаем его
    Page parent_page;
    st = page_manager_.read_page(parent_id, parent_page);
    if (!st.ok()) return st;

    auto* parent_header = BPlusTreePageT<KeyT>::get_header(parent_page);
    auto* parent_keys = BPlusTreePageT<KeyT>::get_keys(parent_page);
    auto* parent_children = BPlusTreePageT<KeyT>::get_internal_values(parent_page);

    // Ищем позицию left_id среди детей. Детей на один больше, чем ключей,
    // поэтому граница цикла — num_keys включительно.
    uint32_t insert_idx = 0;
    while (insert_idx <= parent_header->num_keys && parent_children[insert_idx] != left_id) {
        insert_idx++;
    }
    if (insert_idx > parent_header->num_keys) {
        return Status::Error(StatusCode::CorruptedData,
                             "Child page " + std::to_string(left_id) + " not found in parent");
    }

    // Если место есть — вставляем новый ключ и указатель
    if (parent_header->num_keys < BPlusTreePageT<KeyT>::MAX_KEYS_INTERNAL) {
        for (uint32_t i = parent_header->num_keys; i > insert_idx; --i) {
            parent_keys[i] = parent_keys[i - 1];
            parent_children[i + 1] = parent_children[i];
        }
        parent_keys[insert_idx] = key;
        parent_children[insert_idx + 1] = right_id;
        parent_header->num_keys++;

        st = page_manager_.write_page(parent_id, parent_page);
        if (!st.ok()) return st;

        return set_parent(right_id, parent_id);
    }

    // 3. Сплит внутреннего узла (родитель переполнен)
    const uint32_t total_keys = BPlusTreePageT<KeyT>::MAX_KEYS_INTERNAL + 1;
    std::vector<KeyT> temp_keys(total_keys);
    std::vector<PageId> temp_children(total_keys + 1);

    for (uint32_t i = 0; i <= insert_idx; ++i) {
        temp_children[i] = parent_children[i];
    }
    for (uint32_t i = insert_idx + 1; i <= parent_header->num_keys; ++i) {
        temp_children[i + 1] = parent_children[i];
    }
    temp_children[insert_idx + 1] = right_id;

    for (uint32_t i = 0; i < insert_idx; ++i) {
        temp_keys[i] = parent_keys[i];
    }
    temp_keys[insert_idx] = key;
    for (uint32_t i = insert_idx; i < parent_header->num_keys; ++i) {
        temp_keys[i + 1] = parent_keys[i];
    }

    Page new_internal_page;
    PageId new_internal_id = INVALID_PAGE_ID;
    st = page_manager_.allocate_page(new_internal_id, new_internal_page);
    if (!st.ok()) return st;

    BPlusTreePageT<KeyT>::init_internal_page(new_internal_page, parent_header->parent_page_id);

    auto* new_header = BPlusTreePageT<KeyT>::get_header(new_internal_page);
    auto* new_keys = BPlusTreePageT<KeyT>::get_keys(new_internal_page);
    auto* new_children = BPlusTreePageT<KeyT>::get_internal_values(new_internal_page);

    // Средний ключ уходит наверх и НЕ остаётся ни в одном из узлов
    const uint32_t split_idx = total_keys / 2;
    const KeyT up_key = temp_keys[split_idx];

    parent_header->num_keys = static_cast<uint16_t>(split_idx);
    for (uint32_t i = 0; i < split_idx; ++i) {
        parent_keys[i] = temp_keys[i];
        parent_children[i] = temp_children[i];
    }
    parent_children[split_idx] = temp_children[split_idx];

    new_header->num_keys = static_cast<uint16_t>(total_keys - split_idx - 1);
    for (uint32_t i = 0; i < new_header->num_keys; ++i) {
        new_keys[i] = temp_keys[split_idx + 1 + i];
        new_children[i] = temp_children[split_idx + 1 + i];
    }
    new_children[new_header->num_keys] = temp_children[total_keys];

    st = page_manager_.write_page(parent_id, parent_page);
    if (!st.ok()) return st;

    st = page_manager_.write_page(new_internal_id, new_internal_page);
    if (!st.ok()) return st;

    // Переехавшие дети должны знать нового родителя
    for (uint32_t i = 0; i <= new_header->num_keys; ++i) {
        st = set_parent(new_children[i], new_internal_id);
        if (!st.ok()) return st;
    }

    // Рекурсивно поднимаем up_key к родительскому узлу
    return insert_into_parent(parent_id, up_key, new_internal_id);
}

// ----------------------------------------------------------------------------
// Поиск
// ----------------------------------------------------------------------------

template <typename KeyT>
Result<RecordId> BPlusTreeT<KeyT>::search(const KeyT& key) {
    auto res = find_leaf_page(key);
    if (!res.ok()) return Result<RecordId>(res.status());

    Page leaf_page;
    Status st = page_manager_.read_page(res.value(), leaf_page);
    if (!st.ok()) return Result<RecordId>(st);

    const auto* header = BPlusTreePageT<KeyT>::get_header(leaf_page);
    const auto* keys = BPlusTreePageT<KeyT>::get_keys(leaf_page);
    const auto* values = BPlusTreePageT<KeyT>::get_leaf_values(leaf_page);

    // Бинарный поиск: O(log N) вместо линейного перебора слотов
    const int idx = BPlusTreePageT<KeyT>::find_key_index(leaf_page, key);
    if (idx < header->num_keys && keys[idx] == key) {
        return Result<RecordId>(values[idx]);
    }

    return Result<RecordId>(Status::Error(StatusCode::RecordNotFound,
                                          "Key not found: " + key_to_string(key)));
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::update(const KeyT& key, const RecordId& rid) {
    auto res = find_leaf_page(key);
    if (!res.ok()) return res.status();

    Page leaf_page;
    Status st = page_manager_.read_page(res.value(), leaf_page);
    if (!st.ok()) return st;

    const auto* header = BPlusTreePageT<KeyT>::get_header(leaf_page);
    const auto* keys = BPlusTreePageT<KeyT>::get_keys(leaf_page);
    auto* values = BPlusTreePageT<KeyT>::get_leaf_values(leaf_page);

    const int idx = BPlusTreePageT<KeyT>::find_key_index(leaf_page, key);
    if (idx >= header->num_keys || keys[idx] != key) {
        return Status::Error(StatusCode::RecordNotFound, "Key not found: " + key_to_string(key));
    }

    values[idx] = rid;
    return page_manager_.write_page(res.value(), leaf_page);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::scan_range(const KeyT& low_key, const KeyT& high_key, std::vector<RecordId>& result) {
    result.clear();
    if (low_key > high_key) {
        return Status::OK(); // Пустой диапазон — не ошибка
    }
    // end() держим в переменной: каждый вызов создаёт итератор с 4 КБ кэшем
    // страницы внутри, и вычислять его на каждой итерации цикла расточительно.
    const IndexIteratorT<KeyT> stop = end();
    for (auto it = lower_bound(low_key); it != stop; ++it) {
        auto [key, rid] = *it;
        if (key > high_key) {
            break;
        }
        result.push_back(rid);
    }
    return Status::OK();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::scan_range_half_open(const KeyT& low_key, const KeyT& high_key, std::vector<RecordId>& result) {
    result.clear();
    if (low_key >= high_key) {
        return Status::OK();
    }
    const IndexIteratorT<KeyT> stop = end();
    for (auto it = lower_bound(low_key); it != stop; ++it) {
        auto [key, rid] = *it;
        if (key >= high_key) {
            break;
        }
        result.push_back(rid);
    }
    return Status::OK();
}

// ----------------------------------------------------------------------------
// Удаление
// ----------------------------------------------------------------------------

template <typename KeyT>
Status BPlusTreeT<KeyT>::remove(const KeyT& key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Status::Error(StatusCode::RecordNotFound, "Tree is empty");
    }

    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) {
        return leaf_res.status();
    }

    return remove_from_leaf(leaf_res.value(), key);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::remove_from_leaf(PageId leaf_id, const KeyT& key) {
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePageT<KeyT>::get_header(leaf_page);
    auto* keys = BPlusTreePageT<KeyT>::get_keys(leaf_page);
    auto* values = BPlusTreePageT<KeyT>::get_leaf_values(leaf_page);

    const int remove_idx = BPlusTreePageT<KeyT>::find_key_index(leaf_page, key);
    if (remove_idx >= header->num_keys || keys[remove_idx] != key) {
        return Status::Error(StatusCode::RecordNotFound,
                             "Key not found: " + key_to_string(key));
    }

    // Сдвигаем элементы влево
    for (uint32_t i = static_cast<uint32_t>(remove_idx) + 1; i < header->num_keys; ++i) {
        keys[i - 1] = keys[i];
        values[i - 1] = values[i];
    }
    header->num_keys--;

    st = page_manager_.write_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    // Корень не обязан быть заполнен наполовину
    if (leaf_id == root_page_id_) {
        return adjust_root(leaf_id);
    }

    if (header->num_keys < BPlusTreePageT<KeyT>::MIN_KEYS_LEAF) {
        return coalesce_or_redistribute(leaf_id);
    }

    return Status::OK();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::coalesce_or_redistribute(PageId page_id) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePageT<KeyT>::get_header(page);

    if (page_id == root_page_id_) {
        return adjust_root(page_id);
    }

    const PageId parent_id = header->parent_page_id;
    if (parent_id == INVALID_PAGE_ID) {
        // Узел считает себя корнем, но корень другой — структура повреждена.
        return Status::Error(StatusCode::CorruptedData,
                             "Non-root page " + std::to_string(page_id) + " has no parent");
    }

    Page parent_page;
    st = page_manager_.read_page(parent_id, parent_page);
    if (!st.ok()) return st;

    const auto* parent_header = BPlusTreePageT<KeyT>::get_header(parent_page);
    const auto* parent_children = BPlusTreePageT<KeyT>::get_internal_values(parent_page);

    // Позиция страницы среди детей родителя (детей на один больше, чем ключей)
    uint32_t child_idx = 0;
    while (child_idx <= parent_header->num_keys && parent_children[child_idx] != page_id) {
        child_idx++;
    }
    if (child_idx > parent_header->num_keys) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " not found among parent children");
    }

    // Предпочитаем левого соседа; если его нет — берём правого
    const bool sibling_is_left = (child_idx > 0);
    const uint32_t sibling_idx = sibling_is_left ? child_idx - 1 : child_idx + 1;
    if (sibling_idx > parent_header->num_keys) {
        return Status::OK(); // Соседей нет — перебалансировать нечем
    }

    const PageId sibling_id = parent_children[sibling_idx];
    Page sibling_page;
    st = page_manager_.read_page(sibling_id, sibling_page);
    if (!st.ok()) return st;

    const auto* sibling_header = BPlusTreePageT<KeyT>::get_header(sibling_page);
    const uint16_t min_keys = BPlusTreePageT<KeyT>::min_keys_for(header->page_type);

    // У соседа есть запас — заимствуем один элемент (redistribute)
    if (sibling_header->num_keys > min_keys) {
        return redistribute(page, page_id, sibling_page, sibling_id,
                            parent_page, parent_id, child_idx, sibling_is_left);
    }

    // Иначе — сливаем два узла в один (coalesce)
    return coalesce(page, page_id, sibling_page, sibling_id,
                    parent_page, parent_id, child_idx, sibling_is_left);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::redistribute(Page& page, PageId page_id,
                               Page& sibling, PageId sibling_id,
                               Page& parent, PageId parent_id,
                               uint32_t child_idx, bool sibling_is_left) {
    auto* header = BPlusTreePageT<KeyT>::get_header(page);
    auto* keys = BPlusTreePageT<KeyT>::get_keys(page);
    auto* sib_header = BPlusTreePageT<KeyT>::get_header(sibling);
    auto* sib_keys = BPlusTreePageT<KeyT>::get_keys(sibling);
    auto* parent_keys = BPlusTreePageT<KeyT>::get_keys(parent);

    const bool is_leaf = (header->page_type == BTreePageType::LEAF);
    PageId moved_child = INVALID_PAGE_ID;

    if (is_leaf) {
        auto* values = BPlusTreePageT<KeyT>::get_leaf_values(page);
        auto* sib_values = BPlusTreePageT<KeyT>::get_leaf_values(sibling);

        if (sibling_is_left) {
            // Забираем последний элемент левого соседа в начало страницы
            for (uint32_t i = header->num_keys; i > 0; --i) {
                keys[i] = keys[i - 1];
                values[i] = values[i - 1];
            }
            keys[0] = sib_keys[sib_header->num_keys - 1];
            values[0] = sib_values[sib_header->num_keys - 1];
            header->num_keys++;
            sib_header->num_keys--;

            // Разделитель в родителе — новый первый ключ текущей страницы
            parent_keys[child_idx - 1] = keys[0];
        } else {
            // Забираем первый элемент правого соседа в конец страницы
            keys[header->num_keys] = sib_keys[0];
            values[header->num_keys] = sib_values[0];
            header->num_keys++;

            for (uint32_t i = 1; i < sib_header->num_keys; ++i) {
                sib_keys[i - 1] = sib_keys[i];
                sib_values[i - 1] = sib_values[i];
            }
            sib_header->num_keys--;

            // Разделитель — новый первый ключ правого соседа
            parent_keys[child_idx] = sib_keys[0];
        }
    } else {
        auto* children = BPlusTreePageT<KeyT>::get_internal_values(page);
        auto* sib_children = BPlusTreePageT<KeyT>::get_internal_values(sibling);

        if (sibling_is_left) {
            // Ключ-разделитель опускается в текущий узел,
            // а последний ключ соседа поднимается на его место.
            for (uint32_t i = header->num_keys; i > 0; --i) {
                keys[i] = keys[i - 1];
            }
            for (uint32_t i = header->num_keys + 1; i > 0; --i) {
                children[i] = children[i - 1];
            }
            keys[0] = parent_keys[child_idx - 1];
            children[0] = sib_children[sib_header->num_keys];
            moved_child = children[0];

            parent_keys[child_idx - 1] = sib_keys[sib_header->num_keys - 1];
            header->num_keys++;
            sib_header->num_keys--;
        } else {
            keys[header->num_keys] = parent_keys[child_idx];
            children[header->num_keys + 1] = sib_children[0];
            moved_child = children[header->num_keys + 1];

            parent_keys[child_idx] = sib_keys[0];
            header->num_keys++;

            for (uint32_t i = 1; i < sib_header->num_keys; ++i) {
                sib_keys[i - 1] = sib_keys[i];
            }
            for (uint32_t i = 1; i <= sib_header->num_keys; ++i) {
                sib_children[i - 1] = sib_children[i];
            }
            sib_header->num_keys--;
        }
    }

    Status st = page_manager_.write_page(page_id, page);
    if (!st.ok()) return st;
    st = page_manager_.write_page(sibling_id, sibling);
    if (!st.ok()) return st;
    st = page_manager_.write_page(parent_id, parent);
    if (!st.ok()) return st;

    if (moved_child != INVALID_PAGE_ID) {
        st = set_parent(moved_child, page_id);
        if (!st.ok()) return st;
    }

    return Status::OK();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::coalesce(Page& page, PageId page_id,
                           Page& sibling, PageId sibling_id,
                           Page& parent, PageId parent_id,
                           uint32_t child_idx, bool sibling_is_left) {
    // Нормализуем: всегда сливаем правый узел в левый.
    Page* left = sibling_is_left ? &sibling : &page;
    Page* right = sibling_is_left ? &page : &sibling;
    const PageId left_id = sibling_is_left ? sibling_id : page_id;
    const PageId right_id = sibling_is_left ? page_id : sibling_id;

    // Индекс ключа-разделителя между left и right в родителе,
    // и индекс указателя на right, который надо убрать.
    const uint32_t separator_idx = sibling_is_left ? child_idx - 1 : child_idx;
    const uint32_t removed_child_idx = separator_idx + 1;

    auto* left_header = BPlusTreePageT<KeyT>::get_header(*left);
    auto* left_keys = BPlusTreePageT<KeyT>::get_keys(*left);
    auto* right_header = BPlusTreePageT<KeyT>::get_header(*right);
    auto* right_keys = BPlusTreePageT<KeyT>::get_keys(*right);

    auto* parent_header = BPlusTreePageT<KeyT>::get_header(parent);
    auto* parent_keys = BPlusTreePageT<KeyT>::get_keys(parent);
    auto* parent_children = BPlusTreePageT<KeyT>::get_internal_values(parent);

    std::vector<PageId> reparented;

    if (left_header->page_type == BTreePageType::LEAF) {
        auto* left_values = BPlusTreePageT<KeyT>::get_leaf_values(*left);
        auto* right_values = BPlusTreePageT<KeyT>::get_leaf_values(*right);

        for (uint32_t i = 0; i < right_header->num_keys; ++i) {
            left_keys[left_header->num_keys + i] = right_keys[i];
            left_values[left_header->num_keys + i] = right_values[i];
        }
        left_header->num_keys = static_cast<uint16_t>(left_header->num_keys + right_header->num_keys);

        // Поддерживаем связный список листьев
        left_header->next_page_id = right_header->next_page_id;
    } else {
        auto* left_children = BPlusTreePageT<KeyT>::get_internal_values(*left);
        auto* right_children = BPlusTreePageT<KeyT>::get_internal_values(*right);

        // Ключ-разделитель из родителя опускается вниз между двумя наборами ключей
        left_keys[left_header->num_keys] = parent_keys[separator_idx];
        const uint32_t base = left_header->num_keys + 1;

        for (uint32_t i = 0; i < right_header->num_keys; ++i) {
            left_keys[base + i] = right_keys[i];
        }
        for (uint32_t i = 0; i <= right_header->num_keys; ++i) {
            left_children[base + i] = right_children[i];
            reparented.push_back(right_children[i]);
        }
        left_header->num_keys = static_cast<uint16_t>(base + right_header->num_keys);
    }

    Status st = page_manager_.write_page(left_id, *left);
    if (!st.ok()) return st;

    // Освобождаем правую страницу: обнуляем, чтобы «мусорные» данные
    // не выглядели как валидный узел дерева.
    Page freed;
    freed.clear();
    st = page_manager_.write_page(right_id, freed);
    if (!st.ok()) return st;

    for (PageId child : reparented) {
        st = set_parent(child, left_id);
        if (!st.ok()) return st;
    }

    // Убираем из родителя ключ-разделитель и указатель на правый узел
    for (uint32_t i = separator_idx + 1; i < parent_header->num_keys; ++i) {
        parent_keys[i - 1] = parent_keys[i];
    }
    for (uint32_t i = removed_child_idx + 1; i <= parent_header->num_keys; ++i) {
        parent_children[i - 1] = parent_children[i];
    }
    parent_header->num_keys--;

    st = page_manager_.write_page(parent_id, parent);
    if (!st.ok()) return st;

    // Проверяем родителя на недозаполненность
    if (parent_id == root_page_id_) {
        return adjust_root(parent_id);
    }
    if (parent_header->num_keys < BPlusTreePageT<KeyT>::MIN_KEYS_INTERNAL) {
        return coalesce_or_redistribute(parent_id);
    }

    return Status::OK();
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::adjust_root(PageId root_id) {
    Page root_page;
    Status st = page_manager_.read_page(root_id, root_page);
    if (!st.ok()) return st;

    const auto* header = BPlusTreePageT<KeyT>::get_header(root_page);

    // 1. Корень — лист и он опустел: дерево становится пустым
    if (header->page_type == BTreePageType::LEAF && header->num_keys == 0) {
        Page freed;
        freed.clear();
        st = page_manager_.write_page(root_id, freed);
        if (!st.ok()) return st;

        root_page_id_ = INVALID_PAGE_ID;
        return notify_root_changed();
    }

    // 2. Корень — внутренний узел без ключей: единственный ребёнок становится корнем
    if (header->page_type == BTreePageType::INTERNAL && header->num_keys == 0) {
        const auto* children = BPlusTreePageT<KeyT>::get_internal_values(root_page);
        const PageId new_root_id = children[0];

        st = set_parent(new_root_id, INVALID_PAGE_ID);
        if (!st.ok()) return st;

        Page freed;
        freed.clear();
        st = page_manager_.write_page(root_id, freed);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        return notify_root_changed();
    }

    return Status::OK();
}

// ----------------------------------------------------------------------------
// Проверка целостности (используется в тестах)
// ----------------------------------------------------------------------------

template <typename KeyT>
Status BPlusTreeT<KeyT>::validate() {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Status::OK();
    }
    KeyT prev_key{};
    bool has_prev = false;
    int leaf_depth = -1;
    return validate_subtree(root_page_id_, INVALID_PAGE_ID, &prev_key, &has_prev, 0, &leaf_depth);
}

template <typename KeyT>
Status BPlusTreeT<KeyT>::validate_subtree(PageId page_id, PageId expected_parent,
                                   KeyT* prev_key, bool* has_prev,
                                   int depth, int* leaf_depth) {
    if (depth > MAX_TREE_DEPTH) {
        return Status::Error(StatusCode::CorruptedData, "Tree is too deep");
    }

    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    const auto* header = BPlusTreePageT<KeyT>::get_header(page);
    const auto* keys = BPlusTreePageT<KeyT>::get_keys(page);

    if (header->parent_page_id != expected_parent) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " has wrong parent pointer");
    }

    const uint16_t max_keys = (header->page_type == BTreePageType::LEAF)
                                  ? BPlusTreePageT<KeyT>::MAX_KEYS_LEAF
                                  : BPlusTreePageT<KeyT>::MAX_KEYS_INTERNAL;
    if (header->num_keys > max_keys) {
        return Status::Error(StatusCode::CorruptedData,
                             "Page " + std::to_string(page_id) + " overflows key capacity");
    }

    // Все узлы, кроме корня, должны быть заполнены минимум наполовину
    if (page_id != root_page_id_) {
        const uint16_t min_keys = BPlusTreePageT<KeyT>::min_keys_for(header->page_type);
        if (header->num_keys < min_keys) {
            return Status::Error(StatusCode::CorruptedData,
                                 "Page " + std::to_string(page_id) + " underflows (" +
                                 std::to_string(header->num_keys) + " < " + std::to_string(min_keys) + ")");
        }
    }

    // Ключи внутри узла строго возрастают
    for (uint32_t i = 1; i < header->num_keys; ++i) {
        if (keys[i - 1] >= keys[i]) {
            return Status::Error(StatusCode::CorruptedData,
                                 "Keys are not sorted on page " + std::to_string(page_id));
        }
    }

    if (header->page_type == BTreePageType::LEAF) {
        if (*leaf_depth == -1) {
            *leaf_depth = depth;
        } else if (*leaf_depth != depth) {
            return Status::Error(StatusCode::CorruptedData, "Leaves are on different depths");
        }

        // Глобальный порядок ключей при обходе слева направо
        for (uint32_t i = 0; i < header->num_keys; ++i) {
            if (*has_prev && *prev_key >= keys[i]) {
                return Status::Error(StatusCode::CorruptedData,
                                     "Global key order violated at key " + key_to_string(keys[i]));
            }
            *prev_key = keys[i];
            *has_prev = true;
        }
        return Status::OK();
    }

    if (header->num_keys == 0) {
        return Status::Error(StatusCode::CorruptedData,
                             "Internal page " + std::to_string(page_id) + " has no keys");
    }

    const auto* children = BPlusTreePageT<KeyT>::get_internal_values(page);
    for (uint32_t i = 0; i <= header->num_keys; ++i) {
        st = validate_subtree(children[i], page_id, prev_key, has_prev, depth + 1, leaf_depth);
        if (!st.ok()) return st;
    }

    return Status::OK();
}


// ============================================================================
// Явные инстанцирования: индекс по INT-колонке и по STRING-колонке
// ============================================================================
template class BPlusTreePageT<int32_t>;
template class IndexIteratorT<int32_t>;
template class BPlusTreeT<int32_t>;

template class BPlusTreePageT<StringKey>;
template class IndexIteratorT<StringKey>;
template class BPlusTreeT<StringKey>;
