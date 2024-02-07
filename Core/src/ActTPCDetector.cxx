#include "ActTPCDetector.h"

#include "ActCalibrationManager.h"
#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActMultiStep.h"
#include "ActOptions.h"
#include "ActRANSAC.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActTypes.h"
#include "ActVoxel.h"

#include "TString.h"
#include "TTree.h"

#include "Math/Point3D.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

ActRoot::TPCParameters::TPCParameters(const std::string& type)
{
    // std::cout << "Initializing detector type: " << type << '\n';
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

void ActRoot::TPCParameters::Print() const
{
    std::cout << BOLDCYAN << "==== TPC parameters ====" << '\n';
    std::cout << "-> NPADSX : " << fNPADSX << '\n';
    std::cout << "-> NPADSY : " << fNPADSY << '\n';
    std::cout << "-> NPADSZ : " << fNPADSZ << '\n';
    std::cout << "-> REBINZ :" << fREBINZ << '\n';
    std::cout << "====================" << '\n';
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
    // Init of algorithms based on mode
    auto mode {ActRoot::Options::GetInstance()->GetMode()};
    // Cluster method
    if(mode == ModeType::EReadTPC || mode == ModeType::EFilter || mode == ModeType::EGui)
        if(config->CheckTokenExists("ClusterMethod"))
            InitClusterMethod(config->GetString("ClusterMethod"));
    // Filter method
    if(mode == ModeType::EFilter || mode == ModeType::EGui)
        if(config->CheckTokenExists("FilterMethod"))
            InitFilterMethod(config->GetString("FilterMethod"));
}

void ActRoot::TPCDetector::InitClusterMethod(const std::string& method)
{
    TString m {method};
    m.ToLower();
    if(m == "ransac")
    {
        auto r {std::make_shared<ActCluster::RANSAC>()};
        r->ReadConfiguration();
        fCluster = r;
    }
    else if(m == "climb")
    {
        auto c {std::make_shared<ActCluster::ClIMB>()};
        c->SetTPCParameters(&fPars);
        c->ReadConfiguration();
        fCluster = c;
    }
    else if(m == "none")
        return;
    else
        throw std::runtime_error("TPCDetector::InitClusterMethod: no listed method from Ransac, Climb and None");
}

void ActRoot::TPCDetector::InitFilterMethod(const std::string& method)
{
    TString m {method};
    m.ToLower();
    if(m == "multistep")
    {
        auto m {std::make_shared<ActCluster::MultiStep>()};
        m->SetTPCParameters(&fPars);
        m->SetClusterPtr(fCluster);
        m->ReadConfiguration();
        fFilter = m;
    }
    else if(m == "none")
        return;
    else
        throw std::runtime_error("TPCDetector::InitFilterMethod: no listed method from Multistep or None");
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    // Add LookUp table
    fCalMan->ReadLookUpTable(config->GetString("LookUp"));
    // Pad align table
    fCalMan->ReadPadAlign(config->GetString("PadAlign"));
}

void ActRoot::TPCDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    // delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    tree->SetBranchAddress("data", &fMEvent);
    // Init voxels
    if(fVoxels)
        delete fVoxels;
    fVoxels = new std::vector<ActRoot::Voxel>;
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

void ActRoot::TPCDetector::InitInputFilter(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->SetBranchAddress("TPCData", &fData);
}

void ActRoot::TPCDetector::InitOutputFilter(std::shared_ptr<TTree> tree)
{
    // Directly from input data, because filter
    // modifies the content on the flight
    tree->Branch("TPCData", &fData);
}

void ActRoot::TPCDetector::ClearEventData()
{
    fData->Clear();
    // if opted, clean pad matrix
    if(fCleanSaturatedVoxels)
        fPadMatrix.clear();
    if(fVoxels)
        fVoxels->clear();
}

void ActRoot::TPCDetector::ClearEventFilter()
{
    // Not needed because we are reading directly from ttree
}

void ActRoot::TPCDetector::BuildEventData(int run, int entry)
{
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
            ReadHits(coas, where);
        }
    }
    // Clean duplicated voxels
    if(fCleanDuplicatedVoxels)
        EnsureUniquenessOfVoxels();
    // Clean pad matrix
    if(fCleanSaturatedVoxels)
        CleanPadMatrix();

    // And now build clusters!
    std::tie(fData->fClusters, fData->fRaw) = fCluster->Run(*fVoxels, true); // enable returning of noise
}

void ActRoot::TPCDetector::Recluster()
{
    if(!fData)
        throw std::runtime_error("TPCDetector::Recluster() :  cannot recluster without TPCData set!");
    if(fCluster)
    {
        std::vector<Voxel> voxels;
        voxels = fData->fRaw;
        for(const auto& cluster : fData->fClusters)
            for(const auto& voxel : cluster.GetVoxels())
                voxels.push_back(voxel);
        std::tie(fData->fClusters, fData->fRaw) = fCluster->Run(voxels, true);
    }
}

void ActRoot::TPCDetector::ReadHits(ReducedData& coas, const int& where)
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
        padz = (float)binZ; // store z as binNumber instead of binCenter
        // padz = fPars.GetREBINZ() * binZ + ((fPars.GetREBINZ() <= 1) ? 0.0 : (double)fPars.GetREBINZ() / 2);

        // Build Voxel
        // Apply cut on saturated flag if desired
        if(fCleanSaturatedMEvent && coas.hasSaturation)
        {
            ; // if CleanSatVoxels is enables and voxel does have hasSat = true, do not fill in vector
        }
        else
        {
            Voxel hit {ROOT::Math::XYZPointF(padx, pady, padz), qcal, coas.hasSaturation};
            fVoxels->push_back(hit);
            // push to pad matrix if enabled
            if(fCleanSaturatedVoxels)
            {
                fPadMatrix[{padx, pady}].first.push_back(fVoxels->size() - 1);
                fPadMatrix[{padx, pady}].second += qcal;
            }
        }
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
                fVoxels->erase(fVoxels->begin() + *it);
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
                  fPars.GetNPADSX() * fPars.GetNPADSY() * (int)pos.Z()};
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
    for(const auto& voxel : *fVoxels)
        set.insert(voxel);
    // Back to vector!
    // auto initSize {(int)fData->fVoxels.size()};
    // auto setSize {(int)set.size()};
    // if(initSize != setSize)
    //     std::cout << "Cleaned some voxels! diff : " << (initSize - setSize) << '\n';
    fVoxels->assign(set.begin(), set.end());
}

void ActRoot::TPCDetector::BuildEventFilter()
{
    fFilter->SetTPCData(fData);
    fFilter->Run();
}

void ActRoot::TPCDetector::Print() const
{
    if(fCluster)
        fCluster->Print();
    if(fFilter)
        fFilter->Print();
}

void ActRoot::TPCDetector::PrintReports() const
{
    if(fCluster)
        fCluster->PrintReports();
    if(fFilter)
        fFilter->PrintReports();
}

void ActRoot::TPCDetector::Reconfigure()
{
    if(fCluster)
        fCluster->ReadConfiguration();
    if(fFilter)
        fFilter->ReadConfiguration();
}
