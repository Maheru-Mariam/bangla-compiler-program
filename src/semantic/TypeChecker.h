#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "../parser/AST.h"
#include "SymbolTable.h"
#include "Types.h"
#include "../error/ErrorReporter.h"

class TypeChecker
{
public:
    explicit TypeChecker(ErrorReporter &reporter);

    // Entry point: type-checks the whole program
    void check(ASTNode *root);

private:
    SymbolTable symbols;
    ErrorReporter &errors;

    // --- statement visitors ---
    void checkStatement(ASTNode *node);
    void checkProgram(ProgramNode *node);
    void checkDecl(DeclNode *node);
    void checkAssign(AssignNode *node);
    void checkIf(IfNode *node);
    void checkWhile(WhileNode *node);
    void checkFor(ForNode *node);
    void checkPrint(PrintNode *node);
    void checkBlock(BlockNode *node);

    // --- expression type inference ---
    ValueType inferType(ASTNode *node);
    ValueType inferBinOp(BinOpNode *node);
    ValueType inferUnaryOp(UnaryOpNode *node);

    // same type, OR int value into decimal variable (widening ok, narrowing not)
    bool isAssignable(ValueType targetType, ValueType valueType) const;
};

#endif