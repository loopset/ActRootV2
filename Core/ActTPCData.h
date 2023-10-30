#ifndef ActTPCData_h
#define ActTPCData_h
/*
ActTPCData transforms the raw data of the detector to a calibrated event,
ready to be clusterized
*/

#include "ActVData.h"

#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"

#include <vector>

namespace ActRoot
{
    class Voxel
    {
    public:
        using XYZPoint = ROOT::Math::XYZPoint;

    private:
        XYZPoint fPosition {-1, -1, -1};
        double fCharge {-1};
        bool fIsSaturated {false};

    public:
        Voxel() = default;
        Voxel(const XYZPoint& pos, double charge, bool hasSaturation = false);
        // Overload comparison operators
        friend bool operator<(const Voxel& v1, const Voxel& v2) { return v1.fPosition.X() < v2.fPosition.X(); }
        friend bool operator>(const Voxel& v1, const Voxel& v2) { return !(operator<(v2, v1)); }
        friend bool operator<=(const Voxel& v1, const Voxel& v2) { return !(operator>(v1, v2)); }
        friend bool operator>=(const Voxel& v1, const Voxel& v2) { return !(operator<(v1, v2)); }

        // Setters
        void SetPosition(const XYZPoint& pos) { fPosition = pos; }
        void SetCharge(double charge) { fCharge = charge; }
        void SetIsSaturated(bool sat) { fIsSaturated = sat; }

        // Getters
        const XYZPoint& GetPosition() const { return fPosition; }
        double GetCharge() const { return fCharge; }
        bool GetIsSaturated() const { return fIsSaturated; }

        // Print
        void Print() const;
    };

    class TPCData : public VData
    {
    public:
        std::vector<Voxel> fVoxels;

        TPCData() = default;

        void Clear() override;
        void Print() const override;
    };
} // namespace ActRoot

#endif
