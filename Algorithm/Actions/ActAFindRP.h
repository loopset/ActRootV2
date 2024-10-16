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
public:
    FindRP() : VAction("FindRP") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
    void DetermineBeamLikes();
    void FindPreliminaryRP();
};
} // namespace Actions
} // namespace ActAlgorithm


#endif