#include "ActKinematics.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActParticle.h"

#include "Rtypes.h"

#include "TAttLine.h"
#include "TGraph.h"
#include "TMathBase.h"
#include <TMath.h>

#include <Math/GenVector/BoostX.h>
#include <Math/Vector3D.h>
#include <Math/Vector4D.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

ActPhysics::Kinematics::Kinematics(const std::string& reaction)
{
    ConstructFromStr(reaction);
}

ActPhysics::Kinematics::Kinematics(double m1, double m2, double m3, double m4, double T1, double Eex)
    : fm1(m1),
      fm2(m2),
      fm3(m3),
      fm4(m4),
      fT1Lab(T1),
      fEex(Eex)
{
    Init();
}

ActPhysics::Kinematics::Kinematics(const std::string& p1, const std::string& p2, const std::string& p3,
                                   const std::string& p4, double T1, double Eex)
    : fT1Lab(T1),
      fEex(Eex)
{
    // Init particles
    // 1
    fp1 = Particle(p1);
    fm1 = fp1.GetMass();
    // 2
    fp2 = Particle(p2);
    fm2 = fp2.GetMass();
    // 3
    fp3 = Particle(p3);
    fm3 = fp3.GetMass();
    // 4
    fp4 = Particle(p4);
    fm4 = fp4.GetMass();
    // Init
    Init();
}

ActPhysics::Kinematics::Kinematics(const Particle& p1, const Particle& p2, const Particle& p3, const Particle& p4,
                                   double T1, double Eex)
    : fp1(p1),
      fp2(p2),
      fp3(p3),
      fp4(p4),
      fT1Lab(T1),
      fEex(Eex)
{
    // Set masses manually from particles
    // 1
    fm1 = fp1.GetMass();
    // 2
    fm2 = fp2.GetMass();
    // 3
    fm3 = fp3.GetMass();
    // 4
    fm4 = fp4.GetMass();
    // Init class
    Init();
}

ActPhysics::Kinematics::Kinematics(const std::string& p1, const std::string& p2, const std::string& p3, double T1,
                                   double Eex)
    : fp1(p1),
      fp2(p2),
      fp3(p3),
      fT1Lab(T1),
      fEex(Eex)
{
    // Automatically compute Particle 4
    auto zIn {fp1.GetZ() + fp2.GetZ()};
    auto aIn {fp1.GetA() + fp2.GetA()};
    fp4 = Particle {zIn - fp3.GetZ(), aIn - fp3.GetA()};

    // Set masses
    fm1 = fp1.GetMass();
    fm2 = fp2.GetMass();
    fm3 = fp3.GetMass();
    fm4 = fp4.GetMass();

    // Init class
    Init();
}

ActPhysics::Kinematics::Kinematics(const Particle& p1, const Particle& p2, const Particle& p3, double T1, double Eex)
    : fp1(p1),
      fp2(p2),
      fp3(p3),
      fT1Lab(T1),
      fEex(Eex)
{
    // Automatically compute Particle 4
    auto zIn {fp1.GetZ() + fp2.GetZ()};
    auto aIn {fp1.GetA() + fp2.GetA()};
    fp4 = Particle {zIn - fp3.GetZ(), aIn - fp3.GetA()};

    // Set masses
    fm1 = fp1.GetMass();
    fm2 = fp2.GetMass();
    fm3 = fp3.GetMass();
    fm4 = fp4.GetMass();

    // Init class
    Init();
}

