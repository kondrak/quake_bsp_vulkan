#include "StringHelpers.hpp"

namespace StringHelpers
{
    std::vector<std::string> tokenizeString(const char *str, char c, int tokenLimit)
    {
        std::vector<std::string> result;
        int currCount = 0;

        do
        {
            // stop further tokenization if we reached token number limit
            if (++currCount == tokenLimit)
            {
                result.push_back(std::string(str));
                break;
            }

            const char *begin = str;

            while (*str != c && *str)
                str++;

            result.push_back(std::string(begin, str));

        } while (*str++);

        return result;
    }


    std::string trim(const std::string &str, char c)
    {
        size_t firstPos = str.find_first_not_of(c);
        size_t lastPos  = str.find_last_not_of(c);

        if (firstPos == std::string::npos)
            return str;

        return str.substr(firstPos, lastPos - firstPos + 1);
    }
}