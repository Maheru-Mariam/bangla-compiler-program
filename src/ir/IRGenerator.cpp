#include "IRGenerator.h"
#include "../utils/NumberFormat.h"
#include <sstream>

std::string IRGenerator::newTemp()
{
    return "t" + std::to_string(++tempCount);
}

std::string IRGenerator::newLabel()
{
    return "L" + std::to_string(++labelCount);
}

void IRGenerator::emit(const Instruction &in)
{
    code.push_back(in);
}

std::vector<Instruction> IRGenerator::generate(const ASTNode *root)
{
    code.clear();
    tempCount = 0;
    labelCount = 0;

    if (auto program = dynamic_cast<const ProgramNode *>(root))
    {
        genProgram(program);
    }
    return code;
}

std::string IRGenerator::toText(const std::vector<Instruction> &instructions)
{
    std::ostringstream out;
    for (const Instruction &in : instructions)
    {
        out << instructionToString(in) << "\n";
    }
    return out.str();
}

// ---------------- statements ----------------

void IRGenerator::genProgram(const ProgramNode *node)
{
    for (const auto &stmt : node->statements)
    {
        genStatement(stmt.get());
    }
}

void IRGenerator::genStatement(const ASTNode *node)
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
        genBlock(n);
        return;
    }
}

void IRGenerator::genBlock(const BlockNode *node)
{
    for (const auto &stmt : node->statements)
    {
        genStatement(stmt.get());
    }
}

// Makes the int -> decimal widening visible. The type checker allows an
// integer value to be stored in a দশমিকসংখ্যা variable, and the Python
// backend emits 5.0 rather than 5; the IR shows the same conversion as an
// explicit cast instead of letting it happen silently.
std::string IRGenerator::widenIfNeeded(const ASTNode *value, ValueType target,
                                       const std::string &name, int line)
{
    if (target != ValueType::DECIMAL || value->inferredType != ValueType::INT)
    {
        return name;
    }
    if (auto lit = dynamic_cast<const IntLiteralNode *>(value))
    {
        return formatDecimalLiteral(static_cast<double>(lit->value));
    }
    std::string temp = newTemp();
    emit(Instruction(IROp::Unary, temp, name, "", "(দশমিকসংখ্যা)", line));
    return temp;
}

void IRGenerator::genDecl(const DeclNode *node)
{
    // A declaration and an assignment are the same instruction here;
    // storage is not a concept at this level.
    std::string value = genExpr(node->value.get());
    value = widenIfNeeded(node->value.get(), node->inferredType, value, node->line);
    emit(Instruction(IROp::Assign, node->name, value, "", "", node->line));
}

void IRGenerator::genAssign(const AssignNode *node)
{
    std::string value = genExpr(node->value.get());
    value = widenIfNeeded(node->value.get(), node->inferredType, value, node->line);
    emit(Instruction(IROp::Assign, node->name, value, "", "", node->line));
}

// যদি (cond) { A } নাহয় { B }
//
//     t = cond
//     ifFalse t goto Lelse
//     A
//     goto Lend
// Lelse:
//     B
// Lend:
void IRGenerator::genIf(const IfNode *node)
{
    std::string cond = genExpr(node->condition.get());

    if (node->elseBlock)
    {
        std::string elseLabel = newLabel();
        std::string endLabel = newLabel();

        emit(Instruction(IROp::IfFalseGoto, elseLabel, cond, "", "", node->line));
        genStatement(node->thenBlock.get());
        emit(Instruction(IROp::Goto, endLabel, "", "", "", node->line));
        emit(Instruction(IROp::Label, elseLabel, "", "", "", node->line));
        genStatement(node->elseBlock.get());
        emit(Instruction(IROp::Label, endLabel, "", "", "", node->line));
    }
    else
    {
        std::string endLabel = newLabel();
        emit(Instruction(IROp::IfFalseGoto, endLabel, cond, "", "", node->line));
        genStatement(node->thenBlock.get());
        emit(Instruction(IROp::Label, endLabel, "", "", "", node->line));
    }
}

// যতক্ষণ (cond) { body }
//
// Lstart:
//     t = cond
//     ifFalse t goto Lend
//     body
//     goto Lstart
// Lend:
//
// The condition is re-evaluated on every pass, which is why it sits
// after the label rather than before it.
void IRGenerator::genWhile(const WhileNode *node)
{
    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    emit(Instruction(IROp::Label, startLabel, "", "", "", node->line));
    std::string cond = genExpr(node->condition.get());
    emit(Instruction(IROp::IfFalseGoto, endLabel, cond, "", "", node->line));
    genStatement(node->body.get());
    emit(Instruction(IROp::Goto, startLabel, "", "", "", node->line));
    emit(Instruction(IROp::Label, endLabel, "", "", "", node->line));
}

