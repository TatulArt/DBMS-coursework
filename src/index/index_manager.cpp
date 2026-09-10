#include "./index_manager.h"
#include <algorithm>
#include <cstring>

namespace {

// Заголовок страницы каталога индексов
#pragma pack(push, 1)
struct IndexCatalogPageHeader {
    uint32_t magic;        // Подпись страницы каталога
    PageId next_page_id;   // Следующая страница цепочки (INVALID_PAGE_ID — последняя)
    uint32_t payload_size; // Сколько полезных байт лежит в этой странице
};
#pragma pack(pop)

constexpr uint32_t CATALOG_MAGIC = 0x1DE0CA7A;
constexpr size_t CATALOG_PAYLOAD_CAPACITY = PAGE_SIZE - sizeof(IndexCatalogPageHeader);
constexpr uint32_t CATALOG_FORMAT_VERSION = 1;

// --- примитивы сериализации ---

void put_u32(std::vector<uint8_t>& out, uint32_t v) {
    out.insert(out.end(), reinterpret_cast<uint8_t*>(&v), reinterpret_cast<uint8_t*>(&v) + sizeof(v));
}

void put_u8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

void put_string(std::vector<uint8_t>& out, const std::string& s) {
    const uint32_t len = static_cast<uint32_t>(s.size());
    put_u32(out, len);
    out.insert(out.end(), s.begin(), s.end());
}

bool take_u32(const std::vector<uint8_t>& in, size_t& pos, uint32_t& out) {
    if (pos + sizeof(uint32_t) > in.size()) return false;
    std::memcpy(&out, in.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    return true;
}

bool take_u8(const std::vector<uint8_t>& in, size_t& pos, uint8_t& out) {
    if (pos + 1 > in.size()) return false;
    out = in[pos++];
    return true;
}

bool take_string(const std::vector<uint8_t>& in, size_t& pos, std::string& out) {
    uint32_t len = 0;
    if (!take_u32(in, pos, len)) return false;
    if (pos + len > in.size()) return false;
    out.assign(reinterpret_cast<const char*>(in.data() + pos), len);
    pos += len;
    return true;
}

} // namespace

IndexManager::IndexManager(PageManager& page_manager)
    : page_manager_(page_manager) {}

// ============================================================================
// Подключение каталога к файлу БД
// ============================================================================

Status IndexManager::open() {
    // Если файл пустой — создаём 0-ю страницу метаданных
    if (page_manager_.get_num_pages() == 0) {
        Page meta_page;
        meta_page.clear();
        auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);
        meta->magic_number = DB_MAGIC_NUMBER;
        meta->root_page_id = INVALID_PAGE_ID;
        meta->index_catalog_page_id = INVALID_PAGE_ID;

        Status st = page_manager_.write_page(METADATA_PAGE_ID, meta_page);
        if (!st.ok()) return st;
    }

    auto meta_res = page_manager_.read_metadata();
    if (!meta_res.ok()) {
        return Status::Error(StatusCode::CorruptedData,
                             "Cannot attach index catalog: page 0 is not a valid database header. " +
                             meta_res.status().message);
    }

    attached_ = true;
    return reload();
}

Status IndexManager::reload() {
    if (!attached_) {
        return Status::OK();
    }

    auto meta_res = page_manager_.read_metadata();
    if (!meta_res.ok()) return meta_res.status();

    const PageId first = meta_res.value().index_catalog_page_id;
    index_catalog_.clear();

    if (first == INVALID_PAGE_ID) {
        return Status::OK(); // Каталог ещё не создавался
    }

    auto bytes_res = read_catalog_pages(first);
    if (!bytes_res.ok()) return bytes_res.status();

    return deserialize_catalog(bytes_res.value());
}

Status IndexManager::save() {
    if (!attached_) {
        return Status::OK(); // Каталог живёт только в памяти
    }
    return write_catalog_pages(serialize_catalog());
}

