#pragma once
#include <chrono>
#include <string>
#include <atomic>

class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector inst;
        return inst; // Поправил опечатку inst
    }

    void recordQuery(int query_type, std::chrono::microseconds latency, bool success);
    std::string getStats();

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;

    std::atomic<uint32_t> total_queries_{0};
    std::atomic<uint32_t> error_queries_{0};
    std::atomic<uint64_t> total_latency_us_{0};
    std::atomic<uint32_t> max_queries_in_window_{0};
};
