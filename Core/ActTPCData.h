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
        //int fID {-1};
        bool fIsSaturated {false};
    public:
        Voxel() = default;
        Voxel(const XYZPoint& pos, float charge, bool hasSaturation = false);
        //Voxel(int id, const XYZPoint& pos, float charge, bool hasSaturation = false);
        //Setters
        void SetPosition(const XYZPoint& pos){ fPosition = pos; }
        void SetCharge(float charge){ fCharge = charge; }
        //void SetID(int id){ fID = id; }
        void SetIsSaturated(bool sat){ fIsSaturated = sat; }
        //Getters
        const XYZPoint& GetPosition() const { return fPosition; }
        float GetCharge() const { return fCharge; }
        //int GetID() const { return fID; }
        bool GetIsSaturated() const { return fIsSaturated; }
        //Print
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
}

#endif
