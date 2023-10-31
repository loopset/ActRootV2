#ifndef ActTPCData_h
#define ActTPCData_h
/*
ActTPCData transforms the raw data of the detector to a calibrated event,
ready to be clusterized
*/

#include "ActVData.h"

// #include "Math/GenVector/Cartesian3D.h"
// #include "Math/GenVector/CoordinateSystemTags.h"
// #include "Math/GenVector/PositionVector3D.h"
#include "Math/Point3D.h"

#include <vector>

namespace ActRoot
{
    class Voxel
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZPointI = ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<int>, ROOT::Math::DefaultCoordinateSystemTag>;

    private:
        XYZPointI fPosition {-1, -1, -1};
        float fCharge {-1};
        // int fID {-1};
        bool fIsSaturated {false};

    public:
        Voxel() = default;
        Voxel(const XYZPointI& pos, float charge, bool hasSaturation = false);
        // Overload comparison operators
        friend bool operator<(const Voxel& v1, const Voxel& v2) { return v1.fPosition.X() < v2.fPosition.X(); }
        friend bool operator>(const Voxel& v1, const Voxel& v2) { return !(operator<(v2, v1)); }
        friend bool operator<=(const Voxel& v1, const Voxel& v2) { return !(operator>(v1, v2)); }
        friend bool operator>=(const Voxel& v1, const Voxel& v2) { return !(operator<(v1, v2)); }

        // Voxel(const Voxel&) = default;
        // Voxel& operator=(const Voxel&) = default;
        // Voxel(Voxel&&) = default;
        // Voxel& operator=(Voxel&&) = default;
        // Voxel(int id, const XYZPoint& pos, float charge, bool hasSaturation = false);

        // Setters
        void SetPosition(const XYZPointI& pos) { fPosition = pos; }
        void SetCharge(float charge) { fCharge = charge; }
        // void SetID(int id){ fID = id; }
        void SetIsSaturated(bool sat) { fIsSaturated = sat; }

        // Getters
        const XYZPointI& GetPosition() const { return fPosition; }
        // Casting from int to any type
        template<typename T>
        ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<T>, ROOT::Math::DefaultCoordinateSystemTag> GetPositionAs() const 
        {
            return ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<T>, ROOT::Math::DefaultCoordinateSystemTag> {
                static_cast<T>(fPosition.X()),
                static_cast<T>(fPosition.Y()),
                static_cast<T>(fPosition.Z())
            };
        }
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
