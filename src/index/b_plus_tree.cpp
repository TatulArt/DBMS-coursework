#include "./b_plus_tree.h"
#include <cstring>
#include <algorithm>


// ----------------------------------------------------------------------------
// Реализация IndexIterator
// ----------------------------------------------------------------------------

void IndexIterator::load_current_page() {
    if (current_page_id_ == INVALID_PAGE_ID) {
        page_loaded_ = false;
        return;
    }

    Status st = page_manager_.read_page(current_page_id_, current_page_);
    if (!st.ok()) {
        current_page_id_ = INVALID_PAGE_ID;
        page_loaded_ = false;
        return;
    }
    page_loaded_ = true;
}

std::pair<int32_t, RecordId> IndexIterator::operator*() {
    if (!page_loaded_ || current_page_id_ == INVALID_PAGE_ID) {
        return {0, RecordId{INVALID_PAGE_ID, 0}};
    }

    auto* header = reinterpret_cast<BPlusTreeHeader*>(current_page_.data);
    auto* keys = reinterpret_cast<int32_t*>(current_page_.data + sizeof(BPlusTreeHeader));
    auto* rids = reinterpret_cast<RecordId*>(current_page_.data + sizeof(BPlusTreeHeader) + header->max_keys * sizeof(int32_t));

    return {keys[current_slot_], rids[current_slot_]};
}

IndexIterator& IndexIterator::operator++() {
    if (current_page_id_ == INVALID_PAGE_ID) {
        return *this;
    }

    auto* header = reinterpret_cast<BPlusTreeHeader*>(current_page_.data);
    current_slot_++;

    // Если вышли за границы текущего листа — переходим по next_page_id
    if (current_slot_ >= header->num_keys) {
        current_page_id_ = header->next_page_id;
        current_slot_ = 0;
        load_current_page();
    }

    return *this;
}


// ============================================================================
// Реализация BPlusTreePage (разметка байт 4KB)
// ============================================================================

void BPlusTreePage::init_leaf_page(Page& page, PageId parent_id) {
    page.clear();
    BPlusTreeHeader* header = get_header(page);
    header->page_type = BTreePageType::LEAF;
    header->num_keys = 0;
    header->max_keys = MAX_KEYS_LEAF;
    header->parent_page_id = parent_id;
    header->next_page_id = INVALID_PAGE_ID;
}

void BPlusTreePage::init_internal_page(Page& page, PageId parent_id) {
    page.clear();
    BPlusTreeHeader* header = get_header(page);
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
    auto* header = get_header(page);
    uint8_t* keys_end = page.data + sizeof(BPlusTreeHeader) + (header->max_keys * sizeof(int32_t));
    return reinterpret_cast<RecordId*>(keys_end);
}

PageId* BPlusTreePage::get_internal_values(Page& page) {
    auto* header = get_header(page);
    uint8_t* keys_end = page.data + sizeof(BPlusTreeHeader) + (header->max_keys * sizeof(int32_t));
    return reinterpret_cast<PageId*>(keys_end);
}

int BPlusTreePage::find_key_index(const Page& page, int32_t key) {
    const auto* header = get_header(page);
    const int32_t* keys = get_keys(page);
    
    // Бинарный поиск первого ключа >= key
    const int32_t* it = std::lower_bound(keys, keys + header->num_keys, key);
    return static_cast<int>(std::distance(keys, it));
}

// ============================================================================
// Реализация BPlusTree
// ============================================================================

BPlusTree::BPlusTree(PageManager& page_manager, PageId root_page_id)
    : page_manager_(page_manager), root_page_id_(root_page_id) {}

// Поиск листа возвращает Result<PageId>, чтобы обрабатывать возможные I/O ошибки
Result<PageId> BPlusTree::find_leaf_page(int32_t key) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Status::Error(StatusCode::RecordNotFound, "B+ Tree is empty");
    }

    PageId current_id = root_page_id_;
    while (true) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) {
            return st; // Пробрасываем I/O ошибку от PageManager
        }

        const auto* header = BPlusTreePage::get_header(page);
        if (header->page_type == BTreePageType::LEAF) {
            return current_id; // Достигли нужного листа
        }

        // Внутренний узел: переходим по ветке
        int idx = BPlusTreePage::find_key_index(page, key);
        PageId* children = BPlusTreePage::get_internal_values(page);

        if (idx < header->num_keys && BPlusTreePage::get_keys(page)[idx] == key) {
            current_id = children[idx + 1];
        } else {
            current_id = children[idx];
        }
    }
}

