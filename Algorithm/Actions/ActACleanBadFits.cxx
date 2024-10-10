#include "ActACleanBadFits.h"

#include "ActColors.h"
#include "ActTPCData.h"

void ActAlgorithm::Actions::CleanBadFits::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
}

void ActAlgorithm::Actions::CleanBadFits::Run()
{
    if(!fIsEnabled)
        return;
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        bool isBadFit {std::isnan(it->GetLine().GetDirection().Z())};
        if(isBadFit)
            it = fTPCData->fClusters.erase(it);
        else
            it++;
    }
}

void ActAlgorithm::Actions::CleanBadFits::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  InEnabled : " << fIsEnabled << '\n';
    std::cout << "······························" << RESET << '\n';
}