#ifndef ActCorrDetector_h
#define ActCorrDetector_h

#include "ActMergerData.h"
#include "ActPIDCorrector.h"

#include "TTree.h"

#include "Math/Point3D.h"

#include <memory>
#include <string>
namespace ActRoot
{
    class CorrDetector
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;

    private:
        MergerData* fIn {};
        MergerData* fOut {};

        // Parameters of detector
        ActPhysics::PIDCorrection* fPIDCorr {};
        double fZOffset {};
        bool fEnableThetaCorr {};

    public:
        void ReadConfiguration();

        void InitInputCorr(std::shared_ptr<TTree> tree);
        void InitOutputCorr(std::shared_ptr<TTree> tree);

        void BuildEventCorr();

        void Print() const;

    private:
        void ReadPIDFile(const std::string& file);
        void MoveZ(XYZPoint& p);
        void CorrectAngle();
    };
} // namespace ActRoot

#endif // !ActCorrDetector_h
