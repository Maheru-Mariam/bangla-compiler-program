#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include "../lexer/Token.h"
#include "AST.h"
#include "../error/ErrorReporter.h"

// Thrown internally when a statement cannot be parsed. It is always
// caught inside the parser itself (by parseProgram / parseBlock), which
// then recovers and carries on. Having our own type means we never
// accidentally swallow an unrelated std::runtime_error.
class ParseError : public std::runtime_error
{
public:
    explicit ParseError(const std::string &what) : std::runtime_error(what) {}
};

class Parser
{
public:
    Parser(const std::vector<Token> &tokens, ErrorReporter &reporter);

    // Entry point: parses the whole token stream into a ProgramNode.
    // Never throws: syntax errors are collected in the ErrorReporter.
    ASTNodePtr parseProgram();

private:
    std::vector<Token> tokens;
    size_t pos;
    ErrorReporter &errors;

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
    ASTNodePtr parseForStmt();
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
    // Can this token type legally begin a statement? Used by the error
    // recovery below to find a sensible place to resume parsing.
    bool isStatementStart(TokenType type) const;

    // Reports a syntax error and throws ParseError to unwind to the
    // nearest recovery point. 'line' overrides where the error is
    // reported; by default it is the line of the offending token.
    ParseError error(const Token &token, const std::string &message,
                     int line = -1);

    // Basic error recovery: skip tokens until we reach a point where
    // parsing can sensibly restart (just past a ';', at a '}', or at a
    // token that can begin a new statement).
    void synchronize();
};

#endif