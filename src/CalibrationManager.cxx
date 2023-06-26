#include "CalibrationManager.h"
#include "InputParser.h"
#include <iostream>
#include <string>

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

void ActRoot::CalibrationManager::ReadPadAlign(const std::string &file)
{
    
}

void ActRoot::CalibrationManager::ReadLookUpTable(const std::string &file)
{
    
}
