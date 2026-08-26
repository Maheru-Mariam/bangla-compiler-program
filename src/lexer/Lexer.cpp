#include "Lexer.h"
#include <unordered_map>
#include <stdexcept>

// ---------- keyword table ----------
// Maps exact Bangla keyword text to its TokenType.
static const std::unordered_map<std::string, TokenType> keywords = {
    {"যদি", TokenType::IF},
    {"নাহয়", TokenType::ELSE},
    {"যতক্ষণ", TokenType::WHILE},
    {"দেখাও", TokenType::PRINT},
    {"পূর্ণসংখ্যা", TokenType::TYPE_INT},
    {"দশমিকসংখ্যা", TokenType::TYPE_DECIMAL},
    {"সত্যি", TokenType::TRUE_LIT},
    {"মিথ্যা", TokenType::FALSE_LIT}};

// ---------- Bangla digit table (০ - ৯) ----------
static const std::vector<std::string> banglaDigits = {
    "০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};

Lexer::Lexer(const std::string &sourceCode) : pos(0), line(1)
{
    chars = Utf8Utils::splitCodepoints(sourceCode);
}

// ---------- cursor helpers ----------

bool Lexer::isAtEnd() const
{
    return pos >= chars.size();
}

Utf8Char Lexer::peek() const
{
    if (isAtEnd())
        return "";
    return chars[pos];
}

Utf8Char Lexer::peekNext() const
{
    if (pos + 1 >= chars.size())
        return "";
    return chars[pos + 1];
}

Utf8Char Lexer::advance()
{
    Utf8Char c = chars[pos];
    pos++;
    return c;
}

// ---------- classification helpers ----------

bool Lexer::isWhitespace(const Utf8Char &c) const
{
    return c == " " || c == "\t" || c == "\r" || c == "\n";
}

bool Lexer::isBanglaDigit(const Utf8Char &c) const
{
    for (const auto &d : banglaDigits)
    {
        if (c == d)
            return true;
    }
    return false;
}

bool Lexer::isAsciiDigit(const Utf8Char &c) const
{
    return c.size() == 1 && c[0] >= '0' && c[0] <= '9';
}

int Lexer::banglaDigitValue(const Utf8Char &c) const
{
    for (size_t i = 0; i < banglaDigits.size(); ++i)
    {
        if (c == banglaDigits[i])
            return static_cast<int>(i);
    }
    return -1;
}

bool Lexer::isIdentifierChar(const Utf8Char &c) const
{
    // Anything that's not whitespace, not a digit, and not a recognized
    // ASCII operator/punctuation character counts as part of an identifier.
    // This lets Bangla letters (multi-byte) through freely.
    if (isWhitespace(c) || isBanglaDigit(c) || isAsciiDigit(c))
        return false;
    if (c.size() == 1)
    {
        char ch = c[0];
        static const std::string symbols = "+-*/=<>!&|(){};.#";
        if (symbols.find(ch) != std::string::npos)
            return false;
    }
    return true;
}

// ---------- whitespace & comments ----------

void Lexer::skipWhitespaceAndComments()
{
    while (!isAtEnd())
    {
        Utf8Char c = peek();

        if (c == "\n")
        {
            line++;
            advance();
        }
        else if (isWhitespace(c))
        {
            advance();
        }
        else if (c == "#")
        {
            // comment: skip everything until end of line
            while (!isAtEnd() && peek() != "\n")
            {
                advance();
            }
        }
        else
        {
            break;
        }
    }
}

// ---------- token producers ----------

Token Lexer::scanIdentifierOrKeyword()
{
    std::string text;
    int startLine = line;

    while (!isAtEnd() && isIdentifierChar(peek()))
    {
        text += advance();
    }

    auto it = keywords.find(text);
    if (it != keywords.end())
    {
        return Token(it->second, text, startLine);
    }
    return Token(TokenType::IDENTIFIER, text, startLine);
}

Token Lexer::scanNumber()
{
    std::string text;
    int startLine = line;
    bool isDecimal = false;

    while (!isAtEnd() && isBanglaDigit(peek()))
    {
        text += advance();
    }

    // Check for a decimal point followed by more digits
    if (!isAtEnd() && peek() == ".")
    {
        isDecimal = true;
        text += advance(); // consume '.'
        while (!isAtEnd() && isBanglaDigit(peek()))
        {
            text += advance();
        }
    }

    return Token(isDecimal ? TokenType::DECIMAL_LITERAL : TokenType::INT_LITERAL,
                 text, startLine);
}

Token Lexer::scanOperatorOrPunctuation()
{
    int startLine = line;
    Utf8Char c = advance();
    char ch = c[0]; // safe: all operators/punctuation are single ASCII bytes

    switch (ch)
    {
    case '+':
        return Token(TokenType::PLUS, "+", startLine);
    case '-':
        return Token(TokenType::MINUS, "-", startLine);
    case '*':
        return Token(TokenType::STAR, "*", startLine);
    case '/':
        return Token(TokenType::SLASH, "/", startLine);
    case '(':
        return Token(TokenType::LPAREN, "(", startLine);
    case ')':
        return Token(TokenType::RPAREN, ")", startLine);
    case '{':
        return Token(TokenType::LBRACE, "{", startLine);
    case '}':
        return Token(TokenType::RBRACE, "}", startLine);
    case ';':
        return Token(TokenType::SEMICOLON, ";", startLine);

    case '=':
        if (peek() == "=")
        {
            advance();
            return Token(TokenType::EQUALS, "==", startLine);
        }
        return Token(TokenType::ASSIGN, "=", startLine);

    case '!':
        if (peek() == "=")
        {
            advance();
            return Token(TokenType::NOT_EQUALS, "!=", startLine);
        }
        return Token(TokenType::NOT, "!", startLine);

    case '<':
        if (peek() == "=")
        {
            advance();
            return Token(TokenType::LESS_EQUAL, "<=", startLine);
        }
        return Token(TokenType::LESS, "<", startLine);

    case '>':
        if (peek() == "=")
        {
            advance();
            return Token(TokenType::GREATER_EQUAL, ">=", startLine);
        }
        return Token(TokenType::GREATER, ">", startLine);

    case '&':
        if (peek() == "&")
        {
            advance();
            return Token(TokenType::AND, "&&", startLine);
        }
        return Token(TokenType::UNKNOWN, "&", startLine);

    case '|':
        if (peek() == "|")
        {
            advance();
            return Token(TokenType::OR, "||", startLine);
        }
        return Token(TokenType::UNKNOWN, "|", startLine);

    default:
        return Token(TokenType::UNKNOWN, c, startLine);
    }
}

// ---------- main driver ----------

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (true)
    {
        skipWhitespaceAndComments();

        if (isAtEnd())
        {
            tokens.emplace_back(TokenType::END_OF_FILE, "", line);
            break;
        }

        Utf8Char c = peek();

        if (isBanglaDigit(c))
        {
            tokens.push_back(scanNumber());
        }
        else if (isIdentifierChar(c))
        {
            tokens.push_back(scanIdentifierOrKeyword());
        }
        else
        {
            tokens.push_back(scanOperatorOrPunctuation());
        }
    }

    return tokens;
}