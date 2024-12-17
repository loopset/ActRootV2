#include "ActModularParameters.h"

#include "TString.h"

#include <fstream>
#include <iostream>

std::string ActRoot::ModularParameters::GetName(int vxi)
{
    auto where {fVXI.find(vxi) != fVXI.end()};
    if(where)
        return fVXI[vxi];
    else
        return "";
}

void ActRoot::ModularParameters::ReadActions(const std::vector<std::string>& names, const std::string& file)
{
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("No Action file for ModularParameters");
    TString key {};
    int vxi {};
    int aux0 {};
    int aux1 {};
    while(streamer >> key >> vxi >> aux0 >> aux1)
    {
        for(int i = 0; i < names.size(); i++)
        {
            if(key == names[i])
            {
                fVXI[vxi] = names[i];
                break;
            }
        }
    }
}

void ActRoot::ModularParameters::Print() const
{
    std::cout << "==== ModularParameters ====" << '\n';
    for(const auto& [key, val] : fVXI)
    {
        std::cout << "-- VXI: " << key << " contains Modular " << val << '\n';
    }
    std::cout << "=======================" << '\n';
}
