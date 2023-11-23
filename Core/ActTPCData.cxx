#include "ActTPCData.h"

#include <iostream>

void ActRoot::TPCData::Clear()
{
    fClusters.clear();
    fRPs.clear();
}

void ActRoot::TPCData::Print() const
{
    std::cout << "==== TPCData ====" << '\n';
    std::cout << "N of clusters = " << fClusters.size() << '\n';
    for(const auto& cluster : fClusters)
    {
        cluster.Print();
        cluster.GetLine().Print();
    }
    std::cout << "Reaction Points :" << '\n';
    int i {-1};
    for(const auto& rp : fRPs)
    {
        i++;
        std::cout << "  " << i << " : " << rp << '\n';
    }
    std::cout << "======================" << '\n';
}
