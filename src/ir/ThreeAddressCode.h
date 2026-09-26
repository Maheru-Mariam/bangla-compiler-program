#ifndef THREE_ADDRESS_CODE_H
#define THREE_ADDRESS_CODE_H

#include <string>
#include <vector>

// One three-address instruction.
//
// "Three-address" means an instruction refers to at most three operands:
// a destination and up to two sources. Anything more complicated than
// that — (২ + ৩) * ৪, for instance — is broken into several instructions
// joined by temporary variables (t1, t2, ...).
//
// Control flow becomes explicit here. There is no nesting left: an 'যদি'
// is a conditional jump over a run of instructions, and a loop is a jump
// backwards to a label.
enum class IROp
{
    Assign,      // result = arg1
    Binary,      // result = arg1 <op> arg2
    Unary,       // result = <op> arg1
    Label,       // label:
    Goto,        // goto label
    IfFalseGoto, // ifFalse arg1 goto label
    Print,       // print arg1
    Comment      // # note (not an instruction; explains the code around it)
};

struct Instruction
{
    IROp kind;
    std::string op;     // operator text, for Binary and Unary
    std::string result; // destination, or the label name for Label/Goto
    std::string arg1;
    std::string arg2;
    int line = 0; // source line this instruction came from

    Instruction(IROp k, const std::string &res = "", const std::string &a1 = "",
                const std::string &a2 = "", const std::string &o = "", int ln = 0)
        : kind(k), op(o), result(res), arg1(a1), arg2(a2), line(ln) {}
};

// Renders one instruction. Labels sit at the left margin and everything
// else is indented, which is the usual convention and makes the jump
// targets easy to pick out by eye.
inline std::string instructionToString(const Instruction &in)
{
    const std::string pad = "    ";
    switch (in.kind)
    {
    case IROp::Assign:
        return pad + in.result + " = " + in.arg1;
    case IROp::Binary:
        return pad + in.result + " = " + in.arg1 + " " + in.op + " " + in.arg2;
    case IROp::Unary:
        return pad + in.result + " = " + in.op + in.arg1;
    case IROp::Label:
        return in.result + ":";
    case IROp::Goto:
        return pad + "goto " + in.result;
    case IROp::IfFalseGoto:
        return pad + "ifFalse " + in.arg1 + " goto " + in.result;
    case IROp::Print:
        return pad + "print " + in.arg1;
    case IROp::Comment:
        return pad + "# " + in.arg1;
    }
    return pad + "?";
}

#endif