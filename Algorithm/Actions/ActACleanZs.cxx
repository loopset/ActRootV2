#include "ActACleanZs.h"

#include "ActColors.h"
#include "ActTPCData.h"

void ActAlgorithm::Actions::CleanZs::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("ZDirectionThreshold"))
        fZDirectionThreshold = block->GetDouble("ZDirectionThreshold");
    if(block->CheckTokenExists("MaxSpanInPlane"))
        fMaxSpanInPlane = block->GetDouble("MaxSpanInPlane");
}

void ActAlgorithm::Actions::CleanZs::Run()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Check if the track is vertical
        bool isVertical {std::abs(it->GetLine().GetDirection().Unit().Z()) >= fZDirectionThreshold};
        // 2-> Horizontal range (in X and Y) has to be bellow a threshold
        auto [xmin, xmax] {it->GetXRange()};
        auto [ymin, ymax] {it->GetYRange()};
        bool isNarrow {(xmax - xmin) <= fMaxSpanInPlane && (ymax - ymin) <= fMaxSpanInPlane};
        // Delete if both conditions are acomplished
        if(isVertical && isNarrow)
            it = fTPCData->fClusters.erase(it);
        else
            it++;
    }
}

void ActAlgorithm::Actions::CleanZs::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  ZDirectionThreshold : " << fZDirectionThreshold << '\n';
    std::cout << "  MaxSpanInPlane      : " << fMaxSpanInPlane << '\n';
    std::cout << "······························" << RESET << '\n';
}