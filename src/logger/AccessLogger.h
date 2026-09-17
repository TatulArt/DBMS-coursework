#pragma once
#include <string>
#include <fstream>
#include "logger/LogRecord.h"

class AccessLogger {
public:
    explicit AccessLogger(const std::string& path);
    void append(const LogRecord& record);

private:
    std::ofstream file_;
};
