#include "metrics/MetricsCollector.h"
#include <sstream>
#include <algorithm>

void MetricsCollector::recordQuery(int /*query_type*/,
                                   std::chrono::microseconds latency,
                                   bool success) {
    std::lock_guard<std::mutex> lock(mtx_);
    requests_.push_back({
        std::chrono::steady_clock::now(),
        static_cast<long long>(latency.count()),
        !success
    });
}

void MetricsCollector::cleanupOldLocked(std::chrono::steady_clock::time_point now) {
    const auto cutoff = now - std::chrono::seconds(WINDOW_SECONDS);
    requests_.erase(
        std::remove_if(requests_.begin(), requests_.end(),
            [cutoff](const RequestMetric& m) { return m.timestamp < cutoff; }),
        requests_.end());
}

std::string MetricsCollector::getStats() {
    std::lock_guard<std::mutex> lock(mtx_);
    const auto now = std::chrono::steady_clock::now();
    cleanupOldLocked(now);

    // --- Метрика 1: current RPS — за последнюю 1 секунду ---
    int current_rps = 0;

    // --- Метрика 2: средний и максимальный RPS за 10 минут ---
    // Раскладываем по 600 бакетам: bucket[i] = кол-во запросов,
    // возраст которых в секундах лежит в [i, i+1).
    std::vector<int> rps_buckets(WINDOW_SECONDS, 0);
    long long total_in_window = 0;
    int max_rps = 0;

    // --- Метрика 3: средняя latency за 10 секунд ---
    long long latency_sum_10s = 0;
    int latency_count_10s = 0;

    // --- Метрика 4: Error Rate за последнюю минуту ---
    int errors_last_minute = 0;
    int requests_last_minute = 0;

    for (const auto& m : requests_) {
        const auto age_s = std::chrono::duration_cast<std::chrono::seconds>(
            now - m.timestamp).count();

        if (age_s < 1) {
            current_rps++;
        }

        if (age_s < WINDOW_SECONDS) {
            total_in_window++;
            if (age_s >= 0 && age_s < WINDOW_SECONDS) {
                rps_buckets[static_cast<size_t>(age_s)]++;
            }
        }

        if (age_s < 10) {
            latency_sum_10s += m.latency_us;
            latency_count_10s++;
        }

        if (age_s < 60) {
            requests_last_minute++;
            if (m.is_error) errors_last_minute++;
        }
    }

    for (int v : rps_buckets) {
        if (v > max_rps) max_rps = v;
    }

    const double avg_rps_10min =
        static_cast<double>(total_in_window) / WINDOW_SECONDS;

    const double avg_latency_10s_us = (latency_count_10s > 0)
        ? static_cast<double>(latency_sum_10s) / latency_count_10s
        : 0.0;

    const double error_rate =
        (requests_last_minute > 0)
            ? static_cast<double>(errors_last_minute) * 100.0 / requests_last_minute
            : 0.0;

    std::ostringstream ss;
    ss << "\n=================== DBMS TELEMETRY REPORT ===================\n"
       << "  Current RPS (1s window):        " << current_rps << " req/sec\n"
       << "  Average RPS (10 min window):    " << avg_rps_10min << " req/sec\n"
       << "  Max RPS (10 min window):        " << max_rps << " req/sec\n"
       << "  Avg latency (10s window):       " << avg_latency_10s_us << " us\n"
       << "  Error rate (1 min window):      " << error_rate << " %\n"
       << "  Requests in 10 min:             " << total_in_window << "\n"
       << "=============================================================\n";
    return ss.str();
}