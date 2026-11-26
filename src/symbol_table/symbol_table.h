#ifndef CODEFORGE_SYMBOL_TABLE_SYMBOL_TABLE_H
#define CODEFORGE_SYMBOL_TABLE_SYMBOL_TABLE_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/types.h"

namespace codeforge {

struct Symbol {
    std::string name;
    DataType type;
    int scopeId;
    int scopeDepth;
    std::string scopeLabel;
    int lineDeclared;
    bool isInitialized;
    bool isUsed;

    Symbol(std::string symName,
           DataType symType,
           int sId,
           int sDepth,
           std::string sLabel,
           int line,
           bool initialized = false)
        : name(std::move(symName)),
          type(symType),
          scopeId(sId),
          scopeDepth(sDepth),
          scopeLabel(std::move(sLabel)),
          lineDeclared(line),
          isInitialized(initialized),
          isUsed(false) {}
};

struct Scope {
    int id;
    int parentId;
    int depth;
    std::string label;
    std::unordered_map<std::string, size_t> symbolIndices;

    Scope(int scopeId, int parentScopeId, int scopeDepth, std::string scopeLabel)
        : id(scopeId),
          parentId(parentScopeId),
          depth(scopeDepth),
          label(std::move(scopeLabel)) {}
};

class SymbolTable {
private:
    std::vector<Scope> scopes_;
    std::vector<Symbol> allSymbols_;
    int currentScopeId_;

public:
    SymbolTable();

    // Scope management
    int enterScope(const std::string& label = "block");
    void exitScope();
    int getCurrentScopeId() const;
    int getCurrentScopeDepth() const;

    // Symbol declaration & lookup
    bool declare(const std::string& name, DataType type, int line, bool initialized = false);
    Symbol* lookup(const std::string& name);
    const Symbol* lookup(const std::string& name) const;
    Symbol* lookupCurrentScope(const std::string& name);
    const Symbol* lookupCurrentScope(const std::string& name) const;
    Symbol* lookupInScope(const std::string& name, int scopeId);
    const Symbol* lookupInScope(const std::string& name, int scopeId) const;

    // Inspection & lifecycle
    const std::vector<Symbol>& getAllSymbols() const;
    size_t symbolCount() const;
    void clear();

    // Output formatters for CLI and Web UI (--json)
    void printToTerminal() const;
    std::string toJson() const;
};

} // namespace codeforge

#endif // CODEFORGE_SYMBOL_TABLE_SYMBOL_TABLE_H