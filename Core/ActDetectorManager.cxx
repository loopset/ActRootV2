#include "ActDetectorManager.h"

#include "ActCalibrationManager.h"
#include "ActInputParser.h"
#include "ActMergerDetector.h"
#include "ActModularDetector.h"
#include "ActSilDetector.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActVDetector.h"

#include "TString.h"
#include "TTree.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::DetectorManager::DetectorManager()
{
    // Set assignments for strings to enum type
    fDetDatabase = {
        {"Actar", DetectorType::EActar},
        {"Silicons", DetectorType::ESilicons},
        {"Modular", DetectorType::EModular},
    };
    // Initialize Calibration Manager
    fCalMan = std::make_shared<CalibrationManager>();
}

ActRoot::DetectorManager::DetectorManager(const std::string& file) : DetectorManager()
{
    ReadConfiguration(file);
}

void ActRoot::DetectorManager::ReadConfiguration(const std::string& file)
{
    // Preliminary!
    ActRoot::InputParser parser {file};
    for(const auto& det : parser.GetBlockHeaders())
    {
        // Build detector class
        if(det == "Actar")
            fDetectors[fDetDatabase[det]] = std::make_shared<ActRoot::TPCDetector>();
        else if(det == "Silicons")
            fDetectors[fDetDatabase[det]] = std::make_shared<ActRoot::SilDetector>();
        else if(det == "Modular")
            fDetectors[fDetDatabase[det]] = std::make_shared<ActRoot::ModularDetector>();
        else if(det == "Merger")
            continue;
        else
            throw std::runtime_error("Detector " + det + " not found in Manager database");
        // Read config file
        fDetectors[fDetDatabase[det]]->ReadConfiguration(parser.GetBlock(det));
        // Set CalibrationManager pointer
        fDetectors[fDetDatabase[det]]->SetCalMan(fCalMan);
    }
    // Init Merger detector separately
    InitMerger(parser.GetBlock("Merger"));
}

void ActRoot::DetectorManager::Reconfigure()
{
    for(auto& [_, det] : fDetectors)
        det->Reconfigure();
}

void ActRoot::DetectorManager::ReadCalibrations(const std::string& file)
{
    ActRoot::InputParser parser {file};
    for(const auto& det : parser.GetBlockHeaders())
    {
        auto type {fDetDatabase[det]};
        if(fDetectors.count(type))
            fDetectors[type]->ReadCalibrations(parser.GetBlock(det));
    }
}

void ActRoot::DetectorManager::InitMerger(std::shared_ptr<InputBlock> block)
{
    fMerger = std::make_shared<MergerDetector>();
    SendParametersToMerger();
    fMerger->ReadConfiguration(block);
    fMerger->Print();
}

void ActRoot::DetectorManager::SendParametersToMerger()
{
    // TPC
    if(fDetectors.count(DetectorType::EActar))
    {
        auto p {std::dynamic_pointer_cast<TPCDetector>(fDetectors[DetectorType::EActar])};
        fMerger->SetTPCParameters(p->GetParametersPointer());
    }
    // Silicons
    if(fDetectors.count(DetectorType::ESilicons))
    {
        auto p {std::dynamic_pointer_cast<SilDetector>(fDetectors[DetectorType::ESilicons])};
        fMerger->SetSilParameters(p->GetParametersPointer());
    }
    // Modular
    if(fDetectors.count(DetectorType::EModular))
    {
        auto p {std::dynamic_pointer_cast<ModularDetector>(fDetectors[DetectorType::EModular])};
        fMerger->SetModularParameters(p->GetParametersPointer());
    }
}

void ActRoot::DetectorManager::DeleteDetector(DetectorType type)
{
    auto it {fDetectors.find(type)};
    if(it != fDetectors.end())
        fDetectors.erase(it);
    else
        std::cout << "Could not delete detector" << '\n';
}

void ActRoot::DetectorManager::SetRawToDataDetectors(std::vector<DetectorType> which)
{
    for(auto it = fDetectors.begin(); it != fDetectors.end();)
    {
        bool isInList {std::find(which.begin(), which.end(), it->first) != which.end()};
        if(!isInList)
            it = fDetectors.erase(it);
        else
            it++;
    }
    fIsRawOk = true;
}

void ActRoot::DetectorManager::InitInputRaw(std::shared_ptr<TTree> input)
{
    if(!fIsRawOk)
        throw std::runtime_error("DetectorManager::InitInputRaw -> When in Raw to Data mode, you must specify which "
                                 "detectors via SetRawToDataDetectors");
    for(auto& det : fDetectors)
        det.second->InitInputRaw(input);
}

void ActRoot::DetectorManager::InitOutputData(std::shared_ptr<TTree> input)
{
    for(auto& det : fDetectors)
        det.second->InitOutputData(input);
}

void ActRoot::DetectorManager::InitInputMerger(std::shared_ptr<TTree> input)
{
    fMerger->InitInputMerger(input);
}

void ActRoot::DetectorManager::InitOutputMerger(std::shared_ptr<TTree> output)
{
    fMerger->InitOutputMerger(output);
}

void ActRoot::DetectorManager::BuildEventData()
{
    for(auto& det : fDetectors)
    {
        det.second->ClearEventData();
        det.second->BuildEventData();
    }
}

void ActRoot::DetectorManager::Print() const 
{
    for(const auto& [_, det] : fDetectors)
        det->Print();
}

void ActRoot::DetectorManager::PrintReports() const
{
    for(const auto& [_, det] : fDetectors)
        det->PrintReports();
}

void ActRoot::DetectorManager::SetEventData(DetectorType det, VData* vdata)
{
    if(det == DetectorType::EMerger && fMerger)
        fMerger->SetEventData(vdata);
    else if(fDetectors.count(det))
        fDetectors[det]->SetEventData(vdata);
    else
        std::cout << "Could not locate detector!";
}

void ActRoot::DetectorManager::BuildEventMerger(int run, int entry)
{
    fMerger->ClearEventMerger();
    fMerger->SetCurrentRunEntry(run, entry);
    fMerger->BuildEventMerger();
}
