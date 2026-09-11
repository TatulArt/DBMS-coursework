// ============================================================================
// Интеграционные тесты СУБД через её собственный интерфейс.
//
// Каждый тест запускает настоящий бинарник dbms_cli отдельным процессом и
// подаёт SQL-запросы в его стандартный ввод — ровно так, как это делает
// пользователь в терминале. Проверяется то, что программа напечатала в ответ.
//
// Каждому тесту выделяется свой рабочий каталог: СУБД хранит файлы в ./data
// относительно текущей директории, поэтому тесты не мешают друг другу.
// ============================================================================

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef DBMS_CLI_PATH
#error "DBMS_CLI_PATH не задан: путь к dbms_cli должен приходить из CMake"
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

// Результат одного сеанса работы с СУБД
struct Session {
    std::string output;   // всё, что программа напечатала
    int exit_code{-1};
};

// Запускает dbms_cli в каталоге workdir и отдаёт ему sql в стандартный ввод.
// Возврат из функции происходит после того, как процесс полностью завершился,
// то есть все буферы сброшены и файлы БД закрыты.
Session run_session(const fs::path& workdir, const std::string& sql) {
    static int session_counter = 0;
    const fs::path out_file = workdir / ("session_" + std::to_string(++session_counter) + ".out");

    const std::string command =
        "cd '" + workdir.string() + "' && '" + std::string(DBMS_CLI_PATH) + "'"
        " > '" + out_file.string() + "' 2>&1";

    Session result;

    FILE* pipe = popen(command.c_str(), "w");
    if (pipe == nullptr) {
        ADD_FAILURE() << "Не удалось запустить " << DBMS_CLI_PATH;
        return result;
    }

    std::fwrite(sql.data(), 1, sql.size(), pipe);

    // pclose закрывает stdin (для СУБД это конец ввода) и ждёт завершения
    const int status = pclose(pipe);
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    std::ifstream in(out_file);
    std::stringstream buffer;
    buffer << in.rdbuf();
    result.output = buffer.str();

    return result;
}

// Убирает приглашения "dbms> " и "    > ", которыми СУБД предваряет ответы
std::string strip_prompts(const std::string& raw) {
    std::string cleaned;
    std::istringstream in(raw);
    std::string line;

    while (std::getline(in, line)) {
        for (const std::string& prompt : {std::string("dbms> "), std::string("    > ")}) {
            while (line.rfind(prompt, 0) == 0) {
                line.erase(0, prompt.size());
            }
        }
        cleaned += line;
        cleaned += '\n';
    }
    return cleaned;
}

// Достаёт из вывода все результаты выборок (массивы JSON-объектов).
// СУБД печатает их с отступами: строка "[", строки с объектами, строка "]".
std::vector<json> extract_results(const std::string& raw) {
    std::vector<json> results;
    std::istringstream in(strip_prompts(raw));
    std::string line;
    std::string block;
    bool inside = false;

    while (std::getline(in, line)) {
        if (!inside && line == "[") {
            inside = true;
            block = "[\n";
            continue;
        }
        if (inside) {
            block += line;
            block += '\n';
            if (line == "]") {
                inside = false;
                results.push_back(json::parse(block, nullptr, /*allow_exceptions=*/false));
            }
        }
    }
    return results;
}

// Единственная выборка в выводе — самый частый случай в тестах
json single_result(const Session& session) {
    const std::vector<json> all = extract_results(session.output);
    EXPECT_EQ(all.size(), 1u) << "Ожидалась ровно одна выборка. Вывод:\n" << session.output;
    if (all.size() != 1u) return json::array();
    return all.front();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// Базовый класс: свой каталог на каждый тест
class CliTest : public ::testing::Test {
protected:
    fs::path workdir;

    void SetUp() override {
        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        workdir = fs::current_path() / "cli_it" / (std::string(info->test_suite_name()) + "_" + info->name());
        fs::remove_all(workdir);
        fs::create_directories(workdir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(workdir, ec);
    }

    Session run(const std::string& sql) { return run_session(workdir, sql); }
};

} // namespace

// ----------------------------------------------------------------------------
// 1. Базовый путь: создать базу, таблицу, вставить строки, выбрать их
// ----------------------------------------------------------------------------
TEST_F(CliTest, CreateInsertSelect) {
    Session s = run(R"SQL(
CREATE DATABASE shop;
USE shop;
CREATE TABLE users (id INT INDEXED, name STRING, age INT);
INSERT INTO users (id, name, age) VALUE (1, "Alice", 30);
INSERT INTO users (id, name, age) VALUE (2, "Bob", 25);
SELECT * FROM users;
)SQL");

    EXPECT_EQ(s.exit_code, 0);

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 2u) << s.output;
    EXPECT_EQ(rows[0]["id"], 1);
    EXPECT_EQ(rows[0]["name"], "Alice");
    EXPECT_EQ(rows[0]["age"], 30);
    EXPECT_EQ(rows[1]["name"], "Bob");
}

