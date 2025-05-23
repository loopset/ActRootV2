#ifndef ActKinematics_h
#define ActKinematics_h

#include "ActInputParser.h"
#include "ActParticle.h"

#include "Rtypes.h"

#include "TAttLine.h"

#include <Math/GenVector/BoostX.h>
#include <Math/Vector3Dfwd.h>
#include <Math/Vector4Dfwd.h>
#include <memory>
#include <string>
#include <tuple>

// Forward declarations
class TGraph;
class TGraphErrors;

namespace ActPhysics
{
class Kinematics
{
public:
    using ThreeVector = ROOT::Math::XYZVector;
    using FourVector = ROOT::Math::PxPyPzEVector;
    using LorentzBoostX = ROOT::Math::BoostX;

private:
    std::string fReactionStr {}; //!< Holds std::string of reaction passed to construct class
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
    double fEx {};
    double fT1Lab {};
    double fGamma {};
    double fBeta {};
    double fQvalue {};
    // flag to invert theta CM if reaction is in inverse kinematics
    bool fInverse {}; //!< Flag determining whether reaction occurs in inverse kinematics

    // 4-Vectors
    FourVector fP1Lab {};
    FourVector fP2Lab {};
    FourVector fPInitialLab {};
    FourVector fPInitialCM {};
    FourVector fP3CM {};
    FourVector fP4CM {};
    FourVector fP3Lab {};
    FourVector fP4Lab {};
    LorentzBoostX fBoostTransformation {};

    // Store values in class
    double fThetaCM {};
    double fPhiCM {};
    double fEcm {};
    double fT3Lab {};
    double fT4Lab {};
    double fTheta3Lab {};
    double fTheta4Lab {};
    double fPhi3Lab {};
    double fPhi4Lab {};

public:
    Kinematics() = default;
    Kinematics(const std::string& reaction);
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

    void ComputeRecoilKinematics(double thetaCMRads, double phiCMRads); //!< Method used in simulation to compute recoil
                                                                        //!< lab kinematics given thetaCM and phiCM
    double ReconstructBeamEnergyFromLabKinematics(double T3, double theta3LabRads); // !< Reconstruct T1 from light
                                                                                    // particle. Assumes Ex = 0
    double ReconstructTheta3CMFromLab(double TLab, double thetaLabRads); //!< Reconstructs thetaCM from lab kinematics
    double ReconstructExcitationEnergy(double argT3, double argTheta3LabRads); //!< Reconstructs Ex from lab kinematics
    double ComputeTheoreticalT3(double argTheta3LabRads,
                                const std::string& sol = {"pos"}); //!< Computes theoretical T3 for given theta3 in lab
    [[deprecated("Do not use ComputeMissingMass: prefer ReconstructExcitationEnergy. If in need, check it works fine")]]
    double ComputeMissingMass(double argT3, double argTheta3LabRads, double argPhi3Rads, double Tbeam,
                              double& retTRecoil, ThreeVector& retPRecoil);
    double ComputeTheoreticalTheta4(double argTheta3LabRads,
                                    const std::string& sol = {"pos"}); //!< Computes theoretical theta4 for given theta3
    double ComputeEquivalentBeamEnergy();  //!< For theoretical OMP calculations: equivalent beam energy for same ECM
    double ComputeTheta3FromT3(double T3); //!< Computess theoretical theta3 for given T3

    void Print() const;

    void Draw(); //!< Draw most useful kinematical relations for a reaction

    void Reset(); //!< Resets class. Do not use in general

    TGraph* GetKinematicLine3(double step = 0.1, EColor color = kMagenta, ELineStyle style = kSolid);
    TGraph* GetKinematicLine4(double step = 0.1, EColor color = kBlue, ELineStyle style = kSolid);
    TGraph* GetTheta3vs4Line(double step = 0.1, EColor color = kBlue, ELineStyle style = kSolid);
    TGraph* GetThetaLabvsThetaCMLine(double step = 0.1, EColor color = kMagenta, ELineStyle style = kSolid);
    TGraphErrors* TransfromCMCrossSectionToLab(TGraphErrors* gcm); //!< Transforms a differential angular distribution
                                                                   //!< in CM frame to Lab

    // Setters
    void SetBeamEnergy(double T1);
    void SetEx(double Ex);
    void SetBeamEnergyAndEx(double T1, double Ex);
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
    void ReadConfiguration(const std::string& file);
    // Getters
    double GetT1Lab() const { return fT1Lab; }
    double GetT3Lab() const { return fT3Lab; }
    double GetT4Lab() const { return fT4Lab; }
    double GetTheta3Lab() const { return fTheta3Lab; }
    double GetTheta4Lab() const { return fTheta4Lab; }
    double GetPhi3Lab() const { return fPhi3Lab; }
    double GetPhi4Lab() const { return fPhi4Lab; }
    double GetThetaCM() const { return fThetaCM; }
    double GetPhiCM() const { return fPhiCM; }
    double GetBeta() const { return fBeta; }
    double GetGamma() const { return fGamma; }
    double GetECM() const { return fEcm; } //!< Get Ecm as a "total energy" aka including particle masses
    double GetResonantECM(bool withEx = false) const
    {
        return fEcm - (fm3 + fm4 + (withEx ? fEx : 0));
    } //!< Ecm with outgoing partition masses substracted. Pass bool to determine whether include Ex or not
    double GetEx() const { return fEx; }
    double GetMass(unsigned int index) const;
    std::tuple<double, double, double, double> GetMasses() const;
    FourVector GetPInitialLab() const { return fPInitialLab; }
    double GetQValue() const { return fQvalue; }
    double GetT1Thresh() const;
    const Particle& GetParticle(unsigned int i) const;

private:
    void ConstructFromStr(const std::string& reaction);
    void SetRecoil3LabKinematics();
    void SetRecoil4LabKinematics();
    void ComputeQValue();
    void CheckQValue();
    void Init(); //!< Main function initializing kinematics every time a change is produced
    double GetPhiFromVector(const FourVector& vect);
    double GetThetaFromVector(const FourVector& vect, bool reverse = false);
};
} // namespace ActPhysics

#endif
