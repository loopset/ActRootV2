#include "CalibrationManager.h"

#include "InputParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::CalibrationManager* ActRoot::CalibrationManager::fInstance = nullptr;

ActRoot::CalibrationManager* ActRoot::CalibrationManager::Get()
{
    if(!fInstance)
    {
        std::cout<<"Instatiating singleton of ActRoot::CalibrationManager"<<'\n';
        fInstance = new CalibrationManager();
        return fInstance;
    }
    else
        return fInstance;
}

void ActRoot::CalibrationManager::ReadCalibration(const std::string &file)
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
            //std::cout<<"Sil cal = "<<key<<" val = "<<val<<'\n';
            col++;
        }
    }
}

void ActRoot::CalibrationManager::ReadPadAlign(const std::string &file)
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
            //Clean whitespaces
            row.erase(std::remove_if(row.begin(), row.end(),
                                     [](unsigned char x){return std::isspace(x);}), row.end());
            if(row.length() == 0)
                continue;
            fPadAlign.back().push_back(std::stod(row));
        }
    }
}

void ActRoot::CalibrationManager::ReadLookUpTable(const std::string &file)
{
    //number of rows automatically determined by file
    //number of cols = 6
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("Error loading LT table in CalibrationManager");
    //Run!
    std::string line {};
    while(std::getline(streamer, line))
    {
        //Reset variables
        int col0 {}; int col1 {}; int col2 {};
        int col3 {}; int col4 {}; int col5 {};
        //Line streamer
        std::istringstream lineStreamer {line};
        lineStreamer >> col0 >> col1 >> col2 >> col3
                     >> col4 >> col5;
        //std::cout<<"================="<<'\n';
        //std::cout<<"0 = "<<col0<<" back = "<<col5<<'\n';
        fLT.push_back({col0, col1, col2, col3, col4, col5});
    }
}

double ActRoot::CalibrationManager::ApplyCalibration(const std::string &key, double raw)
{
    double cal {};
    int order {0};
    std::vector<double> coeffs {};
    try
    {
        coeffs = fCalibs.at(key);
    }
    catch(std::exception& e)
    {
        throw std::runtime_error("Could not find calibration for key " + key);
    }
    for(const auto& coef : coeffs)
    {
        cal += coef * std::pow(raw, order);
        order++;
    }
    return cal;
}

bool ActRoot::CalibrationManager::ApplyThreshold(const std::string &key, double raw)
{
    return false;
}

int ActRoot::CalibrationManager::ApplyLookUp(int channel, int col)
{
    return fLT[channel][col];
}

double ActRoot::CalibrationManager::ApplyPadAlignment(int channel, double q)
{
    double qAl {};
    int order {0};
    for(const auto& coef : fPadAlign[channel])
    {
        qAl += coef * std::pow(q, order);
        order++;
    }
    return qAl;
}