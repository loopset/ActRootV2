#include "ActAMerge.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <ios>
#include <memory>

void ActAlgorithm::Actions::Merge::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("DistThresh"))
        fDistThresh = block->GetDouble("DistThresh");
    if(block->CheckTokenExists("MinParallelFactor"))
        fMinParallelFactor = block->GetDouble("MinParallelFactor");
    if(block->CheckTokenExists("Chi2Factor"))
        fChi2Factor = block->GetDouble("Chi2Factor");
}

void ActAlgorithm::Actions::Merge::Run()
{
    if(!fIsEnabled)
        return;

    auto& clusters {fTPCData->fClusters};
    // Sort cluster by increasing size
    std::sort(clusters.begin(), clusters.end(),
              [](ActRoot::Cluster& l, ActRoot::Cluster& r) { return l.GetSizeOfVoxels() < r.GetSizeOfVoxels(); });
    // Verbose
    if(fIsVerbose)
        std::cout << BOLDYELLOW << "---- MergeSimilarClusters ----" << '\n';
    // Set of index to delete
    std::set<int, std::greater<int>> toDelete {};
    // Run for each cluster pair
    for(size_t i = 0; i < clusters.size(); i++)
    {
        // Get the iterator for clusters
        auto iit {clusters.begin() + i};
        for(size_t j = 0; j < clusters.size(); j++)
        {
            bool isIinSet {toDelete.find(i) != toDelete.end()};
            bool isJinSet {toDelete.find(j) != toDelete.end()};

            if(i == j || isIinSet || isJinSet)
                continue;

            // Get inner iterator
            auto jit {clusters.begin() + j};

            // If any of them is set not to merge, do not do that :)
            if(!iit->GetToMerge() || !jit->GetToMerge())
            {
                if(fIsVerbose)
                    std::cout << "   i or j are set not to merge" << '\n';
                continue;
            }

            // 1-> Compute the distance between the gravity point and the fit lines of clusters
            auto gravI {iit->GetLine().GetPoint()};
            auto gravJ {jit->GetLine().GetPoint()};
            auto distI {iit->GetLine().DistanceLineToPoint(gravJ)};
            auto distJ {jit->GetLine().DistanceLineToPoint(gravI)};
            auto dist {std::max(distI, distJ)};
            // Check if dist is below threshold
            bool isBelowThresh {dist <= fDistThresh};

            // 2-> Compare by paralelity
            auto dirI {iit->GetLine().GetDirection().Unit()};
            auto dirJ {jit->GetLine().GetDirection().Unit()};
            bool areParallel {std::abs(dirJ.Dot(dirI)) > fMinParallelFactor};

            // 3-> Check if fit improves
            if(isBelowThresh && areParallel)
            {
                if(fIsVerbose)
                {
                    std::cout << "-> <i,j> : <" << i << "," << j << ">" << '\n';
                    std::cout << "   dist : " << dist << " < thresh ? " << std::boolalpha << isBelowThresh << '\n';
                    std::cout << "   are parallel ? " << std::boolalpha << areParallel << '\n';
                }
                // Sum voxels from both cluster
                std::vector<ActRoot::Voxel> sumVoxels;
                sumVoxels.reserve(iit->GetPtrToVoxels()->size() + jit->GetPtrToVoxels()->size());
                // Add i
                sumVoxels.insert(sumVoxels.end(), iit->GetPtrToVoxels()->begin(), iit->GetPtrToVoxels()->end());
                // Add j
                sumVoxels.insert(sumVoxels.end(), jit->GetPtrToVoxels()->begin(), jit->GetPtrToVoxels()->end());
                // And get fit of summed voxels
                ActPhysics::Line sumLine {};
                sumLine.FitVoxels(sumVoxels);
                // Compare Chi2
                auto newChi2 {sumLine.GetChi2()};
                // oldChi2 is obtained by quadratic sum of chi2s
                auto oldChi2 {std::sqrt(std::pow(iit->GetLine().GetChi2(), 2) + std::pow(jit->GetLine().GetChi2(), 2))};
                // auto oldChi2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
                bool improvesFit {newChi2 < fChi2Factor * oldChi2};

                if(fIsVerbose)
                {
                    std::cout << "   newChi2 < f * oldChi2 : " << newChi2 << " < " << fChi2Factor * oldChi2 << '\n';
                    std::cout << "   improvesFit ? " << std::boolalpha << improvesFit << '\n';
                }

                // Check whether fit is improved: reduces chi2
                if(improvesFit)
                {
                    // Save in bigger cluster
                    // and delete smaller one
                    std::vector<ActRoot::Cluster>::iterator itSave, itDel;
                    int idxDel;
                    if(iit->GetSizeOfVoxels() > jit->GetSizeOfVoxels())
                    {
                        itSave = iit;
                        itDel = jit;
                        idxDel = j;
                    }
                    else
                    {
                        itSave = jit;
                        itDel = iit;
                        idxDel = i;
                    }
                    auto& saveVoxels {itSave->GetRefToVoxels()};
                    auto& delVoxels {itDel->GetRefToVoxels()};
                    saveVoxels.insert(saveVoxels.end(), std::make_move_iterator(delVoxels.begin()),
                                      std::make_move_iterator(delVoxels.end()));
                    // Refit and recompute ranges
                    itSave->ReFit();
                    itSave->ReFillSets();
                    // Mark to delete afterwards!
                    toDelete.insert(idxDel);
                    // Verbose info
                    if(fIsVerbose)
                    {
                        std::cout << "   => merge cluster #" << itDel->GetClusterID()
                                  << " and size : " << itDel->GetSizeOfVoxels() << '\n';
                        std::cout << "      with cluster #" << itSave->GetClusterID()
                                  << " and size : " << itSave->GetSizeOfVoxels() << '\n';
                    }
                }
            }
        }
    }
    // Delete clusters
    for(const auto& idx : toDelete) // toDelete is sorted in greater order
        clusters.erase(clusters.begin() + idx);
    if(fIsVerbose)
        std::cout << "------------------------------" << RESET << '\n';
}
void ActAlgorithm::Actions::Merge::Print() const
{
    // This function just prints the current parameters of the action
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  DistThresh         : " << fDistThresh << '\n';
    std::cout << "  MinParalellFactor  : " << fMinParallelFactor << '\n';
    std::cout << "  Chi2Factor         : " << fChi2Factor << RESET << '\n';
}
