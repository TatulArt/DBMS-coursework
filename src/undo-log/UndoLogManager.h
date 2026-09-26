#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

enum class RevertActionType : uint8_t {
    REVERT_INSERT = 1,  // был INSERT - надо DELETE
    REVERT_DELETE = 2,  // был DELETE - надо INSERT
    REVERT_UPDATE = 3   // был UPDATE - надо вернуть old_row
};

struct UndoRecord {
    RevertActionType actionType;
    uint64_t timeMs;
    std::string table;
    std::vector<uint8_t> beforeRow;  // было (пусто для INSERT)
    std::vector<uint8_t> afterRow;   // стало (пусто для DELETE)
};

class UndoLogManager {
public:
    explicit UndoLogManager(const std::string& path);

    // Запись в журнал
    void logUndoInsert(const std::string& table, uint64_t time,
                       const std::vector<uint8_t>& keys);
    void logUndoDelete(const std::string& table, uint64_t time,
                       const std::vector<uint8_t>& keys,
                       const std::vector<uint8_t>& data);
    void logUndoUpdate(const std::string& table, uint64_t time,
                       const std::vector<uint8_t>& keys,
                       const std::vector<uint8_t>& data);

    // Чтение записей для отката: все операции с timeMs >= targetTime
    // (то есть произошедшие ПОСЛЕ указанного момента)
    std::vector<UndoRecord> getRecordsToRevert(const std::string& table,
                                                uint64_t time);

    // Удалить из журнала записи с timeMs >= cutoff
    void truncateLog(uint64_t time);

    static uint64_t getCurrentTimeMs();

private:
    std::string path_;
    std::mutex mtx_;

    // Низкоуровневые операции с файлом
    void appendRecord(const UndoRecord& rec);
    std::vector<UndoRecord> readAllRecords();
    void rewriteFile(const std::vector<UndoRecord>& records);
};