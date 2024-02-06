#include "ActMergerDetector.h"

#include "ActColors.h"
#include "ActDetectorManager.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActModularDetector.h"
#include "ActMultiStep.h"
#include "ActOptions.h"
#include "ActSilData.h"
#include "ActSilDetector.h"
#include "ActSilSpecs.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"
#include "ActVData.h"

#include "RtypesCore.h"

#include "TEnv.h"
#include "TH1.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TStopwatch.h"
#include "TSystem.h"
#include "TTree.h"

#include "Math/RotationZYX.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

ActRoot::MergerDetector::MergerDetector()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

void ActRoot::MergerDetector::ReadConfiguration(std::shared_ptr<InputBlock> block)
{
    ////////////////// MergerDetector /////////////////////////
    // Read (mandatory) SilSpecs config file
    if(block->CheckTokenExists("IsEnabled"))
        fIsEnabled = block->GetBool("IsEnabled");
    if(block->CheckTokenExists("SilSpecsFile"))
        ReadSilSpecs(block->GetString("SilSpecsFile"));
    // Map GATCONFS to SilLayers, using gat command
    auto gatMap {block->GetMappedValuesVectorOf<std::string>("gat")};
    if(gatMap.size() > 0)
        fGatMap = gatMap;
    // Beam-like and multiplicities
    if(block->CheckTokenExists("ForceBeamLike"))
        fForceBeamLike = block->GetBool("ForceBeamLike");
    if(block->CheckTokenExists("NotBeamMults"))
        fNotBMults = block->GetIntVector("NotBeamMults");
    // Gate on XVertex
    if(block->CheckTokenExists("GateRPX"))
        fGateRPX = block->GetDouble("GateRPX");
    // Conversion to physical units
    if(block->CheckTokenExists("EnableConversion"))
        fEnableConversion = block->GetBool("EnableConversion");
    if(block->CheckTokenExists("DriftFactor"))
        fDriftFactor = block->GetDouble("DriftFactor");
    // Match SP to real placement
    if(block->CheckTokenExists("EnableMatch"))
        fEnableMatch = block->GetBool("EnableMatch");
    if(block->CheckTokenExists("MatchUseZ"))
        fMatchUseZ = block->GetBool("MatchUseZ");
    if(block->CheckTokenExists("ZOffset"))
        fZOffset = block->GetDouble("ZOffset");
    // Enable QProfile
    if(block->CheckTokenExists("EnableQProfile"))
        fEnableQProfile = block->GetBool("EnableQProfile");

    // Disable TH1::AddDirectory
    TH1::AddDirectory(false);

    // Init clocks
    InitClocks();

    //////////////// MultiStep algorithm ///////////////////////////
}

void ActRoot::MergerDetector::InitClocks()
{
    // Init labels
    fClockLabels.resize(7);
    fClockLabels[0] = "IsDoable";
    fClockLabels[1] = "LightOrHeavy";
    fClockLabels[2] = "SP computation";
    fClockLabels[3] = "ConvertToPhysicalUnits";
    fClockLabels[4] = "MatchSPtoRealPlacement";
    fClockLabels[5] = "Angles computation";
    fClockLabels[6] = "Qave and Qprofile";

    for(const auto& _ : fClockLabels)
        fClocks.push_back(TStopwatch {});
}

void ActRoot::MergerDetector::ReadCalibrations(std::shared_ptr<InputBlock> block) {}

void ActRoot::MergerDetector::Reconfigure()
{
    // Workaround: Reconfigure is intended to reset inner algorithms of detector,
    // but Merger is itself its detector, so parse again the input file
    InputParser parser {ActRoot::Options::GetInstance()->GetDetFile()};
    ReadConfiguration(parser.GetBlock(DetectorManager::GetDetectorTypeStr(DetectorType::EMerger)));
}

