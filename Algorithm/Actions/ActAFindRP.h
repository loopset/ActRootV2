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
    double fRPPivotDist {}; // Distance to erase Init and End of tracks (if voxels projection on line is too close of
                            // first/last voxel projection)
    bool fEnableRPDefaultBeam {}; // Enable set direction to short BLs tracks as (1,0,0)
    double fRPDefaultMinX {};     // Parameter that determines if a BL track is short in x
    bool fEnableFineRP {};

public:
    FindRP() : VAction("FindRP") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    // Major functions of FindRP
    void DetermineBeamLikes();
    void FindPreliminaryRP();
    void DeleteInvalidCluster();
    void PerformFinerFits();
    void BreakBeamToHeavy(const ActAlgorithm::VAction::XYZPointF& rp, bool keepSplit = true);
    void CylinderCleaning();
    void MaskBeginEnd(const ActAlgorithm::VAction::XYZPointF& rp);
    void FindPreciseRP();

    // Auxiliary functions
    std::tuple<ActAlgorithm::VAction::XYZPointF, ActAlgorithm::VAction::XYZPointF, double>
    ComputeRPIn3D(ActPhysics::Line::XYZPoint pA, ActPhysics::Line::XYZVector vA, ActPhysics::Line::XYZPoint pB,
                  ActPhysics::Line::XYZVector vB);

    bool IsRPValid(const XYZPointF& rp, ActRoot::TPCParameters* tpc);

    typedef std::pair<ActAlgorithm::VAction::XYZPointF, std::pair<int, int>> RPValue;
    typedef std::pair<ActAlgorithm::VAction::XYZPointF, std::set<int>> RPCluster;
    std::vector<RPCluster> ClusterAndSortRPs(std::vector<RPValue>& rps);

    double GetClusterAngle(const ActPhysics::Line::XYZVector& beam, const ActPhysics::Line::XYZVector& recoil);
};
} // namespace Actions
} // namespace ActAlgorithm


#endif
