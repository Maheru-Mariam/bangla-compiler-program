#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "Token.h"
#include "../utils/Utf8Utils.h"

class Lexer
{
public:
    explicit Lexer(const std::string &sourceCode);

    // Runs the full scan and returns every token found, ending in END_OF_FILE
    std::vector<Token> tokenize();

private:
    std::vector<Utf8Char> chars; // the source, split into individual UTF-8 characters
    size_t pos;                  // index into 'chars' of the character we're about to read
    int line;                    // current line number, for error messages

    // --- low-level cursor helpers ---
    bool isAtEnd() const;
    Utf8Char peek() const;     // look at current char without consuming it
    Utf8Char peekNext() const; // look one char ahead
    Utf8Char advance();        // consume and return current char, move cursor forward

    // --- classification helpers ---
    bool isWhitespace(const Utf8Char &c) const;
    bool isBanglaDigit(const Utf8Char &c) const;
    bool isAsciiDigit(const Utf8Char &c) const;
    bool isIdentifierChar(const Utf8Char &c) const;

    // --- token producers ---
    void skipWhitespaceAndComments();
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanOperatorOrPunctuation();

    int banglaDigitValue(const Utf8Char &c) const; // ০->0 ... ৯->9
};

#endif