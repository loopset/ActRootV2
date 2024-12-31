#include "ActMergerDetector.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActCorrector.h"
#include "ActDetectorManager.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActOptions.h"
#include "ActSilData.h"
#include "ActSilSpecs.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"
#include "ActVoxel.h"

#include "TF1.h"
#include "TH1.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TSpline.h"
#include "TStopwatch.h"
#include "TTree.h"

#include "Math/RotationZYX.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
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

ActRoot::MergerDetector::~MergerDetector()
{
    if(fDelTPCSilMod)
    {
        delete fTPCData;
        fTPCData = nullptr;
        delete fSilData;
        fSilData = nullptr;
        delete fModularData;
        fModularData = nullptr;
    }
    if(fDelMerger)
    {
        delete fMergerData;
        fMergerData = nullptr;
    }
}

void ActRoot::MergerDetector::ReadConfiguration(std::shared_ptr<InputBlock> block)
{
    ////////////////// MergerDetector /////////////////////////
    if(block->CheckTokenExists("IsEnabled"))
        fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    // Read silicon specs
    if(block->CheckTokenExists("SilSpecsFile", !fIsEnabled))
        ReadSilSpecs(block->GetString("SilSpecsFile"));
    if(block->CheckTokenExists("ForceGATCONF", !fIsEnabled))
        fForceGATCONF = block->GetBool("ForceGATCONF");
    // Map GATCONFS to SilLayers, using gat command
    if(fForceGATCONF)
    {
        auto gatMap {block->GetMappedValuesVectorOf<std::string>("gat")};
        if(gatMap.size() > 0)
            fGatMap = gatMap;
    }
    // Beam-like and multiplicities
    if(block->CheckTokenExists("ForceRP", !fIsEnabled))
        fForceRP = block->GetBool("ForceRP");
    if(block->CheckTokenExists("ForceBeamLike", !fIsEnabled))
        fForceBeamLike = block->GetBool("ForceBeamLike");
    if(block->CheckTokenExists("NotBeamMults", !fIsEnabled))
        fNotBMults = block->GetIntVector("NotBeamMults");
    // Conversion to physical units
    if(block->CheckTokenExists("EnableConversion", !fIsEnabled))
        fEnableConversion = block->GetBool("EnableConversion");
    if(block->CheckTokenExists("DriftFactor", !fIsEnabled))
        fDriftFactor = block->GetDouble("DriftFactor");
    // Match SP to real placement
    if(block->CheckTokenExists("EnableMatch", !fIsEnabled))
        fEnableMatch = block->GetBool("EnableMatch");
    if(block->CheckTokenExists("MatchUseZ", !fIsEnabled))
        fMatchUseZ = block->GetBool("MatchUseZ");
    if(block->CheckTokenExists("ZOffset", !fIsEnabled))
        fZOffset = block->GetDouble("ZOffset");
    // Enable QProfile
    if(block->CheckTokenExists("EnableQProfile", !fIsEnabled))
        fEnableQProfile = block->GetBool("EnableQProfile");
    if(block->CheckTokenExists("2DProfile", !fIsEnabled))
        f2DProfile = block->GetBool("2DProfile");
    if(block->CheckTokenExists("EnableRootFind", !fIsEnabled))
        fEnableRootFind = block->GetBool("EnableRootFind");
    if(block->CheckTokenExists("EnableDefaultBeam", !fIsEnabled))
        fEnableDefaultBeam = block->GetBool("EnableDefaultBeam");
    if(block->CheckTokenExists("DefaultBeamXThresh", !fEnableDefaultBeam))
        fDefaultBeamXThresh = block->GetDouble("DefaultBeamXThresh");
    if(block->CheckTokenExists("InvertAngle", !fIsEnabled))
        fInvertAngle = block->GetBool("InvertAngle");

    // Build or not filter method
    if(ActRoot::Options::GetInstance()->GetMode() == ModeType::ECorrect)
        InitCorrector();

    // Disable TH1::AddDirectory
    TH1::AddDirectory(false);

    // Init clocks
    InitClocks();
}

