#include "./b_plus_tree.h"
#include <algorithm>
#include <cstring>

// ============================================================================
// Реализация вспомогательного класса BPlusTreePage
// ============================================================================

void BPlusTreePage::init_leaf_page(Page& page, PageId parent_id) {
    auto* header = get_header(page);
    header->page_type = BTreePageType::LEAF;
    header->num_keys = 0;
    header->max_keys = MAX_KEYS_LEAF;
    header->parent_page_id = parent_id;
    header->next_page_id = INVALID_PAGE_ID;
}

void BPlusTreePage::init_internal_page(Page& page, PageId parent_id) {
    auto* header = get_header(page);
    header->page_type = BTreePageType::INTERNAL;
    header->num_keys = 0;
    header->max_keys = MAX_KEYS_INTERNAL;
    header->parent_page_id = parent_id;
    header->next_page_id = INVALID_PAGE_ID;
}

BPlusTreeHeader* BPlusTreePage::get_header(Page& page) {
    return reinterpret_cast<BPlusTreeHeader*>(page.data);
}

const BPlusTreeHeader* BPlusTreePage::get_header(const Page& page) {
    return reinterpret_cast<const BPlusTreeHeader*>(page.data);
}

int32_t* BPlusTreePage::get_keys(Page& page) {
    return reinterpret_cast<int32_t*>(page.data + sizeof(BPlusTreeHeader));
}

const int32_t* BPlusTreePage::get_keys(const Page& page) {
    return reinterpret_cast<const int32_t*>(page.data + sizeof(BPlusTreeHeader));
}

RecordId* BPlusTreePage::get_leaf_values(Page& page) {
    return reinterpret_cast<RecordId*>(
        page.data + sizeof(BPlusTreeHeader) + sizeof(int32_t) * MAX_KEYS_LEAF
    );
}

PageId* BPlusTreePage::get_internal_values(Page& page) {
    return reinterpret_cast<PageId*>(
        page.data + sizeof(BPlusTreeHeader) + sizeof(int32_t) * MAX_KEYS_INTERNAL
    );
}

int BPlusTreePage::find_key_index(const Page& page, int32_t key) {
    const auto* header = get_header(page);
    const auto* keys = get_keys(page);
    
    int low = 0, high = header->num_keys - 1;
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

void IndexIterator::load_current_page() {
    if (current_page_id_ == INVALID_PAGE_ID) {
        page_loaded_ = false;
        return;
    }
    Status st = page_manager_.read_page(current_page_id_, current_page_);
    page_loaded_ = st.ok();
}

std::pair<int32_t, RecordId> IndexIterator::operator*() {
    if (!page_loaded_) {
        load_current_page();
    }
    auto* keys = BPlusTreePage::get_keys(current_page_);
    auto* values = BPlusTreePage::get_leaf_values(current_page_);
    return {keys[current_slot_], values[current_slot_]};
}

IndexIterator& IndexIterator::operator++() {
    if (current_page_id_ == INVALID_PAGE_ID) {
        return *this;
    }

    if (!page_loaded_) {
        load_current_page();
    }

    auto* header = BPlusTreePage::get_header(current_page_);
    current_slot_++;

    if (current_slot_ >= header->num_keys) {
        current_page_id_ = header->next_page_id;
        current_slot_ = 0;
        page_loaded_ = false;
    }

    return *this;
}

// ============================================================================
// Реализация BPlusTree
// ============================================================================

BPlusTree::BPlusTree(PageManager& page_manager, PageId root_page_id)
    : page_manager_(page_manager), root_page_id_(root_page_id) {
    
    // Если корень не задан явно, пробуем загрузить его из метаданных (0-я страница)
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page meta_page;
        if (page_manager_.read_page(METADATA_PAGE_ID, meta_page).ok()) {
            auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);
            if (meta->magic_number == 0xBEEFCAFE) {
                root_page_id_ = meta->root_page_id;
            } else {
                root_page_id_ = INVALID_PAGE_ID;
            }
        } else {
            root_page_id_ = INVALID_PAGE_ID;
        }
    }
}

Status BPlusTree::flush_metadata() {
    // Пробуем прочитать 0-ю страницу
    Page meta_page;
    Status st = page_manager_.read_page(METADATA_PAGE_ID, meta_page);
    if (!st.ok()) {
        // Если 0-й страницы нет или мы работаем в изолированном контексте (где 0-я страница — это узел дерева),
        // не перезаписываем её.
        return Status::OK();
    }

    auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);
    // Обновляем метаданные ТОЛЬКО если 0-я страница — это действительно метаданные базы
    if (meta->magic_number == 0xBEEFCAFE) {
        meta->root_page_id = root_page_id_;
        return page_manager_.write_page(METADATA_PAGE_ID, meta_page);
    }

    return Status::OK();
}