// ============================================================================
// Сериализация каталога
// ============================================================================

std::vector<uint8_t> IndexManager::serialize_catalog() const {
    std::vector<uint8_t> out;
    put_u32(out, CATALOG_FORMAT_VERSION);
    put_u32(out, static_cast<uint32_t>(index_catalog_.size()));

    // Записи упорядочиваем по имени: так содержимое файла детерминировано
    // и не зависит от порядка обхода хеш-таблицы.
    std::vector<const IndexInfo*> ordered;
    ordered.reserve(index_catalog_.size());
    for (const auto& kv : index_catalog_) {
        ordered.push_back(&kv.second);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const IndexInfo* a, const IndexInfo* b) { return a->index_name < b->index_name; });

    for (const IndexInfo* info : ordered) {
        put_string(out, info->index_name);
        put_string(out, info->table_name);
        put_string(out, info->column_name);
        put_u32(out, info->root_page_id);
        put_u8(out, static_cast<uint8_t>(info->key_type));
    }
    return out;
}

Status IndexManager::deserialize_catalog(const std::vector<uint8_t>& bytes) {
    index_catalog_.clear();

    size_t pos = 0;
    uint32_t version = 0;
    uint32_t count = 0;

    if (!take_u32(bytes, pos, version) || !take_u32(bytes, pos, count)) {
        return Status::Error(StatusCode::CorruptedData, "Index catalog header is truncated");
    }
    if (version != CATALOG_FORMAT_VERSION) {
        return Status::Error(StatusCode::CorruptedData,
                             "Unsupported index catalog version: " + std::to_string(version));
    }

    for (uint32_t i = 0; i < count; ++i) {
        IndexInfo info;
        uint8_t key_type_raw = 0;

        if (!take_string(bytes, pos, info.index_name) ||
            !take_string(bytes, pos, info.table_name) ||
            !take_string(bytes, pos, info.column_name) ||
            !take_u32(bytes, pos, info.root_page_id) ||
            !take_u8(bytes, pos, key_type_raw)) {
            return Status::Error(StatusCode::CorruptedData,
                                 "Index catalog entry #" + std::to_string(i) + " is truncated");
        }

        if (key_type_raw > static_cast<uint8_t>(ColumnType::String)) {
            return Status::Error(StatusCode::CorruptedData, "Unknown column type in index catalog");
        }
        info.key_type = static_cast<ColumnType>(key_type_raw);

        index_catalog_[info.index_name] = std::move(info);
    }

    return Status::OK();
}

