#include "ActRunner.h"

#include "ActGeometry.h"
#include "ActInputParser.h"
#include "ActSRIM.h"

#include "TH3.h"
#include "TMath.h"
#include "TRandom.h"
#include "TRandom3.h"

#include "Math/AxisAngle.h"
#include "Math/GenVector/AxisAnglefwd.h"
#include "Math/GenVector/Rotation3D.h"
#include "Math/Rotation3D.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

ActSim::Runner::Runner(ActPhysics::SRIM* srim) : fsrim(srim), fRand(dynamic_cast<TRandom3*>(gRandom)) {}

ActSim::Runner::Runner(ActPhysics::SRIM* sri, Geometry* geo, TRandom3* rand, double silSigma)
    : fsrim(sri),
      fgeo(geo),
      fRand(rand)
{
    // initialize sil resolution with sigma at 5.5 MeV
    fSilResolution = std::make_unique<TF1>(
        "fSilResolution", [=](double* x, double* p) { return silSigma * TMath::Sqrt(x[0] / 5.5); }, 0.0, 100.0, 1);
}

std::pair<double, double> ActSim::Runner::EnergyAfterSilicons(double T3EnteringSil, double silWidth, double silThresh,
                                                              const std::string& silString, bool enableResolution,
                                                              bool enableStraggling)
{
    // defs
    double eLoss {};
    double T3AfterSil0 {};
    // initial range
    auto RIni {fsrim->EvalDirect(silString, T3EnteringSil)};
    // RLeft without straggling
    auto RLeft {RIni - silWidth};
    if(RLeft < 0) // particle stopped in silicon
    {
        eLoss = T3EnteringSil;
        if(enableResolution)
            eLoss = fRand->Gaus(T3EnteringSil, fSilResolution->Eval(T3EnteringSil));
        T3AfterSil0 = 0.;
        // if threshold
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
            RLeftWithStraggling = RLeft; // no straggling

        T3AfterSil0 = fsrim->EvalInverse(silString, RLeftWithStraggling);

        eLoss = {T3EnteringSil - T3AfterSil0};
        if(enableResolution)
            eLoss = fRand->Gaus(eLoss, fSilResolution->Eval(eLoss));

        if(eLoss < silThresh)
            return {std::nan("threshold"), T3AfterSil0};
        else
            return {eLoss, T3AfterSil0};
    }
}

double ActSim::Runner::ApplyStragglingInMaterialToRLeft(double RIni, double RLeft, const std::string& srimKey)
{
    auto distance {RIni - RLeft};
    auto straggRIni {fsrim->EvalLongStraggling(srimKey, RIni)};
    auto straggRLeft {fsrim->EvalLongStraggling(srimKey, RLeft)};
    auto straggDist {TMath::Sqrt(TMath::Power(straggRIni, 2) - TMath::Power(straggRLeft, 2))};
    double RLeftWithStraggling {};
    do
    {
        auto distanceStragg {fRand->Gaus(distance, straggDist)};
        RLeftWithStraggling = RIni - distanceStragg;
    } while(RLeftWithStraggling < 0.);
    return RLeftWithStraggling; // return RLeft after straggling
}

double ActSim::Runner::EnergyAfterGas(double TIni, double distance, const std::string& gasKey, bool straggling)
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

double ActSim::Runner::EnergyBeforeGas(double Esil, double trackLengthInMM, const std::string& gasKey)
{
    auto RIni {fsrim->EvalDirect(gasKey, Esil)};
    auto RVertex {RIni + trackLengthInMM};
    return fsrim->EvalInverse(gasKey, RVertex);
}

