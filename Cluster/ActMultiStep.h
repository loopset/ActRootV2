#ifndef ActMultiStep_h
#define ActMultiStep_h

#include "ActCluster.h"
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

    };
}

#endif // !ActMultiStep_h