void ActRoot::MergerDetector::SetParameters(ActRoot::VParameters* pars)
{
    if(auto casted {dynamic_cast<TPCParameters*>(pars)}; casted)
        fTPCPars = casted;
    else if(auto casted {dynamic_cast<SilParameters*>(pars)}; casted)
        fSilPars = casted;
    else if(auto casted {dynamic_cast<ModularParameters*>(pars)}; casted)
        fModularPars = casted;
    else
        throw std::invalid_argument(
            "MergerDetector::SetParameters(): could not find a proper cast for the passed pointer");
}

void ActRoot::MergerDetector::InitInputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::InitOutputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    tree->SetBranchStatus("fRaw", false);
    // TPC physics
    if(fTPCData)
        delete fTPCData;
    fTPCData = new TPCData;
    tree->SetBranchAddress("TPCData", &fTPCData);

    // Silicon data
    if(fSilData)
        delete fSilData;
    fSilData = new SilData;
    tree->SetBranchAddress("SilData", &fSilData);

    // Modular data
    if(fModularData)
        delete fModularData;
    fModularData = new ModularData;
    tree->SetBranchAddress("ModularData", &fModularData);
}

void ActRoot::MergerDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fMergerData)
        delete fMergerData;
    fMergerData = new MergerData;
    if(tree)
        tree->Branch("MergerData", &fMergerData);
}

void ActRoot::MergerDetector::ReadSilSpecs(const std::string& file)
{
    fSilSpecs = std::make_shared<ActPhysics::SilSpecs>();
    fSilSpecs->ReadFile(file);
    // fSilSpecs->Print();
}

void ActRoot::MergerDetector::BuildEventFilter() {}


void ActRoot::MergerDetector::DoMerge()
{
    // Check if is enabled
    if(!fIsEnabled)
        return;
    fClocks[0].Start(false);
    auto isDoable {IsDoable()};
    fClocks[0].Stop();
    if(!isDoable)
    {
        fMergerData->Clear();
        return;
    }

    // 2-> Identify light and heavy
    fClocks[1].Start(false);
    LightOrHeavy();
    fClocks[1].Stop();

    // 3-> Compute SP and BP
    fClocks[2].Start(false);
    auto isSPOk {ComputeSiliconPoint()};
    fClocks[2].Stop();
    if(!isSPOk)
    {
        // this checks whether the SP is fine or not
        // probably bc the propagation does not occur in
        // the same sense of motion as defined by Line::fDirection
        fMergerData->Clear();
        return;
    }
    ComputeOtherPoints();
    // 4-> Scale points to physical dimensions
    // if conversion is disabled, no further steps can be done!
    if(!fEnableConversion)
        return;
    fClocks[3].Start(false);
    ConvertToPhysicalUnits();
    fClocks[3].Stop();

    // 5-> Match or not to silicon real placement
    if(fEnableMatch)
    {
        if(fMatchUseZ)
            CorrectZOffset();
        fClocks[4].Start(false);
        auto isMatch {MatchSPtoRealPlacement()};
        fClocks[4].Stop();
        if(!isMatch)
        {
            fMergerData->Clear();
            return;
        }
    }

    // 6-> Get angles
    fClocks[5].Start(false);
    ComputeAngles();
    fClocks[5].Stop();
    // 7-> Qave and charge profile computations
    fClocks[6].Start(false);
    ComputeQave();
    if(fEnableQProfile)
        ComputeQProfile();
    fClocks[6].Stop();
}

void ActRoot::MergerDetector::BuildEventData(int run, int entry)
{
    // Clone before next step if required
    if(fTPCClone)
        *fTPCClone = *fTPCData;
    // Reset clears iterators of MergerData and sets [run, entry]
    Reset(run, entry);
    // Merge
    DoMerge();
}

bool ActRoot::MergerDetector::IsDoable()
{
    auto condA {GateGATCONFandTrackMult() && GateOthers()};
    if(!condA)
        return condA;
    else
    {
        auto condB {GateSilMult()};
        return condB;
    }
}