// ----------------------------------------------------------------------------
// 2. ГЛАВНЫЙ ТЕСТ: данные переживают завершение сеанса.
//
//    Первый процесс создаёт базу и пишет строки, затем полностью завершается.
//    Второй процесс запускается с нуля и обязан увидеть те же данные —
//    значит они действительно оказались на диске, а не в памяти процесса.
// ----------------------------------------------------------------------------
TEST_F(CliTest, DataSurvivesSessionRestart) {
    // --- Сеанс 1: записываем ---
    Session write = run(R"SQL(
CREATE DATABASE warehouse;
USE warehouse;
CREATE TABLE items (id INT INDEXED, title STRING, qty INT);
INSERT INTO items (id, title, qty) VALUE (10, "Болт", 500);
INSERT INTO items (id, title, qty) VALUE (20, "Гайка", 300);
INSERT INTO items (id, title, qty) VALUE (30, "Шайба", 150);
)SQL");
    ASSERT_EQ(write.exit_code, 0) << write.output;
    EXPECT_FALSE(contains(write.output, "Error")) << write.output;

    // Процесс завершился. Файлы БД должны лежать на диске.
    EXPECT_TRUE(fs::exists(workdir / "data"))
        << "СУБД не создала каталог data после первого сеанса";

    // --- Сеанс 2: отдельный запуск программы, только чтение ---
    Session read = run(R"SQL(
USE warehouse;
SELECT * FROM items;
)SQL");
    ASSERT_EQ(read.exit_code, 0) << read.output;

    const json rows = single_result(read);
    ASSERT_EQ(rows.size(), 3u)
        << "После перезапуска прочитано не всё. Вывод второго сеанса:\n" << read.output;

    EXPECT_EQ(rows[0]["id"], 10);
    EXPECT_EQ(rows[0]["title"], "Болт");
    EXPECT_EQ(rows[0]["qty"], 500);
    EXPECT_EQ(rows[2]["title"], "Шайба");

    // --- Сеанс 3: дописываем к уже существующей базе и снова перезапускаемся ---
    Session append = run(R"SQL(
USE warehouse;
INSERT INTO items (id, title, qty) VALUE (40, "Винт", 75);
)SQL");
    ASSERT_EQ(append.exit_code, 0) << append.output;

    Session verify = run(R"SQL(
USE warehouse;
SELECT * FROM items;
)SQL");
    const json after = single_result(verify);
    EXPECT_EQ(after.size(), 4u)
        << "Дозапись в существующую базу не сохранилась:\n" << verify.output;
}

// ----------------------------------------------------------------------------
// 3. Схема таблицы (INDEXED, NOT_NULL, DEFAULT) тоже переживает перезапуск
//    и продолжает действовать в новом процессе
// ----------------------------------------------------------------------------
TEST_F(CliTest, SchemaConstraintsSurviveRestart) {
    Session create = run(R"SQL(
CREATE DATABASE hr;
USE hr;
CREATE TABLE staff (id INT INDEXED, name STRING NOT_NULL, dept STRING DEFAULT "общий отдел");
INSERT INTO staff (id, name) VALUE (1, "Иван");
)SQL");
    ASSERT_EQ(create.exit_code, 0) << create.output;

    // Новый процесс: ограничения должны быть восстановлены из файла схемы
    Session reuse = run(R"SQL(
USE hr;
INSERT INTO staff (id, name) VALUE (1, "Дубликат");
INSERT INTO staff (id) VALUE (2);
INSERT INTO staff (id, name) VALUE (3, "Пётр");
SELECT * FROM staff;
)SQL");
    ASSERT_EQ(reuse.exit_code, 0) << reuse.output;

    // INDEXED остался уникальным
    EXPECT_TRUE(contains(reuse.output, "Duplicate value"))
        << "Уникальность INDEXED не восстановилась после перезапуска:\n" << reuse.output;

    // NOT_NULL остался обязательным
    EXPECT_TRUE(contains(reuse.output, "cannot be NULL"))
        << "NOT_NULL не восстановился после перезапуска:\n" << reuse.output;

    const json rows = single_result(reuse);
    ASSERT_EQ(rows.size(), 2u) << reuse.output;

    // DEFAULT подставился в обоих сеансах, включая строку с пробелом
    EXPECT_EQ(rows[0]["dept"], "общий отдел");
    EXPECT_EQ(rows[1]["dept"], "общий отдел");
}

