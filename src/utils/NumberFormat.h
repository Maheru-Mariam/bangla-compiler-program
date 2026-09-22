#ifndef NUMBER_FORMAT_H
#define NUMBER_FORMAT_H

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>

// Rendering a double as text is easy to get subtly wrong. The default
// stream precision is 6 significant digits, so 3.14159265 would print as
// 3.14159 (silently losing precision), and 4.0 would print as "4", which
// Python would then read as an int rather than a float.
//
// This helper finds the shortest representation that reads back as the
// exact same double, and guarantees a decimal point is present.
inline std::string formatDecimalLiteral(double value)
{
    std::string text;
    for (int precision = 15; precision <= 17; ++precision)
    {
        std::ostringstream oss;
        oss << std::setprecision(precision) << value;
        text = oss.str();
        if (std::strtod(text.c_str(), nullptr) == value)
        {
            break; // round-trips exactly, no need for more digits
        }
    }

    if (text.find('.') == std::string::npos &&
        text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos &&
        text.find("inf") == std::string::npos &&
        text.find("nan") == std::string::npos)
    {
        text += ".0";
    }
    return text;
}

#endif