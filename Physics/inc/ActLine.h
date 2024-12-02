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
    using XYZPointF = ROOT::Math::XYZPointF;
    using XYZVectorF = ROOT::Math::XYZVectorF;

private:
    XYZPointF fSigmas {-1, -1, -1};
    XYZPointF fPoint {-1, -1, -1};
    XYZVectorF fDirection {-1, -1, -1};
    float fChi2 {-1};

public:
    Line() = default;
    Line(XYZPointF point, XYZVectorF direction, float chi);
    Line(const XYZPointF& p1, const XYZPointF& p2);
    ~Line() = default;

    // Getters and setters
    XYZPointF GetPoint() const { return fPoint; }
    XYZPointF GetSigmas() const { return fSigmas; }
    XYZVectorF GetDirection() const { return fDirection; }
    float GetChi2() const { return fChi2; }

    void SetPoint(const XYZPointF& point) { fPoint = point; }
    void SetSigmas(const XYZPointF& sigmas) { fSigmas = sigmas; }
    void SetDirection(const XYZPointF& p1, const XYZPointF& p2) { fDirection = p1 - p2; }
    void SetDirection(const XYZVectorF& direction) { fDirection = direction; }
    void SetChi2(float chi2) { fChi2 = chi2; }

    // Utility funtions
    void Scale(float xy, float z);
    void AlignUsingPoint(const XYZPointF& rp, bool isRecoil = true);
    double DistanceLineToPoint(const XYZPointF& point) const;
    XYZPointF ProjectionPointOnLine(const XYZPointF& point) const;
    XYZPointF MoveToX(float x) const;
    void FitVoxels(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted = true, bool correctOffset = true);
    std::shared_ptr<TPolyLine> GetPolyLine(TString proj, int minX, int maxX, int maxY, int maxZ, int rebinZ) const;

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