// ----------------------------------------------------------------------------
// 4. NOT_NULL: строка с пропущенным обязательным полем не попадает в таблицу
// ----------------------------------------------------------------------------
TEST_F(CliTest, NotNullConstraintRejectsInsert) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, name STRING NOT_NULL);
INSERT INTO t (id, name) VALUE (1, "Ок");
INSERT INTO t (id) VALUE (2);
SELECT * FROM t;
)SQL");

    EXPECT_TRUE(contains(s.output, "cannot be NULL")) << s.output;

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 1u) << "Отклонённая строка всё же попала в таблицу:\n" << s.output;
    EXPECT_EQ(rows[0]["id"], 1);
}

// ----------------------------------------------------------------------------
// 5. INDEXED: значения уникальны, повтор отклоняется
// ----------------------------------------------------------------------------
TEST_F(CliTest, IndexedColumnIsUnique) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, name STRING);
INSERT INTO t (id, name) VALUE (1, "Первый");
INSERT INTO t (id, name) VALUE (1, "Второй");
SELECT * FROM t;
)SQL");

    EXPECT_TRUE(contains(s.output, "Duplicate value")) << s.output;

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 1u) << s.output;
    EXPECT_EQ(rows[0]["name"], "Первый");
}

// ----------------------------------------------------------------------------
// 6. DEFAULT: пропущенное значение заменяется значением по умолчанию
// ----------------------------------------------------------------------------
TEST_F(CliTest, DefaultValueIsSubstituted) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, note STRING DEFAULT "прочерк", score INT DEFAULT 42);
INSERT INTO t (id) VALUE (1);
INSERT INTO t (id, note) VALUE (2, "своё");
SELECT * FROM t;
)SQL");

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 2u) << s.output;
    EXPECT_EQ(rows[0]["note"], "прочерк");
    EXPECT_EQ(rows[0]["score"], 42);
    EXPECT_EQ(rows[1]["note"], "своё") << "Явное значение затёрлось значением по умолчанию";
}

// ----------------------------------------------------------------------------
// 7. Операции сравнения в WHERE
// ----------------------------------------------------------------------------
TEST_F(CliTest, WhereComparisonOperators) {
    const std::string prelude = R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE nums (id INT INDEXED, v INT);
INSERT INTO nums (id, v) VALUE (1, 10);
INSERT INTO nums (id, v) VALUE (2, 20);
INSERT INTO nums (id, v) VALUE (3, 30);
)SQL";

    Session s = run(prelude + R"SQL(
SELECT * FROM nums WHERE v == 20;
SELECT * FROM nums WHERE v != 20;
SELECT * FROM nums WHERE v < 30;
SELECT * FROM nums WHERE v > 10;
SELECT * FROM nums WHERE v <= 20;
SELECT * FROM nums WHERE v >= 20;
)SQL");

    const std::vector<json> r = extract_results(s.output);
    ASSERT_EQ(r.size(), 6u) << s.output;
    EXPECT_EQ(r[0].size(), 1u) << "==";
    EXPECT_EQ(r[1].size(), 2u) << "!=";
    EXPECT_EQ(r[2].size(), 2u) << "<";
    EXPECT_EQ(r[3].size(), 2u) << ">";
    EXPECT_EQ(r[4].size(), 2u) << "<=";
    EXPECT_EQ(r[5].size(), 2u) << ">=";
}

