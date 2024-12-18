#ifndef ActRegion_h
#define ActRegion_h

#include "Rtypes.h"

#include "TGraph.h"

#include "Math/Point3Dfwd.h"

#include <string>
#include <unordered_map>
#include <utility>


namespace ActRoot
{
enum class RegionType
{
    EBeam,
    ELight,
    EHeavy,
    ENone
};

inline const std::unordered_map<RegionType, std::string> kRegionTypeAsStr {{RegionType::EBeam, "Beam"},
                                                                           {RegionType::ELight, "Light"},
                                                                           {RegionType::EHeavy, "Heavy"},
                                                                           {RegionType::ENone, "None"}};

inline std::string RegionTypeAsStr(RegionType type)
{
    if(auto it {kRegionTypeAsStr.find(type)}; it != kRegionTypeAsStr.end())
        return it->second;
    return "";
}

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
    void Print() const;

    bool IsInside(const ROOT::Math::XYZPointF& p) const;

    bool IsInside(const RangePair& x, const RangePair& y) const;

    RegionPair GetCentre() const;

    void FillGraph(TGraph* g, TString proj, double minZ = 0, double maxZ = 0) const;

    ClassDef(Region, 1);
};
} // namespace ActRoot

#endif // !ActRegion_h
