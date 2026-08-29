#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "../types.h"
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>

// Фиксированный размер страницы в СУБД (4096 байт)
constexpr size_t PAGE_SIZE = 4096;
using PageId = uint32_t;

// Структура страницы в оперативной памяти (4KB сырых байт)
struct Page {
    PageId id{0};
    uint8_t data[PAGE_SIZE]{0};
    bool is_dirty{false}; // Флаг: изменялась ли страница в RAM (нужно ли сбрасывать на диск)

    void clear() {
        id = 0;
        std::fill(std::begin(data), std::end(data), 0);
        is_dirty = false;
    }
};

class PageManager {
private:
    std::string file_path_;
    mutable std::fstream file_stream_;
    uint32_t num_pages_{0};

public:
    explicit PageManager(const std::string& file_path);
    ~PageManager();

    // Открытие/создание файла таблицы
    Status open();
    Status close();

    // Основные I/O операции со страницами
    Status read_page(PageId page_id, Page& page_out) const;
    Status write_page(PageId page_id, const Page& page);

    // Выделение новой страницы в конце файла
    Status allocate_page(PageId& new_page_id, Page& page_out);

    
    // Создает новый файл базы данных и инициализирует 0-ю страницу метаданных.
    // @param db_path Путь к создаваемому файлу базы данных
    Status create_database(const std::string& db_path);

    // Вспомогательный метод для сброса/обновления root_page_id в метаданных
    Status update_root_page_id(PageId new_root_id);


    // Вспомогательные методы
    uint32_t get_num_pages() const { return num_pages_; }
    const std::string& get_file_path() const { return file_path_; }

private:
    void update_num_pages();
};

#endif // PAGE_MANAGER_H