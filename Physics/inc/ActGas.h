#ifndef ActGas_h
#define ActGas_h

#include "ActInputParser.h"

#include <memory>
namespace ActPhysics
{
// A class containing essential parameters of a gas
class Gas
{
private:
    double fWorkFunc {};
    double fVDrift {};
    double fTransDiff {};

public:
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
    void ReadConfiguration(const std::string& file);

    // Getters
    double GetWork() const { return fWorkFunc; };
    double GetVDrift() const { return fVDrift; }
    double GetTransDiff() const { return fTransDiff; }

    void Print() const;
};
} // namespace ActPhysics

#endif
