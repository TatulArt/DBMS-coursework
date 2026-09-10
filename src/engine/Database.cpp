#include "Database.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "utils/Error.h"

// Вспомогательная функция для создания папки
static void ensureDirectoryExists(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

Database::Database(const std::string& db_path, const std::string& name) :
    name_(name), dbPath_(db_path) {
    // Создаём папку для базы данных, если её нет
    ensureDirectoryExists(dbPath_);
    loadTables();
}

void Database::createTable(const Schema& schema) {
    if (hasTable(schema.tableName))
        throw SemanticError("Table already exists: " + schema.tableName);

    tables_[schema.tableName] = Table::create(dbPath_, schema);
    saveSchema();
}

void Database::dropTable(const std::string& name) {
    if (!hasTable(name))
        throw SemanticError("Table does not exist: " + name);

    tables_.erase(name);
    Table::drop(dbPath_, name);
    saveSchema();
}

Table& Database::getTable(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end())
        throw SemanticError("Table does not exist: " + name);
    return *it->second;
}

bool Database::hasTable(const std::string& name) const {
    return tables_.find(name) != tables_.end();
}

void Database::loadTables() {
    std::ifstream f(dbPath_ + "/schema.dat");
    if (!f.is_open())
        return; // новая БД — таблиц ещё нет

    std::string line;
    Schema current;
    bool inTable = false;

    while (std::getline(f, line)) {
        if (line.empty())
            continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "TABLE") {
            if (inTable && !current.tableName.empty()) {
                tables_[current.tableName] = std::make_unique<Table>(dbPath_, current);
            }
            current = Schema{};
            ss >> current.tableName;
            inTable = true;
        } else {
            // колонка: name TYPE [INDEXED] [NOT NULL] [DEFAULT value]
            ColumnDef col;
            col.name = token;

            std::string typeStr;
            ss >> typeStr;
            col.type = (typeStr == "INT") ? ColumnType::Int : ColumnType::String;

            std::string mod;
            while (ss >> mod) {
                if (mod == "INDEXED")
                    col.is_indexed = true;
                else if (mod == "NOT NULL")
                    col.is_nullable = false;
                else if (mod == "DEFAULT") {
                    std::string val;
                    ss >> val;
                    if (col.type == ColumnType::Int) {
                        col.default_value = Value(std::stoi(val));
                    } else {
                        // убираем кавычки если есть
                        if (!val.empty() && val.front() == '"')
                            val = val.substr(1, val.size() - 2);
                        col.default_value = Value(val);
                    }
                }
            }
            current.columns.push_back(col);
        }
    }

    // последняя таблица
    if (inTable && !current.tableName.empty())
        tables_[current.tableName] = std::make_unique<Table>(dbPath_, current);
}

void Database::saveSchema() {
    // Убеждаемся, что папка существует перед записью
    ensureDirectoryExists(dbPath_);
    
    std::ofstream f(dbPath_ + "/schema.dat", std::ios::trunc);
    if (!f.is_open())
        throw StorageError("Cannot write schema.dat for: " + name_);

    for (const auto& [name, tbl]: tables_) {
        f << "TABLE " << name << "\n";
        for (const auto& col: tbl->schema().columns) {
            f << col.name << " ";
            f << (col.type == ColumnType::Int ? "INT" : "STRING");
            if (col.is_indexed)
                f << " INDEXED";
            if (!col.is_nullable)
                f << " NOT NULL";
            if (!col.default_value.is_null()) {
                f << " DEFAULT ";
                if ((!(col.default_value).is_null() && (col.default_value).get_type() == ColumnType::Int))
                    f << (col.default_value).get_int();
                else
                    f << "\"" << (col.default_value).get_string() << "\"";
            }
            f << "\n";
        }
    }
}
