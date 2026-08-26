#include "./index_manager.h"
#include <cstring>

IndexManager::IndexManager(PageManager& page_manager)
    : page_manager_(page_manager) {}

Result<IndexInfo> IndexManager::create_index(const std::string& index_name, 
                                             const std::string& table_name, 
                                             const std::string& column_name) {
    // 1. Проверяем, существует ли индекс с таким именем
    if (index_catalog_.find(index_name) != index_catalog_.end()) {
        return Result<IndexInfo>(Status::Error(StatusCode::InvalidArgument, 
                                               "Index already exists: " + index_name));
    }

    // 2. Выделяем начальную страницу под корень B+ дерева
    Page root_page;
    PageId root_id = INVALID_PAGE_ID;
    Status st = page_manager_.allocate_page(root_id, root_page);
    if (!st.ok()) {
        return Result<IndexInfo>(st);
    }

    // 3. Инициализируем пустую листовую страницу для корня
    std::memset(root_page.data, 0, PAGE_SIZE);
    BPlusTreePage::init_leaf_page(root_page, INVALID_PAGE_ID);

    st = page_manager_.write_page(root_id, root_page);
    if (!st.ok()) {
        return Result<IndexInfo>(st);
    }

    // 4. Заносим метаданные индекса в каталог
    IndexInfo info{index_name, table_name, column_name, root_id};
    index_catalog_[index_name] = info;

    return Result<IndexInfo>(info);
}

Result<BPlusTree> IndexManager::get_index(const std::string& index_name) {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Result<BPlusTree>(Status::Error(StatusCode::RecordNotFound, 
                                               "Index not found: " + index_name));
    }

    // Возвращаем объект BPlusTree с актуальным root_page_id из каталога
    return Result<BPlusTree>(BPlusTree(page_manager_, it->second.root_page_id));
}

Status IndexManager::insert_entry(const std::string& index_name, int32_t key, const RecordId& rid) {
    // 1. Находим индекс в каталоге
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Status::Error(StatusCode::RecordNotFound, "Index not found: " + index_name);
    }

    IndexInfo& info = it->second;

    // 2. Создаем временный объект дерева с текущим корнем
    BPlusTree tree(page_manager_, info.root_page_id);

    // 3. Выполняем вставку в B+ дерево
    Status st = tree.insert(key, rid);
    if (!st.ok()) {
        return st;
    }

    // 4. КЛЮЧЕВОЙ МОМЕНТ:
    // Если произошел сплит корня, tree.get_root_page_id() вернет новый PageId.
    // Обязательно обновляем root_page_id в нашем каталоге!
    PageId updated_root_id = tree.get_root_page_id();
    if (updated_root_id != info.root_page_id) {
        info.root_page_id = updated_root_id;
    }

    return Status::OK();
}

Status IndexManager::drop_index(const std::string& index_name) {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Status::Error(StatusCode::RecordNotFound, "Index not found: " + index_name);
    }

    index_catalog_.erase(it);
    return Status::OK();
}

Result<IndexInfo> IndexManager::get_index_info(const std::string& index_name) const {
    auto it = index_catalog_.find(index_name);
    if (it == index_catalog_.end()) {
        return Result<IndexInfo>(Status::Error(StatusCode::RecordNotFound, 
                                               "Index not found: " + index_name));
    }

    return Result<IndexInfo>(it->second);
}