#include "ActSilParameters.h"

#include "ActInputParser.h"

#include <fstream>
#include <iostream>

std::vector<std::string> ActRoot::SilParameters::GetKeys() const
{
    std::vector<std::string> ret;
    for(const auto& [key, _] : fSizes)
        ret.push_back(key);
    return ret;
}

void ActRoot::SilParameters::Print() const
{
    std::cout << "==== SilParameters ====" << '\n';
    for(const auto& [key, vals] : fVXI)
        std::cout << "-- VXI: " << key << " contains Sil " << vals.first << " at idx " << vals.second << '\n';
    std::cout << "=======================" << '\n';
}

void ActRoot::SilParameters::ReadActions(const std::vector<std::string>& layers, const std::vector<std::string>& names,
                                         const std::string& file)
{
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("No Action file for SilParameters");
    std::string key {};
    int vxi {};
    int aux0 {};
    int aux1 {};
    while(streamer >> key >> vxi >> aux0 >> aux1)
    {
        // Clean of whitespaces
        key = StripSpaces(key);
        // Locate a name
        for(int i = 0; i < names.size(); i++)
        {
            // Find iterator
            auto it {key.find(names[i])};
            // And impose it to be at the beginning
            if(it == std::string::npos || it != 0)
                continue;
            // If ok, locate a number
            auto nbr {key.substr(it + names[i].length())};
            // Assert all substr is composed of digits
            auto no {nbr.find_first_not_of("0123456789")};
            if(no != std::string::npos)
                continue;
            // Then, we're good
            int index {};
            try
            {
                index = std::stoi(nbr);
            }
            catch(std::exception& e)
            {
                throw std::runtime_error("SilParameters::ReadActions(): could not locate index of silicon with name " +
                                         names[i]);
            }
            fVXI[vxi] = {layers[i], index};
            break;
        }
    }
}

std::pair<std::string, int> ActRoot::SilParameters::GetSilIndex(int vxi)
{
    auto where {fVXI.find(vxi) != fVXI.end()};
    if(where)
        return fVXI[vxi];
    else
        return {"", -1};
}
