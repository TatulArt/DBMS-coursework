// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.


/**
 ** \file parser.tab.hpp
 ** Define the yy::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_PARSER_TAB_HPP_INCLUDED
# define YY_YY_PARSER_TAB_HPP_INCLUDED
// "%code requires" blocks.
#line 12 "parser.y"

    #include "parser/AST.h"
    #include <memory>
    #include <vector>
    #include <string>
    #include "types.h"
    class SqlScanner;
    struct SelectItem {
        bool is_agg = false;
        SelectColumn col;
        AggregateExpr agg;
    };

#line 63 "parser.tab.hpp"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "location.hh"


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 3 "parser.y"
namespace yy {
#line 199 "parser.tab.hpp"




  /// A Bison parser.
  class parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class value_type
  {
  public:
    /// Type of *this.
    typedef value_type self_type;

    /// Empty construction.
    value_type () YY_NOEXCEPT
      : yyraw_ ()
    {}

    /// Construct and fill.
    template <typename T>
    value_type (YY_RVREF (T) t)
    {
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    value_type (const self_type&) = delete;
    /// Non copyable.
    self_type& operator= (const self_type&) = delete;
#endif

    /// Destruction, allowed only if empty.
    ~value_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    value_type (const self_type&);
    /// Non copyable.
    self_type& operator= (const self_type&);
#endif

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yyraw_;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yyraw_;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // aggregate_expr
      char dummy1[sizeof (AggregateExpr)];

      // type_spec
      char dummy2[sizeof (ColType)];

      // column_definition
      char dummy3[sizeof (ColumnSpec)];

      // select_item
      char dummy4[sizeof (SelectItem)];

      // default_opt
      char dummy5[sizeof (Value)];

      // not_null_opt
      // indexed_opt
      char dummy6[sizeof (bool)];

      // INTEGER
      char dummy7[sizeof (int)];

      // STRING
      // IDENTIFIER
      // TIMESTAMP
      // column_name_or_star
      char dummy8[sizeof (std::string)];

      // query
      // select_stmt
      // where_opt
      // condition
      // or_condition
      // and_condition
      // comparison
      // expr
      // literal
      // column_ref
      // insert_stmt
      // update_stmt
      // delete_stmt
      // create_table_stmt
      // drop_table_stmt
      // create_database_stmt
      // drop_database_stmt
      // use_stmt
      // revert_stmt
      char dummy9[sizeof (std::unique_ptr<ASTNode>)];

      // column_definitions
      char dummy10[sizeof (std::vector<ColumnSpec>)];

      // select_columns
      char dummy11[sizeof (std::vector<SelectItem>)];

      // assignment_list
      char dummy12[sizeof (std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>)];

      // opt_columns
      // column_list
      char dummy13[sizeof (std::vector<std::string>)];

      // value_list
      char dummy14[sizeof (std::vector<std::unique_ptr<ASTNode>>)];

      // values_list
      char dummy15[sizeof (std::vector<std::vector<std::unique_ptr<ASTNode>>>)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me_;
      /// A buffer large enough to store any of the semantic values.
      char yyraw_[size];
    };
  };

#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;

    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    END = 0,                       // "end of file"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    SELECT = 258,                  // SELECT
    INSERT = 259,                  // INSERT
    UPDATE = 260,                  // UPDATE
    DELETE = 261,                  // DELETE
    CREATE = 262,                  // CREATE
    DROP = 263,                    // DROP
    USE = 264,                     // USE
    DATABASE = 265,                // DATABASE
    TABLE = 266,                   // TABLE
    FROM = 267,                    // FROM
    WHERE = 268,                   // WHERE
    SET = 269,                     // SET
    VALUE = 270,                   // VALUE
    INTO = 271,                    // INTO
    AS = 272,                      // AS
    AND = 273,                     // AND
    OR = 274,                      // OR
    BETWEEN = 275,                 // BETWEEN
    LIKE = 276,                    // LIKE
    NOT = 277,                     // NOT
    NULL_ = 278,                   // NULL_
    INDEXED = 279,                 // INDEXED
    SUM = 280,                     // SUM
    COUNT = 281,                   // COUNT
    AVG = 282,                     // AVG
    DEFAULT = 283,                 // DEFAULT
    REVERT = 284,                  // REVERT
    EQ = 285,                      // EQ
    NE = 286,                      // NE
    LE = 287,                      // LE
    GE = 288,                      // GE
    LT = 289,                      // LT
    GT = 290,                      // GT
    ASSIGN = 291,                  // ASSIGN
    SEMICOLON = 292,               // SEMICOLON
    COMMA = 293,                   // COMMA
    LPAREN = 294,                  // LPAREN
    RPAREN = 295,                  // RPAREN
    STAR = 296,                    // STAR
    INTEGER = 297,                 // INTEGER
    STRING = 298,                  // STRING
    IDENTIFIER = 299,              // IDENTIFIER
    TIMESTAMP = 300,               // TIMESTAMP
    INT_TYPE = 301,                // INT_TYPE
    STRING_TYPE = 302,             // STRING_TYPE
    BETWEEN_PREC = 303             // BETWEEN_PREC
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 49, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_SELECT = 3,                            // SELECT
        S_INSERT = 4,                            // INSERT
        S_UPDATE = 5,                            // UPDATE
        S_DELETE = 6,                            // DELETE
        S_CREATE = 7,                            // CREATE
        S_DROP = 8,                              // DROP
        S_USE = 9,                               // USE
        S_DATABASE = 10,                         // DATABASE
        S_TABLE = 11,                            // TABLE
        S_FROM = 12,                             // FROM
        S_WHERE = 13,                            // WHERE
        S_SET = 14,                              // SET
        S_VALUE = 15,                            // VALUE
        S_INTO = 16,                             // INTO
        S_AS = 17,                               // AS
        S_AND = 18,                              // AND
        S_OR = 19,                               // OR
        S_BETWEEN = 20,                          // BETWEEN
        S_LIKE = 21,                             // LIKE
        S_NOT = 22,                              // NOT
        S_NULL_ = 23,                            // NULL_
        S_INDEXED = 24,                          // INDEXED
        S_SUM = 25,                              // SUM
        S_COUNT = 26,                            // COUNT
        S_AVG = 27,                              // AVG
        S_DEFAULT = 28,                          // DEFAULT
        S_REVERT = 29,                           // REVERT
        S_EQ = 30,                               // EQ
        S_NE = 31,                               // NE
        S_LE = 32,                               // LE
        S_GE = 33,                               // GE
        S_LT = 34,                               // LT
        S_GT = 35,                               // GT
        S_ASSIGN = 36,                           // ASSIGN
        S_SEMICOLON = 37,                        // SEMICOLON
        S_COMMA = 38,                            // COMMA
        S_LPAREN = 39,                           // LPAREN
        S_RPAREN = 40,                           // RPAREN
        S_STAR = 41,                             // STAR
        S_INTEGER = 42,                          // INTEGER
        S_STRING = 43,                           // STRING
        S_IDENTIFIER = 44,                       // IDENTIFIER
        S_TIMESTAMP = 45,                        // TIMESTAMP
        S_INT_TYPE = 46,                         // INT_TYPE
        S_STRING_TYPE = 47,                      // STRING_TYPE
        S_BETWEEN_PREC = 48,                     // BETWEEN_PREC
        S_YYACCEPT = 49,                         // $accept
        S_start = 50,                            // start
        S_query = 51,                            // query
        S_select_stmt = 52,                      // select_stmt
        S_select_columns = 53,                   // select_columns
        S_select_item = 54,                      // select_item
        S_aggregate_expr = 55,                   // aggregate_expr
        S_column_name_or_star = 56,              // column_name_or_star
        S_where_opt = 57,                        // where_opt
        S_condition = 58,                        // condition
        S_or_condition = 59,                     // or_condition
        S_and_condition = 60,                    // and_condition
        S_comparison = 61,                       // comparison
        S_expr = 62,                             // expr
        S_literal = 63,                          // literal
        S_column_ref = 64,                       // column_ref
        S_insert_stmt = 65,                      // insert_stmt
        S_opt_columns = 66,                      // opt_columns
        S_column_list = 67,                      // column_list
        S_values_list = 68,                      // values_list
        S_value_list = 69,                       // value_list
        S_update_stmt = 70,                      // update_stmt
        S_assignment_list = 71,                  // assignment_list
        S_delete_stmt = 72,                      // delete_stmt
        S_create_table_stmt = 73,                // create_table_stmt
        S_column_definitions = 74,               // column_definitions
        S_column_definition = 75,                // column_definition
        S_type_spec = 76,                        // type_spec
        S_not_null_opt = 77,                     // not_null_opt
        S_indexed_opt = 78,                      // indexed_opt
        S_default_opt = 79,                      // default_opt
        S_drop_table_stmt = 80,                  // drop_table_stmt
        S_create_database_stmt = 81,             // create_database_stmt
        S_drop_database_stmt = 82,               // drop_database_stmt
        S_use_stmt = 83,                         // use_stmt
        S_revert_stmt = 84                       // revert_stmt
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
        , location (std::move (that.location))
      {
        switch (this->kind ())
    {
      case symbol_kind::S_aggregate_expr: // aggregate_expr
        value.move< AggregateExpr > (std::move (that.value));
        break;

      case symbol_kind::S_type_spec: // type_spec
        value.move< ColType > (std::move (that.value));
        break;

      case symbol_kind::S_column_definition: // column_definition
        value.move< ColumnSpec > (std::move (that.value));
        break;

      case symbol_kind::S_select_item: // select_item
        value.move< SelectItem > (std::move (that.value));
        break;

      case symbol_kind::S_default_opt: // default_opt
        value.move< Value > (std::move (that.value));
        break;

      case symbol_kind::S_not_null_opt: // not_null_opt
      case symbol_kind::S_indexed_opt: // indexed_opt
        value.move< bool > (std::move (that.value));
        break;

      case symbol_kind::S_INTEGER: // INTEGER
        value.move< int > (std::move (that.value));
        break;

      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_TIMESTAMP: // TIMESTAMP
      case symbol_kind::S_column_name_or_star: // column_name_or_star
        value.move< std::string > (std::move (that.value));
        break;

      case symbol_kind::S_query: // query
      case symbol_kind::S_select_stmt: // select_stmt
      case symbol_kind::S_where_opt: // where_opt
      case symbol_kind::S_condition: // condition
      case symbol_kind::S_or_condition: // or_condition
      case symbol_kind::S_and_condition: // and_condition
      case symbol_kind::S_comparison: // comparison
      case symbol_kind::S_expr: // expr
      case symbol_kind::S_literal: // literal
      case symbol_kind::S_column_ref: // column_ref
      case symbol_kind::S_insert_stmt: // insert_stmt
      case symbol_kind::S_update_stmt: // update_stmt
      case symbol_kind::S_delete_stmt: // delete_stmt
      case symbol_kind::S_create_table_stmt: // create_table_stmt
      case symbol_kind::S_drop_table_stmt: // drop_table_stmt
      case symbol_kind::S_create_database_stmt: // create_database_stmt
      case symbol_kind::S_drop_database_stmt: // drop_database_stmt
      case symbol_kind::S_use_stmt: // use_stmt
      case symbol_kind::S_revert_stmt: // revert_stmt
        value.move< std::unique_ptr<ASTNode> > (std::move (that.value));
        break;

      case symbol_kind::S_column_definitions: // column_definitions
        value.move< std::vector<ColumnSpec> > (std::move (that.value));
        break;

      case symbol_kind::S_select_columns: // select_columns
        value.move< std::vector<SelectItem> > (std::move (that.value));
        break;

      case symbol_kind::S_assignment_list: // assignment_list
        value.move< std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> > (std::move (that.value));
        break;

      case symbol_kind::S_opt_columns: // opt_columns
      case symbol_kind::S_column_list: // column_list
        value.move< std::vector<std::string> > (std::move (that.value));
        break;

      case symbol_kind::S_value_list: // value_list
        value.move< std::vector<std::unique_ptr<ASTNode>> > (std::move (that.value));
        break;

      case symbol_kind::S_values_list: // values_list
        value.move< std::vector<std::vector<std::unique_ptr<ASTNode>>> > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructors for typed symbols.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, AggregateExpr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const AggregateExpr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ColType&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ColType& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ColumnSpec&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ColumnSpec& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, SelectItem&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const SelectItem& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, Value&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const Value& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, bool&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const bool& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<ASTNode>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<ASTNode>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<ColumnSpec>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<ColumnSpec>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<SelectItem>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<SelectItem>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::string>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::string>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::unique_ptr<ASTNode>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::unique_ptr<ASTNode>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::vector<std::unique_ptr<ASTNode>>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::vector<std::unique_ptr<ASTNode>>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        // User destructor.
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // Value type destructor.
switch (yykind)
    {
      case symbol_kind::S_aggregate_expr: // aggregate_expr
        value.template destroy< AggregateExpr > ();
        break;

      case symbol_kind::S_type_spec: // type_spec
        value.template destroy< ColType > ();
        break;

      case symbol_kind::S_column_definition: // column_definition
        value.template destroy< ColumnSpec > ();
        break;

      case symbol_kind::S_select_item: // select_item
        value.template destroy< SelectItem > ();
        break;

      case symbol_kind::S_default_opt: // default_opt
        value.template destroy< Value > ();
        break;

      case symbol_kind::S_not_null_opt: // not_null_opt
      case symbol_kind::S_indexed_opt: // indexed_opt
        value.template destroy< bool > ();
        break;

      case symbol_kind::S_INTEGER: // INTEGER
        value.template destroy< int > ();
        break;

      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_TIMESTAMP: // TIMESTAMP
      case symbol_kind::S_column_name_or_star: // column_name_or_star
        value.template destroy< std::string > ();
        break;

      case symbol_kind::S_query: // query
      case symbol_kind::S_select_stmt: // select_stmt
      case symbol_kind::S_where_opt: // where_opt
      case symbol_kind::S_condition: // condition
      case symbol_kind::S_or_condition: // or_condition
      case symbol_kind::S_and_condition: // and_condition
      case symbol_kind::S_comparison: // comparison
      case symbol_kind::S_expr: // expr
      case symbol_kind::S_literal: // literal
      case symbol_kind::S_column_ref: // column_ref
      case symbol_kind::S_insert_stmt: // insert_stmt
      case symbol_kind::S_update_stmt: // update_stmt
      case symbol_kind::S_delete_stmt: // delete_stmt
      case symbol_kind::S_create_table_stmt: // create_table_stmt
      case symbol_kind::S_drop_table_stmt: // drop_table_stmt
      case symbol_kind::S_create_database_stmt: // create_database_stmt
      case symbol_kind::S_drop_database_stmt: // drop_database_stmt
      case symbol_kind::S_use_stmt: // use_stmt
      case symbol_kind::S_revert_stmt: // revert_stmt
        value.template destroy< std::unique_ptr<ASTNode> > ();
        break;

      case symbol_kind::S_column_definitions: // column_definitions
        value.template destroy< std::vector<ColumnSpec> > ();
        break;

      case symbol_kind::S_select_columns: // select_columns
        value.template destroy< std::vector<SelectItem> > ();
        break;

      case symbol_kind::S_assignment_list: // assignment_list
        value.template destroy< std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> > ();
        break;

      case symbol_kind::S_opt_columns: // opt_columns
      case symbol_kind::S_column_list: // column_list
        value.template destroy< std::vector<std::string> > ();
        break;

      case symbol_kind::S_value_list: // value_list
        value.template destroy< std::vector<std::unique_ptr<ASTNode>> > ();
        break;

      case symbol_kind::S_values_list: // values_list
        value.template destroy< std::vector<std::vector<std::unique_ptr<ASTNode>>> > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

#if YYDEBUG || 0
      /// The user-facing name of this symbol.
      const char *name () const YY_NOEXCEPT
      {
        return parser::symbol_name (this->kind ());
      }
#endif // #if YYDEBUG || 0


      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {
      /// Superclass.
      typedef basic_symbol<by_kind> super_type;

      /// Empty symbol.
      symbol_type () YY_NOEXCEPT {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type (token_kind_type (tok), std::move (l))
#else
      symbol_type (int tok, const location_type& l)
        : super_type (token_kind_type (tok), l)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const int& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, std::string v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const std::string& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {}
    };

    /// Build a parser object.
    parser (std::unique_ptr<ASTNode>& result_yyarg, std::string& errorMsg_yyarg, SqlScanner& scanner_yyarg);
    virtual ~parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    parser (const parser&) = delete;
    /// Non copyable.
    parser& operator= (const parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

#if YYDEBUG || 0
    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static const char *symbol_name (symbol_kind_type yysymbol);
#endif // #if YYDEBUG || 0


    // Implementation of make_symbol for each token kind.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror (location_type l)
      {
        return symbol_type (token::YYerror, std::move (l));
      }
#else
      static
      symbol_type
      make_YYerror (const location_type& l)
      {
        return symbol_type (token::YYerror, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF (location_type l)
      {
        return symbol_type (token::YYUNDEF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYUNDEF (const location_type& l)
      {
        return symbol_type (token::YYUNDEF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SELECT (location_type l)
      {
        return symbol_type (token::SELECT, std::move (l));
      }
#else
      static
      symbol_type
      make_SELECT (const location_type& l)
      {
        return symbol_type (token::SELECT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INSERT (location_type l)
      {
        return symbol_type (token::INSERT, std::move (l));
      }
#else
      static
      symbol_type
      make_INSERT (const location_type& l)
      {
        return symbol_type (token::INSERT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UPDATE (location_type l)
      {
        return symbol_type (token::UPDATE, std::move (l));
      }
#else
      static
      symbol_type
      make_UPDATE (const location_type& l)
      {
        return symbol_type (token::UPDATE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DELETE (location_type l)
      {
        return symbol_type (token::DELETE, std::move (l));
      }
#else
      static
      symbol_type
      make_DELETE (const location_type& l)
      {
        return symbol_type (token::DELETE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CREATE (location_type l)
      {
        return symbol_type (token::CREATE, std::move (l));
      }
#else
      static
      symbol_type
      make_CREATE (const location_type& l)
      {
        return symbol_type (token::CREATE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DROP (location_type l)
      {
        return symbol_type (token::DROP, std::move (l));
      }
#else
      static
      symbol_type
      make_DROP (const location_type& l)
      {
        return symbol_type (token::DROP, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_USE (location_type l)
      {
        return symbol_type (token::USE, std::move (l));
      }
#else
      static
      symbol_type
      make_USE (const location_type& l)
      {
        return symbol_type (token::USE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DATABASE (location_type l)
      {
        return symbol_type (token::DATABASE, std::move (l));
      }
#else
      static
      symbol_type
      make_DATABASE (const location_type& l)
      {
        return symbol_type (token::DATABASE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TABLE (location_type l)
      {
        return symbol_type (token::TABLE, std::move (l));
      }
#else
      static
      symbol_type
      make_TABLE (const location_type& l)
      {
        return symbol_type (token::TABLE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FROM (location_type l)
      {
        return symbol_type (token::FROM, std::move (l));
      }
#else
      static
      symbol_type
      make_FROM (const location_type& l)
      {
        return symbol_type (token::FROM, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHERE (location_type l)
      {
        return symbol_type (token::WHERE, std::move (l));
      }
#else
      static
      symbol_type
      make_WHERE (const location_type& l)
      {
        return symbol_type (token::WHERE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SET (location_type l)
      {
        return symbol_type (token::SET, std::move (l));
      }
#else
      static
      symbol_type
      make_SET (const location_type& l)
      {
        return symbol_type (token::SET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_VALUE (location_type l)
      {
        return symbol_type (token::VALUE, std::move (l));
      }
#else
      static
      symbol_type
      make_VALUE (const location_type& l)
      {
        return symbol_type (token::VALUE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTO (location_type l)
      {
        return symbol_type (token::INTO, std::move (l));
      }
#else
      static
      symbol_type
      make_INTO (const location_type& l)
      {
        return symbol_type (token::INTO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AS (location_type l)
      {
        return symbol_type (token::AS, std::move (l));
      }
#else
      static
      symbol_type
      make_AS (const location_type& l)
      {
        return symbol_type (token::AS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND (location_type l)
      {
        return symbol_type (token::AND, std::move (l));
      }
#else
      static
      symbol_type
      make_AND (const location_type& l)
      {
        return symbol_type (token::AND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR (location_type l)
      {
        return symbol_type (token::OR, std::move (l));
      }
#else
      static
      symbol_type
      make_OR (const location_type& l)
      {
        return symbol_type (token::OR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BETWEEN (location_type l)
      {
        return symbol_type (token::BETWEEN, std::move (l));
      }
#else
      static
      symbol_type
      make_BETWEEN (const location_type& l)
      {
        return symbol_type (token::BETWEEN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LIKE (location_type l)
      {
        return symbol_type (token::LIKE, std::move (l));
      }
#else
      static
      symbol_type
      make_LIKE (const location_type& l)
      {
        return symbol_type (token::LIKE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (location_type l)
      {
        return symbol_type (token::NOT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const location_type& l)
      {
        return symbol_type (token::NOT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NULL_ (location_type l)
      {
        return symbol_type (token::NULL_, std::move (l));
      }
#else
      static
      symbol_type
      make_NULL_ (const location_type& l)
      {
        return symbol_type (token::NULL_, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INDEXED (location_type l)
      {
        return symbol_type (token::INDEXED, std::move (l));
      }
#else
      static
      symbol_type
      make_INDEXED (const location_type& l)
      {
        return symbol_type (token::INDEXED, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SUM (location_type l)
      {
        return symbol_type (token::SUM, std::move (l));
      }
#else
      static
      symbol_type
      make_SUM (const location_type& l)
      {
        return symbol_type (token::SUM, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COUNT (location_type l)
      {
        return symbol_type (token::COUNT, std::move (l));
      }
#else
      static
      symbol_type
      make_COUNT (const location_type& l)
      {
        return symbol_type (token::COUNT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AVG (location_type l)
      {
        return symbol_type (token::AVG, std::move (l));
      }
#else
      static
      symbol_type
      make_AVG (const location_type& l)
      {
        return symbol_type (token::AVG, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DEFAULT (location_type l)
      {
        return symbol_type (token::DEFAULT, std::move (l));
      }
#else
      static
      symbol_type
      make_DEFAULT (const location_type& l)
      {
        return symbol_type (token::DEFAULT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_REVERT (location_type l)
      {
        return symbol_type (token::REVERT, std::move (l));
      }
#else
      static
      symbol_type
      make_REVERT (const location_type& l)
      {
        return symbol_type (token::REVERT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EQ (location_type l)
      {
        return symbol_type (token::EQ, std::move (l));
      }
#else
      static
      symbol_type
      make_EQ (const location_type& l)
      {
        return symbol_type (token::EQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NE (location_type l)
      {
        return symbol_type (token::NE, std::move (l));
      }
#else
      static
      symbol_type
      make_NE (const location_type& l)
      {
        return symbol_type (token::NE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LE (location_type l)
      {
        return symbol_type (token::LE, std::move (l));
      }
#else
      static
      symbol_type
      make_LE (const location_type& l)
      {
        return symbol_type (token::LE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GE (location_type l)
      {
        return symbol_type (token::GE, std::move (l));
      }
#else
      static
      symbol_type
      make_GE (const location_type& l)
      {
        return symbol_type (token::GE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LT (location_type l)
      {
        return symbol_type (token::LT, std::move (l));
      }
#else
      static
      symbol_type
      make_LT (const location_type& l)
      {
        return symbol_type (token::LT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GT (location_type l)
      {
        return symbol_type (token::GT, std::move (l));
      }
#else
      static
      symbol_type
      make_GT (const location_type& l)
      {
        return symbol_type (token::GT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (location_type l)
      {
        return symbol_type (token::ASSIGN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const location_type& l)
      {
        return symbol_type (token::ASSIGN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMICOLON (location_type l)
      {
        return symbol_type (token::SEMICOLON, std::move (l));
      }
#else
      static
      symbol_type
      make_SEMICOLON (const location_type& l)
      {
        return symbol_type (token::SEMICOLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (location_type l)
      {
        return symbol_type (token::LPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const location_type& l)
      {
        return symbol_type (token::LPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (location_type l)
      {
        return symbol_type (token::RPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const location_type& l)
      {
        return symbol_type (token::RPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STAR (location_type l)
      {
        return symbol_type (token::STAR, std::move (l));
      }
#else
      static
      symbol_type
      make_STAR (const location_type& l)
      {
        return symbol_type (token::STAR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTEGER (int v, location_type l)
      {
        return symbol_type (token::INTEGER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INTEGER (const int& v, const location_type& l)
      {
        return symbol_type (token::INTEGER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING (std::string v, location_type l)
      {
        return symbol_type (token::STRING, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRING (const std::string& v, const location_type& l)
      {
        return symbol_type (token::STRING, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENTIFIER (std::string v, location_type l)
      {
        return symbol_type (token::IDENTIFIER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENTIFIER (const std::string& v, const location_type& l)
      {
        return symbol_type (token::IDENTIFIER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TIMESTAMP (std::string v, location_type l)
      {
        return symbol_type (token::TIMESTAMP, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_TIMESTAMP (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TIMESTAMP, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT_TYPE (location_type l)
      {
        return symbol_type (token::INT_TYPE, std::move (l));
      }
#else
      static
      symbol_type
      make_INT_TYPE (const location_type& l)
      {
        return symbol_type (token::INT_TYPE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING_TYPE (location_type l)
      {
        return symbol_type (token::STRING_TYPE, std::move (l));
      }
#else
      static
      symbol_type
      make_STRING_TYPE (const location_type& l)
      {
        return symbol_type (token::STRING_TYPE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BETWEEN_PREC (location_type l)
      {
        return symbol_type (token::BETWEEN_PREC, std::move (l));
      }
#else
      static
      symbol_type
      make_BETWEEN_PREC (const location_type& l)
      {
        return symbol_type (token::BETWEEN_PREC, l);
      }
#endif


  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    parser (const parser&);
    /// Non copyable.
    parser& operator= (const parser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef unsigned char state_type;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

#if YYDEBUG || 0
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif // #if YYDEBUG || 0


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const signed char yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const signed char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const signed char yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const unsigned char yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const unsigned char yytable_[];

    static const unsigned char yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const signed char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 137,     ///< Last index in yytable_.
      yynnts_ = 36,  ///< Number of nonterminal symbols.
      yyfinal_ = 38 ///< Termination state number.
    };


    // User arguments.
    std::unique_ptr<ASTNode>& result;
    std::string& errorMsg;
    SqlScanner& scanner;

  };


#line 3 "parser.y"
} // yy
#line 2340 "parser.tab.hpp"




#endif // !YY_YY_PARSER_TAB_HPP_INCLUDED
// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.


/**
 ** \file parser.tab.hpp
 ** Define the yy::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_PARSER_TAB_HPP_INCLUDED
# define YY_YY_PARSER_TAB_HPP_INCLUDED
// "%code requires" blocks.
#line 12 "parser.y"

    #include "parser/AST.h"
    #include <memory>
    #include <vector>
    #include <string>
    #include "types.h"
    class SqlScanner;
    struct SelectItem {
        bool is_agg = false;
        SelectColumn col;
        AggregateExpr agg;
    };

#line 63 "parser.tab.hpp"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "location.hh"


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 3 "parser.y"
namespace yy {
#line 199 "parser.tab.hpp"




  /// A Bison parser.
  class parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class value_type
  {
  public:
    /// Type of *this.
    typedef value_type self_type;

    /// Empty construction.
    value_type () YY_NOEXCEPT
      : yyraw_ ()
    {}

    /// Construct and fill.
    template <typename T>
    value_type (YY_RVREF (T) t)
    {
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    value_type (const self_type&) = delete;
    /// Non copyable.
    self_type& operator= (const self_type&) = delete;
#endif

    /// Destruction, allowed only if empty.
    ~value_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    value_type (const self_type&);
    /// Non copyable.
    self_type& operator= (const self_type&);
#endif

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yyraw_;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yyraw_;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // aggregate_expr
      char dummy1[sizeof (AggregateExpr)];

      // type_spec
      char dummy2[sizeof (ColType)];

      // column_definition
      char dummy3[sizeof (ColumnSpec)];

      // select_item
      char dummy4[sizeof (SelectItem)];

      // default_opt
      char dummy5[sizeof (Value)];

      // not_null_opt
      // indexed_opt
      char dummy6[sizeof (bool)];

      // INTEGER
      char dummy7[sizeof (int)];

      // STRING
      // IDENTIFIER
      // TIMESTAMP
      // column_name_or_star
      char dummy8[sizeof (std::string)];

      // query
      // select_stmt
      // where_opt
      // condition
      // or_condition
      // and_condition
      // comparison
      // expr
      // literal
      // column_ref
      // insert_stmt
      // update_stmt
      // delete_stmt
      // create_table_stmt
      // drop_table_stmt
      // create_database_stmt
      // drop_database_stmt
      // use_stmt
      // revert_stmt
      char dummy9[sizeof (std::unique_ptr<ASTNode>)];

      // column_definitions
      char dummy10[sizeof (std::vector<ColumnSpec>)];

      // select_columns
      char dummy11[sizeof (std::vector<SelectItem>)];

      // assignment_list
      char dummy12[sizeof (std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>)];

      // opt_columns
      // column_list
      char dummy13[sizeof (std::vector<std::string>)];

      // value_list
      char dummy14[sizeof (std::vector<std::unique_ptr<ASTNode>>)];

      // values_list
      char dummy15[sizeof (std::vector<std::vector<std::unique_ptr<ASTNode>>>)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me_;
      /// A buffer large enough to store any of the semantic values.
      char yyraw_[size];
    };
  };

#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;

    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    END = 0,                       // "end of file"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    SELECT = 258,                  // SELECT
    INSERT = 259,                  // INSERT
    UPDATE = 260,                  // UPDATE
    DELETE = 261,                  // DELETE
    CREATE = 262,                  // CREATE
    DROP = 263,                    // DROP
    USE = 264,                     // USE
    DATABASE = 265,                // DATABASE
    TABLE = 266,                   // TABLE
    FROM = 267,                    // FROM
    WHERE = 268,                   // WHERE
    SET = 269,                     // SET
    VALUE = 270,                   // VALUE
    INTO = 271,                    // INTO
    AS = 272,                      // AS
    AND = 273,                     // AND
    OR = 274,                      // OR
    BETWEEN = 275,                 // BETWEEN
    LIKE = 276,                    // LIKE
    NOT = 277,                     // NOT
    NULL_ = 278,                   // NULL_
    INDEXED = 279,                 // INDEXED
    SUM = 280,                     // SUM
    COUNT = 281,                   // COUNT
    AVG = 282,                     // AVG
    DEFAULT = 283,                 // DEFAULT
    REVERT = 284,                  // REVERT
    EQ = 285,                      // EQ
    NE = 286,                      // NE
    LE = 287,                      // LE
    GE = 288,                      // GE
    LT = 289,                      // LT
    GT = 290,                      // GT
    ASSIGN = 291,                  // ASSIGN
    SEMICOLON = 292,               // SEMICOLON
    COMMA = 293,                   // COMMA
    LPAREN = 294,                  // LPAREN
    RPAREN = 295,                  // RPAREN
    STAR = 296,                    // STAR
    INTEGER = 297,                 // INTEGER
    STRING = 298,                  // STRING
    IDENTIFIER = 299,              // IDENTIFIER
    TIMESTAMP = 300,               // TIMESTAMP
    INT_TYPE = 301,                // INT_TYPE
    STRING_TYPE = 302,             // STRING_TYPE
    BETWEEN_PREC = 303             // BETWEEN_PREC
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 49, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_SELECT = 3,                            // SELECT
        S_INSERT = 4,                            // INSERT
        S_UPDATE = 5,                            // UPDATE
        S_DELETE = 6,                            // DELETE
        S_CREATE = 7,                            // CREATE
        S_DROP = 8,                              // DROP
        S_USE = 9,                               // USE
        S_DATABASE = 10,                         // DATABASE
        S_TABLE = 11,                            // TABLE
        S_FROM = 12,                             // FROM
        S_WHERE = 13,                            // WHERE
        S_SET = 14,                              // SET
        S_VALUE = 15,                            // VALUE
        S_INTO = 16,                             // INTO
        S_AS = 17,                               // AS
        S_AND = 18,                              // AND
        S_OR = 19,                               // OR
        S_BETWEEN = 20,                          // BETWEEN
        S_LIKE = 21,                             // LIKE
        S_NOT = 22,                              // NOT
        S_NULL_ = 23,                            // NULL_
        S_INDEXED = 24,                          // INDEXED
        S_SUM = 25,                              // SUM
        S_COUNT = 26,                            // COUNT
        S_AVG = 27,                              // AVG
        S_DEFAULT = 28,                          // DEFAULT
        S_REVERT = 29,                           // REVERT
        S_EQ = 30,                               // EQ
        S_NE = 31,                               // NE
        S_LE = 32,                               // LE
        S_GE = 33,                               // GE
        S_LT = 34,                               // LT
        S_GT = 35,                               // GT
        S_ASSIGN = 36,                           // ASSIGN
        S_SEMICOLON = 37,                        // SEMICOLON
        S_COMMA = 38,                            // COMMA
        S_LPAREN = 39,                           // LPAREN
        S_RPAREN = 40,                           // RPAREN
        S_STAR = 41,                             // STAR
        S_INTEGER = 42,                          // INTEGER
        S_STRING = 43,                           // STRING
        S_IDENTIFIER = 44,                       // IDENTIFIER
        S_TIMESTAMP = 45,                        // TIMESTAMP
        S_INT_TYPE = 46,                         // INT_TYPE
        S_STRING_TYPE = 47,                      // STRING_TYPE
        S_BETWEEN_PREC = 48,                     // BETWEEN_PREC
        S_YYACCEPT = 49,                         // $accept
        S_start = 50,                            // start
        S_query = 51,                            // query
        S_select_stmt = 52,                      // select_stmt
        S_select_columns = 53,                   // select_columns
        S_select_item = 54,                      // select_item
        S_aggregate_expr = 55,                   // aggregate_expr
        S_column_name_or_star = 56,              // column_name_or_star
        S_where_opt = 57,                        // where_opt
        S_condition = 58,                        // condition
        S_or_condition = 59,                     // or_condition
        S_and_condition = 60,                    // and_condition
        S_comparison = 61,                       // comparison
        S_expr = 62,                             // expr
        S_literal = 63,                          // literal
        S_column_ref = 64,                       // column_ref
        S_insert_stmt = 65,                      // insert_stmt
        S_opt_columns = 66,                      // opt_columns
        S_column_list = 67,                      // column_list
        S_values_list = 68,                      // values_list
        S_value_list = 69,                       // value_list
        S_update_stmt = 70,                      // update_stmt
        S_assignment_list = 71,                  // assignment_list
        S_delete_stmt = 72,                      // delete_stmt
        S_create_table_stmt = 73,                // create_table_stmt
        S_column_definitions = 74,               // column_definitions
        S_column_definition = 75,                // column_definition
        S_type_spec = 76,                        // type_spec
        S_not_null_opt = 77,                     // not_null_opt
        S_indexed_opt = 78,                      // indexed_opt
        S_default_opt = 79,                      // default_opt
        S_drop_table_stmt = 80,                  // drop_table_stmt
        S_create_database_stmt = 81,             // create_database_stmt
        S_drop_database_stmt = 82,               // drop_database_stmt
        S_use_stmt = 83,                         // use_stmt
        S_revert_stmt = 84                       // revert_stmt
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
        , location (std::move (that.location))
      {
        switch (this->kind ())
    {
      case symbol_kind::S_aggregate_expr: // aggregate_expr
        value.move< AggregateExpr > (std::move (that.value));
        break;

      case symbol_kind::S_type_spec: // type_spec
        value.move< ColType > (std::move (that.value));
        break;

      case symbol_kind::S_column_definition: // column_definition
        value.move< ColumnSpec > (std::move (that.value));
        break;

      case symbol_kind::S_select_item: // select_item
        value.move< SelectItem > (std::move (that.value));
        break;

      case symbol_kind::S_default_opt: // default_opt
        value.move< Value > (std::move (that.value));
        break;

      case symbol_kind::S_not_null_opt: // not_null_opt
      case symbol_kind::S_indexed_opt: // indexed_opt
        value.move< bool > (std::move (that.value));
        break;

      case symbol_kind::S_INTEGER: // INTEGER
        value.move< int > (std::move (that.value));
        break;

      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_TIMESTAMP: // TIMESTAMP
      case symbol_kind::S_column_name_or_star: // column_name_or_star
        value.move< std::string > (std::move (that.value));
        break;

      case symbol_kind::S_query: // query
      case symbol_kind::S_select_stmt: // select_stmt
      case symbol_kind::S_where_opt: // where_opt
      case symbol_kind::S_condition: // condition
      case symbol_kind::S_or_condition: // or_condition
      case symbol_kind::S_and_condition: // and_condition
      case symbol_kind::S_comparison: // comparison
      case symbol_kind::S_expr: // expr
      case symbol_kind::S_literal: // literal
      case symbol_kind::S_column_ref: // column_ref
      case symbol_kind::S_insert_stmt: // insert_stmt
      case symbol_kind::S_update_stmt: // update_stmt
      case symbol_kind::S_delete_stmt: // delete_stmt
      case symbol_kind::S_create_table_stmt: // create_table_stmt
      case symbol_kind::S_drop_table_stmt: // drop_table_stmt
      case symbol_kind::S_create_database_stmt: // create_database_stmt
      case symbol_kind::S_drop_database_stmt: // drop_database_stmt
      case symbol_kind::S_use_stmt: // use_stmt
      case symbol_kind::S_revert_stmt: // revert_stmt
        value.move< std::unique_ptr<ASTNode> > (std::move (that.value));
        break;

      case symbol_kind::S_column_definitions: // column_definitions
        value.move< std::vector<ColumnSpec> > (std::move (that.value));
        break;

      case symbol_kind::S_select_columns: // select_columns
        value.move< std::vector<SelectItem> > (std::move (that.value));
        break;

      case symbol_kind::S_assignment_list: // assignment_list
        value.move< std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> > (std::move (that.value));
        break;

      case symbol_kind::S_opt_columns: // opt_columns
      case symbol_kind::S_column_list: // column_list
        value.move< std::vector<std::string> > (std::move (that.value));
        break;

      case symbol_kind::S_value_list: // value_list
        value.move< std::vector<std::unique_ptr<ASTNode>> > (std::move (that.value));
        break;

      case symbol_kind::S_values_list: // values_list
        value.move< std::vector<std::vector<std::unique_ptr<ASTNode>>> > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructors for typed symbols.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, AggregateExpr&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const AggregateExpr& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ColType&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ColType& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ColumnSpec&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ColumnSpec& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, SelectItem&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const SelectItem& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, Value&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const Value& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, bool&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const bool& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<ASTNode>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<ASTNode>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<ColumnSpec>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<ColumnSpec>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<SelectItem>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<SelectItem>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::string>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::string>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::unique_ptr<ASTNode>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::unique_ptr<ASTNode>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::vector<std::unique_ptr<ASTNode>>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::vector<std::unique_ptr<ASTNode>>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        // User destructor.
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // Value type destructor.
switch (yykind)
    {
      case symbol_kind::S_aggregate_expr: // aggregate_expr
        value.template destroy< AggregateExpr > ();
        break;

      case symbol_kind::S_type_spec: // type_spec
        value.template destroy< ColType > ();
        break;

      case symbol_kind::S_column_definition: // column_definition
        value.template destroy< ColumnSpec > ();
        break;

      case symbol_kind::S_select_item: // select_item
        value.template destroy< SelectItem > ();
        break;

      case symbol_kind::S_default_opt: // default_opt
        value.template destroy< Value > ();
        break;

      case symbol_kind::S_not_null_opt: // not_null_opt
      case symbol_kind::S_indexed_opt: // indexed_opt
        value.template destroy< bool > ();
        break;

      case symbol_kind::S_INTEGER: // INTEGER
        value.template destroy< int > ();
        break;

      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
      case symbol_kind::S_TIMESTAMP: // TIMESTAMP
      case symbol_kind::S_column_name_or_star: // column_name_or_star
        value.template destroy< std::string > ();
        break;

      case symbol_kind::S_query: // query
      case symbol_kind::S_select_stmt: // select_stmt
      case symbol_kind::S_where_opt: // where_opt
      case symbol_kind::S_condition: // condition
      case symbol_kind::S_or_condition: // or_condition
      case symbol_kind::S_and_condition: // and_condition
      case symbol_kind::S_comparison: // comparison
      case symbol_kind::S_expr: // expr
      case symbol_kind::S_literal: // literal
      case symbol_kind::S_column_ref: // column_ref
      case symbol_kind::S_insert_stmt: // insert_stmt
      case symbol_kind::S_update_stmt: // update_stmt
      case symbol_kind::S_delete_stmt: // delete_stmt
      case symbol_kind::S_create_table_stmt: // create_table_stmt
      case symbol_kind::S_drop_table_stmt: // drop_table_stmt
      case symbol_kind::S_create_database_stmt: // create_database_stmt
      case symbol_kind::S_drop_database_stmt: // drop_database_stmt
      case symbol_kind::S_use_stmt: // use_stmt
      case symbol_kind::S_revert_stmt: // revert_stmt
        value.template destroy< std::unique_ptr<ASTNode> > ();
        break;

      case symbol_kind::S_column_definitions: // column_definitions
        value.template destroy< std::vector<ColumnSpec> > ();
        break;

      case symbol_kind::S_select_columns: // select_columns
        value.template destroy< std::vector<SelectItem> > ();
        break;

      case symbol_kind::S_assignment_list: // assignment_list
        value.template destroy< std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> > ();
        break;

      case symbol_kind::S_opt_columns: // opt_columns
      case symbol_kind::S_column_list: // column_list
        value.template destroy< std::vector<std::string> > ();
        break;

      case symbol_kind::S_value_list: // value_list
        value.template destroy< std::vector<std::unique_ptr<ASTNode>> > ();
        break;

      case symbol_kind::S_values_list: // values_list
        value.template destroy< std::vector<std::vector<std::unique_ptr<ASTNode>>> > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

#if YYDEBUG || 0
      /// The user-facing name of this symbol.
      const char *name () const YY_NOEXCEPT
      {
        return parser::symbol_name (this->kind ());
      }
#endif // #if YYDEBUG || 0


      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {
      /// Superclass.
      typedef basic_symbol<by_kind> super_type;

      /// Empty symbol.
      symbol_type () YY_NOEXCEPT {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type (token_kind_type (tok), std::move (l))
#else
      symbol_type (int tok, const location_type& l)
        : super_type (token_kind_type (tok), l)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const int& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, std::string v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const std::string& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {}
    };

    /// Build a parser object.
    parser (std::unique_ptr<ASTNode>& result_yyarg, std::string& errorMsg_yyarg, SqlScanner& scanner_yyarg);
    virtual ~parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    parser (const parser&) = delete;
    /// Non copyable.
    parser& operator= (const parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

#if YYDEBUG || 0
    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static const char *symbol_name (symbol_kind_type yysymbol);
#endif // #if YYDEBUG || 0


    // Implementation of make_symbol for each token kind.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror (location_type l)
      {
        return symbol_type (token::YYerror, std::move (l));
      }
#else
      static
      symbol_type
      make_YYerror (const location_type& l)
      {
        return symbol_type (token::YYerror, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF (location_type l)
      {
        return symbol_type (token::YYUNDEF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYUNDEF (const location_type& l)
      {
        return symbol_type (token::YYUNDEF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SELECT (location_type l)
      {
        return symbol_type (token::SELECT, std::move (l));
      }
#else
      static
      symbol_type
      make_SELECT (const location_type& l)
      {
        return symbol_type (token::SELECT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INSERT (location_type l)
      {
        return symbol_type (token::INSERT, std::move (l));
      }
#else
      static
      symbol_type
      make_INSERT (const location_type& l)
      {
        return symbol_type (token::INSERT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UPDATE (location_type l)
      {
        return symbol_type (token::UPDATE, std::move (l));
      }
#else
      static
      symbol_type
      make_UPDATE (const location_type& l)
      {
        return symbol_type (token::UPDATE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DELETE (location_type l)
      {
        return symbol_type (token::DELETE, std::move (l));
      }
#else
      static
      symbol_type
      make_DELETE (const location_type& l)
      {
        return symbol_type (token::DELETE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CREATE (location_type l)
      {
        return symbol_type (token::CREATE, std::move (l));
      }
#else
      static
      symbol_type
      make_CREATE (const location_type& l)
      {
        return symbol_type (token::CREATE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DROP (location_type l)
      {
        return symbol_type (token::DROP, std::move (l));
      }
#else
      static
      symbol_type
      make_DROP (const location_type& l)
      {
        return symbol_type (token::DROP, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_USE (location_type l)
      {
        return symbol_type (token::USE, std::move (l));
      }
#else
      static
      symbol_type
      make_USE (const location_type& l)
      {
        return symbol_type (token::USE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DATABASE (location_type l)
      {
        return symbol_type (token::DATABASE, std::move (l));
      }
#else
      static
      symbol_type
      make_DATABASE (const location_type& l)
      {
        return symbol_type (token::DATABASE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TABLE (location_type l)
      {
        return symbol_type (token::TABLE, std::move (l));
      }
#else
      static
      symbol_type
      make_TABLE (const location_type& l)
      {
        return symbol_type (token::TABLE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FROM (location_type l)
      {
        return symbol_type (token::FROM, std::move (l));
      }
#else
      static
      symbol_type
      make_FROM (const location_type& l)
      {
        return symbol_type (token::FROM, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHERE (location_type l)
      {
        return symbol_type (token::WHERE, std::move (l));
      }
#else
      static
      symbol_type
      make_WHERE (const location_type& l)
      {
        return symbol_type (token::WHERE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SET (location_type l)
      {
        return symbol_type (token::SET, std::move (l));
      }
#else
      static
      symbol_type
      make_SET (const location_type& l)
      {
        return symbol_type (token::SET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_VALUE (location_type l)
      {
        return symbol_type (token::VALUE, std::move (l));
      }
#else
      static
      symbol_type
      make_VALUE (const location_type& l)
      {
        return symbol_type (token::VALUE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTO (location_type l)
      {
        return symbol_type (token::INTO, std::move (l));
      }
#else
      static
      symbol_type
      make_INTO (const location_type& l)
      {
        return symbol_type (token::INTO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AS (location_type l)
      {
        return symbol_type (token::AS, std::move (l));
      }
#else
      static
      symbol_type
      make_AS (const location_type& l)
      {
        return symbol_type (token::AS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND (location_type l)
      {
        return symbol_type (token::AND, std::move (l));
      }
#else
      static
      symbol_type
      make_AND (const location_type& l)
      {
        return symbol_type (token::AND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR (location_type l)
      {
        return symbol_type (token::OR, std::move (l));
      }
#else
      static
      symbol_type
      make_OR (const location_type& l)
      {
        return symbol_type (token::OR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BETWEEN (location_type l)
      {
        return symbol_type (token::BETWEEN, std::move (l));
      }
#else
      static
      symbol_type
      make_BETWEEN (const location_type& l)
      {
        return symbol_type (token::BETWEEN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LIKE (location_type l)
      {
        return symbol_type (token::LIKE, std::move (l));
      }
#else
      static
      symbol_type
      make_LIKE (const location_type& l)
      {
        return symbol_type (token::LIKE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (location_type l)
      {
        return symbol_type (token::NOT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const location_type& l)
      {
        return symbol_type (token::NOT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NULL_ (location_type l)
      {
        return symbol_type (token::NULL_, std::move (l));
      }
#else
      static
      symbol_type
      make_NULL_ (const location_type& l)
      {
        return symbol_type (token::NULL_, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INDEXED (location_type l)
      {
        return symbol_type (token::INDEXED, std::move (l));
      }
#else
      static
      symbol_type
      make_INDEXED (const location_type& l)
      {
        return symbol_type (token::INDEXED, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SUM (location_type l)
      {
        return symbol_type (token::SUM, std::move (l));
      }
#else
      static
      symbol_type
      make_SUM (const location_type& l)
      {
        return symbol_type (token::SUM, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COUNT (location_type l)
      {
        return symbol_type (token::COUNT, std::move (l));
      }
#else
      static
      symbol_type
      make_COUNT (const location_type& l)
      {
        return symbol_type (token::COUNT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AVG (location_type l)
      {
        return symbol_type (token::AVG, std::move (l));
      }
#else
      static
      symbol_type
      make_AVG (const location_type& l)
      {
        return symbol_type (token::AVG, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DEFAULT (location_type l)
      {
        return symbol_type (token::DEFAULT, std::move (l));
      }
#else
      static
      symbol_type
      make_DEFAULT (const location_type& l)
      {
        return symbol_type (token::DEFAULT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_REVERT (location_type l)
      {
        return symbol_type (token::REVERT, std::move (l));
      }
#else
      static
      symbol_type
      make_REVERT (const location_type& l)
      {
        return symbol_type (token::REVERT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EQ (location_type l)
      {
        return symbol_type (token::EQ, std::move (l));
      }
#else
      static
      symbol_type
      make_EQ (const location_type& l)
      {
        return symbol_type (token::EQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NE (location_type l)
      {
        return symbol_type (token::NE, std::move (l));
      }
#else
      static
      symbol_type
      make_NE (const location_type& l)
      {
        return symbol_type (token::NE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LE (location_type l)
      {
        return symbol_type (token::LE, std::move (l));
      }
#else
      static
      symbol_type
      make_LE (const location_type& l)
      {
        return symbol_type (token::LE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GE (location_type l)
      {
        return symbol_type (token::GE, std::move (l));
      }
#else
      static
      symbol_type
      make_GE (const location_type& l)
      {
        return symbol_type (token::GE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LT (location_type l)
      {
        return symbol_type (token::LT, std::move (l));
      }
#else
      static
      symbol_type
      make_LT (const location_type& l)
      {
        return symbol_type (token::LT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GT (location_type l)
      {
        return symbol_type (token::GT, std::move (l));
      }
#else
      static
      symbol_type
      make_GT (const location_type& l)
      {
        return symbol_type (token::GT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (location_type l)
      {
        return symbol_type (token::ASSIGN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const location_type& l)
      {
        return symbol_type (token::ASSIGN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMICOLON (location_type l)
      {
        return symbol_type (token::SEMICOLON, std::move (l));
      }
#else
      static
      symbol_type
      make_SEMICOLON (const location_type& l)
      {
        return symbol_type (token::SEMICOLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (location_type l)
      {
        return symbol_type (token::LPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const location_type& l)
      {
        return symbol_type (token::LPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (location_type l)
      {
        return symbol_type (token::RPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const location_type& l)
      {
        return symbol_type (token::RPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STAR (location_type l)
      {
        return symbol_type (token::STAR, std::move (l));
      }
#else
      static
      symbol_type
      make_STAR (const location_type& l)
      {
        return symbol_type (token::STAR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTEGER (int v, location_type l)
      {
        return symbol_type (token::INTEGER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INTEGER (const int& v, const location_type& l)
      {
        return symbol_type (token::INTEGER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING (std::string v, location_type l)
      {
        return symbol_type (token::STRING, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRING (const std::string& v, const location_type& l)
      {
        return symbol_type (token::STRING, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENTIFIER (std::string v, location_type l)
      {
        return symbol_type (token::IDENTIFIER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENTIFIER (const std::string& v, const location_type& l)
      {
        return symbol_type (token::IDENTIFIER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TIMESTAMP (std::string v, location_type l)
      {
        return symbol_type (token::TIMESTAMP, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_TIMESTAMP (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TIMESTAMP, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT_TYPE (location_type l)
      {
        return symbol_type (token::INT_TYPE, std::move (l));
      }
#else
      static
      symbol_type
      make_INT_TYPE (const location_type& l)
      {
        return symbol_type (token::INT_TYPE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING_TYPE (location_type l)
      {
        return symbol_type (token::STRING_TYPE, std::move (l));
      }
#else
      static
      symbol_type
      make_STRING_TYPE (const location_type& l)
      {
        return symbol_type (token::STRING_TYPE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BETWEEN_PREC (location_type l)
      {
        return symbol_type (token::BETWEEN_PREC, std::move (l));
      }
#else
      static
      symbol_type
      make_BETWEEN_PREC (const location_type& l)
      {
        return symbol_type (token::BETWEEN_PREC, l);
      }
#endif


  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    parser (const parser&);
    /// Non copyable.
    parser& operator= (const parser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef unsigned char state_type;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

#if YYDEBUG || 0
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif // #if YYDEBUG || 0


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const signed char yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const signed char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const signed char yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const unsigned char yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const unsigned char yytable_[];

    static const unsigned char yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const signed char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 137,     ///< Last index in yytable_.
      yynnts_ = 36,  ///< Number of nonterminal symbols.
      yyfinal_ = 38 ///< Termination state number.
    };


    // User arguments.
    std::unique_ptr<ASTNode>& result;
    std::string& errorMsg;
    SqlScanner& scanner;

  };


#line 3 "parser.y"
} // yy
#line 2340 "parser.tab.hpp"




#endif // !YY_YY_PARSER_TAB_HPP_INCLUDED