void ActRoot::MergerDetector::InitCorrector()
{
    fFilter = std::make_shared<ActAlgorithm::Corrector>();
    fFilter->ReadConfiguration();
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

void ActRoot::MergerDetector::InitInputFilter(std::shared_ptr<TTree> tree)
{
    if(fMergerData)
        delete fMergerData;
    fMergerData = new MergerData;
    tree->SetBranchAddress("MergerData", &fMergerData);
    // Set to delete
    fDelMerger = true;
}

void ActRoot::MergerDetector::InitOutputFilter(std::shared_ptr<TTree> tree)
{
    tree->Branch("MergerData", &fMergerData);
}

void ActRoot::MergerDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    tree->SetBranchStatus("fRaw*", false);
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

    // Set to delete all these new
    fDelTPCSilMod = true;
}

void ActRoot::MergerDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fMergerData)
        delete fMergerData;
    fMergerData = new MergerData;
    if(tree)
        tree->Branch("MergerData", &fMergerData);

    // Set to delete this
    fDelMerger = true;
}

void ActRoot::MergerDetector::ReadSilSpecs(const std::string& file)
{
    fSilSpecs = std::make_shared<ActPhysics::SilSpecs>();
    fSilSpecs->ReadFile(file);
    // fSilSpecs->Print();
}

void ActRoot::MergerDetector::BuildEventFilter()
{
    if(fFilter)
    {
        fFilter->SetMergerData(fMergerData);
        fFilter->Run();
    }
}


void ActRoot::MergerDetector::DoMerge()
{
    // Check if is enabled
    if(!fIsEnabled)
        return;
    fClocks[0].Start(false);
    auto isDoable {IsDoable()};
    fClocks[0].Stop();
    // Always print Merger configuration
    if(fIsVerbose)
        fPars.Print();
    if(!isDoable)
    {
        if(fIsVerbose)
            std::cout << BOLDRED << "  Event is not doable, skipping" << RESET << '\n';
        fMergerData->Clear();
        // INFO: fFlag is written within IsDoable function
        return;
    }

    // 1-> Default beam?
    DefaultBeamDirection();

    // 2-> Identify light and heavy
    fClocks[1].Start(false);
    LightOrHeavy();
    fClocks[1].Stop();

    // 2.1-> Compute BSP from a X profile
    if(fEnableQProfile)
        ComputeXProfile();

    // 3-> Compute SP and BP
    fClocks[2].Start(false);
    auto isSPOk {ComputeSiliconPoint()};
    fClocks[2].Stop();
    if(!isSPOk)
    {
        // this checks whether the SP is fine or not
        // probably bc the propagation does not occur in
        // the same sense of motion as defined by Line::fDirection
        if(fIsVerbose)
            std::cout << BOLDRED << "MergerDetector::Run(): SP is not OK, skipping event" << RESET << '\n';
        fMergerData->Clear();
        fMergerData->fFlag = "SP not ok";
        return;
    }
    ComputeOtherPoints();

    fClocks[6].Start(false);
    // 3.1-> Qave
    ComputeQave();
    // 3.2 -> QProfile that computes BraggPoint
    if(fEnableQProfile)
        ComputeQProfile();
    fClocks[6].Stop();

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
            fMergerData->fFlag = "SP not matched";
            return;
        }
    }

    // 6-> Get angles
    fClocks[5].Start(false);
    ComputeAngles();
    fClocks[5].Stop();
    // 7-> Everything went fine!
    fMergerData->fFlag = "ok";
}

void ActRoot::MergerDetector::BuildEventData(int run, int entry)
{
    // Reset clears iterators of MergerData and sets [run, entry]
    Reset(run, entry);
    // Merge
    DoMerge();
}

bool ActRoot::MergerDetector::IsDoable()
{
    auto condA {GateGATCONFandTrackMult()};
    if(!condA)
        return condA;
    else
    {
        auto condB {GateSilMult()};
        if(!condB)
            fMergerData->fFlag = "not Sil mult";
        return condB;
    }
}

