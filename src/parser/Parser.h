#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include "../lexer/Token.h"
#include "AST.h"

class Parser
{
public:
    explicit Parser(const std::vector<Token> &tokens);

    // Entry point: parses the whole token stream into a ProgramNode
    ASTNodePtr parseProgram();

private:
    std::vector<Token> tokens;
    size_t pos;

    // --- cursor helpers ---
    const Token &peek() const;     // current token, without consuming
    const Token &previous() const; // most recently consumed token
    bool isAtEnd() const;
    bool check(TokenType type) const; // is current token of this type?
    const Token &advance();           // consume current token, return it
    bool match(TokenType type);       // if current matches, consume and return true
    const Token &expect(TokenType type, const std::string &errorMessage);

    // --- grammar rule functions (one per non-terminal) ---
    ASTNodePtr parseStatement();
    ASTNodePtr parseDeclStmt();
    ASTNodePtr parseAssignStmt();
    ASTNodePtr parseIfStmt();
    ASTNodePtr parseWhileStmt();
    ASTNodePtr parsePrintStmt();
    ASTNodePtr parseBlock();

    ASTNodePtr parseExpression();
    ASTNodePtr parseLogicalOr();
    ASTNodePtr parseLogicalAnd();
    ASTNodePtr parseEquality();
    ASTNodePtr parseRelational();
    ASTNodePtr parseAdditive();
    ASTNodePtr parseMultiplicative();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();

    bool isTypeKeyword(TokenType type) const;

    // Basic error-recovery: skip tokens until the next ';' or '}' or EOF
    void synchronize();
};

#endif