std::pair<ActSim::Runner::XYZPoint, ActSim::Runner::XYZPoint>
ActSim::Runner::SampleVertex(double lengthX, double meanY, double sigmaY, double meanZ, double sigmaZ, TH3F* histBeam)
{
    // X is always common for both manners
    double Xstart {0};
    double Xrp {fRand->Uniform() * lengthX};
    // Y depends completely on the method of calculation
    double Ystart {-1};
    double Yrp {-1};
    // Z of beam at entrance
    double Zstart {fRand->Gaus(meanZ, sigmaZ)};
    double Zrp {-1};
    // Two options depending on the existance of a emittance histogram or not
    if(histBeam)
    {
        // Ystart in this case is sampled from the histogram itself!
        double thetaXY {};
        double thetaXZ {};
        histBeam->GetRandom3(Ystart, thetaXY, thetaXZ);
        // Mind that Y is not centred in the histogram value!
        // Rp values are computed as follows:
        Yrp = Ystart - Xrp * TMath::Tan(thetaXY * TMath::DegToRad());
        Zrp = Zstart - Xrp * TMath::Tan(thetaXZ * TMath::DegToRad());
    }
    else
    {
        // Starting point in Y is also random
        Ystart = fRand->Gaus(meanY, sigmaY);
        // And now rp values
        // This way, emittance is fully random
        Yrp = fRand->Gaus(meanY, sigmaY);
        Zrp = fRand->Gaus(meanZ, sigmaZ);
    }
    XYZPoint start {Xstart, Ystart, Zstart};
    XYZPoint vertex {Xrp, Yrp, Zrp};
    return {std::move(start), std::move(vertex)};
    // XYZPoint ret {};
    // double x {fRand->Uniform() * fgeo->GetDriftParameters().X * 2 *
    //           10.}; //*2 (half length to length) and * 10. (cm to mm)
    // double y {};
    // double z {};
    // if(histBeam)
    // {
    //     z = fRand->Gaus(meanZ, sigmaZ);
    //     // Workaround: histogram and meanY are centered around different values
    //     // So we have to compute an offset
    //     auto yoffset {meanY - histBeam->GetMean()};
    //     double thetaXYHist {};
    //     double thetaXZHist {};
    //     histBeam->GetRandom3(y, thetaXYHist, thetaXZHist);
    //     // Correct y by offset
    //     y += yoffset;
    //     ret = {x, y - x * TMath::Tan(thetaXYHist * TMath::DegToRad()),
    //            z - x * TMath::Tan(thetaXZHist * TMath::DegToRad())};
    // }
    // else
    // {
    //     y = fRand->Gaus(meanY, sigmaY);
    //     z = fRand->Gaus(meanZ, sigmaZ);
    //     ret = {x, y, z};
    // }
    // return ret;
}

ActSim::Runner::XYZPoint ActSim::Runner::DisplacePointToTGeometryFrame(const XYZPoint& point)
{
    XYZPoint ret {point.X() / 10. - fgeo->GetDriftParameters().X, point.Y() / 10. - fgeo->GetDriftParameters().Y,
                  point.Z() / 10. - fgeo->GetDriftParameters().Z};
    return ret;
}

ActSim::Runner::XYZPoint ActSim::Runner::DisplacePointToStandardFrame(const XYZPoint& pointGeo)
{
    XYZPoint ret {(pointGeo.X() + fgeo->GetDriftParameters().X) * 10.,
                  (pointGeo.Y() + fgeo->GetDriftParameters().Y) * 10.,
                  (pointGeo.Z() + fgeo->GetDriftParameters().Z) * 10.};
    return ret;
}

double ActSim::Runner::RandomizeBeamEnergy(double Tini, double sigma)
{
    return fRand->Gaus(Tini, sigma);
}

////////////////////
ActSim::Runner::XYZPoint ActSim::Runner::GetRandomVertex(double lengthX)
{
    double x {fBeamEntrance.X() + fRand->Uniform() * lengthX};
    double y {fRand->Gaus(fBeamEntrance.Y(), fVertexSigmaY)};
    double z {fRand->Gaus(fBeamEntrance.Z(), fVertexSigmaZ)};
    return {x, y, z};
}

ActSim::Runner::XYZVector
ActSim::Runner::RotateToWorldFrame(const XYZVector& vBeamFrame, const XYZVector& beamDir) const
{
    // Usually a sampled reaction returns the angles in the beam frame
    //  But to compute geometrical things (propagate that track to a silicon)
    //  We need to work in the "geometry = world" frame,
    //  where in ACTAR the "beam" goes in {1, 0, 0}
    //  Using XYZPoint and XYZVector is easy to compute it,
    //  as quoted here: https://root-forum.cern.ch/t/get-3x3-rotation-matrix-between-two-tvector3/60070
    //  Using only GenVector classes
    auto originalFrame {beamDir.Unit()};
    XYZVector worldFrame {1, 0, 0};
    auto cross {worldFrame.Cross(originalFrame)}; // this defines the rotation axis
    auto angle {TMath::ACos(originalFrame.Dot(worldFrame))};
    ROOT::Math::AxisAngle axis {cross, angle};
    ROOT::Math::Rotation3D rotation {axis};
    return rotation(vBeamFrame);
}

void ActSim::Runner::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    // 1-> Vertex sampling perpendicular to beam direction
    auto entrance {block->GetDoubleVector("Entrance")};
    fBeamEntrance = {entrance[0], entrance[1], entrance[2]};
    fVertexSigmaY = block->GetDouble("VertexSigmaY");
    fVertexSigmaZ = block->GetDouble("VertexSigmaZ");
}

void ActSim::Runner::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("Simulation")};
    ReadConfiguration(block);
}
