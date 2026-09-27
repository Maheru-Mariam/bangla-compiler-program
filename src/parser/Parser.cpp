#include "Parser.h"
#include "../utils/Utf8Utils.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token> &toks, ErrorReporter &reporter)
    : tokens(toks), pos(0), errors(reporter)
{
    // The lexer always appends END_OF_FILE, but guard anyway so that
    // peek()/previous() can never index an empty vector.
    if (tokens.empty())
    {
        tokens.emplace_back(TokenType::END_OF_FILE, "", 1);
    }
}

// Human-readable description of a token, for error messages.
static std::string describe(const Token &token)
{
    if (token.type == TokenType::END_OF_FILE)
        return "end of file";
    return "\"" + token.lexeme + "\"";
}

// ---------------- cursor helpers ----------------

const Token &Parser::peek() const
{
    return tokens[pos];
}

const Token &Parser::previous() const
{
    // Guard the very first token: if nothing has been consumed yet there
    // is no previous token, and tokens[pos - 1] would wrap around to a
    // huge index and read out of bounds.
    if (pos == 0)
    {
        return tokens[0];
    }
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
    // Report at the end of what we did parse successfully, not at the
    // token that surprised us: a missing ';' belongs to the line the
    // statement was written on, not the line of the next statement.
    int reportLine = (pos > 0) ? previous().line : peek().line;
    throw error(peek(), errorMessage, reportLine);
}

ParseError Parser::error(const Token &token, const std::string &message, int line)
{
    errors.report(line >= 0 ? line : token.line,
                  message + ", but found " + describe(token),
                  "Syntax error");
    return ParseError(message);
}

bool Parser::isTypeKeyword(TokenType type) const
{
    return type == TokenType::TYPE_INT ||
           type == TokenType::TYPE_DECIMAL ||
           type == TokenType::TYPE_TEXT ||
           type == TokenType::TYPE_BOOL;
}

bool Parser::isStatementStart(TokenType type) const
{
    return isTypeKeyword(type) ||
           type == TokenType::IDENTIFIER ||
           type == TokenType::IF ||
           type == TokenType::WHILE ||
           type == TokenType::FOR ||
           type == TokenType::PRINT;
}

void Parser::synchronize()
{
    // Tracks '{'/'}' nesting seen *during this call*, relative to wherever
    // the error happened. Without this, an error thrown partway through a
    // still-open construct (e.g. a missing ')' before that construct's own
    // '{') would make the first '}' we see look like a safe stopping point,
    // even though it actually belongs to — and is still needed to close —
    // that construct's own body, not the one the error interrupted.
    int depth = 0;

    while (!isAtEnd())
    {
        // We just consumed the ';' that ended the broken statement, and are
        // not still skipping over some nested, unbalanced block.
        if (depth == 0 && pos > 0 && previous().type == TokenType::SEMICOLON)
            return;

        if (check(TokenType::LBRACE))
        {
            depth++;
            advance();
            continue;
        }

        if (check(TokenType::RBRACE))
        {
            if (depth == 0)
                return; // Leave the '}' for parseBlock to consume.
            depth--;
            advance();
            continue;
        }

        // A token that can only begin a statement is a safe restart point,
        // but only once we're back at the depth we started at — inside a
        // nested block we're skipping over, it still belongs to that block.
        // Stopping here means one bad statement no longer swallows the
        // statement that follows it.
        if (depth == 0 && isStatementStart(peek().type))
            return;

        advance();
    }
}

// ---------------- top level ----------------

