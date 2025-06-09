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
        fCylinderRadius = conf->GetDouble("CylinderRadius");
    if(conf->CheckTokenExists("MinChi2"))
        fMinChi2 = conf->GetDouble("MinChi2");
    if(conf->CheckTokenExists("SavedIterations"))
        fSavedIterations = conf->GetInt("SavedIterations");
}

void ActAlgorithm::Actions::Split::Run()
{
    if(!fIsEnabled)
        return;

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
        fClusters.insert(fClusters.end(), std::make_move_iterator(toAdd.begin()), std::make_move_iterator(toAdd.end()));
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


void ActAlgorithm::Actions::Split::SortClustersInliersAndOutliers(PairsVector& inliersAndOutliersVector)
{
    std::sort(inliersAndOutliersVector.begin(), inliersAndOutliersVector.end(),
              [](const auto& a, const auto& b) { return a.first.GetVoxels().size() > b.first.GetVoxels().size(); });
}

ActAlgorithm::Actions::Split::PairsVector ActAlgorithm::Actions::Split::GetInliersAndOutliers(ActRoot::Cluster* cluster)
{
    PairsVector inliersAndOutliersVector {};

    const auto& voxels = cluster->GetVoxels();
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
        InliersOutliersPair inliersAndOutliers = {inliers, outliers};
        inliersAndOutliersVector.push_back(inliersAndOutliers);
    }
    // Sort the clusters based on the size of the inliers
    SortClustersInliersAndOutliers(inliersAndOutliersVector);
    return inliersAndOutliersVector;
}

void ActAlgorithm::Actions::Split::GetBestFit(PairsVector& inliersAndOutliersVector)
{
    // Refit only the first `fSavedIterations` inliers
    for(int i = 0; i < fSavedIterations; ++i)
    {
        inliersAndOutliersVector[i].first.ReFit(); // Refit the inlier (first of the pair)
    }

    // Sort the first `fSavedIterations` pairs by chi2 of the inlier
    std::sort(inliersAndOutliersVector.begin(),
              inliersAndOutliersVector.begin() +
                  std::min(fSavedIterations, static_cast<int>(inliersAndOutliersVector.size())),
              [](const auto& a, const auto& b) { return a.first.GetLine().GetChi2() < b.first.GetLine().GetChi2(); });
}

void ActAlgorithm::Actions::Split::ApplyContinuity(std::vector<ActRoot::Cluster>& toAdd, PairsVector& clusterPairs)
{
    // Add the first inlier (best chi2)
    toAdd.push_back(clusterPairs[0].first);
    // Run Climb on the outliers of the first pair and add them
    auto clusterOutliersAfter = fClimb->Run(clusterPairs[0].second.GetRefToVoxels());
    for(const auto& clusterAfter : clusterOutliersAfter.first)
    {
        toAdd.push_back(clusterAfter);
    }
}