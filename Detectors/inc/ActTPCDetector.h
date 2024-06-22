#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActVCluster.h"
#include "ActVData.h"
#include "ActVDetector.h"
#include "ActVFilter.h"
#include "ActVParameters.h"

#include "TStopwatch.h"
#include "TTree.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace ActRoot
{
class InputBlock; // forward declaration

class TPCParameters : public VParameters
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
    // And contain also real sizes in mm
    double fX {};
    double fY {};
    double fZ {};

public:
    TPCParameters() = default;
    TPCParameters(const std::string& type);
    // Setters
    void SetREBINZ(int rebin);
    // Getters
    double GetPadSide() const { return fPadSide; }
    int GetNPADSX() const { return fNPADSX; }
    int GetNPADSY() const { return fNPADSY; }
    int GetNPADSZUNREBIN() const { return fNPADSZ; }
    int GetNPADSZ() const { return fNPADSZ / fREBINZ; }
    int GetREBINZ() const { return fREBINZ; }
    int GetNBCOBO() const { return fNB_COBO; }
    int GetNBASAD() const { return fNB_ASAD; }
    int GetNBAGET() const { return fNB_AGET; }
    int GetNBCHANNEL() const { return fNB_CHANNEL; }
    double GetX() const { return fX; }
    double X() const { return GetX(); }
    double GetY() const { return fY; }
    double Y() const { return GetY(); }
    double GetZ() const { return fZ; }
    double Z() const { return GetZ(); }

    void Print() const override;
};

class TPCDetector : public VDetector
{
private:
    // Parameters of detector
    TPCParameters fPars;

    // Auxiliars to read data
    MEventReduced* fMEvent {};
    std::vector<ActRoot::Voxel> fVoxels {};

    // Data itself
    TPCData* fData {};

    // Preanalysis when reading raw data
    bool fCleanSaturatedMEvent {false};
    bool fCleanDuplicatedVoxels {false};
    bool fCleanPadMatrix {false};
    double fMinTBtoDelete {20};
    double fMinQtoDelete {2000};
    std::unordered_map<unsigned int, std::pair<std::set<unsigned int, std::greater<>>, double>> fPadMatrix;
    std::unordered_map<unsigned int, unsigned int> fGlobalIndex {};

    // Timer for cluster (only cluster) step
    TStopwatch fClusterClock;

    // Cluster method
    std::shared_ptr<ActAlgorithm::VCluster> fCluster {};
    // Filter method
    std::shared_ptr<ActAlgorithm::VFilter> fFilter {};

    // Ensure cleaning of news in this class
    bool fDelMEvent {};
    bool fDelData {};

public:
    TPCDetector() = default;
    ~TPCDetector() override;

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
    void SetInputData(VData* data) override {}
    VData* GetInputData() const override { return nullptr; }
    void SetOutputData(VData* data) override { fData = data->CastAs<TPCData>(); }
    TPCData* GetOutputData() const override { return fData; }

    void SetInputFilter(VData* data) override { fData = data->CastAs<TPCData>(); }
    TPCData* GetInputFilter() const override { return fData; }
    void SetOutputFilter(VData* data) override { fData = data->CastAs<TPCData>(); }
    TPCData* GetOutputFilter() const override { return fData; }

    // Getters
    TPCParameters* GetParameters() override { return &fPars; }

    // Printer of configuration
    void Print() const override;
    // Printer of reports
    void PrintReports() const override;

    // Share MEvent
    void SetMEvent(MEventReduced* mevent) override { fMEvent = mevent; }
    MEventReduced* GetMEvent() override { return fMEvent; }

    // Others
    std::shared_ptr<ActAlgorithm::VCluster> GetCluster() const { return fCluster; }
    std::shared_ptr<ActAlgorithm::VFilter> GetFilter() const { return fFilter; }

    template <typename T>
    std::shared_ptr<T> GetClusterAs() const
    {
        return std::dynamic_pointer_cast<T>(fCluster);
    }

    void Recluster();

private:
    void ReadHits(ReducedData& coas, const int& where);
    void CleanPadMatrix();
    void InitClusterMethod(const std::string& method);
    void InitFilterMethod(const std::string& method);
    unsigned int BuildGlobalIndex(const int& x, const int& y, const int& z);
    unsigned int BuildGlobalPadIndex(const int& x, const int& y);
};
} // namespace ActRoot

#endif
