#ifndef ActMergerDetector_h
#define ActMergerDetector_h

#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActSilData.h"
#include "ActSilSpecs.h"
#include "ActTPCPhysics.h"
#include "ActVData.h"

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

    class MergerDetector
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        using XYZVector = ROOT::Math::XYZVectorF;

    private:
        TPCParameters* fTPCPars {};
        TPCPhysics* fTPCPhyiscs {};

        SilParameters* fSilPars {};
        SilData* fSilData {};
        std::shared_ptr<ActPhysics::SilSpecs> fSilSpecs {};

        ModularParameters* fModularPars {};
        ModularData* fModularData {};

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

        // Store iterators to beam, light and heavy
        decltype(TPCPhysics::fClusters)::iterator fBeamIt;
        decltype(TPCPhysics::fClusters)::iterator fLightIt;
        decltype(TPCPhysics::fClusters)::iterator fHeavyIt;

    public:
        // Setters of pointer to Parameters in DetMan
        void SetTPCParameters(TPCParameters* tpcPars) { fTPCPars = tpcPars; }
        void SetSilParameters(SilParameters* silPars) { fSilPars = silPars; }
        void SetModularParameters(ModularParameters* modPars) { fModularPars = modPars; }
        void SetEventData(VData* vdata);
        // Getters
        MergerData* GetMergerData() const { return fMergerData; }

        // Setter of entry and run number to be written to current MergerData
        void SetCurrentRunEntry(int run, int entry)
        {
            fCurrentRun = run;
            fCurrentEntry = entry;
        }
        // Read configurations
        void ReadConfiguration(std::shared_ptr<InputBlock> block);

        // Init INPUT data
        void InitInputMerger(std::shared_ptr<TTree> tree);

        // Init OUTPUT data
        void InitOutputMerger(std::shared_ptr<TTree> tree);

        // Do merge of all detector data
        void MergeEvent();

        // Mandatory clear
        void ClearOutputMerger();

        // Print settings
        void Print() const;

    private:
        void ReadSilSpecs(const std::string& file);
        bool IsDoable();
        void ConvertToPhysicalUnits();
        bool GateGATCONFandTrackMult();
        bool GateSilMult();
        bool GateOthers();
        void LightOrHeavy();
        void ComputeSiliconPoint();
        void ComputeAngles();
        void Reset();
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
