#include "logger/AccessLogger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

// Конструктор: открываем файл в режиме дозаписи (append)
AccessLogger::AccessLogger(const std::string& path) : file_(path, std::ios::app) {}

// Вспомогательный метод для красивого перевода std::chrono во временную строку
static std::string formatTimePoint(std::chrono::steady_clock::time_point tp) {
    // steady_clock нельзя напрямую перевести в календарное время,
    // поэтому используем системные часы для вычисления текущей календарной точки
    auto system_now = std::chrono::system_clock::now();
    auto steady_now = std::chrono::steady_clock::now();
    auto system_tp = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(
            system_now.time_since_epoch() + (tp - steady_now)
        )
    );

    std::time_t time = std::chrono::system_clock::to_time_t(system_tp);
    std::tm* local_time = std::localtime(&time);
    
    // Достаем миллисекунды для максимальной точности
    auto duration = system_tp.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y.%m.%d-%H:%M:%S", local_time);
    
    return std::string(buf) + "." + std::to_string(millis);
}

// Главный метод логирования согласно ТЗ
void AccessLogger::append(const LogRecord& record) {
    if (!file_.is_open()) return;

    // Вычисляем точную длительность выполнения запроса в микросекундах
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(record.end_time - record.start_time).count();

    // Переводим статус в читаемую строку
    std::string status_str;
    switch (record.status_code) {
        case LogStatus::OK:              status_str = "OK"; break;
        case LogStatus::SYNTAX_ERROR:    status_str = "SYNTAX_ERROR"; break;
        case LogStatus::EXECUTION_ERROR: status_str = "EXECUTION_ERROR"; break;
        case LogStatus::INTERNAL_ERROR:  status_str = "INTERNAL_ERROR"; break;
    }

    // Записываем данные в файл строго по ТЗ: 
    // Календарный старт, календарный финиш, длительность, сессия, обработчик, статус и сам SQL-текст
    file_ << "[" << formatTimePoint(record.start_time) << " -> " << formatTimePoint(record.end_time) << "]"
          << " | Latency: " << duration << " us"
          << " | ClientID: " << record.session_id 
          << " | HandlerID: " << record.handler_id 
          << " | Status: " << status_str 
          << " | Query: " << record.query 
          << std::endl; // Делает \n и принудительный flush() на диск
}
