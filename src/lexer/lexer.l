/* CodeForge Lexer */
%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "common/types.h"
#include "common/error_reporter.h"
#include "ast/ast.h"
#include "parser.tab.hpp"

int line_num = 1;
int col_num = 1;
int token_col = 1;

static int comment_start_line = 1;
static int comment_start_col = 1;

/* Shared frontend state accessed by parser.y and main.cpp */
std::vector<codeforge::TokenInfo> g_tokens;
codeforge::ErrorReporter* g_error_reporter = nullptr;

/* Reset lexer state between runs or unit tests */
void lexer_reset() {
    line_num = 1;
    col_num = 1;
    token_col = 1;
    g_tokens.clear();
}

/* Record matched token metadata and return its Bison token ID */
static int recordToken(const char* tokenName, int tokenId) {
    g_tokens.emplace_back(tokenName, yytext, line_num, token_col);
    return tokenId;
}

/* Capture starting column of every lexeme before advancing col_num */
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
"/*"                    {
                            comment_start_line = line_num;
                            comment_start_col = token_col;
                            BEGIN(BLOCK_COMMENT);
                        }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n       { line_num++; col_num = 1; }
<BLOCK_COMMENT>.        { /* skip characters inside block comment */ }
<BLOCK_COMMENT><<EOF>>  {
                            BEGIN(INITIAL);
                            if (g_error_reporter) {
                                g_error_reporter->reportLexical(
                                    comment_start_line,
                                    "Unterminated multi-line comment",
                                    comment_start_col
                                );
                            }
                            return 0;
                        }

/* Type Keywords (C, C++, Java) */
"int"                   { return recordToken("TOK_INT", TOK_INT); }
"float"                 { return recordToken("TOK_FLOAT", TOK_FLOAT); }
"bool"                  { return recordToken("TOK_BOOL", TOK_BOOL); }
"boolean"               { return recordToken("TOK_BOOLEAN", TOK_BOOLEAN); }

/* Control-Flow Keywords */
"if"                    { return recordToken("TOK_IF", TOK_IF); }
"else"                  { return recordToken("TOK_ELSE", TOK_ELSE); }
"while"                 { return recordToken("TOK_WHILE", TOK_WHILE); }
"for"                   { return recordToken("TOK_FOR", TOK_FOR); }

/* Boolean Literals */
"true"                  { yylval.bval = true;  return recordToken("TOK_TRUE", TOK_TRUE); }
"false"                 { yylval.bval = false; return recordToken("TOK_FALSE", TOK_FALSE); }

/* Multi-Language Output Keywords */
"print"                 { return recordToken("TOK_PRINT", TOK_PRINT); }
"printf"                { return recordToken("TOK_PRINTF", TOK_PRINTF); }
"cout"                  { return recordToken("TOK_COUT", TOK_COUT); }
"std"                   { return recordToken("TOK_STD", TOK_STD); }
"System"                { return recordToken("TOK_SYSTEM", TOK_SYSTEM); }
"out"                   { return recordToken("TOK_OUT", TOK_OUT); }
"println"               { return recordToken("TOK_PRINTLN", TOK_PRINTLN); }

/* Multi-Character Operators */
"=="                    { return recordToken("TOK_EQ", TOK_EQ); }
"!="                    { return recordToken("TOK_NEQ", TOK_NEQ); }
"<="                    { return recordToken("TOK_LE", TOK_LE); }
">="                    { return recordToken("TOK_GE", TOK_GE); }
"&&"                    { return recordToken("TOK_AND", TOK_AND); }
"||"                    { return recordToken("TOK_OR", TOK_OR); }
"<<"                    { return recordToken("TOK_SHL", TOK_SHL); }
"::"                    { return recordToken("TOK_SCOPE", TOK_SCOPE); }

/* Single-Character Operators */
"+"                     { return recordToken("TOK_PLUS", TOK_PLUS); }
"-"                     { return recordToken("TOK_MINUS", TOK_MINUS); }
"*"                     { return recordToken("TOK_MUL", TOK_MUL); }
"/"                     { return recordToken("TOK_DIV", TOK_DIV); }
"%"                     { return recordToken("TOK_MOD", TOK_MOD); }
"<"                     { return recordToken("TOK_LT", TOK_LT); }
">"                     { return recordToken("TOK_GT", TOK_GT); }
"!"                     { return recordToken("TOK_NOT", TOK_NOT); }
"="                     { return recordToken("TOK_ASSIGN", TOK_ASSIGN); }
"."                     { return recordToken("TOK_DOT", TOK_DOT); }

/* Delimiters & Punctuation */
"("                     { return recordToken("TOK_LPAREN", TOK_LPAREN); }
")"                     { return recordToken("TOK_RPAREN", TOK_RPAREN); }
"{"                     { return recordToken("TOK_LBRACE", TOK_LBRACE); }
"}"                     { return recordToken("TOK_RBRACE", TOK_RBRACE); }
";"                     { return recordToken("TOK_SEMI", TOK_SEMI); }
","                     { return recordToken("TOK_COMMA", TOK_COMMA); }

/* Numeric Literals & Identifiers */
{FLOAT_LIT}             { yylval.dval = std::stod(yytext); return recordToken("TOK_FLOAT_LIT", TOK_FLOAT_LIT); }
{INT_LIT}               { yylval.ival = std::stoi(yytext); return recordToken("TOK_INT_LIT", TOK_INT_LIT); }
{ID}                    { yylval.sval = new std::string(yytext); return recordToken("TOK_ID", TOK_ID); }

/* Unrecognized Characters -> Centralized ErrorReporter */
.                       {
                            if (g_error_reporter) {
                                g_error_reporter->reportLexical(
                                    line_num,
                                    std::string("Unexpected character '") + yytext + "'",
                                    token_col
                                );
                            }
                        }

%%

int yywrap(void) { return 1; }