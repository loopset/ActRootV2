#ifndef ActACleanZs_h
#define ActACleanZs_h
#include "ActVAction.h"

namespace ActAlgorithm
{
namespace Actions
{
class CleanZs : public VAction
{
private:
    double fZDirectionThreshold {};
    double fMaxSpanInPlane {};

public:
    CleanZs() : VAction("CleanZs") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock>) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif