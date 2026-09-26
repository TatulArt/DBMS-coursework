#pragma once
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>
#include "metrics/MetricsCollector.h"

class MetricsReporter {
public:
    explicit MetricsReporter(std::chrono::seconds interval) : interval_(interval), is_running_(false) {}
    
    ~MetricsReporter() {
        stop();
    }

    // Метод старта фонового демона (вызывается в DBMSEngine)
    void start(std::function<void(const std::string&)> callback) {
        MetricsCollector::instance().setReportIntervalSeconds(
            static_cast<int>(interval_.count()));
        is_running_ = true;
        worker_thread_ = std::thread([this, callback]() {
            while (is_running_) {
                std::this_thread::sleep_for(interval_);
                if (!is_running_) break;
                callback(MetricsCollector::instance().getStats());
            }
        });
}

    void stop() {
        if (is_running_) {
            is_running_ = false;
            if (worker_thread_.joinable()) {
                worker_thread_.join(); // Мягко дожидаемся завершения потока при выходе из СУБД
            }
        }
    }

private:
    std::chrono::seconds interval_;
    std::atomic<bool> is_running_;
    std::thread worker_thread_;
};
