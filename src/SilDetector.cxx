#include "SilDetector.h"

#include "Buttons.h"
#include "InputParser.h"
#include "SilData.h"
#include "TString.h"
#include "TRegexp.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<std::string> ActRoot::SilParameters::GetKeys() const
{
    std::vector<std::string> ret;
    for(const auto& [key, _] : fSizes)
        ret.push_back(key);
    return ret;
}

void ActRoot::SilParameters::Print() const
{
    std::cout<<"==== SilParameters ===="<<'\n';
    for(const auto& [key, vals] : fVXI)
        std::cout<<"-- VXI: "<<key<<" contains Sil "<<vals.first<<" at idx "<<vals.second<<'\n';
    std::cout<<"======================="<<'\n';
}

void ActRoot::SilParameters::ReadActions(const std::vector<std::string> &layers, const std::vector<std::string> &names, const std::string &file)
{
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("No Action file for SilParameters");
    TString key {}; int vxi {}; int aux0 {}; int aux1 {};
    while (streamer >> key >> vxi >> aux0 >> aux1)
    {
        for(int i = 0; i < names.size(); i++)
        {
            auto index {key.Index(names[i].c_str())};
            if(index != -1)//-1 implies not found
            {
                //and now get index
                auto length {names[i].length()};
                fVXI[vxi] = {layers[i], std::stoi(std::string(key).substr(index + length))};
                break;
            }
        }
    }
}

void ActRoot::SilDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    //Read layer setup
    auto layers {config->GetStringVector("Layers")};
    //Read action file
    auto legacy {config->GetStringVector("Names")};
    auto file {config->GetString("Actions")};
    fPars.ReadActions(layers, legacy, file);
    fPars.Print();
}

void ActRoot::SilDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    auto files {config->GetTokens()};
    for(auto& file : files)
        std::cout<<"Silicon calibration file = "<<file<<'\n';
}

void ActRoot::SilDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    
}

void ActRoot::SilDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new SilData;
}

void ActRoot::SilDetector::BuildEventData()
{
    
}
void ActRoot::SilDetector::ClearEventData()
{
    fData->Clear();
}
