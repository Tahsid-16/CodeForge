#ifndef CODEFORGE_SEMANTIC_SEMANTIC_ANALYZER_H
#define CODEFORGE_SEMANTIC_SEMANTIC_ANALYZER_H

#include "ast/ast.h"
#include "common/error_reporter.h"
#include "common/types.h"
#include "symbol_table/symbol_table.h"

namespace codeforge {

class SemanticAnalyzer : public ASTVisitor {
private:
    SymbolTable& symbolTable_;
    ErrorReporter& errorReporter_;
    SourceLanguage targetLanguage_;

    // Type system & strict-mode helpers
    static bool isNumeric(DataType type);
    static bool isAssignable(DataType target, DataType source);
    static bool isZeroLiteral(const ExprNode* expr);

    void validateTypeKeyword(TypeKeyword keyword, int line, int column);
    void validatePrintFlavor(PrintFlavor flavor, int line, int column);
    void checkBooleanCondition(const ExprNode* condition, const std::string& context, int fallbackLine, int fallbackCol);

public:
    SemanticAnalyzer(SymbolTable& symbolTable,
                     ErrorReporter& errorReporter,
                     SourceLanguage language = SourceLanguage::AUTO);

    // Runs full semantic analysis on the AST; returns true if no new semantic errors were reported
    bool analyze(ProgramNode& root);

    void setTargetLanguage(SourceLanguage language);
    SourceLanguage getTargetLanguage() const;

    // Expression visitors
    void visit(IntLiteralNode& node) override;
    void visit(FloatLiteralNode& node) override;
    void visit(BoolLiteralNode& node) override;
    void visit(IdentifierNode& node) override;
    void visit(BinaryExprNode& node) override;
    void visit(UnaryExprNode& node) override;

    // Statement & root visitors
    void visit(VarDeclNode& node) override;
    void visit(AssignNode& node) override;
    void visit(PrintNode& node) override;
    void visit(BlockNode& node) override;
    void visit(IfNode& node) override;
    void visit(WhileNode& node) override;
    void visit(ForNode& node) override;
    void visit(ProgramNode& node) override;
};

} // namespace codeforge

#endif // CODEFORGE_SEMANTIC_SEMANTIC_ANALYZER_H