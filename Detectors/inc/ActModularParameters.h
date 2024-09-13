#ifndef ActModularParameters_h
#define ActModularParameters_h

#include "ActVParameters.h"

#include <map>
#include <string>
#include <vector>

namespace ActRoot
{
//! A class holding ModularLeaf experimental setups: VXI equivalences
class ModularParameters : public VParameters
{
private:
    std::map<int, std::string> fVXI;

public:
    std::string GetName(int vxi); //!< Get name of ModularLeaf according to Action file
    void ReadActions(const std::vector<std::string>& names,
                     const std::string& file); //!< Read Action file
    void Print() const override;
};
} // namespace ActRoot
#endif
