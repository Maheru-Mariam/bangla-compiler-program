#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "lexer/Token.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "utils/ASTPrinter.h"
#include "semantic/TypeChecker.h"
#include "error/ErrorReporter.h"
#include "optimizer/Optimizer.h"
#include "codegen/CodeGenerator.h"
#include "ir/IRGenerator.h"

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

bool writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        return false;
    }
    file << content;
    return file.good();
}

// Prints every error collected so far and returns the process exit code.
// Each stage calls this and stops, so later stages never run on input
// that an earlier stage could not make sense of.
static int reportFailure(const ErrorReporter &errors, const std::string &stage)
{
    std::cout << "\nCompilation failed during " << stage << " with "
              << errors.count() << " error(s):\n\n";
    errors.printAll();
    return 1;
}

static int run(int argc, char *argv[])
{
    std::string path = "../tests/programs/sample1.bhs";
    if (argc > 1)
    {
        path = argv[1];
    }

    std::string sourceCode = readFile(path);

    // One reporter shared by every stage, so all errors print in the
    // same format and are counted together.
    ErrorReporter errors;

    // ---------- Lexer ----------
    std::cout << "=== Lexer: Tokens ===\n\n";
    Lexer lexer(sourceCode, errors);
    std::vector<Token> tokens = lexer.tokenize();
    std::cout << "Total tokens: " << tokens.size() << "\n\n";
    for (const Token &tok : tokens)
    {
        std::cout << "[Line " << tok.line << "] "
                  << tokenTypeName(tok.type)
                  << "  \"" << tok.lexeme << "\"\n";
    }

    if (errors.hasErrors())
    {
        return reportFailure(errors, "lexical analysis");
    }

    // ---------- Parser ----------
    Parser parser(tokens, errors);
    ASTNodePtr ast = parser.parseProgram();

    if (errors.hasErrors())
    {
        return reportFailure(errors, "parsing");
    }

    std::cout << "\n=== Parser: AST ===\n\n";
    ASTPrinter::print(ast.get());

    // ---------- Semantic Analysis ----------
    std::cout << "\n=== Semantic Analysis ===\n\n";
    TypeChecker checker(errors);
    checker.check(ast.get());

    if (errors.hasErrors())
    {
        return reportFailure(errors, "semantic analysis");
    }
    std::cout << "Type check passed — no errors found.\n";

    // ---------- Optimization ----------
    std::cout << "\n=== Optimizer: Optimized AST ===\n\n";
    ASTNodePtr optimized = Optimizer::optimize(std::move(ast));
    ASTPrinter::print(optimized.get());

    // ---------- Intermediate Code ----------
    // Three-address code: every expression broken into single operations
    // joined by temporaries, and all control flow turned into explicit
    // jumps. The Python backend reads the AST directly, so this stage is
    // for inspection — it is the form a backend targeting assembly or
    // bytecode would consume.
    std::cout << "\n=== Intermediate Code: Three-Address Code ===\n\n";
    IRGenerator irgen;
    std::vector<Instruction> ir = irgen.generate(optimized.get());
    std::cout << IRGenerator::toText(ir);
    std::cout << "\n(" << ir.size() << " instructions)\n";

    // ---------- Code Generation ----------
    std::cout << "\n=== Code Generator: Generated Python ===\n\n";
    CodeGenerator codegen;
    std::string pythonCode = codegen.generate(optimized.get());
    std::cout << pythonCode;

    const std::string outputPath = "output.py";
    if (!writeFile(outputPath, pythonCode))
    {
        std::cerr << "\nError: could not write " << outputPath << "\n";
        return 1;
    }
    std::cout << "\nWritten to " << outputPath << "\n";

    return 0;
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Last line of defence: any unexpected exception becomes a clear
    // message and a non-zero exit code, never a crash.
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error: an unknown internal error occurred.\n";
        return 1;
    }
}