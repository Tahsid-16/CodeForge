/* CodeForge Parser (Step 6: Expressions, Variable Declarations & Assignments) */

%code requires {
    #include <string>
    #include "common/types.h"
    #include "ast/ast.h"
}

%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
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

/* Map exact source keyword to unified semantic DataType while preserving TypeKeyword for Strict Mode */
static codeforge::DataType keywordToDataType(codeforge::TypeKeyword kw) {
    switch (kw) {
        case codeforge::TypeKeyword::KW_INT:     return codeforge::DataType::INT;
        case codeforge::TypeKeyword::KW_FLOAT:   return codeforge::DataType::FLOAT;
        case codeforge::TypeKeyword::KW_BOOL:    return codeforge::DataType::BOOL;
        case codeforge::TypeKeyword::KW_BOOLEAN: return codeforge::DataType::BOOL;
    }
    return codeforge::DataType::UNKNOWN;
}
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

/* Terminal Token Declarations (1-to-1 match with src/lexer/lexer.l) */
%token TOK_INT TOK_FLOAT TOK_BOOL TOK_BOOLEAN
%token TOK_IF TOK_ELSE TOK_WHILE TOK_FOR
%token <bval> TOK_TRUE TOK_FALSE
%token TOK_PRINT TOK_PRINTF TOK_COUT TOK_STD TOK_SYSTEM TOK_OUT TOK_PRINTLN
%token TOK_EQ TOK_NEQ TOK_LE TOK_GE TOK_AND TOK_OR TOK_SHL TOK_SCOPE
%token TOK_PLUS TOK_MINUS TOK_MUL TOK_DIV TOK_MOD
%token TOK_LT TOK_GT TOK_NOT TOK_ASSIGN TOK_DOT
%token TOK_LPAREN TOK_RPAREN TOK_LBRACE TOK_RBRACE TOK_SEMI TOK_COMMA
%token <ival> TOK_INT_LIT
%token <dval> TOK_FLOAT_LIT
%token <sval> TOK_ID

/* Precedence & Associativity (Lowest to Highest) */
%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE

%left TOK_OR
%left TOK_AND
%left TOK_EQ TOK_NEQ
%left TOK_LT TOK_GT TOK_LE TOK_GE
%left TOK_PLUS TOK_MINUS
%left TOK_MUL TOK_DIV TOK_MOD
%right TOK_NOT UMINUS UPLUS

/* Non-Terminal Types */
%type <prog> program stmt_list
%type <kw>   type_spec
%type <stmt> stmt var_decl var_decl_core assign_stmt assign_core
%type <expr> expr

%start program

%%

program
    : stmt_list {
        g_ast_root = $1;
        $$ = $1;
    }
    ;

stmt_list
    : stmt_list stmt {
        if ($2) {
            $1->addStatement($2);
        }
        $$ = $1;
    }
    | /* empty */ {
        $$ = new codeforge::ProgramNode(1);
    }
    ;

stmt
    : var_decl    { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | TOK_SEMI    { $$ = nullptr; }
    ;

type_spec
    : TOK_INT     { $$ = codeforge::TypeKeyword::KW_INT; }
    | TOK_FLOAT   { $$ = codeforge::TypeKeyword::KW_FLOAT; }
    | TOK_BOOL    { $$ = codeforge::TypeKeyword::KW_BOOL; }
    | TOK_BOOLEAN { $$ = codeforge::TypeKeyword::KW_BOOLEAN; }
    ;

var_decl
    : var_decl_core TOK_SEMI { $$ = $1; }
    ;

var_decl_core
    : type_spec TOK_ID {
        std::string name = *$2;
        delete $2;
        $$ = new codeforge::VarDeclNode(
            keywordToDataType($1), $1, std::move(name), nullptr, line_num
        );
    }
    | type_spec TOK_ID TOK_ASSIGN expr {
        std::string name = *$2;
        delete $2;
        $$ = new codeforge::VarDeclNode(
            keywordToDataType($1), $1, std::move(name), $4, line_num
        );
    }
    ;

assign_stmt
    : assign_core TOK_SEMI { $$ = $1; }
    ;

assign_core
    : TOK_ID TOK_ASSIGN expr {
        std::string name = *$1;
        delete $1;
        $$ = new codeforge::AssignNode(std::move(name), $3, line_num);
    }
    ;

expr
    : expr TOK_OR expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::OR,  $1, $3, line_num); }
    | expr TOK_AND expr   { $$ = new codeforge::BinaryExprNode(codeforge::Operator::AND, $1, $3, line_num); }
    | expr TOK_EQ expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::EQ,  $1, $3, line_num); }
    | expr TOK_NEQ expr   { $$ = new codeforge::BinaryExprNode(codeforge::Operator::NEQ, $1, $3, line_num); }
    | expr TOK_LT expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::LT,  $1, $3, line_num); }
    | expr TOK_GT expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::GT,  $1, $3, line_num); }
    | expr TOK_LE expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::LE,  $1, $3, line_num); }
    | expr TOK_GE expr    { $$ = new codeforge::BinaryExprNode(codeforge::Operator::GE,  $1, $3, line_num); }
    | expr TOK_PLUS expr  { $$ = new codeforge::BinaryExprNode(codeforge::Operator::ADD, $1, $3, line_num); }
    | expr TOK_MINUS expr { $$ = new codeforge::BinaryExprNode(codeforge::Operator::SUB, $1, $3, line_num); }
    | expr TOK_MUL expr   { $$ = new codeforge::BinaryExprNode(codeforge::Operator::MUL, $1, $3, line_num); }
    | expr TOK_DIV expr   { $$ = new codeforge::BinaryExprNode(codeforge::Operator::DIV, $1, $3, line_num); }
    | expr TOK_MOD expr   { $$ = new codeforge::BinaryExprNode(codeforge::Operator::MOD, $1, $3, line_num); }
    | TOK_NOT expr              { $$ = new codeforge::UnaryExprNode(codeforge::Operator::NOT, $2, line_num); }
    | TOK_MINUS expr %prec UMINUS { $$ = new codeforge::UnaryExprNode(codeforge::Operator::NEG, $2, line_num); }
    | TOK_PLUS expr  %prec UPLUS  { $$ = new codeforge::UnaryExprNode(codeforge::Operator::POS, $2, line_num); }
    | TOK_LPAREN expr TOK_RPAREN { $$ = $2; }
    | TOK_INT_LIT   { $$ = new codeforge::IntLiteralNode($1, line_num); }
    | TOK_FLOAT_LIT { $$ = new codeforge::FloatLiteralNode($1, line_num); }
    | TOK_TRUE      { $$ = new codeforge::BoolLiteralNode($1, line_num); }
    | TOK_FALSE     { $$ = new codeforge::BoolLiteralNode($1, line_num); }
    | TOK_ID {
        std::string name = *$1;
        delete $1;
        $$ = new codeforge::IdentifierNode(std::move(name), line_num);
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