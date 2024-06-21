#ifndef ActRunner_h
#define ActRunner_h

#include "TF1.h"
#include "TH3F.h"
#include "TRandom3.h"

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
    TRandom3* fRand {};
    ActPhysics::SRIM* fsrim {};
    ActSim::Geometry* fgeo {};
    std::unique_ptr<TF1> fSilResolution {};
    // Hold parameters of vertex sampling
    XYZPoint fBeamEntrance {};
    double fVertexSigmaY {};
    double fVertexSigmaZ {};

public:
    Runner(ActPhysics::SRIM* srim);
    Runner(ActPhysics::SRIM* srim, ActSim::Geometry* geo, TRandom3* rand, double silSigma);

    std::pair<double, double> EnergyAfterSilicons(double T3EnteringSil, double silWidth, double silThresh,
                                                  const std::string& silString, bool enableResolution = true,
                                                  bool enableStraggling = true);
    double EnergyAfterGas(double TIni, double distance, const std::string& gasKey, bool enableStraggling = true);

    double EnergyBeforeGas(double Esil, double trackLengthInMM, const std::string& gasKey);

    XYZPoint SampleVertex(double meanY = -1, double sigmaY = -1, double meanZ = -1, double sigmaZ = -1,
                          TH3F* histBeam = nullptr);
    XYZPoint DisplacePointToTGeometryFrame(const XYZPoint& pointInMM);

    XYZPoint DisplacePointToStandardFrame(const XYZPoint& pointGeo);

    double RandomizeBeamEnergy(double Tini, double sigma);

    double ApplyStragglingInMaterialToRLeft(double RIni, double RLeft, const std::string& srimKey);

    // New versions of functions
    XYZPoint GetRandomVertex(double lengthX);

    // getters for pointers
    ActPhysics::SRIM* GetSRIM() const { return fsrim; }
    ActSim::Geometry* GetGeo() const { return fgeo; }
    TRandom3* GetRand() const { return fRand; }

    // Getters of simulation parameters
    XYZPoint GetEntrance() const { return fBeamEntrance; }

    // Setters of input parameters of a simulation
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
    void ReadConfiguration(const std::string& file);
};

} // namespace ActSim

#endif
