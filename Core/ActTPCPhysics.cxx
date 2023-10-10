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
    std::cout<<"===================="<<'\n';
}
