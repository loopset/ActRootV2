#ifndef ActDetectorManager_h
#define ActDetectorManager_h

/*
A class holding all the detector info and the main interface to the operations
performed on its data
*/

#include "TTree.h"
//#include "BS_thread_pool.hpp"
#include "VDetector.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace ActRoot
{
    enum class DetectorType {EActar, ESilicons};
    
    class DetectorManager
    {
    private:
        std::unordered_map<ActRoot::DetectorType, std::shared_ptr<ActRoot::VDetector>> fDetectors;
        std::unordered_map<std::string, DetectorType> fDetDatabase;
        //BS::thread_pool fTP;
        
    public:
        DetectorManager();
        ~DetectorManager() {};

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
