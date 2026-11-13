#ifndef CODEFORGE_COMMON_ERROR_REPORTER_H
#define CODEFORGE_COMMON_ERROR_REPORTER_H

#include <string>
#include <vector>
#include "common/types.h"

namespace codeforge {

class ErrorReporter {
private:
    std::vector<DiagnosticError> errors_;

public:
    ErrorReporter() = default;

    // Core logging methods used by Track 1 and Track 2
    void report(ErrorPhase phase, int line, int column, const std::string& message);
    void reportLexical(int line, const std::string& message, int column = 1);
    void reportSyntax(int line, const std::string& message, int column = 1);
    void reportSemantic(int line, const std::string& message, int column = 1);

    // State queries used by Track 3 (main.cpp)
    bool hasErrors() const;
    size_t errorCount() const;
    const std::vector<DiagnosticError>& getErrors() const;
    void clear();

    // Output formatters for CLI and Web UI (--json)
    void printToTerminal() const;
    std::string toJson() const;
};

} // namespace codeforge

#endif // CODEFORGE_COMMON_ERROR_REPORTER_H