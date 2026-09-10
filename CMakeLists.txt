cmake_minimum_required(VERSION 3.14)
project(dbms_coursework CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключение GoogleTest через FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Тесты используют только gtest; сборка gmock не нужна (и ломается на новых clang)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Подключение nlohmann_json
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Поиск Flex и Bison в системе.
#
# Грамматика src/parser/parser.y использует %language, %define api.value.type
# и api.parser.class — это возможности Bison 3.0 и новее. В macOS штатно
# установлен Bison 2.3, который на них падает с "invalid directive: `%language'".
# Homebrew ставит Bison 3.x в keg-only каталог (вне PATH), поэтому на macOS
# подсказываем CMake, где его искать. На Linux блок пропускается.
if(APPLE AND NOT BISON_EXECUTABLE)
    foreach(_brew_bison /opt/homebrew/opt/bison/bin/bison /usr/local/opt/bison/bin/bison)
        if(EXISTS ${_brew_bison})
            set(BISON_EXECUTABLE ${_brew_bison} CACHE FILEPATH "Bison 3.x из Homebrew" FORCE)
            message(STATUS "Используется Bison из Homebrew: ${_brew_bison}")
            break()
        endif()
    endforeach()
endif()

find_package(FLEX REQUIRED)
find_package(BISON 3.0 REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)