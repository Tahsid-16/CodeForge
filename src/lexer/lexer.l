/* CodeForge Lexer (Step 3: Operators, Delimiters, Comments & Line/Column Tracking) */
%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include "ast/ast.h"
#include "parser.tab.hpp"

int line_num = 1;
int col_num = 1;
int token_col = 1;

/* Record the starting column of every matched lexeme before advancing col_num */
#define YY_USER_ACTION \
    token_col = col_num; \
    col_num += yyleng;
%}

%x BLOCK_COMMENT

DIGIT       [0-9]
ID          [a-zA-Z_][a-zA-Z0-9_]*
INT_LIT     {DIGIT}+
FLOAT_LIT   {DIGIT}+\.{DIGIT}+

%%

/* Whitespace & Newlines */
[ \t\r]+                { /* skip whitespace and Windows CR */ }
\n                      { line_num++; col_num = 1; }

/* Single-Line & Multi-Line Comments */
"//"[^\r\n]*            { /* skip single-line comment */ }
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n       { line_num++; col_num = 1; }
<BLOCK_COMMENT>.        { /* skip characters inside block comment */ }
<BLOCK_COMMENT><<EOF>>  { BEGIN(INITIAL); /* unterminated comment error wired in Step 4 */ return 0; }

/* Type Keywords (C, C++, Java) */
"int"                   { return TOK_INT; }
"float"                 { return TOK_FLOAT; }
"bool"                  { return TOK_BOOL; }
"boolean"               { return TOK_BOOLEAN; }

/* Control-Flow Keywords */
"if"                    { return TOK_IF; }
"else"                  { return TOK_ELSE; }
"while"                 { return TOK_WHILE; }
"for"                   { return TOK_FOR; }

/* Boolean Literals */
"true"                  { yylval.bval = true;  return TOK_TRUE; }
"false"                 { yylval.bval = false; return TOK_FALSE; }

/* Multi-Language Output Keywords */
"print"                 { return TOK_PRINT; }
"printf"                { return TOK_PRINTF; }
"cout"                  { return TOK_COUT; }
"std"                   { return TOK_STD; }
"System"                { return TOK_SYSTEM; }
"out"                   { return TOK_OUT; }
"println"               { return TOK_PRINTLN; }

/* Multi-Character Operators (must precede single-char operators) */
"=="                    { return TOK_EQ; }
"!="                    { return TOK_NEQ; }
"<="                    { return TOK_LE; }
">="                    { return TOK_GE; }
"&&"                    { return TOK_AND; }
"||"                    { return TOK_OR; }
"<<"                    { return TOK_SHL; }
"::"                    { return TOK_SCOPE; }

/* Single-Character Operators */
"+"                     { return TOK_PLUS; }
"-"                     { return TOK_MINUS; }
"*"                     { return TOK_MUL; }
"/"                     { return TOK_DIV; }
"%"                     { return TOK_MOD; }
"<"                     { return TOK_LT; }
">"                     { return TOK_GT; }
"!"                     { return TOK_NOT; }
"="                     { return TOK_ASSIGN; }
"."                     { return TOK_DOT; }

/* Delimiters & Punctuation */
"("                     { return TOK_LPAREN; }
")"                     { return TOK_RPAREN; }
"{"                     { return TOK_LBRACE; }
"}"                     { return TOK_RBRACE; }
";"                     { return TOK_SEMI; }
","                     { return TOK_COMMA; }

/* Numeric Literals & Identifiers */
{FLOAT_LIT}             { yylval.dval = std::stod(yytext); return TOK_FLOAT_LIT; }
{INT_LIT}               { yylval.ival = std::stoi(yytext); return TOK_INT_LIT; }
{ID}                    { yylval.sval = new std::string(yytext); return TOK_ID; }

/* Catch-all placeholder (routed to ErrorReporter in Step 4) */
.                       { /* unexpected character handling added in Step 4 */ }

%%

int yywrap(void) { return 1; }