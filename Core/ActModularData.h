#ifndef ActModularData_h
#define ActModularData_h

#include <string>
#include <unordered_map>
namespace ActRoot
{
    //! A class storing ModularLeaf data
    class ModularData
    {
    public:
        std::unordered_map<std::string, float> fLeaves; //!< Each leaf holds a float variable in a map

        void Clear(); //!< Reset data
        void Print() const; //!< Print stored data
    };
}

#endif
