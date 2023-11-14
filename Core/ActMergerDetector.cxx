#include "ActMergerDetector.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActModularDetector.h"
#include "ActSilData.h"
#include "ActSilDetector.h"
#include "ActSilSpecs.h"
#include "ActTPCDetector.h"
#include "ActTPCPhysics.h"

#include "TMath.h"
#include "TMathBase.h"
#include "TTree.h"

#include <algorithm>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

void ActRoot::MergerDetector::ReadConfiguration(std::shared_ptr<InputBlock> block)
{
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
}

void ActRoot::MergerDetector::InitInputMerger(std::shared_ptr<TTree> tree)
{
    // TPC physics
    if(fTPCPhyiscs)
        delete fTPCPhyiscs;
    fTPCPhyiscs = new TPCPhysics;
    tree->SetBranchAddress("TPCPhysics", &fTPCPhyiscs);

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

    // Disable TPCData: we only need clusters
    tree->SetBranchStatus("TPCData", false);
}

void ActRoot::MergerDetector::InitOutputMerger(std::shared_ptr<TTree> tree)
{
    if(fMergerData)
        delete fMergerData;
    fMergerData = new MergerData;
    tree->Branch("MergerData", &fMergerData);
}

void ActRoot::MergerDetector::ReadSilSpecs(const std::string& file)
{
    fSilSpecs = std::make_shared<ActPhysics::SilSpecs>();
    fSilSpecs->ReadFile(file);
    // fSilSpecs->Print();
}

void ActRoot::MergerDetector::MergeEvent()
{
    // 1-> Reset iterators
    Reset();
    if(!IsDoable())
        return;
    //
    // 2-> Identify light and heavy
    LightOrHeavy();
    // 3-> Compute SP
    ComputeSiliconPoint();
    // 4-> Scale points to RebinZ
    ScaleToRebinZ();
}

void ActRoot::MergerDetector::ScaleToRebinZ()
{
    // RP
    fMergerData->fRP.SetZ(fMergerData->fRP.Z() * fTPCPars->GetREBINZ());
    // SP
    fMergerData->fSP.SetZ(fMergerData->fSP.Z() * fTPCPars->GetREBINZ());
}

bool ActRoot::MergerDetector::IsDoable()
{
    auto condA {GateGATCONFandTrackMult()};
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
        bool hasIt {false};
        int notBL {};
        for(auto it = fTPCPhyiscs->fClusters.begin(); it != fTPCPhyiscs->fClusters.end(); it++)
        {
            if(it->GetIsBeamLike() && !hasIt) // admit only one beam-like
            {
                hasIt = true;
                fBeamIt = it;
            }
            else
                notBL++;
        }
        hasBL = hasIt;
        hasMult = IsInVector(notBL, fNotBMults);
        // if(notBL > 2)
        // std::cout << "hasMult? " << std::boolalpha << hasMult << '\n';
    }
    else
    {
        hasMult = IsInVector((int)fTPCPhyiscs->fClusters.size(), fNotBMults);
    }
    return isInGat && hasBL && hasMult;
}

bool ActRoot::MergerDetector::GateSilMult()
{
    // 1-> Apply finer thresholds in SilSpecs
    fSilData->ApplyFinerThresholds(fSilSpecs);
    // 2-> Check and write silicon data
    int withE {};
    int withMult {};
    for(const auto& layer : fGatMap[(int)fModularData->Get("GATCONF")])
    {
        // Only check mult of layers with E >  threshold!
        if(int mult {fSilData->GetMult(layer)}; mult > 0)
        {
            withE++;
            if(mult == 1)
            {
                withMult++;
                // Write data
                fMergerData->fSilLayers.push_back(layer);
                fMergerData->fSilEs.push_back(fSilData->fSiE[layer].front());
                fMergerData->fSilNs.push_back(fSilData->fSiN[layer].front());
            }
        }
    }
    return withE == withMult;
}

void ActRoot::MergerDetector::Reset()
{
    // Reset iterators before moving on to work with them
    fBeamIt = fTPCPhyiscs->fClusters.end();
    fLightIt = fTPCPhyiscs->fClusters.end();
    fHeavyIt = fTPCPhyiscs->fClusters.end();
    // Reset other variables
    fMergerData->fRun = fCurrentRun;
    fMergerData->fEntry = fCurrentEntry;
}

double ActRoot::MergerDetector::GetTheta(const XYZVector& beam, const XYZVector& other)
{
    auto dot {beam.Unit().Dot(other.Unit())};
    return TMath::ACos(dot) * TMath::RadToDeg();
}

void ActRoot::MergerDetector::LightOrHeavy()
{
    // .first = angle; .second = index; larger angles at begin
    auto lambda {[](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first > b.first; }};
    std::set<std::pair<double, int>, decltype(lambda)> set(lambda);
    for(int i = 0, size = fTPCPhyiscs->fClusters.size(); i < size; i++)
    {
        auto it {fTPCPhyiscs->fClusters.begin() + i};
        if(it == fBeamIt)
            continue;
        // Get angle
        auto theta {GetTheta(fBeamIt->GetLine().GetDirection(), it->GetLine().GetDirection())};
        set.insert({TMath::Abs(theta), i});
    }
    for(const auto& pair : set)
        std::cout << "Theta : " << pair.first << " at : " << pair.second << '\n';
    // Set iterators
    fLightIt = fTPCPhyiscs->fClusters.begin() + set.begin()->second;
    if(set.size() > 1)
        fHeavyIt = fTPCPhyiscs->fClusters.begin() + std::next(set.begin())->second;
    // Set RP to MergerData
    fMergerData->fRP = fTPCPhyiscs->fRPs.begin()->second;
}

void ActRoot::MergerDetector::ComputeSiliconPoint()
{
    fMergerData->fSP = fSilSpecs->GetLayer(fMergerData->fSilLayers.front())
                           .GetSiliconPointOfTrack(fMergerData->fRP, fLightIt->GetLine().GetDirection().Unit());
    // Redefine signs just in case
    fLightIt->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
    // fMergerData->Print();
}

void ActRoot::MergerDetector::ClearOutputMerger()
{
    fMergerData->Clear();
}

void ActRoot::MergerDetector::Print() const
{
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
    // fSilSpecs->Print();
    std::cout << "::::::::::::::::::::::::" << RESET << '\n';
}
