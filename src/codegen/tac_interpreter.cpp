#include "codegen/tac_interpreter.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace codeforge {

// ============================================================================
// RuntimeValue
// ============================================================================

RuntimeValue RuntimeValue::makeInt(int v) {
    RuntimeValue val;
    val.type = DataType::INT;
    val.intVal = v;
    return val;
}

RuntimeValue RuntimeValue::makeFloat(double v) {
    RuntimeValue val;
    val.type = DataType::FLOAT;
    val.floatVal = (v == 0.0) ? 0.0 : v; // Normalize -0.0 to 0.0
    return val;
}

RuntimeValue RuntimeValue::makeBool(bool v) {
    RuntimeValue val;
    val.type = DataType::BOOL;
    val.boolVal = v;
    return val;
}

RuntimeValue RuntimeValue::makeString(std::string v) {
    RuntimeValue val;
    val.type = DataType::STRING;
    val.strVal = std::move(v);
    return val;
}

double RuntimeValue::asDouble() const {
    if (type == DataType::FLOAT) {
        return floatVal;
    }
    if (type == DataType::INT) {
        return static_cast<double>(intVal);
    }
    if (type == DataType::BOOL) {
        return boolVal ? 1.0 : 0.0;
    }
    return 0.0;
}

bool RuntimeValue::asBool() const {
    if (type == DataType::BOOL) {
        return boolVal;
    }
    if (type == DataType::INT) {
        return intVal != 0;
    }
    if (type == DataType::FLOAT) {
        return floatVal != 0.0;
    }
    return !strVal.empty();
}

std::string RuntimeValue::toDisplayString() const {
    switch (type) {
        case DataType::INT:
            return std::to_string(intVal);
        case DataType::FLOAT: {
            std::ostringstream ss;
            ss << std::setprecision(12) << floatVal;
            return ss.str();
        }
        case DataType::BOOL:
            return boolVal ? "true" : "false";
        case DataType::STRING:
            return strVal;
        default:
            return "";
    }
}

// ============================================================================
// TACInterpreter
// ============================================================================

TACInterpreter::TACInterpreter(size_t maxSteps) : maxSteps_(maxSteps) {}

void TACInterpreter::reset() {
    stepsExecuted_ = 0;
    env_.clear();
    outputs_.clear();
    hasRuntimeError_ = false;
    runtimeError_.clear();
}

void TACInterpreter::setRuntimeError(int line, const std::string& message) {
    hasRuntimeError_ = true;
    if (line > 0) {
        runtimeError_ = "Runtime Error at line " + std::to_string(line) + ": " + message;
    } else {
        runtimeError_ = "Runtime Error: " + message;
    }
}

RuntimeValue TACInterpreter::resolveOperand(const std::string& operand, bool isString) const {
    if (isString) {
        return RuntimeValue::makeString(operand);
    }
    if (operand == "true") {
        return RuntimeValue::makeBool(true);
    }
    if (operand == "false") {
        return RuntimeValue::makeBool(false);
    }

    auto it = env_.find(operand);
    if (it != env_.end()) {
        return it->second;
    }

    // Parse numeric literals
    if (!operand.empty()) {
        bool looksNumeric = (operand[0] >= '0' && operand[0] <= '9') ||
                            operand[0] == '.' ||
                            ((operand[0] == '-' || operand[0] == '+') && operand.size() > 1);
        if (looksNumeric) {
            try {
                if (operand.find('.') != std::string::npos ||
                    operand.find('e') != std::string::npos ||
                    operand.find('E') != std::string::npos) {
                    return RuntimeValue::makeFloat(std::stod(operand));
                }
                return RuntimeValue::makeInt(std::stoi(operand));
            } catch (...) {
                // Fall through to default 0
            }
        }
    }

    return RuntimeValue::makeInt(0);
}

