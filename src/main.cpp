#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/Token.h"
#include "lexer/Lexer.h"
#ifdef _WIN32
#include <windows.h>
#endif

std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string path = "../tests/programs/sample1.bhs"; // default, relative to build/

    if (argc > 1)
    {
        path = argv[1]; // allow overriding via command line
    }

    std::string sourceCode = readFile(path);

    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Total tokens: " << tokens.size() << "\n\n";

    for (const Token &tok : tokens)
    {
        std::cout << "[Line " << tok.line << "] "
                  << tokenTypeName(tok.type)
                  << "  \"" << tok.lexeme << "\"\n";
    }

    return 0;
}