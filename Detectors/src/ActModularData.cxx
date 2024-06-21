#include "ActModularData.h"
#include <iostream>

void ActRoot::ModularData::Clear()
{
    fLeaves.clear();
}

void ActRoot::ModularData::Print() const
{
    std::cout<<"===== ModularData ===="<<'\n';
    for(const auto& [key, val] : fLeaves)
    {
        std::cout<<"-> Key: "<<key<<" Val: "<<val<<'\n';
    }
    std::cout<<"===================="<<'\n';
}
