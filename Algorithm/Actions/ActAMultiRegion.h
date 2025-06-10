#ifndef ActAMultiRegion_h
#define ActAMultiRegion_h

#include "ActClIMB.h"
#include "ActInputParser.h"
#include "ActVAction.h"

#include <memory>

namespace ActAlgorithm
{
namespace Actions
{
class MultiRegion : public VAction
{
public:
    using BrokenVoxels = std::vector<std::vector<ActRoot::Voxel>>; //!< Voxels broken into regions
private:
    std::unordered_map<ActRoot::RegionType, ActRoot::Region> fRegions;
    int fMinVoxelsAfterBreak {}; //!< Minimum number of voxels after breaking into regions

    std::shared_ptr<ActAlgorithm::ClIMB> fClimb {}; // ClIMB continuity algorithm object

public:
    MultiRegion() : VAction("MultiRegion") {};
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    void AddRegion(unsigned int i, const std::vector<double>& vec);
    void CheckRegionsReadout();
    void BreakIntoRegions();
    ActRoot::RegionType AssignClusterToRegion(ActRoot::Cluster& cluster);
    bool BreakCluster(ActRoot::Cluster& cluster, BrokenVoxels& brokenVoxels);
    ActRoot::RegionType AssignVoxelToRegion(const ActRoot::Voxel& voxel);
    void ProcessNotBeam(BrokenVoxels& brokenVoxels);
    void ResetID();
};
} // namespace Actions
} // namespace ActAlgorithm
#endif