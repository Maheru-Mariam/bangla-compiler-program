#include "Parser.h"
#include <stdexcept>
#include <iostream>
#include "../utils/Utf8Utils.h"

Parser::Parser(const std::vector<Token> &toks) : tokens(toks), pos(0) {}

// ---------------- cursor helpers ----------------

const Token &Parser::peek() const
{
    return tokens[pos];
}

const Token &Parser::previous() const
{
    return tokens[pos - 1];
}

bool Parser::isAtEnd() const
{
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const
{
    if (isAtEnd() && type != TokenType::END_OF_FILE)
        return false;
    return peek().type == type;
}

const Token &Parser::advance()
{
    if (!isAtEnd())
        pos++;
    return previous();
}

bool Parser::match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

const Token &Parser::expect(TokenType type, const std::string &errorMessage)
{
    if (check(type))
    {
        return advance();
    }
    std::cerr << "Parse error at line " << peek().line << ": " << errorMessage
              << " (got \"" << peek().lexeme << "\")\n";
    throw std::runtime_error(errorMessage);
}

bool Parser::isTypeKeyword(TokenType type) const
{
    return type == TokenType::TYPE_INT || type == TokenType::TYPE_DECIMAL;
}

void Parser::synchronize()
{
    // Basic error recovery: skip tokens until we hit ';' (end of statement)
    // or '}' (end of block) or run out of tokens.
    while (!isAtEnd())
    {
        if (previous().type == TokenType::SEMICOLON)
            return;
        if (check(TokenType::RBRACE))
            return;
        advance();
    }
}

// ---------------- top level ----------------

ASTNodePtr Parser::parseProgram()
{
    auto program = std::make_unique<ProgramNode>();

    while (!isAtEnd())
    {
        try
        {
            program->statements.push_back(parseStatement());
        }
        catch (const std::runtime_error &)
        {
            synchronize(); // recover and keep parsing rest of the program
        }
    }

    return program;
}

// ---------------- statements ----------------

ASTNodePtr Parser::parseStatement()
{
    if (isTypeKeyword(peek().type))
    {
        return parseDeclStmt();
    }
    if (check(TokenType::IDENTIFIER))
    {
        return parseAssignStmt();
    }
    if (check(TokenType::IF))
    {
        return parseIfStmt();
    }
    if (check(TokenType::WHILE))
    {
        return parseWhileStmt();
    }
    if (check(TokenType::PRINT))
    {
        return parsePrintStmt();
    }

    throw std::runtime_error("Expected a statement");
}

ASTNodePtr Parser::parseDeclStmt()
{
    std::string varType = advance().lexeme; // consume type keyword
    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").lexeme;
    expect(TokenType::ASSIGN, "Expected '=' in declaration");
    ASTNodePtr value = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after declaration");
    return std::make_unique<DeclNode>(varType, name, std::move(value));
}

ASTNodePtr Parser::parseAssignStmt()
{
    std::string name = expect(TokenType::IDENTIFIER, "Expected identifier").lexeme;
    expect(TokenType::ASSIGN, "Expected '=' in assignment");
    ASTNodePtr value = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after assignment");
    return std::make_unique<AssignNode>(name, std::move(value));
}

ASTNodePtr Parser::parseIfStmt()
{
    expect(TokenType::IF, "Expected 'যদি'");
    expect(TokenType::LPAREN, "Expected '(' after 'যদি'");
    ASTNodePtr condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    ASTNodePtr thenBlock = parseBlock();

    ASTNodePtr elseBlock = nullptr;
    if (match(TokenType::ELSE))
    {
        elseBlock = parseBlock();
    }

    return std::make_unique<IfNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

ASTNodePtr Parser::parseWhileStmt()
{
    expect(TokenType::WHILE, "Expected 'যতক্ষণ'");
    expect(TokenType::LPAREN, "Expected '(' after 'যতক্ষণ'");
    ASTNodePtr condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    ASTNodePtr body = parseBlock();
    return std::make_unique<WhileNode>(std::move(condition), std::move(body));
}

ASTNodePtr Parser::parsePrintStmt()
{
    expect(TokenType::PRINT, "Expected 'দেখাও'");
    expect(TokenType::LPAREN, "Expected '(' after 'দেখাও'");
    ASTNodePtr expr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after expression");
    expect(TokenType::SEMICOLON, "Expected ';' after print statement");
    return std::make_unique<PrintNode>(std::move(expr));
}

ASTNodePtr Parser::parseBlock()
{
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<BlockNode>();

    while (!check(TokenType::RBRACE) && !isAtEnd())
    {
        block->statements.push_back(parseStatement());
    }

    expect(TokenType::RBRACE, "Expected '}'");
    return block;
}

// ---------------- expressions (precedence climbing via grammar layers) ----------------

ASTNodePtr Parser::parseExpression()
{
    return parseLogicalOr();
}

ASTNodePtr Parser::parseLogicalOr()
{
    ASTNodePtr left = parseLogicalAnd();
    while (check(TokenType::OR))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseLogicalAnd();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseLogicalAnd()
{
    ASTNodePtr left = parseEquality();
    while (check(TokenType::AND))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseEquality();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseEquality()
{
    ASTNodePtr left = parseRelational();
    while (check(TokenType::EQUALS) || check(TokenType::NOT_EQUALS))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseRelational();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseRelational()
{
    ASTNodePtr left = parseAdditive();
    while (check(TokenType::LESS) || check(TokenType::GREATER) ||
           check(TokenType::LESS_EQUAL) || check(TokenType::GREATER_EQUAL))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseAdditive();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseAdditive()
{
    ASTNodePtr left = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseMultiplicative();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseMultiplicative()
{
    ASTNodePtr left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH))
    {
        std::string op = advance().lexeme;
        ASTNodePtr right = parseUnary();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTNodePtr Parser::parseUnary()
{
    if (check(TokenType::NOT) || check(TokenType::MINUS))
    {
        std::string op = advance().lexeme;
        ASTNodePtr operand = parseUnary(); // self-recursive: handles chained unary like --৫
        return std::make_unique<UnaryOpNode>(op, std::move(operand));
    }
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary()
{
    if (check(TokenType::INT_LITERAL))
    {
        int value = Utf8Utils::banglaDigitsToInt(advance().lexeme);
        return std::make_unique<IntLiteralNode>(value);
    }
    if (check(TokenType::DECIMAL_LITERAL))
    {
        double value = Utf8Utils::banglaDigitsToDouble(advance().lexeme);
        return std::make_unique<DecimalLiteralNode>(value);
    }
    if (check(TokenType::TRUE_LIT))
    {
        advance();
        return std::make_unique<BoolLiteralNode>(true);
    }
    if (check(TokenType::FALSE_LIT))
    {
        advance();
        return std::make_unique<BoolLiteralNode>(false);
    }
    if (check(TokenType::IDENTIFIER))
    {
        std::string name = advance().lexeme;
        return std::make_unique<IdentifierNode>(name);
    }
    if (match(TokenType::LPAREN))
    {
        ASTNodePtr expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    throw std::runtime_error("Expected an expression");
}