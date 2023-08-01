#ifndef ActGeometry_h
#define ActGeometry_h

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"
#include "RtypesCore.h"
#include "TCanvas.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TGeoVolume.h"
#include "TString.h"
#include "TView3D.h"
#include "TGeoNavigator.h"
#include "TObject.h"
#include <map>
#include <string>
#include <tuple>
#include <utility>

namespace ActSim {
    struct DriftChamber
    {
        double X {};
        double Y {};
        double Z {};

        DriftChamber() = default;
        inline DriftChamber(double x, double y, double z)
            : X(x), Y(y), Z(z)
        {}
        ~DriftChamber() = default;

        void Print() const;
    };

    struct SilUnit
    {
        unsigned int fIndex {};
        double fLengthX {};
        double fLengthY {};
        double fLengthZ {};
    
        SilUnit() = default;
        inline SilUnit(unsigned int type, double x, double y, double z)
            : fIndex(type), fLengthX(x), fLengthY(y), fLengthZ(z)
        {}
        ~SilUnit() = default;

        void Print() const;
    };

    struct SilAssembly
    {
        unsigned int fIndex {};
        SilUnit fUnit {};
        std::map<int, std::pair<double, double>> fPlacements {};
        std::pair<double, double> fOffset {-1, -1};
        bool fIsAlongX {}; bool fIsAlongY {};
        bool fHasXOffset {}; bool fHasYOffset {};
        bool fIsMirrored {};

        SilAssembly() = default;
        SilAssembly(unsigned int index, const SilUnit& uniy, bool alongx = false, bool alongy = false);
        ~SilAssembly() = default;
        void SetAssemblyPlacements(const std::map<int, std::pair<double, double>>& placements) {fPlacements = placements;}
        void SetOffsets(double xoffset = -1, double yoffset = -1);
        void SetMirror(bool m){ fIsMirrored = m; }

        void Print() const;
    };

    //! A simple geometry manager implemented in ROOT
    /*!
      This class includes all the basic funtionalities of Geant4 but in root
      and in a simplier manner, allowing to implement realistic-geometry simulations in a macro
     */
    class Geometry
    {
    public:
        using XYZPoint  = ROOT::Math::XYZPoint;
        using XYZVector = ROOT::Math::XYZVector;
    private:
        TGeoManager* fManager {nullptr}; //!< Manager of the geometry
        TGeoNavigator* fNavigator {nullptr}; //!< Navigator queries

        //shapes (stored in maps for silicons, since we have various types)
        TGeoVolume* fTopVol {nullptr}; //!< GeoVolume of world
        TGeoVolume* fDriftVol {nullptr};
        std::map<int, TGeoVolume*> fUnitSilVols {};
        std::map<unsigned int, TGeoVolume*> fAssemblies {};

        //materials and mediums (not relevant, just we need some kind of material)
        TGeoMaterial* fNoneMaterial {nullptr};
        TGeoMedium* fNoneMedium {nullptr};

        //sizes (remember that they are half lengths)
        DriftChamber fActar {}; //!< Drift cage parameters
        std::map<unsigned int, SilAssembly> fAssDataMap {}; //!< Silicon parameters

        //for plotting
        TCanvas* fCanvas {nullptr};

    public:
        Geometry();
        ~Geometry();

        //setters
        void SetDrift(const DriftChamber& dr){fActar = dr;}
        void AddAssemblyData(const SilAssembly& ass){fAssDataMap[ass.fIndex] = ass;}
        //getters
        const DriftChamber& GetDriftParameters() const {return fActar;}
        double GetAssemblyUnitWidth(unsigned int index); //!< Returns full width of silicon unit in mm

        void Construct();
        
        void Print() const;

        void Draw();
        
        void PropagateTrackToSiliconArray(const XYZPoint& initPoint,
                                          const XYZVector& direction,
                                          int assemblyIndex,
                                          bool& isMirror,
                                          double& distance,
                                          int& silType,
                                          int& silIndex,
                                          XYZPoint& newPoint,
                                          bool debug = false);

        void CheckIfStepIsInsideDriftChamber(const XYZPoint& point,
                                             const XYZVector& direction,
                                             double step,
                                             bool& isInside,
                                             XYZPoint& newPoint,
                                             bool debug = false);
    
        void FindBoundaryPoint(const XYZPoint& vertex,
                               const XYZVector& direction,
                               double& distance,
                               XYZPoint& bp,
                               bool debug = false);
    
        void ReadGeometry(std::string path, std::string fileName);

        void WriteGeometry(std::string path, std::string fileName);

    private:
        std::tuple<int, int> GetSilTypeAndIndexFromTString(const TString& path);
        int GetAssemblyIndexFromTString(const TString& path);
        bool GetAssemblyMirrorFromTString(const TString& path);
    };
}

#endif
