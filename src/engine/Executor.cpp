#include "engine/Executor.h"
#include <cstdint>
#include <regex>
#include <iostream>
#include "undo-log/UndoLogManager.h"

Executor::Executor(UndoLogManager* undoLog) : catalog_("./data"), undoLog_(undoLog) {
    for (const auto& name: catalog_.listDatabases()) {
        databases_[name] = std::make_unique<Database>("./data/" + name, name);
    }
}

QueryResult Executor::execCreateDatabase(const CreateDatabaseQuery& q) {
    catalog_.addDatabase(q.dbName);
    databases_[q.dbName] = std::make_unique<Database>("./data/" + q.dbName, q.dbName);
    return {true, "", {}, 0};
}

QueryResult Executor::execDropDatabase(const DropDatabaseQuery& q) {
    catalog_.removeDatabase(q.dbName);
    databases_.erase(q.dbName);
    if (currentDb_ == q.dbName)
        currentDb_ = "";
    return {true, "", {}, 0};
}

QueryResult Executor::execute(ASTNode* node) {
    if (!node)
        return {false, "Empty query", {}, 0};

    switch (node->kind) {
        case NodeKind::INSERT_QUERY:
            return execInsert(*dynamic_cast<InsertQuery*>(node));
        case NodeKind::SELECT_QUERY:
            return execSelect(*dynamic_cast<SelectQuery*>(node));
        case NodeKind::UPDATE_QUERY:
            return execUpdate(*dynamic_cast<UpdateQuery*>(node));
        case NodeKind::DELETE_QUERY:
            return execDelete(*dynamic_cast<DeleteQuery*>(node));
        case NodeKind::CREATE_TABLE_QUERY:
            return execCreateTable(*dynamic_cast<CreateTableQuery*>(node));
        case NodeKind::DROP_TABLE_QUERY:
            return execDropTable(*dynamic_cast<DropTableQuery*>(node));
        case NodeKind::CREATE_DATABASE_QUERY:
            return execCreateDatabase(*dynamic_cast<CreateDatabaseQuery*>(node));
        case NodeKind::DROP_DATABASE_QUERY:
            return execDropDatabase(*dynamic_cast<DropDatabaseQuery*>(node));
        case NodeKind::REVERT_QUERY:
            return execRevert(*dynamic_cast<RevertQuery*>(node));
        case NodeKind::USE_QUERY:
            return execUse(*dynamic_cast<UseQuery*>(node));
        default:
            return {false, "Unknown command", {}, 0};
    }
}

QueryResult Executor::execUse(const UseQuery& q) {
    if (!catalog_.hasDatabase(q.dbName))
        throw SemanticError("Database does not exist: " + q.dbName);
    currentDb_ = q.dbName;
    return {true, "", {}, 0};
}

Database& Executor::currentDatabase() {
    if (currentDb_.empty())
        throw SemanticError("No database selected. Use USE <db_name>");
    auto it = databases_.find(currentDb_);
    if (it == databases_.end())
        throw SemanticError("Database not loaded: " + currentDb_);
    return *it->second;
}

QueryResult Executor::execCreateTable(const CreateTableQuery& q) {
    Database& db = currentDatabase();
    Schema schema;
    schema.tableName = q.tableName;
    for (const auto& col: q.columns) {
        ColumnDef def;
        def.name = col.name;
        def.type = col.type;
        def.notNull = col.notNull;
        def.indexed = col.indexed;
        def.defaultValue = col.defaultValue;
        schema.columns.push_back(def);
    }
    db.createTable(schema);
    return {true, "", {}, 0};
}

QueryResult Executor::execDropTable(const DropTableQuery& q) {
    currentDatabase().dropTable(q.tableName);
    return {true, "", {}, 0};
}

