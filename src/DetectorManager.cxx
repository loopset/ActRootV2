#include "DetectorManager.h"
#include "InputParser.h"

#include "TPCDetector.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::DetectorManager::DetectorManager()
{
    //Set assignments for strings to enum type
    fDetDatabase = {
        {"Actar", DetectorType::EActar},
        {"Silicons", DetectorType::ESilicons}
    };
}

void ActRoot::DetectorManager::ReadConfiguration(const std::string &file)
{
    //Preliminary!
    ActRoot::InputParser parser {file};
    for(const auto& det : parser.GetBlockHeaders())
    {
        //Build detector class
        if(det == "Actar")
            fDetectors[fDetDatabase[det]] = std::make_shared<ActRoot::TPCDetector>();
        else if(det == "Silicons")
            throw std::invalid_argument("No silicon detector implemented yet!");
        else
            throw std::runtime_error("Detector " + det + " not found in Manager database");
        //Read config file
        fDetectors[fDetDatabase[det]]->ReadConfiguration(parser.GetBlock(det));
    }
}