void ActPhysics::Kinematics::ConstructFromStr(const std::string& reaction)
{
    std::string str {reaction};
    // Clean whitespaces
    str.erase(std::remove_if(str.begin(), str.end(), [](auto c) { return std::isspace(c); }), str.end());
    // Locate all components
    auto open {str.find_first_of("(")};
    auto beam {str.substr(0, open)};
    auto comma {str.find_first_of(",")};
    auto target {str.substr(open + 1, comma - (open + 1))};
    auto close {str.find_last_of(")")};
    auto light {str.substr(comma + 1, close - (comma + 1))};
    // After @ there is the beam energy
    auto at {str.find_last_of("@")};
    auto energy {str.substr(at + 1)};
    bool withHeavy {(at - close) > 1};
    std::string heavy {};
    if(withHeavy)
        heavy = str.substr(close + 1, at - (close + 1));
    // After | there is the Ex of the heavy particle
    auto vertical {str.find_last_of("|")};
    bool withEx {};
    std::string ex {};
    if(vertical != std::string::npos)
    {
        ex = str.substr(vertical + 1);
        if(ex.size() > 0)
            withEx = true;
    }
    // Construct
    if(withHeavy)
        *this = Kinematics {beam, target, light, heavy, std::stod(energy), withEx ? std::stod(ex) : 0};
    else
        *this = Kinematics {beam, target, light, std::stod(energy), withEx ? std::stod(ex) : 0};
}

void ActPhysics::Kinematics::Init()
{
    // Determine inversion
    fInverse = (fm1 > fm2);
    // Compute Q value
    ComputeQValue();
    if(fT1Lab == -1)
    {
        // std::cout << MAGENTA << "Using Kinematics with no beam energy set!" << RESET << '\n';
        return;
    }
    CheckQValue();
    double E1Lab {fT1Lab + fm1};
    double p1Lab {TMath::Sqrt(E1Lab * E1Lab - fm1 * fm1)};
    fP1Lab = {p1Lab, 0.0, 0.0, E1Lab}; // beam along X axis! ACTAR TPC reference frame!
    fP2Lab = {0., 0., 0., fm2};
    fPInitialLab = fP1Lab + fP2Lab;

    // and now let's move to CM! For now on, we assume boost along X axis only (rememeber, for actar X is like standar Z
    // axis in particle physics)!
    auto betaVector {fPInitialLab.BoostToCM()};
    if((betaVector.Y() != 0.) || (betaVector.Z() != 0.))
    {
        throw std::runtime_error(
            "Kinematics::Init(): Error -> Boost includes non-null Y and Z values -> This class only works with boost "
            "along X axis, as ACTAR TPC standard reference frame");
    }
    fBoostTransformation.SetBeta(betaVector.X());
    fBeta = fBoostTransformation.Beta();
    fGamma = fBoostTransformation.Gamma();
    fPInitialCM = fBoostTransformation(fPInitialLab);
    fEcm = fPInitialCM.E();
}

void ActPhysics::Kinematics::SetBeamEnergy(double T1)
{
    fT1Lab = T1;
    Init();
}

void ActPhysics::Kinematics::SetEx(double Ex)
{
    fEex = Ex;
    Init();
}

void ActPhysics::Kinematics::SetBeamEnergyAndEx(double T1, double Ex)
{
    fT1Lab = T1;
    fEex = Ex;
    Init();
}

double ActPhysics::Kinematics::GetMass(unsigned int index) const
{
    switch(index)
    {
    case 1: return fm1; break;
    case 2: return fm2; break;
    case 3: return fm3; break;
    case 4: return fm4; break;
    default: throw std::runtime_error("Index out of range: 1, 2, 3 or 4"); break;
    }
}

std::tuple<double, double, double, double> ActPhysics::Kinematics::GetMasses() const
{
    return std::make_tuple(fm1, fm2, fm3, fm4);
}

void ActPhysics::Kinematics::SetRecoil3LabKinematics()
{
    fP3Lab = {fBoostTransformation.Inverse()(fP3CM)};
    fT3Lab = fP3Lab.E() - fm3;
    fTheta3Lab = GetThetaFromVector(fP3Lab);
    fPhi3Lab = GetPhiFromVector(fP3Lab);
}

