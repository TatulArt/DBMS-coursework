#include "catalog/SystemCatalog.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

SystemCatalog::SystemCatalog(const std::string& dataDir) {
    dataDir_ = dataDir;
    
    if (std::filesystem::exists(dataDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dataDir)) {
            if (entry.is_directory()) {
                databases_.push_back(entry.path().filename().string());
            }
        }
    }
}

void SystemCatalog::addDatabase(const std::string& name) {
    std::filesystem::create_directories(dataDir_ + "/" + name);
    databases_.push_back(name);
}

void SystemCatalog::removeDatabase(const std::string& name) {
    std::filesystem::remove_all(dataDir_ + "/" + name);
    // Явно указываем std::remove из <algorithm>
    databases_.erase(std::remove(databases_.begin(), databases_.end(), name), databases_.end());
}

bool SystemCatalog::hasDatabase(const std::string& name) const {
    for (const auto& db : databases_) {
        if (db == name) return true;
    }
    return false;
}

std::vector<std::string> SystemCatalog::listDatabases() const {
    return databases_;
}
