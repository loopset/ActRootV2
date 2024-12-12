#include "ActTPCDetector.h"

#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActMultiAction.h"
#include "ActMultiRegion.h"
#include "ActMultiStep.h"
#include "ActOptions.h"
#include "ActRANSAC.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActTypes.h"
#include "ActVoxel.h"

#include "TString.h"
#include "TTree.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

ActRoot::TPCDetector::~TPCDetector()
{
    if(fDelMEvent)
    {
        delete fMEvent;
        fMEvent = nullptr;
    }
    if(fDelData)
    {
        delete fData;
        fData = nullptr;
    }
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
    if(config->CheckTokenExists("CleanPadMatrix", true))
        fCleanPadMatrix = config->GetBool("CleanPadMatrix");
    if(config->CheckTokenExists("CleanPadMatrixPars", true))
    {
        auto pars {config->GetDoubleVector("CleanPadMatrixPars")};
        fMinTBtoDelete = pars.at(0);
        fMinQtoDelete = pars.at(1);
    }
    if(config->CheckTokenExists("CleanDuplicatedVoxels", true))
        fCleanDuplicatedVoxels = config->GetBool("CleanDuplicatedVoxels");
    // Init of algorithms based on mode
    auto mode {ActRoot::Options::GetInstance()->GetMode()};
    // Cluster method
    if(mode == ModeType::EReadTPC || mode == ModeType::EFilter || mode == ModeType::EFilterMerge ||
       mode == ModeType::EGui)
        if(config->CheckTokenExists("ClusterMethod"))
            InitClusterMethod(config->GetString("ClusterMethod"));
    // Filter method
    if(mode == ModeType::EFilter || mode == ModeType::EFilterMerge || mode == ModeType::EGui)
        if(config->CheckTokenExists("FilterMethod"))
            InitFilterMethod(config->GetString("FilterMethod"));
}

void ActRoot::TPCDetector::InitClusterMethod(const std::string& method)
{
    TString m {method};
    m.ToLower();
    if(m == "ransac")
    {
        fCluster = std::make_shared<ActAlgorithm::RANSAC>();
        fCluster->ReadConfiguration();
    }
    else if(m == "climb")
    {
        auto c {std::make_shared<ActAlgorithm::ClIMB>()};
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
        fFilter = std::make_shared<ActAlgorithm::MultiStep>();
    else if(m == "multiregion")
        fFilter = std::make_shared<ActAlgorithm::MultiRegion>();
    else if(m == "multiaction")
        fFilter = std::make_shared<ActAlgorithm::MultiAction>();
    else if(m == "none")
        return;
    else
        throw std::runtime_error("TPCDetector::InitFilterMethod: no listed method from Multistep or None");
    // Common for all
    fFilter->ReadConfiguration();
    fFilter->SetTPCParameters(&fPars);
    fFilter->SetClusterPtr(fCluster);
    // Data is set afterwards
}

void ActRoot::TPCDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"LookUp", "PadAlign"};
    // Add LookUp table
    fCalMan->ReadLookUpTable(config->GetString("LookUp"));
    // Pad align table
    if(config->CheckTokenExists("PadAlign", true))
        fCalMan->ReadPadAlign(config->GetString("PadAlign"));
}

void ActRoot::TPCDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    // delete if already exists
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    // Set branch address
    tree->SetBranchAddress("data", &fMEvent);
    // Set to delete on destructor
    fDelMEvent = true;
}

void ActRoot::TPCDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->Branch("TPCData", &fData);
    // Set to delete in destructor
    fDelData = true;
}

