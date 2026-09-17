#include "metrics/MetricsCollector.h"
#include <iostream>
#include <sstream>
#include <atomic>
#include <vector>
#include <numeric>

// Внутренние потокобезопасные счетчики для сбора телеметрии
static std::atomic<uint32_t> total_queries{0};
static std::atomic<uint32_t> error_queries{0};
static std::atomic<uint64_t> total_latency_us{0};

// Метод, который вызывается в конце каждого запроса в DBMS_Engine.cpp
void MetricsCollector::recordQuery(int query_type, std::chrono::microseconds latency, bool success) {
    (void)query_type; // Избегаем варнинга о неиспользованном параметре
    total_queries++;
    total_latency_us += latency.count();
    if (!success) {
        error_queries++;
    }
}

// Этот метод дергается фоновым потоком MetricsReporter каждые 10 секунд!
std::string MetricsCollector::getStats() {
    uint32_t queries = total_queries.exchange(0);
    uint32_t errors = error_queries.exchange(0);
    uint64_t latency = total_latency_us.exchange(0);

    // Вычисляем текущий RPS за эти 10 секунд
    double rps = queries / 10.0;
    
    // Вычисляем среднее время обработки запроса в микросекундах
    uint64_t avg_latency = (queries > 0) ? (latency / queries) : 0;
    
    // Вычисляем Error Rate (процент ошибок)
    double error_rate = (queries > 0) ? (static_cast<double>(errors) / queries * 100.0) : 0.0;

    std::stringstream ss;
    ss << "\n=================== DBMS TELEMETRY REPORT ===================\n"
       << "  Current RPS: " << rps << " req/sec\n"
       << "  Average Latency (last 10 sec): " << avg_latency << " us\n"
       << "  Error Rate (last minute window): " << error_rate << "%\n"
       << "=============================================================\n";
    
    return ss.str();
}
