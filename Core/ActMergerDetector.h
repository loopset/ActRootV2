#ifndef ActMergerDetector_h
#define ActMergerDetector_h

#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActSilData.h"
#include "ActSilSpecs.h"
#include "ActTPCData.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TTree.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ActRoot
{
    // forward declarations
    class TPCParameters;
    class SilParameters;
    class ModularParameters;

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

        // Merger data
        MergerData* fMergerData {};

        // Parameters of algorithm
        int fCurrentRun {};
        int fCurrentEntry {};
        // GATCONF cuts
        std::map<int, std::vector<std::string>> fGatMap {};
        // Event multiplicity and beam-likeness
        bool fForceBeamLike {};
        std::vector<int> fNotBMults {};
        // Gate on XVertex position!
        double fGateRPX {};
        // Drift conversion
        bool fEnableConversion {};
        double fDriftFactor {};
        // Matching of silicon placement
        bool fEnableMatch {};
        bool fMatchUseZ {};
        double fZOffset {};
        // Enable computation of QProfile
        bool fEnableQProfile {};

        // Store iterators to beam, light and heavy
        decltype(TPCData::fClusters)::iterator fBeamIt;
        decltype(TPCData::fClusters)::iterator fLightIt;
        decltype(TPCData::fClusters)::iterator fHeavyIt;

    public:
        // Setters of pointer to Parameters in DetMan
        void SetTPCParameters(TPCParameters* tpcPars) { fTPCPars = tpcPars; }
        void SetSilParameters(SilParameters* silPars) { fSilPars = silPars; }
        void SetModularParameters(ModularParameters* modPars) { fModularPars = modPars; }

        // Setter of entry and run number to be written to current MergerData
        void SetCurrentRunEntry(int run, int entry)
        {
            fCurrentRun = run;
            fCurrentEntry = entry;
        }
        void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        void Reconfigure() override;

        // Init inputs
        void InitInputRaw(std::shared_ptr<TTree> tree) override;
        void InitInputMerger(std::shared_ptr<TTree> tree) override;

        // Init outputs
        void InitOutputData(std::shared_ptr<TTree> tree) override;
        void InitOutputMerger(std::shared_ptr<TTree> tree) override;

        // Builders
        void BuildEventData() override;
        void BuildEventMerger() override;

        // Cleaners
        void ClearEventData() override;
        void ClearEventMerger() override;

        // Getters of data
        VData* GetEventData() const override { return nullptr; }
        VData* GetEventMerger() const override { return fMergerData; } // managed by MergerDetector

        // Setters of data
        void SetEventData(VData* vdata) override;

        // Printer of parameters
        void Print() const override;
        // Printer of reports
        void PrintReports() const override;

    private:
        void ReadSilSpecs(const std::string& file);
        bool IsDoable();
        void ConvertToPhysicalUnits();
        bool GateGATCONFandTrackMult();
        bool GateSilMult();
        bool GateOthers();
        void LightOrHeavy();
        void ComputeBoundaryPoint();
        void ComputeSiliconPoint();
        void CorrectZOffset();
        bool MatchSPtoRealPlacement();
        void ComputeAngles();
        void ComputeQave();
        void ComputeQProfile();
        void Reset();
        //// Even inner functions
        void MoveZ(XYZPoint& p);
        double GetTheta3D(const XYZVector& beam, const XYZVector& other);
        XYZVector RotateTrack(XYZVector beam, XYZVector track);
        void ScalePoint(XYZPoint& point, float xy, float z);
        template <typename T>
        inline bool IsInVector(T val, const std::vector<T>& vec)
        {
            return std::find(vec.begin(), vec.end(), val) != vec.end();
        }
    };
} // namespace ActRoot

#endif // !ActMergerDetector_h
