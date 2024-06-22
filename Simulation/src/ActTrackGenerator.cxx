#include "ActTrackGenerator.h"

#include "ActCalibrationManager.h"
#include "ActGas.h"
#include "ActInputParser.h"
#include "ActSRIM.h"
#include "ActTPCDetector.h"
#include "ActTPCLegacyData.h"
#include "ActVoxel.h"

#include "TMath.h"

#include <unistd.h>

#include <memory>
#include <string>
#include <utility>

ActSim::TrackGenerator::TrackGenerator(ActPhysics::SRIM* srim, ActRoot::TPCParameters* tpc, ActPhysics::Gas* gas)
    : fsrim(srim),
      ftpc(tpc),
      fGas(gas)
{
}

void ActSim::TrackGenerator::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    // 1-> Beam sampling
    auto ent {block->GetDoubleVector("BeamEntrance")};
    fBeamEntrance = {ent.at(0), ent.at(1), ent.at(2)};
    fBeamSigmaY = block->GetDouble("BeamSigmaY");
    fBeamSigmaZ = block->GetDouble("BeamSigmaZ");
    // 2-> Lengths to pad conversion
    if(block->CheckTokenExists("RangeStep", true))
        fRangeStep = block->GetDouble("RangeStep");
    fMaxRangeElectrons = block->GetDouble("MaxRangeElectrons");
    fStartTime = block->GetDouble("StartTime");
    fSamplingFreq = block->GetDouble("SamplingFreq");
}

void ActSim::TrackGenerator::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    ReadConfiguration(parser.GetBlock("TrackGenerator"));
}

bool ActSim::TrackGenerator::IsInChamber(const XYZPoint& p)
{
    auto x {0 <= p.X() && p.X() <= ftpc->X()};
    auto y {0 <= p.Y() && p.Y() <= ftpc->Y()};
    auto z {0 <= p.Z() && p.Z() <= ftpc->Z()};
    return x && y && z;
}

void ActSim::TrackGenerator::GenVertex()
{
    fVertex = {fRand->Uniform() * ftpc->X(), fRand->Gaus(fBeamEntrance.Y(), fBeamSigmaY),
               fRand->Gaus(fBeamEntrance.Z(), fBeamSigmaZ)};
}

double ActSim::TrackGenerator::AddBeam(double Tini)
{
    // 1-> Direction
    auto dir {(fVertex - fBeamEntrance).Unit()};
    // 2-> Length
    auto length {(fVertex - fBeamEntrance).R()};
    // 3-> Add!
    return FillCloud("beam", Tini, length, fBeamEntrance, dir);
}

double ActSim::TrackGenerator::AddRecoil(const std::string& which, double Tini, double theta, double phi)
{
    // 1-> Build direction
    XYZVector dir {TMath::Cos(theta), TMath::Sin(theta) * TMath::Sin(phi), TMath::Sin(theta) * TMath::Cos(phi)};
    // 2-> Get range
    auto range {fsrim->EvalDirect(which, Tini)};
    // 3-> Fill
    return FillCloud(which, Tini, range, fVertex, dir);
}

double ActSim::TrackGenerator::FillCloud(const std::string& which, double T, double l, const XYZPoint& point,
                                         const XYZVector& dir)
{
    // Add new row
    fCloud.push_back({});
    auto& vector {fCloud.back()};
    // Iterate over range
    for(double r = 0; r <= l; r += fRangeStep)
    {
        auto p {point + r * dir};
        if(!IsInChamber(p))
            break;
        auto Eit {fsrim->Slow(which, T, fRangeStep)};
        auto charge {T - Eit};
        if(charge <= 0)
            break;
        vector.push_back({(ActRoot::Voxel::XYZPoint)p, static_cast<float>(charge)});
    }
    return T;
}

template <typename T>
std::pair<int, int> ActSim::TrackGenerator::GetPadRange(const T& p, const std::string& which)
{
    double comp {};
    int ref {}; // maximum allowed value: npads - 1
    if(which == "x")
    {
        comp = p.X();
        ref = ftpc->GetNPADSX() - 1;
    }
    else if(which == "y")
    {
        comp = p.Y();
        ref = ftpc->GetNPADSY() - 1;
    }
    else
    {
        ;
    }
    auto low {static_cast<int>((comp - fMaxRangeElectrons) / ftpc->GetPadSide())};
    auto up {static_cast<int>((comp + fMaxRangeElectrons) / ftpc->GetPadSide())};
    return {std::max(0, low), std::min(ref, up)};
}

