#ifndef ActAClean_h
#define ActAClean_h

#include "ActInputParser.h"
#include "ActVAction.h"

#include <memory>

namespace ActAlgorithm
{
namespace Actions
{
class MultiRegion : public VAction
{
public:
    MultiRegion() : VAction("MultiRegion") {};
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm
#endif