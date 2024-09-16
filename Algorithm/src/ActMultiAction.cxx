#include "ActMultiAction.h"

#include "ActABreakChi2.h"
#include "ActAClean.h"
#include "ActACleanPileUp.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActVCluster.h"

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
}

ActAlgorithm::MultiAction::Ptr ActAlgorithm::MultiAction::ConstructAction(const std::string& actionID)
{
    if(fMap.count(actionID))
        return fMap[actionID]();
    else
        throw std::runtime_error("ActAlgorithm::MultiAction::ConstructAction(): " + actionID +
                                 " is not in fMap -> add it and recompile!");
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
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "multiaction.conf";
    // Parse
    ActRoot::InputParser parser {conf};
    // Get list of headers (orderes by first appearance)
    auto headers {parser.GetBlockHeaders()};
    // And init!
    for(const auto& header : headers)
    {
        fActions.push_back(ConstructAction(header));
        fActions.back()->SetClusterPtr(fAlgo);
        fActions.back()->ReadConfiguration(parser.GetBlock(header));
    }
}

void ActAlgorithm::MultiAction::Run()
{
    for(auto& action : fActions)
        action->Run();
}

void ActAlgorithm::MultiAction::Print() const
{
    for(auto& action : fActions)
        action->Print();
}

void ActAlgorithm::MultiAction::PrintReports() const {}