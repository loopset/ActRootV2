#include "ActVoxel.h"

ActRoot::Voxel::Voxel(const XYZPoint& pos, float charge, bool hasSaturation)
    : fPosition(pos),
      fCharge(charge),
      fIsSaturated(hasSaturation)
{
}

void ActRoot::Voxel::Print() const
{
    std::cout << "== Voxel ==" << '\n';
    std::cout << " -> Position  = " << fPosition << '\n';
    std::cout << " -> Charge    = " << fCharge << '\n';
    std::cout << " -> Has sat ? = " << std::boolalpha << fIsSaturated << '\n';
}
