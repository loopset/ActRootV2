#include "ActACleanPileUp.h"

#include "ActColors.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"

void ActAlgorithm::Actions::CleanPileUp::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("XPercent"))
        fXPercent = block->GetDouble("XPercent");
    if(block->CheckTokenExists("LowerZ"))
        fLowerZ = block->GetDouble("LowerZ");
    if(block->CheckTokenExists("UpperZ"))
        fUpperZ = block->GetDouble("UpperZ");
}

void ActAlgorithm::Actions::CleanPileUp::Run()
{
    if(!fIsEnabled)
        return;
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Eval condition of X range
        // Tracks X range must span a
        // minimum percent of ACTAR's length
        auto [xmin, xmax] {it->GetXRange()};
        bool spansActarX {(xmax - xmin) > fXPercent * fTPCPars->GetNPADSX()};

        // 2-> Track must be FULLY contained in the outter region
        // defined by lowerZ and upperZ (the region to be masked)
        auto [zmin, zmax] {it->GetZRange()};
        zmin *= fTPCPars->GetREBINZ();
        zmax *= fTPCPars->GetREBINZ();
        bool isInBeamZMin {fLowerZ <= zmin && zmin <= fUpperZ};
        bool isInBeamZMax {fLowerZ <= zmax && zmax <= fUpperZ};
        bool isInBeamZ {isInBeamZMin || isInBeamZMax};
        if(spansActarX && !isInBeamZ)
        {
            if(fIsVerbose)
            {
                std::cout << BOLDCYAN << "---- " << GetActionID() << " verbose ----" << '\n';
                std::cout << "-> Deleted cluster #" << it->GetClusterID() << " with :" << '\n';
                std::cout << "   zmin    : " << zmin << '\n';
                std::cout << "   zmax    : " << zmax << '\n';
                std::cout << "   xlength : " << (xmax - xmin) << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
            it = fTPCData->fClusters.erase(it);
        }
        else
            it++;
    }
}

void ActAlgorithm::Actions::CleanPileUp::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  XPercent : " << fXPercent << '\n';
    std::cout << "  LowerZ   : " << fLowerZ << '\n';
    std::cout << "  UpperZ   : " << fUpperZ << '\n';
    std::cout << "······························" << RESET << '\n';
}
