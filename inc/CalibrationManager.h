#ifndef ActCalibrationManager_h
#define ActCalibrationManager_h

/*
Singleton class holding the calibrations for all the detectors!
*/

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace ActRoot
{
    //! The singleton class managin all calibrations
    /*!
      Specific methods are implemented for LT and Pad alignment,
      but the other calibrations work the same as nptools
     */
    class CalibrationManager
    {
    private:
        std::unordered_map<std::string, std::vector<double>> fCalibs;//!< General map holding strings as keys for the vector of doubles (coeffs) of calib
        std::vector<std::vector<int>> fLT;//<! Special for Look up table on pad plane
        std::vector<std::vector<double>> fPadAlign;//!< Pad align coefficiens
        std::vector<std::string> fFiles;//!< List of files read in calibration

        //Constructor and destructor are private because we want a singleton
        static CalibrationManager* fInstance;//!< Singleton of class
        CalibrationManager() = default;//<! Private constructor of singleton
        ~CalibrationManager() = default;//<! Private destructor of singleton

    public:
        static CalibrationManager* Get();//!< General method to interface with class

        //Actar: needs improvements but that depends on .txt file format (need to add keys to parameters)
        void ReadCalibration(const std::string& file);
        void ReadLookUpTable(const std::string& file);
        void ReadPadAlign(const std::string& file);
        double ApplyCalibration(const std::string& key, double raw);
        bool ApplyThreshold(const std::string& key, double raw);
        int ApplyLookUp(int channel, int col);
        double ApplyPadAlignment(int channel, double q);
        
    };
}

#endif
