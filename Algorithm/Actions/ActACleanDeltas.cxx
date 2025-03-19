#include "ActACleanDeltas.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActTPCData.h"

#include <cstdlib>
#include <ios>
#include <iostream>
#include <iterator>
#include <set>

void ActAlgorithm::Actions::CleanDeltas::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("Chi2Thresh"))
        fChi2Thresh = block->GetDouble("Chi2Thresh");
    if(block->CheckTokenExists("MaxVoxels"))
        fMaxVoxels = block->GetDouble("MaxVoxels");
    if(block->CheckTokenExists("UseExtVoxels"))
        fUseExtVoxels = block->GetBool("UseExtVoxels");
    if(block->CheckTokenExists("SigmaGap"))
        fSigmaGap = block->GetDouble("SigmaGap");
    if(block->CheckTokenExists("CylinderR"))
        fCylinderR = block->GetDouble("CylinderR");
    if(block->CheckTokenExists("UseCylinder"))
        fUseCylinder = block->GetBool("UseCylinder");
}

void ActAlgorithm::Actions::CleanDeltas::Run()
{
    if(!fIsEnabled)
        return;
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        if(fUseExtVoxels)
            if(!it->GetUseExtVoxels())
                it->SetUseExtVoxels(true);
        // 1-> Check whether cluster has a large Chi2
        auto chi2 {it->GetLine().GetChi2()};
        bool hasLargeChi {chi2 > fChi2Thresh};
        // 2-> Check if less voxeles than delta electron limits
        bool isSmall {it->GetSizeOfVoxels() <= fMaxVoxels};
        // 3-> Check if after all there are clusters with Chi2 = -1
        bool isBadFit {chi2 == -1};
        // If it is too small or bad fit, erase it now
        if(isSmall || isBadFit)
        {
            if(fIsVerbose)
            {
                std::cout << BOLDCYAN << "---- CleanDeltas ----" << '\n';
                std::cout << " Small sized cluster: " << it->GetSizeOfVoxels() << '\n';
                std::cout << RESET << '\n';
            }
            it = fTPCData->fClusters.erase(it);
        }
        else if(hasLargeChi)
        {
            // Compute longitudinal and transversal sizes of clusters
            auto [lon, trans] {ComputeLongTransSigmas(&(*it))};
            auto ratio {lon / trans};
            // Determine whether it has a predominat direction
            auto sigmas {it->GetLine().GetSigmas()};
            std::set<float, std::greater<>> set {sigmas.X(), sigmas.Y(), sigmas.Z()};
            // Maximum sigma
            auto max {*set.begin()};
            // Next-to-maximum sigma
            auto ntmax {*std::next(set.begin())};
            // Difference in sigma
            auto diff {std::abs(max - ntmax)};
            if(diff >= fSigmaGap && fUseCylinder) // is linear and use cylinder cleaning
            // if(ratio >= fSigmaGap && fUseCylinder) // is linear and use cylinder cleaning
            {
                auto& voxels {it->GetRefToVoxels()};
                auto oldSize {voxels.size()};
                auto itKeep {std::partition(voxels.begin(), voxels.end(),
                                            [&](const ActRoot::Voxel& voxel)
                                            {
                                                auto pos {voxel.GetPosition()};
                                                pos += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                                                auto dist {it->GetLine().DistanceLineToPoint(pos)};
                                                return dist <= fCylinderR;
                                            })};
                // if enough voxels remain
                auto remain {std::distance(voxels.begin(), itKeep)};
                if((oldSize != remain) && remain >= fAlgo->GetMinPoints())
                {
                    voxels.erase(itKeep, voxels.end());
                    it->ReFit();
                    it->ReFillSets();
                }
                // Recheck chi2 condition
                auto againHasLarcheChi2 {it->GetLine().GetChi2() > fChi2Thresh};
                if(againHasLarcheChi2)
                {
                    if(fIsVerbose)
                    {
                        std::cout << BOLDCYAN << "---- CleanDeltas ----" << '\n';
                        std::cout << " Large chi2 after cylinder" << '\n';
                        std::cout << "  New chi2    : " << it->GetLine().GetChi2() << '\n';
                        std::cout << "  Old chi2    : " << chi2 << '\n';
                        // std::cout << "  Long size   : " << lon << '\n';
                        // std::cout << "  Trans size  : " << trans << '\n';
                        // std::cout << "  Ratio       : " << ratio << '\n';
                        std::cout << "  Diff sigma  : " << diff << '\n';
                        std::cout << "  Diff voxels : " << oldSize - remain << '\n';
                        std::cout << RESET << '\n';
                    }
                    it = fTPCData->fClusters.erase(it);
                }
                else
                {
                    if(fIsVerbose)
                    {
                        std::cout << BOLDCYAN << "---- CleanDeltas ----" << '\n';
                        std::cout << " Recovered after cylinder!" << '\n';
                        std::cout << "  New chi2    : " << it->GetLine().GetChi2() << '\n';
                        std::cout << "  Old chi2    : " << chi2 << '\n';
                        // std::cout << "  Long size   : " << lon << '\n';
                        // std::cout << "  Trans size  : " << trans << '\n';
                        // std::cout << "  Ratio       : " << ratio << '\n';
                        std::cout << "  Diff sigma  : " << diff << '\n';
                        std::cout << "  Diff voxels : " << oldSize - remain << '\n';
                        std::cout << RESET << '\n';
                    }
                    it++;
                }
            }
            else
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDCYAN << "---- CleanDeltas ----" << '\n';
                    std::cout << " Large chi2: " << it->GetLine().GetChi2() << '\n';
                    if(fUseCylinder)
                    {
                        // std::cout << "  Long size   : " << lon << '\n';
                        // std::cout << "  Trans size  : " << trans << '\n';
                        // std::cout << "  Ratio       : " << ratio << '\n';
                        std::cout << "   Diff sigma : " << diff << '\n';
                    }
                    std::cout << RESET << '\n';
                }
                it = fTPCData->fClusters.erase(it);
            }
        }
        else
            it++;
    }
}

std::pair<double, double> ActAlgorithm::Actions::CleanDeltas::ComputeLongTransSigmas(ActRoot::Cluster* cluster)
{
    // Longitudinal is extracted from sorting in distance
    cluster->SortAlongDir();
    auto lon {(cluster->GetVoxels().front().GetPosition() - cluster->GetVoxels().back().GetPosition()).R()};
    // Transversal is the maximum perp dist to fit
    double trans {-1111};
    for(const auto& v : cluster->GetVoxels())
    {
        auto dist {cluster->GetLine().DistanceLineToPoint(v.GetPosition())};
        if(dist > trans)
            trans = dist;
    }
    return {lon, trans};
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
    std::cout << "  Chi2Thresh   : " << fChi2Thresh << '\n';
    std::cout << "  MaxVoxels    : " << fMaxVoxels << '\n';
    std::cout << "  UseExtVoxels ? " << std::boolalpha << fUseExtVoxels << '\n';
    std::cout << "  UseCylinder  ? " << std::boolalpha << fUseCylinder << '\n';
    if(fUseCylinder)
    {
        std::cout << "  SigmaGap     : " << fSigmaGap << '\n';
        std::cout << "  CylinderR    : " << fCylinderR << '\n';
    }

    std::cout << "······························" << RESET << '\n';
}
