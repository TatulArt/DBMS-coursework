#pragma once
#include <string>
#include <fstream>
#include "logger/LogRecord.h"

class AccessLogger {
public:
    explicit AccessLogger(const std::string& path) : file_(path, std::ios::app) {}
    void append(const LogRecord& record) {}
private:
    std::ofstream file_;
};
