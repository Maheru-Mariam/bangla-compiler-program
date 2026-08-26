#include "Utf8Utils.h"
#include <stdexcept>

namespace Utf8Utils
{

    int codepointLength(unsigned char firstByte)
    {
        // UTF-8 encodes character length in the leading bits of the first byte:
        //   0xxxxxxx -> 1 byte  (standard ASCII, e.g. 'a', ';', '+')
        //   110xxxxx -> 2 bytes
        //   1110xxxx -> 3 bytes (most Bangla letters/digits fall here)
        //   11110xxx -> 4 bytes (rare, e.g. emoji)
        if ((firstByte & 0x80) == 0x00)
            return 1;
        if ((firstByte & 0xE0) == 0xC0)
            return 2;
        if ((firstByte & 0xF0) == 0xE0)
            return 3;
        if ((firstByte & 0xF8) == 0xF0)
            return 4;
        return 1; // fallback: treat malformed byte as its own char, don't crash
    }

    std::vector<Utf8Char> splitCodepoints(const std::string &text)
    {
        std::vector<Utf8Char> result;
        size_t i = 0;

        while (i < text.size())
        {
            unsigned char firstByte = static_cast<unsigned char>(text[i]);
            int len = codepointLength(firstByte);

            // Guard against reading past the end of the string if the
            // input is malformed / truncated
            if (i + len > text.size())
            {
                len = static_cast<int>(text.size() - i);
            }

            result.push_back(text.substr(i, len));
            i += len;
        }

        return result;
    }

    // Maps each Bangla digit codepoint to its numeric value (০->0 ... ৯->9)
    static int singleBanglaDigitValue(const Utf8Char &c)
    {
        static const std::vector<std::string> digits = {
            "০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};
        for (size_t i = 0; i < digits.size(); ++i)
        {
            if (c == digits[i])
                return static_cast<int>(i);
        }
        return -1; // not a Bangla digit
    }

    int banglaDigitsToInt(const std::string &text)
    {
        std::vector<Utf8Char> chars = splitCodepoints(text);
        bool negative = false;
        int result = 0;
        size_t start = 0;

        if (!chars.empty() && chars[0] == "-")
        {
            negative = true;
            start = 1;
        }

        for (size_t i = start; i < chars.size(); ++i)
        {
            int digit = singleBanglaDigitValue(chars[i]);
            if (digit == -1)
            {
                throw std::runtime_error("Invalid Bangla digit in number: " + text);
            }
            result = result * 10 + digit;
        }

        return negative ? -result : result;
    }

    double banglaDigitsToDouble(const std::string &text)
    {
        std::vector<Utf8Char> chars = splitCodepoints(text);
        bool negative = false;
        double result = 0.0;
        size_t i = 0;

        if (!chars.empty() && chars[0] == "-")
        {
            negative = true;
            i = 1;
        }

        // integer part
        for (; i < chars.size() && chars[i] != "."; ++i)
        {
            int digit = singleBanglaDigitValue(chars[i]);
            if (digit == -1)
            {
                throw std::runtime_error("Invalid Bangla digit in number: " + text);
            }
            result = result * 10 + digit;
        }

        // fractional part
        if (i < chars.size() && chars[i] == ".")
        {
            i++; // skip '.'
            double fraction = 0.1;
            for (; i < chars.size(); ++i)
            {
                int digit = singleBanglaDigitValue(chars[i]);
                if (digit == -1)
                {
                    throw std::runtime_error("Invalid Bangla digit in number: " + text);
                }
                result += digit * fraction;
                fraction *= 0.1;
            }
        }

        return negative ? -result : result;
    }

}