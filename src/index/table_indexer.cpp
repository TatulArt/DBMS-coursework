#include "./table_indexer.h"
#include <algorithm>

// ============================================================================
// TableSchema
// ============================================================================

int TableSchema::column_index(const std::string& name) const {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const ColumnDef* TableSchema::column(const std::string& name) const {
    const int idx = column_index(name);
    return (idx < 0) ? nullptr : &columns[static_cast<size_t>(idx)];
}

std::string access_method_to_string(AccessMethod method) {
    switch (method) {
        case AccessMethod::IndexLookup: return "INDEX LOOKUP";
        case AccessMethod::IndexRange:  return "INDEX RANGE SCAN";
        case AccessMethod::FullScan:    return "FULL SCAN";
    }
    return "UNKNOWN";
}

// ============================================================================
// TableIndexer
// ============================================================================

TableIndexer::TableIndexer(PageManager& page_manager,
                           RecordManager& record_manager,
                           IndexManager& index_manager)
    : page_manager_(page_manager),
      record_manager_(record_manager),
      index_manager_(index_manager) {}

std::string TableIndexer::index_name_for(const std::string& table_name,
                                         const std::string& column_name) {
    return "idx_" + table_name + "_" + column_name;
}

Result<int32_t> TableIndexer::to_index_key(const Value& value) {
    if (value.is_null()) {
        return Result<int32_t>(Status::Error(StatusCode::NullConstraintViolation,
                                             "NULL cannot be used as an index key"));
    }
    if (value.get_type() != ColumnType::Int) {
        return Result<int32_t>(Status::Error(
            StatusCode::TypeMismatch,
            "Only INT columns can be indexed: B+ tree keys are int32"));
    }
    return Result<int32_t>(value.get_int());
}

// ----------------------------------------------------------------------------
// Построение индексов
// ----------------------------------------------------------------------------

Status TableIndexer::create_indexes_for_table(const TableSchema& schema) {
    for (const ColumnDef& column : schema.columns) {
        if (!column.is_indexed) {
            continue;
        }
        auto res = build_index(schema, column.name);
        if (!res.ok()) {
            return res.status();
        }
    }
    return Status::OK();
}

Result<IndexInfo> TableIndexer::build_index(const TableSchema& schema,
                                            const std::string& column_name) {
    const int col_idx = schema.column_index(column_name);
    if (col_idx < 0) {
        return Result<IndexInfo>(Status::Error(
            StatusCode::ColumnNotFound,
            "Column " + column_name + " not found in table " + schema.table_name));
    }

    const ColumnDef& column = schema.columns[static_cast<size_t>(col_idx)];
    if (column.type != ColumnType::Int) {
        return Result<IndexInfo>(Status::Error(
            StatusCode::TypeMismatch,
            "Cannot build index on " + schema.table_name + "." + column_name +
                ": column type " + columnTypeToString(column.type) +
                " is not supported by the int32-keyed B+ tree"));
    }

    const std::string index_name = index_name_for(schema.table_name, column_name);

    // Индекс мог быть создан ранее — тогда пересоздаём его с нуля,
    // чтобы build_index можно было использовать и как «перестроить индекс».
    if (index_manager_.has_index(index_name)) {
        Status st = index_manager_.drop_index(index_name);
        if (!st.ok()) return Result<IndexInfo>(st);
    }

    auto create_res = index_manager_.create_index(index_name, schema.table_name,
                                                  column_name, column.type);
    if (!create_res.ok()) {
        return create_res;
    }

    // Полный проход по страницам данных таблицы.
    // В дерево кладём только ключ и физический адрес записи — сами данные
    // не дублируются (требование задания о хранении индексов).
    for (PageId page_id : schema.data_pages) {
        auto page_res = record_manager_.scan_page(page_id, schema.columns);
        if (!page_res.ok()) {
            index_manager_.drop_index(index_name);
            return Result<IndexInfo>(page_res.status());
        }

        for (const Record& record : page_res.value()) {
            const Value& value = record.fields[static_cast<size_t>(col_idx)];

            if (value.is_null()) {
                index_manager_.drop_index(index_name);
                return Result<IndexInfo>(Status::Error(
                    StatusCode::NullConstraintViolation,
                    "Column " + schema.table_name + "." + column_name +
                        " is INDEXED and must not contain NULL"));
            }

            auto key_res = to_index_key(value);
            if (!key_res.ok()) {
                index_manager_.drop_index(index_name);
                return Result<IndexInfo>(key_res.status());
            }

            Status st = index_manager_.insert_entry(index_name, key_res.value(), record.id);
            if (!st.ok()) {
                index_manager_.drop_index(index_name);
                if (st.code == StatusCode::UniqueConstraintViolation) {
                    return Result<IndexInfo>(Status::Error(
                        StatusCode::UniqueConstraintViolation,
                        "Column " + schema.table_name + "." + column_name +
                            " is INDEXED but contains duplicate value " + value.to_string()));
                }
                return Result<IndexInfo>(st);
            }
        }
    }

    return index_manager_.get_index_info(index_name);
}

// ----------------------------------------------------------------------------
// Проверка ограничений целостности
// ----------------------------------------------------------------------------

Status TableIndexer::validate_row(const TableSchema& schema,
                                  const std::vector<Value>& fields) const {
    if (fields.size() != schema.columns.size()) {
        return Status::Error(StatusCode::InvalidArgument,
                             "Row has " + std::to_string(fields.size()) +
                                 " values, but table " + schema.table_name + " has " +
                                 std::to_string(schema.columns.size()) + " columns");
    }

    for (size_t i = 0; i < fields.size(); ++i) {
        const ColumnDef& column = schema.columns[i];
        const Value& value = fields[i];

        if (value.is_null()) {
            // Поле с модификатором INDEXED уникально и не может быть NULL
            if (!column.is_nullable || column.is_indexed) {
                return Status::Error(StatusCode::NullConstraintViolation,
                                     "Column " + column.name + " must not be NULL");
            }
            continue;
        }

        if (value.get_type() != column.type) {
            return Status::Error(StatusCode::TypeMismatch,
                                 "Column " + column.name + " expects " +
                                     columnTypeToString(column.type) + ", got " +
                                     columnTypeToString(value.get_type()));
        }
    }

    return Status::OK();
}

// ----------------------------------------------------------------------------
// Вставка строки
// ----------------------------------------------------------------------------

Result<RecordId> TableIndexer::insert_row(TableSchema& schema, const std::vector<Value>& fields) {
    Status st = validate_row(schema, fields);
    if (!st.ok()) {
        return Result<RecordId>(st);
    }

    // 1. Проверяем уникальность индексируемых колонок ДО записи на диск,
    //    чтобы не пришлось откатывать уже вставленную запись.
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        const ColumnDef& column = schema.columns[i];
        if (!column.is_indexed) continue;

        const std::string index_name = index_name_for(schema.table_name, column.name);
        if (!index_manager_.has_index(index_name)) continue;

        auto key_res = to_index_key(fields[i]);
        if (!key_res.ok()) {
            return Result<RecordId>(key_res.status());
        }
        if (index_manager_.find_entry(index_name, key_res.value()).ok()) {
            return Result<RecordId>(Status::Error(
                StatusCode::UniqueConstraintViolation,
                "Duplicate value " + fields[i].to_string() + " for indexed column " +
                    schema.table_name + "." + column.name));
        }
    }

    // 2. Ищем страницу данных, на которой хватит места.
    //    Сначала пробуем последнюю страницу: при последовательной вставке
    //    место находится сразу, и не нужно перечитывать всю таблицу.
    const size_t needed = RecordManager::record_size(fields, schema.columns);
    PageId target_page = INVALID_PAGE_ID;

    if (!schema.data_pages.empty()) {
        const PageId last_page = schema.data_pages.back();
        auto space_res = record_manager_.page_has_space(last_page, needed);
        if (!space_res.ok()) {
            return Result<RecordId>(space_res.status());
        }
        if (space_res.value()) {
            target_page = last_page;
        }
    }

    // Последняя страница заполнена — ищем дырку среди остальных,
    // чтобы переиспользовать место после удалённых записей.
    if (target_page == INVALID_PAGE_ID) {
        for (PageId page_id : schema.data_pages) {
            auto space_res = record_manager_.page_has_space(page_id, needed);
            if (!space_res.ok()) {
                return Result<RecordId>(space_res.status());
            }
            if (space_res.value()) {
                target_page = page_id;
                break;
            }
        }
    }

    // Свободного места нет — расширяем таблицу новой страницей
    if (target_page == INVALID_PAGE_ID) {
        Page new_page;
        PageId new_page_id = INVALID_PAGE_ID;
        st = page_manager_.allocate_page(new_page_id, new_page);
        if (!st.ok()) {
            return Result<RecordId>(st);
        }
        schema.data_pages.push_back(new_page_id);
        target_page = new_page_id;
    }

    // 3. Пишем запись в кучу
    auto insert_res = record_manager_.insert_record(target_page, fields, schema.columns);
    if (!insert_res.ok()) {
        return insert_res;
    }
    const RecordId rid = insert_res.value();

    // 4. Обновляем индексы. При сбое откатываем всё, что успели сделать.
    std::vector<std::pair<std::string, int32_t>> applied;
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        const ColumnDef& column = schema.columns[i];
        if (!column.is_indexed) continue;

        const std::string index_name = index_name_for(schema.table_name, column.name);
        if (!index_manager_.has_index(index_name)) continue;

        auto key_res = to_index_key(fields[i]);
        if (!key_res.ok()) {
            st = key_res.status();
        } else {
            st = index_manager_.insert_entry(index_name, key_res.value(), rid);
        }

        if (!st.ok()) {
            for (const auto& entry : applied) {
                index_manager_.remove_entry(entry.first, entry.second);
            }
            record_manager_.delete_record(rid);
            return Result<RecordId>(st);
        }
        applied.emplace_back(index_name, key_res.value());
    }

    return Result<RecordId>(rid);
}

