#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/

#include "ActCalibrationManager.h"
#include "ActInputParser.h"
#include "ActMergerDetector.h"
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
    enum class DetectorType
    {
        EActar,
        ESilicons,
        EModular,
        EMerger
    };

    //! Main class interfacing all analysis detectors, with calibrations and main methods
    class DetectorManager
    {
    private:
        std::unordered_map<DetectorType, std::shared_ptr<VDetector>> fDetectors; //!< Pointer to detectors
        std::unordered_map<std::string, DetectorType> fDetDatabase; //!< Equivalence .detector file [Header] to
                                                                    //!< VDetector pointer
        std::shared_ptr<CalibrationManager> fCalMan {}; //!< CalibrationManager is now included in DetectorManager to
                                                        //!< avoid singleton
        std::shared_ptr<MergerDetector> fMerger {};     //!< Merge detector in final step (does not inheritate from
                                                        //!< VDetector so far)
        bool fIsRawOk {false};                          //!< Check that SetRawToData has been called
    public:
        DetectorManager();
        DetectorManager(const std::string& file);
        ~DetectorManager() {};

        // Getter of CalibrationManager
        std::shared_ptr<CalibrationManager> GetCalMan() { return fCalMan; }

        // Read configurations
        void ReadConfiguration(const std::string& file);
        void Reconfigure();
        void ReadCalibrations(const std::string& file);

        // Set detectors to treat when building Raw -> Data
        void SetRawToDataDetectors(std::vector<DetectorType> which);
        void DeleteDetector(DetectorType type);

        // Init INPUT data
        void InitInputRaw(std::shared_ptr<TTree> input);
        void InitInputMerger(std::shared_ptr<TTree> input);

        // Init OUTPUT data
        void InitOutputData(std::shared_ptr<TTree> output);
        void InitOutputMerger(std::shared_ptr<TTree> output);

        // Builder of EVENTs
        void BuildEventData();
        void BuildEventMerger(int run, int entry);

        void Print() const;

        void PrintReports() const;

        void SetEventData(DetectorType det, VData* vdata);

    private:
        void InitMerger(std::shared_ptr<InputBlock> block);
        void SendParametersToMerger();
    };
} // namespace ActRoot
#endif
