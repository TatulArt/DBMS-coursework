#include "page_manager.h"
#include <iostream>
#include <cstring> // Для std::memset
#include <cstdio>  // Для std::fopen / std::fclose

PageManager::PageManager(const std::string& file_path)
    : file_path_(file_path) {}

PageManager::~PageManager() {
    close();
}

Status PageManager::create_database(const std::string& db_path) {
    // Путь к файлу задан в самом PageManager; параметр оставлен для совместимости
    // с существующим API и должен совпадать с file_path_.
    if (!db_path.empty() && db_path != file_path_) {
        return Status::Error(StatusCode::InvalidArgument,
                             "create_database path '" + db_path +
                             "' does not match PageManager path '" + file_path_ + "'");
    }

    Status st = open();
    if (!st.ok()) {
        return st;
    }

    // Если 0-я страница уже существует и содержит корректные метаданные,
    // повторная инициализация затёрла бы существующую базу.
    if (num_pages_ > 0) {
        Page existing;
        if (read_page(METADATA_PAGE_ID, existing).ok()) {
            const auto* meta = reinterpret_cast<const DatabaseMetadata*>(existing.data);
            if (meta->magic_number == DB_MAGIC_NUMBER) {
                return Status::OK(); // База уже инициализирована
            }
        }
    }

    // Формируем 0-ю страницу метаданных.
    // ВАЖНО: раньше здесь вызывался allocate_page(), который перезаписывал
    // meta_id значением num_pages_ и мог «увести» метаданные с 0-й страницы.
    Page meta_page;
    meta_page.clear();
    meta_page.id = METADATA_PAGE_ID;

    auto* meta = reinterpret_cast<DatabaseMetadata*>(meta_page.data);
    meta->magic_number = DB_MAGIC_NUMBER;
    meta->root_page_id = INVALID_PAGE_ID;
    meta->index_catalog_page_id = INVALID_PAGE_ID;

    return write_page(METADATA_PAGE_ID, meta_page);
}

Result<DatabaseMetadata> PageManager::read_metadata() const {
    Page meta_page;
    Status st = read_page(METADATA_PAGE_ID, meta_page);
    if (!st.ok()) {
        return Result<DatabaseMetadata>(st);
    }

    DatabaseMetadata meta{};
    std::memcpy(&meta, meta_page.data, sizeof(DatabaseMetadata));
    if (meta.magic_number != DB_MAGIC_NUMBER) {
        return Result<DatabaseMetadata>(
            Status::Error(StatusCode::CorruptedData, "Invalid database magic number"));
    }
    return Result<DatabaseMetadata>(meta);
}

Status PageManager::write_metadata(const DatabaseMetadata& meta) {
    Page meta_page;
    Status st = read_page(METADATA_PAGE_ID, meta_page);
    if (!st.ok()) {
        // Страницы ещё нет — создаём её с нуля
        meta_page.clear();
        meta_page.id = METADATA_PAGE_ID;
    }

    std::memcpy(meta_page.data, &meta, sizeof(DatabaseMetadata));
    return write_page(METADATA_PAGE_ID, meta_page);
}

Status PageManager::update_root_page_id(PageId new_root_id) {
    auto meta_res = read_metadata();
    if (!meta_res.ok()) {
        return meta_res.status();
    }

    DatabaseMetadata meta = meta_res.value();
    meta.root_page_id = new_root_id;
    return write_metadata(meta);
}

