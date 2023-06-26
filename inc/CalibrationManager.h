#ifndef ActCalibrationManager_h
#define ActCalibrationManager_h

/*
Singleton class holding the calibrations for all the detectors!
*/

#include <string>
#include <unordered_map>
#include <vector>
namespace ActRoot
{    
    class CalibrationManager
    {
    private:
        std::unordered_map<std::string, std::vector<double>> fCalibs;
        std::vector<std::vector<int>> fLT;
        std::vector<std::vector<double>> fPadAlign;
        std::vector<std::string> fFiles;

        //Constructor and destructor are private because we want a singleton
        static CalibrationManager* fInstance;
        CalibrationManager() = default;
        ~CalibrationManager() = default;

    public:
        static CalibrationManager* Get();

        //Actar: needs improvements but that depends on .txt file format (need to add keys to parameters)
        void ReadLookUpTable(const std::string& file);
        void ReadPadAlign(const std::string& file);
        int ApplyLookUp(int channel, int col);
        double ApplyPadAlignment(int channel, double q);
        
    };
}

#endif