bool ActRoot::MergerDetector::GateGATCONFandTrackMult()
{
    // 1-> Apply GATCONF cut
    bool isInGat {true};
    auto gat {(int)fModularData->Get("GATCONF")};
    if(fGatMap.count(gat))
        isInGat = true;
    else
        isInGat = false;
    // 2-> Has BL cluster and not BL multiplicity
    bool hasBL {true};
    bool hasMult {false};
    if(fForceBeamLike)
    {
        int bl {};
        int notBL {};
        for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
        {
            if(it->GetIsBeamLike())
            {
                fBeamIt = it;
                bl++;
            }
            else
                notBL++;
        }
        hasBL = (bl == 1); // admit only one BL
        hasMult = IsInVector(notBL, fNotBMults);
        // if(notBL > 2)
        // std::cout << "hasMult? " << std::boolalpha << hasMult << '\n';
    }
    else
    {
        hasMult = IsInVector((int)fTPCData->fClusters.size(), fNotBMults);
    }
    // 3-> Has RP (either preliminary or fine)
    bool hasRP {fTPCData->fRPs.size() > 0};
    return isInGat && hasBL && hasMult && hasRP;
}

bool ActRoot::MergerDetector::GateSilMult()
{
    // 1-> Apply finer thresholds in SilSpecs
    fSilData->ApplyFinerThresholds(fSilSpecs);
    // 2-> Check and write silicon data
    int withHits {};
    int withMult {};
    for(const auto& layer : fGatMap[(int)fModularData->Get("GATCONF")])
    {
        // Check only layers with hits over threshold!
        if(int mult {fSilData->GetMult(layer)}; mult > 0)
        {
            withHits++;
            if(mult == 1) // and now add to multiplicity == 1 counter
            {
                withMult++;
                // Write data
                fMergerData->fSilLayers.push_back(layer);
                fMergerData->fSilEs.push_back(fSilData->fSiE[layer].front());
                fMergerData->fSilNs.push_back(fSilData->fSiN[layer].front());
            }
        }
    }
    return (withHits == withMult) &&
           (withHits > 0); // bugfix: whitHits > 0 to avoid case in which GATCONF ok but no silicon hit above threshold!
}

bool ActRoot::MergerDetector::GateOthers()
{
    bool condRPX {false};
    if(fTPCData->fRPs.size() > 0)
        condRPX = fTPCData->fRPs.front().X() > fGateRPX;
    return condRPX;
}

void ActRoot::MergerDetector::Reset(const int& run, const int& entry)
{
    // Reset iterators before moving on to work with them
    fBeamIt = fTPCData->fClusters.end();
    fLightIt = fTPCData->fClusters.end();
    fHeavyIt = fTPCData->fClusters.end();
    // Reset other variables
    fMergerData->fRun = run;
    fMergerData->fEntry = entry;
}

double ActRoot::MergerDetector::GetTheta3D(const XYZVector& beam, const XYZVector& other)
{
    auto dot {beam.Unit().Dot(other.Unit())};
    return TMath::ACos(dot) * TMath::RadToDeg();
}

double ActRoot::MergerDetector::GetPhi3D(const XYZVector& beam, const XYZVector& other)
{
    // TODO: Check validity of phi calculation

    // auto ub {beam.Unit()};            // unitary beam
    auto trackUnitary {other.Unit()};
    // XYZVector yz {0, ub.Y(), ub.Z()}; // beam dir in YZ plane
    // auto dot {other.Unit().Dot(yz) / yz.R()};
    // return TMath::ACos(dot) * TMath::RadToDeg();
    return TMath::ATan2(trackUnitary.Y(), trackUnitary.Z()) * TMath::RadToDeg();
}