Status IndexManager::write_catalog_pages(const std::vector<uint8_t>& bytes) {
    auto meta_res = page_manager_.read_metadata();
    if (!meta_res.ok()) return meta_res.status();
    DatabaseMetadata meta = meta_res.value();

    // Собираем список уже выделенных под каталог страниц, чтобы переиспользовать их
    std::vector<PageId> chain;
    PageId cursor = meta.index_catalog_page_id;
    while (cursor != INVALID_PAGE_ID && chain.size() < page_manager_.get_num_pages()) {
        Page page;
        if (!page_manager_.read_page(cursor, page).ok()) break;

        IndexCatalogPageHeader header{};
        std::memcpy(&header, page.data, sizeof(header));
        if (header.magic != CATALOG_MAGIC) break;

        chain.push_back(cursor);
        cursor = header.next_page_id;
    }

    // Сколько страниц нужно (пустой каталог тоже занимает одну страницу)
    const size_t needed = bytes.empty()
                              ? 1
                              : (bytes.size() + CATALOG_PAYLOAD_CAPACITY - 1) / CATALOG_PAYLOAD_CAPACITY;

    while (chain.size() < needed) {
        PageId new_id = INVALID_PAGE_ID;
        Page new_page;
        Status st = page_manager_.allocate_page(new_id, new_page);
        if (!st.ok()) return st;
        chain.push_back(new_id);
    }

    // Раскладываем данные по страницам цепочки
    size_t offset = 0;
    for (size_t i = 0; i < needed; ++i) {
        const size_t chunk = std::min(CATALOG_PAYLOAD_CAPACITY, bytes.size() - offset);

        Page page;
        page.clear();
        IndexCatalogPageHeader header{};
        header.magic = CATALOG_MAGIC;
        header.next_page_id = (i + 1 < needed) ? chain[i + 1] : INVALID_PAGE_ID;
        header.payload_size = static_cast<uint32_t>(chunk);

        std::memcpy(page.data, &header, sizeof(header));
        if (chunk > 0) {
            std::memcpy(page.data + sizeof(header), bytes.data() + offset, chunk);
        }

        Status st = page_manager_.write_page(chain[i], page);
        if (!st.ok()) return st;

        offset += chunk;
    }

    // Лишние страницы старой цепочки больше не используются.
    // Полноценного списка свободных страниц в PageManager пока нет,
    // поэтому просто обнуляем их, чтобы они не читались как часть каталога.
    for (size_t i = needed; i < chain.size(); ++i) {
        Page freed;
        freed.clear();
        Status st = page_manager_.write_page(chain[i], freed);
        if (!st.ok()) return st;
    }

    if (meta.index_catalog_page_id != chain[0]) {
        meta.index_catalog_page_id = chain[0];
        return page_manager_.write_metadata(meta);
    }
    return Status::OK();
}

Result<std::vector<uint8_t>> IndexManager::read_catalog_pages(PageId first_page_id) const {
    std::vector<uint8_t> bytes;
    PageId cursor = first_page_id;
    size_t guard = 0;

    while (cursor != INVALID_PAGE_ID) {
        if (++guard > page_manager_.get_num_pages() + 1) {
            return Result<std::vector<uint8_t>>(
                Status::Error(StatusCode::CorruptedData, "Cycle detected in index catalog chain"));
        }

        Page page;
        Status st = page_manager_.read_page(cursor, page);
        if (!st.ok()) return Result<std::vector<uint8_t>>(st);

        IndexCatalogPageHeader header{};
        std::memcpy(&header, page.data, sizeof(header));

        if (header.magic != CATALOG_MAGIC) {
            return Result<std::vector<uint8_t>>(
                Status::Error(StatusCode::CorruptedData,
                              "Page " + std::to_string(cursor) + " is not an index catalog page"));
        }
        if (header.payload_size > CATALOG_PAYLOAD_CAPACITY) {
            return Result<std::vector<uint8_t>>(
                Status::Error(StatusCode::CorruptedData, "Index catalog page has invalid payload size"));
        }

        bytes.insert(bytes.end(),
                     page.data + sizeof(header),
                     page.data + sizeof(header) + header.payload_size);
        cursor = header.next_page_id;
    }

    return Result<std::vector<uint8_t>>(std::move(bytes));
}

// ============================================================================
// Управление индексами
// ============================================================================

