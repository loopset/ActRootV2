#ifndef ActSilData_h
#define ActSilData_h

#include <string>
#include <unordered_map>
#include <vector>
namespace ActRoot
{
    //! A class holding simple Silicon data
    class SilData
    {
    public:
        std::unordered_map<std::string, std::vector<float>> fSiE; //!< Calibrated silicon energy

        SilData() = default;

        void Clear(); //!< Reset stored variables in SilData 
        void Print() const; //!< Print calibrated silicon data
    };
}

#endif
