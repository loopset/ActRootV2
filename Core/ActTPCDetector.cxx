#include "ActTPCDetector.h"

#include "ActCalibrationManager.h"
#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActMultiStep.h"
#include "ActRANSAC.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActTPCPhysics.h"

#include "TTree.h"

#include "Math/Point3D.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

ActRoot::TPCParameters::TPCParameters(const std::string& type)
{
    std::cout << "Initializing detector type: " << type << '\n';
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
    std::vector<std::string> keys {"Type", "RebinZ", "CleanSaturatedVoxels", "CleanPadMatrix", "CleanPadMatrixPars"};
    // Init detector params
    fPars = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ", true))
        fPars.SetREBINZ(config->GetInt("RebinZ"));
    // MEvent -> TPCData analysis options
    if(config->CheckTokenExists("CleanSaturatedMEvent", true))
        fCleanSaturatedMEvent = config->GetBool("CleanSaturatedMEvent");
    if(config->CheckTokenExists("CleanSaturatedVoxels", true))
        fCleanSaturatedVoxels = config->GetBool("CleanSaturatedVoxels");
    if(config->CheckTokenExists("CleanSaturatedVoxelsPars", true))
    {
        auto pars {config->GetDoubleVector("CleanSaturatedVoxelsPars")};
        fMinTBtoDelete = pars.at(0);
        fMinQtoDelete = pars.at(1);
    }
    if(config->CheckTokenExists("CleanDuplicatedVoxels", true))
        fCleanDuplicatedVoxels = config->GetBool("CleanDuplicatedVoxels");
    // TPCData -> TPCPhysics analysis options
    if(config->CheckTokenExists("ClusterMethod"))
        InitClusterMethod(config->GetString("ClusterMethod"));
}

void ActRoot::TPCDetector::InitClusterMethod(const std::string& method)
{
    if(method == "Ransac")
    {
        fRansac = std::make_shared<ActCluster::RANSAC>();
        fRansac->ReadConfigurationFile();
        fRansac->Print();
    }
    else if(method == "Climb")
    {
        fClimb = std::make_shared<ActCluster::ClIMB>();
        fClimb->SetTPCParameters(&fPars);
        fClimb->ReadConfigurationFile();
        fClimb->Print();
        // ClIMB also requires multistep algorithm
        fMultiStep = std::make_shared<ActCluster::MultiStep>();
        fMultiStep->ReadConfigurationFile();
        fMultiStep->SetClimb(fClimb);
        fMultiStep->SetTPCParameters(&fPars);
        fMultiStep->Print();
    }
    else if(method == "None")
        return;
    else
        throw std::runtime_error("TPCDetector::InitClusterMethod: no listed method from Ransac, Climb and None");
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    // Add LookUp table
    fCalMan->ReadLookUpTable(config->GetString("LookUp"));
    // Pad align table
    fCalMan->ReadPadAlign(config->GetString("PadAlign"));
}

void ActRoot::TPCDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    // delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    tree->SetBranchAddress("data", &fMEvent);
    // Init pad matrix!
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
    // do not attatch output tree so far
}

void ActRoot::TPCDetector::ClearEventData()
{
    fData->Clear();
    // if opted, clean pad matrix
    if(fCleanSaturatedVoxels)
        fPadMatrix.clear();
}

void ActRoot::TPCDetector::ClearEventPhysics()
{
    fPhysics->Clear();
}

void ActRoot::TPCDetector::SetEventData(ActRoot::VData* vdata)
{
    fData = nullptr;
    auto casted {dynamic_cast<ActRoot::TPCData*>(vdata)};
    if(casted)
        fData = casted;
    else
        std::cout << "Error: Could not dynamic_cast to TPCData" << '\n';
}

void ActRoot::TPCDetector::BuildEventData()
{
    int hitID {};
    int hitIDin {};
    for(auto& coas : fMEvent->CoboAsad)
    {
        // locate channel!
        int co {coas.globalchannelid >> 11};
        int as {(coas.globalchannelid - (co << 11)) >> 9};
        int ag {(coas.globalchannelid - (co << 11) - (as << 9)) >> 7};
        int ch {(coas.globalchannelid - (co << 11) - (as << 9) - (ag << 7))};
        int where {co * fPars.GetNBASAD() * fPars.GetNBAGET() * fPars.GetNBCHANNEL() +
                   as * fPars.GetNBAGET() * fPars.GetNBCHANNEL() + ag * fPars.GetNBCHANNEL() + ch};

        // Read hits
        if((co != 31) && (co != 16))
        {
            ReadHits(coas, where, hitIDin);
            hitID++;
        }
    }
    // Clean duplicated voxels
    if(fCleanDuplicatedVoxels)
        EnsureUniquenessOfVoxels();
    // Clean pad matrix
    if(fCleanSaturatedVoxels)
        CleanPadMatrix();
}

