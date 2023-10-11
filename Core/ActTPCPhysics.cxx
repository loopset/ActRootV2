#include "ActTPCPhysics.h"
#include <iostream>

void ActRoot::TPCPhysics::Clear()
{
    fClusters.clear();
}

void ActRoot::TPCPhysics::Print() const
{
    std::cout<<"==== TPCPhysics ===="<<'\n';
    std::cout<<"N clusters = "<<fClusters.size()<<'\n';
    for(const auto& cluster : fClusters)
        std::cout<<"  Cluster n "<<cluster.GetClusterID()<<" with "<<cluster.GetVoxels().size()<<" voxels"<<'\n';   
    std::cout<<"===================="<<'\n';
}
