#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../parser/AST.h"

// Performs simple AST-level optimizations before code generation.
// Currently implements: constant folding (evaluating constant
// arithmetic expressions like ২+৩ at compile time instead of at runtime).
namespace Optimizer
{

    // Returns a new, optimized AST rooted at the given node.
    // Ownership of the input node is consumed (moved from).
    ASTNodePtr optimize(ASTNodePtr node);

}

#endif