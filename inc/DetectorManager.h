#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/

#include "TTree.h"
#include "VDetector.h"

#include <string>
#include <unordered_map>
#include <memory>

namespace ActRoot
{
    enum class DetectorType {EActar, ESilicons};
    
    class DetectorManager
    {
    private:
        std::unordered_map<ActRoot::DetectorType, std::shared_ptr<ActRoot::VDetector>> fDetectors;
        std::unordered_map<std::string, DetectorType> fDetDatabase;
    public:
        DetectorManager();
        ~DetectorManager() {};

        void ReadConfiguration(const std::string& file);
        void ReadCalibrations(const std::string& file);
        void InitializeDataInputRaw(std::shared_ptr<TTree> input, int run);
        void InitializeDataOutput(std::shared_ptr<TTree> input, int run);
    };
}
#endif
