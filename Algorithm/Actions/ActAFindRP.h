#ifndef ActAFindRP_h
#define ActAFindRP_h
#include "ActVAction.h"

#include <set>

namespace ActAlgorithm
{
namespace Actions
{
class FindRP : public VAction
{
private:
    bool fUseExtVoxels {};               //!< Whether to use or not unrebinned Z coordinate in FITS only
    double fBeamLikeXMinThresh {};       // Min value in X that has to be in the cluster to be beam-like
    double fBeamLikeParallelF {};        // Min value for X component
    double fRPDistThresh {};             // Max distance between two lines to form a RP
    double fRPDistCluster {};            // Max distance to cluster RPs
    bool fEnableDeleteInvalidCluster {}; // Bool to enable the function DeleteInvalidCluster
    double fBeamLikeMinVoxels {};        // Min voxels for BL particles
    double fRPMaskXY {};                 // Distance to aply the mask in XY
    double fRPMaskZ {};                  // Distance to aply the mask in Z
    bool fEnableRPDefaultBeam {};        // Enable set direction to short BLs tracks as (1,0,0)
    double fRPDefaultMinX {};            // Parameter that determines if a BL track is short in x
    bool fEnableFineRP {};
    bool fKeepBreakBeam {};      //!< Keep heavy-like cluster after breaking beam starting on RP
    double fMinXSepBreakBeam {}; //!< Min separation preRP.X() to BeamLike.Xend() to separete heavy track
    bool fEnableFixBreakBeam {};
    double fMaxVoxelsFixBreak {};  //!< Max voxels to enable fix voxel routine
    double fMinPercentFixBreak {}; //!< Min percent of voxels inside the bigger one to fix
    bool fEnableCylinder {};       //!< Bool to enable the function CylinderCleaning
    double fCylinderR {};          //!< Cylinder radious for CylinderCleaning function
    double fRPPivotDist {}; //!< Distance to erase Init and End of tracks (if voxels projection on line is too close of
                            // first/last voxel projection)

public:
    // Class to store RPOperations
    struct RPOps
    {
        XYZPointF fRP {};       //!< RP (maybe a mean of RPs after clustering)
        std::set<int> fIdxs {}; //!< Indexes of clusters belonging to RP
        double fMinDist {};     //!< Minimum distance of a cluster to this RP
        int fNrp {};            //!< Number of RPs that contributed to this Cluster of RPs
        int fBLIdx {};          //!< Index of BL cluster this RP belongs to

        void Print() const;
    };

public:
    FindRP() : VAction("FindRP") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    // Major functions of FindRP
    void SetExtVoxels();
    void DetermineBeamLikes();
    bool IsDoable();
    void FindPreliminaryRP();
    void DeleteInvalidCluster();
    void PerformFinerFits();
    bool BreakBeamToHeavy(const ActAlgorithm::VAction::XYZPointF& rp, bool keepSplit = true);
    void CylinderCleaning();
    void MaskAroundRP(const XYZPointF& rp, bool blHasBroken);
    void MaskBeginEnd(const ActAlgorithm::VAction::XYZPointF& rp);
    void FindPreciseRP();

    // Auxiliary functions
    std::tuple<ActAlgorithm::VAction::XYZPointF, ActAlgorithm::VAction::XYZPointF, double>
    ComputeRPIn3D(ActPhysics::Line::XYZPointF pA, ActPhysics::Line::XYZVectorF vA, ActPhysics::Line::XYZPointF pB,
                  ActPhysics::Line::XYZVectorF vB);

    bool IsRPValid(const XYZPointF& rp, ActRoot::TPCParameters* tpc);

    std::vector<RPOps> ClusterAndSortRPs(std::vector<RPOps>& rps);

    double GetClusterAngle(const ActPhysics::Line::XYZVectorF& beam, const ActPhysics::Line::XYZVectorF& recoil);
};
} // namespace Actions
} // namespace ActAlgorithm


#endif
