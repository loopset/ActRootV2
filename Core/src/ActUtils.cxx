#include "ActUtils.h"

#include <algorithm>
#include <cctype>

bool ActRoot::IsEqZero(int val)
{
    return val == 0;
}

bool ActRoot::IsEqZero(float val)
{
    return val * val <= kFloatEps * kFloatEps;
}

bool ActRoot::IsEqZero(double val)
{
    return val * val <= kDoubleEps * kDoubleEps;
}

std::string ActRoot::StripSpaces(std::string line)
{
    // Remove preceding spaces
    while(*line.begin() == ' ')
        line = line.substr(1, line.length());
    // Remove trailing spaces
    if(line.length() > 0)
        while(*line.rbegin() == ' ')
            line = line.substr(0, line.length() - 1);
    // Remove preceding tabs
    while(*line.begin() == '\t')
        line = line.substr(1, line.length());
    // Remove trailing tabs
    if(line.length() > 0)
        while(*line.rbegin() == '\t')
            line = line.substr(0, line.length() - 1);
    return line;
}

std::string ActRoot::ToLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}
