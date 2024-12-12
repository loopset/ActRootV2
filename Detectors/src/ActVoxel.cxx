#include "ActVoxel.h"

#include <utility>
#include <vector>

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
    std::cout << " -> Bin size  = " << fZs.size() << '\n';
}

std::vector<ActRoot::Voxel> ActRoot::Voxel::GetBinVoxels(int rebin) const
{
    std::vector<Voxel> ret;
    for(const auto& z : fZs)
        ret.push_back({{fPosition.X(), fPosition.Y(), fPosition.Z() * rebin + z}, fCharge / fZs.size()});
    return std::move(ret);
}
