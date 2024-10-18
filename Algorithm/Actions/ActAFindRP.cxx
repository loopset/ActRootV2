#include "ActAFindRP.h"

#include "ActAlgoFuncs.h"
#include "ActColors.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActVAction.h"
#include "Math/Point3D.h"

#include <utility>
#include <vector>

void ActAlgorithm::Actions::FindRP::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("BeamLikeParallelF"))
        fBeamLikeParallelF = block->GetDouble("BeamLikeParallelF");
    if(block->CheckTokenExists("BeamLikeXMinThresh"))
        fBeamLikeXMinThresh = block->GetDouble("BeamLikeXMinThresh");
}

void ActAlgorithm::Actions::FindRP::Run()
{
    if(!fIsEnabled)
        return;
    DetermineBeamLikes();
}

void ActAlgorithm::Actions::FindRP::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  XPercent : " << 2 << '\n';

    std::cout << "······························" << RESET << '\n';
}

void ActAlgorithm::Actions::FindRP::DetermineBeamLikes()
{
    int nBeam {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Check if xmin is bellow threshold
        auto [xmin, xmax] {it->GetXRange()};
        bool isInEntrance {xmin <= fBeamLikeXMinThresh};
        // 2-> Check if track is parallel
        auto uDir {it->GetLine().GetDirection().Unit()};
        bool isAlongX {std::abs(uDir.X()) >= fBeamLikeParallelF};
        // 3-> If both conditions true, mark as beam-like
        if(isInEntrance && isAlongX)
        {
            it->SetBeamLike(true);
            nBeam++;
        }
    }
    // Print
    if(fIsVerbose)
    {
        std::cout << BOLDYELLOW << "---- Beam-Like ID verbose ----" << '\n';
        std::cout << "-> N beam clusters  : " << nBeam << '\n';
        std::cout << "-> N total clusters : " << fTPCData->fClusters.size() << '\n';
        std::cout << "-------------------------" << RESET << '\n';
    }
}

typedef std::vector<std::pair<ActAlgorithm::VAction::XYZPoint, std::pair<int, int>>> RPVector; // includes RP and pair of indexes as values
void ActAlgorithm::Actions::FindRP::FindPreliminaryRP()
{
    // If there is only one track, set to delete
    if(fTPCData->fClusters.size() == 0)
        fTPCData->fClusters.begin()->SetToDelete(true);

    // Declare vector of RPs
    RPVector rps;
    // Run
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
    {
        // Get clusters as iterators
        auto out {fTPCData->fClusters.begin() + i};
        for(int j = i + 1; j < size; j++) // only get diferent clusters
        {
            auto in {fTPCData->fClusters.begin() + j};

            // If both are BL, continue
            if(out->GetIsBeamLike() && in->GetIsBeamLike())
                continue;

            // Compute min distance between both lines
            auto [pA, pB, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                               in->GetLine().GetPoint(), in->GetLine().GetDirection())};

            if(dist < 0) // just in case they are parellel
                continue;
            // Build RP as mean of A and B
            XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};

            // Check that all points are valid
            bool checkA {IsRPValid(pA, GetTPCParameters())};
            bool checkB {IsRPValid(pB, GetTPCParameters())};
            //bool checkRP {IsRPValid(rp, GetTPCParameters())};
            //auto checkPoints {checkA && checkB && checkRP};
            // Check distance is bellow threshold
            //bool checkDist {dist <= fRPDistThresh};
            //if(checkPoints && checkDist)
            //    rps.push_back({rp, {i, j}});
            //else
            //    continue;
            //test
        }
    }
}

typedef std::pair<ActAlgorithm::VAction::XYZPoint, std::set<int>> RPCluster;
std::vector<RPCluster> ActAlgorithm::Actions::FindRP::ClusterAndSortRPs(RPVector)
{

}
