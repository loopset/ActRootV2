#ifndef ActDecayGenerator_h
#define ActDecayGenerator_h
#include "ActParticle.h"

#include "TGenPhaseSpace.h"
#include "TLorentzVector.h"

#include <string>
#include <utility>
#include <vector>
namespace ActSim
{
class DecayGenerator
{
private:
    // Store particles
    ActPhysics::Particle fParticle {};              //!< Initial system
    double fInitialMass {};                         //!< Mass of initial system WITHOUT binding energy
    std::vector<ActPhysics::Particle> fProducts {}; //!< Product particles
    std::vector<double> fFinalMasses; //!< Masses of final products (to avoid repetition). No Ex considered so far
    TGenPhaseSpace fGen {};           //!< ROOT's generator
    double fEx {};                    //!< Excitation energy of initial particle

public:
    DecayGenerator() = default;
    template <typename... Args>
    DecayGenerator(const std::string& part, Args&&... args) : fParticle(part)
    {
        ComputeInitialMass();
        // Populate fProducs using C++17 fold expressions
        // string literals are inferred as const char[], hence we must call std::string() ctor before pushing back
        (fProducts.push_back(std::string(std::forward<Args>(args))), ...);
        ComputeFinalMasses();
    }
    void SetDecay(double T, double thetaLab, double philab);
    double Generate();
    TLorentzVector* GetLorentzVector(unsigned int idx);
    double GetFinalMass(unsigned int idx) const;

    void Print() const;

private:
    void ComputeInitialMass();
    void ComputeFinalMasses();
};
} // namespace ActSim
#endif
