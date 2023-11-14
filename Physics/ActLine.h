#ifndef ActLine_h
#define ActLine_h

#include "ActTPCData.h"

#include "TPolyLine.h"
#include "TString.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <memory>
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
        void ScaleZ(float scale);
        void AlignUsingPoint(const XYZPoint& rp);
        double DistanceLineToPoint(const XYZPoint& point) const;
        XYZPoint ProjectionPointOnLine(const XYZPoint& point) const;
        void FitVoxels(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted = true, double qThreshold = -1,
                       bool correctOffset = true);
        void FitCloud(const std::vector<XYZPoint>& cloud, bool correctOffset = true);
        std::shared_ptr<TPolyLine>
        GetPolyLine(TString proj = "xy", int maxX = 128, int maxY = 128, int maxZ = 512, int rebinZ = 4) const;

        // Display parameters of line
        void Print() const;

    private:
        void FitCloudWithThreshold(const std::vector<XYZPoint>& points, const std::vector<double>& charge,
                                   bool correctOffset);
        inline bool IsInRange(double val, double min, double max) const { return (min <= val) && (val <= max); }
        std::shared_ptr<TPolyLine> TreatSaturationLine(TString proj, int maxZ, int rebinZ) const;
    };
} // namespace ActPhysics


#endif
