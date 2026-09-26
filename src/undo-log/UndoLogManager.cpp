#include "undo-log/UndoLogManager.h"

#include <fstream>
#include <cstring>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <iostream>

namespace {

constexpr uint32_t UNDO_MAGIC = 0x554E444FU;
constexpr uint32_t UNDO_VERSION = 1;

void putU8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

void putU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

void putU64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

void putBytes(std::vector<uint8_t>& out, const std::vector<uint8_t>& bytes) {
    putU32(out, static_cast<uint32_t>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

void putString(std::vector<uint8_t>& out, const std::string& s) {
    putU32(out, static_cast<uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

bool takeU8(const std::vector<uint8_t>& in, size_t& pos, uint8_t& v) {
    if (pos + 1 > in.size()) return false;
    v = in[pos++];
    return true;
}

bool takeU32(const std::vector<uint8_t>& in, size_t& pos, uint32_t& v) {
    if (pos + 4 > in.size()) return false;
    v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<uint32_t>(in[pos + i]) << (8 * i);
    }
    pos += 4;
    return true;
}

bool takeU64(const std::vector<uint8_t>& in, size_t& pos, uint64_t& v) {
    if (pos + 8 > in.size()) return false;
    v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(in[pos + i]) << (8 * i);
    }
    pos += 8;
    return true;
}

bool takeBytes(const std::vector<uint8_t>& in, size_t& pos,
               std::vector<uint8_t>& out) {
    uint32_t len = 0;
    if (!takeU32(in, pos, len)) return false;
    if (pos + len > in.size()) return false;
    out.assign(in.begin() + pos, in.begin() + pos + len);
    pos += len;
    return true;
}

bool takeString(const std::vector<uint8_t>& in, size_t& pos, std::string& out) {
    uint32_t len = 0;
    if (!takeU32(in, pos, len)) return false;
    if (pos + len > in.size()) return false;
    out.assign(reinterpret_cast<const char*>(in.data() + pos), len);
    pos += len;
    return true;
}

} // namespace

UndoLogManager::UndoLogManager(const std::string& path) : path_(path) {}

uint64_t UndoLogManager::getCurrentTimeMs() {
    using namespace std::chrono;

    auto now_sys = system_clock::now();
    std::time_t now_t = system_clock::to_time_t(now_sys);

    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now_t);
#else
    localtime_r(&now_t, &local_tm);
#endif

    std::time_t local_epoch = std::mktime(&local_tm);

    auto ms = duration_cast<milliseconds>(now_sys.time_since_epoch()).count() % 1000;

    return static_cast<uint64_t>(local_epoch) * 1000ULL + static_cast<uint64_t>(ms);
}

void UndoLogManager::logUndoInsert(const std::string& table, uint64_t time,
                                   const std::vector<uint8_t>& keys) {
    std::lock_guard<std::mutex> lock(mtx_);
    UndoRecord rec;
    rec.actionType = RevertActionType::REVERT_INSERT;
    rec.timeMs = time;
    rec.table = table;
    rec.afterRow = keys;
    rec.beforeRow = {};
    appendRecord(rec);
}

void UndoLogManager::logUndoDelete(const std::string& table, uint64_t time,
                                   const std::vector<uint8_t>& keys,
                                   const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mtx_);
    UndoRecord rec;
    rec.actionType = RevertActionType::REVERT_DELETE;
    rec.timeMs = time;
    rec.table = table;
    rec.afterRow = keys;
    rec.beforeRow = data;
    appendRecord(rec);
}

void UndoLogManager::logUndoUpdate(const std::string& table, uint64_t time,
                                   const std::vector<uint8_t>& keys,
                                   const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mtx_);
    UndoRecord rec;
    rec.actionType = RevertActionType::REVERT_UPDATE;
    rec.timeMs = time;
    rec.table = table;
    rec.afterRow = keys;
    rec.beforeRow = data;
    appendRecord(rec);
}

