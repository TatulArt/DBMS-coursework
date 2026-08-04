%{
#include "parser.tab.hpp"
%}

%option noyywrap

%%
[0-9]+      { return NUMBER; }
[ \t\n]     ; // ignore whitespace
.           { return 0; }
%%