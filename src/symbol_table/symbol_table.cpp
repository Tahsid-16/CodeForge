#include "symbol_table/symbol_table.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace codeforge {

SymbolTable::SymbolTable() : currentScopeId_(0) {
    clear();
}

void SymbolTable::clear() {
    scopes_.clear();
    allSymbols_.clear();
    scopes_.emplace_back(0, -1, 0, "global");
    currentScopeId_ = 0;
}

int SymbolTable::enterScope(const std::string& label) {
    int newId = static_cast<int>(scopes_.size());
    int parentDepth = scopes_[static_cast<size_t>(currentScopeId_)].depth;
    scopes_.emplace_back(newId, currentScopeId_, parentDepth + 1, label);
    currentScopeId_ = newId;
    return newId;
}

void SymbolTable::exitScope() {
    int parent = scopes_[static_cast<size_t>(currentScopeId_)].parentId;
    if (parent >= 0) {
        currentScopeId_ = parent;
    }
}

int SymbolTable::getCurrentScopeId() const {
    return currentScopeId_;
}

int SymbolTable::getCurrentScopeDepth() const {
    return scopes_[static_cast<size_t>(currentScopeId_)].depth;
}

bool SymbolTable::declare(const std::string& name, DataType type, int line, bool initialized) {
    Scope& activeScope = scopes_[static_cast<size_t>(currentScopeId_)];
    if (activeScope.symbolIndices.find(name) != activeScope.symbolIndices.end()) {
        return false; // Duplicate declaration in the same scope
    }

    size_t index = allSymbols_.size();
    allSymbols_.emplace_back(
        name,
        type,
        activeScope.id,
        activeScope.depth,
        activeScope.label,
        line,
        initialized
    );
    activeScope.symbolIndices[name] = index;
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    int sid = currentScopeId_;
    while (sid >= 0) {
        const Scope& scope = scopes_[static_cast<size_t>(sid)];
        auto it = scope.symbolIndices.find(name);
        if (it != scope.symbolIndices.end()) {
            return &allSymbols_[it->second];
        }
        sid = scope.parentId;
    }
    return nullptr;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    int sid = currentScopeId_;
    while (sid >= 0) {
        const Scope& scope = scopes_[static_cast<size_t>(sid)];
        auto it = scope.symbolIndices.find(name);
        if (it != scope.symbolIndices.end()) {
            return &allSymbols_[it->second];
        }
        sid = scope.parentId;
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrentScope(const std::string& name) {
    return lookupInScope(name, currentScopeId_);
}

const Symbol* SymbolTable::lookupCurrentScope(const std::string& name) const {
    return lookupInScope(name, currentScopeId_);
}

Symbol* SymbolTable::lookupInScope(const std::string& name, int scopeId) {
    if (scopeId < 0 || static_cast<size_t>(scopeId) >= scopes_.size()) {
        return nullptr;
    }
    const Scope& scope = scopes_[static_cast<size_t>(scopeId)];
    auto it = scope.symbolIndices.find(name);
    if (it != scope.symbolIndices.end()) {
        return &allSymbols_[it->second];
    }
    return nullptr;
}

const Symbol* SymbolTable::lookupInScope(const std::string& name, int scopeId) const {
    if (scopeId < 0 || static_cast<size_t>(scopeId) >= scopes_.size()) {
        return nullptr;
    }
    const Scope& scope = scopes_[static_cast<size_t>(scopeId)];
    auto it = scope.symbolIndices.find(name);
    if (it != scope.symbolIndices.end()) {
        return &allSymbols_[it->second];
    }
    return nullptr;
}

const std::vector<Symbol>& SymbolTable::getAllSymbols() const {
    return allSymbols_;
}

size_t SymbolTable::symbolCount() const {
    return allSymbols_.size();
}

void SymbolTable::printToTerminal() const {
    std::cout << "=== Symbol Table ===\n";
    if (allSymbols_.empty()) {
        std::cout << "(empty)\n";
        return;
    }

    std::cout << std::left
              << std::setw(16) << "Name"
              << std::setw(10) << "Type"
              << std::setw(14) << "Scope"
              << std::setw(8)  << "Depth"
              << std::setw(8)  << "Line"
              << std::setw(12) << "Initialized"
              << "Used\n";
    std::cout << std::string(74, '-') << "\n";

    for (const auto& sym : allSymbols_) {
        std::string scopeTag = sym.scopeLabel + "#" + std::to_string(sym.scopeId);
        std::cout << std::left
                  << std::setw(16) << sym.name
                  << std::setw(10) << dataTypeToString(sym.type)
                  << std::setw(14) << scopeTag
                  << std::setw(8)  << sym.scopeDepth
                  << std::setw(8)  << sym.lineDeclared
                  << std::setw(12) << (sym.isInitialized ? "yes" : "no")
                  << (sym.isUsed ? "yes" : "no") << "\n";
    }
}

std::string SymbolTable::toJson() const {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < allSymbols_.size(); ++i) {
        const auto& sym = allSymbols_[i];
        if (i > 0) {
            ss << ",";
        }
        ss << "{"
           << "\"name\":\"" << escapeJson(sym.name) << "\","
           << "\"type\":\"" << escapeJson(dataTypeToString(sym.type)) << "\","
           << "\"scopeId\":" << sym.scopeId << ","
           << "\"scopeDepth\":" << sym.scopeDepth << ","
           << "\"scope\":\"" << escapeJson(sym.scopeLabel) << "\","
           << "\"line\":" << sym.lineDeclared << ","
           << "\"initialized\":" << (sym.isInitialized ? "true" : "false") << ","
           << "\"used\":" << (sym.isUsed ? "true" : "false")
           << "}";
    }
    ss << "]";
    return ss.str();
}

} // namespace codeforge