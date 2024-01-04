#include "ActUtils.h"

bool ActRoot::IsEqZero(int val)
{
    return val == 0;
}

bool ActRoot::IsEqZero(float val)
{
    return val * val <= kFloatEps * kFloatEps;
}

bool ActRoot::IsEqZero(double val)
{
    return val * val <= kDoubleEps * kDoubleEps;
}
