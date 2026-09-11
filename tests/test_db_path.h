#pragma once

#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

// ============================================================================
// Уникальное имя файла БД для текущего теста.
//
// Раньше все тесты одной фикстуры делили одно имя файла в текущем каталоге.
// gtest_discover_tests регистрирует каждый тест отдельной записью ctest,
// поэтому при запуске `ctest -j` они работают параллельными процессами в одном
// каталоге и затирают файлы друг друга. Имя теста и PID процесса делают путь
// уникальным.
// ============================================================================
inline std::string unique_db_file(const std::string& stem) {
    std::string name = stem;

    if (const auto* info = ::testing::UnitTest::GetInstance()->current_test_info()) {
        name += "_";
        name += info->test_suite_name();
        name += "_";
        name += info->name();
    }

    name += "_" + std::to_string(static_cast<long>(::getpid())) + ".db";
    return name;
}
