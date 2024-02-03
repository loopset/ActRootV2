#ifndef ActTPCData_h
#define ActTPCData_h

#include "ActCluster.h"
#include "ActVData.h"
#include "ActVoxel.h"

#include "Math/Point3D.h"

#include <vector>
namespace ActRoot
{
    //! Holder of physics info from an ACTAR TPC event
    class TPCData : public VData
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;

        std::vector<ActCluster::Cluster> fClusters {};
        std::vector<Voxel> fRaw {};
        std::vector<XYZPoint> fRPs {};

    public:
        void Clear() override;
        void Print() const override;
    };
} // namespace ActRoot

#endif
