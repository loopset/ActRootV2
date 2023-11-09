#include "ActMergerDetector.h"

#include "ActColors.h"
#include "ActInputParser.h"
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

#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>

void ActRoot::MergerDetector::ReadConfiguration(std::shared_ptr<InputBlock> block)
{
    // Read (mandatory) SilSpecs config file
    if(block->CheckTokenExists("SilSpecsFile"))
        ReadSilSpecs(block->GetString("SilSpecsFile"));
    // Map GATCONFS to SilLayers, using gat command
    auto gatMap {block->GetMappedValuesAs<std::string>("gat")};
    if(gatMap.size() > 0)
        fGatMap = gatMap;
    // GATCONF force?
    if(block->CheckTokenExists("ForceGATCONF"))
        fForceGat = block->GetBool("ForceGATCONF");
    // Beam-like and multiplicities
    if(block->CheckTokenExists("ForceBeamLike"))
        fForceBeamLike = block->GetBool("ForceBeamLike");
    if(block->CheckTokenExists("NotBeamMults"))
        fNotBMults = block->GetIntVector("NotBeamMults");
    if(block->CheckTokenExists("ForceSilMult"))
        fForceSilMult = block->GetBool("ForceSilMult");
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
    // 2-> Identify light and heavy
    LightOrHeavy();
    // 3-> Compute SP
    ComputeSiliconPoint();
}

bool ActRoot::MergerDetector::IsDoable()
{
    // 1-> Apply GATCONF cut
    bool isInGat {true};
    auto gat {(int)fModularData->Get("GATCONF")};
    if(fForceGat)
    {
        if(fGatMap.count(gat))
            isInGat = true;
        else
            isInGat = false;
    }
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
    bool condA {isInGat && hasBL && hasMult};
    if(!condA)
        return false;
    else
    {
        fHitSilLayer = fGatMap[gat];
        fSilData->ApplyFinerThresholds(fSilSpecs);
        // 3->Check silicon multiplicity
        if(fForceSilMult)
            return fSilData->GetMult(fHitSilLayer) == 1; // force sil mult = 1
        else
            return true;
    }
}

void ActRoot::MergerDetector::Reset()
{
    // Reset iterators before moving on to work with them
    fBeamIt = fTPCPhyiscs->fClusters.end();
    fLightIt = fTPCPhyiscs->fClusters.end();
    fHeavyIt = fTPCPhyiscs->fClusters.end();
    // Reset other variables
    fHitSilLayer = {};
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
    // for(const auto& pair : set)
    //     std::cout << "Theta : " << pair.first << " at : " << pair.second << '\n';
    // Set iterators
    fLightIt = fTPCPhyiscs->fClusters.begin() + set.begin()->second;
    if(set.size() > 1)
        fHeavyIt = fTPCPhyiscs->fClusters.begin() + std::next(set.begin())->second;
    // Set RP to MergerData
    fMergerData->fRP = fTPCPhyiscs->fRPs.begin()->second;
}

void ActRoot::MergerDetector::ComputeSiliconPoint()
{
    fMergerData->fSP = fSilSpecs->GetLayer(fHitSilLayer)
                           .GetSiliconPointOfTrack(fMergerData->fRP, fLightIt->GetLine().GetDirection().Unit());
    // Redefine signs just in case
    fLightIt->GetRefToLine().AlignUsingPoint(fMergerData->fRP);
    fMergerData->Print();
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
        std::cout << "   " << key << " = " << vals << '\n';
    std::cout << "-> ForceGATCONF  ? " << std::boolalpha << fForceGat << '\n';
    std::cout << "-> ForceBeamLike ? " << std::boolalpha << fForceBeamLike << '\n';
    std::cout << "-> ForceSilMult  ? " << std::boolalpha << fForceSilMult << '\n';
    std::cout << "-> NotBeamMults  : ";
    for(const auto& m : fNotBMults)
        std::cout << m << ", ";
    std::cout << '\n';
    // fSilSpecs->Print();
    std::cout << "::::::::::::::::::::::::" << RESET << '\n';
}
