#ifndef ActSilData_h
#define ActSilData_h

#include "ActVData.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ActRoot
{
    //! A class holding simple Silicon data
    class SilData : public VData
    {
    public:
        std::unordered_map<std::string, std::vector<double>> fSiE; //!< Calibrated silicon energy
        std::unordered_map<std::string, std::vector<int>> fSiN; //!< Silicon number
        SilData() = default;

        void Clear() override; //!< Reset stored variables in SilData 
        void Print() const override; //!< Print calibrated silicon data
    };
}

#endif