void ActRoot::TPCDetector::InitInputFilter(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new TPCData;
    tree->SetBranchStatus("fRaw*", false); // do not save fRaw voxels in filter tree
    tree->SetBranchAddress("TPCData", &fData);
    // Delete in destructor
    fDelData = true;
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
    fVoxels.clear();
    fGlobalIndex.clear();
    if(fCleanPadMatrix)
        fPadMatrix.clear();
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
    // Clean pad matrix from saturated tracks along Z
    if(fCleanPadMatrix)
        CleanPadMatrix();

    // And now build clusters!
    if(fCluster)
        std::tie(fData->fClusters, fData->fRaw) = fCluster->Run(fVoxels, true); // enable returning of noise
    else
        fData->fRaw = std::move(fVoxels);
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

unsigned int ActRoot::TPCDetector::BuildGlobalIndex(const int& x, const int& y, const int& z)
{
    return x + y * fPars.GetNPADSX() + z * fPars.GetNPADSX() * fPars.GetNPADSY();
}

unsigned int ActRoot::TPCDetector::BuildGlobalPadIndex(const int& x, const int& y)
{
    return x + y * fPars.GetNPADSX();
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
        throw std::runtime_error("TPCDetector::ReadHits(): error while reading hits in TPCDetector -> LT table out of "
                                 "range, check ACQ parameters");
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
        uint8_t offset {static_cast<uint8_t>((int)padz - (binZ * fPars.GetREBINZ()))};
        padz = (float)binZ; // store z as binNumber instead of binCenter
        // padz = fPars.GetREBINZ() * binZ + ((fPars.GetREBINZ() <= 1) ? 0.0 : (double)fPars.GetREBINZ() / 2);
        // Store the offset to recover precise value lately

        // Build Voxel
        // Apply cut on saturated flag if desired
        if(fCleanSaturatedMEvent && coas.hasSaturation)
        {
            ; // if CleanSatVoxels is enables and voxel does have hasSat = true, do not fill in vector
        }
        else
        {
            // Get global index
            auto global {BuildGlobalIndex(padx, pady, padz)};
            if(fGlobalIndex[global] == 0)
            {
                fVoxels.push_back({ROOT::Math::XYZPointF {(float)padx, (float)pady, padz}, qcal, coas.hasSaturation});
                fVoxels.back().AddZ(offset);
                fGlobalIndex[global] = fVoxels.size() - 1; // map global index to size in fVoxels vector
                if(fCleanPadMatrix)
                {
                    auto global2d {BuildGlobalPadIndex(padx, pady)};
                    fPadMatrix[global2d].first.insert(fVoxels.size() - 1);
                    fPadMatrix[global2d].second += qcal;
                }
            }
            else
            {
                // INFO: we found for REBINZ = 1 still some duplicated voxels
                // could be an issue on Thomas' side so we leave here the cout
                // std::cout << "-> Duplicated voxel with global id : " << global << '\n';
                if(fCleanDuplicatedVoxels)
                    ;
                else
                {
                    auto idx {fGlobalIndex[global]};
                    fVoxels[idx].SetCharge(fVoxels[idx].GetCharge() + qcal);
                    fVoxels[idx].AddZ(offset);
                    if(fCleanPadMatrix)
                    {
                        auto global2d {BuildGlobalPadIndex(padx, pady)};
                        fPadMatrix[global2d].second += qcal;
                    }
                }
            }
        }
    }
}

void ActRoot::TPCDetector::CleanPadMatrix()
{
    for(const auto& [_, pair] : fPadMatrix)
    {
        const auto& idxs {pair.first};
        const auto& totalQ {pair.second};
        if(idxs.size() >= fMinTBtoDelete &&
           totalQ >= fMinQtoDelete) // threshold in time buckets and in Qtotal to delete pad data
        {
            for(const auto& idx : idxs)
                fVoxels.erase(fVoxels.begin() + idx);
        }
    }
}

void ActRoot::TPCDetector::BuildEventFilter()
{
    if(fFilter)
    {
        fFilter->SetTPCData(fData);
        fFilter->Run();
    }
}

void ActRoot::TPCDetector::Print() const
{
    std::cout << BOLDCYAN << "···· TPCDetector ····" << RESET << '\n';
    if(ActRoot::Options::GetInstance()->GetMode() == ModeType::EReadTPC)
    {
        std::cout << BOLDCYAN << "-> CleanSaturation       ? " << std::boolalpha << fCleanSaturatedMEvent << '\n';
        std::cout << "-> CleanPadMatrix        ? " << std::boolalpha << fCleanPadMatrix << '\n';
        std::cout << "-> CleanDuplicatedVoxels ? " << std::boolalpha << fCleanDuplicatedVoxels << RESET << '\n';
    }
    if(fCluster)
        fCluster->Print();
    if(fFilter)
        fFilter->Print();
    std::cout << BOLDCYAN << "······························" << RESET << '\n';
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
    {
        fFilter->ReadConfiguration();
        fFilter->SetTPCParameters(&fPars);
        fFilter->SetClusterPtr(fCluster);
    }
}
