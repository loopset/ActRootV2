#ifndef ActAFindRP_h
#define ActAFindRP_h
#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class FindRP : public VAction
{
private:
    double fBeamLikeXMinThresh {};       // Min value in X that has to be in the cluster to be beam-like
    double fBeamLikeParallelF {};        // Min value for X component
    double fRPDistThresh {};             // Max distance between two lines to form a RP
    double fRPDistCluster {};            // Max distance to cluster RPs
    double fRPDistValidate {};           // Max distance from track to rp
    bool fEnableDeleteInvalidCluster {}; // Bool to enable the function DeleteInvalidCluster
    double fBeamLikeMinVoxels {};        // Min voxels for BL particles
    double fRPMaskXY {};                 // Distance to aply the mask in XY
    double fRPMaskZ {};                  // Distance to aply the mask in Z
    bool fEnableCylinder {};             // Bool to enable the function CylinderCleaning
    double fCylinderR {};                // Cylinder radious for CylinderCleaning function
public:
    FindRP() : VAction("FindRP") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    typedef std::pair<ActAlgorithm::VAction::XYZPoint, std::pair<int, int>> RPValue;
    typedef std::pair<ActAlgorithm::VAction::XYZPoint, std::set<int>> RPCluster;
    void DetermineBeamLikes();
    void FindPreliminaryRP();
    std::tuple<ActAlgorithm::VAction::XYZPoint, ActAlgorithm::VAction::XYZPoint, double>
    ComputeRPIn3D(ActPhysics::Line::XYZPoint pA, ActPhysics::Line::XYZVector vA, ActPhysics::Line::XYZPoint pB,
                  ActPhysics::Line::XYZVector vB);
    bool IsRPValid(const XYZPoint& rp, ActRoot::TPCParameters* tpc);
    std::vector<RPCluster> ClusterAndSortRPs(std::vector<RPValue>& rps);
    void DeleteInvalidCluster();
    void PerformFinerFits();
    void BreakBeamToHeavy(std::vector<ActRoot::Cluster>& clusters, const ActRoot::TPCData::XYZPoint& rp, int minVoxels,
                          bool keepSplit = true, bool isVerbose = false);
    void
    CylinderCleaning(std::vector<ActRoot::Cluster>& clusters, double cylinderR, int minVoxels, bool isVerbose = false);
    void MaskBeginEnd(std::vector<ActRoot::Cluster>& clusters, const ActRoot::TPCData::XYZPoint rp, double pivotDist,
                      int minVoxels, bool isVerbose = false);
};
} // namespace Actions
} // namespace ActAlgorithm


#endif