#ifndef DBMS_PARSER_AST_H
#define DBMS_PARSER_AST_H

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "engine/Schema.h"
#include "types.h"

// Виды узлов
enum class NodeKind {
    SELECT_QUERY,
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_QUERY,
    CREATE_TABLE_QUERY,
    DROP_TABLE_QUERY,
    CREATE_DATABASE_QUERY,
    DROP_DATABASE_QUERY,
    USE_QUERY,
    REVERT_QUERY,
    LITERAL,
    COLUMN_REF,
    BINARY_OP,
    AND_OP,
    OR_OP,
    BETWEEN_OP,
    LIKE_OP,
    STAR
};

// Базовый класс узла
class ASTNode {
public:
    NodeKind kind;
    explicit ASTNode(NodeKind k);
    virtual ~ASTNode() = default;
    [[nodiscard]] virtual std::string toString() const = 0;
};

// Листовые узлы
class Star : public ASTNode {
public:
    Star();
    [[nodiscard]] std::string toString() const override;
};

class ColumnRef : public ASTNode {
public:
    std::string name;
    explicit ColumnRef(const std::string& n);
    [[nodiscard]] std::string toString() const override;
};

class Literal : public ASTNode {
public:
    Value value;
    
    explicit Literal(Value v) : ASTNode(NodeKind::LITERAL), value(std::move(v)) {}
    explicit Literal(int v) : ASTNode(NodeKind::LITERAL), value(v) {}
    explicit Literal(const std::string& v) : ASTNode(NodeKind::LITERAL), value(v) {}
    
    [[nodiscard]] std::string toString() const override;

    bool isNull() const { return (value).is_null(); }
    bool isInt() const { return (!(value).is_null() && (value).get_type() == ColumnType::Int); }
    bool isString() const { return (!(value).is_null() && (value).get_type() == ColumnType::String); }
    int asInt() const { return (value).get_int(); }
    const std::string& asString() const { return (value).get_string(); }
};

// Операторы
class BinaryOp : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOp(const std::string& o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class AndOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    AndOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class OrOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    OrOp(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    [[nodiscard]] std::string toString() const override;
};

class BetweenOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> low;
    std::unique_ptr<ASTNode> high;
    BetweenOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> h);
    [[nodiscard]] std::string toString() const override;
};

class LikeOp : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;
    std::unique_ptr<ASTNode> pattern;
    LikeOp(std::unique_ptr<ASTNode> e, std::unique_ptr<ASTNode> p);
    [[nodiscard]] std::string toString() const override;
};

// Команды
struct SelectColumn {
    std::string name;
    std::string alias;
};

struct AggregateExpr {
    std::string func;
    std::string column;
    std::string alias;
};

class SelectQuery : public ASTNode {
public:
    bool star = false;
    std::vector<SelectColumn> columns;
    std::vector<AggregateExpr> aggregates;
    std::string tableName;
    std::unique_ptr<ASTNode> where;
    SelectQuery();
    [[nodiscard]] std::string toString() const override;
};

class InsertQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::string> columns;
    std::vector<std::vector<std::unique_ptr<ASTNode>>> values;
    InsertQuery();
    [[nodiscard]] std::string toString() const override;
};

class UpdateQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> assignments;
    std::unique_ptr<ASTNode> where;
    UpdateQuery();
    [[nodiscard]] std::string toString() const override;
};

class DeleteQuery : public ASTNode {
public:
    std::string tableName;
    std::unique_ptr<ASTNode> where;
    DeleteQuery();
    [[nodiscard]] std::string toString() const override;
};

struct ColumnSpec {
    std::string name;
    ColumnType type;
    bool notNull = false;
    bool indexed = false;
    Value defaultValue;
    bool is_nullable = true;
    bool is_indexed = false;
};


class CreateTableQuery : public ASTNode {
public:
    std::string tableName;
    std::vector<ColumnSpec> columns;
    CreateTableQuery();
    [[nodiscard]] std::string toString() const override;
};

class DropTableQuery : public ASTNode {
public:
    std::string tableName;
    explicit DropTableQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class CreateDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit CreateDatabaseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class DropDatabaseQuery : public ASTNode {
public:
    std::string dbName;
    explicit DropDatabaseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class UseQuery : public ASTNode {
public:
    std::string dbName;
    explicit UseQuery(const std::string& name);
    [[nodiscard]] std::string toString() const override;
};

class RevertQuery : public ASTNode {
public:
    std::string tableName;
    std::string targetTimestamp;
    explicit RevertQuery(std::string t, std::string ts);
    [[nodiscard]] std::string toString() const override;
};

#endif // DBMS_PARSER_AST_H
