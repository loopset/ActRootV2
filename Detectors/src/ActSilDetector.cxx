#include "ActSilDetector.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActSilData.h"
#include "ActTPCLegacyData.h"

#include "TRegexp.h"
#include "TString.h"

#include <iostream>
#include <string>
#include <vector>

ActRoot::SilDetector::~SilDetector()
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

void ActRoot::SilDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    // Read layer setup
    auto layers {config->GetStringVector("Layers")};
    // Read action file
    auto legacy {config->GetStringVector("Names")};
    auto file {config->GetString("Actions")};
    fPars.ReadActions(layers, legacy, file);
    // fPars.Print();
}

void ActRoot::SilDetector::ReadCalibrations(std::shared_ptr<InputBlock> config)
{
    // Set is enabled or not
    bool enabled {true};
    if(config->CheckTokenExists("IsEnabled", true))
        enabled = config->GetBool("IsEnabled");
    fCalMan->SetIsEnabled(enabled);
    if(!fCalMan->GetIsEnabled())
    {
        std::cout << BOLDCYAN << "CalibrationManager::fIsEnabled == false" << RESET << '\n';
        return;
    }
    auto files {config->GetStringVector("Paths")};
    for(auto& file : files)
        fCalMan->ReadCalibration(file);
}

void ActRoot::SilDetector::InitInputData(std::shared_ptr<TTree> tree)
{
    if(fMEvent)
        delete fMEvent;
    fMEvent = new MEventReduced;
    tree->SetBranchAddress("data", &fMEvent);
    // Set to delete on destructor
    fDelMEvent = true;
}

void ActRoot::SilDetector::InitOutputData(std::shared_ptr<TTree> tree)
{
    if(fData)
        delete fData;
    fData = new SilData;
    tree->Branch("SilData", &fData);
    // Set to delete on destructor
    fDelData = true;
}

void ActRoot::SilDetector::InitInputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::SilDetector::InitOutputFilter(std::shared_ptr<TTree> tree) {}

void ActRoot::SilDetector::BuildEventData(int run, int entry)
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
                auto [layer, sil] = fPars.GetSilIndex(vxi);
                if(sil == -1)
                    continue;
                // Get raw data
                float raw {coas.peakheight[hit]};
                // Check threshold
                std::string threshKey {"Sil_" + layer + "_" + sil + "_P"};
                if(!fCalMan->ApplyThreshold(threshKey, raw, 3))
                    continue;
                // Write silicon number
                fData->fSiN[layer].push_back(sil);
                // Calibrate
                std::string calKey {"Sil_" + layer + "_" + sil + "_E"};
                float cal {static_cast<float>(fCalMan->ApplyCalibration(calKey, raw))};
                fData->fSiE[layer].push_back(cal);
                // std::cout<<"Raw sil = "<<raw<<" |"<<'\n';
                // std::cout<<"Cal sil = "<<cal<<" |"<<'\n';
            }
        }
    }
}

void ActRoot::SilDetector::BuildEventFilter() {}

void ActRoot::SilDetector::ClearEventData()
{
    fData->Clear();
}

void ActRoot::SilDetector::ClearEventFilter() {}

void ActRoot::SilDetector::Print() const {}

void ActRoot::SilDetector::PrintReports() const {}

void ActRoot::SilDetector::Reconfigure() {}