// ----------------------------------------------------------------------------
// 8. BETWEEN задаёт полуоткрытый интервал [low, high) — требование задания
// ----------------------------------------------------------------------------
TEST_F(CliTest, BetweenIsHalfOpen) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE nums (id INT INDEXED, v INT);
INSERT INTO nums (id, v) VALUE (1, 25);
INSERT INTO nums (id, v) VALUE (2, 30);
INSERT INTO nums (id, v) VALUE (3, 35);
SELECT * FROM nums WHERE v BETWEEN 25 AND 35;
)SQL");

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 2u)
        << "BETWEEN должен включать нижнюю границу и исключать верхнюю:\n" << s.output;
    EXPECT_EQ(rows[0]["v"], 25);
    EXPECT_EQ(rows[1]["v"], 30);
}

// ----------------------------------------------------------------------------
// 9. LIKE сопоставляет строку с регулярным выражением
// ----------------------------------------------------------------------------
TEST_F(CliTest, LikeMatchesRegex) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, name STRING);
INSERT INTO t (id, name) VALUE (1, "Alice");
INSERT INTO t (id, name) VALUE (2, "Albert");
INSERT INTO t (id, name) VALUE (3, "Bob");
SELECT * FROM t WHERE name LIKE "Al.*";
)SQL");

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 2u) << s.output;
    EXPECT_EQ(rows[0]["name"], "Alice");
    EXPECT_EQ(rows[1]["name"], "Albert");
}

// ----------------------------------------------------------------------------
// 10. UPDATE и DELETE меняют данные, и изменения тоже ложатся на диск
// ----------------------------------------------------------------------------
TEST_F(CliTest, UpdateAndDeletePersist) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, name STRING, v INT);
INSERT INTO t (id, name, v) VALUE (1, "Первый", 10);
INSERT INTO t (id, name, v) VALUE (2, "Второй", 20);
INSERT INTO t (id, name, v) VALUE (3, "Третий", 30);
UPDATE t SET v = 99 WHERE id == 2;
DELETE FROM t WHERE id == 3;
)SQL");
    ASSERT_EQ(s.exit_code, 0) << s.output;

    // Новый процесс: проверяем, что и правка, и удаление сохранились
    Session after = run(R"SQL(
USE d;
SELECT * FROM t;
)SQL");

    const json rows = single_result(after);
    ASSERT_EQ(rows.size(), 2u) << "DELETE не сохранился:\n" << after.output;

    bool found_updated = false;
    for (const json& row : rows) {
        EXPECT_NE(row["id"], 3) << "Удалённая строка вернулась после перезапуска";
        if (row["id"] == 2) {
            found_updated = true;
            EXPECT_EQ(row["v"], 99) << "UPDATE не сохранился";
        }
    }
    EXPECT_TRUE(found_updated) << after.output;
}

// ----------------------------------------------------------------------------
// 11. Выборка отдельных колонок и псевдонимы через AS
// ----------------------------------------------------------------------------
TEST_F(CliTest, SelectProjectionAndAlias) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE t (id INT INDEXED, name STRING, age INT);
INSERT INTO t (id, name, age) VALUE (1, "Alice", 30);
SELECT name AS who, age FROM t;
)SQL");

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 1u) << s.output;
    EXPECT_TRUE(rows[0].contains("who")) << "Псевдоним AS не применён: " << rows[0].dump();
    EXPECT_EQ(rows[0]["who"], "Alice");
    EXPECT_EQ(rows[0]["age"], 30);
    EXPECT_FALSE(rows[0].contains("id")) << "В выборку попала не запрошенная колонка";
}

// ----------------------------------------------------------------------------
// 12. Запрос может занимать несколько строк — требование задания
// ----------------------------------------------------------------------------
TEST_F(CliTest, MultiLineQueryIsAccepted) {
    Session s = run(
        "CREATE DATABASE d;\n"
        "USE d;\n"
        "CREATE TABLE t (\n"
        "    id INT INDEXED,\n"
        "    name STRING\n"
        ");\n"
        "INSERT INTO t (id, name)\n"
        "     VALUE (1,\n"
        "            \"Многострочный\");\n"
        "SELECT *\n"
        "  FROM t\n"
        " WHERE id == 1;\n");

    const json rows = single_result(s);
    ASSERT_EQ(rows.size(), 1u) << s.output;
    EXPECT_EQ(rows[0]["name"], "Многострочный");
}

