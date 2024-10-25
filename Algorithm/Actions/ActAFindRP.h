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
    double fBeamLikeXMinThresh {}; // Min value in X that has to be in the cluster to be beam-like
    double fBeamLikeParallelF {};  // Min value for X component
    double fRPDistThresh {};       // Max distance between two lines to form a RP
    double fRPDistCluster {};      // Max distance to cluster RPs
    double fRPDistValidate {};     // Max distance from track to rp 
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
    std::vector<RPCluster> ClusterAndSortRPs(std::vector<RPValue> & rps);
};
} // namespace Actions
} // namespace ActAlgorithm


#endif