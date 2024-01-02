#ifndef ActMergerData_h
#define ActMergerData_h

#include "ActVData.h"

#include "TH1.h"

#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <string>
#include <vector>

namespace ActRoot
{
    class MergerData : public VData
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    public:
        TH1F fQProf {};
        XYZPoint fRP {-1, -1, -1};
        XYZPoint fSP {-1, -1, -1};
        XYZPoint fBP {-1, -1, -1};
        std::vector<std::string> fSilLayers {};
        std::vector<float> fSilEs {};
        std::vector<float> fSilNs {};
        float fTrackLength {-1};
        float fThetaBeam {-1};
        float fThetaLight {-1};
        float fThetaDebug {-1};
        float fPhiLight {-1};
        float fQave {-1};
        int fEntry {-1};
        int fRun {-1};

        void Clear() override;
        void Print() const override;
    };
} // namespace ActRoot

#endif // !ActMergerData_h
