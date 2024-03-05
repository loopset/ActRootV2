#include "ActMultiRegion.h"

#include "ActAlgoFuncs.h"
#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActRegion.h"
#include "ActVoxel.h"

#include <algorithm>
#include <ios>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

ActAlgorithm::MultiRegion::MultiRegion()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

void ActAlgorithm::MultiRegion::AddRegion(unsigned int r, const std::vector<double>& vec)
{
    // Assert right dimension
    if(vec.size() != 4)
        throw std::runtime_error("MultiRegion::AddRegion(): vec in config file for idx " + std::to_string(r) +
                                 " has size != 4 required for 2D");
    RegionType type;
    if(r == 0)
        type = RegionType::EBeam;
    else if(r == 1)
        type = RegionType::ELight;
    else if(r == 2)
        type = RegionType::EHeavy;
    else
        type = RegionType::ENone;
    fRegions[type] = {vec[0], vec[1], vec[2], vec[3]};
}

void ActAlgorithm::MultiRegion::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "multiregion.conf";
    // Parse
    ActRoot::InputParser parser {conf};
    auto mr {parser.GetBlock("MultiRegion")};
    // Add regions
    auto regions {mr->GetMappedValuesVectorOf<double>("r")};
    for(const auto& [idx, vec] : regions)
        AddRegion(idx, vec);

    // Enable global algorithm or not
    if(mr->CheckTokenExists("IsEnabled", false))
        fIsEnabled = mr->GetBool("IsEnabled");

    // Parameters of Merge algorithm
    if(mr->CheckTokenExists("EnableMerge"))
        fEnableMerge = mr->GetBool("EnableMerge");
    if(mr->CheckTokenExists("MergeDistThresh", !fEnableMerge))
        fMergeDistThresh = mr->GetDouble("MergeDistThresh");
    if(mr->CheckTokenExists("MergeMinParallel", !fEnableMerge))
        fMergeMinParallel = mr->GetDouble("MergeMinParallel");
    if(mr->CheckTokenExists("MergeChi2Factor", !fEnableMerge))
        fMergeChi2Factor = mr->GetDouble("MergeChi2Factor");

    // Parameters of find RP
    if(mr->CheckTokenExists("RPMaxDist", !fIsEnabled))
        fRPMaxDist = mr->GetDouble("RPMaxDist");

    // Init clocks
    fClockLabels.push_back("BreakIntoRegions");
    fClocks.push_back({});

    fClockLabels.push_back("BreakingClusters");
    fClocks.push_back({});

    fClockLabels.push_back("ProcessingNotBeam");
    fClocks.push_back({});
}

void ActAlgorithm::MultiRegion::Run()
{
    // 0-> Reset previously set variables
    fAssign.clear();
    if(!fIsEnabled)
        return;
    // 1-> Break set vector of clusters into regions
    fClocks[0].Start(false);
    BreakIntoRegions();
    fClocks[0].Stop();
    // 2-> Merge similar tracks
    if(fEnableMerge)
        MergeClusters();
    // 3-> Assign regions!
    Assign();
    // 4-> Find RP
    FindRP();
    // Always reset index at the end
    ResetIndex();
}

ActAlgorithm::RegionType ActAlgorithm::MultiRegion::AssignRangeToRegion(ClusterIt it)
{
    for(const auto& [name, r] : fRegions)
    {
        auto x {it->GetXRange()};
        auto y {it->GetYRange()};
        if(r.IsInside(x, y))
        {
            // Set beam
            if(name == RegionType::EBeam)
                it->SetBeamLike(true);
            return name;
        }
    }
    return RegionType::ENone;
}

ActAlgorithm::RegionType ActAlgorithm::MultiRegion::AssignVoxelToRegion(const ActRoot::Voxel& v)
{
    for(const auto& [name, r] : fRegions)
    {
        if(r.IsInside(v.GetPosition()))
            return name;
    }
    return RegionType::ENone;
}

