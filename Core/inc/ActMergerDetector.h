#ifndef ActMergerDetector_h
#define ActMergerDetector_h

#include "ActCluster.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActMultiStep.h"
#include "ActSilData.h"
#include "ActSilSpecs.h"
#include "ActTPCData.h"
#include "ActVData.h"
#include "ActVDetector.h"
#include "ActVParameters.h"

#include "TStopwatch.h"
#include "TTree.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace ActRoot
{
// forward declarations
class TPCParameters;
class SilParameters;
class ModularParameters;

class MergerParameters : public VParameters
{
public:
    // Just flags setting event-by-event merger settings
    bool fUseRP {};
    bool fIsL1 {};
    bool fIsCal {};

    void Print() const override;
};

class MergerDetector : public VDetector
{
public:
    using XYZPoint = ROOT::Math::XYZPointF;
    using XYZVector = ROOT::Math::XYZVectorF;

private:
    // TPC
    TPCParameters* fTPCPars {};
    TPCData* fTPCData {};
    // Silicons
    SilParameters* fSilPars {};
    SilData* fSilData {};
    std::shared_ptr<ActPhysics::SilSpecs> fSilSpecs {};
    // Modular detector
    ModularParameters* fModularPars {};
    ModularData* fModularData {};

    // Merger
    MergerParameters fPars {};
    MergerData* fMergerData {};

    // Flags to delete news in destructor
    bool fDelTPCSilMod {};
    bool fDelMerger {};

    // Filter = corrector
    std::shared_ptr<ActAlgorithm::VFilter> fFilter {};

    // Is verbose?
    bool fIsVerbose {};

    ///// Parameters of the detector
    // Is enabled?
    bool fIsEnabled {};
    // Enable or not GATCONF validation
    bool fForceGATCONF {};
    // GATCONF cuts if enabled
    std::map<int, std::vector<std::string>> fGatMap {};
    // Event multiplicity and beam-likeness
    bool fForceRP {};
    bool fForceBeamLike {};
    std::vector<int> fNotBMults {};
    // Drift conversion
    bool fEnableConversion {};
    double fDriftFactor {};
    // Matching of silicon placement
    bool fEnableMatch {};
    bool fMatchUseZ {};
    double fZOffset {};
    // Enable computation of QProfile
    bool fEnableQProfile {};
    bool f2DProfile {};

    // Store pointers to beam, light and heavy
    ActRoot::Cluster* fBeamPtr;
    ActRoot::Cluster* fLightPtr;
    ActRoot::Cluster* fHeavyPtr;

    // Time counting
    std::vector<TStopwatch> fClocks {};
    std::vector<std::string> fClockLabels {};

public:
    MergerDetector(); //!< Default constructor that sets verbose mode according to ActRoot::Options
    ~MergerDetector() override;

    // Setters of pointer to Parameters from Detector Manager
    void SetParameters(VParameters* pars);
    void SetTPCParameters(TPCParameters* tpcPars) { fTPCPars = tpcPars; }
    void SetSilParameters(SilParameters* silPars) { fSilPars = silPars; }
    void SetModularParameters(ModularParameters* modPars) { fModularPars = modPars; }

    // Enable verbose mode
    void SetIsVerbose() { fIsVerbose = true; }

    void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
    void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
    void Reconfigure() override;

    // Init inputs
    void InitInputData(std::shared_ptr<TTree> tree) override;
    void InitInputFilter(std::shared_ptr<TTree> tree) override;

    // Init outputs
    void InitOutputData(std::shared_ptr<TTree> tree) override;
    void InitOutputFilter(std::shared_ptr<TTree> tree) override;

    // Builders
    void BuildEventData(int run = -1, int entry = -1) override;
    void BuildEventFilter() override;

    // Cleaners
    void ClearEventData() override;
    void ClearEventFilter() override;

    // Setters and getters of data
    inline void SetInputData(VData* data) override
    {
        if(auto casted {data->CastAs<TPCData>()}; casted)
            fTPCData = casted;
        else if(auto casted {data->CastAs<SilData>()}; casted)
            fSilData = casted;
        else if(auto casted {data->CastAs<ModularData>()}; casted)
            fModularData = casted;
        else
            throw std::invalid_argument("MergerDetector::SetInputData(): could not cast to any input data type!");
    }
    VData* GetInputData() const override { return nullptr; }
    template <typename T>
    T* GetInputData() const
    {
        if constexpr(std::is_same_v<T, TPCData>)
            return dynamic_cast<T*>(fTPCData);
        else if constexpr(std::is_same_v<T, SilData>)
            return dynamic_cast<T*>(fSilData);
        else if constexpr(std::is_same_v<T, ModularData>)
            return dynamic_cast<T*>(fModularData);
    }
    void SetOutputData(VData* data) override { fMergerData = data->CastAs<MergerData>(); }
    MergerData* GetOutputData() const override { return fMergerData; }

    void SetInputFilter(VData* data) override {}
    VData* GetInputFilter() const override { return nullptr; }
    void SetOutputFilter(VData* data) override {}
    VData* GetOutputFilter() const override { return nullptr; }

    // Getters of parameters
    VParameters* GetParameters() override { return nullptr; }

    // Printer of parameters
    void Print() const override;
    // Printer of reports
    void PrintReports() const override;

private:
    void InitCorrector();
    void InitClocks();
    void ReadSilSpecs(const std::string& file);
    void DoMerge();
    // Inner functions of Merger detector
    bool IsDoable();
    void ConvertToPhysicalUnits();
    bool GateGATCONFandTrackMult();
    bool GateSilMult();
    void LightOrHeavy();
    void ComputeOtherPoints();
    bool ComputeSiliconPoint();
    double TrackLengthFromLightIt(bool scale);
    void CorrectZOffset();
    bool MatchSPtoRealPlacement();
    void ComputeAngles();
    void ComputeQave();
    void ComputeQProfile();
    void ComputeBSP();
    void Reset(const int& run, const int& entry);
    //// Even inner functions
    void MoveZ(XYZPoint& p);
    double GetTheta3D(const XYZVector& beam, const XYZVector& other);
    double GetPhi3D(const XYZVector& beam, const XYZVector& other);
    XYZVector RotateTrack(XYZVector beam, XYZVector track);
    void ScalePoint(XYZPoint& point, float xy, float z, bool addOffset = false);
    template <typename T>
    inline bool IsInVector(T val, const std::vector<T>& vec)
    {
        return std::find(vec.begin(), vec.end(), val) != vec.end();
    }
    double GetRangeFromProfile(TH1F* h);
};
} // namespace ActRoot

#endif // !ActMergerDetector_h
