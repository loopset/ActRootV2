#ifndef ActMultiStep_h
#define ActMultiStep_h

#include "ActCluster.h"
#include <string>
#include <vector>

namespace ActCluster
{
class MultiStep
{
private:
    std::vector<ActCluster::Cluster>* fClusters {};

public:
    MultiStep() = default;
    MultiStep(std::vector<ActCluster::Cluster>* clusters);

    // Read config file
    void ReadConfigurationFile(const std::string& infile = "");
};
} // namespace ActCluster

#endif // !ActMultiStep_h
