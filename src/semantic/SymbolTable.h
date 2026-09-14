#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include "Types.h"

// Tracks declared variables and their types, with block-level scoping.
// Each { } block gets its own scope; lookups search outward through
// enclosing scopes (so a while-loop body can see outer variables).
class SymbolTable
{
public:
    SymbolTable()
    {
        enterScope(); // global scope, always present
    }

    void enterScope()
    {
        scopes.emplace_back();
    }

    void exitScope()
    {
        if (!scopes.empty())
            scopes.pop_back();
    }

    // Declares a new variable in the CURRENT (innermost) scope only.
    // Returns false if already declared in this exact scope (redeclaration error).
    bool declare(const std::string &name, ValueType type)
    {
        auto &current = scopes.back();
        if (current.find(name) != current.end())
        {
            return false; // already declared in this scope
        }
        current[name] = type;
        return true;
    }

    // Looks up a variable starting from the innermost scope outward.
    // Returns true and sets outType if found.
    bool lookup(const std::string &name, ValueType &outType) const
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end())
            {
                outType = found->second;
                return true;
            }
        }
        return false;
    }

private:
    std::vector<std::unordered_map<std::string, ValueType>> scopes;
};

#endif