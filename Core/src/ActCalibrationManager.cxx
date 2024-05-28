#include "ActCalibrationManager.h"

#include "ActInputParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::CalibrationManager::CalibrationManager(const std::string& calfile)
{
    ReadCalibration(calfile);
}

void ActRoot::CalibrationManager::ReadCalibration(const std::string& file)
{
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("No calibration file found: " + file);
    std::string line {};
    while(std::getline(streamer, line))
    {
        int col {};
        std::string key {};
        std::istringstream lineStreamer {line};
        std::string buffer {};
        while(std::getline(lineStreamer, buffer, ' '))
        {
            auto val {ActRoot::StripSpaces(buffer)};
            if(val.length() == 0)
                continue;
            if(col == 0)
                key = val;
            else
                fCalibs[key].push_back(std::stod(val));
            col++;
        }
    }
}

void ActRoot::CalibrationManager::ReadPadAlign(const std::string& file)
{
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("Pad align coeff file does not exist in CalibrationManager");
    std::string line {};
    while(std::getline(streamer, line))
    {
        std::istringstream lineStreamer {line};
        std::string row {};
        fPadAlign.push_back({});
        while(std::getline(lineStreamer, row, ' '))
        {
            // Clean whitespaces
            row.erase(std::remove_if(row.begin(), row.end(), [](unsigned char x) { return std::isspace(x); }),
                      row.end());
            if(row.length() == 0)
                continue;
            fPadAlign.back().push_back(std::stod(row));
        }
    }
}

void ActRoot::CalibrationManager::ReadLookUpTable(const std::string& file)
{
    // number of rows automatically determined by file
    // number of cols = 6
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("Error loading LT table in CalibrationManager");
    // Run!
    std::string line {};
    while(std::getline(streamer, line))
    {
        // Reset variables
        int col0 {};
        int col1 {};
        int col2 {};
        int col3 {};
        int col4 {};
        int col5 {};
        // Line streamer
        std::istringstream lineStreamer {line};
        lineStreamer >> col0 >> col1 >> col2 >> col3 >> col4 >> col5;
        // std::cout<<"================="<<'\n';
        // std::cout<<"0 = "<<col0<<" back = "<<col5<<'\n';
        fLT.push_back({col0, col1, col2, col3, col4, col5});
    }
}

double ActRoot::CalibrationManager::ApplyCalibration(const std::string& key, double raw)
{
    if(fCalibs.find(key) != fCalibs.end())
    {
        double cal {0};
        int order {0};
        auto coeffs {fCalibs[key]};
        for(const auto& coef : coeffs)
        {
            cal += coef * std::pow(raw, order);
            order++;
        }
        return cal;
    }
    else
    {
        if(!fIsEnabled)
            return raw;
        throw std::runtime_error(
            "CalibrationManager::ApplyCalibration(): fIsEnabled == true but could not find calibration for key " + key);
    }
}

bool ActRoot::CalibrationManager::ApplyThreshold(const std::string& key, double raw, double nsigma)
{
    if(fCalibs.find(key) != fCalibs.end())
    {
        auto coeffs {fCalibs[key]};
        // Value to compare to
        double threshold {coeffs[0]};
        if(coeffs.size() == 2) // CATS style
        {
            threshold += coeffs[1] * nsigma;
        }
        if(raw < threshold)
            return false;
        else
            return true;
    }
    else
    {
        if(!fIsEnabled)
            return raw;
        throw std::runtime_error(
            "CalibrationManager::ApplyThreshold(): fIsEnabled == true but could not find threshold for key " + key);
    }
}

int ActRoot::CalibrationManager::ApplyLookUp(int channel, int col)
{
    return fLT[channel][col];
}

double ActRoot::CalibrationManager::ApplyPadAlignment(int channel, double q)
{
    // If alignment is disabled, return given value
    if(fPadAlign.size() == 0)
        return q;
    // Else, compute correction with any order given in the file
    double qAl {};
    int order {0};
    for(const auto& coef : fPadAlign[channel])
    {
        qAl += coef * std::pow(q, order);
        order++;
    }
    return qAl;
}

void ActRoot::CalibrationManager::Print() const
{
    std::cout << "===============================================" << '\n';
    std::cout << "Pad align table with size   = " << fPadAlign.size() << '\n';
    std::cout << "Pad look up table with size = " << fLT.size() << '\n';
    std::cout << "Other calibrations = " << '\n';
    for(const auto& [key, vals] : fCalibs)
    {
        std::cout << "-> Key " << key << '\n';
        for(const auto& val : vals)
            std::cout << "   " << val << '\n';
    }
}
