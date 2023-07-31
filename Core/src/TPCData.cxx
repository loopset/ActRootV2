#include "TPCData.h"
#include <ios>
#include <iostream>

ActRoot::Voxel::Voxel(int id, const XYZPoint& pos, float charge, bool hasSaturation)
    : fID(id), fPosition(pos), fCharge(charge), fIsSaturated(hasSaturation)
{}

void ActRoot::Voxel::Print() const
{
    std::cout<<"== ActVoxel id "<<fID<<" =="<<'\n';
    std::cout<<" -> Position  = "<<fPosition<<'\n';
    std::cout<<" -> Charge    = "<<fCharge<<'\n';
    std::cout<<" -> Has sat ? = "<<std::boolalpha<<fIsSaturated<<'\n';
}

void ActRoot::TPCData::Clear()
{
    fVoxels.clear();
}

void ActRoot::TPCData::Print() const
{
    std::cout<<"==== ActTPCData size = "<<fVoxels.size()<<" ===="<<'\n';
    for(const auto& voxel : fVoxels)
        voxel.Print();
}