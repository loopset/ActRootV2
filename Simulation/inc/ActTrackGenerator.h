#ifndef ActTrackGenerator_h
#define ActTrackGenerator_h

#include "ActInputParser.h"
#include "ActTPCLegacyData.h"
#include "ActVoxel.h"

#include "TH2.h"
#include "TRandom.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// forward declarations
namespace ActPhysics
{
class SRIM;
class Gas;
} // namespace ActPhysics

namespace ActRoot
{
class TPCParameters;
class CalibrationManager;
} // namespace ActRoot

namespace ActSim
{
class TrackGenerator
{
public:
    typedef ROOT::Math::XYZPoint XYZPoint;
    typedef ROOT::Math::XYZVector XYZVector;

private:
    // A pointer to SRIM class
    ActPhysics::SRIM* fsrim {};
    // A pointer to the TPC parameters
    ActRoot::TPCParameters* ftpc {};
    // A pointer to the gas properties
    ActPhysics::Gas* fGas {};
    // A pointer to a generator
    TRandom* fRand {gRandom};
    // A pointer to the Calibration
    ActRoot::CalibrationManager* fCalMan {};
    // Parameters to read
    XYZPoint fBeamEntrance {};
    double fBeamSigmaY {};
    double fBeamSigmaZ {};
    double fMaxRangeElectrons {2}; // mm

    // General parameters of simulator
    double fRangeStep {0.75}; // mm
    // Sampling frequency
    double fSamplingFreq {12.5};
    // Time start of trigger
    double fStartTime {};

    // Vectors of simulated points
    std::vector<std::vector<ActRoot::Voxel>> fCloud;
    // Vector to convert
    std::map<std::pair<int, int>, std::vector<std::pair<int, double>>> fData;
    // Vertex
    XYZPoint fVertex {-1, -1, -1};
    // TPCData in Thomas version
    MEventReduced fMEvent {};


public:
    TrackGenerator() = default;
    TrackGenerator(ActPhysics::SRIM* srim, ActRoot::TPCParameters* tpc, ActPhysics::Gas* gas);

    // Reader of configuration
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);
    void ReadConfiguration(const std::string& file);

    // Setters
    void SetTRandom(TRandom* rand) { fRand = rand; }
    void SetCalibrations(ActRoot::CalibrationManager* cals) { fCalMan = cals; }
    // Getters
    const XYZPoint& GetVertex() const { return fVertex; }

    // Generate a vertex
    void GenVertex();

    // Add a beam
    double AddBeam(double Tini);
    // Add a recoil track
    double AddRecoil(const std::string& which, double Tini, double theta, double phi);

    // Reset method
    void Reset();

    // Main method to convert to voxels
    void Convert();

    // Fill histograms
    void Fill2D(TH2* h, const std::string& which);

private:
    bool IsInChamber(const XYZPoint& p);
    double FillCloud(const std::string& which, double T, double l, const XYZPoint& point, const XYZVector& dir);
    template <typename T>
    std::pair<int, int> GetPadRange(const T& p, const std::string& which);
    double SpreadCharge(const XYZPoint& p, double x, double y);
    void FillMEvent();
};
} // namespace ActSim

#endif
