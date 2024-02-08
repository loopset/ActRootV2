#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/
#include "ActCalibrationManager.h"
#include "ActInputIterator.h"
#include "ActMergerDetector.h"
#include "ActModularDetector.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"
#include "ActVDetector.h"

#include "TTree.h"
// #include "BS_thread_pool.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace ActRoot
{

//! Main class interfacing all analysis detectors, with calibrations and main methods
class DetectorManager
{
private:
    static std::unordered_map<std::string, DetectorType> fDetDatabase;       //!< Equivalence .detector file [Header] to
                                                                             //!< VDetector pointer
    std::unordered_map<DetectorType, std::shared_ptr<VDetector>> fDetectors; //!< Pointer to detectors
    std::shared_ptr<CalibrationManager> fCalMan {}; //!< CalibrationManager is now included in DetectorManager to
                                                    //!< avoid singleton
    bool fIsVerbose {};
    ModeType fMode {ModeType::ENone};

public:
    DetectorManager() = default;
    DetectorManager(ModeType mode);
    ~DetectorManager() {};

    // Setter of mode
    void SetMode(ModeType mode) { fMode = mode; }
    // Get mode
    ModeType GetMode() const { return fMode; }

    // Getters of equivalences between DetectorMode and string
    static std::string GetDetectorTypeStr(DetectorType type);

    // Getter of CalibrationManager
    std::shared_ptr<CalibrationManager> GetCalMan() { return fCalMan; }

    // Read configurations
    void ReadDetectorFile(const std::string& file, bool print = true);
    void ReadCalibrationsFile(const std::string& file);
    void Reconfigure();

    // Set input and output data
    void InitInput(std::shared_ptr<TTree> input);
    void InitOutput(std::shared_ptr<TTree> output);

    // Build functions
    void BuildEvent(const int& run, const int& entry);

    // Get detectors class
    std::shared_ptr<VDetector> GetDetector(DetectorType type) { return fDetectors[type]; }
    template <typename T>
    inline std::shared_ptr<T> GetDetectorAs()
    {
        DetectorType type {DetectorType::ENone};
        if constexpr(std::is_same_v<T, TPCDetector>)
            type = DetectorType::EActar;
        else if constexpr(std::is_same_v<T, SilDetector>)
            type = DetectorType::ESilicons;
        else if constexpr(std::is_same_v<T, ModularDetector>)
            type = DetectorType::EModular;
        else if constexpr(std::is_same_v<T, MergerDetector>)
            type = DetectorType::EMerger;
        else
            throw std::runtime_error("DetectorManager::GetDetectorAs(): could not cast to passed type");
        if(fDetectors.count(type))
            return std::dynamic_pointer_cast<T>(fDetectors[type]);
        else
            throw std::runtime_error("DetectorManager::GetDetectorAs<>(): could not find " + GetDetectorTypeStr(type) +
                                     " in fDetectors");
    }

    void SetIsVerbose() { fIsVerbose = true; }

    void DeleteDetector(DetectorType type);

    void Print() const;

    void PrintReports() const;

    void SendWrapperData(InputWrapper* wrap);

private:
    void InitDetectors();
    void InitCalibrationManager();
    void InitMerger(bool print);
    void SendParametersToMerger();
    void InitCorr(bool print);
};
} // namespace ActRoot
#endif