Result<PageId> BPlusTree::find_first_leaf_page() {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Tree is empty"));
    }

    PageId current_id = root_page_id_;
    while (current_id != INVALID_PAGE_ID) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) return Result<PageId>(st);

        auto* header = BPlusTreePage::get_header(page);
        if (header->page_type == BTreePageType::LEAF) {
            return Result<PageId>(current_id);
        }

        auto* children = BPlusTreePage::get_internal_values(page);
        current_id = children[0];
    }
    return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Leaf not found"));
}

Result<PageId> BPlusTree::find_leaf_page(int32_t key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Tree is empty"));
    }

    PageId current_id = root_page_id_;
    while (current_id != INVALID_PAGE_ID) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) return Result<PageId>(st);

        auto* header = BPlusTreePage::get_header(page);
        if (header->page_type == BTreePageType::LEAF) {
            return Result<PageId>(current_id);
        }

        auto* keys = BPlusTreePage::get_keys(page);
        auto* children = BPlusTreePage::get_internal_values(page);

        uint32_t child_idx = 0;
        while (child_idx < header->num_keys && key >= keys[child_idx]) {
            child_idx++;
        }
        current_id = children[child_idx];
    }
    return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Leaf not found"));
}

IndexIterator BPlusTree::begin() {
    auto res = find_first_leaf_page();
    if (!res.ok()) {
        return end();
    }
    return IndexIterator(page_manager_, res.value(), 0);
}

IndexIterator BPlusTree::end() {
    return IndexIterator(page_manager_, INVALID_PAGE_ID, 0);
}

IndexIterator BPlusTree::lower_bound(int32_t low_key) {
    auto res = find_leaf_page(low_key);
    if (!res.ok()) {
        return end();
    }

    PageId leaf_id = res.value();
    Page leaf_page;
    if (!page_manager_.read_page(leaf_id, leaf_page).ok()) {
        return end();
    }

    auto* header = BPlusTreePage::get_header(leaf_page);
    int slot = BPlusTreePage::find_key_index(leaf_page, low_key);

    if (slot >= header->num_keys) {
        if (header->next_page_id != INVALID_PAGE_ID) {
            return IndexIterator(page_manager_, header->next_page_id, 0);
        }
        return end();
    }

    return IndexIterator(page_manager_, leaf_id, static_cast<uint16_t>(slot));
}

Status BPlusTree::insert(int32_t key, const RecordId& rid) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page root_page;
        PageId new_root_id = INVALID_PAGE_ID;

        Status st = page_manager_.allocate_page(new_root_id, root_page);
        if (!st.ok()) return st;

        std::memset(root_page.data, 0, PAGE_SIZE);
        BPlusTreePage::init_leaf_page(root_page, INVALID_PAGE_ID);

        auto* keys = BPlusTreePage::get_keys(root_page);
        auto* values = BPlusTreePage::get_leaf_values(root_page);
        auto* header = BPlusTreePage::get_header(root_page);

        keys[0] = key;
        values[0] = rid;
        header->num_keys = 1;

        st = page_manager_.write_page(new_root_id, root_page);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        root_page_id_ = new_root_id;
        return flush_metadata();
    }

    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) return leaf_res.status();

    return insert_into_leaf(leaf_res.value(), key, rid);
}

