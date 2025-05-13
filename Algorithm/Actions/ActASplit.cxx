#include "ActASplit.h"

#include "ActClIMB.h"
#include "ActColors.h"
#include "ActTPCData.h"

#include "TRandom3.h"

#include <memory>
#include <numeric>

void ActAlgorithm::Actions::Split::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> conf)
{
    fIsEnabled = conf->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(conf->CheckTokenExists("NiterRANSAC"))
        fNiterRANSAC = conf->GetInt("NiterRANSAC");
    if(conf->CheckTokenExists("CylinderRadius"))
        fCylinderRadius = conf->GetInt("CylinderRadius");
    if(conf->CheckTokenExists("MinChi2"))
        fMinChi2 = conf->GetInt("MinChi2");
}

void ActAlgorithm::Actions::Split::Run()
{
    gRandom->SetSeed(0);
    auto& fClusters {fTPCData->fClusters};
    for(int i = 0; i < 2; i++) // Search a maximum of 2 times for the best clusters
    {
        std::vector<ActRoot::Cluster> toAdd {};
        for(auto it = fClusters.begin(); it != fClusters.end();)
        {
            if(it->GetLine().GetChi2() < fMinChi2) // First we need to see what cluster need RANSAC by its chi2
            {
                it++;
                continue;
            }

            // 1. Select a random subset of points to be the model
            // 2. Solve for the model
            // 3. Find the inliers to the model
            // 4. Repeat 1-3 for N iterations, and choose the model with the most inliers

            // Get the inliers and outliers for this iteration of RANSAC
            auto inliersAndOutliersVector = GetInliersAndOutliers(&(*it));
            GetBestFit(inliersAndOutliersVector);
            ApplyContinuity(toAdd, inliersAndOutliersVector);
            // Apply changes to fClusters
            it = fClusters.erase(it);
        }
        fClusters.insert(fClusters.end(), std::make_move_iterator(toAdd.begin()),
                             std::make_move_iterator(toAdd.end()));
    }
}

void ActAlgorithm::Actions::Split::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  NiterRANSAC    : " << fNiterRANSAC << '\n';
    std::cout << "  CylinderRadius : " << fCylinderRadius << '\n';
    std::cout << "  MinChi2        : " << fMinChi2 << '\n';
    std::cout << "······························" << RESET << '\n';
}


void ActAlgorithm::Actions::Split::SortClustersInliersAndOutliers(InliersOutliersPair& inliersAndOutliersVector)
{
    auto& inliersVector  = inliersAndOutliersVector.first;
    auto& outliersVector = inliersAndOutliersVector.second;

    const auto size = inliersVector.size();
    std::vector<int> index(size);
    std::iota(index.begin(), index.end(), 0); // make index vectors

    // Sort index vector based on the size of the inliers
    std::sort(index.begin(), index.end(), [&](size_t a, size_t b) {
        return inliersVector[a].GetVoxels().size() > inliersVector[b].GetVoxels().size();
    });

    // Reorder the inliers and outliers vectors based on the sorted indices
    std::vector<ActRoot::Cluster> sortedInliers, sortedOutliers;
    sortedInliers.reserve(size);
    sortedOutliers.reserve(size);
    
    for (int i : index)
    {
        sortedInliers.emplace_back(std::move(inliersVector[i]));
        sortedOutliers.emplace_back(std::move(outliersVector[i]));
    }

    // Replace the original vectors with the sorted ones
    inliersVector  = std::move(sortedInliers);
    outliersVector = std::move(sortedOutliers);
}

ActAlgorithm::Actions::Split::InliersOutliersPair
ActAlgorithm::Actions::Split::GetInliersAndOutliers(ActRoot::Cluster* cluster)
{
    const auto& voxels = cluster->GetVoxels();
    std::vector<ActRoot::Cluster> inliersVector {};
    std::vector<ActRoot::Cluster> outliersVector {};
    for(int i = 0; i < fNiterRANSAC; i++)
    {
        ActRoot::Cluster inliers {};
        ActRoot::Cluster outliers {};

        // 1. Select a random subset of points to be the model
        double sizeCluster = voxels.size();
        auto voxel1 = voxels[gRandom->Integer(sizeCluster)];
        auto voxel2 = voxels[gRandom->Integer(sizeCluster)];
        // 2. Solve for the model
        ActRoot::Line line(voxel1.GetPosition(), voxel2.GetPosition());
        // 3. Find the inliers to the model
        for(auto voxel : voxels)
        {
            if(line.DistanceLineToPoint(voxel.GetPosition()) < fCylinderRadius)
            {
                inliers.AddVoxel(voxel);
            }
            else
            {
                outliers.AddVoxel(voxel);
            }
        }
        inliersVector.push_back(inliers);
        outliersVector.push_back(outliers);
    }
    // Sort the clusters based on the size of the inliers
    InliersOutliersPair inliersAndOutliersVector = {inliersVector, outliersVector};
    SortClustersInliersAndOutliers(inliersAndOutliersVector);
    return inliersAndOutliersVector;
}

void ActAlgorithm::Actions::Split::GetBestFit(InliersOutliersPair& inliersAndOutliersVector)
{
    auto& inliersVector = inliersAndOutliersVector.first;
    auto& outliersVector = inliersAndOutliersVector.second;
    // Initialize variables for loop
    for(int i = 0; i < fSavedIterations; i++)
    {
        inliersVector[i].ReFit();
    }
    // Sort the first fSavedIteration clusters by chi2
    std::sort(inliersVector.begin(), inliersVector.begin() + fSavedIterations,
              [](const ActRoot::Cluster& a, const ActRoot::Cluster& b)
              { return a.GetLine().GetChi2() < b.GetLine().GetChi2(); });
}

void ActAlgorithm::Actions::Split::ApplyContinuity(std::vector<ActRoot::Cluster>& toAdd,
                                                   InliersOutliersPair& inliersAndOutliersVector)
{
    toAdd.push_back(inliersAndOutliersVector.first[0]);
    auto clusterOutliersAfter = fClimb->Run(inliersAndOutliersVector.second[0].GetRefToVoxels());
    for(auto clusterAfter : clusterOutliersAfter.first)
    {
        toAdd.push_back(clusterAfter);
    }
}