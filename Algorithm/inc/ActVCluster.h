#ifndef ActVCluster_h
#define ActVCluster_h

#include "ActCluster.h"
#include "ActVoxel.h"

#include <utility>
#include <vector>

namespace ActAlgorithm
{
class VCluster
{
public:
    using ClusterRet = std::pair<std::vector<ActRoot::Cluster>, std::vector<ActRoot::Voxel>>;

protected:
    int fMinPoints {};

public:
    VCluster() = default;
    VCluster(int minPoints) : fMinPoints(minPoints) {}
    virtual ~VCluster() = default;

    void SetMinPoint(int npoints) { fMinPoints = npoints; }
    int GetMinPoints() const { return fMinPoints; }

    virtual void ReadConfiguration() = 0;
    virtual ClusterRet Run(const std::vector<ActRoot::Voxel>& voxels, bool addNoise = false) = 0;
    virtual void Print() const = 0;
    virtual void PrintReports() const = 0;
};
} // namespace ActCluster

#endif // !ActVCluster_h
