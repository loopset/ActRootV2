#include "ActTPCData.h"
#include <ios>
#include <iostream>

ActRoot::Voxel::Voxel(const XYZPointI& pos, float charge, bool hasSaturation)
    : fPosition(pos)
    , fCharge(charge)
    , fIsSaturated(hasSaturation)
{
}

// ActRoot::Voxel::Voxel(int id, const XYZPoint& pos, float charge, bool hasSaturation)
//     : fID(id), fPosition(pos), fCharge(charge), fIsSaturated(hasSaturation)
// {}

void ActRoot::Voxel::Print() const
{
    std::cout << "== Voxel ==" << '\n';
    std::cout << " -> Position  = " << fPosition << '\n';
    std::cout << " -> Charge    = " << fCharge << '\n';
    std::cout << " -> Has sat ? = " << std::boolalpha << fIsSaturated << '\n';
}

void ActRoot::TPCData::Clear() { fVoxels.clear(); }

void ActRoot::TPCData::Print() const
{
    std::cout << "==== TPCData ====" << '\n';
    std::cout << "-> Voxels size = " << fVoxels.size() << '\n';
    for(const auto& voxel : fVoxels)
        voxel.Print();
}
