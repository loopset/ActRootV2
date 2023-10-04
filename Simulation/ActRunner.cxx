#include "ActRunner.h"

#include "ActSRIM.h"
#include "ActGeometry.h"

#include "TH3.h"
#include "TMath.h"
#include "TRandom3.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

ActSim::Runner::Runner(ActPhysics::SRIM* sri,
                       Geometry* geo,
                       TRandom3* rand,
                       double silSigma)
    : fsrim(sri), fgeo(geo), fRand(rand)
{
    //initialize sil resolution with sigma at 5.5 MeV
    fSilResolution = std::make_unique<TF1>("fSilResolution",
                                           [=](double* x, double* p)
                                           {return silSigma * TMath::Sqrt(x[0] / 5.5);},
                                           0.0, 100.0, 1);
}

std::pair<double, double> ActSim::Runner::EnergyAfterSilicons(double T3EnteringSil,
                                                             double silWidth,
                                                             double silThresh,
                                                             const std::string& silString,
                                                             bool enableResolution,
                                                             bool enableStraggling)
{
    //defs
    double eLoss {}; double T3AfterSil0 {};
    //initial range
    auto RIni {fsrim->EvalDirect(silString, T3EnteringSil)};
    //RLeft without straggling
    auto RLeft {RIni - silWidth};
    if(RLeft < 0)//particle stopped in silicon
    {
        eLoss = T3EnteringSil;
        if(enableResolution)
            eLoss = fRand->Gaus(T3EnteringSil, fSilResolution->Eval(T3EnteringSil));
        T3AfterSil0 = 0.;
        //if threshold
        if(eLoss < silThresh)
            return {std::nan("threshold"), 0.};
        else
        {
            return {eLoss, T3AfterSil0};
        }
    }
    else
    {
        double RLeftWithStraggling {};
        if(enableStraggling)
            RLeftWithStraggling = ApplyStragglingInMaterialToRLeft(RIni, RLeft, silString);
        else
            RLeftWithStraggling = RLeft;//no straggling
        
        T3AfterSil0 = T3EnteringSil - fsrim->EvalInverse(silString, RLeftWithStraggling);

        eLoss = {T3EnteringSil - T3AfterSil0};
        if(enableResolution)
            eLoss = fRand->Gaus(eLoss, fSilResolution->Eval(eLoss));

        if(eLoss < silThresh)
            return {std::nan("threshold"), T3AfterSil0};
        else
            return {eLoss, T3AfterSil0};    
    }
}

double ActSim::Runner::ApplyStragglingInMaterialToRLeft(double RIni,
                                                       double RLeft,
                                                       const std::string& srimKey)
{
    auto distance {RIni - RLeft};
    auto straggRIni {fsrim->EvalLongStraggling(srimKey, RIni)};
    auto straggRLeft{fsrim->EvalLongStraggling(srimKey, RLeft)};
    auto straggDist {TMath::Sqrt(TMath::Power(straggRIni, 2) - TMath::Power(straggRLeft, 2))};
    double RLeftWithStraggling {};
    do
    {
        auto distanceStragg {fRand->Gaus(distance, straggDist)};
        RLeftWithStraggling = RIni - distanceStragg;
    }
    while(RLeftWithStraggling < 0.);
    return RLeftWithStraggling;//return RLeft after straggling
}

double ActSim::Runner::EnergyAfterGas(double TIni, double distance, const std::string &gasKey, bool straggling)
{
    auto RIni {fsrim->EvalDirect(gasKey, TIni)};
    auto RLeft {RIni - distance};
    if(RLeft < 0)
        return std::nan("stopped in gas");
    if(!straggling)
        return fsrim->EvalInverse(gasKey, RLeft);
    else
    {
        auto RLeftWithStraggling {ApplyStragglingInMaterialToRLeft(RIni, RLeft, gasKey)};
        return fsrim->EvalInverse(gasKey, RLeftWithStraggling);
    }
}

double ActSim::Runner::EnergyBeforeGas(double Esil, double trackLengthInMM,
                                      const std::string& gasKey)
{
    auto RIni {fsrim->EvalDirect(gasKey, Esil)};
    auto RVertex {RIni + trackLengthInMM};
    return fsrim->EvalInverse(gasKey, RVertex);
}

ActSim::Runner::XYZPoint ActSim::Runner::SampleVertex(double meanY, double sigmaY, double meanZ, double sigmaZ, TH3F* histBeam)
{
    XYZPoint ret {};
    double x { fRand->Uniform() * fgeo->GetDriftParameters().X * 2 *10.};//*2 (half length to length) and * 10. (cm to mm)
    double y {}; double z {};
    if(histBeam)
    {
        z = fRand->Gaus(meanZ, sigmaZ);
        double thetaXYHist {}; double thetaXZHist {};
        histBeam->GetRandom3(y, thetaXYHist, thetaXZHist);
        ret = {
            x,
            y - x * TMath::Tan(thetaXYHist * TMath::DegToRad()),
            z - x * TMath::Tan(thetaXZHist * TMath::DegToRad())
        };
    }
    else
    {
        y = fRand->Uniform() * fgeo->GetDriftParameters().Y * 2 *10.;
        z = fRand->Uniform() * fgeo->GetDriftParameters().Z * 2 * 10.;
        ret = {x, y, z};
    }
    return ret;
}

ActSim::Runner::XYZPoint ActSim::Runner::DisplacePointToTGeometryFrame(const XYZPoint &point)
{
    XYZPoint ret
        {
            point.X() / 10. - fgeo->GetDriftParameters().X,
            point.Y() / 10. - fgeo->GetDriftParameters().Y,
            point.Z() / 10. - fgeo->GetDriftParameters().Z
        };
    return ret;
}

ActSim::Runner::XYZPoint ActSim::Runner::DisplacePointToStandardFrame(const XYZPoint &pointGeo)
{
    XYZPoint ret
        {
            (pointGeo.X() + fgeo->GetDriftParameters().X) * 10.,
            (pointGeo.Y() + fgeo->GetDriftParameters().Y) * 10. ,
            (pointGeo.Z() + fgeo->GetDriftParameters().Z) * 10.
        };
    return ret;
}

double ActSim::Runner::RandomizeBeamEnergy(double Tini, double sigma)
{
    return fRand->Gaus(Tini, sigma);
}
