#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>
#include <vector>

class ErrorReporter
{
public:
    void report(int line, const std::string &message);
    bool hasErrors() const;
    void printAll() const;

private:
    std::vector<std::string> errors;
};

#endif