bool ActRoot::MergerDetector::GateGATCONFandTrackMult()
{
    // 1-> Apply GATCONF cut
    bool isInGat {true};
    if(fForceGATCONF)
    {
        auto gat {(int)fModularData->Get("GATCONF")};
        if(fGatMap.count(gat))
        {
            isInGat = true;
            // Check if L1 trigger
            if(IsInVector({"L1"}, fGatMap[gat]))
                fPars.fIsL1 = true;
        }
        else
            isInGat = false;
    }
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
                fBeamPtr = &(*it);
                fMergerData->fBeamIdx = std::distance(fTPCData->fClusters.begin(), it);
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
    bool hasRP {true};
    if(fForceRP && fTPCData->fClusters.size() > 1)
    {
        fPars.fUseRP = true;
        hasRP = fTPCData->fRPs.size() > 0;
    }
    else
        fPars.fUseRP = false;
    // 4-> Set calibration mode
    if(!fForceRP && !fPars.fIsL1 && fTPCData->fClusters.size() == 1)
        fPars.fIsCal = true;
    // Verbose info
    if(fIsVerbose)
    {
        std::cout << BOLDCYAN << "---- Merge valitation 1 ----" << '\n';
        std::cout << " -> IsInGATCONF  ? " << std::boolalpha << isInGat << '\n';
        std::cout << " -> HasBeamLike  ? " << std::boolalpha << hasBL << '\n';
        std::cout << " -> HasTrackMult ? " << std::boolalpha << hasMult << '\n';
        std::cout << " -> HasRP        ? " << std::boolalpha << hasRP << '\n';
        // fPars.Print();
    }
    // Set flag
    if(!isInGat)
        fMergerData->fFlag = "not in GATCONF";
    else if(!hasBL)
        fMergerData->fFlag = "no Beam-like";
    else if(!hasMult)
        fMergerData->fFlag = "no track mult";
    else if(!hasMult)
        fMergerData->fFlag = "no RP";
    else
        ;
    return isInGat && hasBL && hasMult && hasRP;
}