void ActRoot::MergerDetector::LightOrHeavy()
{
    // Firstly, set sign of X direction of BL to be always positive
    fMergerData->fRP = fTPCData->fRPs.front();
    auto& refLine {fBeamIt->GetRefToLine()};
    const auto& oldDir {refLine.GetDirection()};
    refLine.SetDirection({std::abs(oldDir.X()), oldDir.Y(), oldDir.Z()});
    // Rank by larger angle
    // .first = angle; .second = index; larger angles at begin
    auto lambda {[](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first > b.first; }};
    std::set<std::pair<double, int>, decltype(lambda)> set(lambda);
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
    {
        auto it {fTPCData->fClusters.begin() + i};
        if(it == fBeamIt)
            continue;
        // Orient track following RP and gravity point
        it->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
        // Get angle
        auto theta {GetTheta3D(fBeamIt->GetLine().GetDirection(), it->GetLine().GetDirection())};
        set.insert({TMath::Abs(theta), i});
    }
    if(fIsVerbose)
    {
        std::cout << BOLDCYAN << "---- Merger LightOrHeavy ----" << '\n';
        for(const auto& pair : set)
            std::cout << "Theta : " << pair.first << " at : " << pair.second << '\n';
        std::cout << "------------------------------" << RESET << '\n';
    }
    // Set iterators
    fLightIt = fTPCData->fClusters.begin() + set.begin()->second;
    if(set.size() > 1)
        fHeavyIt = fTPCData->fClusters.begin() + std::next(set.begin())->second;
}

bool ActRoot::MergerDetector::ComputeSiliconPoint()
{
    // Align cluster according to RP
    fLightIt->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
    // Compute SP
    bool isOk {};
    std::tie(fMergerData->fSP, isOk) =
        fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
            .GetSiliconPointOfTrack(fLightIt->GetLine().GetPoint(), fLightIt->GetLine().GetDirection());
    // And compute track length
    fMergerData->fTrackLength = (fMergerData->fSP - fMergerData->fRP).R();
    return isOk;
}

void ActRoot::MergerDetector::MoveZ(XYZPoint& p)
{
    p.SetZ(p.Z() + fZOffset);
}

void ActRoot::MergerDetector::CorrectZOffset()
{
    // Basically for all points
    // 1-> RP
    MoveZ(fMergerData->fRP);
    // 2-> SP
    MoveZ(fMergerData->fSP);
    // 3-> Por all iterators
    for(auto& it : {fBeamIt, fLightIt, fHeavyIt})
    {
        if(it == fTPCData->fClusters.end())
            continue;
        auto p {it->GetLine().GetPoint()};
        MoveZ(p);
        it->GetRefToLine().SetPoint(p);
    }
}

bool ActRoot::MergerDetector::MatchSPtoRealPlacement()
{
    // Use only first value in std::vector<int> of Ns
    auto n {fMergerData->fSilNs.front()};
    auto layer {fMergerData->fSilLayers.front()};
    // And check!
    return fSilSpecs->GetLayer(layer).MatchesRealPlacement(n, fMergerData->fSP, fMatchUseZ);
}

ActRoot::MergerDetector::XYZVector ActRoot::MergerDetector::RotateTrack(XYZVector beam, XYZVector track)
{
    // Ensure unitary vecs
    beam = beam.Unit();
    track = track.Unit();
    // Compute rotated angles
    auto z {TMath::ATan2(beam.Y(), beam.X())};
    auto y {TMath::ATan2(beam.Z(), beam.X())};
    auto x {TMath::ATan2(beam.Z(), beam.Y())};

    ROOT::Math::RotationZYX rot {-z, -y, -x}; // following ACTAR's ref frame
    return rot(track).Unit();
}

void ActRoot::MergerDetector::ScalePoint(XYZPoint& point, float xy, float z, bool addOffset)
{
    if(addOffset) // when converting a bin point to physical units which wasnt already corrected
        point += XYZVector {0.5, 0.5, 0.5};
    point.SetX(point.X() * xy);
    point.SetY(point.Y() * xy);
    point.SetZ(point.Z() * z);
}

