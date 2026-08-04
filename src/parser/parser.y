%{
#include <iostream>
void yyerror(const char *s);
int yylex();
%}

%token NUMBER

%%
program:
    /* empty */
    ;
%%

void yyerror(const char *s) {
    std::cerr << "Parse error: " << s << std::endl;
}