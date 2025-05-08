#include "ActASplit.h"

#include "ActClIMB.h"
#include "ActColors.h"
#include "ActTPCData.h"

#include "TRandom3.h"

#include <memory>

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
    // Initialize climb algorithm object    
    auto climb {new ActAlgorithm::ClIMB(fTPCPars, 10)};
    std::vector<ActRoot::Cluster> bestClusters {}; // Save the clusters with the best Chi2
    std::vector<ActRoot::Cluster> clustersFinal {}; // Prepare output
    while(true)
    {
        std::vector<ActRoot::Cluster> clustersToRANSAC {};
        // First we need to see what cluster need RANSAC by its chi2
        CheckChi2(fClusters, clustersToRANSAC, clustersFinal);
        if(clustersToRANSAC.size() == 0)
        {
            break;
        }
        // Let's do the RANSAC algorithm
        // 1. Select a random subset of points to be the model
        // 2. Solve for the model
        // 3. Find the inliers to the model
        // 4. Repeat 1-3 for N iterations, and choose the model with the most inliers

        std::vector<ActRoot::Cluster> clustersIteration; // Save clusters after the RANSAC iteration
        for(auto cluster : clustersToRANSAC)
        {
            // 1. Select a random subset of points to be the model
            // 2. Solve for the model
            // 3. Find the inliers to the model
            // 4. Repeat 1-3 for N iterations, and choose the model with the most inliers

            int Niterations = 20;
            // Get the inliers and outliers for this iteration of RANSAC
            auto inliersAndOutliersVector = GetInliersAndOutliers(cluster, Niterations);
            // Get the best fit 
            auto bestFit = GetBestFit(inliersAndOutliersVector, 6);
            auto bestCluster = bestFit.first;
            bestClusters.push_back(bestCluster);
            auto outliers = bestFit.second;
            // Apply continuity to the best cluster and to the outliers with ClIMB algorithm
            ApplyContinuity(clustersIteration, outliers, bestCluster, climb);
        }

        fClusters = clustersIteration;
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

void CheckChi2(std::vector<ActRoot::Cluster>& clusters, std::vector<ActRoot::Cluster>& clustersToRANSAC,
               std::vector<ActRoot::Cluster>& clustersFinal)
{
    for(auto cluster : clusters)
    {
        std::cout << "Chi2 of the cluster: " << cluster.GetLine().GetChi2() << std::endl;
        if(cluster.GetLine().GetChi2() > 1.5)
        {
            clustersToRANSAC.push_back(cluster);
        }
        else
        {
            clustersFinal.push_back(
                cluster); // If not need of RANSAC is already a separated cluster in the shape of a line
        }
    }
}

std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>
SortClustersInliersAndOutliers(std::vector<ActRoot::Cluster>& inliersVector,
                               std::vector<ActRoot::Cluster>& outliersVector)
{
    // 1. Create a vector of pairs
    std::vector<std::pair<ActRoot::Cluster, ActRoot::Cluster>> pairedClusters;
    for(int i = 0; i < inliersVector.size(); i++)
    {
        pairedClusters.emplace_back(inliersVector[i], outliersVector[i]);
    }
    // 2. Sort the pairs based on the size of the inliers
    std::sort(pairedClusters.begin(), pairedClusters.end(),
              [](const auto& a, const auto& b) { return a.first.GetVoxels().size() > b.first.GetVoxels().size(); });
    // 3. Unpack sorted pairs back into inliersVector and outliersVector
    for(int i = 0; i < pairedClusters.size(); i++)
    {
        inliersVector[i] = pairedClusters[i].first;
        outliersVector[i] = pairedClusters[i].second;
    }

    return {inliersVector, outliersVector};
}

std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>
GetInliersAndOutliers(ActRoot::Cluster& cluster, int Niterations)
{
    const auto& voxels = cluster.GetVoxels();
    std::vector<ActRoot::Cluster> inliersVector {};
    std::vector<ActRoot::Cluster> outliersVector {};
    for(int i = 0; i < Niterations; i++)
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
            if(line.DistanceLineToPoint(voxel.GetPosition()) < 2.5)
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
    auto output = SortClustersInliersAndOutliers(inliersVector, outliersVector);
    return output;
}

std::pair<ActRoot::Cluster&, ActRoot::Cluster&>
GetBestFit(std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>& inliersAndOutliersVector,
           int nClusterFit)
{
    double chi2compare = 1000;
    auto& inliersVector = inliersAndOutliersVector.first;
    auto& outliersVector = inliersAndOutliersVector.second;
    // Initialize variables for loop
    ActRoot::Cluster* bestCluster = &inliersVector[0];
    ActRoot::Cluster* bestOutlier = &outliersVector[0];
    std::pair<ActRoot::Cluster, ActRoot::Cluster> output {};
    for(int i = 0; i < nClusterFit; i++)
    {
        auto& inliers = inliersVector[i];

        // Do the fit
        ActRoot::Cluster tempCluster = inliers; // Work on a temporary cluster
        tempCluster.ReFit();
        double chi2 = tempCluster.GetLine().GetChi2();

        if(chi2 < chi2compare)
        {
            chi2compare = chi2;
            bestCluster = &inliersVector[i];  // Guardamos referencia al mejor cluster
            bestOutlier = &outliersVector[i]; // Guardamos referencia al outlier correspondiente
        }
    }
    return {*bestCluster, *bestOutlier};
}

void ApplyContinuity(std::vector<ActRoot::Cluster>& clustersIteration, ActRoot::Cluster& outliers,
                     ActRoot::Cluster& bestCluster, ActAlgorithm::ClIMB* climb)
{
    // auto clustersInliersAfter = climb->Run(bestCluster.GetRefToVoxels());
    // for(auto clusterAfter : clustersInliersAfter.first)
    //{
    //     clustersFinal.push_back(clusterAfter);
    // }
    clustersIteration.push_back(bestCluster);
    auto clusterOutliersAfter = climb->Run(outliers.GetRefToVoxels());
    for(auto clusterAfter : clusterOutliersAfter.first)
    {
        clustersIteration.push_back(clusterAfter);
    }
}