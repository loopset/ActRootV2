#include "ActRegion.h"

#include "Math/Point3D.h"

#include <iostream>


void ActAlgorithm::Region::Print() const
{
    std::cout << "X  [" << fX.first << ", " << fX.second << "], Y [" << fY.first << ", " << fY.second << "]" << '\n';
}

bool ActAlgorithm::Region::IsInside(const ROOT::Math::XYZPointF& p) const
{
    return (fX.first <= p.X() && p.X() < fX.second) && (fY.first <= p.Y() && p.Y() < fY.second);
}

bool ActAlgorithm::Region::IsInside(const RangePair& x, const RangePair& y) const
{

    bool condX {(fX.first <= x.first && x.first < fX.second) && (fX.first <= x.second && x.second < fX.second)};
    bool condY {(fY.first <= y.first && y.first < fY.second) && (fY.first <= y.second && y.second < fY.second)};
    return condX && condY;
}

ActAlgorithm::Region::RegionPair ActAlgorithm::Region::GetCentre() const
{
    return {(fX.first + fX.second) / 2, (fY.first + fY.second) / 2};
}

void ActAlgorithm::Region::FillGraph(TGraph* g, TString proj, double minZ, double maxZ) const
{
    proj.ToLower();
    if(proj == "xy")
    {
        g->SetPoint(0, fX.first, fY.first);
        g->SetPoint(1, fX.first, fY.second);
        g->SetPoint(2, fX.second, fY.second);
        g->SetPoint(3, fX.second, fY.first);
        g->SetPoint(4, fX.first, fY.first);
    }
    if(proj == "xz")
    {
        g->SetPoint(0, fX.first, minZ);
        g->SetPoint(1, fX.first, maxZ);
        g->SetPoint(2, fX.second, maxZ);
        g->SetPoint(3, fX.second, minZ);
        g->SetPoint(4, fX.first, minZ);
    }
    if(proj == "yz")
    {
        g->SetPoint(0, fY.first, minZ);
        g->SetPoint(1, fY.first, maxZ);
        g->SetPoint(2, fY.second, maxZ);
        g->SetPoint(3, fY.second, minZ);
        g->SetPoint(4, fY.first, minZ);
    }
}