Result<IndexInfo> IndexManager::create_index(const std::string& index_name,
                                             const std::string& table_name,
                                             const std::string& column_name,
                                             ColumnType key_type) {
    if (index_name.empty() || table_name.empty() || column_name.empty()) {
        return Result<IndexInfo>(Status::Error(StatusCode::InvalidArgument,
                                               "Index name, table name and column name must not be empty"));
    }

    if (index_catalog_.find(index_name) != index_catalog_.end()) {
        return Result<IndexInfo>(Status::Error(StatusCode::InvalidArgument,
                                               "Index already exists: " + index_name));
    }

    // Дублирующий индекс по той же колонке не имеет смысла
    for (const auto& kv : index_catalog_) {
        if (kv.second.table_name == table_name && kv.second.column_name == column_name) {
            return Result<IndexInfo>(Status::Error(
                StatusCode::InvalidArgument,
                "Column " + table_name + "." + column_name +
                    " is already indexed by " + kv.second.index_name));
        }
    }

    // Выделяем корневую страницу дерева и инициализируем её как пустой лист.
    // Раскладка листа зависит от размера ключа, поэтому инициализируем
    // страницу тем классом, который соответствует типу колонки.
    Page root_page;
    PageId root_id = INVALID_PAGE_ID;
    Status st = page_manager_.allocate_page(root_id, root_page);
    if (!st.ok()) {
        return Result<IndexInfo>(st);
    }

    if (key_type == ColumnType::Int) {
        BPlusTreePage::init_leaf_page(root_page, INVALID_PAGE_ID);
    } else {
        StringBPlusTreePage::init_leaf_page(root_page, INVALID_PAGE_ID);
    }
    st = page_manager_.write_page(root_id, root_page);
    if (!st.ok()) {
        return Result<IndexInfo>(st);
    }

    IndexInfo info;
    info.index_name = index_name;
    info.table_name = table_name;
    info.column_name = column_name;
    info.root_page_id = root_id;
    info.key_type = key_type;

    index_catalog_[index_name] = info;

    st = save();
    if (!st.ok()) {
        index_catalog_.erase(index_name);
        return Result<IndexInfo>(st);
    }

    return Result<IndexInfo>(info);
}

Status IndexManager::drop_index(const std::string& index_name) {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Status::Error(StatusCode::RecordNotFound, "Index not found: " + index_name);
    }

    // Примечание: страницы удалённого дерева остаются занятыми в файле.
    // Их переиспользование требует списка свободных страниц в PageManager,
    // которого пока нет.
    index_catalog_.erase(it);
    return save();
}

Result<IndexInfo*> IndexManager::lookup(const std::string& index_name) {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Result<IndexInfo*>(Status::Error(StatusCode::RecordNotFound,
                                                "Index not found: " + index_name));
    }
    return Result<IndexInfo*>(&it->second);
}

template <typename KeyT>
BPlusTreeT<KeyT> IndexManager::make_tree(IndexInfo& info) {
    BPlusTreeT<KeyT> tree(page_manager_, info.root_page_id);

    // Дерево не должно писать свой корень в 0-ю страницу метаданных:
    // там хранится только один root_page_id, и несколько индексов
    // затирали бы корни друг друга. Вместо этого корень сохраняет каталог.
    const std::string name = info.index_name;
    tree.set_root_listener([this, name](PageId new_root) -> Status {
        auto it = index_catalog_.find(name);
        if (it == index_catalog_.end()) {
            return Status::Error(StatusCode::RecordNotFound, "Index disappeared from catalog: " + name);
        }
        if (it->second.root_page_id == new_root) {
            return Status::OK();
        }
        it->second.root_page_id = new_root;
        return save();
    });

    return tree;
}

Result<BPlusTree> IndexManager::get_index(const std::string& index_name) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) {
        return Result<BPlusTree>(info_res.status());
    }
    IndexInfo& info = *info_res.value();

    if (info.key_type != ColumnType::Int) {
        return Result<BPlusTree>(Status::Error(
            StatusCode::TypeMismatch,
            "Index " + index_name + " has " + columnTypeToString(info.key_type) +
                " keys, use get_string_index()"));
    }
    return Result<BPlusTree>(make_tree<int32_t>(info));
}

Result<StringBPlusTree> IndexManager::get_string_index(const std::string& index_name) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) {
        return Result<StringBPlusTree>(info_res.status());
    }
    IndexInfo& info = *info_res.value();

    if (info.key_type != ColumnType::String) {
        return Result<StringBPlusTree>(Status::Error(
            StatusCode::TypeMismatch,
            "Index " + index_name + " has " + columnTypeToString(info.key_type) +
                " keys, use get_index()"));
    }
    return Result<StringBPlusTree>(make_tree<StringKey>(info));
}

