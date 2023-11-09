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
        else if (det == "Merger")
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

void ActRoot::DetectorManager::ReadCalibrations(const std::string& file)
{
    ActRoot::InputParser parser {file};
    for(const auto& det : parser.GetBlockHeaders())
    {
        fDetectors[fDetDatabase[det]]->ReadCalibrations(parser.GetBlock(det));
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

std::shared_ptr<ActRoot::VDetector> ActRoot::DetectorManager::GetDetector(DetectorType type)
{
    if(fDetectors.count(type))
        return fDetectors[type];
    else
        throw std::runtime_error("DetMan::GetDetector() received an unexistent detector!");
}

void ActRoot::DetectorManager::DeleteDelector(DetectorType type)
{
    auto it {fDetectors.find(type)};
    if(it != fDetectors.end())
        fDetectors.erase(it);
    else
        std::cout << "Could not delete detector" << '\n';
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

void ActRoot::DetectorManager::InitializeDataInput(std::shared_ptr<TTree> input)
{
    for(auto& det : fDetectors)
        det.second->InitInputData(input);
}

void ActRoot::DetectorManager::InitializePhysicsOutput(std::shared_ptr<TTree> output)
{
    for(auto& det : fDetectors)
        det.second->InitOutputPhysics(output);
}

void ActRoot::DetectorManager::BuildEventData()
{
    for(auto& det : fDetectors)
    {
        det.second->ClearEventData();
    }
    // This has to be splitted since MEvent could only be read from one branch at the same time
    // And that branch is in Actar detector, so we pass the pointer manually to the other detectors
    // 1-->Actar
    if(fDetectors[DetectorType::EActar])
        fDetectors[DetectorType::EActar]->BuildEventData();
    // 2-->Silicons
    if(fDetectors.find(DetectorType::ESilicons) != fDetectors.end())
    {
        fDetectors[DetectorType::ESilicons]->SetMEvent(fDetectors[DetectorType::EActar]->GetMEvent());
        fDetectors[DetectorType::ESilicons]->BuildEventData();
    }
    // 3-->Modular
    if(fDetectors.find(DetectorType::EModular) != fDetectors.end())
    {
        fDetectors[DetectorType::EModular]->SetMEvent(fDetectors[DetectorType::EActar]->GetMEvent());
        fDetectors[DetectorType::EModular]->BuildEventData();
    }
}

void ActRoot::DetectorManager::BuildEventPhysics()
{
    for(auto& det : fDetectors)
    {
        det.second->ClearEventPhysics();
        det.second->BuildEventPhysics();
    }
}

void ActRoot::DetectorManager::PrintReports() const
{
    for(const auto& [_, det] : fDetectors)
        det->PrintReports();
}

void ActRoot::DetectorManager::SetEventData(DetectorType det, VData* vdata)
{
    if(fDetectors.count(det))
        fDetectors[det]->SetEventData(vdata);
    else
        std::cout << "Could not locate detector!";
}

void ActRoot::DetectorManager::InitializeMergerInput(std::shared_ptr<TTree> tree)
{
    fMerger->InitInputMerger(tree);
}

void ActRoot::DetectorManager::InitializeMergerOutput(std::shared_ptr<TTree> tree)
{
    fMerger->InitOutputMerger(tree);
}

void ActRoot::DetectorManager::BuildEventMerger()
{
    fMerger->ClearOutputMerger();
    fMerger->MergeEvent();
}
