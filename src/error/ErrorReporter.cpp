#include "ErrorReporter.h"
#include <iostream>

void ErrorReporter::report(int line, const std::string &message)
{
    errors.push_back("[Line " + std::to_string(line) + "] Error: " + message);
}

bool ErrorReporter::hasErrors() const
{
    return !errors.empty();
}

void ErrorReporter::printAll() const
{
    for (const auto &e : errors)
    {
        std::cout << e << "\n";
    }
}