// ============================================================================
// Преобразование значения колонки в ключ дерева
// ============================================================================

Result<int32_t> IndexManager::to_int_key(const Value& value) {
    if (value.is_null()) {
        return Result<int32_t>(Status::Error(StatusCode::NullConstraintViolation,
                                             "NULL cannot be used as an index key"));
    }
    if (value.get_type() != ColumnType::Int) {
        return Result<int32_t>(Status::Error(StatusCode::TypeMismatch,
                                             "Index expects an INT key, got " +
                                                 columnTypeToString(value.get_type())));
    }
    return Result<int32_t>(value.get_int());
}

Result<StringKey> IndexManager::to_string_key(const Value& value) {
    if (value.is_null()) {
        return Result<StringKey>(Status::Error(StatusCode::NullConstraintViolation,
                                               "NULL cannot be used as an index key"));
    }
    if (value.get_type() != ColumnType::String) {
        return Result<StringKey>(Status::Error(StatusCode::TypeMismatch,
                                               "Index expects a STRING key, got " +
                                                   columnTypeToString(value.get_type())));
    }
    return StringKey::from_string(value.get_string());
}
// ============================================================================
// Операции над содержимым индекса
// ============================================================================

Status IndexManager::sync_root(IndexInfo& info, PageId actual_root) {
    // Обычно корень уже обновлён listener-ом дерева. Подстраховываемся
    // на случай, когда listener по какой-то причине не сработал.
    if (actual_root != info.root_page_id) {
        info.root_page_id = actual_root;
        return save();
    }
    return Status::OK();
}

template <typename KeyT>
Status IndexManager::insert_typed(IndexInfo& info, const KeyT& key, const RecordId& rid) {
    BPlusTreeT<KeyT> tree = make_tree<KeyT>(info);
    Status st = tree.insert(key, rid);
    if (!st.ok()) return st;
    return sync_root(info, tree.get_root_page_id());
}

template <typename KeyT>
Status IndexManager::remove_typed(IndexInfo& info, const KeyT& key) {
    BPlusTreeT<KeyT> tree = make_tree<KeyT>(info);
    Status st = tree.remove(key);
    if (!st.ok()) return st;
    return sync_root(info, tree.get_root_page_id());
}

Status IndexManager::insert_entry(const std::string& index_name, const Value& key, const RecordId& rid) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto k = to_int_key(key);
        if (!k.ok()) return k.status();
        return insert_typed<int32_t>(info, k.value(), rid);
    }

    auto k = to_string_key(key);
    if (!k.ok()) return k.status();
    return insert_typed<StringKey>(info, k.value(), rid);
}

Status IndexManager::remove_entry(const std::string& index_name, const Value& key) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto k = to_int_key(key);
        if (!k.ok()) return k.status();
        return remove_typed<int32_t>(info, k.value());
    }

    auto k = to_string_key(key);
    if (!k.ok()) return k.status();
    return remove_typed<StringKey>(info, k.value());
}

Status IndexManager::update_entry(const std::string& index_name, const Value& key, const RecordId& rid) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto k = to_int_key(key);
        if (!k.ok()) return k.status();
        return make_tree<int32_t>(info).update(k.value(), rid);
    }

    auto k = to_string_key(key);
    if (!k.ok()) return k.status();
    return make_tree<StringKey>(info).update(k.value(), rid);
}

Result<RecordId> IndexManager::find_entry(const std::string& index_name, const Value& key) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return Result<RecordId>(info_res.status());
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto k = to_int_key(key);
        if (!k.ok()) return Result<RecordId>(k.status());
        return make_tree<int32_t>(info).search(k.value());
    }

    auto k = to_string_key(key);
    if (!k.ok()) return Result<RecordId>(k.status());
    return make_tree<StringKey>(info).search(k.value());
}

