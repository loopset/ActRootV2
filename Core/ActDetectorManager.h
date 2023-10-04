#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/

#include "ActCalibrationManager.h"
#include "TTree.h"
//#include "BS_thread_pool.hpp"
#include "ActVDetector.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace ActRoot
{
    enum class DetectorType {EActar, ESilicons, EModular};

    //! Main class interfacing all analysis detectors, with calibrations and main methods
    class DetectorManager
    {
    private:
        std::unordered_map<ActRoot::DetectorType, std::shared_ptr<ActRoot::VDetector>> fDetectors;//!< Pointer to detectors
        std::unordered_map<std::string, DetectorType> fDetDatabase;//!< Equivalence .detector file [Header] to VDetector pointer
        std::shared_ptr<CalibrationManager> fCalMan {};//!< CalibrationManager is now included in DetectorManager to avoid singleton
        
    public:
        DetectorManager();
        DetectorManager(const std::string& file);
        ~DetectorManager() {};

        std::shared_ptr<CalibrationManager> GetCalMan() {return fCalMan;}
        void DeleteDelector(DetectorType type);
        int GetNumberOfDetectors() const {return fDetectors.size();}
        void ReadConfiguration(const std::string& file);
        void ReadCalibrations(const std::string& file);
        void InitializeDataInputRaw(std::shared_ptr<TTree> input, int run);
        void InitializeDataOutput(std::shared_ptr<TTree> output);
        void BuildEventData();
    };
}
#endif