// ----------------------------------------------------------------------------
// Удаление строки
// ----------------------------------------------------------------------------

Status TableIndexer::remove_from_indexes(const TableSchema& schema,
                                         const std::vector<Value>& fields) {
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        const ColumnDef& column = schema.columns[i];
        if (!column.is_indexed) continue;

        const std::string index_name = index_name_for(schema.table_name, column.name);
        if (!index_manager_.has_index(index_name)) continue;

        auto key_res = to_index_key(fields[i]);
        if (!key_res.ok()) {
            return key_res.status();
        }

        Status st = index_manager_.remove_entry(index_name, key_res.value());
        // Отсутствие ключа не считаем ошибкой: индекс мог быть построен позже
        if (!st.ok() && st.code != StatusCode::RecordNotFound) {
            return st;
        }
    }
    return Status::OK();
}

Status TableIndexer::delete_row(const TableSchema& schema, const RecordId& rid) {
    auto record_res = record_manager_.get_record(rid, schema.columns);
    if (!record_res.ok()) {
        return record_res.status();
    }

    // Сначала чистим индексы: пока запись на месте, её значения ещё доступны
    Status st = remove_from_indexes(schema, record_res.value().fields);
    if (!st.ok()) {
        return st;
    }

    return record_manager_.delete_record(rid);
}

