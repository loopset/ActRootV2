#ifndef ActASplit_h
#define ActASplit_h

#include "ActCluster.h"
#include "ActVAction.h"


namespace ActAlgorithm
{
namespace Actions
{
class Split : public VAction
{
private:
    int fNiterRANSAC {};       // Iterations for RANSAC algorithm
    double fCylinderRadius {}; // Cylinder radius to define inliers
    double fMinChi2 {};        // Minimum value of cluster's chi2 to apply RANSAC to it
    int fSavedIterations {};   // Iterations with higher amount of inliers used to get best cluster (least chi2)
public:
    Split() : VAction("Split") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    void CheckChi2(std::vector<ActRoot::Cluster>& clusters, std::vector<ActRoot::Cluster>& clustersToRANSAC,
                   std::vector<ActRoot::Cluster>& clustersFinal);
    std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>
    SortClustersInliersAndOutliers(std::vector<ActRoot::Cluster>& inliersVector,
                                   std::vector<ActRoot::Cluster>& outliersVector);
    std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>
    GetInliersAndOutliers(ActRoot::Cluster& cluster, int Niterations);
    std::pair<ActRoot::Cluster&, ActRoot::Cluster&>
    GetBestFit(std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>& inliersAndOutliersVector,
               int nClusterFit);
    void ApplyContinuity(std::vector<ActRoot::Cluster>& clustersIteration, ActRoot::Cluster& outliers,
                         ActRoot::Cluster& bestCluster, ActAlgorithm::VCluster* climb);
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
