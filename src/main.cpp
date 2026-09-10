#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "engine/DBMS_Engine.h"

// Автоматическое создание папки для файлов баз данных, если её нет в системе
void ensureDirectoriesExist() {
    if (!std::filesystem::exists("data")) {
        std::filesystem::create_directories("data");
    }
}

void runInteractiveMode(dbms::DBMSEngine& engine) {
    std::cout << "=================================================================\n";
    std::cout << " СУБД запущенна в интерактивном режиме (стандарт C++17)\n";
    std::cout << " Запросы должны строго завершаться знаком ';'\n";
    std::cout << " Выход из программы: Ctrl + D или Ctrl + C\n";
    std::cout << "=================================================================\n";
    
    std::string query_buffer;
    std::string line;

    while (true) {
        if (query_buffer.empty()) std::cout << "dbms> ";
        else std::cout << "    > ";

        if (!std::getline(std::cin, line)) break;
        query_buffer += line + "\n";

        // Накопление многострочного ввода до точки с запятой (Требование ТЗ!)
        size_t semicolon_pos = query_buffer.find(';');
        if (semicolon_pos != std::string::npos) {
            std::string single_query = query_buffer.substr(0, semicolon_pos + 1);
            query_buffer = query_buffer.substr(semicolon_pos + 1);

            // Передаем запрос в координатор СУБД (он сам залогирует и выполнит)
            engine.processQueryBuffer(single_query, "interactive_session");
        }
    }
}

void runBatchMode(const std::string& filepath, dbms::DBMSEngine& engine) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть пакетный файл: " << filepath << "\n";
        return;
    }

    std::string query_buffer;
    std::string line;
    int line_num = 1;

    // Последовательное чтение и выполнение команд из файла сценария по ТЗ
    while (std::getline(file, line)) {
        query_buffer += line + " ";
        size_t semicolon_pos = query_buffer.find(';');
        if (semicolon_pos != std::string::npos) {
            std::string single_query = query_buffer.substr(0, semicolon_pos + 1);
            query_buffer = query_buffer.substr(semicolon_pos + 1);

            engine.processQueryBuffer(single_query, "batch_line_" + std::to_string(line_num));
        }
        line_num++;
    }
}

int main(int argc, char* argv[]) {
    try {
        ensureDirectoriesExist();
        
        // Инстанцируем движок СУБД (он поднимет логгеры, метрики и твой парсер)
        dbms::DBMSEngine engine("data/access.log");

        // Маршрутизация режимов работы из аргументов командной строки по ТЗ
        if (argc == 1) {
            runInteractiveMode(engine);
        } 
        else if (argc == 2) {
            runBatchMode(argv[1], engine);
        } 
        else {
            std::cerr << "Использование:\n  Терминал:  ./dbms_cli\n  Сценарий:  ./dbms_cli script.txt\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Критический сбой СУБД: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
