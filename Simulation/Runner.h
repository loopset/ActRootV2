#ifndef ActSimRunner_h
#define ActSimRunner_h

#include "Physics/SRIM.h"
#include "Geometry.h"
#include "Physics/Kinematics.h"
#include "KinematicGenerator.h"

#include "TF1.h"
#include "TRandom3.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "TH3F.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ActSim
{
    //!A class holding utility functions to build up a simulation macro
    class Runner
    {
    public:
        using XYZPoint = ROOT::Math::XYZPoint;
        using XYZVector = ROOT::Math::XYZVector;
    private:
        TRandom3* fRand {};
        ActPhysics::SRIM* fsrim {};
        Geometry* fgeo {};
        std::unique_ptr<TF1> fSilResolution {};

    public:
        Runner(ActPhysics::SRIM* srim,
               Geometry* geo,
               TRandom3* rand,
               double silSigma);

        std::pair<double, double> EnergyAfterSilicons(double T3EnteringSil,
                                                      double silWidth,
                                                      double silThresh,
                                                      const std::string& silString,
                                                      bool enableResolution = true,
                                                      bool enableStraggling = true);
        double EnergyAfterGas(double TIni,
                              double distance,
                              const std::string& gasKey,
                              bool enableStraggling = true);

        double EnergyBeforeGas(double Esil, double trackLengthInMM,
                               const std::string& gasKey);

        XYZPoint SampleVertex(double meanY = -1, double sigmaY = -1,
                              double meanZ = -1, double sigmaZ = -1,
                              TH3F* histBeam = nullptr);
        XYZPoint DisplacePointToTGeometryFrame(const XYZPoint& pointInMM);

        XYZPoint DisplacePointToStandardFrame(const XYZPoint& pointGeo);

        double RandomizeBeamEnergy(double Tini, double sigma);

        double ApplyStragglingInMaterialToRLeft(double RIni, double RLeft,
                                                const std::string& srimKey);
    
        //getters for pointers
        ActPhysics::SRIM* GetSRIM() const { return fsrim; }
        Geometry* GetGeo() const { return fgeo; }
        TRandom3* GetRand() const { return fRand; }
    };

}

#endif