void ActPhysics::Kinematics::SetRecoil4LabKinematics()
{
    fP4Lab = {fBoostTransformation.Inverse()(fP4CM)};
    fT4Lab = fP4Lab.E() - (fm4 + fEex);
    fTheta4Lab = GetThetaFromVector(fP4Lab);
    fPhi4Lab = GetPhiFromVector(fP4Lab);
}

void ActPhysics::Kinematics::ComputeRecoilKinematics(double thetaCMRads, double phiCMRads)
{
    // Invert theta CM if needed
    if(fInverse)
        thetaCMRads = TMath::Pi() - thetaCMRads;
    // Set angles
    fThetaCM = thetaCMRads;
    fPhiCM = phiCMRads;
    // Compute kinematics in CM for 3rd particle
    double E3CM {0.5 * (fEcm * fEcm + fm3 * fm3 - (fm4 + fEex) * (fm4 + fEex)) / fEcm};
    double p3CM {TMath::Sqrt(E3CM * E3CM - fm3 * fm3)};
    fP3CM = {p3CM * TMath::Cos(fThetaCM), p3CM * TMath::Sin(fThetaCM) * TMath::Sin(fPhiCM),
             p3CM * TMath::Sin(fThetaCM) * TMath::Cos(fPhiCM), E3CM};
    // And now for 4th particle
    fP4CM = fPInitialCM - fP3CM;
    // And now back to Lab
    SetRecoil3LabKinematics();
    SetRecoil4LabKinematics();
}

void ActPhysics::Kinematics::Print() const
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << BOLDYELLOW << "---- Kinematics ----" << '\n';
    for(auto p : {&fp1, &fp2, &fp3, &fp4})
    {
        std::cout << "->Particle specs : " << '\n';
        p->Print();
    }
    std::cout << "······························" << '\n';
    std::cout << "-> Beam energy : " << fT1Lab << " MeV\n";
    std::cout << "--> transforms at CM with gamma: " << fGamma << " and beta: " << fBeta << '\n';
    std::cout << "--> transforms at CM with E_{CM}: " << fEcm << '\n';
    std::cout << "--> Recoil 3 with energy: " << fT3Lab << " at theta: " << fTheta3Lab * TMath::RadToDeg()
              << " degrees and phi: " << fPhi3Lab * TMath::RadToDeg() << " degrees" << '\n';
    std::cout << "--> Recoil 4 with energy: " << fT4Lab << " at theta: " << fTheta4Lab * TMath::RadToDeg()
              << " degrees and phi: " << fPhi3Lab * TMath::RadToDeg() << " degrees" << '\n';
    std::cout << "------------------------------" << RESET << '\n';
}

double ActPhysics::Kinematics::GetPhiFromVector(const FourVector& vect)
{
    double phi {};
    // we use the ATan2 function, but converting it to [0., 2pi) range
    phi = TMath::ATan2(vect.Y(), vect.Z());
    if(phi < 0.)
        phi += 2.0 * TMath::Pi();

    return phi;
}

double ActPhysics::Kinematics::GetThetaFromVector(const FourVector& vect, bool reverse)
{
    if(!reverse)
        return TMath::ACos(vect.X() / TMath::Sqrt(vect.Vect().Mag2()));
    else
        return TMath::Pi() - TMath::ACos(vect.X() / TMath::Sqrt(vect.Vect().Mag2()));
}

double ActPhysics::Kinematics::ReconstructBeamEnergyFromLabKinematics(double argT3, double argTheta3LabRads)
{
    double a {fm2 - argT3 - fm3};
    double b {TMath::Sqrt(argT3 * argT3 + 2.0 * argT3 * fm3) * TMath::Cos(argTheta3LabRads)};
    double c {0.5 * (fm4 * fm4 - fm3 * fm3 - fm1 * fm1 - fm2 * fm2) - fm1 * (fm2 - argT3 - fm3) + fm2 * (argT3 + fm3)};
    double A {b * b - a * a};
    double B {2.0 * (b * b * fm1 + a * c)};
    double C {-c * c};
    double Delta {B * B - 4.0 * A * C};
    // std::cout<<"a: "<<a<<" b: "<<b<<" c: "<<c<<'\n';
    // std::cout<<"A: "<<A<<" B: "<<B<<" C: "<<C<<" Delta: "<<Delta<<'\n';
    if(Delta < 0.0)
        return std::nan("D<0");
    // only positive solution has physical interest
    double solPos {(-B + TMath::Sqrt(Delta)) / (2 * A)};
    // double solNeg { (- B - TMath::Sqrt(Delta)) / (2 * A)};
    return solPos;
}

