#include "codegen/tac.h"
#include <iomanip>
#include <sstream>

namespace codeforge {

std::string tacOpToString(TACOp op) {
    switch (op) {
        case TACOp::ASSIGN:   return "ASSIGN";
        case TACOp::ADD:      return "ADD";
        case TACOp::SUB:      return "SUB";
        case TACOp::MUL:      return "MUL";
        case TACOp::DIV:      return "DIV";
        case TACOp::MOD:      return "MOD";
        case TACOp::NEG:      return "NEG";
        case TACOp::POS:      return "POS";
        case TACOp::NOT:      return "NOT";
        case TACOp::EQ:       return "EQ";
        case TACOp::NEQ:      return "NEQ";
        case TACOp::LT:       return "LT";
        case TACOp::GT:       return "GT";
        case TACOp::LE:       return "LE";
        case TACOp::GE:       return "GE";
        case TACOp::AND:      return "AND";
        case TACOp::OR:       return "OR";
        case TACOp::LABEL:    return "LABEL";
        case TACOp::GOTO:     return "GOTO";
        case TACOp::IF_FALSE: return "IF_FALSE";
        case TACOp::PRINT:    return "PRINT";
    }
    return "UNKNOWN";
}

namespace {

std::string formatOperand(const std::string& val, bool isString) {
    if (!isString) {
        return val;
    }
    return "\"" + escapeJson(val) + "\"";
}

} // namespace

std::string TACInstruction::formatArg1() const {
    return formatOperand(arg1, arg1IsString);
}

std::string TACInstruction::formatArg2() const {
    return formatOperand(arg2, arg2IsString);
}

std::string TACInstruction::toString() const {
    const std::string a1 = formatArg1();
    const std::string a2 = formatArg2();

    switch (op) {
        case TACOp::ASSIGN:   return result + " = " + a1;
        case TACOp::ADD:      return result + " = " + a1 + " + " + a2;
        case TACOp::SUB:      return result + " = " + a1 + " - " + a2;
        case TACOp::MUL:      return result + " = " + a1 + " * " + a2;
        case TACOp::DIV:      return result + " = " + a1 + " / " + a2;
        case TACOp::MOD:      return result + " = " + a1 + " % " + a2;
        case TACOp::NEG:      return result + " = -" + a1;
        case TACOp::POS:      return result + " = +" + a1;
        case TACOp::NOT:      return result + " = !" + a1;
        case TACOp::EQ:       return result + " = " + a1 + " == " + a2;
        case TACOp::NEQ:      return result + " = " + a1 + " != " + a2;
        case TACOp::LT:       return result + " = " + a1 + " < " + a2;
        case TACOp::GT:       return result + " = " + a1 + " > " + a2;
        case TACOp::LE:       return result + " = " + a1 + " <= " + a2;
        case TACOp::GE:       return result + " = " + a1 + " >= " + a2;
        case TACOp::AND:      return result + " = " + a1 + " && " + a2;
        case TACOp::OR:       return result + " = " + a1 + " || " + a2;
        case TACOp::LABEL:    return result + ":";
        case TACOp::GOTO:     return "goto " + result;
        case TACOp::IF_FALSE: return "ifFalse " + a1 + " goto " + result;
        case TACOp::PRINT:    return "print " + a1;
    }
    return "";
}

void TACProgram::emit(TACInstruction instr) {
    instructions_.push_back(std::move(instr));
}

void TACProgram::clear() {
    instructions_.clear();
}

const std::vector<TACInstruction>& TACProgram::getInstructions() const {
    return instructions_;
}

bool TACProgram::empty() const {
    return instructions_.empty();
}

size_t TACProgram::size() const {
    return instructions_.size();
}

void TACProgram::printToTerminal(std::ostream& os) const {
    os << "=== Three-Address Code (TAC) ===\n";
    if (instructions_.empty()) {
        os << "(no instructions generated)\n";
        return;
    }

    for (size_t i = 0; i < instructions_.size(); ++i) {
        const auto& instr = instructions_[i];
        os << std::setw(4) << i << ":  ";
        if (instr.op == TACOp::LABEL) {
            os << instr.toString() << "\n";
        } else {
            os << "    " << instr.toString() << "\n";
        }
    }
}

std::string TACProgram::toJson() const {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < instructions_.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        const auto& instr = instructions_[i];
        ss << "{"
           << "\"index\":" << i << ","
           << "\"op\":\"" << escapeJson(tacOpToString(instr.op)) << "\","
           << "\"result\":\"" << escapeJson(instr.result) << "\","
           << "\"arg1\":\"" << escapeJson(instr.formatArg1()) << "\","
           << "\"arg2\":\"" << escapeJson(instr.formatArg2()) << "\","
           << "\"text\":\"" << escapeJson(instr.toString()) << "\","
           << "\"line\":" << instr.line
           << "}";
    }
    ss << "]";
    return ss.str();
}

} // namespace codeforge