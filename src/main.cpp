#include <iostream>
#include <string>
#include <fstream>
#include "engine/DBMS_Engine.h"

int main(int argc, char** argv) {
    dbms::DBMSEngine engine;

    if (argc == 1) {
        // Интерактивный режим
        std::string query;
        std::cout << "DBMS> ";
        while (std::getline(std::cin, query)) {
            if (query == "exit" || query == "exit;") break;
            
            engine.processQueryBuffer(query, "interactive");
            std::cout << "DBMS> ";
        }
    } else {
        // Пакетный режим — читаем из файла
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << argv[1] << std::endl;
            return 1;
        }
        std::string line;
        std::string buffer;
        while (std::getline(file, line)) {
            buffer += line + "\n";
            if (line.find(';') != std::string::npos) {
                engine.processQueryBuffer(buffer, "batch");
                buffer.clear();
            }
        }
    }

    return 0;
}