Status IndexManager::range_scan(const std::string& index_name,
                                const Value& low_key, const Value& high_key,
                                std::vector<RecordId>& result) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto lo = to_int_key(low_key);
        auto hi = to_int_key(high_key);
        if (!lo.ok()) return lo.status();
        if (!hi.ok()) return hi.status();
        return make_tree<int32_t>(info).scan_range(lo.value(), hi.value(), result);
    }

    auto lo = to_string_key(low_key);
    auto hi = to_string_key(high_key);
    if (!lo.ok()) return lo.status();
    if (!hi.ok()) return hi.status();
    return make_tree<StringKey>(info).scan_range(lo.value(), hi.value(), result);
}

Status IndexManager::range_scan_half_open(const std::string& index_name,
                                          const Value& low_key, const Value& high_key,
                                          std::vector<RecordId>& result) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    if (info.key_type == ColumnType::Int) {
        auto lo = to_int_key(low_key);
        auto hi = to_int_key(high_key);
        if (!lo.ok()) return lo.status();
        if (!hi.ok()) return hi.status();
        return make_tree<int32_t>(info).scan_range_half_open(lo.value(), hi.value(), result);
    }

    auto lo = to_string_key(low_key);
    auto hi = to_string_key(high_key);
    if (!lo.ok()) return lo.status();
    if (!hi.ok()) return hi.status();
    return make_tree<StringKey>(info).scan_range_half_open(lo.value(), hi.value(), result);
}

Status IndexManager::full_scan(const std::string& index_name, std::vector<RecordId>& result) {
    auto info_res = lookup(index_name);
    if (!info_res.ok()) return info_res.status();
    IndexInfo& info = *info_res.value();

    result.clear();

    if (info.key_type == ColumnType::Int) {
        BPlusTree tree = make_tree<int32_t>(info);
        const IndexIterator stop = tree.end();
        for (auto it = tree.begin(); it != stop; ++it) {
            result.push_back((*it).second);
        }
        return Status::OK();
    }

    StringBPlusTree tree = make_tree<StringKey>(info);
    const StringIndexIterator stop = tree.end();
    for (auto it = tree.begin(); it != stop; ++it) {
        result.push_back((*it).second);
    }
    return Status::OK();
}

// ============================================================================
// Справочные методы
// ============================================================================

Result<IndexInfo> IndexManager::get_index_info(const std::string& index_name) const {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Result<IndexInfo>(Status::Error(StatusCode::RecordNotFound,
                                               "Index not found: " + index_name));
    }
    return Result<IndexInfo>(it->second);
}

Result<IndexInfo> IndexManager::find_index_for_column(const std::string& table_name,
                                                      const std::string& column_name) const {
    for (const auto& kv : index_catalog_) {
        if (kv.second.table_name == table_name && kv.second.column_name == column_name) {
            return Result<IndexInfo>(kv.second);
        }
    }
    return Result<IndexInfo>(Status::Error(
        StatusCode::RecordNotFound,
        "No index on column " + table_name + "." + column_name));
}

bool IndexManager::has_index(const std::string& index_name) const {
    return index_catalog_.find(index_name) != index_catalog_.end();
}

std::vector<IndexInfo> IndexManager::list_indexes() const {
    std::vector<IndexInfo> out;
    out.reserve(index_catalog_.size());
    for (const auto& kv : index_catalog_) {
        out.push_back(kv.second);
    }
    std::sort(out.begin(), out.end(),
              [](const IndexInfo& a, const IndexInfo& b) { return a.index_name < b.index_name; });
    return out;
}

std::vector<IndexInfo> IndexManager::indexes_for_table(const std::string& table_name) const {
    std::vector<IndexInfo> out;
    for (const auto& kv : index_catalog_) {
        if (kv.second.table_name == table_name) {
            out.push_back(kv.second);
        }
    }
    std::sort(out.begin(), out.end(),
              [](const IndexInfo& a, const IndexInfo& b) { return a.index_name < b.index_name; });
    return out;
}
