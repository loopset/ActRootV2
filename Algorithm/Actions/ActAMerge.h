#ifndef ActAMerge_h
#define ActAMerge_h

#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class Merge : public VAction
{
private:
    // Parameters of this action
    double fDistThresh {}; // Min distance of cluster line to others cluster line gravity point
    double fMinParallelFactor {}; // Min parallelity between 2 clusters to consider merging
    double fChi2Factor {};

public:
    Merge() : VAction("Merge") {};
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif