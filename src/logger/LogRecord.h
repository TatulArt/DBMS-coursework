#pragma once
#include <string>
#include <chrono>

enum class LogStatus { OK, SYNTAX_ERROR, EXECUTION_ERROR, INTERNAL_ERROR };

struct LogRecord {
    std::string query;
    std::string session_id;
    std::string handler_id;
    LogStatus status_code = LogStatus::OK;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;

    LogRecord(const std::string& q, const std::string& sid, const std::string& hid)
        : query(q), session_id(sid), handler_id(hid) {
        start_time = std::chrono::steady_clock::now();
    }
    void finalize() { end_time = std::chrono::steady_clock::now(); }
};
