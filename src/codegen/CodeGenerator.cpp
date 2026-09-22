#include "CodeGenerator.h"
#include <stdexcept>
#include "../utils/NumberFormat.h"

// Renders a double exactly enough that reading it back gives the same
// value, then makes sure it still looks like a float to Python.
// std::ostringstream's default of 6 significant digits would turn
// ৩.১৪১৫৯২৬৫ into 3.14159, and 4.0 into "4", which Python reads as an int.
std::string CodeGenerator::formatDecimal(double value)
{
    return formatDecimalLiteral(value);
}

// Wraps text in double quotes for Python, escaping the characters that
// would otherwise end the string or change its meaning. Bangla characters
// pass through untouched: Python 3 source is UTF-8.
std::string CodeGenerator::quoteForPython(const std::string &text)
{
    std::string out = "\"";
    for (char c : text)
    {
        switch (c)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\r':
            out += "\\r";
            break;
        default:
            out += c;
            break;
        }
    }
    out += "\"";
    return out;
}

// Emits an expression, inserting the int -> decimal widening the type
// checker allowed. Without this, দশমিকসংখ্যা ক = ৫; would emit "ক = 5",
// making ক an int in Python even though the language says it is a decimal.
std::string CodeGenerator::genExprAs(const ASTNode *value, ValueType targetType)
{
    std::string code = genExpr(value);

    if (targetType == ValueType::DECIMAL && value->inferredType == ValueType::INT)
    {
        // An integer literal can just be written as a decimal literal;
        // anything else gets an explicit conversion.
        if (auto lit = dynamic_cast<const IntLiteralNode *>(value))
        {
            return formatDecimal(static_cast<double>(lit->value));
        }
        return "float(" + code + ")";
    }
    return code;
}

std::string CodeGenerator::generate(const ASTNode *root)
{
    out.str("");
    indentLevel = 0;

    if (auto program = dynamic_cast<const ProgramNode *>(root))
    {
        genProgram(program);
    }

    return out.str();
}

void CodeGenerator::emitIndent()
{
    for (int i = 0; i < indentLevel; ++i)
    {
        out << "    "; // 4 spaces per level, Python convention
    }
}

void CodeGenerator::genProgram(const ProgramNode *node)
{
    for (const auto &stmt : node->statements)
    {
        genStatement(stmt.get());
    }
}

void CodeGenerator::genStatement(const ASTNode *node)
{
    if (auto n = dynamic_cast<const DeclNode *>(node))
    {
        genDecl(n);
        return;
    }
    if (auto n = dynamic_cast<const AssignNode *>(node))
    {
        genAssign(n);
        return;
    }
    if (auto n = dynamic_cast<const IfNode *>(node))
    {
        genIf(n);
        return;
    }
    if (auto n = dynamic_cast<const WhileNode *>(node))
    {
        genWhile(n);
        return;
    }
    if (auto n = dynamic_cast<const ForNode *>(node))
    {
        genFor(n);
        return;
    }
    if (auto n = dynamic_cast<const PrintNode *>(node))
    {
        genPrint(n);
        return;
    }
    if (auto n = dynamic_cast<const BlockNode *>(node))
    {
        genBlockBody(n);
        return;
    }
}

void CodeGenerator::genDecl(const DeclNode *node)
{
    emitIndent();
    // Python has no declared types, so this becomes a plain assignment.
    // Bangla identifiers are valid Python 3 identifiers, kept as-is.
    // node->inferredType holds the DECLARED type, set by the TypeChecker.
    out << node->name << " = "
        << genExprAs(node->value.get(), node->inferredType) << "\n";
}

void CodeGenerator::genAssign(const AssignNode *node)
{
    emitIndent();
    // node->inferredType holds the type the variable was declared with.
    out << node->name << " = "
        << genExprAs(node->value.get(), node->inferredType) << "\n";
}

void CodeGenerator::genIf(const IfNode *node)
{
    emitIndent();
    out << "if " << genExpr(node->condition.get()) << ":\n";
    indentLevel++;
    genStatement(node->thenBlock.get());
    indentLevel--;

    if (node->elseBlock)
    {
        emitIndent();
        out << "else:\n";
        indentLevel++;
        genStatement(node->elseBlock.get());
        indentLevel--;
    }
}

void CodeGenerator::genWhile(const WhileNode *node)
{
    emitIndent();
    out << "while " << genExpr(node->condition.get()) << ":\n";
    indentLevel++;
    genStatement(node->body.get());
    indentLevel--;
}

void CodeGenerator::genFor(const ForNode *node)
{
    // প্রতি (গণনা = ০ থেকে ৫)      ->  for গণনা in range(0, 5):
    // প্রতি (গণনা = ০ থেকে ১০ ধাপ ২) ->  for গণনা in range(0, 10, 2):
    emitIndent();
    out << "for " << node->varName << " in range("
        << genExpr(node->start.get()) << ", "
        << genExpr(node->end.get());
    if (node->step)
    {
        out << ", " << genExpr(node->step.get());
    }
    out << "):\n";

    indentLevel++;
    genStatement(node->body.get());
    indentLevel--;
}

void CodeGenerator::genPrint(const PrintNode *node)
{
    emitIndent();
    out << "print(" << genExpr(node->expression.get()) << ")\n";
}

void CodeGenerator::genBlockBody(const BlockNode *node)
{
    if (node->statements.empty())
    {
        emitIndent();
        out << "pass\n"; // Python requires SOMETHING inside a block
        return;
    }
    for (const auto &stmt : node->statements)
    {
        genStatement(stmt.get());
    }
}

std::string CodeGenerator::genExpr(const ASTNode *node)
{
    if (auto n = dynamic_cast<const IntLiteralNode *>(node))
    {
        return std::to_string(n->value);
    }
    if (auto n = dynamic_cast<const DecimalLiteralNode *>(node))
    {
        return formatDecimal(n->value);
    }
    if (auto n = dynamic_cast<const StringLiteralNode *>(node))
    {
        return quoteForPython(n->value);
    }
    if (auto n = dynamic_cast<const BoolLiteralNode *>(node))
    {
        return n->value ? "True" : "False"; // Python capitalizes booleans
    }
    if (auto n = dynamic_cast<const IdentifierNode *>(node))
    {
        return n->name;
    }
    if (auto n = dynamic_cast<const BinOpNode *>(node))
    {
        std::string op = n->op;
        if (op == "&&")
            op = "and";
        else if (op == "||")
            op = "or";
        return "(" + genExpr(n->left.get()) + " " + op + " " + genExpr(n->right.get()) + ")";
    }
    if (auto n = dynamic_cast<const UnaryOpNode *>(node))
    {
        std::string op = (n->op == "!") ? "not " : n->op;
        return "(" + op + genExpr(n->operand.get()) + ")";
    }

    throw std::runtime_error("Unknown expression node in code generation");
}