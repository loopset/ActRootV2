#ifndef ActMultiRegion_h
#define ActMultiRegion_h

#include "ActCluster.h"
#include "ActRegion.h"
#include "ActVFilter.h"
#include "ActVoxel.h"

#include "TStopwatch.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ActAlgorithm
{
class MultiRegion : public VFilter
{
public:
    typedef std::vector<ActRoot::Cluster>::iterator ClusterIt;
    typedef std::vector<ClusterIt> ClusterItVec;
    typedef std::vector<std::vector<ActRoot::Voxel>> BrokenVoxels;

private:
    std::unordered_map<RegionType, Region> fRegions;
    std::unordered_map<RegionType, ClusterItVec> fAssign;

    // Time control
    std::vector<TStopwatch> fClocks;
    std::vector<std::string> fClockLabels;

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
    void BreakIntoRegions();
    RegionType AssignRangeToRegion(ClusterIt it);
    RegionType AssignVoxelToRegion(const ActRoot::Voxel& v);
    bool BreakCluster(ClusterIt it, BrokenVoxels& broken, std::vector<RegionType>& assigments);
    void ProcessNotBeam(BrokenVoxels& broken, std::vector<RegionType>& assigments);
    void FindRP();
    void ResetIndex();
};
} // namespace ActAlgorithm

#endif