Status PageManager::open() {
    if (file_stream_.is_open()) {
        return Status::OK(); // Повторный open() не должен ломать состояние
    }

    // Открываем файл для чтения и записи в бинарном режиме
    file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);

    // Если файла нет — создаём его пустым и открываем повторно.
    //
    // ВАЖНО: файл создаётся средствами C stdio, а не через std::ofstream.
    // Если пустой файл создать потоком C++ и затем переоткрыть его как fstream,
    // все последующие записи в него становятся примерно в 2000 раз медленнее
    // (замерено: 5.5 мс против 0.003 мс на страницу). Из-за этого полный прогон
    // тестов занимал ~29 секунд вместо долей секунды.
    if (!file_stream_.is_open()) {
        file_stream_.clear();

        std::FILE* created = std::fopen(file_path_.c_str(), "wb");
        if (created == nullptr) {
            return Status::Error(StatusCode::IOError, "Failed to create database file: " + file_path_);
        }
        std::fclose(created);

        // Повторно открываем в режиме in/out
        file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!file_stream_.is_open()) {
        return Status::Error(StatusCode::IOError, "Failed to open database file: " + file_path_);
    }

    update_num_pages();
    return Status::OK();
}

Status PageManager::flush() {
    if (!file_stream_.is_open()) {
        return Status::OK();
    }
    file_stream_.flush();
    if (!file_stream_) {
        return Status::Error(StatusCode::IOError, "Failed to flush database file: " + file_path_);
    }
    return Status::OK();
}

Status PageManager::close() {
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
        file_stream_.clear();
    }
    return Status::OK();
}

void PageManager::update_num_pages() {
    if (!file_stream_.is_open()) {
        num_pages_ = 0;
        return;
    }
    file_stream_.seekg(0, std::ios::end);
    std::streamoff file_size = file_stream_.tellg();
    if (file_size < 0) {
        num_pages_ = 0;
        return;
    }
    num_pages_ = static_cast<uint32_t>(file_size / PAGE_SIZE);
}

Status PageManager::read_page(PageId page_id, Page& page_out) const {
    if (!file_stream_.is_open()) {
        return Status::Error(StatusCode::IOError, "Database file is not open");
    }

    if (page_id >= num_pages_) {
        return Status::Error(StatusCode::InvalidArgument, 
                             "PageId " + std::to_string(page_id) + " out of bounds (total pages: " + std::to_string(num_pages_) + ")");
    }

    // Вычисляем смещение в байтах: offset = page_id * 4096
    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    file_stream_.seekg(offset, std::ios::beg);

    file_stream_.read(reinterpret_cast<char*>(page_out.data), PAGE_SIZE);

    if (!file_stream_) {
        file_stream_.clear();
        return Status::Error(StatusCode::IOError, "Failed to read page " + std::to_string(page_id) + " from disk");
    }

    page_out.id = page_id;
    page_out.is_dirty = false;
    return Status::OK();
}

Status PageManager::write_page(PageId page_id, const Page& page) {
    if (!file_stream_.is_open()) {
        return Status::Error(StatusCode::IOError, "Database file is not open");
    }
    if (page_id == INVALID_PAGE_ID) {
        return Status::Error(StatusCode::InvalidArgument, "Attempt to write INVALID_PAGE_ID");
    }

    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    file_stream_.seekp(offset, std::ios::beg);

    file_stream_.write(reinterpret_cast<const char*>(page.data), PAGE_SIZE);

    if (!file_stream_) {
        file_stream_.clear();
        return Status::Error(StatusCode::IOError, "Failed to write page " + std::to_string(page_id) + " to disk");
    }

    if (sync_on_write_) {
        file_stream_.flush();
        if (!file_stream_) {
            file_stream_.clear();
            return Status::Error(StatusCode::IOError, "Failed to flush page " + std::to_string(page_id));
        }
    }

    // Счётчик страниц поддерживается инкрементально.
    // Раньше здесь вызывался update_num_pages(), который на каждой записи
    // делал seek в конец файла — это давало заметную просадку по скорости.
    if (page_id >= num_pages_) {
        num_pages_ = page_id + 1;
    }

    return Status::OK();
}

Status PageManager::allocate_page(PageId& new_page_id, Page& page_out) {
    if (!file_stream_.is_open()) {
        return Status::Error(StatusCode::IOError, "Database file is not open");
    }

    new_page_id = num_pages_;
    page_out.clear();
    page_out.id = new_page_id;

    // Пишем пустую страницу в конец файла
    return write_page(new_page_id, page_out);
}
