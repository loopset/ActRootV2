#ifndef ActMergerParameters_h
#define ActMergerParameters_h
#include "ActVParameters.h"

namespace ActRoot
{
class MergerParameters : public VParameters
{
public:
    // Just flags setting event-by-event merger settings
    bool fUseRP {};
    bool fIsL1 {};
    bool fIsCal {};

    void Print() const override;
};

} // namespace ActRoot
#endif
