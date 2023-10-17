#include "ActMultiStep.h"
#include "ActCluster.h"
#include <vector>

ActCluster::MultiStep::MultiStep(std::vector<ActCluster::Cluster>* clusters)
    : fClusters(clusters)
{}