// প্রতি (i = start থেকে end ধাপ step) { body }
//
//     i = start
//     tEnd = end
//     tStep = step
// Lstart:
//     tCond = i < tEnd        (or i > tEnd for a negative step)
//     ifFalse tCond goto Lend
//     body
//     i = i + tStep
//     goto Lstart
// Lend:
//
// The bounds are evaluated once, before the loop, which is what the
// language promises: changing them inside the body has no effect.
void IRGenerator::genFor(const ForNode *node)
{
    std::string start = genExpr(node->start.get());
    emit(Instruction(IROp::Assign, node->varName, start, "", "", node->line));

    std::string endTemp = newTemp();
    std::string endValue = genExpr(node->end.get());
    emit(Instruction(IROp::Assign, endTemp, endValue, "", "", node->line));

    std::string stepTemp = newTemp();
    std::string stepValue = node->step ? genExpr(node->step.get()) : "1";
    emit(Instruction(IROp::Assign, stepTemp, stepValue, "", "", node->line));

    // A countdown has to test the other way round. After optimization the
    // step is usually a literal, so its sign is known here.
    std::string compare = "<";
    if (node->step)
    {
        if (auto lit = dynamic_cast<const IntLiteralNode *>(node->step.get()))
        {
            if (lit->value < 0)
            {
                compare = ">";
            }
        }
        else
        {
            emit(Instruction(IROp::Comment, "", "step is not a constant; the test below assumes it is positive",
                             "", "", node->line));
        }
    }

    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    emit(Instruction(IROp::Label, startLabel, "", "", "", node->line));
    std::string condTemp = newTemp();
    emit(Instruction(IROp::Binary, condTemp, node->varName, endTemp, compare, node->line));
    emit(Instruction(IROp::IfFalseGoto, endLabel, condTemp, "", "", node->line));

    genStatement(node->body.get());

    std::string nextTemp = newTemp();
    emit(Instruction(IROp::Binary, nextTemp, node->varName, stepTemp, "+", node->line));
    emit(Instruction(IROp::Assign, node->varName, nextTemp, "", "", node->line));
    emit(Instruction(IROp::Goto, startLabel, "", "", "", node->line));
    emit(Instruction(IROp::Label, endLabel, "", "", "", node->line));
}

void IRGenerator::genPrint(const PrintNode *node)
{
    std::string value = genExpr(node->expression.get());
    emit(Instruction(IROp::Print, "", value, "", "", node->line));
}

// ---------------- expressions ----------------

// Returns the name that holds the expression's value. Literals and
// variables are their own names; anything computed lands in a temporary.
std::string IRGenerator::genExpr(const ASTNode *node)
{
    if (auto n = dynamic_cast<const IntLiteralNode *>(node))
    {
        return std::to_string(n->value);
    }
    if (auto n = dynamic_cast<const DecimalLiteralNode *>(node))
    {
        return formatDecimalLiteral(n->value);
    }
    if (auto n = dynamic_cast<const StringLiteralNode *>(node))
    {
        return "\"" + n->value + "\"";
    }
    if (auto n = dynamic_cast<const BoolLiteralNode *>(node))
    {
        return n->value ? "সত্যি" : "মিথ্যা";
    }
    if (auto n = dynamic_cast<const IdentifierNode *>(node))
    {
        return n->name;
    }

    if (auto n = dynamic_cast<const BinOpNode *>(node))
    {
        // Depth first: both operands must be in temporaries (or be simple
        // names) before the operation itself can be written down. This is
        // what flattens a nested expression into a sequence.
        std::string left = genExpr(n->left.get());
        std::string right = genExpr(n->right.get());
        std::string temp = newTemp();
        emit(Instruction(IROp::Binary, temp, left, right, n->op, n->line));
        return temp;
    }

    if (auto n = dynamic_cast<const UnaryOpNode *>(node))
    {
        std::string operand = genExpr(n->operand.get());
        std::string temp = newTemp();
        emit(Instruction(IROp::Unary, temp, operand, "", n->op, n->line));
        return temp;
    }

    return "?";
}