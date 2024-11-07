#include "ActACleanDeltas.h"

#include "ActColors.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"

void ActAlgorithm::Actions::CleanDeltas::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("DeltaChi2Threshold"))
        fDeltaChi2Threshold = block->GetDouble("DeltaChi2Threshold");
    if(block->CheckTokenExists("DeltaMaxVoxeles"))
        fDeltaMaxVoxels = block->GetDouble("DeltaMaxVoxeles");
}

void ActAlgorithm::Actions::CleanDeltas::Run()
{
    if(!fIsEnabled)
        return;
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Check whether cluster has a large Chi2
        bool hasLargeChi {it->GetLine().GetChi2() > fDeltaChi2Threshold};
        // 2-> Check if less voxeles than delta electron limits
        bool isSmall {it->GetSizeOfVoxels() <= fDeltaMaxVoxels};
        // 3-> Check if after all there are clusters with Chi2 = -1
        bool isBadFit {it->GetLine().GetChi2() == -1};
        if(fIsVerbose)
        {
            std::cout << BOLDCYAN << "---- CleanDeltas verbose ----" << '\n';
            std::cout << "Chi2 : " << it->GetLine().GetChi2() << '\n';
            std::cout << "SizeVoxels: " << it->GetSizeOfVoxels() << '\n';
            std::cout << "-------------------" << RESET << '\n';
        }

        if(hasLargeChi || isSmall || isBadFit)
            it = fTPCData->fClusters.erase(it);
        else
            it++;
    }
}

void ActAlgorithm::Actions::CleanDeltas::Print() const
{
    // This function just prints the current parameters of the action
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  Chi2Thresh         : " << fDeltaChi2Threshold << '\n';
    std::cout << "  MaxVoxeles         : " << fDeltaMaxVoxels  << '\n';

    std::cout << "······························" << RESET << '\n';
}