void ActRoot::MergerDetector::ConvertToPhysicalUnits()
{
    // Convert points
    auto xy {fTPCPars->GetPadSide()};
    ScalePoint(fMergerData->fWP, xy, fDriftFactor);
    ScalePoint(fMergerData->fRP, xy, fDriftFactor);
    ScalePoint(fMergerData->fBP, xy, fDriftFactor);
    ScalePoint(fMergerData->fSP, xy, fDriftFactor);

    // Scale Line in Clusters
    for(auto& it : {fBeamIt, fLightIt, fHeavyIt})
    {
        if(it != fTPCData->fClusters.end())
            it->GetRefToLine().Scale(xy, fDriftFactor);
    }

    // And recompute track length
    fMergerData->fTrackLength = (fMergerData->fSP - fMergerData->fRP).R();
}

void ActRoot::MergerDetector::ComputeAngles()
{
    // Theta Light
    fMergerData->fThetaLight = GetTheta3D(fBeamIt->GetLine().GetDirection(), fLightIt->GetLine().GetDirection());
    fMergerData->fThetaLegacy = fMergerData->fThetaLight;
    // Debug: angle computed assuming beam exactly along X axis
    fMergerData->fThetaDebug = GetTheta3D({1, 0, 0}, fLightIt->GetLine().GetDirection());
    // Phi Light
    fMergerData->fPhiLight = GetPhi3D(fBeamIt->GetLine().GetDirection(), fLightIt->GetLine().GetDirection());
    // Beam angles
    auto beamDir {fBeamIt->GetLine().GetDirection().Unit()};
    fMergerData->fThetaBeam = GetTheta3D({1, 0, 0}, beamDir);
    fMergerData->fThetaBeamZ = TMath::ATan(beamDir.Z() / beamDir.X()) * TMath::RadToDeg();
    fMergerData->fPhiBeamY = TMath::ATan(beamDir.Y() / beamDir.X()) * TMath::RadToDeg();
    // Theta Heavy
    if(fBeamIt != fTPCData->fClusters.end())
        fMergerData->fThetaHeavy = GetTheta3D(fBeamIt->GetLine().GetDirection(), fHeavyIt->GetLine().GetDirection());
}

void ActRoot::MergerDetector::ComputeOtherPoints()
{
    // Boundary point: light track at ACTAR's flanges
    fMergerData->fBP = fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
                           .GetBoundaryPointOfTrack(fTPCPars, fLightIt->GetLine().GetPoint(),
                                                    fLightIt->GetLine().GetDirection().Unit());
    // Window point: beam entrance point at X = 0 from fit parameters
    fMergerData->fWP = fBeamIt->GetLine().MoveToX(0);
}

void ActRoot::MergerDetector::ComputeQave()
{
    std::sort(fLightIt->GetRefToVoxels().begin(), fLightIt->GetRefToVoxels().end());
    // Get min
    auto front {fLightIt->GetVoxels().front().GetPosition()};
    auto back {fLightIt->GetVoxels().back().GetPosition()};
    // Scale them
    ScalePoint(front, fTPCPars->GetPadSide(), fDriftFactor, true);
    ScalePoint(back, fTPCPars->GetPadSide(), fDriftFactor, true);
    // Get projections
    auto min {fLightIt->GetLine().ProjectionPointOnLine(front)};
    auto max {fLightIt->GetLine().ProjectionPointOnLine(back)};
    // Convert them to physical points
    // ScalePoint(min, fTPCPars->GetPadSide(), fDriftFactor);
    // ScalePoint(max, fTPCPars->GetPadSide(), fDriftFactor);
    // Dist in mm
    auto dist {(max - min).R()};
    // auto dist {(fMergerData->fRP - fMergerData->fBP).R()};
    // std::cout << "=========================" << '\n';
    // std::cout << "front : " << front << '\n';
    // std::cout << "back  : " << back << '\n';
    // std::cout << "min : " << min << '\n';
    // std::cout << "max : " << max << '\n';
    // std::cout << "dist (max - min) : " << (max - min).R() << '\n';
    // std::cout << "RP : " << fMergerData->fRP << '\n';
    // std::cout << "BP : " << fMergerData->fBP << '\n';
    // std::cout << "dist (RP - BP) : " << (fMergerData->fRP - fMergerData->fBP).R() << '\n';
    // Sum to obtain total Q
    auto qTotal {std::accumulate(fLightIt->GetVoxels().begin(), fLightIt->GetVoxels().end(), 0.f,
                                 [](float sum, const Voxel& v) { return sum + v.GetCharge(); })};
    // std::cout << "Qtotal : " << qTotal << '\n';
    // std::cout << "Qtotal / dist : " << qTotal / dist << '\n';
    fMergerData->fQave = qTotal / dist;
}

