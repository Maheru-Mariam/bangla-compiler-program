#include "Utf8Utils.h"

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

}