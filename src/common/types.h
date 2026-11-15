#ifndef CODEFORGE_COMMON_TYPES_H
#define CODEFORGE_COMMON_TYPES_H

#include <string>
#include <vector>
#include <sstream>

namespace codeforge {

    // Target language mode for Strict Mode enforcement
enum class SourceLanguage {
    AUTO, // Infer from file extension (.c, .cpp, .java), fallback to permissive
    C,
    CPP,
    JAVA
};

// Tracks exact type keyword used in source code for strict language validation
enum class TypeKeyword {
    KW_INT,
    KW_FLOAT,
    KW_BOOL,    // C / C++
    KW_BOOLEAN  // Java only
};

inline std::string sourceLanguageToString(SourceLanguage lang) {
    switch (lang) {
        case SourceLanguage::AUTO: return "auto";
        case SourceLanguage::C:    return "C";
        case SourceLanguage::CPP:  return "C++";
        case SourceLanguage::JAVA: return "Java";
    }
    return "auto";
}

// ============================================================================
// Core Type System
// ============================================================================
enum class DataType {
    INT,
    FLOAT,
    BOOL,
    VOID,
    UNKNOWN, // Initial state before semantic analysis visits the node
    ERROR    // Poison type used to prevent cascading semantic errors
};

// ============================================================================
// Operators (Binary & Unary)
// ============================================================================
enum class Operator {
    // Arithmetic
    ADD,        // +
    SUB,        // -
    MUL,        // *
    DIV,        // /
    MOD,        // %

    // Relational
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NEQ,        // !=

    // Logical
    AND,        // &&
    OR,         // ||
    NOT,        // ! (Unary)

    // Unary Sign
    NEG,        // - (Unary negation)
    POS         // + (Unary plus)
};

// Records which surface syntax produced a PrintNode (for AST visualization)
enum class PrintFlavor {
    GENERIC_PRINT, // print(x);
    C_PRINTF,      // printf(x);
    CPP_COUT,      // cout << x; / std::cout << x;
    JAVA_SYSOUT    // System.out.println(x);
};

// ============================================================================
// Token & Diagnostic Contracts (Shared by Lexer, Parser, Semantics, and CLI/Web)
// ============================================================================
struct TokenInfo {
    std::string type;   // e.g., "TOKEN_INT", "TOKEN_IDENT"
    std::string lexeme; // e.g., "int", "counter"
    int line;
    int column;

    TokenInfo(std::string t, std::string lex, int ln, int col = 1)
        : type(std::move(t)), lexeme(std::move(lex)), line(ln), column(col) {}
};

enum class ErrorPhase {
    LEXICAL,
    SYNTAX,
    SEMANTIC
};

struct DiagnosticError {
    ErrorPhase phase;
    int line;
    int column;
    std::string message;

    DiagnosticError(ErrorPhase p, int ln, int col, std::string msg)
        : phase(p), line(ln), column(col), message(std::move(msg)) {}
};

// ============================================================================
// Operator Classification Helpers (Used by Semantic Analyzer & TAC Generator)
// ============================================================================
inline bool isArithmeticOp(Operator op) {
    return op == Operator::ADD || op == Operator::SUB ||
           op == Operator::MUL || op == Operator::DIV || op == Operator::MOD;
}

inline bool isRelationalOp(Operator op) {
    return op == Operator::LT || op == Operator::GT ||
           op == Operator::LE || op == Operator::GE;
}

inline bool isEqualityOp(Operator op) {
    return op == Operator::EQ || op == Operator::NEQ;
}

inline bool isLogicalOp(Operator op) {
    return op == Operator::AND || op == Operator::OR || op == Operator::NOT;
}

// ============================================================================
// String Conversion & JSON Serialization Helpers
// ============================================================================
inline std::string dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT:     return "int";
        case DataType::FLOAT:   return "float";
        case DataType::BOOL:    return "bool";
        case DataType::VOID:    return "void";
        case DataType::UNKNOWN: return "unknown";
        case DataType::ERROR:   return "error";
    }
    return "unknown";
}

inline std::string operatorToString(Operator op) {
    switch (op) {
        case Operator::ADD: return "+";
        case Operator::SUB: return "-";
        case Operator::MUL: return "*";
        case Operator::DIV: return "/";
        case Operator::MOD: return "%";
        case Operator::LT:  return "<";
        case Operator::GT:  return ">";
        case Operator::LE:  return "<=";
        case Operator::GE:  return ">=";
        case Operator::EQ:  return "==";
        case Operator::NEQ: return "!=";
        case Operator::AND: return "&&";
        case Operator::OR:  return "||";
        case Operator::NOT: return "!";
        case Operator::NEG: return "-";
        case Operator::POS: return "+";
    }
    return "?";
}

inline std::string printFlavorToString(PrintFlavor flavor) {
    switch (flavor) {
        case PrintFlavor::GENERIC_PRINT: return "print";
        case PrintFlavor::C_PRINTF:      return "printf (C)";
        case PrintFlavor::CPP_COUT:      return "cout (C++)";
        case PrintFlavor::JAVA_SYSOUT:   return "System.out.println (Java)";
    }
    return "print";
}

inline std::string errorPhaseToString(ErrorPhase phase) {
    switch (phase) {
        case ErrorPhase::LEXICAL:  return "Lexical Error";
        case ErrorPhase::SYNTAX:   return "Syntax Error";
        case ErrorPhase::SEMANTIC: return "Semantic Error";
    }
    return "Error";
}

// Safely escapes strings for --json output so quotes/newlines don't break JSON.parse()
inline std::string escapeJson(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:   ss << c;      break;
        }
    }
    return ss.str();
}

} // namespace codeforge

#endif // CODEFORGE_COMMON_TYPES_H