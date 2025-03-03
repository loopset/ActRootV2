#include "ActDecayGenerator.h"

#include "ActColors.h"
#include "ActConstants.h"
#include "ActParticle.h"

#include "TLorentzVector.h"

#include <cmath>
#include <ios>
#include <iostream>
#include <numeric>

void ActSim::DecayGenerator::ComputeInitialMass()
{
    fInitialMass = fParticle.GetMass();
}

void ActSim::DecayGenerator::ComputeFinalMasses()
{
    fFinalMasses.clear();
    for(const auto& p : fProducts)
        fFinalMasses.push_back(p.GetMass() * ActPhysics::Constants::kMeVToGeV); // Ex considered in particle's mass
}

void ActSim::DecayGenerator::CheckQvalue()
{
    // Devay is possible if Q <= 0
    // Otherwise, like for a d -> p + n, you must force an Ex
    // equivalent to the binding energy
    auto sumFinal {std::reduce(fFinalMasses.begin(), fFinalMasses.end(), 0.)};
    if(fInitialMass * ActPhysics::Constants::kMeVToGeV < sumFinal)
    {
        std::cout << BOLDRED << "DecayGenerator:Check(): sum(finalMasses) > initialMass" << '\n';
        std::cout << "  TGenPhaseSpace will return NaNs. Check your particles' Exs" << RESET << '\n';
    }
}

void ActSim::DecayGenerator::SetDecay(double T, double thetalab, double philab)
{
    fGen = {};
    // Build Lorentz vector for initial state
    auto E {T + fInitialMass};
    auto p {std::sqrt(E * E - fInitialMass * fInitialMass)};
    // WARNING: for the TLorentzVector in the output to be usable in terms of angles
    // the beam axis has to be Z. This is only an inversion in the coordinates HERE
    TLorentzVector initial {p * std::sin(thetalab) * std::cos(philab), p * std::sin(thetalab) * std::sin(philab),
                            p * std::cos(thetalab), E};
    initial *= ActPhysics::Constants::kMeVToGeV;
    // And set it!
    fGen.SetDecay(initial, fFinalMasses.size(), fFinalMasses.data());
}

double ActSim::DecayGenerator::Generate()
{
    return fGen.Generate();
}

TLorentzVector* ActSim::DecayGenerator::GetLorentzVector(unsigned int idx)
{
    auto* ret {fGen.GetDecay(idx)};
    *ret *= 1.0 / ActPhysics::Constants::kMeVToGeV;
    return ret;
}

double ActSim::DecayGenerator::GetFinalMass(unsigned int idx) const
{
    return fFinalMasses[idx] / ActPhysics::Constants::kMeVToGeV;
}

void ActSim::DecayGenerator::Print() const
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << BOLDCYAN;
    std::cout << "::::: DecayGenerator :::::" << '\n';
    std::cout << "-> Particle : " << fParticle.GetName() << '\n';
    std::cout << "    Mass    : " << fParticle.GetMass() << " MeV/c2" << '\n';
    std::cout << "    BE      : " << fParticle.GetBE() << " MeV" << '\n';
    std::cout << "    Ex      : " << fParticle.GetEx() << " MeV" << '\n';
    std::cout << "-> To " << fProducts.size() << " products : " << '\n';
    for(const auto& p : fProducts)
        std::cout << "    " << p.GetName() << ", Mass : " << p.GetMass() << " MeV/c2, Ex : " << p.GetEx() << " MeV"
                  << '\n';
    std::cout << RESET << std::endl;
}
