#ifndef ActTPCData_h
#define ActTPCData_h

#include "ActCluster.h"
#include "ActVData.h"
#include "ActVoxel.h"

#include "Rtypes.h"

#include "Math/Point3D.h"

#include <vector>
namespace ActRoot
{
//! Holder of physics info from an ACTAR TPC event
class TPCData : public VData
{
public:
    using XYZPoint = ROOT::Math::XYZPointF;

    std::vector<Cluster> fClusters {};
    std::vector<Voxel> fRaw {};
    std::vector<XYZPoint> fRPs {};

public:
    void Clear() override;
    void ClearFilter() override;
    void Print() const override;

    ClassDefOverride(TPCData, 1);
};
} // namespace ActRoot

#endif
