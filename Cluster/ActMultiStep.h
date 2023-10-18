#ifndef ActMultiStep_h
#define ActMultiStep_h

#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActTPCData.h"
#include "Math/Point3Dfwd.h"

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
        //Pointer to vector to analyze
        std::vector<ActCluster::Cluster>* fClusters {};
        //Pointer to current ClIMB
        std::shared_ptr<ActCluster::ClIMB> fClimb {};
        // Settings of the different parts
        // 1-> Break multi beam events
        double fChi2Threshold;
        double fEntranceBeamRegionX;
        double fBeamWindowY;
        double fBeamWindowZ;

    public:
        MultiStep() = default;

        //Print method
        void Print() const;
        // Setters and getters
        void SetClusters(std::vector<ActCluster::Cluster>* clusters) { fClusters = clusters; }
        std::vector<ActCluster::Cluster>* GetClusters() const { return fClusters; }
        void SetClimb(std::shared_ptr<ActCluster::ClIMB> climb){fClimb = climb;}

        // Read config file
        void ReadConfigurationFile(const std::string& infile = "");

        // Break multibeam events
        void RunBreakBeamClusters();

    private:
        std::vector<ActRoot::Voxel> SeparateBeamRegion(ItType cluster);
        void ClusterNotBeamRegion(const std::vector<ActRoot::Voxel>& voxels);
        inline bool IsInBeamCylinder(const XYZPoint& pos, const XYZPoint& gravity)
        {
            bool condY {(gravity.Y() - fBeamWindowY) <= pos.Y() && pos.Y() <= (gravity.Y() + fBeamWindowY)};
            bool condZ {(gravity.Z() - fBeamWindowZ) <= pos.Z() && pos.Y() <= (gravity.Z() + fBeamWindowZ)};
            
            return condY && condZ;
        }
    };
} // namespace ActCluster

#endif // !ActMultiStep_h
