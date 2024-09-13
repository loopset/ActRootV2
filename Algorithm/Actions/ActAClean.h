#ifndef ActAClean_h
#define ActAClean_h

#include "ActInputParser.h"
#include "ActVAction.h"

#include <memory>

namespace ActAlgorithm
{
namespace Actions
{
class Clean : public VAction
{
public:
    Clean() : VAction("Clean") {};
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm
#endif