void ActPhysics::Kinematics::ComputeQValue()
{
    // does not depend on T1 beam!
    fQvalue = (fm1 + fm2 - fm3 - (fm4 + fEex));
}

double ActPhysics::Kinematics::GetT1Thresh() const
{
    return -fQvalue * (fm1 + fm2 + fm3 + (fm4 + fEex)) / (2.0 * fm2);
}

void ActPhysics::Kinematics::CheckQValue()
{
    if(fQvalue < 0.0)
    {
        auto T1threshold {GetT1Thresh()};
        if(fT1Lab < T1threshold)
        {
            throw std::runtime_error(("Kinematics::CheckQValue(): Reaction has a threshold energy of " +
                                      std::to_string(T1threshold) + " MeV, but given beam has only " +
                                      std::to_string(fT1Lab) + " MeV!"));
        }
    }
}

double ActPhysics::Kinematics::ReconstructExcitationEnergy(double argT3, double argTheta3LabRads)
{
    // mass code:
    //  fm1 = beam
    //  fm2 = target (at rest)
    //  fm3 = light recoil (ejectile)
    //  fm4 = heavy recoil
    double p3 {TMath::Sqrt(argT3 * (argT3 + 2.0 * fm3))};
    // std::cout<<"p3: "<<p3<<'\n';
    double E3 {argT3 + fm3}; // TOTAL energy
    // std::cout<<"E3: "<<E3<<'\n';
    double invariant4Mass {TMath::Power(fEcm, 2) + TMath::Power(fm3, 2) -
                           2.0 * fEcm * (fGamma * (E3 + fBeta * p3 * TMath::Cos(argTheta3LabRads)))};
    // WATCH OUT!!! in the above formula, usually one has (E3 - beta * p3 * cos())
    // but here, since beta is already negative (since we are using ROOT's lorentz transformations)
    // we use the general +: - is already included in beta!
    double recfEex {TMath::Sqrt(invariant4Mass) - fm4};
    return recfEex;
}

double ActPhysics::Kinematics::ReconstructTheta3CMFromLab(double TLab, double thetaLabRads)
{
    // Build momentum and energy
    auto pLab {TMath::Sqrt(TLab * (TLab + 2 * fm3))};
    double ELab {TLab + fm3}; // TOTAL energy
    // Build 4-momemtum
    double phi {0.}; // without generality loss
    FourVector PLab {pLab * TMath::Cos(thetaLabRads), pLab * TMath::Sin(thetaLabRads) * TMath::Sin(phi),
                     pLab * TMath::Sin(thetaLabRads) * TMath::Cos(phi), ELab};
    // move to CM
    auto PCM {fBoostTransformation(PLab)};
    return GetThetaFromVector(PCM, fInverse);
}

double ActPhysics::Kinematics::ComputeTheoreticalT3(double argTheta3LabRads, const std::string& sol)
{
    double A {(TMath::Power(fEcm, 2) + fm3 * fm3 - (fm4 + fEex) * (fm4 + fEex)) / (2.0 * fGamma * fEcm)};
    double B {TMath::Abs(fBeta) * TMath::Cos(argTheta3LabRads)};
    double Delta {A * A * B * B - B * B * fm3 * fm3 * (1.0 - B * B)};
    if(Delta < 0)
        return -11;
    double denom {1.0 - B * B};
    if(sol == "pos")
    {
        auto val {(A + TMath::Sqrt(Delta)) / denom - fm3};
        return val;
    }
    else if(sol == "neg")
    {
        auto val {(A - TMath::Sqrt(Delta)) / denom - fm3};
        return val;
    }
    else
    {
        throw std::runtime_error("sol arg only admits two options: pos or neg!");
    }
}

