#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "ActClIMB.h"
#include "ActInputData.h"
#include "ActRANSAC.h"
#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
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
        // Auxiliars to read data
        MEventReduced* fMEvent {};
        std::vector<ActRoot::Voxel>* fVoxels {};
        // Data itself
        TPCData* fData {};
        // Preanalysis when reading raw data
        bool fCleanSaturatedMEvent {false};
        bool fCleanSaturatedVoxels {false};
        double fMinTBtoDelete {20};
        double fMinQtoDelete {2000};
        std::map<std::pair<int, int>, std::pair<std::vector<unsigned int>, double>> fPadMatrix;
        bool fCleanDuplicatedVoxels {false};
        // Timer for cluster (only cluster) step
        TStopwatch fClusterClock;
        // Have a commom ransac
        std::shared_ptr<ActCluster::RANSAC> fRansac {};
        // Have a common ClIMB
        std::shared_ptr<ActCluster::ClIMB> fClimb {};

    public:
        TPCDetector() = default;
        virtual ~TPCDetector() = default;

        // Getters
        const TPCParameters& GetTPCPars() const { return fPars; }
        TPCParameters* GetParametersPointer() { return &fPars; }

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
        TPCData* GetEventData() const override { return fData; }
        VData* GetEventMerger() const override { return nullptr; } // managed by MergerDetector

        // Setters of data
        void SetEventData(VData* vdata) override;

        // Printer of configuration
        void Print() const override;
        // Printer of reports
        void PrintReports() const override;

        // Share MEvent
        void SetMEvent(MEventReduced* mevent) override { fMEvent = mevent; }
        MEventReduced* GetMEvent() override { return fMEvent; }

    private:
        void ReadHits(ReducedData& coas, const int& where);
        void CleanPadMatrix();
        void InitClusterMethod(const std::string& method);
        void EnsureUniquenessOfVoxels();
    };
} // namespace ActRoot

#endif
