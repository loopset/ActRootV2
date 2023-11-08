#ifndef ActSilSpecs
#define ActSilSpecs

#include "ActInputParser.h"

#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <memory.h>

#include <map>

namespace ActPhysics
{
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
    };

    class SilLayer
    {
    public:
        using XYZPointD = ROOT::Math::XYZPointD;
        using XYZVectorD = ROOT::Math::XYZVectorD;

    private:
        std::map<int, std::pair<double, double>> fPlacements;
        std::map<int, double> fThresholds;
        SilUnit fUnit;
        XYZPointD fPoint;   //!< Point of layer: basically, contains offset
        XYZVectorD fNormal; //!< Normal vector of silicon plane

    public:
        SilLayer() = default;
        void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
        void Print() const;

        // Getters and setters
        const std::map<int, std::pair<double, double>>& GetPlacements() const { return fPlacements; }
        const std::map<int, double>& GetThresholds() const { return fThresholds; }
        const SilUnit& GetUnit() const { return fUnit; }
        const XYZPointD& GetPoint() const { return fPoint; }
        const XYZVectorD& GetNormal() const { return fNormal; }
    };

    class SilSpecs
    {
    private:
        std::unordered_map<std::string, SilLayer> fLayers;

    public:
        void ReadFile(const std::string& file);
        void Print() const;
        const SilLayer& GetLayer(const std::string& name) { return fLayers[name]; }
    };
} // namespace ActPhysics

#endif // !ActSilSpecs
