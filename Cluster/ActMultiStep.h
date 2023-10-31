#ifndef ActMultiStep_h
#define ActMultiStep_h

#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActTPCData.h"

#include "Math/Point3Dfwd.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

// forward declaration
namespace ActRoot
{
    class TPCParameters;
}
namespace ActCluster
{
    class MultiStep
    {
    public:
        using ItType = std::vector<ActCluster::Cluster>::iterator;
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        // Pointer to TPC Parameters
        ActRoot::TPCParameters* fTPC {};
        // Pointer to vector to analyze
        std::vector<ActCluster::Cluster>* fClusters {};
        // Pointer to current ClIMB
        std::shared_ptr<ActCluster::ClIMB> fClimb {};
        // Settings of the different parts
        // General flag to control whether this is enabled or not
        bool fIsEnabled;
        // 1-> Break multi beam events
        bool fEnableBreakMultiBeam;
        bool fFitNotBeam;
        double fChi2Threshold;
        double fMinSpanX;
        double fLengthXToBreak;
        double fBeamWindowY;
        double fBeamWindowZ;
        int fBreakLengthThres;
        // 1.1-> Break multitrack clusters
        bool fEnableBreakMultiTracks;
        double fBeamWindowScaling;
        // 2-> Clean pileup of beams
        bool fEnableCleanPileUp;
        double fPileUpXPercent;
        double fBeamLowerZ;
        double fBeamUpperZ;
        // 3-> Clean vertical Z
        bool fEnableCleanZs;
        double fZDirectionThreshold;
        double fZMinSpanInPlane;
        // 4-> Merge similar tracks
        bool fEnableMerge;
        double fMergeMinParallelFactor;
        double fMergeChi2CoverageFactor;
        // 5-> Clean remaining clusters
        bool fEnableCleanDeltas;
        double fDeltaChi2Threshold;
        double fDeltaMaxVoxels;

    public:
        MultiStep() = default;

        // Print method
        void Print() const;
        // Setters and getters
        void SetTPCParameters(ActRoot::TPCParameters* pars) { fTPC = pars; }
        void SetClusters(std::vector<ActCluster::Cluster>* clusters) { fClusters = clusters; }
        std::vector<ActCluster::Cluster>* GetClusters() const { return fClusters; }
        void SetClimb(std::shared_ptr<ActCluster::ClIMB> climb) { fClimb = climb; }

        // Read config file
        void ReadConfigurationFile(const std::string& infile = "");

        // Main method
        void Run();
        // Initial clean of pileup beams
        void CleanPileup();
        // Cleaning of Zs
        void CleanZs();
        // Cleaning of remaining clusters: likely delta e-
        void CleanDeltas();
        // Break multibeam events
        void BreakBeamClusters();
        // Break multitrack events
        void BreakTrackClusters();
        // Merge quasialigned tracks which got broken due to non-continuity
        void MergeSimilarTracks();

    private:
        bool ManualIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, double scale = 1);
        template<typename T>
        bool AutoIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, T xBreak, T meanWidthY, T meanWidthZ, T offset = 2);
        std::tuple<XYZPoint, double, double> DetermineBreakPoint(ItType it);
        void ResetIndex();
        void PrintStep() const;
        void DetermineBeamLikes();
    };
} // namespace ActCluster

#endif // !ActMultiStep_h
