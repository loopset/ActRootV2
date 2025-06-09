#ifndef ActAMultiregion_h
#define ActAMultiregion_h

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

    int fymin {};                //!< Beam region y min
    int fymax {};                //!< Beam region y max
    int fxmin {};                //!< Beam region x min
    int fxmax {};                //!< Beam region x max
    int fMinVoxelsAfterBreak {}; //!< Minimum number of voxels after breaking into regions

    std::shared_ptr<ActAlgorithm::ClIMB> fClimb {}; // ClIMB continuity algorithm object

public:
    MultiRegion() : VAction("MultiRegion") {};
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    void BreakIntoRegions();
    bool CheckClusterIsInRegion(ActRoot::Cluster& cluster, const ActRoot::Region& region);
    bool BreakCluster(ActRoot::Cluster& cluster, BrokenVoxels& brokenVoxels, ActRoot::Region& region);
    ActRoot::RegionType AssignVoxelToRegion(const ActRoot::Voxel& voxel, const ActRoot::Region& region);
    void ProcessNotBeam(BrokenVoxels& brokenVoxels, const ActRoot::Region& region);
    void ResetID();
};
} // namespace Actions
} // namespace ActAlgorithm
#endif