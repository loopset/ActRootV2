#include "ActTPCPhysics.h"
#include <iostream>

void ActRoot::TPCPhysics::Clear()
{
    fClusters.clear();
}

void ActRoot::TPCPhysics::Print() const
{
    std::cout<<"==== TPCPhysics ===="<<'\n';
    std::cout<<"N of clusters = "<<fClusters.size()<<'\n';
    for(const auto& cluster : fClusters)
    {
        cluster.Print();
        cluster.GetLine().Print();
    }  
    std::cout<<"======================"<<'\n';
}