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
    double fZDirectionThreshold {}; // Min Z direction component
    double fMaxSpanInPlane {};      // Max width of the cluster in X or Y

public:
    CleanZs() : VAction("CleanZs") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock>) override;
    void Run() override;
    void Print() const override;
};
} // namespace Actions
} // namespace ActAlgorithm

#endif