#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class RevertActionType { REVERT_INSERT, REVERT_DELETE, REVERT_UPDATE };

struct UndoRecord {
    RevertActionType actionType;
    std::vector<uint8_t> keys;
    std::vector<uint8_t> oldRowData;
};

class UndoLogManager {
public:
    explicit UndoLogManager(const std::string& path) {}
    void logUndoInsert(const std::string& table, uint64_t time, const std::vector<uint8_t>& keys) {}
    void logUndoDelete(const std::string& table, uint64_t time, const std::vector<uint8_t>& keys, const std::vector<uint8_t>& data) {}
    void logUndoUpdate(const std::string& table, uint64_t time, const std::vector<uint8_t>& keys, const std::vector<uint8_t>& data) {}
    std::vector<UndoRecord> getRecordsToRevert(const std::string& table, uint64_t time) { return {}; }
    void truncateLog(uint64_t time) {}
    static uint64_t getCurrentTimeMs() { return 0; }
};
