#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "../semantic/Types.h"

// Base class for every node in the tree.
// Every specific node type (IfNode, BinOpNode, etc.) inherits from this.
class ASTNode
{
public:
    int line = 0; // source line this node came from, for error messages

    // Filled in by the TypeChecker and carried through the Optimizer, so
    // the CodeGenerator knows what kind of value each node produces.
    // For expressions this is the type of the value. For DeclNode and
    // AssignNode it is the declared type of the variable being written to.
    // Stays UNKNOWN until type checking runs.
    ValueType inferredType = ValueType::UNKNOWN;

    virtual ~ASTNode() = default;
};

// Convenience alias: a smart pointer that automatically frees AST nodes
// when no longer needed, so we don't have to manually delete anything.
using ASTNodePtr = std::unique_ptr<ASTNode>;

// ---------------- Expressions ----------------

// A literal integer value, e.g. ৫
class IntLiteralNode : public ASTNode
{
public:
    int value;
    explicit IntLiteralNode(int v) : value(v) {}
};

// A literal decimal value, e.g. ৩.১৪
class DecimalLiteralNode : public ASTNode
{
public:
    double value;
    explicit DecimalLiteralNode(double v) : value(v) {}
};

// A literal text value, e.g. "হ্যালো". 'value' holds the decoded text
// (escape sequences already applied by the lexer).
class StringLiteralNode : public ASTNode
{
public:
    std::string value;
    explicit StringLiteralNode(const std::string &v) : value(v) {}
};

// A boolean literal: সত্যি or মিথ্যা
class BoolLiteralNode : public ASTNode
{
public:
    bool value;
    explicit BoolLiteralNode(bool v) : value(v) {}
};

// A variable reference, e.g. গণনা
class IdentifierNode : public ASTNode
{
public:
    std::string name;
    explicit IdentifierNode(const std::string &n) : name(n) {}
};

// A binary operation, e.g. গণনা + ১   or   গণনা < সীমা
class BinOpNode : public ASTNode
{
public:
    std::string op; // "+", "-", "*", "/", "<", "==", "&&", etc.
    ASTNodePtr left;
    ASTNodePtr right;

    BinOpNode(const std::string &o, ASTNodePtr l, ASTNodePtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

// A unary operation, e.g. -৫ or !মিথ্যা
class UnaryOpNode : public ASTNode
{
public:
    std::string op; // "-" or "!"
    ASTNodePtr operand;

    UnaryOpNode(const std::string &o, ASTNodePtr operand_)
        : op(o), operand(std::move(operand_)) {}
};

// ---------------- Statements ----------------

// A variable declaration: type identifier = expression;
class DeclNode : public ASTNode
{
public:
    std::string varType; // "পূর্ণসংখ্যা" or "দশমিকসংখ্যা"
    std::string name;
    ASTNodePtr value;

    DeclNode(const std::string &t, const std::string &n, ASTNodePtr v)
        : varType(t), name(n), value(std::move(v)) {}
};

// An assignment: identifier = expression;
class AssignNode : public ASTNode
{
public:
    std::string name;
    ASTNodePtr value;

    AssignNode(const std::string &n, ASTNodePtr v)
        : name(n), value(std::move(v)) {}
};

// A block: { statement* }
class BlockNode : public ASTNode
{
public:
    std::vector<ASTNodePtr> statements;
};

// If-else: যদি (cond) block [নাহয় block]
class IfNode : public ASTNode
{
public:
    ASTNodePtr condition;
    ASTNodePtr thenBlock;
    ASTNodePtr elseBlock; // nullptr if no else branch

    IfNode(ASTNodePtr cond, ASTNodePtr thenB, ASTNodePtr elseB)
        : condition(std::move(cond)), thenBlock(std::move(thenB)), elseBlock(std::move(elseB)) {}
};

// While loop: যতক্ষণ (cond) block
class WhileNode : public ASTNode
{
public:
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileNode(ASTNodePtr cond, ASTNodePtr b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

// Range loop: প্রতি (নাম = start থেকে end [ধাপ step]) block
//
// The loop variable is implicitly পূর্ণসংখ্যা and lives only inside the
// loop. 'end' is exclusive, matching Python's range().
class ForNode : public ASTNode
{
public:
    std::string varName;
    ASTNodePtr start;
    ASTNodePtr end;
    ASTNodePtr step; // nullptr when no ধাপ clause was given (step of 1)
    ASTNodePtr body;

    ForNode(const std::string &name, ASTNodePtr from, ASTNodePtr to,
            ASTNodePtr by, ASTNodePtr b)
        : varName(name), start(std::move(from)), end(std::move(to)),
          step(std::move(by)), body(std::move(b)) {}
};

// Print statement: দেখাও(expression);
class PrintNode : public ASTNode
{
public:
    ASTNodePtr expression;
    explicit PrintNode(ASTNodePtr e) : expression(std::move(e)) {}
};

// The whole program: a sequence of top-level statements
class ProgramNode : public ASTNode
{
public:
    std::vector<ASTNodePtr> statements;
};

#endif