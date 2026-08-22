#ifndef DBMS_PAIN_INDEX_MANAGER_H
#define DBMS_PAIN_INDEX_MANAGER_H

#include <string>
#include <vector>
#include <stdexcept>
#include "types.h"

// Это stub

class IndexManager {
private:
    std::string filePath_; // Имитируем, что у нас есть файл

public:
    // Конструктор, который принимает путь к файлу индекса (как в коде друга)
    explicit IndexManager(const std::string& filePath) : filePath_(filePath) {
        // В заглушке ничего не открываем, просто сохраняем путь
    }

    // Вставка ключа и ID записи в индекс
    void insertKey(const Value& key, RecordID rid) {
        // Заглушка: просто ничего не делаем, но и не падаем
        // В реальности здесь был бы вызов tree_->insert(key, rid);
        // Если ключ NULL — кидаем ошибку, чтобы соблюсти контракт
        if (val::isNull(key)) {
            throw std::runtime_error("INDEXED column cannot be NULL");
        }
    }

    // Поиск записи по ключу
    RecordID findKey(const Value& key) {
        // Заглушка: всегда кидаем ошибку "не найдено"
        // В реальности здесь был бы вызов tree_->search(key);
        throw std::runtime_error("Index not found or not implemented (Stub)");
    }

    // Удаление ключа из индекса
    void removeKey(const Value& key) {
        // Заглушка: ничего не делаем
    }

    std::vector<RecordID> rangeQuery(const Value& low, const Value& high) {
        return {}; 
    }
};

#endif // DBMS_PAIN_INDEX_MANAGER_H