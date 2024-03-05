#ifndef ActMultiRegion_h
#define ActMultiRegion_h

#include "ActCluster.h"
#include "ActMultiStep.h"
#include "ActRegion.h"
#include "ActVFilter.h"
#include "ActVoxel.h"

#include "TStopwatch.h"

#include <memory>
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

    // Pointer to MultiStep
    std::shared_ptr<ActAlgorithm::MultiStep> fMStep;

    // Parameters of algorithm
    bool fIsEnabled {};
    // Merge of similar tracks
    bool fEnableMerge {};
    double fMergeDistThresh {};
    double fMergeMinParallel {};
    double fMergeChi2Factor {};
    // RP
    double fRPMaxDist {};

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

    // Setters
    void SetMultiStep(std::shared_ptr<MultiStep> step) { fMStep = step; }

    // Getters
    std::shared_ptr<MultiStep> GetMultiStep() const { return fMStep; }

private:
    void AddRegion(unsigned int i, const std::vector<double>& vec);
    void BreakIntoRegions();
    RegionType AssignRangeToRegion(ClusterIt it);
    RegionType AssignVoxelToRegion(const ActRoot::Voxel& v);
    bool BreakCluster(ClusterIt it, BrokenVoxels& broken);
    void ProcessNotBeam(BrokenVoxels& broken);
    void MergeClusters();
    void Assign();
    void FindRP();
    void ResetIndex();
};
} // namespace ActAlgorithm

#endif
