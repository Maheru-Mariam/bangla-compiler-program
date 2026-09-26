#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include "../parser/AST.h"
#include "ThreeAddressCode.h"
#include <string>
#include <vector>

// Lowers the optimized AST into three-address code.
//
// This is an inspection stage: it shows the program in a flat,
// machine-like form in which every expression is broken down into single
// operations and all control flow is explicit jumps. The Python backend
// reads the AST directly, so nothing downstream depends on this output —
// but it is the representation a backend targeting assembly, bytecode or
// WebAssembly would consume.
class IRGenerator
{
public:
    // Walks the tree and returns the instruction list.
    std::vector<Instruction> generate(const ASTNode *root);

    // Renders a whole instruction list as text, one per line.
    static std::string toText(const std::vector<Instruction> &code);

private:
    std::vector<Instruction> code;
    int tempCount = 0;
    int labelCount = 0;

    std::string newTemp();  // t1, t2, t3, ...
    std::string newLabel(); // L1, L2, L3, ...
    void emit(const Instruction &in);

    void genStatement(const ASTNode *node);
    void genProgram(const ProgramNode *node);
    void genBlock(const BlockNode *node);
    void genDecl(const DeclNode *node);
    void genAssign(const AssignNode *node);
    void genIf(const IfNode *node);
    void genWhile(const WhileNode *node);
    void genFor(const ForNode *node);
    void genPrint(const PrintNode *node);

    // Emits whatever instructions the expression needs and returns the
    // name holding its value — a temporary, a variable name, or a literal.
    std::string genExpr(const ASTNode *node);

    // Applies the int -> decimal widening the type checker permitted,
    // so the conversion is visible rather than implied.
    std::string widenIfNeeded(const ASTNode *value, ValueType target,
                              const std::string &name, int line);
};

#endif