bool ActRoot::MergerDetector::GateSilMult()
{
    if(fPars.fIsL1)
        return true;
    // If not calibration mode
    if(!fPars.fIsCal)
    {
        // 1-> Apply finer thresholds in SilSpecs
        fSilData->ApplyFinerThresholds(fSilSpecs);
        // 2-> Check and write silicon data
        int withHits {};
        int withMult {};
        for(const auto& layer : (fForceGATCONF ? fGatMap[(int)fModularData->Get("GATCONF")] : fSilData->GetLayers()))
        {
            // Check if layer indeed exists (L1 trigger not registered in silicon)
            if(!fSilSpecs->CheckLayersExists(layer))
                continue;
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

        bool condHitsPerLayer {withHits == withMult};
        bool condHits {withHits >
                       0}; // bugfix: whitHits > 0 to avoid case in which GATCONF ok but no silicon hit above threshold!

        if(fIsVerbose)
        {
            std::cout << BOLDCYAN << "---- Merge valitation 2 ----" << '\n';
            std::cout << "-> HasSilHits         ? " << std::boolalpha << condHits << '\n';
            std::cout << "-> HasMult1PerLayer   ? " << std::boolalpha << condHitsPerLayer << '\n';
        }
        return condHits && condHitsPerLayer;
    }
    else
    {
        // If is calibration just get the maximum of all elements
        for(const auto& layer : fSilData->GetLayers())
        {
            auto itMax {std::max_element(fSilData->fSiE[layer].begin(), fSilData->fSiE[layer].end())};
            auto idx {std::distance(fSilData->fSiE[layer].begin(), itMax)};
            // Write data
            fMergerData->fSilLayers.push_back(layer);
            fMergerData->fSilEs.push_back(*itMax);
            fMergerData->fSilNs.push_back(fSilData->fSiN[layer][idx]);
        }
        return (fMergerData->fSilLayers.size() > 0);
    }
}

void ActRoot::MergerDetector::Reset(const int& run, const int& entry)
{
    // Reset parameters
    fPars = {};
    // Reset pointers before moving on to work with them
    fBeamPtr = nullptr;
    fLightPtr = nullptr;
    fHeavyPtr = nullptr;
    // Reset other variables
    fMergerData->fRun = run;
    fMergerData->fEntry = entry;
}

void ActRoot::MergerDetector::DefaultBeamDirection()
{
    if(fBeamPtr && fEnableDefaultBeam)
    {
        auto [xmin, xmax] {fBeamPtr->GetXRange()};
        if((xmax - xmin) < fDefaultBeamXThresh)
        {
            fBeamPtr->GetRefToLine().SetDirection({1, 0, 0});
            if(fIsVerbose)
            {
                std::cout << BOLDCYAN << "---- Merger DefaultBeam ----" << '\n';
                std::cout << "-> Setting {1, 0 ,0} dir for cluster #" << fBeamPtr->GetClusterID() << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
        }
    }
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
    // 0-> If calibration, go straigth to unique cluster
    if(fPars.fIsCal)
    {
        fLightPtr = &(fTPCData->fClusters.front());
        fMergerData->fLightIdx = 0;
        // Sort and align
        std::sort(fLightPtr->GetRefToVoxels().begin(), fLightPtr->GetRefToVoxels().end());
        fLightPtr->GetRefToLine().AlignUsingPoint(fLightPtr->GetRefToVoxels().front().GetPosition(), true);
        return;
    }
    // If no beam like, just set light ptr
    if(!fBeamPtr)
    {
        fLightPtr = &(fTPCData->fClusters.front());
        fMergerData->fLightIdx = 0;
        // Sort and align
        std::sort(fLightPtr->GetRefToVoxels().begin(), fLightPtr->GetRefToVoxels().end());
        fLightPtr->GetRefToLine().AlignUsingPoint(fLightPtr->GetRefToVoxels().front().GetPosition(), true);
        if(fIsVerbose)
        {
            std::cout << BOLDCYAN << "---- Merger LightOrHeavy ----" << '\n';
            std::cout << "  Setting directly cluster #" << fLightPtr->GetClusterID() << " as light" << RESET << '\n';
            std::cout << "------------------------------" << RESET << '\n';
        }
        return;
    }
    // 1-> Set RP
    if(fPars.fUseRP)
        fMergerData->fRP = fTPCData->fRPs.front();
    // 2-> Set sign of X direction of BL to be always positive
    auto& refLine {fBeamPtr->GetRefToLine()};
    const auto& oldDir {refLine.GetDirection()};
    refLine.SetDirection({std::abs(oldDir.X()), oldDir.Y(), oldDir.Z()});
    // 3-> Rank by larger angle
    // .first = angle; .second = index; larger angles at begin
    auto lambda {[&](const std::pair<double, int>& a, const std::pair<double, int>& b)
                 {
                     if(!fInvertAngle)
                         return a.first > b.first;
                     else
                         return a.first < b.first; // Small angle is the light particle
                 }};
    std::set<std::pair<double, int>, decltype(lambda)> set(lambda);
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
    {
        auto it {fTPCData->fClusters.begin() + i};
        auto ptr {&(*it)};
        if(ptr == fBeamPtr)
            continue;
        // Orient track following RP and gravity point
        // only if RP exists; if not, set X direction to be positive
        if(fPars.fUseRP)
            it->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
        else
            it->GetRefToLine().AlignUsingPoint({0, (float)fTPCPars->GetNPADSY() / 2, (float)fTPCPars->GetNPADSZ() / 2});
        // Get angle
        auto theta {GetTheta3D(fBeamPtr->GetLine().GetDirection(), it->GetLine().GetDirection())};
        set.insert({TMath::Abs(theta), i});
    }
    if(fIsVerbose)
    {
        std::cout << BOLDCYAN << "---- Merger LightOrHeavy ----" << '\n';
        for(const auto& pair : set)
            std::cout << "Theta : " << pair.first << " at : " << pair.second << '\n';
        std::cout << "------------------------------" << RESET << '\n';
    }
    // Set pointers
    fMergerData->fLightIdx = set.begin()->second;
    fLightPtr = &(*(fTPCData->fClusters.begin() + set.begin()->second));
    if(set.size() > 1)
    {
        fMergerData->fHeavyIdx = std::next(set.begin())->second;
        fHeavyPtr = &(*(fTPCData->fClusters.begin() + std::next(set.begin())->second));
    }
}

double ActRoot::MergerDetector::TrackLengthFromLightIt(bool scale)
{
    // Sort voxels
    std::sort(fLightPtr->GetRefToVoxels().begin(), fLightPtr->GetRefToVoxels().end());
    // Distance
    auto begin {fLightPtr->GetRefToVoxels().front().GetPosition()};
    auto end {fLightPtr->GetRefToVoxels().back().GetPosition()};
    if(scale)
    {
        ScalePoint(begin, fTPCPars->GetPadSide(), fDriftFactor);
        ScalePoint(end, fTPCPars->GetPadSide(), fDriftFactor);
    }
    return (begin - end).R();
}

bool ActRoot::MergerDetector::ComputeSiliconPoint()
{
    if(fPars.fIsL1)
        return true;
    // Compute SP
    bool isOk {};
    std::tie(fMergerData->fSP, isOk) =
        fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
            .GetSiliconPointOfTrack(fLightPtr->GetLine().GetPoint(), fLightPtr->GetLine().GetDirection());
    // And compute track length
    if(fPars.fUseRP)
        fMergerData->fTrackLength = (fMergerData->fSP - fMergerData->fRP).R();
    else
        fMergerData->fTrackLength = TrackLengthFromLightIt(false);
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
    for(auto& it : {fBeamPtr, fLightPtr, fHeavyPtr})
    {
        if(it == nullptr)
            continue;
        auto p {it->GetLine().GetPoint()};
        MoveZ(p);
        it->GetRefToLine().SetPoint(p);
    }
}

bool ActRoot::MergerDetector::MatchSPtoRealPlacement()
{
    if(fPars.fIsL1)
        return true;
    // Use only first value in std::vector<int> of Ns
    auto n {fMergerData->fSilNs.front()};
    auto layer {fMergerData->fSilLayers.front()};
    // And check!
    auto ret {fSilSpecs->GetLayer(layer).MatchesRealPlacement(n, fMergerData->fSP, fMatchUseZ)};
    if(!ret && fIsVerbose)
    {
        std::cout << BOLDCYAN << "---- Merger MatchSP ----" << '\n';
        std::cout << "  Layer : " << layer << '\n';
        std::cout << "  Pad   : " << n << '\n';
        std::cout << "  SP    : " << fMergerData->fSP << '\n';
        std::cout << "  does not match real placement at" << '\n';
        auto xy {fSilSpecs->GetLayer(layer).GetPlacements().at(n).first};
        auto w {fSilSpecs->GetLayer(layer).GetUnit().GetWidth()};
        std::cout << "  XY    : [" << xy - w / 2 << ", " << xy + w / 2 << "]" << '\n';
        std::cout << "------------------------------" << RESET << '\n';
    }
    return ret;
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
    if(fPars.fUseRP)
        ScalePoint(fMergerData->fRP, xy, fDriftFactor);
    if(!fPars.fIsCal)
    {
        ScalePoint(fMergerData->fWP, xy, fDriftFactor);
        ScalePoint(fMergerData->fBSP, xy, fDriftFactor);
    }
    if(!fPars.fIsL1)
    {
        ScalePoint(fMergerData->fBP, xy, fDriftFactor);
        ScalePoint(fMergerData->fSP, xy, fDriftFactor);
    }

    // Scale Line in Clusters
    for(auto& ptr : {fBeamPtr, fLightPtr, fHeavyPtr})
    {
        if(ptr != nullptr)
            ptr->GetRefToLine().Scale(xy, fDriftFactor);
    }

    // And recompute track length
    if(fPars.fUseRP && !fPars.fIsL1)
        fMergerData->fTrackLength = (fMergerData->fSP - fMergerData->fRP).R();
    else
        fMergerData->fTrackLength = TrackLengthFromLightIt(true);
}

void ActRoot::MergerDetector::ComputeAngles()
{
    XYZVector beamDir {};
    if(fBeamPtr != nullptr)
        beamDir = fBeamPtr->GetLine().GetDirection().Unit();
    else
        beamDir = {1, 0, 0};
    // Theta Light
    fMergerData->fThetaLight = GetTheta3D(beamDir, fLightPtr->GetLine().GetDirection());
    fMergerData->fThetaLegacy = fMergerData->fThetaLight;
    // Debug: angle computed assuming beam exactly along X axis
    fMergerData->fThetaDebug = GetTheta3D({1, 0, 0}, fLightPtr->GetLine().GetDirection());
    // Phi Light
    fMergerData->fPhiLight = GetPhi3D(beamDir, fLightPtr->GetLine().GetDirection());
    // Beam angles
    fMergerData->fThetaBeam = GetTheta3D({1, 0, 0}, beamDir);
    fMergerData->fThetaBeamZ = TMath::ATan(beamDir.Z() / beamDir.X()) * TMath::RadToDeg();
    fMergerData->fPhiBeamY = TMath::ATan(beamDir.Y() / beamDir.X()) * TMath::RadToDeg();
    // Theta Heavy
    if(fHeavyPtr != nullptr)
        fMergerData->fThetaHeavy = GetTheta3D(fBeamPtr->GetLine().GetDirection(), fHeavyPtr->GetLine().GetDirection());
}

void ActRoot::MergerDetector::ComputeOtherPoints()
{
    // Boundary point: light track at ACTAR's flanges
    // but only if SP could be computed
    if(!fPars.fIsL1)
        fMergerData->fBP =
            fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
                .GetBoundaryPointOfTrack(fTPCPars->GetNPADSX(), fTPCPars->GetNPADSY(), fLightPtr->GetLine().GetPoint(),
                                         fLightPtr->GetLine().GetDirection().Unit());
    // Window point: beam entrance point at X = 0 from fit parameters
    if(fBeamPtr != nullptr)
        fMergerData->fWP = fBeamPtr->GetLine().MoveToX(0);
    if((fPars.fIsCal || fTPCData->fClusters.size() == 1) && fLightPtr != nullptr)
        fMergerData->fWP = fLightPtr->GetLine().MoveToX(0);
}

void ActRoot::MergerDetector::ComputeQave()
{
    if(!fLightPtr)
        return;
    // Copy cluster to avoid any modifications to other parts of the algorithm
    auto cluster {*fLightPtr};
    // Convert and sort along line
    if(fEnableConversion)
        cluster.ScaleVoxels(fTPCPars->GetPadSide(), fDriftFactor);
    cluster.SortAlongDir();
    // Count distance bc there could be gaps
    double gapThresh {6.5};
    double newdist {};
    for(int v = 1; v < cluster.GetSizeOfVoxels(); v++)
    {
        auto p0 {cluster.GetLine().ProjectionPointOnLine(cluster.GetVoxels()[v - 1].GetPosition())};
        auto p1 {cluster.GetLine().ProjectionPointOnLine(cluster.GetVoxels()[v].GetPosition())};
        auto d {(p1 - p0).R()};
        if(d < gapThresh)
            newdist += d;
    }
    auto dist {newdist};
    // // Legacy version
    // fLightPtr->SortAlongDir();
    // // std::sort(fLightPtr->GetRefToVoxels().begin(), fLightPtr->GetRefToVoxels().end());
    // // Get min
    // auto front {fLightPtr->GetVoxels().front().GetPosition()};
    // auto back {fLightPtr->GetVoxels().back().GetPosition()};
    // // Scale them
    // if(fEnableConversion)
    // {
    //     ScalePoint(front, fTPCPars->GetPadSide(), fDriftFactor, true);
    //     ScalePoint(back, fTPCPars->GetPadSide(), fDriftFactor, true);
    // }
    // // Get projections
    // auto min {fLightPtr->GetLine().ProjectionPointOnLine(front)};
    // auto max {fLightPtr->GetLine().ProjectionPointOnLine(back)};
    // // Convert them to physical points
    // // ScalePoint(min, fTPCPars->GetPadSide(), fDriftFactor);
    // // ScalePoint(max, fTPCPars->GetPadSide(), fDriftFactor);
    // // Dist in mm
    // auto dist {(max - min).R()};
    // if(fIsVerbose)
    // {
    //     std::cout << "-> ComputeQave" << '\n';
    //     std::cout << "   NewDist : " << newdist << '\n';
    //     std::cout << "   NewMin  : " << newmin << '\n';
    //     std::cout << "   NewMax  : " << newmax << '\n';
    //     cluster.GetLine().Print();
    //     std::cout << "   OldDist : " << dist << '\n';
    //     std::cout << "   OldMin  : " << min << '\n';
    //     std::cout << "   OldMax  : " << max << '\n';
    //     fLightPtr->GetLine().Print();
    // }
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
    auto qTotal {std::accumulate(fLightPtr->GetVoxels().begin(), fLightPtr->GetVoxels().end(), 0.f,
                                 [](float sum, const Voxel& v) { return sum + v.GetCharge(); })};
    // std::cout << "Qtotal : " << qTotal << '\n';
    // std::cout << "Qtotal / dist : " << qTotal / dist << '\n';
    fMergerData->fQave = qTotal / dist;
}

void ActRoot::MergerDetector::ComputeQProfile()
{
    if(!fLightPtr)
        return;
    // 0-> Init histogram
    TH1F h {"hQProfile", "QProfile", 100, -5, 150};
    h.SetTitle("QProfile;dist [mm];Q [au]");
    // 1-> Ref point is either WP or beginning of projection on line
    XYZPoint ref {};
    XYZPoint ref3D {};
    if(fPars.fUseRP && fMergerData->fRP.X() != -1)
        ref = fMergerData->fRP;
    else if(!fPars.fUseRP && fMergerData->fWP.X() != -1)
        ref = fMergerData->fWP;
    else // default case: go to beginning of light track
    {
        std::sort(fLightPtr->GetRefToVoxels().begin(), fLightPtr->GetRefToVoxels().end());
        auto front {fLightPtr->GetVoxels().front().GetPosition()};
        // Convert it to physical units
        if(fEnableConversion)
            ScalePoint(front, fTPCPars->GetPadSide(), fDriftFactor, true);
        auto ref {fLightPtr->GetLine().ProjectionPointOnLine(front)};
    }
    // Safe check: align again using reference point
    fLightPtr->GetRefToLine().AlignUsingPoint(ref, true);
    // Declare line to use, bc it depends on 3D or 2D mode
    ActRoot::Line line {fLightPtr->GetLine()};
    // Save in 3D before setting 2D (if so)
    ref3D = ref;
    if(f2DProfile)
    {
        // Set ref.Z component to be 0
        ref.SetZ(0);
        const auto& gp {fLightPtr->GetLine().GetPoint()};
        line = {{gp.X(), gp.Y(), 0}, ref};
    }
    // Use 3 divisions to get better resolution
    float div {1.f / 3};
    for(const auto& v : fLightPtr->GetVoxels())
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
                    XYZPoint bin {(pos.X() + 0.5f) + ix * div, (pos.Y() + 0.5f) + iy * div,
                                  (f2DProfile) ? 0.f : (pos.Z() + 0.5f) + iz * div};
                    // Convert to physical units
                    if(fEnableConversion)
                        ScalePoint(bin, fTPCPars->GetPadSide(), fDriftFactor, false); // +0.5 already considered
                    // Project it on line
                    auto proj {line.ProjectionPointOnLine(bin)};
                    // Fill histograms
                    auto dist {(proj - ref).R()};
                    h.Fill(dist, q / ((f2DProfile) ? 9 : 27));
                    if(f2DProfile)
                        break;
                }
            }
        }
    }
    fMergerData->fQProf = h;
    // Compute range from profile
    auto distMax {GetRangeFromProfile(&h)};
    // And move to point
    fMergerData->fBraggP = ref + distMax * line.GetDirection().Unit();
    if(f2DProfile)
    {
        // Get the Z value manually
        // auto p3d {ref3D + distMax * fLightPtr->GetLine().GetDirection().Unit()};
        // Simpliest way of doing this!
        auto p3d {fLightPtr->GetLine().MoveToX(fMergerData->fBraggP.X())};
        fMergerData->fBraggP.SetZ(p3d.Z());
        // This method works! It can be checked that
        // manually computing the mean if Z values in the (X,Y) region
        // just calculated returns the ~ same Z value!!!

        // std::vector<double> zetas;
        // auto x {fMergerData->fBraggP.X()};
        // auto y {fMergerData->fBraggP.Y()};
        // for(auto& v : fLightPtr->GetVoxels())
        // {
        //     const auto& pos {v.GetPosition()};
        //     double shift {2};
        //     if(std::abs(pos.X() - x) <= shift && std::abs(pos.Y() - y) <= shift)
        //         zetas.push_back(pos.Z() + 0.5);
        // }
        // // Get Z mean
        // fMergerData->fBraggP.SetZ(TMath::Mean(zetas.begin(), zetas.end()));
    }
}

