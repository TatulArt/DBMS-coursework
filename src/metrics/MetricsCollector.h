#pragma once
#include <chrono>

// Не определяем QueryType здесь — он будет из DBMS_Engine.h
// Просто используем int для универсальности

class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector inst;
        return inst;
    }
    
    void recordQuery(int type, std::chrono::microseconds latency, bool success) {
        // Заглушка
    }
};
