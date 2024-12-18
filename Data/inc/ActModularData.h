#ifndef ActModularData_h
#define ActModularData_h

#include "ActVData.h"

#include "Rtypes.h"

#include <string>
#include <unordered_map>

namespace ActRoot
{
//! A class storing ModularLeaf data
class ModularData : public VData
{
public:
    std::unordered_map<std::string, float> fLeaves; //!< Each leaf holds a float variable in a map

    float Get(const std::string& leave) { return fLeaves[leave]; }

    void Clear() override;       //!< Reset data
    void Print() const override; //!< Print stored data

    ClassDefOverride(ModularData, 1);
};
} // namespace ActRoot

#endif
