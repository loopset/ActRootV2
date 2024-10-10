#ifndef ActACleanBadFits_h
#define ActACleanBadFits_h
#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class CleanBadFits : public VAction
{
private:
public:
    CleanBadFits() : VAction("CleanBadFits") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif