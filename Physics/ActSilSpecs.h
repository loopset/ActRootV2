#ifndef ActSilSpecs
#define ActSilSpecs

#include "ActInputParser.h"
#include "ActTPCDetector.h"

#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <memory.h>

#include <map>

namespace ActPhysics
{
    enum class SilSide
    {
        ELeft,
        ERight,
        EFront,
        EBack
    };

    //! A class representing a unit of silicon detector: geometry specs
    class SilUnit
    {
    private:
        double fWidth {};
        double fHeight {};
        double fThick {};

    public:
        SilUnit() = default;
        SilUnit(double height, double width, double thick) : fHeight(height), fWidth(width), fThick(thick) {}
        void Print() const;
        double GetWidth() const { return fWidth; }
        double GetHeight() const { return fHeight; };
        double GetThickness() const { return fThick; }
    };

    class SilLayer
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        std::map<int, std::pair<double, double>> fPlacements;
        std::map<int, double> fThresholds;
        SilUnit fUnit;     //!< Specifications of unit silicon
        XYZPoint fPoint;   //!< Point of layer: basically, contains offset
        XYZVector fNormal; //!< Normal vector of silicon plane
        SilSide fSide;     //!< Enum to spec side of layer with respect to ACTAR's frame

    public:
        SilLayer() = default;
        void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
        void Print() const;

        // Getters and setters
        const std::map<int, std::pair<double, double>>& GetPlacements() const { return fPlacements; }
        const std::map<int, double>& GetThresholds() const { return fThresholds; }
        template <typename T>
        bool ApplyThreshold(int idx, T val) const
        {
            return fThresholds.at(idx) <= val;
        }
        const SilUnit& GetUnit() const { return fUnit; }
        const XYZPoint& GetPoint() const { return fPoint; }
        const XYZVector& GetNormal() const { return fNormal; }

        // Other functions of interest
        std::pair<XYZPoint, bool> GetSiliconPointOfTrack(const XYZPoint& point, const XYZVector& vector) const;
        XYZPoint
        GetBoundaryPointOfTrack(ActRoot::TPCParameters* fTPC, const XYZPoint& point, const XYZVector& vector) const;
        bool MatchesRealPlacement(int i, const XYZPoint& sp, bool useZ = true) const;
    };

    class SilSpecs
    {
    private:
        std::unordered_map<std::string, SilLayer> fLayers;

    public:
        void ReadFile(const std::string& file);
        void Print() const;
        const SilLayer& GetLayer(const std::string& name) { return fLayers[name]; };
    };
} // namespace ActPhysics

#endif // !ActSilSpecs