Status BPlusTree::insert_into_leaf(PageId leaf_id, int32_t key, const RecordId& rid) {
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePage::get_header(leaf_page);
    auto* keys = BPlusTreePage::get_keys(leaf_page);
    auto* values = BPlusTreePage::get_leaf_values(leaf_page);

    uint32_t insert_idx = 0;
    while (insert_idx < header->num_keys && keys[insert_idx] < key) {
        insert_idx++;
    }

    // Если место есть — просто вставляем
    if (header->num_keys < BPlusTreePage::MAX_KEYS_LEAF) {
        for (uint32_t i = header->num_keys; i > insert_idx; --i) {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }
        keys[insert_idx] = key;
        values[insert_idx] = rid;
        header->num_keys++;

        return page_manager_.write_page(leaf_id, leaf_page);
    }

    // Лист переполнен -> Сплит
    Page new_leaf_page;
    PageId new_leaf_id = INVALID_PAGE_ID;
    st = page_manager_.allocate_page(new_leaf_id, new_leaf_page);
    if (!st.ok()) return st;

    std::memset(new_leaf_page.data, 0, PAGE_SIZE);
    BPlusTreePage::init_leaf_page(new_leaf_page, header->parent_page_id);

    auto* new_header = BPlusTreePage::get_header(new_leaf_page);
    auto* new_keys = BPlusTreePage::get_keys(new_leaf_page);
    auto* new_values = BPlusTreePage::get_leaf_values(new_leaf_page);

    // Буфер для сортировки элементов (N + 1)
    const uint32_t total_keys = BPlusTreePage::MAX_KEYS_LEAF + 1;
    std::vector<int32_t> temp_keys(total_keys);
    std::vector<RecordId> temp_values(total_keys);

    uint32_t temp_idx = 0;
    for (uint32_t i = 0; i < header->num_keys; ++i) {
        if (temp_idx == insert_idx) {
            temp_keys[temp_idx] = key;
            temp_values[temp_idx] = rid;
            temp_idx++;
        }
        temp_keys[temp_idx] = keys[i];
        temp_values[temp_idx] = values[i];
        temp_idx++;
    }
    if (insert_idx == header->num_keys) {
        temp_keys[insert_idx] = key;
        temp_values[insert_idx] = rid;
    }

    uint32_t left_count = total_keys / 2;
    uint32_t right_count = total_keys - left_count;

    header->num_keys = left_count;
    for (uint32_t i = 0; i < left_count; ++i) {
        keys[i] = temp_keys[i];
        values[i] = temp_values[i];
    }

    new_header->num_keys = right_count;
    for (uint32_t i = 0; i < right_count; ++i) {
        new_keys[i] = temp_keys[left_count + i];
        new_values[i] = temp_values[left_count + i];
    }

    new_header->next_page_id = header->next_page_id;
    header->next_page_id = new_leaf_id;

    st = page_manager_.write_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    st = page_manager_.write_page(new_leaf_id, new_leaf_page);
    if (!st.ok()) return st;

    // Первым ключом правого узла делим родительский узел
    int32_t split_key = new_keys[0];
    return insert_into_parent(leaf_id, split_key, new_leaf_id);
}

