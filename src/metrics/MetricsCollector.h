#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector inst;
        return inst;
    }

    // Вызывается из DBMSEngine после каждого запроса
    void recordQuery(int query_type,
                     std::chrono::microseconds latency,
                     bool success);

    // Вызывается фоновым потоком MetricsReporter
    std::string getStats();

    // Период вывода отчёта (для расчёта RPS в окне 1 сек)
    void setReportIntervalSeconds(int seconds) { report_interval_s_ = seconds; }

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;

    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    struct RequestMetric {
        std::chrono::steady_clock::time_point timestamp;
        long long latency_us;
        bool is_error;
    };

    std::mutex mtx_;
    std::vector<RequestMetric> requests_;

    int report_interval_s_{5};

    static constexpr int WINDOW_SECONDS = 600;  // 10 минут

    void cleanupOldLocked(std::chrono::steady_clock::time_point now);
};