void ActAlgorithm::MultiRegion::ProcessNotBeam(BrokenVoxels& broken)
{
    fClocks[2].Start(false);
    // Auxiliar structure
    std::vector<std::unordered_map<RegionType, std::vector<ActRoot::Voxel>>> aux;
    // 1-> Run for each voxel
    for(const auto& cluster : broken)
    {
        // Init row
        aux.push_back({});
        for(const auto& v : cluster)
        {
            auto r {AssignVoxelToRegion(v)};
            aux.back()[r].push_back(v);
        }
    }
    // Build new clusters and insert back
    auto& clusters {fData->fClusters};
    for(auto& regions : aux)
    {
        for(auto& [name, voxels] : regions)
        {
            auto [newClusters, noise] {fAlgo->Run(voxels)};
            for(int idx = 0, size = newClusters.size(); idx < size; idx++)
            {
                newClusters[idx].SetClusterID(fData->fClusters.size() + idx);
                newClusters[idx].SetRegionType(name);
                if(fIsVerbose)
                {
                    std::cout << "-> Adding new cluster " << idx << '\n';
                    clusters.back().Print();
                }
            }
            fData->fClusters.insert(fData->fClusters.end(), std::make_move_iterator(newClusters.begin()),
                                    std::make_move_iterator(newClusters.end()));
        }
    }
    fClocks[2].Stop();
}

bool ActAlgorithm::MultiRegion::BreakCluster(ClusterIt it, BrokenVoxels& broken)
{
    fClocks[1].Start(false);
    it->SetRegionType(RegionType::EBeam);
    // 1-> Attempt to break the beam region
    if(auto r {fRegions.find(RegionType::EBeam)}; r != fRegions.end())
    {
        if(fIsVerbose)
        {
            std::cout << "-> Breaking cluster " << it->GetClusterID() << '\n';
            std::cout << "   with initial size : " << it->GetSizeOfVoxels() << '\n';
        }
        // Get voxels by reference
        auto& refVoxels {it->GetRefToVoxels()};
        // Partition them according to beam region
        auto part {std::partition(refVoxels.begin(), refVoxels.end(),
                                  [&](const ActRoot::Voxel& v) { return r->second.IsInside(v.GetPosition()); })};
        // New voxels
        broken.push_back({});
        broken.back().insert(broken.back().end(), std::make_move_iterator(part),
                             std::make_move_iterator(refVoxels.end()));
        refVoxels.erase(part, refVoxels.end());
        if(fIsVerbose)
            std::cout << "   after size : " << refVoxels.size() << '\n';
        // Refit
        it->ReFit();
        it->ReFillSets();
        // Mark not to merge
        it->SetToMerge(false);
    }
    fClocks[1].Stop();
    // Mark to delete is sized fell below threshold
    // Return true if it is fine
    // False if must delete
    return (it->GetSizeOfVoxels() >= fAlgo->GetMinPoints());
}

void ActAlgorithm::MultiRegion::BreakIntoRegions()
{
    if(fIsVerbose)
        std::cout << BOLDYELLOW << "---- MultiRegion Verbose ----" << '\n';
    // New voxels to process later
    BrokenVoxels broken;
    // Clusters to delete
    std::set<unsigned int, std::greater<unsigned int>> toDelete;
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    {
        // Attempt to assign region based on cluster ranges
        auto r {AssignRangeToRegion(it)};
        // If found, save and continue
        if(r != RegionType::ENone)
        {
            it->SetRegionType(r);
            continue;
        }
        else
        {
            auto ok {BreakCluster(it, broken)};
            // If size of remaining voxels is lower than Cluster algorithm threshold, mark to delete
            if(!ok)
                toDelete.insert(std::distance(fData->fClusters.begin(), it));
        }
    }
    // 2-> Delete clusters
    for(const auto& idx : toDelete)
    {
        fData->fClusters.erase(fData->fClusters.begin() + idx);
        if(fIsVerbose)
            std::cout << "-> Deleting cluster " << (fData->fClusters.begin() + idx)->GetClusterID() << '\n';
    }

    // 2-> Process the leftover voxels
    ProcessNotBeam(broken);
    ResetIndex();
    if(fIsVerbose)
        std::cout << RESET << '\n';
}

void ActAlgorithm::MultiRegion::ResetIndex()
{
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
        (fData->fClusters)[i].SetClusterID(i);
}

void ActAlgorithm::MultiRegion::Assign()
{
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    {
        fAssign[it->GetRegionType()].push_back(it);
    }
}

