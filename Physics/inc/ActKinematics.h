#ifndef ActKinematics_h
#define ActKinematics_h

#include "ActParticle.h"

#include "Rtypes.h"

#include "TAttLine.h"
#include "TGraph.h"

#include <Math/GenVector/BoostX.h>
#include <Math/Vector3Dfwd.h>
#include <Math/Vector4Dfwd.h>
#include <string>
#include <tuple>
#include <utility>

namespace ActPhysics
{
class Kinematics
{
public:
    using ThreeVector = ROOT::Math::XYZVector;
    using FourVector = ROOT::Math::PxPyPzEVector;
    using LorentzBoostX = ROOT::Math::BoostX;

private:
    ActPhysics::Particle fp1 {};
    ActPhysics::Particle fp2 {};
    ActPhysics::Particle fp3 {};
    ActPhysics::Particle fp4 {};
    // auxiliar: since this class before the advent of ActPhysics::Particle was using only masses,
    // we keep it that way: copy Particle::fMass into fmi
    double fm1 {};
    double fm2 {};
    double fm3 {};
    double fm4 {};
    double fEex {};
    double fT1Lab {};
    double fGamma {};
    double fBeta {};
    double fQvalue {};

    // vectors
    FourVector fP1Lab {};
    FourVector fP2Lab {};
    FourVector fPInitialLab {};
    FourVector fPInitialCM {};
    FourVector fP3CM {};
    FourVector fP4CM {};
    FourVector fP3Lab {};
    FourVector fP4Lab {};
    LorentzBoostX fBoostTransformation {};
    // auxiliar to avoid calling member functions every time
    double fTheta3CM {};
    double fTheta4CM {};
    double fPhi3CM {};
    double fPhi4CM {};
    double fEcm {};
    double fT3Lab {};
    double fT4Lab {};
    double fTheta3Lab {};
    double fTheta4Lab {};
    double fPhi3Lab {};
    double fPhi4Lab {};

public:
    Kinematics() = default;
    Kinematics(double m1, double m2, double m3, double m4, double T1 = -1,
               double Eex = 0.); //!< Constructor using masses
    Kinematics(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4,
               double T1 = -1, double Eex = 0); //!< Constructor using strings to get Particle
    Kinematics(const std::string& p1, const std::string& p2, const std::string& p3, double T1 = -1,
               double Eex = 0); //!< Constructor with string of Particle and automatically computed Particle 4
                                //!< (heavy)
    Kinematics(const Particle& p1, const Particle& p2, const Particle& p3, const Particle& p4, double T1 = -1,
               double Eex = 0); //!< Constructor copying Particles
    Kinematics(const Particle& p1, const Particle& p2, const Particle& p3, double T1 = -1,
               double Eex = 0); //!< Constructor by copy of particle and automatically computed Particle 4 (heavy)
    Kinematics(const Kinematics&) = default;
    Kinematics& operator=(const Kinematics&) = default;
    ~Kinematics() = default;

    void ComputeRecoilKinematics(double thetaCMRads, double phiCMRads, int anglesFrom = 4, bool computeBoth = false);
    double ReconstructBeamEnergyFromLabKinematics(double T3, double theta3LabRads);
    double ReconstructTheta3CMFromLab(double TLab, double thetaLabRads);
    double ReconstructExcitationEnergy(double argT3, double argTheta3LabRads);
    double ComputeTheoreticalT3(double argTheta3LabRads, const std::string& sol = {"pos"});
    double ComputeMissingMass(double argT3, double argTheta3LabRads, double argPhi3Rads, double Tbeam,
                              double& retTRecoil, ThreeVector& retPRecoil);
    double ComputeTheoreticalTheta4(double argTheta3LabRads, const std::string& sol = {"pos"});

    void Print() const;

    void Reset();

    TGraph* GetKinematicLine3(double step = 0.1, EColor color = kMagenta, ELineStyle style = kSolid);

    // Setters
    void SetBeamEnergy(double T1);
    void SetEx(double Ex);
    // getters
    double GetT1Lab() const { return fT1Lab; }
    double GetT3Lab() const { return fT3Lab; }
    double GetT4Lab() const { return fT4Lab; }
    double GetTheta3Lab() const { return fTheta3Lab; }
    double GetTheta4Lab() const { return fTheta4Lab; }
    double GetPhi3Lab() const { return fPhi3Lab; }
    double GetPhi4Lab() const { return fPhi4Lab; }
    double GetTheta3CM() const { return fTheta3CM; }
    double GetTheta4CM() const { return fTheta4CM; }
    double GetPhi3CM() const { return fPhi3CM; }
    double GetPhi4CM() const { return fPhi4CM; }
    double GetBeta() const { return fBeta; }
    double GetGamma() const { return fGamma; }
    double GetECM() const { return fEcm; }
    double GetEex() const { return fEex; }
    double GetMass(unsigned int index) const;
    std::tuple<double, double, double, double> GetMasses() const;
    FourVector GetPInitialLab() const { return fPInitialLab; }
    double GetQValue() const { return fQvalue; }
    double GetT1Thresh() const;

private:
    void SetRecoilsCMKinematicsThrough3(double fTheta3CMRads, double phi3CMRads);
    void SetRecoilsCMKinematicsThrough4(double theta4CMRads, double phi4CMRads);
    void SetRecoil3LabKinematics();
    void SetRecoil4LabKinematics();
    void ComputeQValue();
    void CheckQValue();
    void Init();
    double GetPhiFromVector(FourVector vect);
    double GetThetaFromVector(FourVector vect);
};
} // namespace ActPhysics

#endif
