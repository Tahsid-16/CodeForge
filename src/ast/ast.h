#ifndef CODEFORGE_AST_AST_H
#define CODEFORGE_AST_AST_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "common/types.h"

namespace codeforge {

// Forward declarations of concrete AST nodes
class IntLiteralNode;
class FloatLiteralNode;
class BoolLiteralNode;
class IdentifierNode;
class BinaryExprNode;
class UnaryExprNode;
class VarDeclNode;
class AssignNode;
class PrintNode;
class BlockNode;
class IfNode;
class WhileNode;
class ForNode;
class ProgramNode;

// ============================================================================
// AST Visitor Interface
// Implemented by:
//   - SemanticAnalyzer (Track 2)
//   - ASTPrinter / ASTJsonSerializer (Track 2)
//   - TACGenerator (Track 3)
// ============================================================================
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(IntLiteralNode& node) = 0;
    virtual void visit(FloatLiteralNode& node) = 0;
    virtual void visit(BoolLiteralNode& node) = 0;
    virtual void visit(IdentifierNode& node) = 0;
    virtual void visit(BinaryExprNode& node) = 0;
    virtual void visit(UnaryExprNode& node) = 0;

    // Statements & Root
    virtual void visit(VarDeclNode& node) = 0;
    virtual void visit(AssignNode& node) = 0;
    virtual void visit(PrintNode& node) = 0;
    virtual void visit(BlockNode& node) = 0;
    virtual void visit(IfNode& node) = 0;
    virtual void visit(WhileNode& node) = 0;
    virtual void visit(ForNode& node) = 0;
    virtual void visit(ProgramNode& node) = 0;
};

// ============================================================================
// Abstract Base Nodes
// ============================================================================
class ASTNode {
public:
    int line;
    int column;

    explicit ASTNode(int ln = 0, int col = 1) : line(ln), column(col) {}
    virtual ~ASTNode() = default;

    virtual void accept(ASTVisitor& visitor) = 0;
};

class ExprNode : public ASTNode {
public:
    DataType inferredType = DataType::UNKNOWN;

    explicit ExprNode(int ln = 0, int col = 1) : ASTNode(ln, col) {}

    bool hasTypeError() const {
        return inferredType == DataType::ERROR;
    }
};

class StmtNode : public ASTNode {
public:
    explicit StmtNode(int ln = 0, int col = 1) : ASTNode(ln, col) {}
};

// ============================================================================
// Expression Nodes
// ============================================================================
class IntLiteralNode : public ExprNode {
public:
    int value;

