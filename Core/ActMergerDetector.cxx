#include "ActMergerDetector.h"

#include "ActModularData.h"
#include "ActModularDetector.h"
#include "ActSilData.h"
#include "ActSilDetector.h"
#include "ActSilSpecs.h"
#include "ActTPCDetector.h"
#include "ActTPCPhysics.h"

#include "TTree.h"

#include <iostream>
#include <memory>
#include <string>

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

void ActRoot::MergerDetector::InitOutputMerger(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::ReadSilSpecs(const std::string& file)
{
    fSilSpecs = std::make_shared<ActPhysics::SilSpecs>();
    fSilSpecs->ReadFile(file);
    fSilSpecs->Print();
}

void ActRoot::MergerDetector::MergeEvent() {}

void ActRoot::MergerDetector::ComputeSiliconPoint() {}
