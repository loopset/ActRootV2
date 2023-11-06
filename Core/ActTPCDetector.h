#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "ActClIMB.h"
#include "ActInputData.h"
#include "ActMultiStep.h"
#include "ActRANSAC.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActTPCPhysics.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TStopwatch.h"
#include "TTree.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace ActRoot
{
    class InputBlock; // forward declaration

    class TPCParameters
    {
    private:
        double fPadSide {2}; // mm
        int fNPADSX;
        int fNPADSY;
        int fNPADSZ;
        int fREBINZ {1};
        int fNB_COBO {18};
        int fNB_ASAD {4};
        int fNB_AGET {4};
        int fNB_CHANNEL {68};

    public:
        TPCParameters() = default;
        TPCParameters(const std::string& type);
        // Setters
        void SetREBINZ(int rebin) { fREBINZ = rebin; }
        // Getters
        double GetPadSide() const { return fPadSide; }
        int GetNPADSX() const { return fNPADSX; }
        int GetNPADSY() const { return fNPADSY; }
        int GetNPADSZ() const { return fNPADSZ; }
        int GetREBINZ() const { return fREBINZ; }
        int GetNBCOBO() const { return fNB_COBO; }
        int GetNBASAD() const { return fNB_ASAD; }
        int GetNBAGET() const { return fNB_AGET; }
        int GetNBCHANNEL() const { return fNB_CHANNEL; }
    };

    class TPCDetector : public ActRoot::VDetector
    {
    private:
        // Parameters of detector
        TPCParameters fPars;
        // Data
        TPCData* fData {};
        // Preanalysis when reading raw data
        bool fCleanSaturatedMEvent {false};
        bool fCleanSaturatedVoxels {false};
        double fMinTBtoDelete {20};
        double fMinQtoDelete {2000};
        std::map<std::pair<int, int>, std::pair<std::vector<unsigned int>, double>> fPadMatrix;
        bool fCleanDuplicatedVoxels {false};
        // Physics data
        TPCPhysics* fPhysics {};
        // Timer for cluster (only cluster) step
        TStopwatch fClusterClock;
        // Have a commom ransac
        std::shared_ptr<ActCluster::RANSAC> fRansac {};
        // Have a common ClIMB
        std::shared_ptr<ActCluster::ClIMB> fClimb {};
        // Have common filters
        std::shared_ptr<ActCluster::MultiStep> fMultiStep {};

    public:
        TPCDetector() = default;
        virtual ~TPCDetector() = default;

        // Getters
        const TPCParameters& GetTPCPars() const { return fPars; }

        void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        void InitInputData(std::shared_ptr<TTree> tree) override;
        void InitOutputData(std::shared_ptr<TTree> tree) override;
        void InitOutputPhysics(std::shared_ptr<TTree> tree) override;
        void BuildEventData() override;
        void BuildEventPhysics() override;
        void ClearEventData() override;
        void ClearEventPhysics() override;

        // Base class getters
        TPCData* GetEventData() const override { return fData; }
        TPCPhysics* GetEventPhysics() const override { return fPhysics; }

        // And setters
        void SetEventData(VData* vdata) override;

        // Printer of parameters
        void PrintReports() const override;

        ////////////////////////////////
        // ActTPCData* GetDataPointer() { return fData; }

    private:
        void ReadHits(ReducedData& coas, const int& where, int& hitID);
        void CleanPadMatrix();
        void InitClusterMethod(const std::string& method);
        void EnsureUniquenessOfVoxels();
    };
} // namespace ActRoot

#endif
