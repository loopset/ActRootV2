#ifndef ActSilSpecs
#define ActSilSpecs

#include "ActInputParser.h"
#include "ActSilMatrix.h"

#include "Math/GenVector/Cartesian3D.h"
#include "Math/GenVector/DisplacementVector3D.h"
#include "Math/GenVector/PositionVector3D.h"
#include "Math/Point3D.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <memory.h>

#include <map>
#include <memory>
#include <string>

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
    template <typename T>
    using Point = ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<T>>;
    template <typename T>
    using Vector = ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<T>>;
    using XYZPointF = Point<float>;
    using XYZVectorF = Vector<float>;

private:
    std::map<int, std::pair<double, double>> fPlacements;
    std::map<int, double> fThresholds;
    SilUnit fUnit;                         //!< Specifications of unit silicon
    XYZPointF fPoint;                      //!< Point of layer: basically, contains offset
    XYZVectorF fNormal;                    //!< Normal vector of silicon plane
    std::shared_ptr<SilMatrix> fMatrix {}; //!< Pointer to SiliconMatrix
    SilSide fSide;                         //!< Enum to spec side of layer with respect to ACTAR's frame

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
    const XYZPointF& GetPoint() const { return fPoint; }
    const XYZVectorF& GetNormal() const { return fNormal; }
    std::shared_ptr<SilMatrix> GetSilMatrix() const { return fMatrix; }

    // Operations
    template <typename T>
    std::pair<Point<T>, bool>
    GetSiliconPointOfTrack(const Point<T>& point, const Vector<T>& vector, bool scale = false) const;
    template <typename T>
    Point<T> GetBoundaryPointOfTrack(int padx, int pady, const Point<T>& point, const Vector<T>& vector) const;
    template <typename T>
    bool MatchesRealPlacement(int i, const Point<T>& sp, bool useZ = true) const;
    template <typename T>
    int GetIndexOfMatch(const Point<T>& p) const;

private:
    std::shared_ptr<SilMatrix> BuildSilMatrix() const;
};

class SilSpecs
{
public:
    typedef ROOT::Math::XYZPoint XYZPoint;
    typedef ROOT::Math::XYZVector XYZVector;
    typedef std::tuple<std::string, int, XYZPoint> SearchRet;

private:
    std::unordered_map<std::string, SilLayer> fLayers;

public:
    void ReadFile(const std::string& file);
    void Print() const;

    bool CheckLayersExists(const std::string& name) const { return fLayers.count(name); }
    const SilLayer& GetLayer(const std::string& name) { return fLayers[name]; };
    SearchRet FindLayerAndIdx(const XYZPoint& p, const XYZVector& v, bool verbose = false);
    void EraseLayer(const std::string& name);
};
} // namespace ActPhysics

#endif // !ActSilSpecs
