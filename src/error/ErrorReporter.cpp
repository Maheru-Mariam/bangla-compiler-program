#include "ErrorReporter.h"
#include <iostream>

void ErrorReporter::report(int line, const std::string &message,
                           const std::string &kind)
{
    errors.push_back("[Line " + std::to_string(line) + "] " + kind + ": " + message);
}

size_t ErrorReporter::count() const
{
    return errors.size();
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