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
        is_running_ = true;
        worker_thread_ = std::thread([this, callback]() {
            while (is_running_) {
                // Фоновый поток засыпает строго на 10 секунд по ТЗ!
                std::this_thread::sleep_for(interval_);
                
                if (!is_running_) break;

                // Просыпаемся, забираем живые цифры из коллектора и отдаем в std::cout энджина
                std::string stats = MetricsCollector::instance().getStats();
                callback(stats);
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
