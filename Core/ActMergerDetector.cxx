#include "ActMergerDetector.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActModularDetector.h"
#include "ActMultiStep.h"
#include "ActSilData.h"
#include "ActSilDetector.h"
#include "ActSilSpecs.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActVData.h"

#include "TEnv.h"
#include "TH1.h"
#include "TMath.h"
#include "TMathBase.h"
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
#include <utility>
#include <vector>

ActRoot::MergerDetector::MergerDetector()
{
    fMultiStep = std::make_shared<ActCluster::MultiStep>();
}

void ActRoot::MergerDetector::ReadConfiguration(std::shared_ptr<InputBlock> block)
{
    ////////////////// MergerDetector /////////////////////////
    // automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/merger.conf";
    if(gSystem->AccessPathName(envfile.c_str()))
    {
        throw std::runtime_error(
            "MergerDetector::ReadConfiguration: could not locate conf.merger in ActRoot's configs path!");
    }
    // Parse!
    ActRoot::InputParser parser {envfile};
    block = parser.GetBlock("Merger");
    // Read (mandatory) SilSpecs config file
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

    //////////////// MultiStep algorithm ///////////////////////////
    fMultiStep->ReadConfigurationFile();
}

void ActRoot::MergerDetector::ReadCalibrations(std::shared_ptr<InputBlock> block) {}

void ActRoot::MergerDetector::Reconfigure()
{
    ReadConfiguration(nullptr);
}

void ActRoot::MergerDetector::SetEventData(ActRoot::VData* vdata)
{
    if(auto casted {dynamic_cast<TPCData*>(vdata)}; casted)
        fTPCData = casted;
    if(auto casted {dynamic_cast<SilData*>(vdata)}; casted)
        fSilData = casted;
    if(auto casted {dynamic_cast<ModularData*>(vdata)}; casted)
        fModularData = casted;
}

