/* CodeForge Lexer (Step 2: Core Keywords, Identifiers & Literals) */
%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include "ast/ast.h"
#include "parser.tab.hpp"

int line_num = 1;
%}

DIGIT       [0-9]
ID          [a-zA-Z_][a-zA-Z0-9_]*
INT_LIT     {DIGIT}+
FLOAT_LIT   {DIGIT}+\.{DIGIT}+

%%

[ \t\r]+        { /* skip whitespace and Windows CR */ }
\n              { line_num++; }

/* Type Keywords (C, C++, Java) */
"int"           { return TOK_INT; }
"float"         { return TOK_FLOAT; }
"bool"          { return TOK_BOOL; }
"boolean"       { return TOK_BOOLEAN; }

/* Control-Flow Keywords */
"if"            { return TOK_IF; }
"else"          { return TOK_ELSE; }
"while"         { return TOK_WHILE; }
"for"           { return TOK_FOR; }

/* Boolean Literals */
"true"          { yylval.bval = true;  return TOK_TRUE; }
"false"         { yylval.bval = false; return TOK_FALSE; }

/* Multi-Language Output Keywords */
"print"         { return TOK_PRINT; }
"printf"        { return TOK_PRINTF; }
"cout"          { return TOK_COUT; }
"std"           { return TOK_STD; }
"System"        { return TOK_SYSTEM; }
"out"           { return TOK_OUT; }
"println"       { return TOK_PRINTLN; }

/* Numeric Literals & Identifiers */
{FLOAT_LIT}     { yylval.dval = std::stod(yytext); return TOK_FLOAT_LIT; }
{INT_LIT}       { yylval.ival = std::stoi(yytext); return TOK_INT_LIT; }
{ID}            { yylval.sval = new std::string(yytext); return TOK_ID; }

/* Catch-all placeholder (routed to ErrorReporter in Step 4) */
.               { /* unexpected character handling added in Step 4 */ }

%%

int yywrap(void) { return 1; }