Result<RecordId> BPlusTree::search(int32_t key) {
    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) {
        return leaf_res.status();
    }

    PageId leaf_id = leaf_res.value();
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    int idx = BPlusTreePage::find_key_index(leaf_page, key);
    const auto* header = BPlusTreePage::get_header(leaf_page);

    if (idx < header->num_keys && BPlusTreePage::get_keys(leaf_page)[idx] == key) {
        RecordId* rids = BPlusTreePage::get_leaf_values(leaf_page);
        return rids[idx];
    }

    return Status::Error(StatusCode::RecordNotFound, "Key not found in B+ Tree index");
}

Status BPlusTree::insert(int32_t key, const RecordId& rid) {
    // 1. Создание корня, если дерево еще пустое
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page root_page;
        Status st = page_manager_.allocate_page(root_page_id_, root_page);
        if (!st.ok()) return st;

        BPlusTreePage::init_leaf_page(root_page);
        auto* header = BPlusTreePage::get_header(root_page);
        
        BPlusTreePage::get_keys(root_page)[0] = key;
        BPlusTreePage::get_leaf_values(root_page)[0] = rid;
        header->num_keys = 1;

        return page_manager_.write_page(root_page_id_, root_page);
    }

    // 2. Ищем листовую страницу
    auto leaf_res = find_leaf_page(key);
    if (!leaf_res.ok()) return leaf_res.status();

    return insert_into_leaf(leaf_res.value(), key, rid);
}

Status BPlusTree::insert_into_leaf(PageId leaf_id, int32_t key, const RecordId& rid) {
    Page leaf_page;
    Status st = page_manager_.read_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    auto* header = BPlusTreePage::get_header(leaf_page);
    int idx = BPlusTreePage::find_key_index(leaf_page, key);

    int32_t* keys = BPlusTreePage::get_keys(leaf_page);
    RecordId* rids = BPlusTreePage::get_leaf_values(leaf_page);

    // Сдвиг элементов для сохранения отсортированности
    for (int i = header->num_keys; i > idx; --i) {
        keys[i] = keys[i - 1];
        rids[i] = rids[i - 1];
    }

    keys[idx] = key;
    rids[idx] = rid;
    header->num_keys++;

    st = page_manager_.write_page(leaf_id, leaf_page);
    if (!st.ok()) return st;

    // Расщепление страницы при переполнении
    if (header->num_keys >= header->max_keys) {
        return split_leaf(leaf_id);
    }

    return Status::OK();
}

Status BPlusTree::split_leaf(PageId leaf_id) {
    Page old_leaf;
    Status st = page_manager_.read_page(leaf_id, old_leaf);
    if (!st.ok()) return st;

    auto* old_header = BPlusTreePage::get_header(old_leaf);

    // Выделяем новую страницу с полной проверкой статуса
    PageId new_leaf_id;
    Page new_leaf;
    st = page_manager_.allocate_page(new_leaf_id, new_leaf);
    if (!st.ok()) return st;

    BPlusTreePage::init_leaf_page(new_leaf, old_header->parent_page_id);
    auto* new_header = BPlusTreePage::get_header(new_leaf);

    int split_idx = old_header->num_keys / 2;
    int move_count = old_header->num_keys - split_idx;

    // Перенос элементов
    std::memcpy(BPlusTreePage::get_keys(new_leaf), 
                BPlusTreePage::get_keys(old_leaf) + split_idx, 
                move_count * sizeof(int32_t));

    std::memcpy(BPlusTreePage::get_leaf_values(new_leaf), 
                BPlusTreePage::get_leaf_values(old_leaf) + split_idx, 
                move_count * sizeof(RecordId));

    old_header->num_keys = split_idx;
    new_header->num_keys = move_count;

    // Связывание односвязного списка листьев
    new_header->next_page_id = old_header->next_page_id;
    old_header->next_page_id = new_leaf_id;

    int32_t promoted_key = BPlusTreePage::get_keys(new_leaf)[0];

    // Синхронизируем обе страницы с диском
    st = page_manager_.write_page(leaf_id, old_leaf);
    if (!st.ok()) return st;

    st = page_manager_.write_page(new_leaf_id, new_leaf);
    if (!st.ok()) return st;

    // Поднимаем медианный ключ родителю
    return insert_into_parent(leaf_id, promoted_key, new_leaf_id);
}