// ----------------------------------------------------------------------------
// Выборка
// ----------------------------------------------------------------------------

Result<Record> TableIndexer::fetch(const TableSchema& schema, const RecordId& rid) {
    return record_manager_.get_record(rid, schema.columns);
}

Result<std::vector<Record>> TableIndexer::fetch_all(const TableSchema& schema,
                                                    const std::vector<RecordId>& rids) {
    std::vector<Record> records;
    records.reserve(rids.size());

    for (const RecordId& rid : rids) {
        auto res = record_manager_.get_record(rid, schema.columns);
        if (!res.ok()) {
            // Индекс может ссылаться на слот, освобождённый в обход TableIndexer.
            // Такие «висячие» ссылки просто пропускаем, а не роняем весь запрос.
            if (res.status().code == StatusCode::RecordNotFound) {
                continue;
            }
            return Result<std::vector<Record>>(res.status());
        }
        records.push_back(res.value());
    }

    return Result<std::vector<Record>>(std::move(records));
}

Result<SelectResult> TableIndexer::full_scan(const TableSchema& schema) {
    SelectResult out;
    out.method = AccessMethod::FullScan;

    for (PageId page_id : schema.data_pages) {
        auto page_res = record_manager_.scan_page(page_id, schema.columns);
        if (!page_res.ok()) {
            return Result<SelectResult>(page_res.status());
        }
        out.pages_examined++;
        for (const Record& record : page_res.value()) {
            out.records.push_back(record);
        }
    }

    return Result<SelectResult>(std::move(out));
}

