#include "ASTPrinter.h"
#include "NumberFormat.h"
#include <iostream>

namespace ASTPrinter
{

    // Helper: print this node's own label (without recursing into children)
    static std::string labelFor(const ASTNode *node)
    {
        if (auto n = dynamic_cast<const ProgramNode *>(node))
            return "Program";
        if (auto n = dynamic_cast<const DeclNode *>(node))
            return "Decl (" + n->varType + " " + n->name + ")";
        if (auto n = dynamic_cast<const AssignNode *>(node))
            return "Assign (" + n->name + ")";
        if (auto n = dynamic_cast<const IfNode *>(node))
            return "If";
        if (auto n = dynamic_cast<const WhileNode *>(node))
            return "While";
        if (auto n = dynamic_cast<const ForNode *>(node))
            return "For (" + n->varName + ")";
        if (auto n = dynamic_cast<const PrintNode *>(node))
            return "Print";
        if (auto n = dynamic_cast<const BlockNode *>(node))
            return "Block";
        if (auto n = dynamic_cast<const BinOpNode *>(node))
            return "BinOp (" + n->op + ")";
        if (auto n = dynamic_cast<const UnaryOpNode *>(node))
            return "UnaryOp (" + n->op + ")";
        if (auto n = dynamic_cast<const IntLiteralNode *>(node))
            return "IntLiteral (" + std::to_string(n->value) + ")";
        if (auto n = dynamic_cast<const DecimalLiteralNode *>(node))
            return "DecimalLiteral (" + formatDecimalLiteral(n->value) + ")";
        if (auto n = dynamic_cast<const StringLiteralNode *>(node))
            return "StringLiteral (\"" + n->value + "\")";
        if (auto n = dynamic_cast<const BoolLiteralNode *>(node))
            return std::string("BoolLiteral (") + (n->value ? "true" : "false") + ")";
        if (auto n = dynamic_cast<const IdentifierNode *>(node))
            return "Identifier (" + n->name + ")";
        return "UnknownNode";
    }

    // Helper: gather this node's direct children as raw pointers, in order
    static std::vector<const ASTNode *> childrenOf(const ASTNode *node)
    {
        std::vector<const ASTNode *> children;

        if (auto n = dynamic_cast<const ProgramNode *>(node))
        {
            for (auto &s : n->statements)
                children.push_back(s.get());
        }
        else if (auto n = dynamic_cast<const DeclNode *>(node))
        {
            children.push_back(n->value.get());
        }
        else if (auto n = dynamic_cast<const AssignNode *>(node))
        {
            children.push_back(n->value.get());
        }
        else if (auto n = dynamic_cast<const IfNode *>(node))
        {
            children.push_back(n->condition.get());
            children.push_back(n->thenBlock.get());
            if (n->elseBlock)
                children.push_back(n->elseBlock.get());
        }
        else if (auto n = dynamic_cast<const WhileNode *>(node))
        {
            children.push_back(n->condition.get());
            children.push_back(n->body.get());
        }
        else if (auto n = dynamic_cast<const ForNode *>(node))
        {
            children.push_back(n->start.get());
            children.push_back(n->end.get());
            if (n->step)
                children.push_back(n->step.get());
            children.push_back(n->body.get());
        }
        else if (auto n = dynamic_cast<const PrintNode *>(node))
        {
            children.push_back(n->expression.get());
        }
        else if (auto n = dynamic_cast<const BlockNode *>(node))
        {
            for (auto &s : n->statements)
                children.push_back(s.get());
        }
        else if (auto n = dynamic_cast<const BinOpNode *>(node))
        {
            children.push_back(n->left.get());
            children.push_back(n->right.get());
        }
        else if (auto n = dynamic_cast<const UnaryOpNode *>(node))
        {
            children.push_back(n->operand.get());
        }
        // literals/identifiers have no children

        return children;
    }

    void print(const ASTNode *node, const std::string &prefix, bool isLast)
    {
        if (!node)
            return;

        std::cout << prefix << (isLast ? "└── " : "├── ") << labelFor(node) << "\n";

        std::string childPrefix = prefix + (isLast ? "    " : "│   ");
        std::vector<const ASTNode *> children = childrenOf(node);

        for (size_t i = 0; i < children.size(); ++i)
        {
            bool lastChild = (i == children.size() - 1);
            print(children[i], childPrefix, lastChild);
        }
    }

}