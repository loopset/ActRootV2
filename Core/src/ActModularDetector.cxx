#include "ActModularDetector.h"

#include "ActInputParser.h"
#include "ActModularData.h"
#include "ActTPCLegacyData.h"

#include "TString.h"

#include <fstream>
#include <iostream>
#include <string>

std::string ActRoot::ModularParameters::GetName(int vxi)
{
    auto where {fVXI.find(vxi) != fVXI.end()};
    if(where)
        return fVXI[vxi];
    else
        return "";
}

void ActRoot::ModularParameters::ReadActions(const std::vector<std::string>& names, const std::string& file)
{
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("No Action file for ModularParameters");
    TString key {};
    int vxi {};
    int aux0 {};
    int aux1 {};
    while(streamer >> key >> vxi >> aux0 >> aux1)
    {
        for(int i = 0; i < names.size(); i++)
        {
            if(key == names[i])
            {
                fVXI[vxi] = names[i];
                break;
            }
        }
    }
}

void ActRoot::ModularParameters::Print() const
{
    std::cout << "==== ModularParameters ====" << '\n';
    for(const auto& [key, val] : fVXI)
    {
        std::cout << "-- VXI: " << key << " contains Modular " << val << '\n';
    }
    std::cout << "=======================" << '\n';
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

ActRoot::ModularDetector::~ModularDetector()
{
    if(fDelMEvent)
    {
        delete fMEvent;
        fMEvent = nullptr;
    }
    if(fDelData)
    {
        delete fData;
        fData = nullptr;
    }
}

void ActRoot::ModularDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    // Read action file
    auto names {config->GetStringVector("Names")};
    auto file {config->GetString("Actions")};
    fPars.ReadActions(names, file);
    // fPars.Print();
}

void ActRoot::ModularDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    ; // Do not calibrate Modular Leaves so far
}

void ActRoot::ModularDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    // if(fMEvent)
    //     delete fMEvent;
    // fMEvent = new MEventReduced;
    // tree->SetBranchAddress("data", &fMEvent);
}

void ActRoot::ModularDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new ModularData;
    tree->Branch("ModularData", &fData);
    // Set to delete on destructor
    fDelData = true;
}

void ActRoot::ModularDetector::InitInputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::ModularDetector::InitOutputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::ModularDetector::BuildEventData(int run, int entry)
{
    for(auto& coas : fMEvent->CoboAsad)
    {
        // locate channel!
        int co {coas.globalchannelid >> 11};
        if(co == 31)
        {
            for(int hit = 0, size = coas.peakheight.size(); hit < size; hit++)
            {
                auto vxi {coas.peaktime[hit]};
                auto leaf {fPars.GetName(vxi)};
                if(leaf.length() == 0)
                    continue;
                // Write
                fData->fLeaves[leaf] = coas.peakheight[hit];
            }
        }
    }
}

void ActRoot::ModularDetector::BuildEventFilter() {}

void ActRoot::ModularDetector::ClearEventData()
{
    fData->Clear();
}

void ActRoot::ModularDetector::ClearEventFilter() {}

void ActRoot::ModularDetector::Print() const {}

void ActRoot::ModularDetector::PrintReports() const {}

void ActRoot::ModularDetector::Reconfigure() {}
