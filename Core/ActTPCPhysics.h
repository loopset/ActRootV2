#ifndef ActTPCPhysics_h
#define ActTPCPhysics_h

#include "ActCluster.h"
#include "ActVData.h"

#include "Math/Point3D.h"

#include <utility>
#include <vector>
namespace ActRoot
{
    //! Holder of physics info from an ACTAR TPC event
    class TPCPhysics : public VData
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;

        std::vector<ActCluster::Cluster> fClusters;
        std::vector<XYZPoint> fRPs;

    public:
        void Clear() override;
        void Print() const override;
    };
} // namespace ActRoot

#endif
