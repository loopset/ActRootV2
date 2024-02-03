#include "ActInputParser.h"

#include "TString.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

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

std::string ActRoot::InputBlock::GetToken(const std::string& line)
{
    // Find separator
    auto pos {line.find(ActRoot::kTokenSeparator)};
    if(pos == line.npos) // if not found, likely it is a continuation of a previous token -> return empty token
        return "";
    return line.substr(0, pos);
}

void ActRoot::InputBlock::GetValues(const std::string& line, const std::string& token, bool findTokenSeparator)
{
    // Find separator if exists
    std::string values {};
    if(!findTokenSeparator)
    {
        values = line;
    }
    else
    {
        auto pos = line.find(ActRoot::kTokenSeparator);
        values = line.substr(pos + 1);
    }
    // split by value separator
    std::size_t previous {0};
    std::size_t actual {};
    while((actual = values.find_first_of(ActRoot::kValueSeparator, previous)) != std::string::npos)
    {
        if(actual > previous)
            fValues[token].push_back(StripSpaces(values.substr(previous, actual - previous)));
        previous = actual + 1;
    }
    // push the rest of the line to the vector
    if(previous < values.length())
        fValues[token].push_back(StripSpaces(values.substr(previous)));
}

void ActRoot::InputBlock::AddLine(const std::string& line)
{
    auto token {GetToken(line)};
    if(token.length() == 0) // is a continuation of the .back() token; append to it
    {
        GetValues(line, fTokens.back(), false); // false bc this new line does not have Token= listed
        return;
    }
    fTokens.push_back(token);
    GetValues(line, token);
}

bool ActRoot::InputBlock::CheckTokenExists(const std::string& token, bool soft)
{
    bool exists {static_cast<bool>(fValues.count(token))};
    if(!exists && !soft)
        throw std::runtime_error("Token " + token + " does not exit in InputBlock");
    return exists;
}

bool ActRoot::InputBlock::IsVector(const std::string& token)
{
    auto size {fValues[token].size()};
    if(size == 0)
        throw std::runtime_error("Token " + token + " has empty data");
    else if(size == 1)
        return false;
    else
        return true;
}


std::string ActRoot::InputBlock::GetString(const std::string& token)
{
    CheckTokenExists(token);
    if(IsVector(token))
        std::cout << "Token " + token + " really is a vector";
    return fValues[token].front();
}

int ActRoot::InputBlock::StringToInt(const std::string& val)
{
    int ret {};
    try
    {
        ret = std::stoi(val);
        return ret;
    }
    catch(std::exception& e)
    {
        std::cout << "Could not convert to int value " << val << '\n';
        throw std::runtime_error(e.what());
    }
}

double ActRoot::InputBlock::StringToDouble(const std::string& val)
{
    double ret {};
    try
    {
        ret = std::stod(val);
        return ret;
    }
    catch(std::exception& e)
    {
        std::cout << "Could not convert to double value " << val << '\n';
        throw std::runtime_error(e.what());
    }
}

bool ActRoot::InputBlock::StringToBool(const std::string& val)
{
    bool ret {};
    TString aux {val};
    aux.ToLower();
    std::string lower {aux};
    if(lower != "true" && lower != "false")
        throw std::runtime_error("Could not convert to bool value " + val);
    std::istringstream(lower) >> std::boolalpha >> ret;
    return ret;
}

int ActRoot::InputBlock::GetInt(const std::string& token)
{
    CheckTokenExists(token);
    if(IsVector(token))
        std::cout << "Token " + token + " really is a vector";
    return StringToInt(fValues[token].front());
}

bool ActRoot::InputBlock::GetBool(const std::string& token)
{
    CheckTokenExists(token);
    if(IsVector(token))
        std::cout << "Token " + token + " really is a vector";
    return StringToBool(fValues[token].front());
}

double ActRoot::InputBlock::GetDouble(const std::string& token)
{
    CheckTokenExists(token);
    if(IsVector(token))
        std::cout << "Token " + token + " really is a vector";
    return StringToDouble(fValues[token].front());
}

std::vector<std::string> ActRoot::InputBlock::GetStringVector(const std::string& token)
{
    CheckTokenExists(token);
    return fValues[token];
}

