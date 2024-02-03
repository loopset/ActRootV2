#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/
#include "ActCalibrationManager.h"
#include "ActCorrDetector.h"
#include "ActInputParser.h"
#include "ActMergerDetector.h"
#include "ActModularDetector.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TTree.h"
// #include "BS_thread_pool.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ActRoot
{

//! Main class interfacing all analysis detectors, with calibrations and main methods
class DetectorManager
{
private:
    std::unordered_map<DetectorType, std::shared_ptr<VDetector>> fDetectors; //!< Pointer to detectors
    std::unordered_map<std::string, DetectorType> fDetDatabase;              //!< Equivalence .detector file [Header] to
                                                                             //!< VDetector pointer
    std::shared_ptr<CalibrationManager> fCalMan {}; //!< CalibrationManager is now included in DetectorManager to
                                                    //!< avoid singleton
    std::shared_ptr<MergerDetector> fMerger {};     //!< Merge detector in final step (does not inheritate from
                                                    //!< VDetector so far)
    std::shared_ptr<CorrDetector> fCorr {};         //!< A simple detector to correct Qave and Zoffset
    bool fIsRawOk {};                               //!< Check that SetRawToData has been called
    bool fIsCluster {};                             //!< Convert Raw to Data only for TPC
    bool fIsData {};                                //!< Convert Raw to Data only for Sil and others
    bool fIsVerbose {};

public:
    DetectorManager();
    DetectorManager(const std::string& file);
    ~DetectorManager() {};

    // Getter of CalibrationManager
    std::shared_ptr<CalibrationManager> GetCalMan() { return fCalMan; }

    // Read configurations
    void ReadConfiguration(const std::string& file, bool print = true);
    void Reconfigure();
    void ReadCalibrations(const std::string& file);

    // Set is verbose mode
    void SetIsVerbose()
    {
        fIsVerbose = true;
        fMerger->SetIsVerbose();
    }

    // Set detectors to treat when building Raw -> Data
    void SetIsCluster()
    {
        fIsCluster = true;
        fIsData = false;
        fIsRawOk = true;
    }
    void SetIsData()
    {
        fIsData = true;
        fIsCluster = false;
        fIsRawOk = true;
    }
    // Get detector class
    std::shared_ptr<VDetector> GetDetector(DetectorType type) { return fDetectors[type]; }
    std::shared_ptr<TPCDetector> GetTPCDetector()
    {
        return std::dynamic_pointer_cast<TPCDetector>(fDetectors[DetectorType::EActar]);
    }
    std::shared_ptr<SilDetector> GetSilDetector()
    {
        return std::dynamic_pointer_cast<SilDetector>(fDetectors[DetectorType::ESilicons]);
    }
    std::shared_ptr<ModularDetector> GetModularDetector()
    {
        return std::dynamic_pointer_cast<ModularDetector>(fDetectors[DetectorType::EModular]);
    }
    void DeleteDetector(DetectorType type);

    // Init INPUT data
    void InitInputRaw(std::shared_ptr<TTree> input);
    void InitInputMerger(std::shared_ptr<TTree> input);
    void InitInputCorr(std::shared_ptr<TTree> input);

    // Init OUTPUT data
    void InitOutputData(std::shared_ptr<TTree> output);
    void InitOutputMerger(std::shared_ptr<TTree> output);
    void InitOutputCorr(std::shared_ptr<TTree> output);

    // Builder of EVENTs
    void BuildEventData();
    void BuildEventMerger(int run, int entry);
    void BuildEventCorr();

    void Print() const;

    void PrintReports() const;

    void SetEventData(DetectorType det, VData* vdata);

    // Others
    std::shared_ptr<MergerDetector> GetMerger() const { return fMerger; }
    std::shared_ptr<CorrDetector> GetCorr() const { return fCorr; }

private:
    void InitMerger(bool print);
    void SendParametersToMerger();
    void InitCorr(bool print);
};
} // namespace ActRoot
#endif