    explicit IntLiteralNode(int val, int ln = 0)
        : ExprNode(ln), value(val) {
        inferredType = DataType::INT;
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class FloatLiteralNode : public ExprNode {
public:
    double value;

    explicit FloatLiteralNode(double val, int ln = 0)
        : ExprNode(ln), value(val) {
        inferredType = DataType::FLOAT;
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class BoolLiteralNode : public ExprNode {
public:
    bool value;

    explicit BoolLiteralNode(bool val, int ln = 0)
        : ExprNode(ln), value(val) {
        inferredType = DataType::BOOL;
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class IdentifierNode : public ExprNode {
public:
    std::string name;
    int resolvedScopeId = -1; // Set by SemanticAnalyzer to disambiguate shadowed vars

    explicit IdentifierNode(std::string idName, int ln = 0)
        : ExprNode(ln), name(std::move(idName)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class BinaryExprNode : public ExprNode {
public:
    Operator op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    BinaryExprNode(Operator oper, ExprNode* lhs, ExprNode* rhs, int ln = 0)
        : ExprNode(ln), op(oper), left(lhs), right(rhs) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class UnaryExprNode : public ExprNode {
public:
    Operator op;
    std::unique_ptr<ExprNode> operand;

    UnaryExprNode(Operator oper, ExprNode* expr, int ln = 0)
        : ExprNode(ln), op(oper), operand(expr) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// ============================================================================
// Statement Nodes
// ============================================================================
class VarDeclNode : public StmtNode {
public:
    DataType varType;
    TypeKeyword rawKeyword;
    std::string name;
    std::unique_ptr<ExprNode> initializer; // nullptr if declared without initialization
    int resolvedScopeId = -1;              // Scope ID where this variable lives

    VarDeclNode(DataType type, TypeKeyword kw, std::string varName, ExprNode* init = nullptr, int ln = 0)
        : StmtNode(ln), varType(type), rawKeyword(kw), name(std::move(varName)), initializer(init) {}

    // Convenience overload defaulting rawKeyword from DataType
    VarDeclNode(DataType type, std::string varName, ExprNode* init = nullptr, int ln = 0)
        : StmtNode(ln),
          varType(type),
          rawKeyword(type == DataType::BOOL ? TypeKeyword::KW_BOOL :
                     type == DataType::FLOAT ? TypeKeyword::KW_FLOAT : TypeKeyword::KW_INT),
          name(std::move(varName)),
          initializer(init) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};
class AssignNode : public StmtNode {
public:
    std::string name;
    std::unique_ptr<ExprNode> value;
    int resolvedScopeId = -1; // Scope ID of the target variable

    AssignNode(std::string varName, ExprNode* valExpr, int ln = 0)
        : StmtNode(ln), name(std::move(varName)), value(valExpr) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// Convergent output node for print(), printf(), cout <<, and System.out.println()
class PrintNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> expression;
    PrintFlavor flavor;

    // Constructor 1: Standard (defaults to GENERIC_PRINT)
    PrintNode(ExprNode* expr, int ln = 0)
        : StmtNode(ln), expression(expr), flavor(PrintFlavor::GENERIC_PRINT) {}

    // Constructor 2: Language-flavor aware (for C / C++ / Java distinction in AST UI)
    PrintNode(ExprNode* expr, PrintFlavor flv, int ln = 0)
        : StmtNode(ln), expression(expr), flavor(flv) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class BlockNode : public StmtNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;
    int scopeId = -1; // Assigned by SymbolTable when entering block scope

    explicit BlockNode(int ln = 0) : StmtNode(ln) {}

    void addStatement(StmtNode* stmt) {
        if (stmt) {
            statements.emplace_back(stmt);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class IfNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenBranch;
    std::unique_ptr<StmtNode> elseBranch; // nullptr if there is no else branch

    IfNode(ExprNode* cond, StmtNode* thenStmt, StmtNode* elseStmt = nullptr, int ln = 0)
        : StmtNode(ln), condition(cond), thenBranch(thenStmt), elseBranch(elseStmt) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class WhileNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> body;

    WhileNode(ExprNode* cond, StmtNode* loopBody, int ln = 0)
        : StmtNode(ln), condition(cond), body(loopBody) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ForNode : public StmtNode {
public:
    std::unique_ptr<StmtNode> init;      // VarDeclNode or AssignNode (nullable)
    std::unique_ptr<ExprNode> condition; // Loop termination condition (nullable)
    std::unique_ptr<StmtNode> update;    // AssignNode step (nullable)
    std::unique_ptr<StmtNode> body;
    int scopeId = -1;                    // Scope wrapping the loop header (e.g., for(int i=0;...))

    ForNode(StmtNode* initStmt, ExprNode* condExpr, StmtNode* updateStmt, StmtNode* loopBody, int ln = 0)
        : StmtNode(ln), init(initStmt), condition(condExpr), update(updateStmt), body(loopBody) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// ============================================================================
// Root Compilation Unit
// ============================================================================
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;

    explicit ProgramNode(int ln = 1) : ASTNode(ln) {}

    void addStatement(StmtNode* stmt) {
        if (stmt) {
            statements.emplace_back(stmt);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

} // namespace codeforge

#endif // CODEFORGE_AST_AST_H