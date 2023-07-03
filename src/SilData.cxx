#include "SilData.h"
#include <iostream>

void ActRoot::SilData::Clear()
{
    fSiE.clear();
}

void ActRoot::SilData::Print() const
{
    for(const auto& [key, vec] : fSiE)
    {
        std::cout<<"==== SilData ===="<<'\n';
        std::cout<<"-- Layer "<<key<<'\n';
        for(int i = 0; i < vec.size(); i++)
        {
            std::cout<<"Idx = "<<i<<" Val = "<<vec[i]<<" MeV"<<'\n';
        }
    }
}
