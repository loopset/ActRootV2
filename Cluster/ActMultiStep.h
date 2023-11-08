#ifndef ActMultiStep_h
#define ActMultiStep_h

#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActTPCData.h"
#include "TStopwatch.h"

#include "Math/Point3Dfwd.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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
        // Pointer to RP vector
        std::map<std::pair<int, int>, XYZPoint>* fRPs {};
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
        double fTrackChi2Threshold;
        double fBeamWindowScaling;
        // 2-> Clean pileup of beams
        bool fEnableCleanPileUp;
        double fPileUpChangeZThreshold;
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
        // 6-> Determine RP and clean unreacted beam
        bool fEnableRP;
        double fBeamLikeXMinThresh;
        double fBeamLikeParallelF;
        double fRPDistThresh;
        double fRPMaskXY;
        double fRPMaskZ;
        std::vector<int> fAllowedMults;
        double fRPPivotDist;
        // 7-> Clean bad fits at the very begining of the algorithm
        bool fEnableCleanBadFits;
        // Vector of TStopwatch to asses performance of algorithm
        std::vector<TStopwatch> fClocks;
        std::vector<std::string> fCLabels;
        // Bool to set verbose mode
        bool fIsVerbose;

    public:
        MultiStep() = default;

        // Print method
        void Print() const;
        // Setters and getters
        void SetTPCParameters(ActRoot::TPCParameters* pars) { fTPC = pars; }
        void SetClusters(std::vector<ActCluster::Cluster>* clusters) { fClusters = clusters; }
        std::vector<ActCluster::Cluster>* GetClusters() const { return fClusters; }
        void SetClimb(std::shared_ptr<ActCluster::ClIMB> climb) { fClimb = climb; }
        void SetRPs(std::map<std::pair<int, int>, XYZPoint>* rps) { fRPs = rps; }
        std::map<std::pair<int, int>, XYZPoint>* GetRPs() const { return fRPs; }

        // Read config file
        void ReadConfigurationFile(const std::string& infile = "");

        // Print time reports
        void PrintClocks() const;

        // Main method
        void Run();
        // Erase bad fits: Chi2 = -1
        void CleanBadFits();
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
        // Find Reaction Point if it exists
        void FindRP();
        // Determine beam-like clusters
        void DetermineBeamLikes();
        // Delete clusters without valid RP
        void DeleteInvalidClusters();
        // Determine precise RP
        void DeterminePreciseRP();

    private:
        // Initialize clocks to measure execution time of each step
        void InitClocks();
        // Check if voxel is in cylindir using manually-given widths
        bool ManualIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, double scale = 1);
        // Do the same but using auto-determined widths
        template <typename T>
        bool
        AutoIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, T xBreak, T meanWidthY, T meanWidthZ, T offset = 2);
        // Use preliminary method to obtain a better aproximation to the RP
        std::tuple<XYZPoint, double, double> DetermineBreakPoint(ItType it);
        // Reset indexes of fClusters
        void ResetIndex();
        // Print during execution of each step
        void PrintStep() const;
        // Using matrix calculus, returns two closest point between two lines in 3D
        std::tuple<XYZPoint, XYZPoint, double> ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB);
        // Check if RP is valid according to ranges
        bool IsRPValid(const XYZPoint& rp);
        // Get -very- preliminary theta to rank RPs
        double GetThetaAngle(const XYZVector& dir);
    };
} // namespace ActCluster

#endif // !ActMultiStep_h
