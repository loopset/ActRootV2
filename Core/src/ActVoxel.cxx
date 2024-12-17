#include "ActVoxel.h"

#include <cmath>
#include <vector>

ActRoot::Voxel::Voxel(const XYZPointF& pos, float charge, bool hasSaturation)
    : fPosition(pos),
      fCharge(charge),
      fIsSaturated(hasSaturation)
{
}

void ActRoot::Voxel::Print() const
{
    std::cout << "===== Voxel =====" << '\n';
    std::cout << " -> Position    : " << fPosition << '\n';
    std::cout << " -> Charge      : " << fCharge << '\n';
    std::cout << " -> Has sat ?   : " << std::boolalpha << fIsSaturated << '\n';
    std::cout << " -> Z content   : " << fZs.size() << '\n';
}

std::vector<ActRoot::Voxel> ActRoot::Voxel::GetExtended() const
{
    std::vector<ActRoot::Voxel> ret;
    auto q {fCharge / fZs.size()}; // equally distributed charge
    // With offset already corrected
    for(const auto& z : fZs)
        ret.push_back(Voxel {{fPosition.X() + 0.5f, fPosition.Y() + 0.5f, RecoverFloat(fPosition.Z(), z)}, q});
    return ret;
}

ActRoot::Voxel::FractionalZ ActRoot::Voxel::ExtractDecimalPart(double v)
{
    double i {};
    double d {};
    d = std::modf(v, &i);
    return static_cast<int>(std::round(d * 1e4));
}

float ActRoot::Voxel::RecoverFloat(float integer, FractionalZ decimal)
{
    return integer + (float)decimal / 1e4;
}
