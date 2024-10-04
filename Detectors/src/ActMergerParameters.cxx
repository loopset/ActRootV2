#include "ActMergerParameters.h"

#include "ActColors.h"

#include <iostream>
#include <stdexcept>

std::string ActRoot::MergerModeToStr(ActRoot::MergerMode mode)
{
    switch(mode)
    {
    case MergerMode::ENone: return "None";
    case ActRoot::MergerMode::ECalibration: return "Calibration";
    case ActRoot::MergerMode::EAutomatic: return "Automatic";
    default: throw std::runtime_error("MergerModeToStr(): received unknown MergerMode");
    }
}

ActRoot::MergerMode ActRoot::StrToMergerMode(const std::string& str)
{
    if(str == "None")
        return MergerMode::ENone;
    if(str == "Calibration")
        return MergerMode::ECalibration;
    if(str == "Automatic")
        return MergerMode::EAutomatic;
    throw std::runtime_error("StrToMergerMode(): str " + str + " is not mapped to any MergerMode");
}

void ActRoot::MergerParameters::Print() const
{
    std::cout << BOLDYELLOW << ":::: MergerParameters ::::" << '\n';
    std::cout << "-> UseRP ? " << std::boolalpha << fUseRP << '\n';
    std::cout << "-> IsL1  ? " << std::boolalpha << fIsL1 << '\n';
    std::cout << "-> IsCal ? " << std::boolalpha << fIsCal << '\n';
    std::cout << "::::::::::::::::::::::::::::::" << RESET << '\n';
}
