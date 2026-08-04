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
FetchContent_MakeAvailable(googletest)

# Подключение nlohmann_json
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Поиск Flex и Bison в системе
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)