Result<SelectResult> TableIndexer::select_equal(const TableSchema& schema,
                                                const std::string& column_name,
                                                const Value& value) {
    const int col_idx = schema.column_index(column_name);
    if (col_idx < 0) {
        return Result<SelectResult>(Status::Error(
            StatusCode::ColumnNotFound,
            "Column " + column_name + " not found in table " + schema.table_name));
    }

    // ------------------------------------------------------------------
    // Быстрый путь: по колонке есть B+ индекс и значение — целое число.
    // Вместо чтения всех страниц данных спускаемся по дереву и читаем
    // ровно одну страницу с найденной записью.
    // ------------------------------------------------------------------
    const std::string index_name = index_name_for(schema.table_name, column_name);
    if (!value.is_null() && index_manager_.has_index(index_name)) {
        auto key_res = to_index_key(value);
        if (key_res.ok()) {
            SelectResult out;
            out.method = AccessMethod::IndexLookup;

            auto rid_res = index_manager_.find_entry(index_name, key_res.value());
            if (rid_res.ok()) {
                auto record_res = record_manager_.get_record(rid_res.value(), schema.columns);
                if (record_res.ok()) {
                    out.records.push_back(record_res.value());
                    out.pages_examined = 1;
                } else if (record_res.status().code != StatusCode::RecordNotFound) {
                    return Result<SelectResult>(record_res.status());
                }
            } else if (rid_res.status().code != StatusCode::RecordNotFound) {
                return Result<SelectResult>(rid_res.status());
            }

            return Result<SelectResult>(std::move(out));
        }
    }

    // ------------------------------------------------------------------
    // Медленный путь: индекса нет — полный последовательный просмотр
    // ------------------------------------------------------------------
    auto scan_res = full_scan(schema);
    if (!scan_res.ok()) return scan_res;

    SelectResult out;
    out.method = AccessMethod::FullScan;
    out.pages_examined = scan_res.value().pages_examined;

    for (const Record& record : scan_res.value().records) {
        const Value& field = record.fields[static_cast<size_t>(col_idx)];
        if (field.is_null() != value.is_null()) continue;
        if (field.is_null()) {
            out.records.push_back(record);
            continue;
        }
        if (field.get_type() == value.get_type() && field == value) {
            out.records.push_back(record);
        }
    }

    return Result<SelectResult>(std::move(out));
}

Result<SelectResult> TableIndexer::select_range(const TableSchema& schema,
                                                const std::string& column_name,
                                                const Value& low,
                                                const Value& high,
                                                bool high_inclusive) {
    const int col_idx = schema.column_index(column_name);
    if (col_idx < 0) {
        return Result<SelectResult>(Status::Error(
            StatusCode::ColumnNotFound,
            "Column " + column_name + " not found in table " + schema.table_name));
    }
    if (low.is_null() || high.is_null()) {
        return Result<SelectResult>(Status::Error(StatusCode::InvalidArgument,
                                                  "Range boundaries must not be NULL"));
    }

    // Быстрый путь: диапазонный обход листьев B+ дерева через связный список
    const std::string index_name = index_name_for(schema.table_name, column_name);
    if (index_manager_.has_index(index_name)) {
        auto low_res = to_index_key(low);
        auto high_res = to_index_key(high);

        if (low_res.ok() && high_res.ok()) {
            const int32_t low_key = low_res.value();
            int32_t high_key = high_res.value();

            std::vector<RecordId> rids;
            Status st;
            if (high_inclusive) {
                st = index_manager_.range_scan(index_name, low_key, high_key, rids);
            } else {
                auto tree_res = index_manager_.get_index(index_name);
                if (!tree_res.ok()) return Result<SelectResult>(tree_res.status());
                st = tree_res.value().scan_range_half_open(low_key, high_key, rids);
            }
            if (!st.ok()) return Result<SelectResult>(st);

            auto records_res = fetch_all(schema, rids);
            if (!records_res.ok()) return Result<SelectResult>(records_res.status());

            SelectResult out;
            out.method = AccessMethod::IndexRange;
            out.records = std::move(records_res.value());

            // Считаем именно различные страницы, а не количество записей:
            // соседние по ключу записи обычно лежат на одной странице.
            std::vector<PageId> touched;
            for (const Record& record : out.records) {
                touched.push_back(record.id.page_id);
            }
            std::sort(touched.begin(), touched.end());
            touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
            out.pages_examined = touched.size();

            return Result<SelectResult>(std::move(out));
        }
    }

    // Медленный путь: полный скан с проверкой границ
    auto scan_res = full_scan(schema);
    if (!scan_res.ok()) return scan_res;

    SelectResult out;
    out.method = AccessMethod::FullScan;
    out.pages_examined = scan_res.value().pages_examined;

    for (const Record& record : scan_res.value().records) {
        const Value& field = record.fields[static_cast<size_t>(col_idx)];
        if (field.is_null()) continue;
        if (field.get_type() != low.get_type() || field.get_type() != high.get_type()) continue;

        const bool above_low = !(field < low);
        const bool below_high = high_inclusive ? !(high < field) : (field < high);
        if (above_low && below_high) {
            out.records.push_back(record);
        }
    }

    return Result<SelectResult>(std::move(out));
}
