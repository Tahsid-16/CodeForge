#ifndef CODEFORGE_CODEGEN_TAC_H
#define CODEFORGE_CODEGEN_TAC_H

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include "common/types.h"

namespace codeforge {

enum class TACOp {
    // Assignment
    ASSIGN,

    // Binary Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    // Unary
    NEG,
    POS,
    NOT,

    // Relational & Equality
    EQ,
    NEQ,
    LT,
    GT,
    LE,
    GE,

    // Logical
    AND,
    OR,

    // Control Flow & I/O
    LABEL,
    GOTO,
    IF_FALSE,
    PRINT
};

std::string tacOpToString(TACOp op);

struct TACInstruction {
    TACOp op;
    std::string result;
    std::string arg1;
    std::string arg2;
    int line;
    bool arg1IsString;
    bool arg2IsString;
    DataType resultType;

    TACInstruction(TACOp operation,
                   std::string res = "",
                   std::string a1 = "",
                   std::string a2 = "",
                   int ln = 0,
                   bool a1Str = false,
                   bool a2Str = false,
                   DataType resType = DataType::UNKNOWN)
        : op(operation),
          result(std::move(res)),
          arg1(std::move(a1)),
          arg2(std::move(a2)),
          line(ln),
          arg1IsString(a1Str),
          arg2IsString(a2Str),
          resultType(resType) {}

    std::string formatArg1() const;
    std::string formatArg2() const;
    std::string toString() const;
};

class TACProgram {
private:
    std::vector<TACInstruction> instructions_;

public:
    TACProgram() = default;

    void emit(TACInstruction instr);
    void clear();

    const std::vector<TACInstruction>& getInstructions() const;
    bool empty() const;
    size_t size() const;

    void printToTerminal(std::ostream& os = std::cout) const;
    std::string toJson() const;
};

} // namespace codeforge

#endif // CODEFORGE_CODEGEN_TAC_H