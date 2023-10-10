#include "ActTPCDetector.h"

#include "ActCalibrationManager.h"
#include "ActCluster.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActRANSAC.h"
#include "ActTPCPhysics.h"
#include "Math/Point3D.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "TTree.h"

#include <algorithm>
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
    std::vector<std::string> keys {"Type", "RebinZ", "CleanSaturatedVoxels",
        "CleanPadMatrix", "CleanPadMatrixPars"};
    //Init detector params
    fPars = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ", true))
        fPars.SetREBINZ(config->GetInt("RebinZ"));
    //Read some analysis configurations
    if(config->CheckTokenExists("CleanSaturatedVoxels", true))
        fCleanSaturatedVoxels = config->GetBool("CleanSaturatedVoxels");
    if(config->CheckTokenExists("CleanPadMatrix", true))
        fCleanPadMatrix = config->GetBool("CleanPadMatrix");
    if(config->CheckTokenExists("CleanPadMatrixPars", true))
    {
        auto pars {config->GetDoubleVector("CleanPadMatrixPars")};
        fMinTBtoDelete = pars.at(0);
        fMinQtoDelete  = pars.at(1);
    }
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    //Add LookUp table
    fCalMan->ReadLookUpTable(config->GetString("LookUp"));
    //Pad align table
    fCalMan->ReadPadAlign(config->GetString("PadAlign"));
}

void ActRoot::TPCDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    //delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    tree->SetBranchAddress("data", &fMEvent);
    //Init pad matrix!
    fPadMatrix = {};
}

void ActRoot::TPCDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->Branch("TPCData", &fData);
}

void ActRoot::TPCDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->SetBranchAddress("TPCData", &fData);
}

void ActRoot::TPCDetector::InitOutputPhysics(std::shared_ptr<TTree> tree)
{
    if(fPhysics)
        delete fPhysics;
    fPhysics = new TPCPhysics;
    //do not attatch output tree so far
}

void ActRoot::TPCDetector::ClearEventData()
{
    fData->Clear();
    //if opted, clean pad matrix
    if(fCleanPadMatrix)
        fPadMatrix.clear();
}

void ActRoot::TPCDetector::ClearEventPhysics()
{
    fPhysics->Clear();
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
    //Clean pad matrix
    if(fCleanPadMatrix)
        CleanPadMatrix();
}

void ActRoot::TPCDetector::ReadHits(ReducedData& coas, const int& where, int& hitID)
{
    int padx {}; int pady {};
    try
    {
        padx = fCalMan->ApplyLookUp(where, 4);
        pady = fCalMan->ApplyLookUp(where, 5);
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
        float qcal {static_cast<float>(fCalMan->ApplyPadAlignment(where, qraw))};
        
        //Apply rebinning (if desired)
        int binZ {(int)padz / fPars.GetREBINZ()};
        padz = fPars.GetREBINZ() * binZ + ((fPars.GetREBINZ() <= 1) ? 0.0 : (double)fPars.GetREBINZ() / 2);
        
        //Build Voxel
        //Apply cut on saturated flag if desired
        if(fCleanSaturatedVoxels && coas.hasSaturation)
        {
            ;//if CleanSatVoxels is enables and voxel does have hasSat = true, do not fill in vector
        }
        else
        {
            Voxel hit {ROOT::Math::XYZPointF(padx, pady, padz), qcal, coas.hasSaturation};
            fData->fVoxels.push_back(hit);
            //push to pad matrix if enabled
            if(fCleanPadMatrix)
            {
                fPadMatrix[{padx, pady}].first.push_back(fData->fVoxels.size() - 1);
                fPadMatrix[{padx, pady}].second += qcal;
            }
        }
        //Increase hit id
        hitID++;        
    }
}

void ActRoot::TPCDetector::CleanPadMatrix()
{
    for(const auto& [_, pair] : fPadMatrix)
    {
        const auto& vals {pair.first};
        const auto& totalQ {pair.second};
        if(vals.size() >= fMinTBtoDelete && totalQ >= fMinQtoDelete)//threshold in time buckets and in Qtotal to delete pad data
        {
            for(auto it = vals.rbegin(); it != vals.rend(); it++)
                fData->fVoxels.erase(fData->fVoxels.begin() + *it);
        }
    }
}

void ActRoot::TPCDetector::BuildEventPhysics()
{
    //Use RANSAC
    ActCluster::RANSAC ransac {500, 20, 10.};
    //ransac.ReadConfigurationFile();
    fPhysics->fClusters = ransac.Run(fData->fVoxels);
    fPhysics->Print();
}
