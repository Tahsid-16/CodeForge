#ifndef CODEFORGE_CODEGEN_TAC_INTERPRETER_H
#define CODEFORGE_CODEGEN_TAC_INTERPRETER_H

#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "codegen/tac.h"
#include "common/types.h"

namespace codeforge {

struct RuntimeValue {
    DataType type = DataType::INT;
    int intVal = 0;
    double floatVal = 0.0;
    bool boolVal = false;
    std::string strVal;

    static RuntimeValue makeInt(int v);
    static RuntimeValue makeFloat(double v);
    static RuntimeValue makeBool(bool v);
    static RuntimeValue makeString(std::string v);

    double asDouble() const;
    bool asBool() const;
    std::string toDisplayString() const;
};

class TACInterpreter {
private:
    size_t maxSteps_;
    size_t stepsExecuted_ = 0;
    std::unordered_map<std::string, RuntimeValue> env_;
    std::vector<std::string> outputs_;
    bool hasRuntimeError_ = false;
    std::string runtimeError_;

    RuntimeValue resolveOperand(const std::string& operand, bool isString) const;
    void setRuntimeError(int line, const std::string& message);

public:
    explicit TACInterpreter(size_t maxSteps = 10000);

    // Executes a TACProgram; returns true if execution finished with no runtime error
    bool execute(const TACProgram& program);
    void reset();

    const std::vector<std::string>& getOutputs() const;
    bool hasRuntimeError() const;
    const std::string& getRuntimeError() const;
    size_t getStepsExecuted() const;

    // CLI & --json formatters
    void printOutputToTerminal(std::ostream& os = std::cout) const;
    std::string outputsToJson() const;
    std::string runtimeErrorToJson() const;
};

} // namespace codeforge

#endif // CODEFORGE_CODEGEN_TAC_INTERPRETER_H