void ActAlgorithm::MultiRegion::FindRP()
{
    typedef std::vector<std::pair<XYZPoint, std::pair<int, int>>> RPVector; // includes RP and pair of indexes as values
    auto comp {[](const auto& l, const auto& r) { return l.first < r.first; }};
    typedef std::set<std::pair<double, std::pair<XYZPoint, std::pair<int, int>>>, decltype(comp)> RPSet;
    RPSet rps(comp);
    // Verbose
    if(fIsVerbose)
    {
        std::cout << BOLDGREEN << "---- FindRP verbose ----" << '\n';
        for(const auto& [name, its] : fAssign)
        {
            std::cout << "-> Region : " << RegionTypeAsStr(name) << '\n';
            for(const auto& it : its)
                std::cout << "   cluster #" << it->GetClusterID() << '\n';
        }
    }
    for(const auto& [oname, oclusters] : fAssign)
    {
        if(oname != RegionType::EBeam)
            continue;
        for(const auto& out : oclusters)
        {
            for(const auto& [iname, iclusters] : fAssign)
            {
                if(iname == RegionType::EBeam)
                    continue;
                for(const auto& in : iclusters)
                {
                    // Compute RP
                    auto [pA, pB, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                                       in->GetLine().GetPoint(), in->GetLine().GetDirection())};
                    // Get mean
                    XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};
                    // Ckeck all of them are valid
                    auto okA {IsRPValid(pA, fMStep->GetTPCParameters())};
                    auto okB {IsRPValid(pB, fMStep->GetTPCParameters())};
                    auto okAB {IsRPValid(rp, fMStep->GetTPCParameters())};
                    // Distance condition
                    auto okDist {std::abs(dist) <= fRPMaxDist};
                    // Verbose
                    if(fIsVerbose)
                    {
                        std::cout << "······························" << '\n';
                        std::cout << "<i, j> : <" << out->GetClusterID() << ", " << in->GetClusterID() << ">" << '\n';
                        std::cout << "okA    : " << std::boolalpha << okA << '\n';
                        std::cout << "okB    : " << std::boolalpha << okB << '\n';
                        std::cout << "okAB   : " << std::boolalpha << okAB << '\n';
                        std::cout << "okDist : " << std::boolalpha << okDist << '\n';
                        std::cout << "dist   : " << dist << '\n';
                    }
                    if(okA && okB && okAB && okDist)
                        rps.insert({dist, {rp, {out->GetClusterID(), in->GetClusterID()}}});
                }
            }
        }
    }


    // Print
    if(fIsVerbose)
    {
        for(const auto& [dist, data] : rps)
        {
            std::cout << "RP : " << data.first << " at <" << data.second.first << ", " << data.second.second << ">"
                      << '\n';
            std::cout << "dist : " << dist << '\n';
        }
        std::cout << RESET << '\n';
    }

    // Add to TPCData!
    if(rps.size() > 0)
        fData->fRPs.push_back(rps.begin()->second.first);
}

void ActAlgorithm::MultiRegion::MergeClusters()
{
    MergeSimilarClusters(&(fData->fClusters), fMergeDistThresh, fMergeMinParallel, fMergeChi2Factor, fIsVerbose);
    // Reset index
    ResetIndex();
}

void ActAlgorithm::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "**** MultiRegion ****" << '\n';
    std::cout << "-> IsEnabled   ? " << std::boolalpha << fIsEnabled << '\n';
    if(fIsEnabled)
    {
        for(const auto& [type, r] : fRegions)
        {
            std::cout << "-> Region : " << RegionTypeAsStr(type) << '\n';
            std::cout << "   ";
            r.Print();
        }
        std::cout << "-> EnableMerge ? " << std::boolalpha << fEnableMerge << '\n';
        if(fEnableMerge)
        {
            std::cout << "   MergeDistThresh  : " << fMergeDistThresh << '\n';
            std::cout << "   MergeMinParallel : " << fMergeMinParallel << '\n';
            std::cout << "   MergeChi2Factor  : " << fMergeChi2Factor << '\n';
        }
        std::cout << "-> RPMaxDist  : " << fRPMaxDist << '\n';
    }
    std::cout << "******************************" << RESET << '\n';
    // if(fMStep)
    //     fMStep->Print();
}

void ActAlgorithm::MultiRegion::PrintReports() const
{
    std::cout << BOLDMAGENTA << "---- MultiRegion time reports ----" << '\n';
    for(int i = 0; i < fClockLabels.size(); i++)
    {
        std::cout << "-> Clock : " << fClockLabels[i] << '\n';
        std::cout << "   ";
        fClocks[i].Print();
    }
    std::cout << RESET << '\n';
}
