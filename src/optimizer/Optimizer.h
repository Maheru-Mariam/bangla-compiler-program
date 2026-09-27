#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../parser/AST.h"
#include "../error/ErrorReporter.h"

// Performs simple AST-level optimizations before code generation.
// Currently implements: constant folding (evaluating constant
// arithmetic expressions like ২+৩ at compile time instead of at runtime).
namespace Optimizer
{

    // Returns a new, optimized AST rooted at the given node.
    // Ownership of the input node is consumed (moved from).
    // Some checks (like a 'প্রতি' step folding down to zero) can only be
    // caught once constant folding has run, so the optimizer reports
    // those through the same ErrorReporter the earlier stages use.
    ASTNodePtr optimize(ASTNodePtr node, ErrorReporter &errors);

}

#endif