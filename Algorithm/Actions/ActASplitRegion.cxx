#include "ActASplitRegion.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <functional>
#include <memory>
#include <set>

void ActAlgorithm::Actions::SplitRegion::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> conf)
{
    fIsEnabled = conf->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(conf->CheckTokenExists("MinVoxelsAfterBreak"))
        fMinVoxelsAfterBreak = conf->GetInt("MinVoxelsAfterBreak");

    auto regions {conf->GetMappedValuesVectorOf<double>("r")};
    for(const auto& [idx, vec] : regions)
        AddRegion(idx, vec);
    CheckRegionsReadout();
}

void ActAlgorithm::Actions::SplitRegion::Run()
{
    if(!fIsEnabled)
        return;
    // New voxels outside beam region
    BrokenVoxels brokenVoxels {};
    // Clusters to delete
    std::set<unsigned int, std::greater<unsigned int>> toDelete {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // If cluster range is already inside the defined region, skip it
        auto r {AssignClusterToRegion(*it)};
        if(r != ActRoot::RegionType::ENone)
        {
            it->SetRegionType(r);
            continue;
        }
        // If not, break the cluster into regions
        auto isOk {BreakCluster(*it, brokenVoxels)};
        // If size of remaining voxels is lower than Cluster algorithm threshold, mark to delete
        if(!isOk)
        {
            toDelete.insert(std::distance(fTPCData->fClusters.begin(), it));
        }
    }
    // Delete clusters
    for(const auto& idx : toDelete)
    {
        fTPCData->fClusters.erase(fTPCData->fClusters.begin() + idx);
        if(fIsVerbose)
            std::cout << "-> Deleting cluster " << (fTPCData->fClusters.begin() + idx)->GetClusterID() << '\n';
    }
    // Process the leftover voxels
    ProcessNotBeam(brokenVoxels);
    ResetID();
}

void ActAlgorithm::Actions::SplitRegion::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  MinVoxelsAfterBreak : " << fMinVoxelsAfterBreak << '\n';
    std::cout << "······························" << RESET << '\n';
}

void ActAlgorithm::Actions::SplitRegion::AddRegion(unsigned int r, const std::vector<double>& vec)
{
    // Assert right dimension
    if(vec.size() != 4)
        throw std::runtime_error("MultiRegion::AddRegion(): vec in config file for idx " + std::to_string(r) +
                                 " has size != 4 required for 2D");
    ActRoot::RegionType type;
    if(r == 0)
        type = ActRoot::RegionType::EBeam;
    else if(r == 1)
        type = ActRoot::RegionType::ELight;
    else if(r == 2)
        type = ActRoot::RegionType::EHeavy;
    else
        type = ActRoot::RegionType::ENone;
    fRegions[type] = {vec[0], vec[1], vec[2], vec[3]};
}

void ActAlgorithm::Actions::SplitRegion::CheckRegionsReadout()
{
    bool beam {};
    for(const auto& [name, r] : fRegions)
        if(name == ActRoot::RegionType::EBeam)
            beam = true;
    if(!beam)
        throw std::runtime_error("MultiRegion::CheckRegions(): algorithm does not work without a Beam region set. Add "
                                 "it with the r0 command");
}

ActRoot::RegionType ActAlgorithm::Actions::SplitRegion::AssignClusterToRegion(ActRoot::Cluster& cluster)
{
    for(const auto& [name, r] : fRegions)
    {
        auto x {cluster.GetXRange()};
        auto y {cluster.GetYRange()};
        if(r.IsInside(x, y))
        {
            return name;
        }
    }
    return ActRoot::RegionType::ENone;
}

bool ActAlgorithm::Actions::SplitRegion::BreakCluster(ActRoot::Cluster& cluster, BrokenVoxels& brokenVoxels)
{
    // Break the cluster into regions
    if(auto r {fRegions.find(ActRoot::RegionType::EBeam)}; r != fRegions.end())
    {
        auto& refVoxels {cluster.GetRefToVoxels()};
        auto part {std::partition(refVoxels.begin(), refVoxels.end(),
                                [&](const ActRoot::Voxel& v) { return r->second.IsInside(v.GetPosition()); })};
        // New voxels
        brokenVoxels.push_back({});
        brokenVoxels.back().insert(brokenVoxels.back().end(), std::make_move_iterator(part),
                                std::make_move_iterator(refVoxels.end()));
        refVoxels.erase(part, refVoxels.end());
        // Refit
        cluster.ReFit();
        cluster.ReFillSets();
        cluster.SetRegionType(r->first);
    }
    // Check if size fell below threshold
    return {cluster.GetSizeOfVoxels() >= fMinVoxelsAfterBreak};
}

ActRoot::RegionType ActAlgorithm::Actions::SplitRegion::AssignVoxelToRegion(const ActRoot::Voxel& voxel)
{
    for(const auto& [name, r] : fRegions)
    {
        if(r.IsInside(voxel.GetPosition()))
        {
            return name;
        }
    }
    return ActRoot::RegionType::ENone;
}

void ActAlgorithm::Actions::SplitRegion::ProcessNotBeam(BrokenVoxels& brokenVoxels)
{
    // Auxiliar structure
    std::vector<std::unordered_map<ActRoot::RegionType, std::vector<ActRoot::Voxel>>> aux;
    // 1-> Run for each voxel
    for(const auto& cluster : brokenVoxels)
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
    auto& clusters {fTPCData->fClusters};
    for(auto& regions : aux)
    {
        for(auto& [name, voxels] : regions)
        {
            auto [newClusters, noise] {fAlgo->Run(voxels)};
            for(int idx = 0, size = newClusters.size(); idx < size; idx++)
            {
                newClusters[idx].SetClusterID(fTPCData->fClusters.size() + idx);
                newClusters[idx].SetRegionType(name);
                if(fIsVerbose)
                {
                    std::cout << "-> Adding new cluster " << idx << '\n';
                    clusters.back().Print();
                }
            }
            fTPCData->fClusters.insert(fTPCData->fClusters.end(), std::make_move_iterator(newClusters.begin()),
                                    std::make_move_iterator(newClusters.end()));
        }
    }
}

void ActAlgorithm::Actions::SplitRegion::ResetID()
{
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
        (fTPCData->fClusters)[i].SetClusterID(i);
}