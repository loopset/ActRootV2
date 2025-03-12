#include "ActKinematicGenerator.h"

#include "ActColors.h"
#include "ActConstants.h"
#include "ActKinematics.h"
#include "ActParticle.h"

#include "TGenPhaseSpace.h"
#include "TList.h"
#include "TLorentzVector.h"

#include <iostream>
#include <stdexcept>
#include <vector>

ActSim::KinematicGenerator::KinematicGenerator(const std::string& p1, const std::string& p2, const std::string& p3,
                                               const std::string& p4, int protonPS, int neutronPS)
    : fneutronPS(neutronPS),
      fprotonPS(protonPS)
{
    // Create particles
    // 1
    fBinParts.push_back(ActPhysics::Particle(p1));
    // 2
    fBinParts.push_back(ActPhysics::Particle(p2));
    // 3
    fBinParts.push_back(ActPhysics::Particle(p3));
    // 4
    fBinParts.push_back(ActPhysics::Particle(p4));
    // Compute heavy particle in final state
    ComputeHeavyMass();
}

ActSim::KinematicGenerator::KinematicGenerator(const ActPhysics::Particle& p1, const ActPhysics::Particle& p2,
                                               const ActPhysics::Particle& p3, const ActPhysics::Particle& p4,
                                               int protonPS, int neutronPS)
    : fneutronPS(neutronPS),
      fprotonPS(protonPS)
{
    // Create particles
    // 1
    fBinParts.push_back(p1);
    // 2
    fBinParts.push_back(p2);
    // 3
    fBinParts.push_back(p3);
    // 4
    fBinParts.push_back(p4);
    // Compute heavy particle in final state
    ComputeHeavyMass();
}


void ActSim::KinematicGenerator::ComputeHeavyMass()
{
    int A {fBinParts.back().GetA()};
    int Z {fBinParts.back().GetZ()};
    if(fneutronPS == 0 && fprotonPS == 0)
        fHeavyPart = fBinParts.back();
    else if(fneutronPS > 0 && fprotonPS == 0)
    {
        fHeavyPart = ActPhysics::Particle(Z, A - fneutronPS);
    }
    else if(fneutronPS == 0 && fprotonPS > 0)
    {
        fHeavyPart = ActPhysics::Particle(Z - fprotonPS, A - fprotonPS);
    }
    else
        throw std::runtime_error(
            "No KinematicGenerator implemented: both neutronPS and protonPS enabled at the same time");
}

void ActSim::KinematicGenerator::Init()
{
    fKin = ActPhysics::Kinematics(fBinParts[0].GetMass(), fBinParts[1].GetMass(), fBinParts[2].GetMass(),
                                  fBinParts[3].GetMass(), fEBeam, fEx);
    // Set TGenPhase Space
    fGen = TGenPhaseSpace();
    auto initial {GetInitialState()};
    auto finalstate {GetFinalState()};
    fGen.SetDecay(initial, finalstate.size(), &(finalstate[0]));
}

TLorentzVector ActSim::KinematicGenerator::GetInitialState()
{
    auto init {fKin.GetPInitialLab()};
    // WARNING! TGenPhaseSpace::Theta() needs beam along Z direction
    //  that is, out X direction in SimKinematics
    auto lorentz {TLorentzVector(init.Z(), init.Y(), init.X(), init.E())};
    lorentz *= ActPhysics::Constants::kMeVToGeV;
    // lorentz.Print();
    return lorentz;
}

std::vector<double> ActSim::KinematicGenerator::GetFinalState()
{
    std::vector<double> ret(2 + fneutronPS + fprotonPS);                      // masses in final state
    ret[0] = fBinParts[2].GetMass() * ActPhysics::Constants::kMeVToGeV;       // light recoil
    ret[1] = (fHeavyPart.GetMass() + fEx) * ActPhysics::Constants::kMeVToGeV; // heavy recoil
    // And now proton and neutron masses
    int counter {2};
    // 1st, proton
    for(int p = 0; p < fprotonPS; p++)
    {
        ret[counter] = ActPhysics::Constants::kpMass * ActPhysics::Constants::kMeVToGeV;
        counter++;
    }
    // 2nd, neutron
    for(int n = 0; n < fneutronPS; n++)
    {
        ret[counter] = ActPhysics::Constants::knMass * ActPhysics::Constants::kMeVToGeV;
        counter++;
    }
    return ret;
}

double ActSim::KinematicGenerator::Generate()
{
    auto w {fGen.Generate()};
    // Scale to MeV all recoils
    for(int i = 0, size = fGen.GetNt(); i < size; i++)
        *fGen.GetDecay(i) *= 1. / ActPhysics::Constants::kMeVToGeV;
    // Return weight
    return w;
}

TLorentzVector* ActSim::KinematicGenerator::GetLorentzVector(unsigned int idx)
{
    auto* ret {fGen.GetDecay(idx)};
    return ret;
}

void ActSim::KinematicGenerator::Print() const
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << BOLDCYAN;
    std::cout << "==== KinematicGenerator ====" << '\n';
    for(auto& p : fBinParts)
        std::cout << " Particle " << p.GetName() << " with mass " << p.GetMass() << " MeV/c2" << '\n';
    std::cout << " Phase space settings: " << '\n';
    std::cout << " -> proton  = " << fprotonPS << '\n';
    std::cout << " -> neutron = " << fneutronPS << '\n';
    std::cout << " -> heavy recoil = " << fHeavyPart.GetName() << '\n';
    std::cout << "    with mass " << fHeavyPart.GetMass() << " MeV/c2" << '\n';
    std::cout << RESET << std::endl;
}