void UndoLogManager::appendRecord(const UndoRecord& rec) {
    std::ifstream check(path_, std::ios::binary | std::ios::ate);
    const bool need_header = !check.is_open() || check.tellg() == 0;
    check.close();

    std::ofstream out(path_, std::ios::binary | std::ios::app);
    if (!out.is_open()) return;

    if (need_header) {
        std::vector<uint8_t> header;
        putU32(header, UNDO_MAGIC);
        putU32(header, UNDO_VERSION);
        out.write(reinterpret_cast<const char*>(header.data()),
                  static_cast<std::streamsize>(header.size()));
    }

    std::vector<uint8_t> buf;
    putU8(buf, static_cast<uint8_t>(rec.actionType));
    putU64(buf, rec.timeMs);
    putString(buf, rec.table);
    putBytes(buf, rec.afterRow);
    putBytes(buf, rec.beforeRow);

    out.write(reinterpret_cast<const char*>(buf.data()),
              static_cast<std::streamsize>(buf.size()));
}

std::vector<UndoRecord> UndoLogManager::readAllRecords() {
    std::vector<UndoRecord> records;

    std::ifstream in(path_, std::ios::binary);
    if (!in.is_open()) return records;

    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());

    if (data.size() < 8) return records;

    size_t pos = 0;
    uint32_t magic = 0, version = 0;
    takeU32(data, pos, magic);
    takeU32(data, pos, version);

    if (magic != UNDO_MAGIC) return records;

    while (pos < data.size()) {
        UndoRecord rec;
        uint8_t action = 0;

        if (!takeU8(data, pos, action)) break;
        if (!takeU64(data, pos, rec.timeMs)) break;
        if (!takeString(data, pos, rec.table)) break;
        if (!takeBytes(data, pos, rec.afterRow)) break;
        if (!takeBytes(data, pos, rec.beforeRow)) break;

        rec.actionType = static_cast<RevertActionType>(action);
        records.push_back(std::move(rec));
    }

    return records;
}

std::vector<UndoRecord> UndoLogManager::getRecordsToRevert(
    const std::string& /*table*/, uint64_t time) {
    std::lock_guard<std::mutex> lock(mtx_);

    std::vector<UndoRecord> all = readAllRecords();
    std::vector<UndoRecord> result;

    std::cerr << "[UNDO] readAllRecords returned " << all.size()
              << " records, filter timeMs >= " << time << "\n";

    for (auto& rec : all) {
        std::cerr << "[UNDO]   rec: action=" << (int)rec.actionType
                  << " timeMs=" << rec.timeMs
                  << " table=" << rec.table
                  << " afterRow=" << rec.afterRow.size()
                  << " beforeRow=" << rec.beforeRow.size()
                  << " -> " << (rec.timeMs >= time ? "MATCH" : "skip") << "\n";

        if (rec.timeMs >= time) {
            result.push_back(std::move(rec));
        }
    }

    std::cerr << "[UNDO] matched: " << result.size() << "\n";

    std::sort(result.begin(), result.end(),
              [](const UndoRecord& a, const UndoRecord& b) {
                  return a.timeMs > b.timeMs;
              });

    return result;
}

void UndoLogManager::truncateLog(uint64_t cutoff) {
    std::lock_guard<std::mutex> lock(mtx_);

    std::vector<UndoRecord> all = readAllRecords();
    std::vector<UndoRecord> kept;

    for (auto& rec : all) {
        if (rec.timeMs < cutoff) {
            kept.push_back(std::move(rec));
        }
    }

    rewriteFile(kept);
}

void UndoLogManager::rewriteFile(const std::vector<UndoRecord>& records) {
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;

    std::vector<uint8_t> header;
    putU32(header, UNDO_MAGIC);
    putU32(header, UNDO_VERSION);
    out.write(reinterpret_cast<const char*>(header.data()),
              static_cast<std::streamsize>(header.size()));

    for (const auto& rec : records) {
        std::vector<uint8_t> buf;
        putU8(buf, static_cast<uint8_t>(rec.actionType));
        putU64(buf, rec.timeMs);
        putString(buf, rec.table);
        putBytes(buf, rec.afterRow);
        putBytes(buf, rec.beforeRow);

        out.write(reinterpret_cast<const char*>(buf.data()),
                  static_cast<std::streamsize>(buf.size()));
    }
}