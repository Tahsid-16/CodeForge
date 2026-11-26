#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ast/ast.h"
#include "ast/ast_printer.h"
#include "common/error_reporter.h"
#include "common/types.h"
#include "semantic/semantic_analyzer.h"
#include "symbol_table/symbol_table.h"

// Flex & Bison external symbols
extern FILE* yyin;
extern int yyparse();
extern void lexer_reset();
extern std::vector<codeforge::TokenInfo> g_tokens;
extern codeforge::ErrorReporter* g_error_reporter;
extern codeforge::ProgramNode* g_ast_root;

namespace {

using namespace codeforge;

std::string toLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool parseLanguageFlag(const std::string& raw, SourceLanguage& out) {
    std::string val = toLowerCopy(raw);
    if (val == "auto") {
        out = SourceLanguage::AUTO;
        return true;
    }
    if (val == "c") {
        out = SourceLanguage::C;
        return true;
    }
    if (val == "cpp" || val == "c++") {
        out = SourceLanguage::CPP;
        return true;
    }
    if (val == "java") {
        out = SourceLanguage::JAVA;
        return true;
    }
    return false;
}

SourceLanguage inferLanguageFromFilename(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return SourceLanguage::AUTO;
    }
    std::string ext = toLowerCopy(path.substr(dot));
    if (ext == ".c") {
        return SourceLanguage::C;
    }
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
        return SourceLanguage::CPP;
    }
    if (ext == ".java") {
        return SourceLanguage::JAVA;
    }
    return SourceLanguage::AUTO;
}

void printTokensTerminal(const std::vector<TokenInfo>& tokens) {
    std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
    if (tokens.empty()) {
        std::cout << "(none)\n";
        return;
    }
    std::cout << std::left
              << std::setw(20) << "Type"
              << std::setw(24) << "Lexeme"
              << std::setw(8)  << "Line"
              << "Col\n";
    std::cout << std::string(58, '-') << "\n";
    for (const auto& tok : tokens) {
        std::cout << std::left
                  << std::setw(20) << tok.type
                  << std::setw(24) << tok.lexeme
                  << std::setw(8)  << tok.line
                  << tok.column << "\n";
    }
}

std::string tokensToJson(const std::vector<TokenInfo>& tokens) {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << "{"
           << "\"type\":\"" << escapeJson(tokens[i].type) << "\","
           << "\"lexeme\":\"" << escapeJson(tokens[i].lexeme) << "\","
           << "\"line\":" << tokens[i].line << ","
           << "\"column\":" << tokens[i].column
           << "}";
    }
    ss << "]";
    return ss.str();
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " <source_file> [options]\n"
              << "Options:\n"
              << "  --lang=<auto|c|cpp|java>  Set strict language mode (default: infer from extension)\n"
              << "  --tokens                  Print lexer token stream\n"
              << "  --ast                     Print annotated Abstract Syntax Tree\n"
              << "  --symbols                 Print Symbol Table\n"
              << "  --all                     Print tokens, AST, and Symbol Table\n"
              << "  --json                    Emit structured JSON output for Web UI\n"
              << "  -h, --help                Show this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    SourceLanguage langArg = SourceLanguage::AUTO;
    bool langExplicit = false;
    bool showTokens = false;
    bool showAst = false;
    bool showSymbols = false;
    bool jsonMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--tokens") {
            showTokens = true;
        } else if (arg == "--ast") {
            showAst = true;
        } else if (arg == "--symbols") {
            showSymbols = true;
        } else if (arg == "--all") {
            showTokens = true;
            showAst = true;
            showSymbols = true;
        } else if (arg == "--json") {
            jsonMode = true;
        } else if (arg.rfind("--lang=", 0) == 0) {
            if (!parseLanguageFlag(arg.substr(7), langArg)) {
                std::cerr << "Invalid --lang value: " << arg.substr(7) << "\n";
                return 1;
            }
            langExplicit = true;
        } else if (arg == "--lang" && i + 1 < argc) {
            std::string nextVal = argv[++i];
            if (!parseLanguageFlag(nextVal, langArg)) {
                std::cerr << "Invalid --lang value: " << nextVal << "\n";
                return 1;
            }
            langExplicit = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else if (inputPath.empty()) {
            inputPath = arg;
        } else {
            std::cerr << "Unexpected extra argument: " << arg << "\n";
            return 1;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Error: no input source file provided.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Default CLI view when no specific section flags are passed
    if (!showTokens && !showAst && !showSymbols) {
        showAst = true;
        showSymbols = true;
    }

    SourceLanguage effectiveLang =
        langExplicit ? langArg : inferLanguageFromFilename(inputPath);

    FILE* file = std::fopen(inputPath.c_str(), "r");
    if (!file) {
        std::cerr << "Error: could not open file '" << inputPath << "'.\n";
        return 1;
    }

    codeforge::ErrorReporter errorReporter;
    codeforge::SymbolTable symbolTable;

    lexer_reset();
    g_error_reporter = &errorReporter;
    g_ast_root = nullptr;
    yyin = file;

    yyparse();
    std::fclose(file);
    yyin = nullptr;
    g_error_reporter = nullptr;

    std::unique_ptr<codeforge::ProgramNode> astRoot(g_ast_root);
    g_ast_root = nullptr;

    // Run semantic analysis whenever the parser produced an AST
    if (astRoot) {
        codeforge::SemanticAnalyzer analyzer(symbolTable, errorReporter, effectiveLang);
        analyzer.analyze(*astRoot);
    }

    bool success = !errorReporter.hasErrors();

    if (jsonMode) {
        codeforge::ASTJsonSerializer astJson;
        std::cout << "{"
                  << "\"success\":" << (success ? "true" : "false") << ","
                  << "\"file\":\"" << codeforge::escapeJson(inputPath) << "\","
                  << "\"language\":\"" << codeforge::escapeJson(codeforge::sourceLanguageToString(effectiveLang)) << "\","
                  << "\"tokens\":" << tokensToJson(g_tokens) << ","
                  << "\"ast\":" << (astRoot ? astJson.toJson(*astRoot) : "null") << ","
                  << "\"symbols\":" << symbolTable.toJson() << ","
                  << "\"errors\":" << errorReporter.toJson()
                  << "}\n";
        return success ? 0 : 1;
    }

    std::cout << "CodeForge Compiler | File: " << inputPath
              << " | Mode: " << codeforge::sourceLanguageToString(effectiveLang)
              << " | Tokens: " << g_tokens.size() << "\n\n";

    if (showTokens) {
        printTokensTerminal(g_tokens);
        std::cout << "\n";
    }

    if (showAst && astRoot) {
        codeforge::ASTPrinter printer;
        printer.print(*astRoot);
        std::cout << "\n";
    }

    if (showSymbols) {
        symbolTable.printToTerminal();
        std::cout << "\n";
    }

    if (errorReporter.hasErrors()) {
        errorReporter.printToTerminal();
        return 1;
    }

    std::cout << "Semantic analysis passed with 0 errors.\n";
    return 0;
}