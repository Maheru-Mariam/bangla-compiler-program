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
    void check(const ASTNode *root);

private:
    SymbolTable symbols;
    ErrorReporter &errors;

    // --- statement visitors ---
    void checkStatement(const ASTNode *node);
    void checkProgram(const ProgramNode *node);
    void checkDecl(const DeclNode *node);
    void checkAssign(const AssignNode *node);
    void checkIf(const IfNode *node);
    void checkWhile(const WhileNode *node);
    void checkPrint(const PrintNode *node);
    void checkBlock(const BlockNode *node);

    // --- expression type inference ---
    ValueType inferType(const ASTNode *node);
    ValueType inferBinOp(const BinOpNode *node);
    ValueType inferUnaryOp(const UnaryOpNode *node);

    // Reasonable-assignment check: same type, OR int value into decimal
    // variable (widening is fine; narrowing decimal->int is NOT allowed)
    bool isAssignable(ValueType targetType, ValueType valueType) const;
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
        void check(const ASTNode *root);

    private:
        SymbolTable symbols;
        ErrorReporter &errors;

        // --- statement visitors ---
        void checkStatement(const ASTNode *node);
        void checkProgram(const ProgramNode *node);
        void checkDecl(const DeclNode *node);
        void checkAssign(const AssignNode *node);
        void checkIf(const IfNode *node);
        void checkWhile(const WhileNode *node);
        void checkPrint(const PrintNode *node);
        void checkBlock(const BlockNode *node);

        // --- expression type inference ---
        ValueType inferType(const ASTNode *node);
        ValueType inferBinOp(const BinOpNode *node);
        ValueType inferUnaryOp(const UnaryOpNode *node);

        // Reasonable-assignment check: same type, OR int value into decimal
        // variable (widening is fine; narrowing decimal->int is NOT allowed)
        bool isAssignable(ValueType targetType, ValueType valueType) const;
    };

#endif

    int currentLineHint; // fallback line number for nodes with no line info
};

#endif