void ActRoot::MergerDetector::ComputeQProfile()
{
    // 0-> Init histogram
    TH1F h {"hQProfile", "QProfile", 100, -5, 150};
    h.SetTitle("QProfile;dist [mm];Q [au]");
    // Voxels should be already ordered
    // 1-> Ref point is vector.begin() projection on line
    auto front {fLightIt->GetVoxels().front().GetPosition()};
    // Convert it to physical units
    ScalePoint(front, fTPCPars->GetPadSide(), fDriftFactor, true);
    auto ref {fLightIt->GetLine().ProjectionPointOnLine(front)};
    // Use 3 divisions in voxel to obtain a better profile
    float div {1.f / 3};
    for(const auto& v : fLightIt->GetVoxels())
    {
        const auto& pos {v.GetPosition()};
        auto q {v.GetCharge()};
        // Run for 3 divisions
        for(int ix = -1; ix < 2; ix++)
        {
            for(int iy = -1; iy < 2; iy++)
            {
                for(int iz = -1; iz < 2; iz++)
                {
                    XYZPoint bin {pos.X() + ix * div, pos.Y() + iy * div, pos.Z() + iz * div};
                    // Convert to physical units
                    ScalePoint(bin, fTPCPars->GetPadSide(), fDriftFactor, true);
                    // Project it on line
                    auto proj {fLightIt->GetLine().ProjectionPointOnLine(bin)};
                    // Fill histograms
                    auto dist {(proj - ref).R()};
                    h.Fill(dist, q / 27);
                }
            }
        }
    }
    fMergerData->fQProf = h;
}

void ActRoot::MergerDetector::ClearEventFilter() {}

void ActRoot::MergerDetector::ClearEventData()
{
    fMergerData->Clear();
}

void ActRoot::MergerDetector::Print() const
{

    std::cout << BOLDYELLOW << ":::: Merger detector ::::" << '\n';
    std::cout << "-> IsEnabled     ? " << std::boolalpha << fIsEnabled << '\n';
    if(fIsEnabled)
    {
        std::cout << "-> GATCONF map   : " << '\n';
        for(const auto& [key, vals] : fGatMap)
        {
            std::cout << "   " << key << " = ";
            for(const auto& s : vals)
                std::cout << s << ", ";
            std::cout << '\n';
        }
        std::cout << "-> ForceBeamLike ? " << std::boolalpha << fForceBeamLike << '\n';
        std::cout << "-> NotBeamMults  : ";
        for(const auto& m : fNotBMults)
            std::cout << m << ", ";
        std::cout << '\n';
        std::cout << "-> GateRPX       : " << fGateRPX << '\n';
        std::cout << "-> EnableMatch   ? " << std::boolalpha << fEnableMatch << '\n';
        std::cout << "-> MatchUseZ     ? " << std::boolalpha << fMatchUseZ << '\n';
        std::cout << "-> MatchZOffset  : " << fZOffset << '\n';
        std::cout << "-> EnableQProf   ? " << std::boolalpha << fEnableQProfile << '\n';
    }
    // fSilSpecs->Print();
    std::cout << "::::::::::::::::::::::::" << RESET << '\n';
}

void ActRoot::MergerDetector::PrintReports() const
{
    std::cout << BOLDCYAN << "==== MergerDetector time report ====" << '\n';
    for(int i = 0; i < fClockLabels.size(); i++)
    {
        std::cout << "Timer : " << fClockLabels[i] << '\n';
        fClocks[i].Print();
    }
    std::cout << RESET << '\n';
}
