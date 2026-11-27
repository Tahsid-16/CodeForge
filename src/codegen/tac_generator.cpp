#include "codegen/tac_generator.h"
#include <sstream>

namespace codeforge {

std::string TACGenerator::newTemp() {
    return "t" + std::to_string(tempCounter_++);
}

std::string TACGenerator::newLabel() {
    return "L" + std::to_string(labelCounter_++);
}

std::string TACGenerator::scopedVarName(const std::string& name, int scopeId) {
    if (scopeId > 0) {
        return name + "#" + std::to_string(scopeId);
    }
    return name;
}

TACOp TACGenerator::mapBinaryOp(Operator op) {
    switch (op) {
        case Operator::ADD: return TACOp::ADD;
        case Operator::SUB: return TACOp::SUB;
        case Operator::MUL: return TACOp::MUL;
        case Operator::DIV: return TACOp::DIV;
        case Operator::MOD: return TACOp::MOD;
        case Operator::EQ:  return TACOp::EQ;
        case Operator::NEQ: return TACOp::NEQ;
        case Operator::LT:  return TACOp::LT;
        case Operator::GT:  return TACOp::GT;
        case Operator::LE:  return TACOp::LE;
        case Operator::GE:  return TACOp::GE;
        case Operator::AND: return TACOp::AND;
        case Operator::OR:  return TACOp::OR;
        default:            return TACOp::ASSIGN;
    }
}

TACOp TACGenerator::mapUnaryOp(Operator op) {
    switch (op) {
        case Operator::NEG: return TACOp::NEG;
        case Operator::POS: return TACOp::POS;
        case Operator::NOT: return TACOp::NOT;
        default:            return TACOp::ASSIGN;
    }
}

std::string TACGenerator::defaultLiteralForType(DataType type) {
    switch (type) {
        case DataType::INT:    return "0";
        case DataType::FLOAT:  return "0.0";
        case DataType::BOOL:   return "false";
        case DataType::STRING: return "";
        default:               return "0";
    }
}

const TACProgram& TACGenerator::generate(ProgramNode& root) {
    program_.clear();
    lastExpr_ = Operand{};
    tempCounter_ = 0;
    labelCounter_ = 0;

    root.accept(*this);
    return program_;
}

const TACProgram& TACGenerator::getProgram() const {
    return program_;
}

// ============================================================================
// Expression Visitors
// ============================================================================

void TACGenerator::visit(IntLiteralNode& node) {
    lastExpr_ = Operand{std::to_string(node.value), false, DataType::INT};
}

void TACGenerator::visit(FloatLiteralNode& node) {
    std::ostringstream ss;
    ss << node.value;
    std::string text = ss.str();
    // Preserve decimal point so integer-valued floats (e.g. 5.0) stay float in TAC
    if (text.find('.') == std::string::npos &&
        text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        text += ".0";
    }
    lastExpr_ = Operand{std::move(text), false, DataType::FLOAT};
}

void TACGenerator::visit(BoolLiteralNode& node) {
    lastExpr_ = Operand{node.value ? "true" : "false", false, DataType::BOOL};
}

void TACGenerator::visit(StringLiteralNode& node) {
    lastExpr_ = Operand{node.value, true, DataType::STRING};
}

void TACGenerator::visit(IdentifierNode& node) {
    lastExpr_ = Operand{scopedVarName(node.name, node.resolvedScopeId), false, node.inferredType};
}

void TACGenerator::visit(BinaryExprNode& node) {
    Operand lhs;
    if (node.left) {
        node.left->accept(*this);
        lhs = lastExpr_;
    }

    Operand rhs;
    if (node.right) {
        node.right->accept(*this);
        rhs = lastExpr_;
    }

    std::string temp = newTemp();
    program_.emit(TACInstruction(
        mapBinaryOp(node.op),
        temp,
        lhs.value,
        rhs.value,
        node.line,
        lhs.isString,
        rhs.isString,
        node.inferredType
    ));

    lastExpr_ = Operand{std::move(temp), false, node.inferredType};
}

void TACGenerator::visit(UnaryExprNode& node) {
    Operand operand;
    if (node.operand) {
        node.operand->accept(*this);
        operand = lastExpr_;
    }

    std::string temp = newTemp();
    program_.emit(TACInstruction(
        mapUnaryOp(node.op),
        temp,
        operand.value,
        "",
        node.line,
        operand.isString,
        false,
        node.inferredType
    ));

    lastExpr_ = Operand{std::move(temp), false, node.inferredType};
}

// ============================================================================
// Statement & Root Visitors
// ============================================================================

void TACGenerator::visit(VarDeclNode& node) {
    std::string target = scopedVarName(node.name, node.resolvedScopeId);

    if (node.initializer) {
        node.initializer->accept(*this);
        program_.emit(TACInstruction(
            TACOp::ASSIGN,
            target,
            lastExpr_.value,
            "",
            node.line,
            lastExpr_.isString,
            false,
            node.varType
        ));
    } else {
        program_.emit(TACInstruction(
            TACOp::ASSIGN,
            target,
            defaultLiteralForType(node.varType),
            "",
            node.line,
            node.varType == DataType::STRING,
            false,
            node.varType
        ));
    }
}

void TACGenerator::visit(AssignNode& node) {
    std::string target = scopedVarName(node.name, node.resolvedScopeId);

    if (node.value) {
        node.value->accept(*this);
        program_.emit(TACInstruction(
            TACOp::ASSIGN,
            target,
            lastExpr_.value,
            "",
            node.line,
            lastExpr_.isString,
            false,
            lastExpr_.type
        ));
    }
}

void TACGenerator::visit(PrintNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
        program_.emit(TACInstruction(
            TACOp::PRINT,
            "",
            lastExpr_.value,
            "",
            node.line,
            lastExpr_.isString,
            false,
            lastExpr_.type
        ));
    }
}

