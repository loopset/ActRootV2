#ifndef ActASplit_h
#define ActASplit_h

#include "ActCluster.h"
#include "ActContinuity.h"
#include "ActVAction.h"


namespace ActAlgorithm
{
namespace Actions
{
class Split : public VAction
{
public:
    using InliersOutliersPair =
        std::pair<ActRoot::Cluster, ActRoot::Cluster>; // In first position inliers, second outliers
    using PairsVector = std::vector<InliersOutliersPair>;

private:
    int fNiterRANSAC {};       //!< Iterations for RANSAC algorithm
    double fCylinderRadius {}; //!< Cylinder radius to define inliers
    double fMinChi2 {};        //!< Minimum value of cluster's chi2 to apply RANSAC to it
    int fSavedIterations {};   //!< Iterations with higher amount of inliers used to get best cluster (least chi2)

    std::shared_ptr<ActAlgorithm::Continuity> fContinuity {}; // ClIMB continuity algorithm object
public:
    Split() : VAction("Split") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    void SortClustersInliersAndOutliers(PairsVector& inliersAndOutliersVector);
    PairsVector GetInliersAndOutliers(ActRoot::Cluster* cluster);
    void GetBestFit(PairsVector& inliersAndOutliersVector);
    void ApplyContinuity(std::vector<ActRoot::Cluster>& toAdd, PairsVector& inliersAndOutliersVector);
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
