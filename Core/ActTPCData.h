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
        using XYZPoint = ROOT::Math::XYZPointF;

    private:
        XYZPoint fPosition {-1, -1, -1};
        float fCharge {-1};
        // int fID {-1};
        bool fIsSaturated {false};

    public:
        Voxel() = default;
        Voxel(const XYZPoint& pos, float charge, bool hasSaturation = false);
        // Overload comparison operators
        friend bool operator<(const Voxel& v1, const Voxel& v2)
        {
            const auto& p1 {v1.GetPosition()};
            const auto& p2 {v2.GetPosition()};
            if(p1.X() != p2.X())
                return p1.X() < p2.X();
            if(p1.Y() != p2.Y())
                return p1.Y() < p2.Y();
            return p1.Z() < p2.Z();
        }
        friend bool operator>(const Voxel& v1, const Voxel& v2) { return operator<(v2, v1); }
        friend bool operator<=(const Voxel& v1, const Voxel& v2) { return !(operator>(v1, v2)); }
        friend bool operator>=(const Voxel& v1, const Voxel& v2) { return !(operator<(v1, v2)); }

        // Setters
        void SetPosition(const XYZPoint& pos) { fPosition = pos; }
        void SetCharge(float charge) { fCharge = charge; }
        // void SetID(int id){ fID = id; }
        void SetIsSaturated(bool sat) { fIsSaturated = sat; }
        // Getters
        const XYZPoint& GetPosition() const { return fPosition; }
        float GetCharge() const { return fCharge; }
        // int GetID() const { return fID; }
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