Status BPlusTree::insert_into_parent(PageId left_child_id, int32_t key, PageId right_child_id) {
    Page left_child;
    Status st = page_manager_.read_page(left_child_id, left_child);
    if (!st.ok()) return st;

    auto* left_header = BPlusTreePage::get_header(left_child);

    // Если родителя нет — мы расщепили корень, создаем новый корень
    if (left_header->parent_page_id == INVALID_PAGE_ID) {
        Page new_root;
        PageId new_root_id;
        st = page_manager_.allocate_page(new_root_id, new_root);
        if (!st.ok()) return st;

        BPlusTreePage::init_internal_page(new_root);
        auto* root_header = BPlusTreePage::get_header(new_root);
        root_header->num_keys = 1;
        
        BPlusTreePage::get_keys(new_root)[0] = key;
        PageId* children = BPlusTreePage::get_internal_values(new_root);
        children[0] = left_child_id;
        children[1] = right_child_id;

        // Корректируем родительские ссылки у потомков
        left_header->parent_page_id = new_root_id;
        st = page_manager_.write_page(left_child_id, left_child);
        if (!st.ok()) return st;

        Page right_child;
        st = page_manager_.read_page(right_child_id, right_child);
        if (!st.ok()) return st;

        BPlusTreePage::get_header(right_child)->parent_page_id = new_root_id;
        st = page_manager_.write_page(right_child_id, right_child);
        if (!st.ok()) return st;

        st = page_manager_.write_page(new_root_id, new_root);
        if (!st.ok()) return st;

        root_page_id_ = new_root_id;
        return Status::OK();
    }

    // Если родитель существует, добавляем новый ключ в него
    PageId parent_id = left_header->parent_page_id;
    Page parent_page;
    st = page_manager_.read_page(parent_id, parent_page);
    if (!st.ok()) return st;

    auto* parent_header = BPlusTreePage::get_header(parent_page);
    int idx = BPlusTreePage::find_key_index(parent_page, key);

    int32_t* keys = BPlusTreePage::get_keys(parent_page);
    PageId* children = BPlusTreePage::get_internal_values(parent_page);

    for (int i = parent_header->num_keys; i > idx; --i) {
        keys[i] = keys[i - 1];
        children[i + 1] = children[i];
    }

    keys[idx] = key;
    children[idx + 1] = right_child_id;
    parent_header->num_keys++;

    return page_manager_.write_page(parent_id, parent_page);
}

// ----------------------------------------------------------------------------
// Расширение BPlusTree для поддержки Range Scan
// ----------------------------------------------------------------------------

Result<PageId> BPlusTree::find_first_leaf_page() {
    if (root_page_id_ == INVALID_PAGE_ID) {
        return Result<PageId>(Status::Error(StatusCode::RecordNotFound, "Tree is empty"));
    }

    PageId current_id = root_page_id_;
    while (current_id != INVALID_PAGE_ID) {
        Page page;
        Status st = page_manager_.read_page(current_id, page);
        if (!st.ok()) {
            return Result<PageId>(st);
        }

        auto* header = reinterpret_cast<BPlusTreeHeader*>(page.data);
        if (header->page_type == BTreePageType::LEAF) {
            return Result<PageId>(current_id);
        }

        auto* children = reinterpret_cast<PageId*>(
            page.data + sizeof(BPlusTreeHeader) + header->max_keys * sizeof(int32_t)
        );
        current_id = children[0];
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
    auto leaf_res = find_leaf_page(low_key);
    if (!leaf_res.ok()) {
        return end();
    }

    PageId leaf_id = leaf_res.value();
    Page page;
    if (!page_manager_.read_page(leaf_id, page).ok()) {
        return end();
    }

    auto* header = reinterpret_cast<BPlusTreeHeader*>(page.data);
    auto* keys = reinterpret_cast<int32_t*>(page.data + sizeof(BPlusTreeHeader));

    // Находим первый элемент >= low_key
    uint16_t idx = 0;
    while (idx < header->num_keys && keys[idx] < low_key) {
        idx++;
    }

    // Если в этом листе все ключи меньше low_key, переходим на следующий лист
    if (idx >= header->num_keys) {
        if (header->next_page_id == INVALID_PAGE_ID) {
            return end();
        }
        return IndexIterator(page_manager_, header->next_page_id, 0);
    }

    return IndexIterator(page_manager_, leaf_id, idx);
}