ASTNodePtr Parser::parseProgram()
{
    auto program = std::make_unique<ProgramNode>();
    program->line = peek().line;

    while (!isAtEnd())
    {
        size_t positionBefore = pos;
        try
        {
            program->statements.push_back(parseStatement());
        }
        catch (const ParseError &)
        {
            synchronize();
            // Guarantee forward progress. If the failed statement and the
            // recovery both left the cursor exactly where it was, consume
            // the offending token so the same error cannot repeat forever,
            // then skip ahead to the next real boundary so that one bad
            // line does not produce a cascade of errors.
            if (pos == positionBefore)
            {
                advance();
                synchronize();
            }
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
    if (check(TokenType::FOR))
    {
        return parseForStmt();
    }
    if (check(TokenType::PRINT))
    {
        return parsePrintStmt();
    }

    throw error(peek(), "Expected the start of a statement");
}

ASTNodePtr Parser::parseDeclStmt()
{
    int startLine = peek().line;
    std::string varType = advance().lexeme;
    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").lexeme;
    expect(TokenType::ASSIGN, "Expected '=' in declaration");
    ASTNodePtr value = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after declaration");
    auto node = std::make_unique<DeclNode>(varType, name, std::move(value));
    node->line = startLine;
    return node;
}

ASTNodePtr Parser::parseAssignStmt()
{
    int startLine = peek().line;
    std::string name = expect(TokenType::IDENTIFIER, "Expected identifier").lexeme;
    expect(TokenType::ASSIGN, "Expected '=' in assignment");
    ASTNodePtr value = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after assignment");
    auto node = std::make_unique<AssignNode>(name, std::move(value));
    node->line = startLine;
    return node;
}

ASTNodePtr Parser::parseIfStmt()
{
    int startLine = peek().line;
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

    auto node = std::make_unique<IfNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
    node->line = startLine;
    return node;
}

ASTNodePtr Parser::parseWhileStmt()
{
    int startLine = peek().line;
    expect(TokenType::WHILE, "Expected 'যতক্ষণ'");
    expect(TokenType::LPAREN, "Expected '(' after 'যতক্ষণ'");
    ASTNodePtr condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    ASTNodePtr body = parseBlock();
    auto node = std::make_unique<WhileNode>(std::move(condition), std::move(body));
    node->line = startLine;
    return node;
}

// প্রতি (গণনা = ০ থেকে ৫) { ... }
// প্রতি (গণনা = ০ থেকে ১০ ধাপ ২) { ... }
ASTNodePtr Parser::parseForStmt()
{
    int startLine = peek().line;
    expect(TokenType::FOR, "Expected 'প্রতি'");
    expect(TokenType::LPAREN, "Expected '(' after 'প্রতি'");

    std::string name = expect(TokenType::IDENTIFIER, "Expected a loop variable name").lexeme;
    expect(TokenType::ASSIGN, "Expected '=' after the loop variable");
    ASTNodePtr from = parseExpression();
    expect(TokenType::TO, "Expected 'থেকে' after the starting value");
    ASTNodePtr to = parseExpression();

    ASTNodePtr by = nullptr;
    if (match(TokenType::STEP))
    {
        by = parseExpression();
    }

    expect(TokenType::RPAREN, "Expected ')' after the loop range");
    ASTNodePtr body = parseBlock();

    auto node = std::make_unique<ForNode>(name, std::move(from), std::move(to),
                                          std::move(by), std::move(body));
    node->line = startLine;
    return node;
}

ASTNodePtr Parser::parsePrintStmt()
{
    int startLine = peek().line;
    expect(TokenType::PRINT, "Expected 'দেখাও'");
    expect(TokenType::LPAREN, "Expected '(' after 'দেখাও'");
    ASTNodePtr expr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after expression");
    expect(TokenType::SEMICOLON, "Expected ';' after print statement");
    auto node = std::make_unique<PrintNode>(std::move(expr));
    node->line = startLine;
    return node;
}

ASTNodePtr Parser::parseBlock()
{
    int startLine = peek().line;
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<BlockNode>();
    block->line = startLine;

    while (!check(TokenType::RBRACE) && !isAtEnd())
    {
        size_t positionBefore = pos;
        try
        {
            block->statements.push_back(parseStatement());
        }
        catch (const ParseError &)
        {
            // Recover inside the block, so one bad statement does not
            // discard the rest of the block along with it.
            synchronize();
            if (pos == positionBefore)
            {
                advance();
                synchronize();
            }
        }
    }

    expect(TokenType::RBRACE, "Expected '}' to close this block");
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
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseLogicalAnd();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseLogicalAnd()
{
    ASTNodePtr left = parseEquality();
    while (check(TokenType::AND))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseEquality();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseEquality()
{
    ASTNodePtr left = parseRelational();
    while (check(TokenType::EQUALS) || check(TokenType::NOT_EQUALS))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseRelational();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseRelational()
{
    ASTNodePtr left = parseAdditive();
    while (check(TokenType::LESS) || check(TokenType::GREATER) ||
           check(TokenType::LESS_EQUAL) || check(TokenType::GREATER_EQUAL))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseAdditive();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseAdditive()
{
    ASTNodePtr left = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseMultiplicative();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseMultiplicative()
{
    ASTNodePtr left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr right = parseUnary();
        auto node = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
        node->line = opLine;
        left = std::move(node);
    }
    return left;
}

ASTNodePtr Parser::parseUnary()
{
    if (check(TokenType::NOT) || check(TokenType::MINUS))
    {
        int opLine = peek().line;
        std::string op = advance().lexeme;
        ASTNodePtr operand = parseUnary(); // self-recursive: handles chained unary like --৫
        auto node = std::make_unique<UnaryOpNode>(op, std::move(operand));
        node->line = opLine;
        return node;
    }
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary()
{
    if (check(TokenType::INT_LITERAL))
    {
        const Token &tok = peek();
        int lineNum = tok.line;
        int value = 0;
        try
        {
            value = Utf8Utils::banglaDigitsToInt(tok.lexeme);
        }
        catch (const std::exception &)
        {
            throw error(tok, "Invalid integer literal '" + tok.lexeme + "'");
        }
        advance();
        auto node = std::make_unique<IntLiteralNode>(value);
        node->line = lineNum;
        return node;
    }
    if (check(TokenType::DECIMAL_LITERAL))
    {
        const Token &tok = peek();
        int lineNum = tok.line;
        double value = 0.0;
        try
        {
            value = Utf8Utils::banglaDigitsToDouble(tok.lexeme);
        }
        catch (const std::exception &)
        {
            throw error(tok, "Invalid decimal literal '" + tok.lexeme + "'");
        }
        advance();
        auto node = std::make_unique<DecimalLiteralNode>(value);
        node->line = lineNum;
        return node;
    }
    if (check(TokenType::STRING_LITERAL))
    {
        int lineNum = peek().line;
        std::string text = advance().lexeme;
        auto node = std::make_unique<StringLiteralNode>(text);
        node->line = lineNum;
        return node;
    }
    if (check(TokenType::TRUE_LIT))
    {
        int lineNum = peek().line;
        advance();
        auto node = std::make_unique<BoolLiteralNode>(true);
        node->line = lineNum;
        return node;
    }
    if (check(TokenType::FALSE_LIT))
    {
        int lineNum = peek().line;
        advance();
        auto node = std::make_unique<BoolLiteralNode>(false);
        node->line = lineNum;
        return node;
    }
    if (check(TokenType::IDENTIFIER))
    {
        int lineNum = peek().line;
        std::string name = advance().lexeme;
        auto node = std::make_unique<IdentifierNode>(name);
        node->line = lineNum;
        return node;
    }
    if (match(TokenType::LPAREN))
    {
        ASTNodePtr expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    throw error(peek(), "Expected an expression");
}