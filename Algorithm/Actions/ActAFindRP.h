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
public:
    FindRP() : VAction("FindRP") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
    void DetermineBeamLikes();
};
} // namespace Actions
} // namespace ActAlgorithm


#endif