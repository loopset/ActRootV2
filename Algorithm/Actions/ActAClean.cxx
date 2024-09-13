#include "ActAClean.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <memory>

void ActAlgorithm::Actions::Clean::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    std::cout << "Reading Clean config" << '\n';
}

void ActAlgorithm::Actions::Clean::Run()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        std::cout << "Running clean for cluster #" << it->GetClusterID() << '\n';
    }
}

void ActAlgorithm::Actions::Clean::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << RESET << '\n';
}
