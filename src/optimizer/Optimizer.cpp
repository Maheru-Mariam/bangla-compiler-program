#include "Optimizer.h"
#include <memory>

namespace Optimizer
{

    // Helper: copies the source line and the type the TypeChecker worked
    // out onto a replacement node. Every rebuilt node must carry these
    // across, or the code generator loses the type information.
    static ASTNodePtr carryOver(ASTNodePtr node, int line, ValueType type)
    {
        node->line = line;
        node->inferredType = type;
        return node;
    }

    // Tries to constant-fold a BinOpNode whose children are already
    // optimized. If both sides are numeric literals, computes the result
    // at compile time and returns a literal node instead. Otherwise
    // returns nullptr to signal "couldn't fold, keep as BinOp".
    static ASTNodePtr tryFoldBinOp(BinOpNode *bin)
    {
        // The TypeChecker already decided what this expression produces;
        // the folded literal must agree with it.
        const bool wantsDecimal = (bin->inferredType == ValueType::DECIMAL);

        auto *leftInt = dynamic_cast<IntLiteralNode *>(bin->left.get());
        auto *rightInt = dynamic_cast<IntLiteralNode *>(bin->right.get());
        auto *leftDec = dynamic_cast<DecimalLiteralNode *>(bin->left.get());
        auto *rightDec = dynamic_cast<DecimalLiteralNode *>(bin->right.get());

        bool leftIsNum = leftInt || leftDec;
        bool rightIsNum = rightInt || rightDec;
        if (!leftIsNum || !rightIsNum)
            return nullptr;

        double lv = leftInt ? leftInt->value : leftDec->value;
        double rv = rightInt ? rightInt->value : rightDec->value;
        // '/' always yields a decimal, so consult the checked type rather
        // than just looking at the operands.
        bool resultIsDecimal = wantsDecimal || leftDec || rightDec;

        const std::string &op = bin->op;

        // Only fold arithmetic operators; leave comparisons/logic to codegen
        if (op == "+" || op == "-" || op == "*" || op == "/")
        {
            if (op == "/" && rv == 0)
                return nullptr; // don't fold div-by-zero, let it fail at runtime

            double result =
                (op == "+") ? lv + rv : (op == "-") ? lv - rv
                                    : (op == "*")   ? lv * rv
                                                    : lv / rv;

            if (resultIsDecimal)
            {
                return std::make_unique<DecimalLiteralNode>(result);
            }
            else
            {
                return std::make_unique<IntLiteralNode>(static_cast<int>(result));
            }
            // note: '/' never reaches the int branch, since division is
            // always decimal — so no truncate-vs-floor mismatch is possible.
        }

        return nullptr; // not an arithmetic op, don't fold
    }

    ASTNodePtr optimize(ASTNodePtr node)
    {
        if (!node)
            return nullptr;
        int line = node->line;
        ValueType type = node->inferredType;

        if (auto n = dynamic_cast<ProgramNode *>(node.get()))
        {
            auto result = std::make_unique<ProgramNode>();
            result->line = line;
            result->inferredType = type;
            for (auto &stmt : n->statements)
            {
                result->statements.push_back(optimize(std::move(stmt)));
            }
            return result;
        }

        if (auto n = dynamic_cast<BlockNode *>(node.get()))
        {
            auto result = std::make_unique<BlockNode>();
            result->line = line;
            result->inferredType = type;
            for (auto &stmt : n->statements)
            {
                result->statements.push_back(optimize(std::move(stmt)));
            }
            return result;
        }

        if (auto n = dynamic_cast<DeclNode *>(node.get()))
        {
            auto value = optimize(std::move(n->value));
            auto result = std::make_unique<DeclNode>(n->varType, n->name, std::move(value));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<AssignNode *>(node.get()))
        {
            auto value = optimize(std::move(n->value));
            auto result = std::make_unique<AssignNode>(n->name, std::move(value));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<IfNode *>(node.get()))
        {
            auto cond = optimize(std::move(n->condition));
            auto thenB = optimize(std::move(n->thenBlock));
            auto elseB = n->elseBlock ? optimize(std::move(n->elseBlock)) : nullptr;
            auto result = std::make_unique<IfNode>(std::move(cond), std::move(thenB), std::move(elseB));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<WhileNode *>(node.get()))
        {
            auto cond = optimize(std::move(n->condition));
            auto body = optimize(std::move(n->body));
            auto result = std::make_unique<WhileNode>(std::move(cond), std::move(body));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<ForNode *>(node.get()))
        {
            auto from = optimize(std::move(n->start));
            auto to = optimize(std::move(n->end));
            auto by = n->step ? optimize(std::move(n->step)) : nullptr;
            auto body = optimize(std::move(n->body));
            auto result = std::make_unique<ForNode>(n->varName, std::move(from), std::move(to),
                                                    std::move(by), std::move(body));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<PrintNode *>(node.get()))
        {
            auto expr = optimize(std::move(n->expression));
            auto result = std::make_unique<PrintNode>(std::move(expr));
            result->line = line;
            result->inferredType = type;
            return result;
        }

        if (auto n = dynamic_cast<BinOpNode *>(node.get()))
        {
            n->left = optimize(std::move(n->left));
            n->right = optimize(std::move(n->right));

            if (auto folded = tryFoldBinOp(n))
            {
                return carryOver(std::move(folded), line, type);
            }
            return node; // couldn't fold, keep the BinOp with optimized children
        }

        if (auto n = dynamic_cast<UnaryOpNode *>(node.get()))
        {
            n->operand = optimize(std::move(n->operand));

            // fold unary minus on a literal, e.g. -৫ -> IntLiteral(-5)
            if (n->op == "-")
            {
                if (auto lit = dynamic_cast<IntLiteralNode *>(n->operand.get()))
                {
                    return carryOver(std::make_unique<IntLiteralNode>(-lit->value), line, type);
                }
                if (auto lit = dynamic_cast<DecimalLiteralNode *>(n->operand.get()))
                {
                    return carryOver(std::make_unique<DecimalLiteralNode>(-lit->value), line, type);
                }
            }
            return node;
        }

        // Literals and identifiers: nothing to optimize, return as-is
        return node;
    }

}