std::vector<int> ActRoot::InputBlock::GetIntVector(const std::string& token)
{
    CheckTokenExists(token);
    std::vector<int> ret {};
    for(int v = 0; v < fValues[token].size(); v++)
    {
        const auto& val {fValues[token][v]};
        if(val != ActRoot::kExpandValue)
            ret.push_back(StringToInt(val));
        if(val == ActRoot::kExpandValue)
        {
            // Begin is previous value
            // End is next value
            int end {};
            try
            {
                end = StringToInt(fValues[token].at(v + 1));
            }
            catch(std::out_of_range& e)
            {
                throw std::out_of_range("... expansion requires next (begin, END] element to be present");
            }
            auto expansion {ExpandInt(ret.back(), end)};
            // Insert
            ret.insert(ret.end(), expansion.begin(), expansion.end());
            // End is already in ret, skip next iteration
            v += 1;
        }
    }
    return ret;
}

std::vector<bool> ActRoot::InputBlock::GetBoolVector(const std::string& token)
{
    CheckTokenExists(token);
    std::vector<bool> ret {};
    for(int v = 0; v < fValues[token].size(); v++)
        ret.push_back(StringToBool(fValues[token][v]));
    return ret;
}

std::vector<double> ActRoot::InputBlock::GetDoubleVector(const std::string& token)
{
    CheckTokenExists(token);
    std::vector<double> ret {};
    for(int v = 0; v < fValues[token].size(); v++)
        ret.push_back(StringToDouble(fValues[token][v]));
    return ret;
}

std::vector<int> ActRoot::InputBlock::ExpandInt(int begin, int end)
{
    std::vector<int> ret;
    for(int i = begin + 1; i <= end; i++)
        ret.push_back(i);
    return ret;
}

void ActRoot::InputParser::ReadFile(const std::string& filename)
{
    // Open file
    std::ifstream file {filename};
    if(!file)
        throw std::runtime_error("ActRoot::InputParser: error! " + filename + " could not be opened");
    std::string rawLine {};
    bool inHeader {false};
    while(std::getline(file, rawLine))
    {
        auto line {StripSpaces(rawLine)};
        if(IsComment(line))
            continue;
        auto header {IsBlockHeader(line)};
        if(header != "")
        {
            fBlocks.push_back(std::make_shared<InputBlock>(header));
            inHeader = true;
        }
        else if(inHeader)
        {
            fBlocks.back()->AddLine(line);
        }
        else
            continue;
    }
    file.close();
}

bool ActRoot::InputParser::IsComment(const std::string& line)
{
    if(line.length() == 0)
        return true;

    auto pos {line.find(ActRoot::kCommentSeparator)};
    if(pos != std::string::npos)
        return true;
    else
        return false;
}

std::string ActRoot::InputParser::IsBlockHeader(const std::string& line)
{
    auto opening {line.find(ActRoot::kBlockOpening)};
    if(opening != std::string::npos)
    {
        auto closing {line.find(ActRoot::kBlockClosing)};
        return line.substr(opening + 1, closing - 1);
    }
    else
        return "";
}

void ActRoot::InputParser::Print() const
{
    for(auto& block : fBlocks)
    {
        auto vals {block->GetAllReadValues()};
        auto name {block->GetBlockName()};
        std::cout << "== Block " << name << " ==" << '\n';
        for(const auto& [token, vec] : vals)
        {
            std::cout << " Token  = " << token << '\n';
            for(const auto& e : vec)
                std::cout << " -Val: " << e << '\n';
        }
    }
}

bool ActRoot::InputParser::CheckBlockExists(const std::string& token) const
{
    for(auto& block : fBlocks)
        if(block->GetBlockName() == token)
            return true;
    return false;
}

ActRoot::BlockPtr ActRoot::InputParser::GetBlock(const std::string& token) const
{
    for(auto& block : fBlocks)
        if(block->GetBlockName() == token)
            return block;
    throw std::runtime_error("No token " + token + " was found in file!");
}

std::vector<std::string> ActRoot::InputParser::GetBlockHeaders() const
{
    std::vector<std::string> headers {};
    for(auto& block : fBlocks)
    {
        headers.push_back(block->GetBlockName());
    }
    return headers;
}
