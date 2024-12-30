#include "ActTPCData.h"

#include <iostream>

void ActRoot::TPCData::Clear()
{
    fClusters.clear();
    fRaw.clear();
    fRPs.clear();
}

void ActRoot::TPCData::ClearFilter()
{
    // WARNING: This function must set to default values variables
    // that are not written to disk, otherwise UNDEFINED BEAHAVIOUR arises
    // Useful only for Filter tasks, hence the name
    // As of December 24, it is only needed for TPCData in
    // UseExtVoxels and IsDefault Cluster members
    for(auto it = fClusters.begin(); it != fClusters.end(); it++)
    {
        it->SetUseExtVoxels(false); // values must follow default ones in class def
        it->SetIsDefault(false);
    }
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
    std::cout << "Noise or Raw voxels size = " << fRaw.size() << '\n';
    std::cout << "Reaction Points :" << '\n';
    int i {-1};
    for(const auto& rp : fRPs)
    {
        i++;
        std::cout << "  " << i << " : " << rp << '\n';
    }
    std::cout << "======================" << '\n';
}
