#ifndef ActCalibrationManager_h
#define ActCalibrationManager_h

/*
Singleton class holding the calibrations for all the detectors!
*/

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace ActRoot
{
    //! Class managing all calibrations (NO LONGER a singleton)
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

    public:
        CalibrationManager() = default;//<! Default constructor
        CalibrationManager(const std::string& calfile);//!< Construct and read a standard calibration file at once
        ~CalibrationManager() = default;//<! Destructor
        
        //Actar: needs improvements but that depends on .txt file format (need to add keys to parameters)
        void ReadCalibration(const std::string& file);
        void ReadLookUpTable(const std::string& file);
        void ReadPadAlign(const std::string& file);
        double ApplyCalibration(const std::string& key, double raw);
        bool ApplyThreshold(const std::string& key, double raw, double nsigma = 1);
        int ApplyLookUp(int channel, int col);
        double ApplyPadAlignment(int channel, double q);
        void Print() const;
    };
}

#endif
