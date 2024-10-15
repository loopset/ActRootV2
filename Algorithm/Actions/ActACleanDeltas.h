#ifndef ActACleanDeltas_h
#define ActACleanDeltas_h
#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class CleanDeltas : public VAction
{
private:
    double fDeltaChi2Threshold {}; // Chi2 threshold (bad fit)
    double fDeltaMaxVoxels {};     // Max amount of voxeles for a delta e- trace

public:
    CleanDeltas() : VAction("CleanDeltas") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif