#include "ActMergerDetector.h"

#include "ActTPCDetector.h"
#include "ActSilDetector.h"
#include "ActModularDetector.h"

#include "ActModularData.h"
#include "ActSilData.h"
#include "ActTPCPhysics.h"

#include "TTree.h"

#include <iostream>
#include <memory>

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
}

void ActRoot::MergerDetector::InitOutputMerger(std::shared_ptr<TTree> tree) {}

void ActRoot::MergerDetector::MergeEvent() 
{

}
