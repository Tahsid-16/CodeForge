#include "semantic/semantic_analyzer.h"
#include <string>

namespace codeforge {

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable,
                                   ErrorReporter& errorReporter,
                                   SourceLanguage language)
    : symbolTable_(symbolTable),
      errorReporter_(errorReporter),
      targetLanguage_(language) {}

bool SemanticAnalyzer::analyze(ProgramNode& root) {
    symbolTable_.clear();
    size_t errorsBefore = errorReporter_.errorCount();
    root.accept(*this);
    return errorReporter_.errorCount() == errorsBefore;
}

void SemanticAnalyzer::setTargetLanguage(SourceLanguage language) {
    targetLanguage_ = language;
}

SourceLanguage SemanticAnalyzer::getTargetLanguage() const {
    return targetLanguage_;
}

// ============================================================================
// Helpers
// ============================================================================

bool SemanticAnalyzer::isNumeric(DataType type) {
    return type == DataType::INT || type == DataType::FLOAT;
}

bool SemanticAnalyzer::isAssignable(DataType target, DataType source) {
    if (target == source) {
        return true;
    }
    // Allow safe widening conversion: int -> float
    if (target == DataType::FLOAT && source == DataType::INT) {
        return true;
    }
    return false;
}

bool SemanticAnalyzer::isZeroLiteral(const ExprNode* expr) {
    if (!expr) {
        return false;
    }
    if (const auto* intLit = dynamic_cast<const IntLiteralNode*>(expr)) {
        return intLit->value == 0;
    }
    if (const auto* floatLit = dynamic_cast<const FloatLiteralNode*>(expr)) {
        return floatLit->value == 0.0;
    }
    return false;
}

void SemanticAnalyzer::validateTypeKeyword(TypeKeyword keyword, int line, int column) {
    if (targetLanguage_ == SourceLanguage::AUTO) {
        return;
    }

    if (targetLanguage_ == SourceLanguage::JAVA && keyword == TypeKeyword::KW_BOOL) {
        errorReporter_.reportSemantic(
            line,
            "Strict Java mode error: use 'boolean' instead of 'bool'.",
            column
        );
    } else if ((targetLanguage_ == SourceLanguage::C || targetLanguage_ == SourceLanguage::CPP) &&
               keyword == TypeKeyword::KW_BOOLEAN) {
        errorReporter_.reportSemantic(
            line,
            "Strict " + sourceLanguageToString(targetLanguage_) +
                " mode error: use 'bool' instead of 'boolean'.",
            column
        );
    }
}

void SemanticAnalyzer::validatePrintFlavor(PrintFlavor flavor, int line, int column) {
    if (targetLanguage_ == SourceLanguage::AUTO || flavor == PrintFlavor::GENERIC_PRINT) {
        return;
    }

    switch (targetLanguage_) {
        case SourceLanguage::C:
            if (flavor != PrintFlavor::C_PRINTF) {
                errorReporter_.reportSemantic(
                    line,
                    "Strict C mode error: '" + printFlavorToString(flavor) +
                        "' is not valid in C; use printf() or print().",
                    column
                );
            }
            break;

        case SourceLanguage::CPP:
            if (flavor == PrintFlavor::JAVA_SYSOUT) {
                errorReporter_.reportSemantic(
                    line,
                    "Strict C++ mode error: 'System.out.println' is Java syntax; use cout or printf().",
                    column
                );
            }
            break;

        case SourceLanguage::JAVA:
            if (flavor != PrintFlavor::JAVA_SYSOUT) {
                errorReporter_.reportSemantic(
                    line,
                    "Strict Java mode error: '" + printFlavorToString(flavor) +
                        "' is not valid in Java; use System.out.println() or print().",
                    column
                );
            }
            break;

        case SourceLanguage::AUTO:
            break;
    }
}

void SemanticAnalyzer::checkBooleanCondition(const ExprNode* condition,
                                             const std::string& context,
                                             int fallbackLine,
                                             int fallbackCol) {
    if (!condition || condition->hasTypeError()) {
        return;
    }
    if (condition->inferredType != DataType::BOOL) {
        int ln = condition->line > 0 ? condition->line : fallbackLine;
        int col = condition->column > 0 ? condition->column : fallbackCol;
        errorReporter_.reportSemantic(
            ln,
            "Condition in '" + context + "' must have type 'bool', got '" +
                dataTypeToString(condition->inferredType) + "'.",
            col
        );
    }
}

// ============================================================================
// Expression Visitors
// ============================================================================

void SemanticAnalyzer::visit(IntLiteralNode& node) {
    node.inferredType = DataType::INT;
}

void SemanticAnalyzer::visit(FloatLiteralNode& node) {
    node.inferredType = DataType::FLOAT;
}

