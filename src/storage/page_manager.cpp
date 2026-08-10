#include "page_manager.h"
#include <iostream>

PageManager::PageManager(const std::string& file_path)
    : file_path_(file_path) {}

PageManager::~PageManager() {
    close();
}

Status PageManager::open() {
    // Открываем файл для чтения и записи в бинарном режиме
    file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);

    // Если файл не существует, создаём его
    if (!file_stream_.is_open()) {
        file_stream_.clear();
        file_stream_.open(file_path_, std::ios::out | std::ios::binary);
        file_stream_.close();
        
        // Повторно открываем в режиме in/out
        file_stream_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!file_stream_.is_open()) {
        return Status::Error(StatusCode::IOError, "Failed to open database file: " + file_path_);
    }

    update_num_pages();
    return Status::OK();
}

Status PageManager::close() {
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
    return Status::OK();
}

void PageManager::update_num_pages() {
    if (!file_stream_.is_open()) {
        num_pages_ = 0;
        return;
    }
    file_stream_.seekp(0, std::ios::end);
    std::streamoff file_size = file_stream_.tellp();
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

    std::streamoff offset = static_cast<std::streamoff>(page_id) * PAGE_SIZE;
    file_stream_.seekp(offset, std::ios::beg);

    file_stream_.write(reinterpret_cast<const char*>(page.data), PAGE_SIZE);
    file_stream_.flush(); // Гарантируем сброс на диск

    if (!file_stream_) {
        return Status::Error(StatusCode::IOError, "Failed to write page " + std::to_string(page_id) + " to disk");
    }

    update_num_pages();
    return Status::OK();
}

Status PageManager::allocate_page(PageId& new_page_id, Page& page_out) {
    new_page_id = num_pages_;
    page_out.clear();
    page_out.id = new_page_id;

    // Пишем пустую страницу в конец файла
    Status status = write_page(new_page_id, page_out);
    if (!status.ok()) {
        return status;
    }

    return Status::OK();
}