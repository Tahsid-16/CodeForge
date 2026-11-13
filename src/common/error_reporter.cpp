#include "common/error_reporter.h"
#include <iostream>
#include <sstream>

namespace codeforge {

void ErrorReporter::report(ErrorPhase phase, int line, int column, const std::string& message) {
    errors_.emplace_back(phase, line, column, message);
}

void ErrorReporter::reportLexical(int line, const std::string& message, int column) {
    report(ErrorPhase::LEXICAL, line, column, message);
}

void ErrorReporter::reportSyntax(int line, const std::string& message, int column) {
    report(ErrorPhase::SYNTAX, line, column, message);
}

void ErrorReporter::reportSemantic(int line, const std::string& message, int column) {
    report(ErrorPhase::SEMANTIC, line, column, message);
}

bool ErrorReporter::hasErrors() const {
    return !errors_.empty();
}

size_t ErrorReporter::errorCount() const {
    return errors_.size();
}

const std::vector<DiagnosticError>& ErrorReporter::getErrors() const {
    return errors_;
}

void ErrorReporter::clear() {
    errors_.clear();
}

void ErrorReporter::printToTerminal() const {
    for (const auto& err : errors_) {
        std::cerr << "[" << errorPhaseToString(err.phase) << "] "
                  << "Line " << err.line << ": " << err.message << "\n";
    }
}

std::string ErrorReporter::toJson() const {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < errors_.size(); ++i) {
        const auto& err = errors_[i];
        ss << "    {\n"
           << "      \"phase\": \"" << escapeJson(errorPhaseToString(err.phase)) << "\",\n"
           << "      \"line\": " << err.line << ",\n"
           << "      \"column\": " << err.column << ",\n"
           << "      \"message\": \"" << escapeJson(err.message) << "\"\n"
           << "    }" << (i + 1 < errors_.size() ? "," : "") << "\n";
    }
    ss << "  ]";
    return ss.str();
}

} // namespace codeforge