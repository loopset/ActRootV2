#ifndef ActKinematicGenerator_h
#define ActKinematicGenerator_h

#include "ActKinematics.h"
#include "ActParticle.h"

#include "TGenPhaseSpace.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include <string>
#include <vector>

namespace ActSim
{
//! A class that wraps together ActPhysics::Kinematics and a TGenPhaseSpace
/*!
  In this way, all sorts of kinematics can be generated within the same class
 */
class KinematicGenerator
{
private:
    ActPhysics::Kinematics fKin; //!< Save always the binary kinematics
    TGenPhaseSpace fGen;         //!< Phase space generator
    TVector3 fBeta {-1, -1, -1}; //!< Store also betas of decaying particles, as calculated in TGenPhaseSpace
    std::vector<ActPhysics::Particle> fBinParts; //!< Vector with outgoing particle definitions
    ActPhysics::Particle fHeavyPart;             //!< Automatically computed heavy particle
    int fneutronPS;
    int fprotonPS;
    // store passed values as members
    double fEBeam {};
    double fEx {};

public:
    KinematicGenerator() = default;
    KinematicGenerator(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4,
                       int protonPS = 0, int neutronPS = 0);
    KinematicGenerator(const ActPhysics::Particle& p1, const ActPhysics::Particle& p2, const ActPhysics::Particle& p3,
                       const ActPhysics::Particle& p4, int protonPS = 0, int neutronPS = 0);

    // Setters
    void SetBeamEnergy(double beamE)
    {
        fEBeam = beamE;
        Init();
    }
    void SetExcitationEnergy(double Ex)
    {
        fEx = Ex;
        Init();
    }
    void SetBeamAndExEnergies(double beamE, double Ex)
    {
        fEBeam = beamE;
        fEx = Ex;
        Init();
    }
    // Getters
    ActPhysics::Kinematics* GetBinaryKinematics() { return &fKin; }
    double Generate();
    TLorentzVector* GetLorentzVector(unsigned int idx);
    TVector3* GetBeta() { return &fBeta; }
    int GetNt() const { return fGen.GetNt(); }
    void Print() const;

private:
    void Init();
    void ComputeHeavyMass();
    TLorentzVector GetInitialState();
    std::vector<double> GetFinalState();
};
} // namespace ActSim

#endif
