#include "ActAMultiRegion.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <functional>
#include <memory>
#include <set>

void ActAlgorithm::Actions::MultiRegion::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> conf)
{
    fIsEnabled = conf->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(conf->CheckTokenExists("xmin"))
        fxmin = conf->GetInt("xmin");
    if(conf->CheckTokenExists("xmax"))
        fxmax = conf->GetInt("xmax");
    if(conf->CheckTokenExists("ymin"))
        fymin = conf->GetInt("ymin");
    if(conf->CheckTokenExists("ymax"))
        fymax = conf->GetDouble("ymax");
    if(conf->CheckTokenExists("MinVoxelsAfterBreak"))
        fMinVoxelsAfterBreak = conf->GetInt("MinVoxelsAfterBreak");
}

void ActAlgorithm::Actions::MultiRegion::Run()
{
    if(!fIsEnabled)
        return;
    // New voxels outside beam region
    BrokenVoxels brokenVoxels {};
    auto beamRegion {ActRoot::Region(fxmin, fxmax, fymin, fymax)};
    // Clusters to delete
    std::set<unsigned int, std::greater<unsigned int>> toDelete {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // If cluster range is already inside the defined region, skip it
        if(CheckClusterIsInRegion(*it, beamRegion))
        {
            it->SetRegionType(ActRoot::RegionType::EBeam);
            continue;
        }
        // If not, break the cluster into regions
        auto isOk {BreakCluster(*it, brokenVoxels, beamRegion)};
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
    ProcessNotBeam(brokenVoxels, beamRegion);
    ResetID();
}

void ActAlgorithm::Actions::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << RESET << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  ymin    : " << fymin << '\n';
    std::cout << "  ymax : " << fymax << '\n';
    std::cout << "  xmin    : " << fxmin << '\n';
    std::cout << "  xmax : " << fxmax << '\n';
    std::cout << "  MinVoxelsAfterBreak : " << fMinVoxelsAfterBreak << '\n';
    std::cout << "······························" << RESET << '\n';
}

bool ActAlgorithm::Actions::MultiRegion::CheckClusterIsInRegion(ActRoot::Cluster& cluster,
                                                                const ActRoot::Region& region)
{
    // Check if the cluster is in the defined region
    auto x {cluster.GetXRange()};
    auto y {cluster.GetYRange()};
    if(region.IsInside(x, y))
        return true;
    else
        return false;
}

bool ActAlgorithm::Actions::MultiRegion::BreakCluster(ActRoot::Cluster& cluster, BrokenVoxels& brokenVoxels,
                                                      ActRoot::Region& region)
{
    // Break the cluster into regions
    auto& refVoxels {cluster.GetRefToVoxels()};
    auto part {std::partition(refVoxels.begin(), refVoxels.end(),
                              [&](const ActRoot::Voxel& v) { return region.IsInside(v.GetPosition()); })};
    // New voxels
    brokenVoxels.push_back({});
    brokenVoxels.back().insert(brokenVoxels.back().end(), std::make_move_iterator(part),
                               std::make_move_iterator(refVoxels.end()));
    refVoxels.erase(part, refVoxels.end());
    // Refit
    cluster.ReFit();
    cluster.ReFillSets();
    cluster.SetRegionType(ActRoot::RegionType::EBeam);
    // Check if size fell below threshold
    return {cluster.GetSizeOfVoxels() >= fMinVoxelsAfterBreak};
}

ActRoot::RegionType ActAlgorithm::Actions::MultiRegion::AssignVoxelToRegion(const ActRoot::Voxel& voxel,
                                                                           const ActRoot::Region& region)
{
    // Assign the voxel to the region
    if(region.IsInside(voxel.GetPosition()))
        return ActRoot::RegionType::EBeam;
    else
        return ActRoot::RegionType::ENone;
}

void ActAlgorithm::Actions::MultiRegion::ProcessNotBeam(BrokenVoxels& brokenVoxels, const ActRoot::Region& region)
{
    std::vector<ActRoot::Voxel> beamVoxels;
    std::vector<ActRoot::Voxel> otherVoxels;

    for(const auto& cluster : brokenVoxels)
    {
        for(const auto& voxel : cluster)
        {
            auto r = AssignVoxelToRegion(voxel, region);
            if(r == ActRoot::RegionType::EBeam)
                beamVoxels.push_back(voxel);
            else
                otherVoxels.push_back(voxel);
        }
    }

    // Apply continuity algorithm to broken voxels
    auto processRegion = [&](const std::vector<ActRoot::Voxel>& voxels, ActRoot::RegionType regionType)
    {
        auto [newClusters, noise] = fAlgo->Run(voxels);
        for(int idx = 0; idx < static_cast<int>(newClusters.size()); ++idx)
        {
            newClusters[idx].SetClusterID(fTPCData->fClusters.size() + idx);
            newClusters[idx].SetRegionType(regionType);

            if(fIsVerbose)
            {
                std::cout << "-> Adding new cluster " << idx << '\n';
                fTPCData->fClusters.back().Print();
            }
        }

        fTPCData->fClusters.insert(fTPCData->fClusters.end(),
                                std::make_move_iterator(newClusters.begin()),
                                std::make_move_iterator(newClusters.end()));
    };

    // 3 -> Procesa cada conjunto
    processRegion(beamVoxels, ActRoot::RegionType::EBeam);
    processRegion(otherVoxels, ActRoot::RegionType::ENone);
}

void ActAlgorithm::Actions::MultiRegion::ResetID()
{
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
        (fTPCData->fClusters)[i].SetClusterID(i);
}