#ifndef ActMultiRegion_h
#define ActMultiRegion_h

#include "ActCluster.h"
#include "ActRegion.h"
#include "ActTPCDetector.h"
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

    // Pointer to TPCParameters
    ActRoot::TPCParameters* fTPC {};

    // Parameters of algorithm
    bool fIsEnabled {};
    // Cleaning of pileup
    bool fEnableCleanPileUp;
    double fPileUpXPercent;
    double fPileUpLowerZ;
    double fPileUpUpperZ;
    // Determine beam-likes
    double fBlXDirThresh {};
    double fBLXBegin {};
    // Merge of similar tracks
    bool fEnableMerge {};
    double fMergeDistThresh {};
    double fMergeMinParallel {};
    double fMergeChi2Factor {};
    // Cleaning of clusters
    bool fEnableClean {};
    double fCleanCylinderR {};
    int fCleanMinVoxels {};
    double fCleanMaxChi2 {};
    // RP
    double fRPMaxDist {};
    double fRPClusterDist {};
    bool fRPDelete {};
    double fRPPivotDist {};
    bool fRPOutsideBeam {};
    bool fRPEnableFine {};
    // Clean split RP
    bool fRPBreakAfter {};
    bool fKeepSplitRP {};
    // Enable final cleaning
    bool fEnableFinalClean {};
    double fCausalShiftX {};
    bool fEnableCleanMult1 {};

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
    void SetTPCParameters(ActRoot::TPCParameters* pars) { fTPC = pars; }

    // Getters
    const std::unordered_map<RegionType, Region> GetRegions() const { return fRegions; }
    ActRoot::TPCParameters* GetTPCParameters() const { return fTPC; }

private:
    void AddRegion(unsigned int i, const std::vector<double>& vec);
    void CheckRegions();
    void BreakIntoRegions();
    RegionType AssignRangeToRegion(ClusterIt it);
    RegionType AssignVoxelToRegion(const ActRoot::Voxel& v);
    bool BreakCluster(ClusterIt it, BrokenVoxels& broken);
    void ProcessNotBeam(BrokenVoxels& broken);
    void MergeClusters();
    void MarkBeamLikes();
    void CleanPileUp();
    void CleanClusters();
    void Sort();
    void FindRP();
    void DeleteAfterRP();
    void DoFinerFits();
    void FindFineRP();
    void FinalClean();
    void ResetID();
};
} // namespace ActAlgorithm

#endif
