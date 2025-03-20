#ifndef ActASplit_h
#define ActASplit_h

#include "ActVAction.h"

// forward declarations
namespace ActRoot
{
class Cluster;
}

namespace ActAlgorithm
{
namespace Actions
{
class Split : public VAction
{
private:
public:
    Split() : VAction("Split") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override {};

private:
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
