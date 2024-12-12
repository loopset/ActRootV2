#ifndef ActVoxel_h
#define ActVoxel_h

#include "Math/GenVector/Cartesian3D.h"
#include "Math/GenVector/CoordinateSystemTags.h"
#include "Math/GenVector/PositionVector3D.h"
#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"

#include <stdint.h>

#include <cstdint>
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
    std::vector<uint8_t> fZs {};
    bool fIsSaturated {false};

public:
    Voxel() = default;
    Voxel(const XYZPoint& pos, float charge, bool hasSaturation = false);
    // Overload equality operators
    friend bool operator==(const Voxel& v1, const Voxel& v2)
    {
        auto p1 {v1.GetPositionAs<int>()};
        auto p2 {v2.GetPositionAs<int>()};
        return p1 == p2;
    }
    friend bool operator!=(const Voxel& v1, const Voxel& v2) { return !(operator==(v1, v2)); }
    // Overload comparison operators
    // We dont do a cast to int here since it will be time-consuming
    // floats are guaranteed to be eq. safe
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
    void AddZ(float z) { fZs.push_back((uint8_t)z); }

    // Getters
    const XYZPoint& GetPosition() const { return fPosition; }
    template <typename T>
    ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<T>, ROOT::Math::DefaultCoordinateSystemTag>
    GetPositionAs() const
    {
        return {static_cast<T>(fPosition.X()), static_cast<T>(fPosition.Y()), static_cast<T>(fPosition.Z())};
    }
    float GetCharge() const { return fCharge; }
    // int GetID() const { return fID; }
    bool GetIsSaturated() const { return fIsSaturated; }
    const std::vector<uint8_t>& GetZs() const { return fZs; }
    std::vector<Voxel> GetBinVoxels(int rebin) const;

    // Print
    void Print() const;
};
} // namespace ActRoot
#endif // !ActVoxel_h
