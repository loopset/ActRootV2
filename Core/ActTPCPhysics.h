#ifndef ActTPCPhysics_h
#define ActTPCPhysics_h

#include "ActCluster.h"
#include "ActVData.h"

#include <vector>
namespace ActRoot
{
    //! Holder of physics info from an ACTAR TPC event
    class TPCPhysics : public VData
    {
    public:
        std::vector<ActCluster::Cluster> fClusters;

    public:
        void Clear() override;
        void Print() const override;
    };
}

#endif
