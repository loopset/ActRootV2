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

#include "TTree.h"
// #include "BS_thread_pool.hpp"
#include "ActVDetector.h"

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
        EModular
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
        std::shared_ptr<MergerDetector> fMerger {}; //!< Merge detector in final step (does not inheritate from VDetector so
                                                //!< far)

    public:
        DetectorManager();
        DetectorManager(const std::string& file);
        ~DetectorManager() {};

        // Getters and deleters of detectors
        std::shared_ptr<CalibrationManager> GetCalMan() { return fCalMan; }
        std::shared_ptr<ActRoot::VDetector> GetDetector(DetectorType type);
        void DeleteDelector(DetectorType type);
        int GetNumberOfDetectors() const { return fDetectors.size(); }

        // Read configurations
        void ReadConfiguration(const std::string& file);
        void ReadCalibrations(const std::string& file);

        // Init INPUT data
        void InitializeDataInputRaw(std::shared_ptr<TTree> input, int run);
        void InitializeDataInput(std::shared_ptr<TTree> input);
        void InitializeMergerInput(std::shared_ptr<TTree> input);

        // Init OUTPUT data
        void InitializeDataOutput(std::shared_ptr<TTree> output);
        void InitializePhysicsOutput(std::shared_ptr<TTree> output);
        void InitializeMergerOutput(std::shared_ptr<TTree> output);

        // Builder of EVENTs
        void BuildEventData();
        void BuildEventPhysics();
        void BuildEventMerger();

        void PrintReports() const;

        void SetEventData(DetectorType det, VData* vdata);

    private:
        void InitMerger(std::shared_ptr<InputBlock> block);
        void SendParametersToMerger();
    };
} // namespace ActRoot
#endif
