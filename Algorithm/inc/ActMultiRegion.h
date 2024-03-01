#ifndef ActMultiRegion_h
#define ActMultiRegion_h

#include "ActRegion.h"
#include "ActVFilter.h"

#include <unordered_map>

namespace ActAlgorithm
{
class MultiRegion : public VFilter
{
private:
    std::unordered_map<RegionType, Region> fRegions;

public:
    MultiRegion();
    ~MultiRegion() override = default;

    // Override virtual methods
    void ReadConfiguration() override;

    void Run() override;

    void Print() const override;

    void PrintReports() const override;

private:
    void AddRegion(unsigned int i, const std::vector<double>& vec);
    std::string RegionTypeToStr(const RegionType& r) const;
};
} // namespace ActAlgorithm

#endif
