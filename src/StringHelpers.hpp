#ifndef STRINGHELPERS_HPP
#define STRINGHELPERS_HPP

#include <vector>
#include <string>

namespace StringHelpers
{
    // tokenize tokenLimit elements using c as delimeter
    std::vector<std::string> tokenizeString(const char *str, char c, int tokenLimit = 0);

    // trim 'c' character from start and end of a string
    std::string trim(const std::string &str, char c);
}

#endif