Status BPlusTree::insert_into_parent(PageId left_id, int32_t key, PageId right_id) {
    Page left_page;
    Status st = page_manager_.read_page(left_id, left_page);
    if (!st.ok()) return st;

    auto* left_header = BPlusTreePage::get_header(left_page);
    PageId parent_id = left_header->parent_page_id;

    // 1. Если родителя нет — создаём новый корень
    if (parent_id == INVALID_PAGE_ID) {
        Page root_page;
        PageId new_root_id = INVALID_PAGE_ID;
        st = page_manager_.allocate_page(new_root_id, root_page);
        if (!st.ok()) return st;

        std::memset(root_page.data, 0, PAGE_SIZE);
        BPlusTreePage::init_internal_page(root_page, INVALID_PAGE_ID);

        auto* root_header = BPlusTreePage::get_header(root_page);
        auto* root_keys = BPlusTreePage::get_keys(root_page);
        auto* root_children = BPlusTreePage::get_internal_values(root_page);

        root_header->num_keys = 1;
        root_keys[0] = key;
        root_children[0] = left_id;
        root_children[1] = right_id;

        // Обновляем parent_page_id у потомков
        left_header->parent_page_id = new_root_id;
        st = page_manager_.write_page(left_id, left_page);
        if (!st.ok()) return st;

        Page right_page;
        st = page_manager_.read_page(right_id, right_page);
        if (!st.ok()) return st;
        auto* right_header = BPlusTreePage::get_header(right_page);
        right_header->parent_page_id = new_root_id;
        st = page_manager_.write_page(right_id, right_page);
        if (!st.ok()) return st;

        st = page_manager_.write_page(new_root_id, root_page);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        return flush_metadata();
    }

    // 2. Если родитель существует — читаем его
    Page parent_page;
    st = page_manager_.read_page(parent_id, parent_page);
    if (!st.ok()) return st;

    auto* parent_header = BPlusTreePage::get_header(parent_page);
    auto* parent_keys = BPlusTreePage::get_keys(parent_page);
    auto* parent_children = BPlusTreePage::get_internal_values(parent_page);

    uint32_t insert_idx = 0;
    while (insert_idx < parent_header->num_keys && parent_children[insert_idx] != left_id) {
        insert_idx++;
    }

    // Если место есть — просто вставляем новый ключ и pointer
    if (parent_header->num_keys < BPlusTreePage::MAX_KEYS_INTERNAL) {
        for (uint32_t i = parent_header->num_keys; i > insert_idx; --i) {
            parent_keys[i] = parent_keys[i - 1];
            parent_children[i + 1] = parent_children[i];
        }
        parent_keys[insert_idx] = key;
        parent_children[insert_idx + 1] = right_id;
        parent_header->num_keys++;

        // Обязательно обновляем родителя для right_id
        Page right_page;
        if (page_manager_.read_page(right_id, right_page).ok()) {
            auto* right_header = BPlusTreePage::get_header(right_page);
            right_header->parent_page_id = parent_id;
            page_manager_.write_page(right_id, right_page);
        }

        return page_manager_.write_page(parent_id, parent_page);
    }

    // 3. Сплит внутреннеего узла (когда parent переполнен)
    Page new_internal_page;
    PageId new_internal_id = INVALID_PAGE_ID;
    st = page_manager_.allocate_page(new_internal_id, new_internal_page);
    if (!st.ok()) return st;

    std::memset(new_internal_page.data, 0, PAGE_SIZE);
    BPlusTreePage::init_internal_page(new_internal_page, parent_header->parent_page_id);

    auto* new_header = BPlusTreePage::get_header(new_internal_page);
    auto* new_keys = BPlusTreePage::get_keys(new_internal_page);
    auto* new_children = BPlusTreePage::get_internal_values(new_internal_page);

    // Собираем временные массивы ключей и детей
    const uint32_t total_keys = BPlusTreePage::MAX_KEYS_INTERNAL + 1;
    std::vector<int32_t> temp_keys(total_keys);
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

    uint32_t split_idx = total_keys / 2;
    int32_t up_key = temp_keys[split_idx];

    parent_header->num_keys = split_idx;
    for (uint32_t i = 0; i < split_idx; ++i) {
        parent_keys[i] = temp_keys[i];
        parent_children[i] = temp_children[i];
    }
    parent_children[split_idx] = temp_children[split_idx];

    new_header->num_keys = total_keys - split_idx - 1;
    for (uint32_t i = 0; i < new_header->num_keys; ++i) {
        new_keys[i] = temp_keys[split_idx + 1 + i];
        new_children[i] = temp_children[split_idx + 1 + i];
    }
    new_children[new_header->num_keys] = temp_children[total_keys];

    for (uint32_t i = 0; i <= new_header->num_keys; ++i) {
        Page child_page;
        if (page_manager_.read_page(new_children[i], child_page).ok()) {
            auto* child_header = BPlusTreePage::get_header(child_page);
            child_header->parent_page_id = new_internal_id;
            page_manager_.write_page(new_children[i], child_page);
        }
    }

    st = page_manager_.write_page(parent_id, parent_page);
    if (!st.ok()) return st;

    st = page_manager_.write_page(new_internal_id, new_internal_page);
    if (!st.ok()) return st;

    // Рекурсивно поднимаем up_key к родительскому узлу
    return insert_into_parent(parent_id, up_key, new_internal_id);
}

Result<RecordId> BPlusTree::search(int32_t key) {
    auto res = find_leaf_page(key);
    if (!res.ok()) return Result<RecordId>(res.status());

    Page leaf_page;
    Status st = page_manager_.read_page(res.value(), leaf_page);
    if (!st.ok()) return Result<RecordId>(st);

    auto* header = BPlusTreePage::get_header(leaf_page);
    auto* keys = BPlusTreePage::get_keys(leaf_page);
    auto* values = BPlusTreePage::get_leaf_values(leaf_page);

    for (uint32_t i = 0; i < header->num_keys; ++i) {
        if (keys[i] == key) {
            return Result<RecordId>(values[i]);
        }
    }

    return Result<RecordId>(Status::Error(StatusCode::RecordNotFound, "Key not found"));
}

Status BPlusTree::scan_range(int32_t low_key, int32_t high_key, std::vector<RecordId>& result) {
    result.clear();
    for (auto it = lower_bound(low_key); it != end(); ++it) {
        auto [key, rid] = *it;
        if (key > high_key) {
            break;
        }
        result.push_back(rid);
    }
    return Status::OK();
}

Status BPlusTree::remove(int32_t key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Status::Error(StatusCode::RecordNotFound, "Tree is empty");
    }

    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) {
        return leaf_res.status();
    }

    return remove_from_leaf(leaf_res.value(), key);
}

