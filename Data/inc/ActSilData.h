#ifndef ActSilData_h
#define ActSilData_h

#include "ActVData.h"
#include "Rtypes.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// forward declaration
namespace ActPhysics
{
class SilSpecs;
}

namespace ActRoot
{
//! A class holding simple Silicon data
class SilData : public VData
{
public:
    std::unordered_map<std::string, std::vector<float>> fSiE; //!< Calibrated silicon energy
    std::unordered_map<std::string, std::vector<int>> fSiN;   //!< Silicon number
    SilData() = default;

    std::vector<std::string> GetLayers() const;
    void ApplyFinerThresholds(std::shared_ptr<ActPhysics::SilSpecs> specs);
    int GetMult(const std::string& key) { return fSiN[key].size(); }

    void Clear() override;       //!< Reset stored variables in SilData
    void Print() const override; //!< Print calibrated silicon data

    ClassDefOverride(SilData, 1);
};
} // namespace ActRoot

#endif