void ActRoot::MergerDetector::InitInputRaw(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::InitOutputData(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::InitInputMerger(std::shared_ptr<TTree> tree)
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

void ActRoot::MergerDetector::InitOutputMerger(std::shared_ptr<TTree> tree)
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

void ActRoot::MergerDetector::BuildEventData() {}

void ActRoot::MergerDetector::DoMultiStep()
{
    // Send data to MultiStep
    fMultiStep->SetClusters(&fTPCData->fClusters);
    fMultiStep->SetRPs(&fTPCData->fRPs);
    fMultiStep->Run();
}

void ActRoot::MergerDetector::DoMerge()
{
    // 1-> Reset iterators
    Reset();
    if(!IsDoable())
    {
        fMergerData->Clear();
        return;
    }
    // 2-> Identify light and heavy
    LightOrHeavy();
    // 3-> Compute SP and BP
    ComputeBoundaryPoint();
    ComputeSiliconPoint();
    // 4-> Scale points to physical dimensions
    // if conversion is disabled, no further steps can be done!
    if(!fEnableConversion)
        return;
    ConvertToPhysicalUnits();
    // 5-> Match or not to silicon real placement
    if(fEnableMatch)
    {
        if(fMatchUseZ)
            CorrectZOffset();
        if(!MatchSPtoRealPlacement())
        {
            fMergerData->Clear();
            return;
        }
    }
    // 6-> Get angles
    ComputeAngles();
    // 7-> Qave and charge profile computations
    ComputeQave();
    if(fEnableQProfile)
        ComputeQProfile();
}

void ActRoot::MergerDetector::BuildEventMerger()
{
    // MultiStep
    DoMultiStep();
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
    // 2-> Has BL cluster and not BL mult
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
    return isInGat && hasBL && hasMult;
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

void ActRoot::MergerDetector::Reset()
{
    // Reset iterators before moving on to work with them
    fBeamIt = fTPCData->fClusters.end();
    fLightIt = fTPCData->fClusters.end();
    fHeavyIt = fTPCData->fClusters.end();
    // Reset other variables
    fMergerData->fRun = fCurrentRun;
    fMergerData->fEntry = fCurrentEntry;
}

double ActRoot::MergerDetector::GetTheta3D(const XYZVector& beam, const XYZVector& other)
{
    auto dot {beam.Unit().Dot(other.Unit())};
    return TMath::ACos(dot) * TMath::RadToDeg();
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
        // Get angle
        auto theta {GetTheta3D(fBeamIt->GetLine().GetDirection(), it->GetLine().GetDirection())};
        set.insert({TMath::Abs(theta), i});
    }
    // for(const auto& pair : set)
    //     std::cout << "Theta : " << pair.first << " at : " << pair.second << '\n';
    // Set iterators
    fLightIt = fTPCData->fClusters.begin() + set.begin()->second;
    if(set.size() > 1)
        fHeavyIt = fTPCData->fClusters.begin() + std::next(set.begin())->second;
}

void ActRoot::MergerDetector::ComputeSiliconPoint()
{
    fMergerData->fSP =
        fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
            .GetSiliconPointOfTrack(fLightIt->GetLine().GetPoint(), fLightIt->GetLine().GetDirection().Unit());
    // Redefine signs just in case
    fLightIt->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
    // fMergerData->Print();
    // And compute track length
    fMergerData->fTrackLength = (fMergerData->fSP - fMergerData->fRP).R();
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

void ActRoot::MergerDetector::ScalePoint(XYZPoint& point, float xy, float z)
{
    point.SetX(point.X() * xy);
    point.SetY(point.Y() * xy);
    point.SetZ(point.Z() * z);
}

void ActRoot::MergerDetector::ConvertToPhysicalUnits()
{
    // Convert points
    auto xy {fTPCPars->GetPadSide()};
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
    // Using the simpler 3D version
    auto theta {TMath::ACos(fBeamIt->GetLine().GetDirection().Unit().Dot(fLightIt->GetLine().GetDirection().Unit()))};
    theta *= TMath::RadToDeg();
    // For phi, we use {0, 0, 1} as reference
    auto phi {TMath::ACos(XYZVector {0, 0, 1}.Dot(fLightIt->GetLine().GetDirection().Unit()))};
    phi *= TMath::RadToDeg();
    // Write to MergerData
    fMergerData->fThetaLight = theta;
    fMergerData->fPhiLight = phi;
}

void ActRoot::MergerDetector::ComputeBoundaryPoint()
{
    fMergerData->fBP = fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
                           .GetBoundaryPointOfTrack(fTPCPars, fLightIt->GetLine().GetPoint(),
                                                    fLightIt->GetLine().GetDirection().Unit());
}

void ActRoot::MergerDetector::ComputeQave()
{
    std::sort(fLightIt->GetRefToVoxels().begin(), fLightIt->GetRefToVoxels().end());
    // Get min
    auto min {fLightIt->GetLine().ProjectionPointOnLine(fLightIt->GetVoxels().front().GetPosition())};
    auto max {fLightIt->GetLine().ProjectionPointOnLine(fLightIt->GetVoxels().back().GetPosition())};
    // Convert them to physical points
    ScalePoint(min, fTPCPars->GetPadSide(), fDriftFactor);
    ScalePoint(max, fTPCPars->GetPadSide(), fDriftFactor);
    // Dist in mm
    auto dist {(max - min).R()};
    // Sum to obtain total Q
    auto qTotal {std::accumulate(fLightIt->GetVoxels().begin(), fLightIt->GetVoxels().end(), 0.f,
                                 [](float sum, const Voxel& v) { return sum + v.GetCharge(); })};
    fMergerData->fQave = qTotal / dist;
}

void ActRoot::MergerDetector::ComputeQProfile()
{
    // 0-> Init histogram
    TH1F h {"hQProfile", "QProfile", 100, -5, 150};
    h.SetTitle("QProfile;dist [mm];Q [au]");
    // Voxels should be already ordered
    // 1-> Ref point is vector.begin() projection on line
    auto ref {fLightIt->GetLine().ProjectionPointOnLine(fLightIt->GetVoxels().front().GetPosition())};
    // Convert to physical units
    ScalePoint(ref, fTPCPars->GetPadSide(), fDriftFactor);
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
                    // Project it on line
                    auto proj {fLightIt->GetLine().ProjectionPointOnLine(bin)};
                    ScalePoint(proj, fTPCPars->GetPadSide(), fDriftFactor);
                    auto dist {(proj - ref).R()};
                    h.Fill(dist, q / 27);
                }
            }
        }
    }
    fMergerData->fQProf = h;
}

void ActRoot::MergerDetector::ClearEventData() {}

void ActRoot::MergerDetector::ClearEventMerger()
{
    fMergerData->Clear();
}

void ActRoot::MergerDetector::Print() const
{
    fMultiStep->Print();

    std::cout << BOLDYELLOW << ":::: Merger detector ::::" << '\n';
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
    // fSilSpecs->Print();
    std::cout << "::::::::::::::::::::::::" << RESET << '\n';
}

void ActRoot::MergerDetector::PrintReports() const {}
