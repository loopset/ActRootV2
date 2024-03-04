#include "ActMultiRegion.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActRegion.h"
#include "ActVoxel.h"

#include <algorithm>
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
    // 1-> Break set vector of clusters into regions
    fClocks[0].Start(false);
    BreakIntoRegions();
    fClocks[0].Stop();
    // 2-> Find reaction point
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

void ActAlgorithm::MultiRegion::ProcessNotBeam(BrokenVoxels& broken, std::vector<RegionType>& assignments)
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
                assignments.push_back(name);
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

bool ActAlgorithm::MultiRegion::BreakCluster(ClusterIt it, BrokenVoxels& broken, std::vector<RegionType>& assignments)
{
    fClocks[1].Start(false);
    assignments.push_back(RegionType::EBeam);
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
        std::cout << BOLDCYAN << "---- MultiRegion Verbose ----" << '\n';
    // 1-> Preliminar assignment and breaking of cluster
    std::vector<RegionType> assignments;
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
            assignments.push_back(r);
            continue;
        }
        else
        {
            auto ok {BreakCluster(it, broken, assignments)};
            // If size of remaining voxels is lower than Cluster algorithm threshold, mark to delete
            if(!ok)
                toDelete.insert(std::distance(fData->fClusters.begin(), it));
        }
    }
    // 2-> Delete clusters
    for(const auto& idx : toDelete)
    {
        assignments.erase(assignments.begin() + idx);
        fData->fClusters.erase(fData->fClusters.begin() + idx);
        if(fIsVerbose)
            std::cout << "-> Deleting cluster " << (fData->fClusters.begin() + idx)->GetClusterID() << '\n';
    }

    // 2-> Process the leftover voxels
    ProcessNotBeam(broken, assignments);
    // 3-> Definitive assignment with new clusters
    // (push_back in the middle of the previous for loop could invalidate previously stored iterators)
    if(fIsVerbose)
        std::cout << "-> Region assignments :" << '\n';
    for(int c = 0, size = fData->fClusters.size(); c < size; c++)
    {
        auto it {fData->fClusters.begin() + c};
        auto r {assignments.at(c)};
        fAssign[r].push_back(it);
        // it->Print();
        if(fIsVerbose)
            std::cout << "   cluster #" << it->GetClusterID() << " at region : " << RegionTypeToStr(r) << '\n';
    }
    if(fIsVerbose)
        std::cout << RESET << '\n';
}

void ActAlgorithm::MultiRegion::ResetIndex()
{
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
        (fData->fClusters)[i].SetClusterID(i);
}

void ActAlgorithm::MultiRegion::FindRP()
{
    for(const auto& [name, clusters] : fAssign)
    {
        if(fIsVerbose)
        {
            std::cout << BOLDYELLOW << "-> Region : " << RegionTypeToStr(name) << '\n';
            for(const auto& cluster : clusters)
            {
                std::cout << "   Cluster #" << cluster->GetClusterID() << '\n';
            }
            std::cout << RESET;
        }
    }
}

std::string ActAlgorithm::MultiRegion::RegionTypeToStr(const RegionType& r) const
{
    if(r == RegionType::EBeam)
        return "Beam";
    if(r == RegionType::ELight)
        return "Light";
    if(r == RegionType::EHeavy)
        return "Heavy";
    if(r == RegionType::ENone)
        return "None";
    return "";
}

void ActAlgorithm::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "**** MultiRegion ****" << '\n';
    for(const auto& [type, r] : fRegions)
    {
        std::cout << "-> Region : " << RegionTypeToStr(type) << '\n';
        std::cout << "   ";
        r.Print();
    }
    std::cout << "******************************" << RESET << '\n';
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
