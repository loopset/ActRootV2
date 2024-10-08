#ifndef ActACleanPileUp_h
#define ActACleanPileUp_h
#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class CleanPileUp : public VAction
{
private:
    double fXPercent {}; //!< Min X percent to be considered pileup
    double fLowerZ {};   //!< Lower Z frontier
    double fUpperZ {};   //!< Upper Z frontier
public:
    CleanPileUp() : VAction("CleanPileUp") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
