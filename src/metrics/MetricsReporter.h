#pragma once
#include <chrono>
#include <functional>
#include <string>

class MetricsReporter {
public:
    explicit MetricsReporter(std::chrono::seconds interval) {}
    void start(std::function<void(const std::string&)> callback) {}
};
