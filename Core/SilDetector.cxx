#include "SilDetector.h"

#include "CalibrationManager.h"
#include "InputParser.h"
#include "SilData.h"
#include "TPCLegacyData.h"
#include "TString.h"
#include "TRegexp.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
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

std::pair<std::string, int> ActRoot::SilParameters::GetSilIndex(int vxi)
{
    auto where {fVXI.find(vxi) != fVXI.end()};
    if(where)
        return fVXI[vxi];
    else
        return {"", -1};
}

void ActRoot::SilDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    //Read layer setup
    auto layers {config->GetStringVector("Layers")};
    //Read action file
    auto legacy {config->GetStringVector("Names")};
    auto file {config->GetString("Actions")};
    fPars.ReadActions(layers, legacy, file);
    //fPars.Print();
}

void ActRoot::SilDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    auto files {config->GetTokens()};
    for(auto& file : files)
        CalibrationManager::Get()->ReadCalibration(file);
}

void ActRoot::SilDetector::InitInputRawData(std::shared_ptr<TTree> tree, int run)
{
    ;
}

void ActRoot::SilDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new SilData;
    tree->Branch("SilData", &fData);
}

void ActRoot::SilDetector::BuildEventData()
{
    for(auto& coas : fMEvent->CoboAsad)
    {
        //locate channel!
        int co {coas.globalchannelid >> 11};
        if(co == 31)
        {
            for(int hit = 0, size = coas.peakheight.size(); hit < size; hit++)
            {
                auto vxi {coas.peaktime[hit]};
                auto [layer, sil] = fPars.GetSilIndex(vxi);
                if(sil == -1)
                    continue;
                //Get raw data
                float raw {coas.peakheight[hit]};
                //Check threshold
                std::string threshKey {"Sil_" + layer + "_" + sil + "_P"};
                if(!CalibrationManager::Get()->ApplyThreshold(threshKey, raw, 3))
                    continue;
                //Write silicon number
                fData->fSiN[layer].push_back(sil);                
                //Calibrate
                std::string calKey {"Sil_" + layer + "_" + sil + "_E"};
                float cal {static_cast<float>(CalibrationManager::Get()->ApplyCalibration(calKey, raw))};
                fData->fSiE[layer].push_back(cal);
            }
        }
    }
}

void ActRoot::SilDetector::ClearEventData()
{
    fData->Clear();
}
