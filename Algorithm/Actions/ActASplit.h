#ifndef ActASplit_h
#define ActASplit_h

#include "ActClIMB.h"
#include "ActCluster.h"
#include "ActVAction.h"


namespace ActAlgorithm
{
namespace Actions
{
class Split : public VAction
{
public:
    using InliersOutliersPair = std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Cluster>>;

private:
    int fNiterRANSAC {};       //!< Iterations for RANSAC algorithm
    double fCylinderRadius {}; //!< Cylinder radius to define inliers
    double fMinChi2 {};        //!< Minimum value of cluster's chi2 to apply RANSAC to it
    int fSavedIterations {};   //!< Iterations with higher amount of inliers used to get best cluster (least chi2)

    std::shared_ptr<ActAlgorithm::ClIMB> fClimb {}; // ClIMB continuity algorithm object
public:
    Split() : VAction("Split") { fClimb = std::make_shared<ActAlgorithm::ClIMB>(fTPCPars, 10); }

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    void SortClustersInliersAndOutliers(InliersOutliersPair& inliersAndOutliersVector);
    InliersOutliersPair GetInliersAndOutliers(ActRoot::Cluster* cluster);
    void GetBestFit(InliersOutliersPair& inliersAndOutliersVector);
    void ApplyContinuity(std::vector<ActRoot::Cluster>& toAdd, InliersOutliersPair& inliersAndOutliersVector);
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