void SemanticAnalyzer::visit(BoolLiteralNode& node) {
    node.inferredType = DataType::BOOL;
}

void SemanticAnalyzer::visit(IdentifierNode& node) {
    Symbol* sym = symbolTable_.lookup(node.name);
    if (!sym) {
        errorReporter_.reportSemantic(
            node.line,
            "Undeclared variable '" + node.name + "'.",
            node.column
        );
        node.inferredType = DataType::ERROR;
        node.resolvedScopeId = -1;
        return;
    }

    sym->isUsed = true;
    node.inferredType = sym->type;
    node.resolvedScopeId = sym->scopeId;
}

void SemanticAnalyzer::visit(UnaryExprNode& node) {
    if (!node.operand) {
        node.inferredType = DataType::ERROR;
        return;
    }

    node.operand->accept(*this);
    if (node.operand->hasTypeError()) {
        node.inferredType = DataType::ERROR;
        return;
    }

    DataType opType = node.operand->inferredType;

    if (node.op == Operator::NOT) {
        if (opType != DataType::BOOL) {
            errorReporter_.reportSemantic(
                node.line,
                "Unary operator '!' requires 'bool' operand, got '" +
                    dataTypeToString(opType) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }
        node.inferredType = DataType::BOOL;
        return;
    }

    if (node.op == Operator::NEG || node.op == Operator::POS) {
        if (!isNumeric(opType)) {
            errorReporter_.reportSemantic(
                node.line,
                "Unary operator '" + operatorToString(node.op) +
                    "' requires numeric operand, got '" + dataTypeToString(opType) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }
        node.inferredType = opType;
        return;
    }

    errorReporter_.reportSemantic(
        node.line,
        "Unsupported unary operator '" + operatorToString(node.op) + "'.",
        node.column
    );
    node.inferredType = DataType::ERROR;
}

void SemanticAnalyzer::visit(BinaryExprNode& node) {
    if (!node.left || !node.right) {
        node.inferredType = DataType::ERROR;
        return;
    }

    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->hasTypeError() || node.right->hasTypeError()) {
        node.inferredType = DataType::ERROR;
        return;
    }

    DataType lhs = node.left->inferredType;
    DataType rhs = node.right->inferredType;

    // Arithmetic: +, -, *, /, %
    if (isArithmeticOp(node.op)) {
        if (node.op == Operator::MOD) {
            if (lhs != DataType::INT || rhs != DataType::INT) {
                errorReporter_.reportSemantic(
                    node.line,
                    "Operator '%' requires 'int' operands, got '" +
                        dataTypeToString(lhs) + "' and '" + dataTypeToString(rhs) + "'.",
                    node.column
                );
                node.inferredType = DataType::ERROR;
                return;
            }
            if (isZeroLiteral(node.right.get())) {
                errorReporter_.reportSemantic(
                    node.line,
                    "Modulo by zero in expression.",
                    node.column
                );
                node.inferredType = DataType::ERROR;
                return;
            }
            node.inferredType = DataType::INT;
            return;
        }

        if (!isNumeric(lhs) || !isNumeric(rhs)) {
            errorReporter_.reportSemantic(
                node.line,
                "Operator '" + operatorToString(node.op) +
                    "' requires numeric operands, got '" +
                    dataTypeToString(lhs) + "' and '" + dataTypeToString(rhs) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }

        if (node.op == Operator::DIV && isZeroLiteral(node.right.get())) {
            errorReporter_.reportSemantic(
                node.line,
                "Division by zero in expression.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }

        node.inferredType = (lhs == DataType::FLOAT || rhs == DataType::FLOAT)
                                ? DataType::FLOAT
                                : DataType::INT;
        return;
    }

    // Relational: <, >, <=, >=
    if (isRelationalOp(node.op)) {
        if (!isNumeric(lhs) || !isNumeric(rhs)) {
            errorReporter_.reportSemantic(
                node.line,
                "Relational operator '" + operatorToString(node.op) +
                    "' requires numeric operands, got '" +
                    dataTypeToString(lhs) + "' and '" + dataTypeToString(rhs) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }
        node.inferredType = DataType::BOOL;
        return;
    }

    // Equality: ==, !=
    if (isEqualityOp(node.op)) {
        bool validComparison = (isNumeric(lhs) && isNumeric(rhs)) ||
                               (lhs == DataType::BOOL && rhs == DataType::BOOL);
        if (!validComparison) {
            errorReporter_.reportSemantic(
                node.line,
                "Equality operator '" + operatorToString(node.op) +
                    "' cannot compare '" + dataTypeToString(lhs) +
                    "' and '" + dataTypeToString(rhs) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }
        node.inferredType = DataType::BOOL;
        return;
    }

    // Logical: &&, ||
    if (isLogicalOp(node.op)) {
        if (lhs != DataType::BOOL || rhs != DataType::BOOL) {
            errorReporter_.reportSemantic(
                node.line,
                "Logical operator '" + operatorToString(node.op) +
                    "' requires 'bool' operands, got '" +
                    dataTypeToString(lhs) + "' and '" + dataTypeToString(rhs) + "'.",
                node.column
            );
            node.inferredType = DataType::ERROR;
            return;
        }
        node.inferredType = DataType::BOOL;
        return;
    }

    errorReporter_.reportSemantic(
        node.line,
        "Unsupported binary operator '" + operatorToString(node.op) + "'.",
        node.column
    );
    node.inferredType = DataType::ERROR;
}

// ============================================================================
// Statement & Root Visitors
// ============================================================================

void SemanticAnalyzer::visit(VarDeclNode& node) {
    validateTypeKeyword(node.rawKeyword, node.line, node.column);

    bool hasInitializer = (node.initializer != nullptr);
    if (hasInitializer) {
        // Evaluate initializer before declaring so outer-scope shadowing works cleanly
        // and self-referencing undeclared variables are caught.
        node.initializer->accept(*this);
        if (!node.initializer->hasTypeError() &&
            !isAssignable(node.varType, node.initializer->inferredType)) {
            errorReporter_.reportSemantic(
                node.line,
                "Type mismatch in initialization of '" + node.name +
                    "': cannot assign '" + dataTypeToString(node.initializer->inferredType) +
                    "' to '" + dataTypeToString(node.varType) + "'.",
                node.column
            );
        }
    }

    if (!symbolTable_.declare(node.name, node.varType, node.line, hasInitializer)) {
        const Symbol* existing = symbolTable_.lookupCurrentScope(node.name);
        std::string extra = existing
                                ? " (previously declared at line " +
                                      std::to_string(existing->lineDeclared) + ")"
                                : "";
        errorReporter_.reportSemantic(
            node.line,
            "Redeclaration of variable '" + node.name + "' in the same scope" + extra + ".",
            node.column
        );
    }

    node.resolvedScopeId = symbolTable_.getCurrentScopeId();
}

void SemanticAnalyzer::visit(AssignNode& node) {
    if (node.value) {
        node.value->accept(*this);
    }

    Symbol* sym = symbolTable_.lookup(node.name);
    if (!sym) {
        errorReporter_.reportSemantic(
            node.line,
            "Assignment to undeclared variable '" + node.name + "'.",
            node.column
        );
        node.resolvedScopeId = -1;
        return;
    }

    node.resolvedScopeId = sym->scopeId;
    sym->isInitialized = true;

    if (node.value && !node.value->hasTypeError() &&
        !isAssignable(sym->type, node.value->inferredType)) {
        errorReporter_.reportSemantic(
            node.line,
            "Type mismatch in assignment to '" + node.name +
                "': cannot assign '" + dataTypeToString(node.value->inferredType) +
                "' to '" + dataTypeToString(sym->type) + "'.",
            node.column
        );
    }
}

void SemanticAnalyzer::visit(PrintNode& node) {
    validatePrintFlavor(node.flavor, node.line, node.column);

    if (!node.expression) {
        return;
    }

    node.expression->accept(*this);
    if (!node.expression->hasTypeError() && node.expression->inferredType == DataType::VOID) {
        errorReporter_.reportSemantic(
            node.line,
            "Cannot print an expression of type 'void'.",
            node.column
        );
    }
}

void SemanticAnalyzer::visit(BlockNode& node) {
    node.scopeId = symbolTable_.enterScope("block");
    for (const auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    symbolTable_.exitScope();
}

void SemanticAnalyzer::visit(IfNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        checkBooleanCondition(node.condition.get(), "if", node.line, node.column);
    }
    if (node.thenBranch) {
        node.thenBranch->accept(*this);
    }
    if (node.elseBranch) {
        node.elseBranch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        checkBooleanCondition(node.condition.get(), "while", node.line, node.column);
    }
    if (node.body) {
        node.body->accept(*this);
    }
}

void SemanticAnalyzer::visit(ForNode& node) {
    node.scopeId = symbolTable_.enterScope("for");

    if (node.init) {
        node.init->accept(*this);
    }
    if (node.condition) {
        node.condition->accept(*this);
        checkBooleanCondition(node.condition.get(), "for", node.line, node.column);
    }
    if (node.update) {
        node.update->accept(*this);
    }
    if (node.body) {
        node.body->accept(*this);
    }

    symbolTable_.exitScope();
}

void SemanticAnalyzer::visit(ProgramNode& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

} // namespace codeforge