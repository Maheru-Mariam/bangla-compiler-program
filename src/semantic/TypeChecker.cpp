#include "TypeChecker.h"
#include <iostream>

TypeChecker::TypeChecker(ErrorReporter &reporter)
    : errors(reporter) {}

void TypeChecker::check(const ASTNode *root)
{
    if (auto program = dynamic_cast<const ProgramNode *>(root))
    {
        checkProgram(program);
    }
}

// ---------------- statements ----------------

void TypeChecker::checkProgram(const ProgramNode *node)
{
    for (const auto &stmt : node->statements)
    {
        checkStatement(stmt.get());
    }
}

void TypeChecker::checkStatement(const ASTNode *node)
{
    if (auto n = dynamic_cast<const DeclNode *>(node))
    {
        checkDecl(n);
        return;
    }
    if (auto n = dynamic_cast<const AssignNode *>(node))
    {
        checkAssign(n);
        return;
    }
    if (auto n = dynamic_cast<const IfNode *>(node))
    {
        checkIf(n);
        return;
    }
    if (auto n = dynamic_cast<const WhileNode *>(node))
    {
        checkWhile(n);
        return;
    }
    if (auto n = dynamic_cast<const PrintNode *>(node))
    {
        checkPrint(n);
        return;
    }
    if (auto n = dynamic_cast<const BlockNode *>(node))
    {
        checkBlock(n);
        return;
    }
}

void TypeChecker::checkDecl(const DeclNode *node)
{
    ValueType declaredType = typeFromKeyword(node->varType);
    ValueType valueType = inferType(node->value.get());

    if (!symbols.declare(node->name, declaredType))
    {
        errors.report(node->line, "Variable '" + node->name + "' is already declared in this scope");
        return;
    }

    if (!isAssignable(declaredType, valueType))
    {
        errors.report(node->line,
                      "Cannot assign value of type " + valueTypeName(valueType) +
                          " to variable '" + node->name + "' of type " + valueTypeName(declaredType));
    }
}

void TypeChecker::checkAssign(const AssignNode *node)
{
    ValueType existingType;
    if (!symbols.lookup(node->name, existingType))
    {
        errors.report(node->line, "Variable '" + node->name + "' was not declared before use");
        return;
    }

    ValueType valueType = inferType(node->value.get());
    if (!isAssignable(existingType, valueType))
    {
        errors.report(node->line,
                      "Cannot assign value of type " + valueTypeName(valueType) +
                          " to variable '" + node->name + "' of type " + valueTypeName(existingType));
    }
}

void TypeChecker::checkIf(const IfNode *node)
{
    ValueType condType = inferType(node->condition.get());
    if (condType != ValueType::BOOL)
    {
        errors.report(node->line,
                      "Condition in 'যদি' must be a boolean expression, got " + valueTypeName(condType));
    }

    symbols.enterScope();
    checkStatement(node->thenBlock.get());
    symbols.exitScope();

    if (node->elseBlock)
    {
        symbols.enterScope();
        checkStatement(node->elseBlock.get());
        symbols.exitScope();
    }
}

void TypeChecker::checkWhile(const WhileNode *node)
{
    ValueType condType = inferType(node->condition.get());
    if (condType != ValueType::BOOL)
    {
        errors.report(node->line,
                      "Condition in 'যতক্ষণ' must be a boolean expression, got " + valueTypeName(condType));
    }

    symbols.enterScope();
    checkStatement(node->body.get());
    symbols.exitScope();
}

void TypeChecker::checkPrint(const PrintNode *node)
{
    // Any type is printable — we just need to make sure the expression
    // itself is well-typed (inferType will report errors internally).
    inferType(node->expression.get());
}

void TypeChecker::checkBlock(const BlockNode *node)
{
    for (const auto &stmt : node->statements)
    {
        checkStatement(stmt.get());
    }
}

// ---------------- expression type inference ----------------

