#ifndef ActRunner_h
#define ActRunner_h

#include "TF1.h"
#include "TH3.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <memory>
#include <string>
#include <utility>

// forward declarations
namespace ActRoot
{
class InputBlock;
}

namespace ActPhysics
{
class SRIM;
}

namespace ActSim
{
class Geometry;
}

namespace ActSim
{
//! A class holding utility functions to build up a simulation macro
class Runner
{
public:
    using XYZPoint = ROOT::Math::XYZPoint;
    using XYZVector = ROOT::Math::XYZVector;

private:
    TRandom* fRand {};
    ActPhysics::SRIM* fsrim {};
    ActSim::Geometry* fgeo {};
    std::unique_ptr<TF1> fSilResolution {};
    // Hold parameters of vertex sampling
    XYZPoint fBeamEntrance {};
    double fVertexSigmaY {};
    double fVertexSigmaZ {};

public:
    Runner(ActPhysics::SRIM* srim);
    Runner(ActPhysics::SRIM* srim, ActSim::Geometry* geo, TRandom* rand, double silSigma);

    [[deprecated("Use ActPhysics::SRIM::Slow() function instead")]]
    std::pair<double, double> EnergyAfterSilicons(double T3EnteringSil, double silWidth, double silThresh,
                                                  const std::string& silString, bool enableResolution = true,
                                                  bool enableStraggling = true);
    [[deprecated("Use ActPhysics::SRIM::Slow() function instead")]]
    double EnergyAfterGas(double TIni, double distance, const std::string& gasKey, bool enableStraggling = true);

    [[deprecated("Use ActPhysics::SRIM::EvalInitialEnergy() function instead")]]
    double EnergyBeforeGas(double Esil, double trackLengthInMM, const std::string& gasKey);

    [[deprecated("Must be manually implemented now")]]
    std::pair<XYZPoint, XYZPoint> SampleVertex(double lengthX, double meanY = -1, double sigmaY = -1, double meanZ = -1,
                                               double sigmaZ = -1, TH3F* histBeam = nullptr);
    [[deprecated("Do not use Geometry class")]]
    XYZPoint DisplacePointToTGeometryFrame(const XYZPoint& pointInMM);

    [[deprecated("Do not use Geometry class")]]
    XYZPoint DisplacePointToStandardFrame(const XYZPoint& pointGeo);

    double RandomizeBeamEnergy(double Tini, double sigma);

    [[deprecated("Use ActPhysics::SRIM::SlowWithStraggling")]]
    double ApplyStragglingInMaterialToRLeft(double RIni, double RLeft, const std::string& srimKey);

    // Function that rotates tracks
    XYZVector RotateToWorldFrame(const XYZVector& vBeamFrame, const XYZVector& beamDir) const;

    // New versions of functions
    [[deprecated("Do not use Geometry class")]]
    XYZPoint GetRandomVertex(double lengthX);

    // getters for pointers
    [[deprecated("Do not use this method")]]
    ActPhysics::SRIM* GetSRIM() const { return fsrim; }
    [[deprecated("Do not use this method")]]
    ActSim::Geometry* GetGeo() const { return fgeo; }
    [[deprecated("Do not use this method")]]
    TRandom* GetRand() const { return fRand; }

    // Getters of simulation parameters
    [[deprecated("Do not use this class")]]
    XYZPoint GetEntrance() const { return fBeamEntrance; }

    // Setters of input parameters of a simulation
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
    [[deprecated("Do not use this class")]]
    void ReadConfiguration(const std::string& file);
};

} // namespace ActSim

#endif