void TACGenerator::visit(BlockNode& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

void TACGenerator::visit(IfNode& node) {
    Operand cond;
    if (node.condition) {
        node.condition->accept(*this);
        cond = lastExpr_;
    }

    if (node.elseBranch) {
        std::string elseLabel = newLabel();
        std::string endLabel = newLabel();

        program_.emit(TACInstruction(
            TACOp::IF_FALSE, elseLabel, cond.value, "", node.line, cond.isString, false
        ));

        if (node.thenBranch) {
            node.thenBranch->accept(*this);
        }

        program_.emit(TACInstruction(TACOp::GOTO, endLabel, "", "", node.line));
        program_.emit(TACInstruction(TACOp::LABEL, elseLabel, "", "", node.line));
        node.elseBranch->accept(*this);
        program_.emit(TACInstruction(TACOp::LABEL, endLabel, "", "", node.line));
    } else {
        std::string endLabel = newLabel();

        program_.emit(TACInstruction(
            TACOp::IF_FALSE, endLabel, cond.value, "", node.line, cond.isString, false
        ));

        if (node.thenBranch) {
            node.thenBranch->accept(*this);
        }

        program_.emit(TACInstruction(TACOp::LABEL, endLabel, "", "", node.line));
    }
}

void TACGenerator::visit(WhileNode& node) {
    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    program_.emit(TACInstruction(TACOp::LABEL, startLabel, "", "", node.line));

    if (node.condition) {
        node.condition->accept(*this);
        program_.emit(TACInstruction(
            TACOp::IF_FALSE, endLabel, lastExpr_.value, "", node.line, lastExpr_.isString, false
        ));
    }

    if (node.body) {
        node.body->accept(*this);
    }

    program_.emit(TACInstruction(TACOp::GOTO, startLabel, "", "", node.line));
    program_.emit(TACInstruction(TACOp::LABEL, endLabel, "", "", node.line));
}

void TACGenerator::visit(ForNode& node) {
    if (node.init) {
        node.init->accept(*this);
    }

    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    program_.emit(TACInstruction(TACOp::LABEL, startLabel, "", "", node.line));

    if (node.condition) {
        node.condition->accept(*this);
        program_.emit(TACInstruction(
            TACOp::IF_FALSE, endLabel, lastExpr_.value, "", node.line, lastExpr_.isString, false
        ));
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (node.update) {
        node.update->accept(*this);
    }

    program_.emit(TACInstruction(TACOp::GOTO, startLabel, "", "", node.line));
    program_.emit(TACInstruction(TACOp::LABEL, endLabel, "", "", node.line));
}

void TACGenerator::visit(ProgramNode& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

} // namespace codeforge