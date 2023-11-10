#ifndef ActMergerData_h
#define ActMergerData_h

#include "ActVData.h"

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
        XYZPoint fRP {-1, -1, -1};
        XYZPoint fSP {-1, -1, -1};
        std::vector<std::string> fSilLayers {};
        std::vector<float> fSilEs {};
        std::vector<float> fSilNs {};
       
        void Clear() override;
        void Print() const override;
    };
} // namespace ActRoot

#endif // !ActMergerData_h
