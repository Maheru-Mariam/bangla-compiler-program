#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>
#include <vector>

class ErrorReporter
{
public:
    // 'kind' labels which compiler stage produced the error, e.g.
    // "Lexical error", "Syntax error". Defaults to a plain "Error".
    void report(int line, const std::string &message,
                const std::string &kind = "Error");
    bool hasErrors() const;
    size_t count() const;
    void printAll() const;

private:
    std::vector<std::string> errors;
};

#endif