#ifndef UTF8_UTILS_H
#define UTF8_UTILS_H

#include <string>
#include <vector>

// A single "character" from the user's point of view (one Bangla letter,
// one digit, one symbol) — but stored as its raw UTF-8 bytes, since a
// Bangla letter is usually 3 bytes, not 1.
using Utf8Char = std::string;

namespace Utf8Utils
{

    // Splits a full UTF-8 source string into a list of individual
    // "characters" (each possibly multi-byte), preserving order.
    // e.g. "যদি" -> ["য", "দ", "ি"]   (3 codepoints, each ~3 bytes)
    std::vector<Utf8Char> splitCodepoints(const std::string &text);

    // Returns how many bytes the UTF-8 character starting at this byte
    // should occupy, based on its leading byte pattern (1, 2, 3, or 4).
    int codepointLength(unsigned char firstByte);

    // Converts a string of Bangla digits (e.g. "৫২") into a normal int.
    // Also tolerates a leading '-' for negative numbers, just in case.
    int banglaDigitsToInt(const std::string &text);

    // Converts a string of Bangla digits with an optional '.' decimal
    // point (e.g. "৩.১৪") into a normal double.
    double banglaDigitsToDouble(const std::string &text);

}

#endif