/* CodeForge Parser (Step 5: Bison Foundation — %union, Tokens & Precedence) */

%code requires {
    #include <string>
    #include "common/types.h"
    #include "ast/ast.h"
}

%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include "common/types.h"
#include "common/error_reporter.h"
#include "ast/ast.h"

/* External symbols defined in src/lexer/lexer.l */
extern int yylex(void);
extern int line_num;
extern int token_col;
extern codeforge::ErrorReporter* g_error_reporter;

/* Root AST pointer populated by yyparse() and consumed by src/main.cpp */
codeforge::ProgramNode* g_ast_root = nullptr;

void yyerror(const char* s);
%}

/* Semantic value union shared with src/lexer/lexer.l via parser.tab.hpp */
%union {
    int ival;
    double dval;
    bool bval;
    std::string* sval;
    codeforge::DataType dtype;
    codeforge::TypeKeyword kw;
    codeforge::ExprNode* expr;
    codeforge::StmtNode* stmt;
    codeforge::BlockNode* block;
    codeforge::ProgramNode* prog;
}

/* ============================================================================
 * Terminal Token Declarations (1-to-1 match with src/lexer/lexer.l)
 * ============================================================================ */

/* Type Keywords */
%token TOK_INT TOK_FLOAT TOK_BOOL TOK_BOOLEAN

/* Control-Flow Keywords */
%token TOK_IF TOK_ELSE TOK_WHILE TOK_FOR

/* Boolean Literals */
%token <bval> TOK_TRUE TOK_FALSE

/* Multi-Language Output Keywords */
%token TOK_PRINT TOK_PRINTF TOK_COUT TOK_STD TOK_SYSTEM TOK_OUT TOK_PRINTLN

/* Multi-Character Operators */
%token TOK_EQ TOK_NEQ TOK_LE TOK_GE TOK_AND TOK_OR TOK_SHL TOK_SCOPE

/* Single-Character Operators */
%token TOK_PLUS TOK_MINUS TOK_MUL TOK_DIV TOK_MOD
%token TOK_LT TOK_GT TOK_NOT TOK_ASSIGN TOK_DOT

/* Delimiters & Punctuation */
%token TOK_LPAREN TOK_RPAREN TOK_LBRACE TOK_RBRACE TOK_SEMI TOK_COMMA

/* Numeric Literals & Identifiers */
%token <ival> TOK_INT_LIT
%token <dval> TOK_FLOAT_LIT
%token <sval> TOK_ID

/* ============================================================================
 * Precedence & Associativity (Lowest to Highest)
 * ============================================================================ */

/* Dangling-else resolution (binds else to the nearest unmatched if) */
%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE

/* Expression operator precedence matching C / C++ / Java */
%left TOK_OR
%left TOK_AND
%left TOK_EQ TOK_NEQ
%left TOK_LT TOK_GT TOK_LE TOK_GE
%left TOK_PLUS TOK_MINUS
%left TOK_MUL TOK_DIV TOK_MOD
%right TOK_NOT UMINUS UPLUS

/* ============================================================================
 * Non-Terminal Types & Start Symbol
 * ============================================================================ */
%type <prog> program stmt_list

/* Activated as rules are added in Steps 6 & 7:
 * %type <kw>    type_spec
 * %type <stmt>  stmt var_decl var_decl_core assign_stmt assign_core
 * %type <stmt>  if_stmt while_stmt for_stmt for_init for_update print_stmt
 * %type <block> block
 * %type <expr>  expr for_cond
 */

%start program

%%

program
    : stmt_list {
        g_ast_root = $1;
        $$ = $1;
    }
    ;

stmt_list
    : /* empty (statement rules added in Steps 6 & 7) */ {
        $$ = new codeforge::ProgramNode(1);
    }
    ;

%%

void yyerror(const char* s) {
    if (g_error_reporter) {
        g_error_reporter->reportSyntax(line_num, s ? s : "Syntax error", token_col);
    } else {
        std::fprintf(stderr, "Syntax Error at line %d, col %d: %s\n",
                     line_num, token_col, s ? s : "Syntax error");
    }
}