void ActRoot::TPCDetector::ReadHits(ReducedData& coas, const int& where, int& hitID)
{
    int padx {};
    int pady {};
    try
    {
        padx = fCalMan->ApplyLookUp(where, 4);
        pady = fCalMan->ApplyLookUp(where, 5);
    }
    catch(std::exception& e)
    {
        throw std::runtime_error(
            "Error while reading hits in TPCDetector -> LT table out of range, check ACQ parameters");
    }
    if(pady == -1) // unused channel
        return;
    for(size_t i = 0, maxI = coas.peakheight.size(); i < maxI; i++)
    {
        float padz {coas.peaktime[i]};
        if(padz < 0)
            continue;
        float qraw {coas.peakheight[i]};
        float qcal {static_cast<float>(fCalMan->ApplyPadAlignment(where, qraw))};

        // Apply rebinning (if desired)
        int binZ {(int)padz / fPars.GetREBINZ()};
        padz = fPars.GetREBINZ() * binZ + ((fPars.GetREBINZ() <= 1) ? 0.0 : (double)fPars.GetREBINZ() / 2);

        // Build Voxel
        // Apply cut on saturated flag if desired
        if(fCleanSaturatedMEvent && coas.hasSaturation)
        {
            ; // if CleanSatVoxels is enables and voxel does have hasSat = true, do not fill in vector
        }
        else
        {
            Voxel hit {ROOT::Math::XYZPointF(padx, pady, padz), qcal, coas.hasSaturation};
            fData->fVoxels.push_back(hit);
            // push to pad matrix if enabled
            if(fCleanSaturatedVoxels)
            {
                fPadMatrix[{padx, pady}].first.push_back(fData->fVoxels.size() - 1);
                fPadMatrix[{padx, pady}].second += qcal;
            }
        }
        // Increase hit id
        hitID++;
    }
}

void ActRoot::TPCDetector::CleanPadMatrix()
{
    for(const auto& [_, pair] : fPadMatrix)
    {
        const auto& vals {pair.first};
        const auto& totalQ {pair.second};
        if(vals.size() >= fMinTBtoDelete &&
           totalQ >= fMinQtoDelete) // threshold in time buckets and in Qtotal to delete pad data
        {
            for(auto it = vals.rbegin(); it != vals.rend(); it++)
                fData->fVoxels.erase(fData->fVoxels.begin() + *it);
        }
    }
}

void ActRoot::TPCDetector::EnsureUniquenessOfVoxels()
{
    // Declare unordered_set
    // Hash function
    auto hash = [&](const Voxel& v)
    {
        const auto& pos {v.GetPosition()};
        auto ret {(int)pos.X() + fPars.GetNPADSX() * (int)pos.Y() +
                  fPars.GetNPADSX() * (int)fPars.GetNPADSY() * pos.Z()};
        return ret;
    };
    auto equal = [](const Voxel& a, const Voxel& b)
    {
        const auto& pa {a.GetPosition()};
        const auto& pb {b.GetPosition()};
        bool bx {(int)pa.X() == (int)pb.X()};
        bool by {(int)pa.Y() == (int)pb.Y()};
        bool bz {(int)pa.Z() == (int)pb.Z()};
        return bx && by && bz;
    };
    std::unordered_set<Voxel, decltype(hash), decltype(equal)> set(
        10, hash, equal); // 10 = initial bucket count? I think it is not important
    // Add from vector to it
    for(const auto& voxel : fData->fVoxels)
        set.insert(voxel);
    // Back to vector!
    fData->fVoxels.assign(set.begin(), set.end());
}

void ActRoot::TPCDetector::BuildEventPhysics()
{
    // Build based on pointer existence
    if(fRansac)
    {
        fClusterClock.Start(false);
        fPhysics->fClusters = fRansac->Run(fData->fVoxels);
        fClusterClock.Stop();
    }
    else if(fClimb)
    {
        fClusterClock.Start(false);
        fPhysics->fClusters = fClimb->Run(fData->fVoxels);
        fClusterClock.Stop();
        // Apply MultiStep algorithm
        fMultiStep->SetClusters(&(fPhysics->fClusters));
        fMultiStep->SetRPs(&(fPhysics->fRPs));
        fMultiStep->Run();
    }
    else
    {
        fPhysics->fClusters = {};
    }
}

void ActRoot::TPCDetector::PrintReports() const
{
    std::cout << BOLDGREEN << "---- ClusterMethod.Run() timer ----" << '\n';
    fClusterClock.Print();
    std::cout << RESET;

    if(fMultiStep)
        fMultiStep->PrintClocks();
}