void ActRoot::MergerDetector::ComputeXProfile()
{
    // Set points to project according to mode
    bool isOkReaction {fBeamPtr != nullptr && !fPars.fIsCal};
    bool isOkOther {(fPars.fIsCal || fPars.fIsL1) && fLightPtr != nullptr};
    if(isOkReaction || isOkOther)
    {
        TH1F hQprojX {"hQProjX", "All Q along X;X [pad];Q_{proj X}", 128, 0, 128};
        std::vector<ActRoot::Cluster*> ptrs {fBeamPtr, fLightPtr, fHeavyPtr};
        // Workaround: we analyze all the tracks in the event to find the BSP or compute the X profile
        // if(isOkReaction)
        //     ptrs = {fBeamPtr, fLightPtr, fHeavyPtr};
        // if(isOkOther)
        //     ptrs = {fLightPtr};
        // Run for the set points!
        for(auto& ptr : ptrs)
        {
            if(!ptr)
                continue;
            for(const auto& v : ptr->GetVoxels())
                hQprojX.Fill(v.GetPosition().X(), v.GetCharge());
        }
        // Compute x max from profile
        auto xMax {GetRangeFromProfile(&hQprojX, false)}; // dont smooth
        fMergerData->fBSP = {(float)xMax, 0, 0};
        // Save in MergerData
        fMergerData->fQprojX = hQprojX;
    }
}

