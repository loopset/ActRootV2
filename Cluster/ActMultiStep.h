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

namespace ActCluster
{
    class MultiStep
    {
    public:
        using ItType = std::vector<ActCluster::Cluster>::iterator;
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        // Pointer to vector to analyze
        std::vector<ActCluster::Cluster>* fClusters {};
        // Pointer to current ClIMB
        std::shared_ptr<ActCluster::ClIMB> fClimb {};
        // Settings of the different parts
        // General flag to control whether this is enabled or not
        bool fIsEnabled;
        // 1-> Break multi beam events
        bool fFitNotBeam;
        double fChi2Threshold;
        double fEntranceBeamRegionX;
        double fMinSpanX;
        double fLengthXToBreak;
        double fBeamWindowY;
        double fBeamWindowZ;

    public:
        MultiStep() = default;

        // Print method
        void Print() const;
        // Setters and getters
        void SetClusters(std::vector<ActCluster::Cluster>* clusters) { fClusters = clusters; }
        std::vector<ActCluster::Cluster>* GetClusters() const { return fClusters; }
        void SetClimb(std::shared_ptr<ActCluster::ClIMB> climb) { fClimb = climb; }

        // Read config file
        void ReadConfigurationFile(const std::string& infile = "");

        // Break multibeam events
        void RunBreakBeamClusters();

    private:
        bool IsInBeamCylinder(const XYZPoint& pos, const XYZPoint& gravity);
    };
} // namespace ActCluster

#endif // !ActMultiStep_h