ValueType TypeChecker::inferType(const ASTNode *node)
{
    if (dynamic_cast<const IntLiteralNode *>(node))
        return ValueType::INT;
    if (dynamic_cast<const DecimalLiteralNode *>(node))
        return ValueType::DECIMAL;
    if (dynamic_cast<const BoolLiteralNode *>(node))
        return ValueType::BOOL;

    if (auto n = dynamic_cast<const IdentifierNode *>(node))
    {
        ValueType type;
        if (symbols.lookup(n->name, type))
        {
            return type;
        }
        errors.report(n->line, "Variable '" + n->name + "' was not declared before use");
        return ValueType::UNKNOWN;
    }

    if (auto n = dynamic_cast<const BinOpNode *>(node))
    {
        return inferBinOp(n);
    }

    if (auto n = dynamic_cast<const UnaryOpNode *>(node))
    {
        return inferUnaryOp(n);
    }

    return ValueType::UNKNOWN;
}

ValueType TypeChecker::inferBinOp(const BinOpNode *node)
{
    ValueType left = inferType(node->left.get());
    ValueType right = inferType(node->right.get());
    const std::string &op = node->op;

    bool leftNumeric = (left == ValueType::INT || left == ValueType::DECIMAL);
    bool rightNumeric = (right == ValueType::INT || right == ValueType::DECIMAL);

    // Arithmetic: + - * /
    if (op == "+" || op == "-" || op == "*" || op == "/")
    {
        if (!leftNumeric || !rightNumeric)
        {
            errors.report(node->line,
                          "Arithmetic operator '" + op + "' requires numeric operands, got " +
                              valueTypeName(left) + " and " + valueTypeName(right));
            return ValueType::UNKNOWN;
        }
        // if either side is decimal, result is decimal (widening)
        return (left == ValueType::DECIMAL || right == ValueType::DECIMAL)
                   ? ValueType::DECIMAL
                   : ValueType::INT;
    }

    // Relational: < > <= >=
    if (op == "<" || op == ">" || op == "<=" || op == ">=")
    {
        if (!leftNumeric || !rightNumeric)
        {
            errors.report(node->line,
                          "Comparison operator '" + op + "' requires numeric operands, got " +
                              valueTypeName(left) + " and " + valueTypeName(right));
        }
        return ValueType::BOOL;
    }

    // Equality: == !=
    if (op == "==" || op == "!=")
    {
        if (left != right && !(leftNumeric && rightNumeric))
        {
            errors.report(node->line,
                          "Cannot compare " + valueTypeName(left) + " with " + valueTypeName(right));
        }
        return ValueType::BOOL;
    }

    // Logical: && ||
    if (op == "&&" || op == "||")
    {
        if (left != ValueType::BOOL || right != ValueType::BOOL)
        {
            errors.report(node->line,
                          "Logical operator '" + op + "' requires boolean operands, got " +
                              valueTypeName(left) + " and " + valueTypeName(right));
        }
        return ValueType::BOOL;
    }

    return ValueType::UNKNOWN;
}

ValueType TypeChecker::inferUnaryOp(const UnaryOpNode *node)
{
    ValueType operandType = inferType(node->operand.get());

    if (node->op == "-")
    {
        if (operandType != ValueType::INT && operandType != ValueType::DECIMAL)
        {
            errors.report(node->line,
                          "Unary '-' requires a numeric operand, got " + valueTypeName(operandType));
            return ValueType::UNKNOWN;
        }
        return operandType;
    }

    if (node->op == "!")
    {
        if (operandType != ValueType::BOOL)
        {
            errors.report(node->line,
                          "Unary '!' requires a boolean operand, got " + valueTypeName(operandType));
            return ValueType::UNKNOWN;
        }
        return ValueType::BOOL;
    }

    return ValueType::UNKNOWN;
}

// ---------------- assignability rule ----------------

bool TypeChecker::isAssignable(ValueType targetType, ValueType valueType) const
{
    if (targetType == valueType)
        return true;
    // allow widening: int value into a decimal variable
    if (targetType == ValueType::DECIMAL && valueType == ValueType::INT)
        return true;
    return false;
}