#include "ActMultiAction.h"

#include "ActABreakChi2.h"
#include "ActAClean.h"
#include "ActACleanBadFits.h"
#include "ActACleanDeltas.h"
#include "ActACleanPileUp.h"
#include "ActACleanZs.h"
#include "ActAFindRP.h"
#include "ActAMerge.h"
#include "ActASplit.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActVCluster.h"

#include <dlfcn.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

ActAlgorithm::MultiAction::MultiAction()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
    // Map actions
    fMap["Clean"] = &RegisterAction<Actions::Clean>;
    fMap["BreakChi2"] = &RegisterAction<Actions::BreakChi2>;
    fMap["CleanPileUp"] = &RegisterAction<Actions::CleanPileUp>;
    fMap["Merge"] = &RegisterAction<Actions::Merge>;
    fMap["CleanDeltas"] = &RegisterAction<Actions::CleanDeltas>;
    fMap["CleanZs"] = &RegisterAction<Actions::CleanZs>;
    fMap["CleanBadFits"] = &RegisterAction<Actions::CleanBadFits>;
    fMap["FindRP"] = &RegisterAction<Actions::FindRP>;
    fMap["Split"] = &RegisterAction<Actions::Split>;
}

ActAlgorithm::MultiAction::Ptr ActAlgorithm::MultiAction::ConstructAction(const std::string& actionID)
{
    if(fMap.count(actionID))
        return fMap[actionID]();
    else
        throw std::runtime_error("ActAlgorithm::MultiAction::ConstructAction(): " + actionID +
                                 " is not in fMap -> register it and recompile!");
}

void ActAlgorithm::MultiAction::SetTPCParameters(ActRoot::TPCParameters* pars)
{
    fTPC = pars;
    for(auto& action : fActions)
        action->SetTPCParameters(fTPC);
}

void ActAlgorithm::MultiAction::SetTPCData(ActRoot::TPCData* data)
{
    fData = data;
    // And overrided because we need to do it for all actions
    for(auto& action : fActions)
        action->SetTPCData(fData);
}

void ActAlgorithm::MultiAction::SetClusterPtr(std::shared_ptr<VCluster> ptr)
{
    fAlgo = ptr;
    for(auto& action : fActions)
        action->SetClusterPtr(fAlgo);
}

void ActAlgorithm::MultiAction::ReadConfiguration()
{
    // Clear always in case we re-read configuration
    fActions.clear();
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "multiaction.conf";
    // Parse
    ActRoot::InputParser parser {conf};
    // Get list of headers (orderes by first appearance)
    auto headers {parser.GetBlockHeaders()};
    // And init!
    for(const auto& header : headers)
    {
        // User action
        if(header == "User")
        {
            LoadUserAction(parser.GetBlock(header));
            // If there use UserConfig header, pass it to the UserAction
            if(std::find(headers.begin(), headers.end(), "UserConfig") != headers.end())
                fActions.back()->ReadConfiguration(parser.GetBlock("UserConfig"));
        }
        // UserConfig
        else if(header == "UserConfig")
            continue; // UserConfig is passed to UserAction in previous if condition
        else
        {
            fActions.push_back(ConstructAction(header));
            fActions.back()->ReadConfiguration(parser.GetBlock(header));
        }
        // Set ClusterPtr for all. Data and Parameters are managed by TPCDetector
        fActions.back()->SetClusterPtr(fAlgo);
    }
}

void ActAlgorithm::MultiAction::Run()
{
    fTimer.Start(false);
    for(auto& action : fActions)
    {
        action->Run();
        ResetClusterID();
    }
    fTimer.Stop();
}

void ActAlgorithm::MultiAction::Print() const
{
    for(auto& action : fActions)
        action->Print();
}

void ActAlgorithm::MultiAction::PrintReports() const
{
    std::cout << BOLDYELLOW << "···· MultiAction time report ····" << '\n';
    fTimer.Print();
    std::cout << "······························" << RESET << '\n';
}

void ActAlgorithm::MultiAction::ResetClusterID()
{
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
        fData->fClusters[i].SetClusterID(i);
}

void ActAlgorithm::MultiAction::LoadUserAction(std::shared_ptr<ActRoot::InputBlock> block)
{
    // Get name
    auto name {block->GetString("Name")};
    // Get path from block
    auto path {block->GetString("Path")};
    // Build path + name
    auto file {ActRoot::Options::GetInstance()->GetProjectDir() + path + name};
    // If it doesnt exist, throw error
    if(!std::filesystem::exists(file))
        throw std::runtime_error("MA::LoadUserAction(): cannot locate library");

    // Load the .so
    auto library {dlopen(file.c_str(), RTLD_NOW | RTLD_GLOBAL)};
    if(!library)
        throw std::runtime_error("MA::LoadUserAction: failed to execute dlopen()");
    auto creator {reinterpret_cast<VAction* (*)()>(dlsym(library, "CreateUserAction"))};
    if(!creator)
    {
        dlclose(library);
        throw std::runtime_error("MA::LoadUserAction(): cannot open CreateUserAction() in library " + name);
    }
    // If success, call constructor in extern "C" function
    fActions.push_back(std::shared_ptr<VAction>(creator()));
}
