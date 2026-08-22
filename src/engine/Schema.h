#ifndef DBMS_PAIN_SCHEMA_H
#define DBMS_PAIN_SCHEMA_H

#include <string>
#include <vector>
#include "types.h"

// ============================================================
// СХЕМА ТАБЛИЦЫ
// ============================================================
struct Schema {
    std::string tableName;
    std::vector<ColumnDef> columns;

    // Найти индекс колонки по имени (-1 если не найдена)
    int columnIndex(const std::string& name) const {
        for (int i = 0; i < static_cast<int>(columns.size()); i++) {
            if (columns[i].name == name) return i;
        }
        return -1;
    }

    // Найти индекс первой INDEXED колонки (-1 если нет)
    int indexedColumn() const {
        for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
            if (columns[i].indexed) return i;
        }
        return -1;
    }
};

#endif // DBMS_PAIN_SCHEMA_H