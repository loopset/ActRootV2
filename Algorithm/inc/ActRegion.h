#ifndef ActRegion_h
#define ActRegion_h

#include "Math/Point3D.h"

#include <iostream>
#include <utility>
namespace ActAlgorithm
{

enum class RegionType
{
    EBeam,
    ELight,
    EHeavy,
    ENone
};

class Region
{
public:
    typedef std::pair<double, double> RegionPair;
    typedef std::pair<float, float> RangePair;

private:
    RegionPair fX {};
    RegionPair fY {};

public:
    Region() = default;
    Region(double xmin, double xmax, double ymin, double ymax) : fX(xmin, xmax), fY(ymin, ymax) {}

    // Getters
    const RegionPair& GetX() const { return fX; }
    const RegionPair& GetY() const { return fY; }

    // Other functions
    inline void Print() const
    {
        std::cout << "X in [" << fX.first << ", " << fX.second << "], Y in [" << fY.first << ", " << fY.second << "]"
                  << '\n';
    }

    inline bool IsInside(const ROOT::Math::XYZPointF& p) const
    {
        return (fX.first <= p.X() && p.X() < fX.second) && (fY.first <= p.Y() && p.Y() < fY.second);
    }

    inline bool IsInside(const RangePair& x, const RangePair& y) const
    {
        bool condX {(fX.first <= x.first && x.first < fX.second) && (fX.first <= x.second && x.second < fX.second)};
        bool condY {(fY.first <= y.first && y.first < fY.second) && (fY.first <= y.second && y.second < fY.second)};
        return condX && condY;
    }

    inline RegionPair GetCentre() const { return {(fX.first + fX.second) / 2, (fY.first + fY.second) / 2}; }
};
} // namespace ActAlgorithm

#endif // !ActRegion_h