QueryResult Executor::execInsert(const InsertQuery& q) {
    std::cout << "DEBUG: execInsert CALLED" << std::endl;
    
    Database& db = currentDatabase();
    Table& tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;
    
    for (const auto& rowAst : q.values) {
        std::vector<Value> record(schema.columns.size(), std::nullopt);

        std::cout << "DEBUG execInsert: rowAst.size() = " << rowAst.size() << std::endl;
        std::cout << "DEBUG execInsert: q.columns.size() = " << q.columns.size() << std::endl;

        if (q.columns.empty()) {
            std::cout << "DEBUG: Columns not specified, using schema order" << std::endl;
            for (size_t i = 0; i < rowAst.size() && i < schema.columns.size(); ++i) {
                auto* lit = dynamic_cast<const Literal*>(rowAst[i].get());
                if (!lit) {
                    throw SemanticError("Expected literal value in INSERT");
                }
                record[i] = lit->value;
            }
        } else {
            for (size_t i = 0; i < q.columns.size(); ++i) {
                int idx = schema.columnIndex(q.columns[i]);
                if (idx == -1) {
                    throw SemanticError("Unknown column: " + q.columns[i]);
                }

                auto* lit = dynamic_cast<const Literal*>(rowAst[i].get());
                if (!lit) {
                    throw SemanticError("Expected literal value in INSERT");
                }

                record[idx] = lit->value;
            }
        }

        for (size_t i = 0; i < schema.columns.size(); ++i) {
            if (!val::isNull(record[i])) continue;

            const auto& col = schema.columns[i];
            if (col.defaultValue.has_value()) {
                record[i] = col.defaultValue.value();
            } else if (col.notNull) {
                throw SemanticError("Column '" + col.name + "' cannot be NULL");
            }
        }

        tbl.insert(record);
        
        if (undoLog_) {
            auto keys = serializeKey(record, schema);
            undoLog_->logUndoInsert(currentDb_ + "." + q.tableName,
                                    UndoLogManager::getCurrentTimeMs(), keys);
        }
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execDelete(const DeleteQuery& q) {
    Database& db = currentDatabase();
    Table& tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;
    std::vector<RecordID> toDelete;

    tbl.scan([&](RecordID recordID, const std::vector<Value>& record) {
        if (matches(record, schema, q.where.get()))
            toDelete.push_back(recordID);
    });

    for (auto recordID: toDelete) {
        if (undoLog_) {
            auto old = tbl.fetch(recordID);
            auto keys = serializeKey(old, schema);
            auto rowData = serializeRow(old);
            undoLog_->logUndoDelete(currentDb_ + "." + q.tableName,
                                    UndoLogManager::getCurrentTimeMs(), keys, rowData);
        }
        tbl.remove(recordID);
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execUpdate(const UpdateQuery& q) {
    Database& db = currentDatabase();
    Table& tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    int affected = 0;
    std::vector<std::pair<RecordID, std::vector<Value>>> toUpdate;

    tbl.scan([&](RecordID recordID, const std::vector<Value>& record) {
        if (!matches(record, schema, q.where.get()))
            return;

        std::vector<Value> newRecord = record;
        for (const auto& [colName, expr]: q.assignments) {
            int idx = schema.columnIndex(colName);
            if (idx == -1)
                throw SemanticError("Unknown column: " + colName);
            newRecord[idx] = resolve(expr.get(), record, schema);
        }
        toUpdate.emplace_back(recordID, std::move(newRecord));
    });

    for (auto& [recordID, newRecord]: toUpdate) {
        if (undoLog_) {
            auto old = tbl.fetch(recordID);
            auto keys = serializeKey(old, schema);
            auto rowData = serializeRow(old);
            undoLog_->logUndoUpdate(currentDb_ + "." + q.tableName,
                                    UndoLogManager::getCurrentTimeMs(), keys, rowData);
        }
        tbl.update(recordID, newRecord);
        affected++;
    }

    return {true, "", {}, affected};
}

QueryResult Executor::execSelect(const SelectQuery& q) {
    Database& db = currentDatabase();
    Table& tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    if (int idxCol = schema.indexedColumn();
        idxCol != -1 && q.where && q.where->kind == NodeKind::BINARY_OP) {
        auto* bin = dynamic_cast<const BinaryOp*>(q.where.get());
        if (bin->op == "==") {
            bool leftIsCol = bin->left->kind == NodeKind::COLUMN_REF;
            bool rightIsCol = bin->right->kind == NodeKind::COLUMN_REF;
            const ASTNode* colNode = leftIsCol ? bin->left.get() : bin->right.get();
            const ASTNode* valNode = leftIsCol ? bin->right.get() : bin->left.get();

            if (!rightIsCol || !leftIsCol) {
                auto* ref = dynamic_cast<const ColumnRef*>(colNode);
                if (schema.columnIndex(ref->name) == idxCol) {
                    Value key = resolve(valNode, {}, schema);
                    try {
                        RecordID recordID = tbl.findByIndex(ref->name, key);
                        auto record = tbl.fetch(recordID);
                        std::vector<Row> rows;
                        if (q.aggregates.empty())
                            rows.push_back(project(record, schema, q));
                        return {true, "", rows, 0};
                    } catch (const IndexError&) {
                        return {true, "", {}, 0};
                    }
                }
            }
        }
    }

    if (!q.aggregates.empty()) {
        struct Acc {
            double sum = 0;
            int count = 0;
            bool hasVal = false;
        };
        std::vector<Acc> accs(q.aggregates.size());

        tbl.scan([&](RecordID, const std::vector<Value>& record) {
            if (!matches(record, schema, q.where.get()))
                return;
            for (size_t i = 0; i < q.aggregates.size(); i++) {
                const auto& agg = q.aggregates[i];
                if (agg.func == "COUNT") {
                    accs[i].count++;
                } else {
                    int colIdx = schema.columnIndex(agg.column);
                    if (colIdx == -1)
                        throw SemanticError("Unknown column: " + agg.column);
                    const Value& v = record[colIdx];
                    if (!val::isNull(v)) {
                        double d = val::isInt(v) ? val::getInt(v) : 0;
                        accs[i].sum += d;
                        accs[i].count++;
                        accs[i].hasVal = true;
                    }
                }
            }
        });

        Row row;
        for (size_t i = 0; i < q.aggregates.size(); i++) {
            const auto& agg = q.aggregates[i];
            std::string label = agg.alias.empty() ? agg.func + "(" + agg.column + ")" : agg.alias;
            if (agg.func == "COUNT") {
                row.emplace_back(label, Value(accs[i].count));
            } else if (agg.func == "SUM") {
                row.emplace_back(label, Value(static_cast<int>(accs[i].sum)));
            } else if (agg.func == "AVG") {
                if (accs[i].count == 0)
                    row.emplace_back(label, std::nullopt);
                else
                    row.emplace_back(label, Value(static_cast<int>(accs[i].sum / accs[i].count)));
            }
        }
        return {true, "", {row}, 0};
    }

    std::vector<Row> rows;
    tbl.scan([&](RecordID, const std::vector<Value>& record) {
        if (matches(record, schema, q.where.get()))
            rows.push_back(project(record, schema, q));
    });

    return {true, "", rows, 0};
}

std::string Executor::toJSON(const std::vector<Row>& rows) {
    std::string out = "[\n";
    for (size_t i = 0; i < rows.size(); i++) {
        out += "  {";
        const auto& row = rows[i];
        for (size_t j = 0; j < row.size(); j++) {
            const auto& [name, value] = row[j];
            out += "\"" + name + "\": ";
            if (val::isNull(value)) {
                out += "null";
            } else if (val::isInt(value)) {
                out += std::to_string(val::getInt(value));
            } else if (val::isString(value)) {
                out += "\"" + val::getString(value) + "\"";
            } else {
                out += "null";
            }
            if (j + 1 < row.size())
                out += ", ";
        }
        out += "}";
        if (i + 1 < rows.size())
            out += ",";
        out += "\n";
    }
    out += "]";
    return out;
}

Value Executor::resolve(const ASTNode* node, const std::vector<Value>& record,
                        const Schema& schema) {
    if (node->kind == NodeKind::LITERAL) {
        return dynamic_cast<const Literal*>(node)->value;
    }

    if (node->kind == NodeKind::COLUMN_REF) {
        auto* ref = dynamic_cast<const ColumnRef*>(node);
        const int idx = schema.columnIndex(ref->name);
        if (idx == -1)
            throw SemanticError("Unknown column: " + ref->name);
        return record[idx];
    }

    throw SemanticError("Unexpected node type in expression");
}

bool Executor::matches(const std::vector<Value>& record, const Schema& schema,
                       const ASTNode* where) {
    if (!where)
        return true;

    switch (where->kind) {
        case NodeKind::OR_OP: {
            auto* n = dynamic_cast<const OrOp*>(where);
            return matches(record, schema, n->left.get()) ||
                   matches(record, schema, n->right.get());
        }
        case NodeKind::AND_OP: {
            auto* n = dynamic_cast<const AndOp*>(where);
            return matches(record, schema, n->left.get()) &&
                   matches(record, schema, n->right.get());
        }
        case NodeKind::BINARY_OP: {
            auto* n = dynamic_cast<const BinaryOp*>(where);
            Value lv = resolve(n->left.get(), record, schema);
            Value rv = resolve(n->right.get(), record, schema);

            if (val::isNull(lv) || val::isNull(rv))
                return false;

            if (n->op == "==")
                return valueEqual(lv, rv);
            if (n->op == "!=")
                return !valueEqual(lv, rv);
            if (n->op == "<")
                return valueLess(lv, rv);
            if (n->op == ">")
                return valueLess(rv, lv);
            if (n->op == "<=")
                return !valueLess(rv, lv);
            if (n->op == ">=")
                return !valueLess(lv, rv);

            throw SemanticError("Unknown operator: " + n->op);
        }

        case NodeKind::BETWEEN_OP: {
            auto* n = dynamic_cast<const BetweenOp*>(where);
            Value low = resolve(n->low.get(), record, schema);
            Value high = resolve(n->high.get(), record, schema);
            Value val = resolve(n->expr.get(), record, schema);

            if (val::isNull(low) || val::isNull(val) || val::isNull(high))
                return false;

            return !valueLess(val, low) && !valueLess(high, val);
        }
        case NodeKind::LIKE_OP: {
            auto* n = dynamic_cast<const LikeOp*>(where);
            Value val = resolve(n->expr.get(), record, schema);
            Value pattern = resolve(n->pattern.get(), record, schema);

            if (val::isNull(val) || val::isNull(pattern))
                return false;

            if (!val::isString(val) || !val::isString(pattern))
                throw SemanticError("LIKE requires string operands");

            try {
                std::regex re(val::getString(pattern));
                return std::regex_match(val::getString(val), re);
            } catch (const std::regex_error&) {
                throw SemanticError("Invalid regex pattern: " + val::getString(pattern));
            }
        }
        default:
            throw SemanticError("Unexpected node type in WHERE clause");
    }
}

Row Executor::project(const std::vector<Value>& record, const Schema& schema,
                      const SelectQuery& q) {
    Row row;

    if (q.star) {
        for (size_t i = 0; i < schema.columns.size(); i++)
            row.emplace_back(schema.columns[i].name, record[i]);

    } else {
        for (const auto& [name, alias]: q.columns) {
            const auto idx = schema.columnIndex(name);
            if (idx == -1)
                throw SemanticError("Unknown column: " + name);
            auto nameOut = alias.empty() ? name : alias;
            row.emplace_back(nameOut, record[idx]);
        }
    }
    return row;
}

std::vector<uint8_t> Executor::serializeRow(const std::vector<Value>& record) {
    std::vector<uint8_t> buf;
    for (const auto& v: record) {
        if (val::isNull(v)) {
            buf.push_back(0);
        } else if (val::isInt(v)) {
            buf.push_back(1);
            int32_t n = val::getInt(v);
            uint8_t* p = reinterpret_cast<uint8_t*>(&n);
            buf.insert(buf.end(), p, p + 4);
        } else if (val::isString(v)) {
            buf.push_back(2);
            const std::string& s = val::getString(v);
            uint32_t len = static_cast<uint32_t>(s.size());
            uint8_t* p = reinterpret_cast<uint8_t*>(&len);
            buf.insert(buf.end(), p, p + 4);
            buf.insert(buf.end(), s.begin(), s.end());
        }
    }
    return buf;
}

std::vector<Value> Executor::deserializeRow(const std::vector<uint8_t>& buf) {
    std::vector<Value> record;
    size_t i = 0;
    while (i < buf.size()) {
        uint8_t tag = buf[i++];
        if (tag == 0) {
            record.push_back(std::nullopt);
        } else if (tag == 1) {
            int32_t n;
            std::memcpy(&n, buf.data() + i, 4);
            i += 4;
            record.push_back(Value(n));
        } else {
            uint32_t len;
            std::memcpy(&len, buf.data() + i, 4);
            i += 4;
            std::string s(buf.begin() + i, buf.begin() + i + len);
            i += len;
            record.push_back(Value(s));
        }
    }
    return record;
}

std::vector<uint8_t> Executor::serializeKey(const std::vector<Value>& record,
                                            const Schema& schema) {
    int idxCol = schema.indexedColumn();
    if (idxCol != -1)
        return serializeRow({record[idxCol]});
    return serializeRow(record);
}

static uint64_t parseTimestamp(const std::string& ts) {
    int year, month, day, hour, min, sec, ms;
    if (std::sscanf(ts.c_str(), "%d.%d.%d-%d:%d:%d.%d", &year, &month, &day, &hour, &min, &sec,
                    &ms) != 7) {
        throw SemanticError("Invalid timestamp format: " + ts +
                            " (expected yyyy.mm.dd-hh:mm:ss.msmsms)");
    }

    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = -1;

    std::time_t epoch = std::mktime(&t);
    if (epoch == -1)
        throw SemanticError("Cannot convert timestamp: " + ts);

    return static_cast<uint64_t>(epoch) * 1000ULL + ms;
}

QueryResult Executor::execRevert(const RevertQuery& q) {
    if (!undoLog_)
        return {false, "Undo log not configured", {}, 0};

    Database& db = currentDatabase();
    Table& tbl = db.getTable(q.tableName);
    const Schema& schema = tbl.schema();

    std::string fullName = currentDb_ + "." + q.tableName;

    uint64_t timeMs = parseTimestamp(q.targetTimestamp);
    auto records = undoLog_->getRecordsToRevert(fullName, timeMs);

    int affected = 0;
    for (const auto& rec: records) {
        switch (rec.actionType) {
            case RevertActionType::REVERT_INSERT: {
                auto key = deserializeRow(rec.keys);
                int idxCol = schema.indexedColumn();

                if (idxCol == -1)
                    throw SemanticError("Cannot revert INSERT without indexed column");

                RecordID rid = tbl.findByIndex(schema.columns[idxCol].name, key[0]);
                tbl.remove(rid);
                break;
            }

            case RevertActionType::REVERT_DELETE: {
                auto row = deserializeRow(rec.oldRowData);
                tbl.insert(row);
                break;
            }

            case RevertActionType::REVERT_UPDATE: {
                auto oldRow = deserializeRow(rec.oldRowData);
                auto key = deserializeRow(rec.keys);
                int idxCol = schema.indexedColumn();

                if (idxCol == -1)
                    throw SemanticError("Cannot revert UPDATE without indexed column");

                RecordID rid = tbl.findByIndex(schema.columns[idxCol].name, key[0]);
                tbl.update(rid, oldRow);
                break;
            }
        }
        affected++;
    }

    undoLog_->truncateLog(timeMs);

    return {true, "", {}, affected};
}
