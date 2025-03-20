#include "ActASplit.h"

#include "ActTPCData.h"

#include <memory>

void ActAlgorithm::Actions::Split::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> conf)
{
    fIsEnabled = conf->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
}

void ActAlgorithm::Actions::Split::Run()
{
    auto& fClusters {fTPCData->fClusters};

}
