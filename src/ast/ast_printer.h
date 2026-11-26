#ifndef CODEFORGE_AST_AST_PRINTER_H
#define CODEFORGE_AST_AST_PRINTER_H

#include <iostream>
#include <sstream>
#include <string>
#include "ast/ast.h"

namespace codeforge {

class ASTPrinter : public ASTVisitor {
private:
    std::ostringstream out_;
    std::string currentPrefix_;
    std::string pendingLabel_;
    bool isLastChild_ = true;
    bool isRoot_ = true;

    void writeHeader(const std::string& text);
    std::string childPrefix() const;
    void visitChild(ASTNode* node, const std::string& label, bool isLast);

public:
    ASTPrinter() = default;

    std::string toString(ProgramNode& root);
    void print(ProgramNode& root, std::ostream& os = std::cout);

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

class ASTJsonSerializer : public ASTVisitor {
private:
    std::ostringstream out_;

    void visitOrNull(ASTNode* node);

public:
    ASTJsonSerializer() = default;

    std::string toJson(ProgramNode& root);

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

#endif // CODEFORGE_AST_AST_PRINTER_H