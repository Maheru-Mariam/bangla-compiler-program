#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../parser/AST.h"
#include <string>
#include <sstream>

// Walks a (type-checked, optimized) AST and emits equivalent Python 3 source code.
class CodeGenerator
{
public:
    // Generates and returns the full Python source as a string.
    std::string generate(const ASTNode *root);

private:
    std::ostringstream out;
    int indentLevel = 0;

    void emitIndent();
    void genStatement(const ASTNode *node);
    void genProgram(const ProgramNode *node);
    void genDecl(const DeclNode *node);
    void genAssign(const AssignNode *node);
    void genIf(const IfNode *node);
    void genWhile(const WhileNode *node);
    void genFor(const ForNode *node);
    void genPrint(const PrintNode *node);
    void genBlockBody(const BlockNode *node); // statements only, no braces (Python uses indent)

    std::string genExpr(const ASTNode *node);

    // Emits 'value', widened to a Python float if an int value is being
    // stored in a decimal variable (দশমিকসংখ্যা ক = ৫;  ->  ক = 5.0).
    std::string genExprAs(const ASTNode *value, ValueType targetType);

    // Renders text as a Python string literal, with escaping.
    static std::string quoteForPython(const std::string &text);

    // Renders a double as Python source: full precision, and always with
    // a decimal point so that 4.0 does not silently become the int 4.
    static std::string formatDecimal(double value);
};

#endif