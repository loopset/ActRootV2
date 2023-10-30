#ifndef ActModularData_h
#define ActModularData_h

#include "ActVData.h"

#include <string>
#include <unordered_map>

namespace ActRoot
{
    //! A class storing ModularLeaf data
    class ModularData : public VData
    {
    public:
        std::unordered_map<std::string, double> fLeaves; //!< Each leaf holds a double variable in a map

        void Clear() override; //!< Reset data
        void Print() const override; //!< Print stored data
    };
}

#endif
