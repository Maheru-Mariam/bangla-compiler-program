#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/Token.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "utils/ASTPrinter.h"

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

    std::string path = "../tests/programs/sample1.bhs";
    if (argc > 1)
    {
        path = argv[1];
    }

    std::string sourceCode = readFile(path);

    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "=== Tokens: " << tokens.size() << " ===\n\n";

    Parser parser(tokens);
    ASTNodePtr ast = parser.parseProgram();

    std::cout << "\n=== AST ===\n\n";
    ASTPrinter::print(ast.get());

    return 0;
}