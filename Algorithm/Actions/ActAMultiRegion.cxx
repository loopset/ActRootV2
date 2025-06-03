#include "ActAMultiRegion.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <memory>

void ActAlgorithm::Actions::MultiRegion::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    std::cout << "Reading MultiRegion config" << '\n';
}

void ActAlgorithm::Actions::MultiRegion::Run()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        std::cout << "Running MultiRegion for cluster #" << it->GetClusterID() << '\n';
    }
}

void ActAlgorithm::Actions::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << RESET << '\n';
}