#pragma once
#include <string>
#include <vector>

class SystemCatalog {
public:
    explicit SystemCatalog(const std::string& dataDir);
    
    void addDatabase(const std::string& name);
    void removeDatabase(const std::string& name);
    bool hasDatabase(const std::string& name) const;
    std::vector<std::string> listDatabases() const;

private:
    std::string dataDir_;
    std::vector<std::string> databases_;
};
