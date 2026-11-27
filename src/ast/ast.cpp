#include "ast/ast_printer.h"
#include <cstddef>

namespace codeforge {

// ============================================================================
// ASTPrinter (ASCII Tree Output for Terminal)
// ============================================================================

void ASTPrinter::writeHeader(const std::string& text) {
    if (!isRoot_) {
        out_ << currentPrefix_ << (isLastChild_ ? "`-- " : "|-- ");
    }
    if (!pendingLabel_.empty()) {
        out_ << pendingLabel_ << ": ";
        pendingLabel_.clear();
    }
    out_ << text << "\n";
}

std::string ASTPrinter::childPrefix() const {
    if (isRoot_) {
        return "";
    }
    return currentPrefix_ + (isLastChild_ ? "    " : "|   ");
}

void ASTPrinter::visitChild(ASTNode* node, const std::string& label, bool isLast) {
    std::string savedPrefix = currentPrefix_;
    bool savedIsLast = isLastChild_;
    bool savedIsRoot = isRoot_;
    std::string savedLabel = pendingLabel_;

    currentPrefix_ = childPrefix();
    isLastChild_ = isLast;
    isRoot_ = false;
    pendingLabel_ = label;

    if (node) {
        node->accept(*this);
    } else {
        writeHeader("<null>");
    }

    currentPrefix_ = savedPrefix;
    isLastChild_ = savedIsLast;
    isRoot_ = savedIsRoot;
    pendingLabel_ = savedLabel;
}

std::string ASTPrinter::toString(ProgramNode& root) {
    out_.str("");
    out_.clear();
    currentPrefix_.clear();
    pendingLabel_.clear();
    isLastChild_ = true;
    isRoot_ = true;

    root.accept(*this);
    return out_.str();
}

void ASTPrinter::print(ProgramNode& root, std::ostream& os) {
    os << "=== Abstract Syntax Tree ===\n";
    os << toString(root);
}

void ASTPrinter::visit(IntLiteralNode& node) {
    writeHeader("IntLiteral(" + std::to_string(node.value) + ") : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
}

void ASTPrinter::visit(FloatLiteralNode& node) {
    std::ostringstream val;
    val << node.value;
    writeHeader("FloatLiteral(" + val.str() + ") : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
}

void ASTPrinter::visit(BoolLiteralNode& node) {
    writeHeader(std::string("BoolLiteral(") + (node.value ? "true" : "false") +
                ") : " + dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
}

void ASTPrinter::visit(StringLiteralNode& node) {
    writeHeader("StringLiteral(\"" + escapeJson(node.value) + "\") : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
}

void ASTPrinter::visit(IdentifierNode& node) {
    std::string scopeInfo = node.resolvedScopeId >= 0
                                ? ", scope#" + std::to_string(node.resolvedScopeId)
                                : "";
    writeHeader("Identifier(" + node.name + scopeInfo + ") : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
}

void ASTPrinter::visit(BinaryExprNode& node) {
    writeHeader("BinaryExpr('" + operatorToString(node.op) + "') : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
    visitChild(node.left.get(), "left", false);
    visitChild(node.right.get(), "right", true);
}

void ASTPrinter::visit(UnaryExprNode& node) {
    writeHeader("UnaryExpr('" + operatorToString(node.op) + "') : " +
                dataTypeToString(node.inferredType) + " [line " +
                std::to_string(node.line) + "]");
    visitChild(node.operand.get(), "operand", true);
}

void ASTPrinter::visit(VarDeclNode& node) {
    std::string scopeInfo = node.resolvedScopeId >= 0
                                ? ", scope#" + std::to_string(node.resolvedScopeId)
                                : "";
    writeHeader("VarDecl(" + dataTypeToString(node.varType) + " " + node.name +
                scopeInfo + ") [line " + std::to_string(node.line) + "]");
    if (node.initializer) {
        visitChild(node.initializer.get(), "init", true);
    }
}

void ASTPrinter::visit(AssignNode& node) {
    std::string scopeInfo = node.resolvedScopeId >= 0
                                ? ", scope#" + std::to_string(node.resolvedScopeId)
                                : "";
    writeHeader("Assign(" + node.name + scopeInfo + ") [line " +
                std::to_string(node.line) + "]");
    visitChild(node.value.get(), "value", true);
}

void ASTPrinter::visit(PrintNode& node) {
    writeHeader("Print(" + printFlavorToString(node.flavor) + ") [line " +
                std::to_string(node.line) + "]");
    visitChild(node.expression.get(), "expr", true);
}

void ASTPrinter::visit(BlockNode& node) {
    std::string scopeInfo = node.scopeId >= 0
                                ? " (scope#" + std::to_string(node.scopeId) + ")"
                                : "";
    writeHeader("Block" + scopeInfo + " [line " + std::to_string(node.line) + "]");
    for (size_t i = 0; i < node.statements.size(); ++i) {
        visitChild(node.statements[i].get(), "", i + 1 == node.statements.size());
    }
}

void ASTPrinter::visit(IfNode& node) {
    writeHeader("If [line " + std::to_string(node.line) + "]");
    bool hasElse = (node.elseBranch != nullptr);
    visitChild(node.condition.get(), "cond", false);
    visitChild(node.thenBranch.get(), "then", !hasElse);
    if (hasElse) {
        visitChild(node.elseBranch.get(), "else", true);
    }
}

void ASTPrinter::visit(WhileNode& node) {
    writeHeader("While [line " + std::to_string(node.line) + "]");
    visitChild(node.condition.get(), "cond", false);
    visitChild(node.body.get(), "body", true);
}

void ASTPrinter::visit(ForNode& node) {
    std::string scopeInfo = node.scopeId >= 0
                                ? " (scope#" + std::to_string(node.scopeId) + ")"
                                : "";
    writeHeader("For" + scopeInfo + " [line " + std::to_string(node.line) + "]");
    visitChild(node.init.get(), "init", false);
    visitChild(node.condition.get(), "cond", false);
    visitChild(node.update.get(), "update", false);
    visitChild(node.body.get(), "body", true);
}

void ASTPrinter::visit(ProgramNode& node) {
    writeHeader("Program [line " + std::to_string(node.line) + "]");
    for (size_t i = 0; i < node.statements.size(); ++i) {
        visitChild(node.statements[i].get(), "", i + 1 == node.statements.size());
    }
}

// ============================================================================
// ASTJsonSerializer (JSON Output for --json / Web UI)
// ============================================================================

void ASTJsonSerializer::visitOrNull(ASTNode* node) {
    if (node) {
        node->accept(*this);
    } else {
        out_ << "null";
    }
}

std::string ASTJsonSerializer::toJson(ProgramNode& root) {
    out_.str("");
    out_.clear();
    root.accept(*this);
    return out_.str();
}

void ASTJsonSerializer::visit(IntLiteralNode& node) {
    out_ << "{"
         << "\"nodeType\":\"IntLiteral\","
         << "\"line\":" << node.line << ","
         << "\"value\":" << node.value << ","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\""
         << "}";
}

void ASTJsonSerializer::visit(FloatLiteralNode& node) {
    out_ << "{"
         << "\"nodeType\":\"FloatLiteral\","
         << "\"line\":" << node.line << ","
         << "\"value\":" << node.value << ","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\""
         << "}";
}

void ASTJsonSerializer::visit(BoolLiteralNode& node) {
    out_ << "{"
         << "\"nodeType\":\"BoolLiteral\","
         << "\"line\":" << node.line << ","
         << "\"value\":" << (node.value ? "true" : "false") << ","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\""
         << "}";
}

void ASTJsonSerializer::visit(StringLiteralNode& node) {
    out_ << "{"
         << "\"nodeType\":\"StringLiteral\","
         << "\"line\":" << node.line << ","
         << "\"value\":\"" << escapeJson(node.value) << "\","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\""
         << "}";
}

void ASTJsonSerializer::visit(IdentifierNode& node) {
    out_ << "{"
         << "\"nodeType\":\"Identifier\","
         << "\"line\":" << node.line << ","
         << "\"name\":\"" << escapeJson(node.name) << "\","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\","
         << "\"resolvedScopeId\":" << node.resolvedScopeId
         << "}";
}

void ASTJsonSerializer::visit(BinaryExprNode& node) {
    out_ << "{"
         << "\"nodeType\":\"BinaryExpr\","
         << "\"line\":" << node.line << ","
         << "\"op\":\"" << escapeJson(operatorToString(node.op)) << "\","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\","
         << "\"left\":";
    visitOrNull(node.left.get());
    out_ << ",\"right\":";
    visitOrNull(node.right.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(UnaryExprNode& node) {
    out_ << "{"
         << "\"nodeType\":\"UnaryExpr\","
         << "\"line\":" << node.line << ","
         << "\"op\":\"" << escapeJson(operatorToString(node.op)) << "\","
         << "\"inferredType\":\"" << escapeJson(dataTypeToString(node.inferredType)) << "\","
         << "\"operand\":";
    visitOrNull(node.operand.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(VarDeclNode& node) {
    out_ << "{"
         << "\"nodeType\":\"VarDecl\","
         << "\"line\":" << node.line << ","
         << "\"varType\":\"" << escapeJson(dataTypeToString(node.varType)) << "\","
         << "\"name\":\"" << escapeJson(node.name) << "\","
         << "\"resolvedScopeId\":" << node.resolvedScopeId << ","
         << "\"initializer\":";
    visitOrNull(node.initializer.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(AssignNode& node) {
    out_ << "{"
         << "\"nodeType\":\"Assign\","
         << "\"line\":" << node.line << ","
         << "\"name\":\"" << escapeJson(node.name) << "\","
         << "\"resolvedScopeId\":" << node.resolvedScopeId << ","
         << "\"value\":";
    visitOrNull(node.value.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(PrintNode& node) {
    out_ << "{"
         << "\"nodeType\":\"Print\","
         << "\"line\":" << node.line << ","
         << "\"flavor\":\"" << escapeJson(printFlavorToString(node.flavor)) << "\","
         << "\"expression\":";
    visitOrNull(node.expression.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(BlockNode& node) {
    out_ << "{"
         << "\"nodeType\":\"Block\","
         << "\"line\":" << node.line << ","
         << "\"scopeId\":" << node.scopeId << ","
         << "\"statements\":[";
    for (size_t i = 0; i < node.statements.size(); ++i) {
        if (i > 0) {
            out_ << ",";
        }
        visitOrNull(node.statements[i].get());
    }
    out_ << "]}";
}

void ASTJsonSerializer::visit(IfNode& node) {
    out_ << "{"
         << "\"nodeType\":\"If\","
         << "\"line\":" << node.line << ","
         << "\"condition\":";
    visitOrNull(node.condition.get());
    out_ << ",\"thenBranch\":";
    visitOrNull(node.thenBranch.get());
    out_ << ",\"elseBranch\":";
    visitOrNull(node.elseBranch.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(WhileNode& node) {
    out_ << "{"
         << "\"nodeType\":\"While\","
         << "\"line\":" << node.line << ","
         << "\"condition\":";
    visitOrNull(node.condition.get());
    out_ << ",\"body\":";
    visitOrNull(node.body.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(ForNode& node) {
    out_ << "{"
         << "\"nodeType\":\"For\","
         << "\"line\":" << node.line << ","
         << "\"scopeId\":" << node.scopeId << ","
         << "\"init\":";
    visitOrNull(node.init.get());
    out_ << ",\"condition\":";
    visitOrNull(node.condition.get());
    out_ << ",\"update\":";
    visitOrNull(node.update.get());
    out_ << ",\"body\":";
    visitOrNull(node.body.get());
    out_ << "}";
}

void ASTJsonSerializer::visit(ProgramNode& node) {
    out_ << "{"
         << "\"nodeType\":\"Program\","
         << "\"line\":" << node.line << ","
         << "\"statements\":[";
    for (size_t i = 0; i < node.statements.size(); ++i) {
        if (i > 0) {
            out_ << ",";
        }
        visitOrNull(node.statements[i].get());
    }
    out_ << "]}";
}

} // namespace codeforge