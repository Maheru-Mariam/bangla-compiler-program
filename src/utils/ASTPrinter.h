#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "../parser/AST.h"
#include <string>

namespace ASTPrinter
{

    // Prints the whole tree starting from the root node, indentation-based,
    // similar to the tree() pretty-printer you've used in Python.
    void print(const ASTNode *node, const std::string &prefix = "", bool isLast = true);

}

#endif