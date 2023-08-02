#include "DetectorManager.h"
#include "InputParser.h"

#include "SilDetector.h"
#include "TPCData.h"
#include "TPCDetector.h"
#include "TString.h"
#include "TTree.h"
#include "VDetector.h"

#include <iostream>
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
            fDetectors[fDetDatabase[det]] = std::make_shared<ActRoot::SilDetector>();
        else
            throw std::runtime_error("Detector " + det + " not found in Manager database");
        //Read config file
        fDetectors[fDetDatabase[det]]->ReadConfiguration(parser.GetBlock(det));
    }
    //std::cout<<"Detector size = "<<fDetectors.size()<<'\n';
    //fTP.reset(fDetectors.size());
}

void ActRoot::DetectorManager::ReadCalibrations(const std::string &file)
{
    ActRoot::InputParser parser {file};
    for(const auto& det : parser.GetBlockHeaders())
    {
        fDetectors[fDetDatabase[det]]->ReadCalibrations(parser.GetBlock(det));
    }
}

void ActRoot::DetectorManager::DeleteDelector(DetectorType type)
{
    auto it {fDetectors.find(type)};
    if(it != fDetectors.end())
        fDetectors.erase(it);
    else
        std::cout<<"Could not delete detector"<<'\n';
}

void ActRoot::DetectorManager::InitializeDataInputRaw(std::shared_ptr<TTree> input, int run)
{
    for(auto& det : fDetectors)
            det.second->InitInputRawData(input, run);
}

void ActRoot::DetectorManager::InitializeDataOutput(std::shared_ptr<TTree> input)
{
    for(auto& det : fDetectors)
        det.second->InitOutputData(input);
}

void ActRoot::DetectorManager::BuildEventData()
{
    for(auto& det : fDetectors)
    {
        det.second->ClearEventData();
        //det.second->BuildEventData();
    }
    // //1-->Actar
    if(fDetectors[DetectorType::EActar])
        fDetectors[DetectorType::EActar]->BuildEventData();
    //2-->Silicons
    if(fDetectors.find(DetectorType::ESilicons) != fDetectors.end())
    {
        fDetectors[DetectorType::ESilicons]->SetMEvent(fDetectors[DetectorType::EActar]->GetMEvent());
        fDetectors[DetectorType::ESilicons]->BuildEventData();
        //fTP.push_task(&ActRoot::VDetector::BuildEventData, fDetectors[DetectorType::ESilicons]);
    }
    // //fTP.wait_for_tasks();
}