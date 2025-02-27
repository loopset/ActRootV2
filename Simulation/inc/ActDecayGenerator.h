#ifndef ActDecayGenerator_h
#define ActDecayGenerator_h
#include "ActParticle.h"

#include "TGenPhaseSpace.h"
#include "TLorentzVector.h"

#include <stdexcept>
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
    std::vector<double> fFinalMasses; //!< Masses of final products (to avoid repetition). Ex considered in the masses
    TGenPhaseSpace fGen {};           //!< ROOT's generator

public:
    DecayGenerator() = default;
    template <typename... Args>
    DecayGenerator(Args&&... args)
    {
        // Check number of parameters
        if(sizeof...(args) < 3)
            throw std::invalid_argument("DecayGenerator(): cannot construct with less than 3 arguments.");
        // Populate
        bool first {true};
        (
            [&]()
            {
                if(first) // first is the decaying particle
                {
                    fParticle = ActPhysics::Particle {std::forward<Args>(args)};
                    first = false;
                }
                else // then the N decay products
                    fProducts.emplace_back(std::forward<Args>(args));
            }(),
            ...); // This is a C++17 fold expression to unpack variadic arguments "args"
        ComputeInitialMass();
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