bool TACInterpreter::execute(const TACProgram& program) {
    reset();

    const auto& instructions = program.getInstructions();
    if (instructions.empty()) {
        return true;
    }

    // Pre-index all LABEL positions for O(1) jumps
    std::unordered_map<std::string, size_t> labelPositions;
    for (size_t i = 0; i < instructions.size(); ++i) {
        if (instructions[i].op == TACOp::LABEL) {
            labelPositions[instructions[i].result] = i;
        }
    }

    size_t pc = 0;
    while (pc < instructions.size()) {
        if (++stepsExecuted_ > maxSteps_) {
            setRuntimeError(
                instructions[pc].line,
                "Execution halted after exceeding step limit (" +
                    std::to_string(maxSteps_) + " instructions). Possible infinite loop."
            );
            return false;
        }

        const auto& instr = instructions[pc];

        switch (instr.op) {
            case TACOp::LABEL:
                ++pc;
                break;

            case TACOp::GOTO: {
                auto it = labelPositions.find(instr.result);
                if (it == labelPositions.end()) {
                    setRuntimeError(instr.line, "Jump to undefined label '" + instr.result + "'.");
                    return false;
                }
                pc = it->second;
                break;
            }

            case TACOp::IF_FALSE: {
                RuntimeValue cond = resolveOperand(instr.arg1, instr.arg1IsString);
                if (!cond.asBool()) {
                    auto it = labelPositions.find(instr.result);
                    if (it == labelPositions.end()) {
                        setRuntimeError(instr.line, "Branch to undefined label '" + instr.result + "'.");
                        return false;
                    }
                    pc = it->second;
                } else {
                    ++pc;
                }
                break;
            }

            case TACOp::ASSIGN: {
                RuntimeValue val = resolveOperand(instr.arg1, instr.arg1IsString);
                // Preserve target float type on int -> float widening
                if (instr.resultType == DataType::FLOAT && val.type == DataType::INT) {
                    val = RuntimeValue::makeFloat(static_cast<double>(val.intVal));
                } else if (auto existing = env_.find(instr.result);
                           existing != env_.end() &&
                           existing->second.type == DataType::FLOAT &&
                           val.type == DataType::INT) {
                    val = RuntimeValue::makeFloat(static_cast<double>(val.intVal));
                }
                env_[instr.result] = std::move(val);
                ++pc;
                break;
            }

            case TACOp::ADD:
            case TACOp::SUB:
            case TACOp::MUL:
            case TACOp::DIV:
            case TACOp::MOD: {
                RuntimeValue lhs = resolveOperand(instr.arg1, instr.arg1IsString);
                RuntimeValue rhs = resolveOperand(instr.arg2, instr.arg2IsString);

                if (instr.op == TACOp::MOD) {
                    if (rhs.intVal == 0) {
                        setRuntimeError(instr.line, "Modulo by zero.");
                        return false;
                    }
                    env_[instr.result] = RuntimeValue::makeInt(lhs.intVal % rhs.intVal);
                    ++pc;
                    break;
                }

                bool useFloat = (lhs.type == DataType::FLOAT || rhs.type == DataType::FLOAT ||
                                 instr.resultType == DataType::FLOAT);

                if (instr.op == TACOp::DIV) {
                    if (useFloat) {
                        if (rhs.asDouble() == 0.0) {
                            setRuntimeError(instr.line, "Division by zero.");
                            return false;
                        }
                        env_[instr.result] = RuntimeValue::makeFloat(lhs.asDouble() / rhs.asDouble());
                    } else {
                        if (rhs.intVal == 0) {
                            setRuntimeError(instr.line, "Division by zero.");
                            return false;
                        }
                        env_[instr.result] = RuntimeValue::makeInt(lhs.intVal / rhs.intVal);
                    }
                    ++pc;
                    break;
                }

                if (useFloat) {
                    double a = lhs.asDouble();
                    double b = rhs.asDouble();
                    double res = 0.0;
                    if (instr.op == TACOp::ADD) res = a + b;
                    else if (instr.op == TACOp::SUB) res = a - b;
                    else if (instr.op == TACOp::MUL) res = a * b;
                    env_[instr.result] = RuntimeValue::makeFloat(res);
                } else {
                    int a = lhs.intVal;
                    int b = rhs.intVal;
                    int res = 0;
                    if (instr.op == TACOp::ADD) res = a + b;
                    else if (instr.op == TACOp::SUB) res = a - b;
                    else if (instr.op == TACOp::MUL) res = a * b;
                    env_[instr.result] = RuntimeValue::makeInt(res);
                }
                ++pc;
                break;
            }

            case TACOp::NEG: {
                RuntimeValue val = resolveOperand(instr.arg1, instr.arg1IsString);
                if (val.type == DataType::FLOAT) {
                    env_[instr.result] = RuntimeValue::makeFloat(-val.floatVal);
                } else {
                    env_[instr.result] = RuntimeValue::makeInt(-val.intVal);
                }
                ++pc;
                break;
            }

            case TACOp::POS: {
                RuntimeValue val = resolveOperand(instr.arg1, instr.arg1IsString);
                env_[instr.result] = std::move(val);
                ++pc;
                break;
            }

            case TACOp::NOT: {
                RuntimeValue val = resolveOperand(instr.arg1, instr.arg1IsString);
                env_[instr.result] = RuntimeValue::makeBool(!val.asBool());
                ++pc;
                break;
            }

            case TACOp::EQ:
            case TACOp::NEQ: {
                RuntimeValue lhs = resolveOperand(instr.arg1, instr.arg1IsString);
                RuntimeValue rhs = resolveOperand(instr.arg2, instr.arg2IsString);
                bool equal = false;

                if (lhs.type == DataType::STRING && rhs.type == DataType::STRING) {
                    equal = (lhs.strVal == rhs.strVal);
                } else if (lhs.type == DataType::BOOL && rhs.type == DataType::BOOL) {
                    equal = (lhs.boolVal == rhs.boolVal);
                } else {
                    equal = (lhs.asDouble() == rhs.asDouble());
                }

                env_[instr.result] = RuntimeValue::makeBool(
                    instr.op == TACOp::EQ ? equal : !equal
                );
                ++pc;
                break;
            }

            case TACOp::LT:
            case TACOp::GT:
            case TACOp::LE:
            case TACOp::GE: {
                RuntimeValue lhs = resolveOperand(instr.arg1, instr.arg1IsString);
                RuntimeValue rhs = resolveOperand(instr.arg2, instr.arg2IsString);
                double a = lhs.asDouble();
                double b = rhs.asDouble();
                bool res = false;

                if (instr.op == TACOp::LT) res = (a < b);
                else if (instr.op == TACOp::GT) res = (a > b);
                else if (instr.op == TACOp::LE) res = (a <= b);
                else if (instr.op == TACOp::GE) res = (a >= b);

                env_[instr.result] = RuntimeValue::makeBool(res);
                ++pc;
                break;
            }

            case TACOp::AND: {
                RuntimeValue lhs = resolveOperand(instr.arg1, instr.arg1IsString);
                RuntimeValue rhs = resolveOperand(instr.arg2, instr.arg2IsString);
                env_[instr.result] = RuntimeValue::makeBool(lhs.asBool() && rhs.asBool());
                ++pc;
                break;
            }

            case TACOp::OR: {
                RuntimeValue lhs = resolveOperand(instr.arg1, instr.arg1IsString);
                RuntimeValue rhs = resolveOperand(instr.arg2, instr.arg2IsString);
                env_[instr.result] = RuntimeValue::makeBool(lhs.asBool() || rhs.asBool());
                ++pc;
                break;
            }

            case TACOp::PRINT: {
                RuntimeValue val = resolveOperand(instr.arg1, instr.arg1IsString);
                outputs_.push_back(val.toDisplayString());
                ++pc;
                break;
            }
        }
    }

    return true;
}

const std::vector<std::string>& TACInterpreter::getOutputs() const {
    return outputs_;
}

bool TACInterpreter::hasRuntimeError() const {
    return hasRuntimeError_;
}

const std::string& TACInterpreter::getRuntimeError() const {
    return runtimeError_;
}

size_t TACInterpreter::getStepsExecuted() const {
    return stepsExecuted_;
}

void TACInterpreter::printOutputToTerminal(std::ostream& os) const {
    os << "=== Program Output ===\n";
    if (outputs_.empty() && !hasRuntimeError_) {
        os << "(no output)\n";
        return;
    }

    for (const auto& line : outputs_) {
        os << line << "\n";
    }

    if (hasRuntimeError_) {
        os << "[" << runtimeError_ << "]\n";
    }
}

std::string TACInterpreter::outputsToJson() const {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < outputs_.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << "\"" << escapeJson(outputs_[i]) << "\"";
    }
    ss << "]";
    return ss.str();
}

std::string TACInterpreter::runtimeErrorToJson() const {
    if (!hasRuntimeError_) {
        return "null";
    }
    return "\"" + escapeJson(runtimeError_) + "\"";
}

} // namespace codeforge