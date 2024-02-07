#include "ActDetectorManager.h"

#include "ActCalibrationManager.h"
#include "ActInputIterator.h"
#include "ActInputParser.h"
#include "ActMergerDetector.h"
#include "ActModularDetector.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"
#include "ActVData.h"

#include "TTree.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, ActRoot::DetectorType> ActRoot::DetectorManager::fDetDatabase = {
    {"Actar", DetectorType::EActar},   {"Silicons", DetectorType::ESilicons},       {"Modular", DetectorType::EModular},
    {"Merger", DetectorType::EMerger}, {"Corrections", DetectorType::ECorrections}, {"None", DetectorType::ENone},
};

std::string ActRoot::DetectorManager::GetDetectorTypeStr(ActRoot::DetectorType type)
{
    for(const auto& [key, val] : fDetDatabase)
        if(val == type)
            return key;
    return "";
}

ActRoot::DetectorManager::DetectorManager(ActRoot::ModeType mode) : fMode(mode)
{
    InitDetectors();
    InitCalibrationManager();
}

void ActRoot::DetectorManager::InitDetectors()
{
    if(fMode == ModeType::EReadTPC)
        fDetectors[DetectorType::EActar] = std::make_shared<ActRoot::TPCDetector>();
    else if(fMode == ModeType::EReadSilMod)
    {
        // Silicon
        fDetectors[DetectorType::ESilicons] = std::make_shared<ActRoot::SilDetector>();
        fDetectors[DetectorType::EModular] = std::make_shared<ActRoot::ModularDetector>();
    }
    else if(fMode == ModeType::EFilter)
    {
        // Thus far only TPCDetector has a filter implemented
        fDetectors[DetectorType::EActar] = std::make_shared<ActRoot::TPCDetector>();
    }
    else if(fMode == ModeType::EMerge || fMode == ModeType::EGui)
    {
        fDetectors[DetectorType::EActar] = std::make_shared<ActRoot::TPCDetector>();
        fDetectors[DetectorType::ESilicons] = std::make_shared<ActRoot::SilDetector>();
        fDetectors[DetectorType::EModular] = std::make_shared<ActRoot::ModularDetector>();
        fDetectors[DetectorType::EMerger] = std::make_shared<ActRoot::MergerDetector>();
    }
    else if(fMode == ModeType::ECorrect)
    {
        // fDetectors[DetectorType::ECorrections] = std::make_shared<ActRoot::CorrDetector>();
    }
    else
        throw std::runtime_error("DetectorManager::InitDetectors() : fMode is None, enter other mode!");
}

void ActRoot::DetectorManager::InitCalibrationManager()
{
    fCalMan = std::make_shared<ActRoot::CalibrationManager>();
    for(auto& [key, det] : fDetectors)
        det->SetCalMan(fCalMan);
}

void ActRoot::DetectorManager::ReadDetectorFile(const std::string& file, bool print)
{
    ActRoot::InputParser parser {file};
    for(auto& [key, det] : fDetectors)
    {
        std::string str {GetDetectorTypeStr(key)};
        det->ReadConfiguration(parser.GetBlock(str));
        if(print)
            det->Print();
    }
    // Workaround for Merger: needs access to all the other parameters
    if(fMode == ModeType::EMerge || fMode == ModeType::EGui)
    {
        auto merger {std::dynamic_pointer_cast<ActRoot::MergerDetector>(fDetectors[DetectorType::EMerger])};
        for(auto& [key, det] : fDetectors)
        {
            if(key == DetectorType::EMerger)
                continue;
            merger->SetParameters(det->GetParameters());
        }
    }
}

void ActRoot::DetectorManager::ReadCalibrationsFile(const std::string& file)
{
    ActRoot::InputParser parser {file};
    auto headers {parser.GetBlockHeaders()};
    for(auto& [key, det] : fDetectors)
    {
        std::string str {GetDetectorTypeStr(key)};
        if(std::find(headers.begin(), headers.end(), str) != headers.end())
            det->ReadCalibrations(parser.GetBlock(str));
    }
}

void ActRoot::DetectorManager::Reconfigure()
{
    for(auto& [key, det] : fDetectors)
    {
        det->Reconfigure();
        det->Print();
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

void ActRoot::DetectorManager::InitInput(std::shared_ptr<TTree> input)
{
    if(fMode == ModeType::EReadTPC || fMode == ModeType::EReadSilMod)
        for(auto& [key, det] : fDetectors)
            det->InitInputData(input);
    else if(fMode == ModeType::EFilter)
        for(auto& [key, det] : fDetectors)
            det->InitInputFilter(input);
    else if(fMode == ModeType::EMerge)
        fDetectors[DetectorType::EMerger]->InitInputData(input);
    else if(fMode == ModeType::ECorrect)
        ;
    else
        ;
    // Workaround for EData mode
    if(fMode == ModeType::EReadSilMod)
        fDetectors[DetectorType::EModular]->SetMEvent(fDetectors[DetectorType::ESilicons]->GetMEvent());
}

void ActRoot::DetectorManager::InitOutput(std::shared_ptr<TTree> output)
{
    if(fMode == ModeType::EReadTPC || fMode == ModeType::EReadSilMod)
        for(auto& [key, det] : fDetectors)
            det->InitOutputData(output);
    else if(fMode == ModeType::EFilter)
        for(auto& [key, det] : fDetectors)
            det->InitOutputFilter(output);
    else if(fMode == ModeType::EMerge)
        fDetectors[DetectorType::EMerger]->InitOutputData(output);
    else if(fMode == ModeType::ECorrect)
        ;
    else
        ;
}

void ActRoot::DetectorManager::BuildEvent()
{
    if(fMode == ModeType::EReadTPC || fMode == ModeType::EReadSilMod)
        for(auto& [key, det] : fDetectors)
        {
            det->ClearEventData();
            det->BuildEventData();
        }
    if(fMode == ModeType::EFilter)
        for(auto& [key, det] : fDetectors)
        {
            det->ClearEventFilter();
            det->BuildEventFilter();
        }
    else if(fMode == ModeType::EMerge)
    {
        fDetectors[DetectorType::EMerger]->ClearEventData();
        fDetectors[DetectorType::EMerger]->BuildEventData();
    }
    else if(fMode == ModeType::EGui)
    {
        throw std::runtime_error(
            "DetectorManager::BuildEvent(): EVisual mode not supported: Clone2 is explicited in EventPainter");
    }
    else if(fMode == ModeType::ECorrect)
        ; // fDetectors[]
    else
        ;
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

void ActRoot::DetectorManager::SendWrapperData(ActRoot::InputWrapper* wrap)
{
    // 1-> TPC
    GetDetectorAs<TPCDetector>()->SetInputFilter(wrap->GetTPCData());
    // 2-> Sil
    GetDetectorAs<SilDetector>()->SetInputFilter(wrap->GetSilData());
    // 3-> Modular
    GetDetectorAs<ModularDetector>()->SetInputFilter(wrap->GetModularData());
    // 4-> Merger
    auto merger {GetDetectorAs<MergerDetector>()};
    merger->SetInputData(wrap->GetTPCData());
    merger->SetInputData(wrap->GetSilData());
    merger->SetInputData(wrap->GetModularData());
    merger->SetOutputData(wrap->GetMergerData());
}