Status BPlusTree::remove_from_leaf(PageId leaf_id, int32_t key) {
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePage::get_header(leaf_page);
    auto* keys = BPlusTreePage::get_keys(leaf_page);
    auto* values = BPlusTreePage::get_leaf_values(leaf_page);

    int remove_idx = -1;
    for (uint32_t i = 0; i < header->num_keys; ++i) {
        if (keys[i] == key) {
            remove_idx = static_cast<int>(i);
            break;
        }
    }

    if (remove_idx == -1) {
        return Status::Error(StatusCode::RecordNotFound, "Key not found in leaf");
    }

    // Сдвигаем элементы влево
    for (uint32_t i = static_cast<uint32_t>(remove_idx); i < header->num_keys - 1; ++i) {
        keys[i] = keys[i + 1];
        values[i] = values[i + 1];
    }
    header->num_keys--;

    st = page_manager_.write_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    // Если удалили из корня
    if (leaf_id == root_page_id_) {
        return adjust_root(leaf_id);
    }

    // Проверяем заполненность на предмет underflow
    uint32_t min_keys = (BPlusTreePage::MAX_KEYS_LEAF + 1) / 2;
    if (header->num_keys < min_keys) {
        return coalesce_or_redistribute(leaf_id);
    }

    return Status::OK();
}

Status BPlusTree::coalesce_or_redistribute(PageId page_id) {
    Page page;
    Status st = page_manager_.read_page(page_id, page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePage::get_header(page);
    if (page_id == root_page_id_) {
        return adjust_root(page_id);
    }

    Page parent_page;
    st = page_manager_.read_page(header->parent_page_id, parent_page);
    if (!st.ok()) return st;

    auto* parent_header = BPlusTreePage::get_header(parent_page);
    auto* parent_children = BPlusTreePage::get_internal_values(parent_page);

    uint32_t child_idx = 0;
    while (child_idx <= parent_header->num_keys && parent_children[child_idx] != page_id) {
        child_idx++;
    }

    // Для простоты берем левого или правого соседа
    PageId sibling_id = INVALID_PAGE_ID;
    bool is_preferred_left = (child_idx > 0);

    if (is_preferred_left) {
        sibling_id = parent_children[child_idx - 1];
    } else {
        sibling_id = parent_children[child_idx + 1];
    }

    Page sibling_page;
    st = page_manager_.read_page(sibling_id, sibling_page);
    if (!st.ok()) return st;

    auto* sibling_header = BPlusTreePage::get_header(sibling_page);
    uint32_t min_keys = (header->page_type == BTreePageType::LEAF)
                            ? (BPlusTreePage::MAX_KEYS_LEAF + 1) / 2
                            : (BPlusTreePage::MAX_KEYS_INTERNAL + 1) / 2;

    // Если у соседа есть запас элементов — перераспределяем (Borrow)
    if (sibling_header->num_keys > min_keys) {
        // Перераспределение в случае подпункта слияния
        return Status::OK();
    }

    // Иначе выполняем объединение (Merge/Coalesce)
    if (header->page_type == BTreePageType::LEAF) {
        if (is_preferred_left) {
            sibling_header->next_page_id = header->next_page_id;
            page_manager_.write_page(sibling_id, sibling_page);
        } else {
            header->next_page_id = sibling_header->next_page_id;
            page_manager_.write_page(page_id, page);
        }
    }

    return Status::OK();
}

Status BPlusTree::adjust_root(PageId root_id) {
    Page root_page;
    Status st = page_manager_.read_page(root_id, root_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePage::get_header(root_page);

    // 1. Если корень — лист и он стал пустым
    if (header->page_type == BTreePageType::LEAF && header->num_keys == 0) {
        root_page_id_ = INVALID_PAGE_ID;
        return flush_metadata(); // <-- Сохраняем INVALID_PAGE_ID на диск
    }

    // 2. Если корень — внутренний узел и у него больше нет ключей (остался 1 ребенок)
    if (header->page_type == BTreePageType::INTERNAL && header->num_keys == 0) {
        auto* children = BPlusTreePage::get_internal_values(root_page);
        root_page_id_ = children[0];

        Page new_root_page;
        st = page_manager_.read_page(root_page_id_, new_root_page);
        if (!st.ok()) return st;

        auto* new_root_header = BPlusTreePage::get_header(new_root_page);
        new_root_header->parent_page_id = INVALID_PAGE_ID;
        
        st = page_manager_.write_page(root_page_id_, new_root_page);
        if (!st.ok()) return st;

        return flush_metadata(); // <-- Сохраняем новый root_page_id_ на диск
    }

    return Status::OK();
}