double ActSim::TrackGenerator::SpreadCharge(const XYZPoint& p, double x, double y)
{
    // Convert pads to mm
    x *= ftpc->GetPadSide();
    y *= ftpc->GetPadSide();
    // Portions
    auto low {0.25 * ftpc->GetPadSide()};
    auto up {0.75 * ftpc->GetPadSide()};
    // 1-> low, low
    double val {TMath::Exp(0.5 * (TMath::Power((x + low - p.X()) / fGas->GetTransDiff(), 2) +
                                  TMath::Power((y + low - p.Y()) / fGas->GetTransDiff(), 2)))};
    // 2-> up, low
    val += TMath::Exp(0.5 * (TMath::Power((x + up - p.X()) / fGas->GetTransDiff(), 2) +
                             TMath::Power((y + low - p.Y()) / fGas->GetTransDiff(), 2)));
    // 3-> low, up
    val += TMath::Exp(0.5 * (TMath::Power((x + low - p.X()) / fGas->GetTransDiff(), 2) +
                             TMath::Power((y + up - p.Y()) / fGas->GetTransDiff(), 2)));
    // 4-> up, up
    val += TMath::Exp(0.5 * (TMath::Power((x + up - p.X()) / fGas->GetTransDiff(), 2) +
                             TMath::Power((y + up - p.Y()) / fGas->GetTransDiff(), 2)));
    return val;
}

void ActSim::TrackGenerator::Convert()
{
    for(const auto& track : fCloud)
    {
        for(const auto& raw : track)
        {
            auto pos {raw.GetPositionAs<double>()};
            auto q {raw.GetCharge()};
            // Number of electrons
            // DeltaE to eV divided by work func
            auto Ne {q * 1e6 / fGas->GetWork()};
            // Following ACTARSim, use poisson distribution
            Ne = fRand->PoissonD(Ne);
            // Get ranges in pads of point
            auto [minPadX, maxPadX] {GetPadRange(pos, "x")};
            auto [minPadY, maxPadY] {GetPadRange(pos, "y")};
            // Iterate
            for(int padx = minPadX; padx <= maxPadX; padx++)
            {
                for(int pady = minPadY; pady <= maxPadY; pady++)
                {
                    // Compute Z value: time bucket
                    auto tb {static_cast<int>((pos.Z()) / fGas->GetVDrift() * fSamplingFreq)};
                    // Append trigger time
                    tb += fStartTime;
                    // Break if outside range
                    if(tb < 0 || tb > ftpc->GetNPADSZUNREBIN())
                        break;
                    // Push
                    auto tq {6.55e-4 * 600 * Ne * 1. / 2 / TMath::Pi() / TMath::Power(fGas->GetTransDiff(), 2) * 1};
                    tq *= SpreadCharge(pos, padx, pady);
                    fData[{padx, pady}].push_back({tb, tq});
                }
            }
        }
    }
    FillMEvent();
}

void ActSim::TrackGenerator::Reset()
{
    fCloud.clear();
    fData.clear();
    fMEvent = {};
    fVertex = {-1, -1, -1};
}

void ActSim::TrackGenerator::Fill2D(TH2* h, const std::string& which)
{
    h->Reset();
    for(const auto& [pad, signal] : fData)
    {
        auto [x, y] {pad};
        for(const auto& vals : signal)
        {
            auto [z, q] {vals};
            if(which == "xy")
                h->Fill(x, y, q);
            else if(which == "xz")
                h->Fill(x, z, q);
            else
                h->Fill(y, z, q);
        }
    }
}

void ActSim::TrackGenerator::FillMEvent()
{
    for(const auto& [pad, signal] : fData)
    {
        auto [x, y] {pad};
        // Init legacy data
        ReducedData datared;
        // Convert to globalchannel
        auto [co, as, ag, ch] {fCalMan->ApplyInvLookUp(x, y)};
        datared.globalchannelid = ch + (ag << 7) + (as << 9) + (co << 11);
        // Append signal info
        for(const auto& vals : signal)
        {
            auto [tb, q] {vals};
            datared.peaktime.push_back(tb);
            datared.peakheight.push_back(q);
        }
        fMEvent.CoboAsad.push_back(datared);
    }
}
