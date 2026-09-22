#include "Lexer.h"
#include <unordered_map>
#include <stdexcept>

// ---------- keyword table ----------
// Maps exact Bangla keyword text to its TokenType.
static const std::unordered_map<std::string, TokenType> keywords = {
    {"যদি", TokenType::IF},
    {"নাহয়", TokenType::ELSE},
    {"যতক্ষণ", TokenType::WHILE},
    {"প্রতি", TokenType::FOR},
    {"থেকে", TokenType::TO},
    {"ধাপ", TokenType::STEP},
    {"দেখাও", TokenType::PRINT},
    {"পূর্ণসংখ্যা", TokenType::TYPE_INT},
    {"দশমিকসংখ্যা", TokenType::TYPE_DECIMAL},
    {"লেখা", TokenType::TYPE_TEXT},
    {"বুলিয়ান", TokenType::TYPE_BOOL},
    {"সত্যি", TokenType::TRUE_LIT},
    {"মিথ্যা", TokenType::FALSE_LIT}};

// ---------- Bangla digit table (০ - ৯) ----------
static const std::vector<std::string> banglaDigits = {
    "০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};

Lexer::Lexer(const std::string &sourceCode, ErrorReporter &reporter)
    : pos(0), line(1), errors(reporter)
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

// English letters are not valid in identifiers, but we detect them so we
// can give a clear error instead of "unexpected character" one at a time.
static bool isAsciiLetter(const Utf8Char &c)
{
    if (c.size() != 1)
        return false;
    char ch = c[0];
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
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

// Decodes one UTF-8 character back into its Unicode codepoint, so we can
// test it against the Bangla ranges below.
static unsigned int codepointOf(const Utf8Char &c)
{
    if (c.empty())
        return 0;
    unsigned char b0 = static_cast<unsigned char>(c[0]);
    if ((b0 & 0x80) == 0x00)
        return b0;
    if ((b0 & 0xE0) == 0xC0 && c.size() >= 2)
        return ((b0 & 0x1Fu) << 6) |
               (static_cast<unsigned char>(c[1]) & 0x3Fu);
    if ((b0 & 0xF0) == 0xE0 && c.size() >= 3)
        return ((b0 & 0x0Fu) << 12) |
               ((static_cast<unsigned char>(c[1]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(c[2]) & 0x3Fu);
    if ((b0 & 0xF8) == 0xF0 && c.size() >= 4)
        return ((b0 & 0x07u) << 18) |
               ((static_cast<unsigned char>(c[1]) & 0x3Fu) << 12) |
               ((static_cast<unsigned char>(c[2]) & 0x3Fu) << 6) |
               (static_cast<unsigned char>(c[3]) & 0x3Fu);
    return 0;
}

// A Bangla letter (consonant or independent vowel): the only thing an
// identifier is allowed to START with.
static bool isBanglaLetter(const Utf8Char &c)
{
    unsigned int cp = codepointOf(c);
    return (cp >= 0x0985 && cp <= 0x098C) || // vowels অ - ঌ
           (cp >= 0x098F && cp <= 0x0990) || // এ ঐ
           (cp >= 0x0993 && cp <= 0x09A8) || // ও - ন
           (cp >= 0x09AA && cp <= 0x09B0) || // প - র
           (cp == 0x09B2) ||                 // ল
           (cp >= 0x09B6 && cp <= 0x09B9) || // শ - হ
           (cp == 0x09BD) ||                 // ঽ avagraha
           (cp == 0x09CE) ||                 // ৎ khanda ta
           (cp >= 0x09DC && cp <= 0x09DD) || // ড় ঢ়
           (cp >= 0x09DF && cp <= 0x09E1) || // য় ঌ ৡ
           (cp >= 0x09F0 && cp <= 0x09F1);   // ৰ ৱ
}

// Vowel signs (কারচিহ্ন), hasant, candrabindu and friends. These never
// start a word but are everywhere inside one — গণনা ends with 'া'.
static bool isBanglaMark(const Utf8Char &c)
{
    unsigned int cp = codepointOf(c);
    return (cp >= 0x0981 && cp <= 0x0983) || // ঁ ং ঃ
           (cp == 0x09BC) ||                 // nukta
           (cp >= 0x09BE && cp <= 0x09C4) || // া ি ী ু ূ ৃ ৄ
           (cp >= 0x09C7 && cp <= 0x09C8) || // ে ৈ
           (cp >= 0x09CB && cp <= 0x09CD) || // ো ৌ ্ (hasant)
           (cp == 0x09D7) ||                 // au length mark
           (cp >= 0x09E2 && cp <= 0x09E3);   // vowel signs ৢ ৣ
}

// Can this character appear INSIDE an identifier (after the first letter)?
// Bangla letters, Bangla vowel marks, Bangla digits ০-৯, and underscore.
//
// Identifiers in this language are deliberately Bangla-only. That keeps
// the language true to its purpose, and it also means a generated Python
// name can never collide with a Python keyword like 'print' or 'class',
// since none of those can be written in Bangla letters.
bool Lexer::isIdentifierChar(const Utf8Char &c) const
{
    return isBanglaLetter(c) || isBanglaMark(c) || isBanglaDigit(c) || c == "_";
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

// Scans a text literal: "..." with \n, \t, \" and \\ escapes.
// The token's lexeme holds the DECODED text (escapes already applied),
// so later stages never have to think about escapes again.
Token Lexer::scanString()
{
    int startLine = line;
    advance(); // consume the opening quote
    std::string text;

    while (!isAtEnd() && peek() != "\"")
    {
        Utf8Char c = peek();

        if (c == "\n")
        {
            // A newline inside a text literal is almost always a missing
            // closing quote, so stop here rather than swallowing the file.
            errors.report(startLine, "Text literal is not closed before the end of the line",
                          "Lexical error");
            return Token(TokenType::STRING_LITERAL, text, startLine);
        }

        if (c == "\\")
        {
            advance(); // consume the backslash
            Utf8Char esc = peek();
            if (esc == "n")
            {
                text += "\n";
                advance();
            }
            else if (esc == "t")
            {
                text += "\t";
                advance();
            }
            else if (esc == "\"")
            {
                text += "\"";
                advance();
            }
            else if (esc == "\\")
            {
                text += "\\";
                advance();
            }
            else
            {
                errors.report(line, "Unknown escape sequence '\\" + esc + "' in text literal",
                              "Lexical error");
                advance();
            }
            continue;
        }

        text += advance();
    }

    if (isAtEnd())
    {
        errors.report(startLine, "Text literal is not closed before the end of the file",
                      "Lexical error");
        return Token(TokenType::STRING_LITERAL, text, startLine);
    }

    advance(); // consume the closing quote
    return Token(TokenType::STRING_LITERAL, text, startLine);
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
        errors.report(startLine, "Unexpected character '&' (did you mean '&&'?)",
                      "Lexical error");
        return Token(TokenType::UNKNOWN, "&", startLine);

    case '|':
        if (peek() == "|")
        {
            advance();
            return Token(TokenType::OR, "||", startLine);
        }
        errors.report(startLine, "Unexpected character '|' (did you mean '||'?)",
                      "Lexical error");
        return Token(TokenType::UNKNOWN, "|", startLine);

    default:
        // Unknown symbol: record it and keep scanning so that a single
        // stray character does not hide every later error in the file.
        errors.report(startLine, "Unexpected character '" + c + "'", "Lexical error");
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

        if (c == "\"")
        {
            tokens.push_back(scanString());
        }
        else if (isBanglaDigit(c))
        {
            tokens.push_back(scanNumber());
        }
        else if (isBanglaLetter(c))
        {
            // Identifiers must begin with a Bangla letter.
            tokens.push_back(scanIdentifierOrKeyword());
        }
        else if (isAsciiLetter(c))
        {
            // English names are not part of this language. Consume the whole
            // word so we report it once, with a message that explains why.
            std::string word;
            int startLine = line;
            while (!isAtEnd() && (isAsciiLetter(peek()) || isAsciiDigit(peek()) || peek() == "_"))
            {
                word += advance();
            }
            errors.report(startLine,
                          "Identifiers must be written in Bangla; '" + word +
                              "' uses English letters",
                          "Lexical error");
            tokens.push_back(Token(TokenType::UNKNOWN, word, startLine));
        }
        else
        {
            tokens.push_back(scanOperatorOrPunctuation());
        }
    }

    return tokens;
}