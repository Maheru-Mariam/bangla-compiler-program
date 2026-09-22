#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// Every kind of token our lexer can produce
enum class TokenType
{
    // Keywords
    IF,           // যদি
    ELSE,         // নাহয়
    WHILE,        // যতক্ষণ
    FOR,          // প্রতি
    TO,           // থেকে
    STEP,         // ধাপ
    PRINT,        // দেখাও
    TYPE_INT,     // পূর্ণসংখ্যা
    TYPE_DECIMAL, // দশমিকসংখ্যা
    TYPE_TEXT,    // লেখা
    TYPE_BOOL,    // বুলিয়ান
    TRUE_LIT,     // সত্যি
    FALSE_LIT,    // মিথ্যা

    // Literals & identifiers
    IDENTIFIER,
    INT_LITERAL,
    DECIMAL_LITERAL,
    STRING_LITERAL,

    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    ASSIGN,        // =
    EQUALS,        // ==
    NOT_EQUALS,    // !=
    LESS,          //
    GREATER,       // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=
    AND,           // &&
    OR,            // ||
    NOT,           // !

    // Punctuation
    LPAREN,    // (
    RPAREN,    // )
    LBRACE,    // {
    RBRACE,    // }
    SEMICOLON, // ;

    // Special
    END_OF_FILE,
    UNKNOWN
};

// A single token: its type, the raw text it came from, and its line number
struct Token
{
    TokenType type;
    std::string lexeme; // the actual text, e.g. "গণনা" or "৫"
    int line;

    Token(TokenType t, const std::string &lex, int ln)
        : type(t), lexeme(lex), line(ln) {}
};

// Human-readable name for each token type — used for debugging/printing only
inline std::string tokenTypeName(TokenType type)
{
    switch (type)
    {
    case TokenType::IF:
        return "IF";
    case TokenType::ELSE:
        return "ELSE";
    case TokenType::WHILE:
        return "WHILE";
    case TokenType::FOR:
        return "FOR";
    case TokenType::TO:
        return "TO";
    case TokenType::STEP:
        return "STEP";
    case TokenType::PRINT:
        return "PRINT";
    case TokenType::TYPE_INT:
        return "TYPE_INT";
    case TokenType::TYPE_DECIMAL:
        return "TYPE_DECIMAL";
    case TokenType::TYPE_TEXT:
        return "TYPE_TEXT";
    case TokenType::TYPE_BOOL:
        return "TYPE_BOOL";
    case TokenType::TRUE_LIT:
        return "TRUE_LIT";
    case TokenType::FALSE_LIT:
        return "FALSE_LIT";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::INT_LITERAL:
        return "INT_LITERAL";
    case TokenType::DECIMAL_LITERAL:
        return "DECIMAL_LITERAL";
    case TokenType::STRING_LITERAL:
        return "STRING_LITERAL";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::STAR:
        return "STAR";
    case TokenType::SLASH:
        return "SLASH";
    case TokenType::ASSIGN:
        return "ASSIGN";
    case TokenType::EQUALS:
        return "EQUALS";
    case TokenType::NOT_EQUALS:
        return "NOT_EQUALS";
    case TokenType::LESS:
        return "LESS";
    case TokenType::GREATER:
        return "GREATER";
    case TokenType::LESS_EQUAL:
        return "LESS_EQUAL";
    case TokenType::GREATER_EQUAL:
        return "GREATER_EQUAL";
    case TokenType::AND:
        return "AND";
    case TokenType::OR:
        return "OR";
    case TokenType::NOT:
        return "NOT";
    case TokenType::LPAREN:
        return "LPAREN";
    case TokenType::RPAREN:
        return "RPAREN";
    case TokenType::LBRACE:
        return "LBRACE";
    case TokenType::RBRACE:
        return "RBRACE";
    case TokenType::SEMICOLON:
        return "SEMICOLON";
    case TokenType::END_OF_FILE:
        return "END_OF_FILE";
    case TokenType::UNKNOWN:
        return "UNKNOWN";
    default:
        return "?";
    }
}

#endif