%{
#undef YY_DECL
#define YY_DECL int SqlScanner::yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yyloc)

#include "parser/SqlScanner.h"
#include "parser.tab.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>

void yyerror(const char* msg);

// Вспомогательная функция для проверки регистра и возврата токена
int process_keyword_or_ident(const std::string& text, yy::parser::semantic_type* yylval) {
    // Карта всех поддерживаемых ключевых слов СУБД (в верхнем регистре)
    static const std::unordered_map<std::string, int> KEYWORDS = {
        {"SELECT", yy::parser::token::SELECT},
        {"INSERT", yy::parser::token::INSERT},
        {"UPDATE", yy::parser::token::UPDATE},
        {"DELETE", yy::parser::token::DELETE},
        {"CREATE", yy::parser::token::CREATE},
        {"DROP", yy::parser::token::DROP},
        {"USE", yy::parser::token::USE},
        {"DATABASE", yy::parser::token::DATABASE},
        {"TABLE", yy::parser::token::TABLE},
        {"FROM", yy::parser::token::FROM},
        {"WHERE", yy::parser::token::WHERE},
        {"SET", yy::parser::token::SET},
        {"VALUE", yy::parser::token::VALUE},
        {"INTO", yy::parser::token::INTO},
        {"AS", yy::parser::token::AS},
        {"AND", yy::parser::token::AND},
        {"OR", yy::parser::token::OR},
        {"BETWEEN", yy::parser::token::BETWEEN},
        {"LIKE", yy::parser::token::LIKE},
        {"NOT", yy::parser::token::NOT},
        {"NOT_NULL", yy::parser::token::NOT_NULL},
        {"NULL", yy::parser::token::NULL_},
        {"INDEXED", yy::parser::token::INDEXED},
        {"SUM", yy::parser::token::SUM},
        {"COUNT", yy::parser::token::COUNT},
        {"AVG", yy::parser::token::AVG},
        {"DEFAULT", yy::parser::token::DEFAULT},
        {"REVERT", yy::parser::token::REVERT},
        {"INT", yy::parser::token::INT_TYPE},
        {"STRING", yy::parser::token::STRING_TYPE}
    };

    // Переводим слово в верхний регистр для поиска в мапе
    std::string upper_text = text;
    std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(), ::toupper);

    auto it = KEYWORDS.find(upper_text);
    if (it != KEYWORDS.end()) {
        // Нашли ключевое слово! Проверяем строгое правило ТЗ на смешение регистров.
        bool has_upper = false;
        bool has_lower = false;
        for (char c : text) {
            if (std::isupper(static_cast<unsigned char>(c))) has_upper = true;
            if (std::islower(static_cast<unsigned char>(c))) has_lower = true;
        }
        
        if (has_upper && has_lower) {
            std::string err = "Validation Error: Mixed case in keyword '" + text + "' is forbidden.";
            yyerror(err.c_str());
            return yy::parser::token::YYerror; // Возвращаем токен ошибки Bison
        }
        return it->second; // Регистр корректен, возвращаем токен команды
    }

    // Если это не ключевое слово, значит это обычное имя сущности (БД, таблицы, колонки)
    yylval->emplace<std::string>(text);
    return yy::parser::token::IDENTIFIER;
}
%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="SqlScanner"
%option prefix="SqlScanner"

/* Регулярные выражения */
DIGIT           [0-9]
LETTER          [a-zA-Z]
IDENT           {LETTER}({LETTER}|{DIGIT}|_)*
INTEGER         {DIGIT}+
STRING          \"([^\\\"]|\\.)*\"
WHITESPACE      [ \t\r\n]+
TIMESTAMP       [0-9]{4}\.[0-9]{2}\.[0-9]{2}-[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{6}

%%

 /* Операторы */
"=="            { return yy::parser::token::EQ; }
"!="            { return yy::parser::token::NE; }
"<="            { return yy::parser::token::LE; }
">="            { return yy::parser::token::GE; }
"<"             { return yy::parser::token::LT; }
">"             { return yy::parser::token::GT; }
"="             { return yy::parser::token::ASSIGN; }

 /* Разделители */
";"             { return yy::parser::token::SEMICOLON; }
","             { return yy::parser::token::COMMA; }
"("             { return yy::parser::token::LPAREN; }
")"             { return yy::parser::token::RPAREN; }
"*"             { return yy::parser::token::STAR; }

 /* Литералы */
{INTEGER}       {
                    yylval->emplace<int>(std::stoi(yytext));
                    return yy::parser::token::INTEGER;
                }
{STRING}        {
                    std::string s(yytext + 1);
                    s.pop_back();  // убираем последнюю кавычку
                    size_t pos = 0;
                    while ((pos = s.find("\\\"", pos)) != std::string::npos) {
                        s.replace(pos, 2, "\"");
                        pos++;
                    }
                    yylval->emplace<std::string>(s);
                    return yy::parser::token::STRING;
                }

{IDENT}         {
                    // Обрабатываем идентификатор через умную функцию
                    return process_keyword_or_ident(yytext, yylval);
                }

{TIMESTAMP}     {
                    yylval->emplace<std::string>(yytext);
                    return yy::parser::token::TIMESTAMP;
                }

 /* Пропуск пробелов и переводов строк */
{WHITESPACE}    { /* ignore */ }

 /* Обработка ошибок */
.               {
                    std::string msg = "Unexpected character: ";
                    msg += yytext;
                    yyloc->initialize();
                    yyloc->lines(yylineno);
                    yyerror(msg.c_str());
                    return yy::parser::token::YYerror;
                }

%%
