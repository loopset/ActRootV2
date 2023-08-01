#ifndef ActKinematicGenerator_h
#define ActKinematicGenerator_h

#include "TGenPhaseSpace.h"
#include "Kinematics.h"
#include "Particle.h"

#include "TLorentzVector.h"
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
        ActPhysics::Kinematics fKin;
        TGenPhaseSpace fGen;
        std::vector<ActPhysics::Particle> fBinParts;
        ActPhysics::Particle fHeavyPart;
        int fneutronPS;
        int fprotonPS;
        //store passed values as members
        double fEBeam {};
        double fEx {};
        //Constants: masses of proton and neutron
        double kprotonMass;
        double kneutronMass;
        double kMevToGeV {1.0E-3};//!< TGenPhaseSpace requires GeV units!
        
    public:
        KinematicGenerator() = default;
        KinematicGenerator(const std::string& p1, const std::string& p2,
                           const std::string& p3, const std::string& p4,
                           int protonPS = 0, int neutronPS = 0);
        KinematicGenerator(const ActPhysics::Particle& p1, const ActPhysics::Particle& p2,
                           const ActPhysics::Particle& p3, const ActPhysics::Particle& p4,
                           int protonPS = 0, int neutronPS = 0);

        //Setters
        void SetBeamEnergy(double beamE){fEBeam = beamE; Init();}
        void SetExcitationEnergy(double Ex){fEx = Ex; Init();}
        void SetBeamAndExEnergies(double beamE, double Ex){fEBeam = beamE; fEx = Ex; Init();}
        //Getters
        ActPhysics::Kinematics& GetBinaryKinematics() {return fKin;}
        double Generate();
        TLorentzVector* GetLorentzVector(unsigned int idx);
        void Print() const;

    private:
        void Init();
        void ComputeHeavyMass();
        TLorentzVector GetInitialState();
        std::vector<double> GetFinalState();
        
    };
}

#endif