double ActPhysics::Kinematics::ComputeTheoreticalTheta4(double argTheta3LabRads, const std::string& sol)
{
    // get T3
    auto T3 {ComputeTheoreticalT3(argTheta3LabRads, sol)};
    if(T3 == -11 || !std::isfinite(T3))
        return -22;
    // build total energy and momentum
    double p3 {TMath::Sqrt(T3 * (T3 + 2 * fm3))};
    // lets assume phi = 0 always
    FourVector P3Lab {p3 * TMath::Cos(argTheta3LabRads), 0.0, p3 * TMath::Sin(argTheta3LabRads), T3 + fm3};
    FourVector P4Lab = {fPInitialLab - P3Lab};
    // and get theta
    return GetThetaFromVector(P4Lab);
}

double ActPhysics::Kinematics::ComputeMissingMass(double argT3, double argTheta3LabRads, double argPhi3Rads,
                                                  double Tbeam, double& retTRecoil, ThreeVector& retPRecoil)
{
    FourVector initialLab {};
    double E1Lab {fT1Lab + fm1};
    double p1Lab {TMath::Sqrt(E1Lab * E1Lab - fm1 * fm1)};
    FourVector newP1Lab {p1Lab, 0.0, 0.0, E1Lab}; // beam along X axis! ACTAR TPC reference frame!
    initialLab = newP1Lab + fP2Lab;

    // determine mass of the missing 4th particle
    double E3Lab {argT3 + fm3};
    double p3Lab {TMath::Sqrt(E3Lab * E3Lab - fm3 * fm3)};
    FourVector final3Lab {p3Lab * TMath::Cos(argTheta3LabRads),
                          p3Lab * TMath::Sin(argTheta3LabRads) * TMath::Sin(argPhi3Rads),
                          p3Lab * TMath::Sin(argTheta3LabRads) * TMath::Cos(argPhi3Rads), E3Lab};
    auto missingVector {initialLab - final3Lab};
    double missingMass {missingVector.M()};
    // return recoil kinetic energy
    retTRecoil = initialLab.E() - final3Lab.E() - fm4; // assumming fEex = 0.0
    retPRecoil = missingVector.Vect();
    return missingMass;
}

void ActPhysics::Kinematics::Reset()
{
    *this = Kinematics(fm1, fm2, fm3, fm4, fT1Lab, fEex);
}

TGraph* ActPhysics::Kinematics::GetKinematicLine3(double step, EColor color, ELineStyle style)
{
    auto* ret {new TGraph};
    ret->SetTitle(";#theta_{Lab} [#circ];E_{3} [MeV]");
    ret->SetLineWidth(2);
    ret->SetLineColor(color);
    ret->SetLineStyle(style);
    for(double thetaCM = 0; thetaCM < 180; thetaCM += step)
    {
        ComputeRecoilKinematics(thetaCM * TMath::DegToRad(), 0.);
        double thetaLab {GetTheta3Lab() * TMath::RadToDeg()};
        double T3Lab {GetT3Lab()};
        if(std::isfinite(thetaLab) && std::isfinite(T3Lab))
            ret->SetPoint(ret->GetN(), thetaLab, T3Lab);
    }

    return ret;
}

