#include "TPCDetector.h"

#include "CalibrationManager.h"
#include "InputParser.h"
#include "Math/Point3D.h"
#include "TPCData.h"
#include "TPCLegacyData.h"
#include "TTree.h"

#include <cstddef>
#include <exception>
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
    if(config->CheckTokenExists("RebinZ", true))
        fPars.SetREBINZ(config->GetInt("RebinZ"));
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    //Add LookUp table
    CalibrationManager::Get()->ReadLookUpTable(config->GetString("LookUp"));
    //Pad align table
    CalibrationManager::Get()->ReadPadAlign(config->GetString("PadAlign"));    
}

void ActRoot::TPCDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    //delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    fCurrentRun = run;
    tree->SetBranchAddress("data", &fMEvent);
}

void ActRoot::TPCDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->Branch("TPCData", &fData);
}

void ActRoot::TPCDetector::ClearEventData()
{
    fData->Clear();
}

void ActRoot::TPCDetector::BuildEventData()
{
    int hitID {};
    int hitIDin {};
    for(auto& coas : fMEvent->CoboAsad)
    {
        //locate channel!
        int co {coas.globalchannelid >> 11};
        int as {(coas.globalchannelid - (co << 11)) >> 9};
        int ag {(coas.globalchannelid - (co << 11) - (as << 9)) >> 7};
        int ch {(coas.globalchannelid - (co << 11) - (as << 9)
                 - (ag << 7))};
        int where {co * fPars.GetNBASAD() * fPars.GetNBAGET() * fPars.GetNBCHANNEL()
        + as * fPars.GetNBAGET() * fPars.GetNBCHANNEL()
        + ag * fPars.GetNBCHANNEL()
        + ch};

        //Read hits
        if((co != 31) && (co != 16))
        {
            ReadHits(coas, where, hitIDin);
            hitID++;
        }
    }
}

void ActRoot::TPCDetector::ReadHits(ReducedData& coas, const int& where, int& hitID)
{
    int padx {}; int pady {};
    try
    {
        padx = CalibrationManager::Get()->ApplyLookUp(where, 4);
        pady = CalibrationManager::Get()->ApplyLookUp(where, 5);
    }
    catch(std::exception& e)
    {
        throw std::runtime_error("Error while reading hits in TPCDetector -> LT table out of range, check ACQ parameters");
    }
    if(pady == -1)//unused channel
        return;
    for(size_t i = 0, maxI = coas.peakheight.size(); i < maxI; i++)
    {
        float padz {coas.peaktime[i]};
        if(padz < 0)
            continue;
        float qraw {coas.peakheight[i]};
        float qcal {static_cast<float>(CalibrationManager::Get()->ApplyPadAlignment(where, qraw))};
        
        //Apply rebinning (if desired)
        int binZ {(int)padz / fPars.GetREBINZ()};
        padz = fPars.GetREBINZ() * binZ + ((fPars.GetREBINZ() <= 1) ? 0.0 : (double)fPars.GetREBINZ() / 2);
        
        //Build Voxel
        Voxel hit {hitID, ROOT::Math::XYZPointF(padx, pady, padz), qcal, coas.hasSaturation};
        fData->fVoxels.push_back(hit);
        
        //Increase hit id
        hitID++;        
    }
}
