#include "TPCDetector.h"

#include "CalibrationManager.h"
#include "InputParser.h"
#include "TPCData.h"
#include "TPCLegacyData.h"
#include "TTree.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::TPCParameters::TPCParameters(const std::string& type)
{
    std::cout<<"Initializing detector type: "<<type<<'\n';
    if(type == "Actar")
    {
        fNPADSX = 128;
        fNPADSY = 128;
        fNPADSZ = 512;
    }
    else if(type == "protoActar")
    {
        fNPADSX = 64;
        fNPADSY = 32;
        fNPADSZ = 512;
    }
    else
        throw std::runtime_error("No TPCParameters config available for passed " + type);
}

void ActRoot::TPCDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"Type", "RebinZ"};
    //Init detector params
    fPars = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ"))
        fPars.SetREBINZ(config->GetInt("RebinZ"));
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    //Add LookUp table
    CalibrationManager::Get()->ReadLookUpTable(config->GetString("LookUp"));
    //Pad align table
    //CalibrationManager::Get()->ReadPadAlign(config->GetString("PadAlign"));    
}

void ActRoot::TPCDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    //delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    fCurrentRun = run;
    tree->SetBranchAddress("data", &fMEvent);
    //std::cout<<"Initializing TPC input raw data at run "<<run<<'\n';
}

void ActRoot::TPCDetector::InitOutputData()
{
    if(fData)
        delete fData;
    fData = new TPCData;
}

void ActRoot::TPCDetector::BuildEventData()
{
    
}
