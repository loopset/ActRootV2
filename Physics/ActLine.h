#ifndef ActLine_h
#define ActLine_h

#include "ActVoxel.h"

#include "TPolyLine.h"
#include "TString.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <memory>
#include <string>
#include <vector>

namespace ActPhysics
{
    class Line
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        XYZPoint fSigmas {-1, -1, -1};
        XYZPoint fPoint {-1, -1, -1};
        XYZVector fDirection {-1, -1, -1};
        float fChi2 {-1};

    public:
        Line() = default;
        Line(XYZPoint point, XYZVector direction, float chi);
        Line(const XYZPoint& p1, const XYZPoint& p2);
        ~Line() = default;

        // Getters and setters
        XYZPoint GetPoint() const { return fPoint; }
        XYZPoint GetSigmas() const { return fSigmas; }
        XYZVector GetDirection() const { return fDirection; }
        float GetChi2() const { return fChi2; }

        void SetPoint(const XYZPoint& point) { fPoint = point; }
        void SetSigmas(const XYZPoint& sigmas) { fSigmas = sigmas; }
        void SetDirection(const XYZPoint& p1, const XYZPoint& p2) { fDirection = p1 - p2; }
        void SetDirection(const XYZVector& direction) { fDirection = direction; }
        void SetChi2(float chi2) { fChi2 = chi2; }

        // Utility funtions
        void Scale(float xy, float z);
        void AlignUsingPoint(const XYZPoint& rp, bool isRecoil = true);
        double DistanceLineToPoint(const XYZPoint& point) const;
        XYZPoint ProjectionPointOnLine(const XYZPoint& point) const;
        XYZPoint MoveToX(float x) const;
        void FitVoxels(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted = true, bool correctOffset = true);
        std::shared_ptr<TPolyLine>
        GetPolyLine(TString proj = "xy", int maxX = 128, int maxY = 128, int maxZ = 512, int rebinZ = 4) const;

        // Display parameters of line
        void Print() const;

    private:
        void DoFit(const std::vector<ActRoot::Voxel>& points, bool qWeighted, bool correctOffset);
        void Fit2Dfrom3D(double Mi, double Mj, double Sii, double Sjj, double Sij, double w,
                         const std::string& degenerated = "z");
        void Chi2Dfrom3D(const std::vector<ActRoot::Voxel>& voxels, bool correctOffset);
        inline bool IsInRange(double val, double min, double max) const { return (min <= val) && (val <= max); }
        std::shared_ptr<TPolyLine> TreatSaturationLine(TString proj, int maxZ, int rebinZ) const;
    };
} // namespace ActPhysics


#endif