double ActRoot::MergerDetector::GetRangeFromProfile(TH1F* h, bool smooth)
{
    // 1-> Smooth the histogram
    if(smooth)
        h->Smooth();
    // 2-> Find maximum
    auto maxBin {h->GetMaximumBin()};
    auto xMax {h->GetBinCenter(maxBin)};
    auto yMax {h->GetBinContent(maxBin)};
    // 3-> Set reference point
    auto range {yMax / 5};
    // 4-> Create interpolation functions
    // Create TSpline
    auto spe {std::make_unique<TSpline3>(h)};
    // And now function
    auto func {std::make_unique<TF1>("func", [&](double* x, double* p) { return spe->Eval(x[0]); }, 0, 128, 1)};
    // Find maximum in the range [xMax, xRangeMax of histogram]
    double ret {};
    if(fEnableRootFind)
        ret = func->GetX(range, xMax, h->GetXaxis()->GetXmax());
    return ret;
}

void ActRoot::MergerDetector::ClearEventFilter()
{
    // Not needed because we are reading directly from ttree
    fMergerData->ClearFilter();
}

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
        std::cout << "-> ForceGATCONF  ? " << std::boolalpha << fForceGATCONF << '\n';
        std::cout << "-> GATCONF map   : " << '\n';
        for(const auto& [key, vals] : fGatMap)
        {
            std::cout << "   " << key << " = ";
            for(const auto& s : vals)
                std::cout << s << ", ";
            std::cout << '\n';
        }
        std::cout << "-> ForceRP       ? " << std::boolalpha << fForceRP << '\n';
        std::cout << "-> ForceBeamLike ? " << std::boolalpha << fForceBeamLike << '\n';
        std::cout << "-> InvertAngle   ? " << std::boolalpha << fInvertAngle << '\n';
        std::cout << "-> NotBeamMults  : ";
        for(const auto& m : fNotBMults)
            std::cout << m << ", ";
        std::cout << '\n';
        std::cout << "-> EnableConver  ? " << std::boolalpha << fEnableConversion << '\n';
        if(fEnableConversion)
        {
            std::cout << "-> DriftFactor   : " << fDriftFactor << '\n';
            std::cout << "-> EnableMatch   ? " << std::boolalpha << fEnableMatch << '\n';
            std::cout << "-> MatchUseZ     ? " << std::boolalpha << fMatchUseZ << '\n';
            std::cout << "-> MatchZOffset  : " << fZOffset << '\n';
        }
        std::cout << "-> EnableQProf   ? " << std::boolalpha << fEnableQProfile << '\n';
        if(fEnableQProfile)
        {
            std::cout << "-> 2DProfile     ? " << std::boolalpha << f2DProfile << '\n';
            std::cout << "-> EnableRootFind? " << std::boolalpha << fEnableRootFind << '\n';
        }
        std::cout << "-> DefaultBeam   ? " << std::boolalpha << fEnableDefaultBeam << '\n';
        if(fEnableDefaultBeam)
            std::cout << "-> DefaultMinX   : " << std::boolalpha << fDefaultBeamXThresh << '\n';
    }
    // fSilSpecs->Print();
    std::cout << "::::::::::::::::::::::::" << RESET << '\n';

    if(fFilter)
        fFilter->Print();
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
