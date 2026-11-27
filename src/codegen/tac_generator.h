#ifndef CODEFORGE_CODEGEN_TAC_GENERATOR_H
#define CODEFORGE_CODEGEN_TAC_GENERATOR_H

#include <string>
#include "ast/ast.h"
#include "codegen/tac.h"

namespace codeforge {

class TACGenerator : public ASTVisitor {
private:
    struct Operand {
        std::string value;
        bool isString = false;
        DataType type = DataType::UNKNOWN;
    };

    TACProgram program_;
    Operand lastExpr_;
    int tempCounter_ = 0;
    int labelCounter_ = 0;

    std::string newTemp();
    std::string newLabel();

    static std::string scopedVarName(const std::string& name, int scopeId);
    static TACOp mapBinaryOp(Operator op);
    static TACOp mapUnaryOp(Operator op);
    static std::string defaultLiteralForType(DataType type);

public:
    TACGenerator() = default;

    // Resets state and lowers the entire AST into Three-Address Code
    const TACProgram& generate(ProgramNode& root);
    const TACProgram& getProgram() const;

    // Expression visitors
    void visit(IntLiteralNode& node) override;
    void visit(FloatLiteralNode& node) override;
    void visit(BoolLiteralNode& node) override;
    void visit(StringLiteralNode& node) override;
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

#endif // CODEFORGE_CODEGEN_TAC_GENERATOR_H