TGraph* ActPhysics::Kinematics::GetKinematicLine4(double step, EColor color, ELineStyle style)
{
    auto* ret {new TGraph};
    ret->SetTitle(";#theta_{Lab} [#circ];E_{4} [MeV]");
    ret->SetLineWidth(2);
    ret->SetLineColor(color);
    ret->SetLineStyle(style);
    for(double thetaCM = 0; thetaCM < 180; thetaCM += step)
    {
        ComputeRecoilKinematics(thetaCM * TMath::DegToRad(), 0.);
        double thetaLab {GetTheta4Lab() * TMath::RadToDeg()};
        double T4Lab {GetT4Lab()};
        if(std::isfinite(thetaLab) && std::isfinite(T4Lab))
            ret->SetPoint(ret->GetN(), thetaLab, T4Lab);
    }
    return ret;
}

TGraph* ActPhysics::Kinematics::GetTheta3vs4Line(double step, EColor color, ELineStyle style)
{
    auto* ret {new TGraph};
    ret->SetTitle(";#theta_{3} [#circ];#theta_{4} [#circ]");
    ret->SetLineWidth(2);
    ret->SetLineColor(color);
    ret->SetLineStyle(style);
    for(double thetaCM = 0; thetaCM < 180; thetaCM += step)
    {
        ComputeRecoilKinematics(thetaCM * TMath::DegToRad(), 0.);
        auto theta3 {GetTheta3Lab() * TMath::RadToDeg()};
        auto theta4 {GetTheta4Lab() * TMath::RadToDeg()};
        if(std::isfinite(theta3) && std::isfinite(theta4))
            ret->SetPoint(ret->GetN(), theta3, theta4);
    }
    return ret;
}

TGraph* ActPhysics::Kinematics::GetThetaLabvsThetaCMLine(double step, EColor color, ELineStyle style)
{
    auto* ret {new TGraph};
    ret->SetTitle(";#theta_{3, CM} [#circ];#theta_{3, Lab} [#circ]");
    ret->SetLineWidth(2);
    ret->SetLineColor(color);
    ret->SetLineStyle(style);
    for(double thetaCM = 0; thetaCM < 180; thetaCM += step)
    {
        ComputeRecoilKinematics(thetaCM * TMath::DegToRad(), 0.);
        auto thetaLab {GetTheta3Lab() * TMath::RadToDeg()};
        if(std::isfinite(thetaLab))
            ret->SetPoint(ret->GetN(), thetaCM, thetaLab);
    }
    return ret;
}

const ActPhysics::Particle& ActPhysics::Kinematics::GetParticle(unsigned int i) const
{
    if(i == 1)
        return fp1;
    else if(i == 2)
        return fp2;
    else if(i == 3)
        return fp3;
    else
        return fp4;
}

double ActPhysics::Kinematics::ComputeEquivalentBeamEnergy()
{
    // This means getting the kinetic energy of the ejectile that would give
    // the same centre-of-mass energy as the combination of beam and target
    // This is done in DIRECT kinematics, therefore,
    // the excitation energy corresponds to the 3rd particle:
    // d (1) + 20O (2) -> 19O (3) + t (4)
    // Assuming particle 3 at rest
    double Teq {(TMath::Power(fEcm, 2) - TMath::Power(fm3 + fEex, 2) - TMath::Power(fm4, 2)) / (2 * (fm3 + fEex)) -
                fm4};
    return Teq;
}

void ActPhysics::Kinematics::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    // Specify particles to be read
    auto p1 {block->GetString("Beam")};
    auto p2 {block->GetString("Target")};
    auto p3 {block->GetString("Light")};
    std::string p4 {};
    if(block->CheckTokenExists("Heavy", true))
        p4 = block->GetString("Heavy");
    // Nominal beam energy
    auto T1 {block->GetDouble("BeamEnergy")};
    // Excitation energy of the heavy particle
    double Ex {};
    if(block->CheckTokenExists("Ex", true))
        Ex = block->GetDouble("Ex");
    // Construct
    if(p4.length() > 0)
        *this = Kinematics {p1, p2, p3, p4, T1, Ex};
    else
        *this = Kinematics {p1, p2, p3, T1, Ex};
}

void ActPhysics::Kinematics::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("Kinematics")};
    ReadConfiguration(block);
}
