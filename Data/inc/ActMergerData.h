#ifndef ActMergerData_h
#define ActMergerData_h

#include "ActVData.h"

#include "Rtypes.h"

#include "TH1.h"

#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <ostream>
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
    TH1F fQprojX {};
    TH1F fQProf {};
    XYZPoint fWP {-1, -1, -1}; // window point aka entrance point
    XYZPoint fRP {-1, -1, -1};
    XYZPoint fSP {-1, -1, -1};
    XYZPoint fBP {-1, -1, -1};
    XYZPoint fBSP {-1, -1, -1};
    XYZPoint fBraggP {-1, -1, -1};
    std::vector<std::string> fSilLayers {};
    std::vector<float> fSilEs {};
    std::vector<float> fSilNs {};
    float fTrackLength {-1};
    float fThetaBeam {-1};
    float fThetaBeamZ {-1}; // emittance angle along Z
    float fPhiBeamY {-1};   // emittance angle along Y
    float fThetaLight {-1};
    float fThetaDebug {-1};
    float fThetaLegacy {-1}; // this is just fThetaLight but will not be corrected in CorrDetector
    float fThetaHeavy {-1};
    float fPhiLight {-1};
    float fQave {-1};
    int fBeamIdx {-1};
    int fLightIdx {-1};
    int fHeavyIdx {-1};
    int fEntry {-1};
    int fRun {-1};

    void Clear() override;
    void Print() const override;
    void Stream(std::ostream& streamer) const;

    ClassDefOverride(MergerData, 1);
};
} // namespace ActRoot

#endif // !ActMergerData_h