// ----------------------------------------------------------------------------
// 13. Регистр ключевых слов: любой допустим, смешение внутри слова — нет
// ----------------------------------------------------------------------------
TEST_F(CliTest, KeywordCaseRules) {
    Session s = run(R"SQL(
create database d;
use d;
CREATE TABLE t (id INT INDEXED);
insert into t (id) value (1);
select * from t;
CrEaTe TABLE bad (id INT);
)SQL");

    const json rows = single_result(s);
    EXPECT_EQ(rows.size(), 1u) << "Строчные ключевые слова должны работать:\n" << s.output;

    EXPECT_TRUE(contains(s.output, "Mixed case"))
        << "Смешение регистров внутри ключевого слова должно отклоняться:\n" << s.output;
}

// ----------------------------------------------------------------------------
// 14. Ошибки не роняют программу: после любой из них СУБД продолжает работать
// ----------------------------------------------------------------------------
TEST_F(CliTest, ErrorsDoNotCrashTheProgram) {
    Session s = run(R"SQL(
SELECT * FROM nothing;
USE no_such_db;
CREATE DATABASE d;
USE d;
SELECT * FROM no_such_table;
CREATE TABLE t (id INT INDEXED, v INT);
INSERT INTO t (id, v) VALUE ("не число", 1);
SELECT * FROM t WHERE v == "строка";
это вообще не sql;
SELECT * FROM t WHERE;
INSERT INTO t (id, v) VALUE (1, 100);
SELECT * FROM t;
)SQL");

    EXPECT_EQ(s.exit_code, 0) << "Программа завершилась аварийно:\n" << s.output;

    // После всех ошибок последний корректный запрос обязан отработать
    const std::vector<json> r = extract_results(s.output);
    ASSERT_FALSE(r.empty()) << s.output;
    const json& last = r.back();
    ASSERT_EQ(last.size(), 1u) << "СУБД не восстановилась после ошибок:\n" << s.output;
    EXPECT_EQ(last[0]["v"], 100);
}

// ----------------------------------------------------------------------------
// 15. Несколько таблиц и баз в одном файле сценария
// ----------------------------------------------------------------------------
TEST_F(CliTest, MultipleDatabasesAndTables) {
    Session s = run(R"SQL(
CREATE DATABASE first;
CREATE DATABASE second;
USE first;
CREATE TABLE a (id INT INDEXED, tag STRING);
INSERT INTO a (id, tag) VALUE (1, "из первой");
USE second;
CREATE TABLE a (id INT INDEXED, tag STRING);
INSERT INTO a (id, tag) VALUE (1, "из второй");
SELECT * FROM a;
USE first;
SELECT * FROM a;
)SQL");

    const std::vector<json> r = extract_results(s.output);
    ASSERT_EQ(r.size(), 2u) << s.output;
    ASSERT_EQ(r[0].size(), 1u);
    ASSERT_EQ(r[1].size(), 1u);
    EXPECT_EQ(r[0][0]["tag"], "из второй");
    EXPECT_EQ(r[1][0]["tag"], "из первой")
        << "Таблицы разных баз с одинаковым именем перепутаны";
}

// ----------------------------------------------------------------------------
// 16. DROP TABLE удаляет таблицу, и она не возвращается после перезапуска
// ----------------------------------------------------------------------------
TEST_F(CliTest, DropTablePersists) {
    Session s = run(R"SQL(
CREATE DATABASE d;
USE d;
CREATE TABLE keep (id INT INDEXED);
CREATE TABLE gone (id INT INDEXED);
INSERT INTO gone (id) VALUE (1);
DROP TABLE gone;
)SQL");
    ASSERT_EQ(s.exit_code, 0) << s.output;

    Session after = run(R"SQL(
USE d;
SELECT * FROM gone;
)SQL");

    EXPECT_TRUE(contains(after.output, "gone"))
        << "Ожидалось сообщение об отсутствующей таблице:\n" << after.output;
    EXPECT_TRUE(extract_results(after.output).empty())
        << "Удалённая таблица вернулась после перезапуска:\n" << after.output;
}
