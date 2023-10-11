#ifndef ActLine_h
#define ActLine_h

#include "ActTPCData.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "TPolyLine.h"
#include "TString.h"

#include <memory>
#include <vector>

namespace ActPhysics
{
    class Line
    {
    public:
        using XYZPoint  = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        XYZPoint fPoint {-1, -1, -1};
        XYZVector fDirection {-1, -1, -1};
        float fChi2 {-1};

    public:
        Line() = default;
        Line(XYZPoint point, XYZVector direction, float chi);
        Line(const XYZPoint& p1, const XYZPoint& p2);
        Line(const Line& ) = default;
        Line(Line&& ) = default;
        Line &operator=(const Line& ) = default;
        Line &operator=(Line&& ) = default;
        ~Line() = default;

        //Getters and setters
        XYZPoint GetPoint() const {return fPoint;}
        XYZVector GetDirection() const {return fDirection;}
        float GetChi2() const {return fChi2;}
        
        void SetPoint(const XYZPoint& point){fPoint = point;}
        void SetDirection(const XYZPoint& p1, const XYZPoint& p2){fDirection = p1 - p2;}
        void SetChi2(float chi2){fChi2 = chi2;}

        //Utility funtions
        double DistanceLineToPoint(const XYZPoint& point) const;
        void FitVoxels(const std::vector<ActRoot::Voxel>& voxels, double qThreshold = -1);
        void FitCloud(const std::vector<XYZPoint>& cloud);
        std::shared_ptr<TPolyLine> GetPolyLine(TString proj = "xy",
                                               int maxX = 128, int maxY = 128, int maxZ = 512) const;
        
    private:
        void FitCloudWithThreshold(const std::vector<XYZPoint>& points, const std::vector<double>& charge);
        inline bool IsInRange(double val, double min, double max) const
        {
            return (min <= val) && (val <= max);
        }
    };
}


#endif
