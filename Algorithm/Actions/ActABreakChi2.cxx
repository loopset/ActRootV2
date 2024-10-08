#include "ActABreakChi2.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"

#include <ios>
#include <iostream>
#include <memory>

void ActAlgorithm::Actions::BreakChi2::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    // Always read first the IsEnabled parameter (inherited from VAction)
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    // And then proceed reading the rest of parameters
    if(block->CheckTokenExists("Chi2Thresh"))
        fChi2Thresh = block->GetDouble("Chi2Thresh");
    if(block->CheckTokenExists("MinXRange"))
        fMinXRange = block->GetDouble("MinXRange");
    if(block->CheckTokenExists("LengthXToBreak"))
        fLengthXToBreak = block->GetDouble("LengthXToBreak");
    if(block->CheckTokenExists("BeamWindowY"))
        fBeamWindowY = block->GetDouble("BeamWindowY");
    if(block->CheckTokenExists("BeamWindowZ"))
        fBeamWindowZ = block->GetDouble("BeamWindowZ");
    if(block->CheckTokenExists("DoClusterNotBeam"))
        fDoClusterNotBeam = block->GetBool("DoClusterNotBeam");
    // Inner part to break multitrack events
    if(block->CheckTokenExists("DoBreakMultiTracks"))
        fDoBreakMultiTracks = block->GetBool("DoBreakMultiTracks");
    if(block->CheckTokenExists("TrackChi2Thresh"))
        fTrackChi2Thresh = block->GetDouble("TrackChi2Thresh");
    if(block->CheckTokenExists("BeamWindowScale"))
        fBeamWindowScale = block->GetDouble("BeamWindowScale");
}

void ActAlgorithm::Actions::BreakChi2::Run()
{
    if(!fIsEnabled)
        return;
    std::vector<ActRoot::Cluster> toAppend {};
    // Pointer to cluster vector
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Check whether we meet conditions to execute this
        bool isBadFit {it->GetLine().GetChi2() > fChi2Thresh};
        auto [xmin, xmax] {it->GetXRange()};
        bool hasMinXExtent {(xmax - xmin) > fMinXRange};
        auto isBreakable {isBadFit && hasMinXExtent};
        if(!isBreakable)
        {
            it++;
            continue;
        }
        else
        {
            // 2-> Compute gravity point in the range
            // [xmin, xmin + fLengthXToBreak]
            auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};

            // 3->Modify original cluster: move non-beam voxels outside to
            //  be clusterized independently
            auto& refToVoxels {it->GetRefToVoxels()};
            auto initSize {refToVoxels.size()};
            auto toMove {std::partition(refToVoxels.begin(), refToVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            auto pos {voxel.GetPosition()};
                                            pos += XYZVectorF {0.5, 0.5, 0.5};
                                            // must move to centre of bin aka voxel
                                            return ManualIsInBeam(pos, gravity);
                                        })};
            // Create vector to move to
            std::vector<ActRoot::Voxel> notBeam {};
            notBeam.insert(notBeam.end(), std::make_move_iterator(toMove), std::make_move_iterator(refToVoxels.end()));
            refToVoxels.erase(toMove, refToVoxels.end());

            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "---- " << GetActionID() << " verbose for ID : " << it->GetClusterID()
                          << " ----" << '\n';
                std::cout << "Chi2          = " << it->GetLine().GetChi2() << '\n';
                std::cout << "isBadFit      = " << std::boolalpha << isBadFit << '\n';
                std::cout << "XExtent       = [" << xmin << ", " << xmax << "]" << '\n';
                std::cout << "hasMinXExtent ? " << std::boolalpha << hasMinXExtent << '\n';
                std::cout << "Gravity        = " << gravity << '\n';
                std::cout << "Init clusters  = " << fTPCData->fClusters.size() << '\n';
                std::cout << "Init size beam = " << initSize << '\n';
                std::cout << "Remaining beam = " << refToVoxels.size() << '\n';
                std::cout << "Not beam       = " << notBeam.size() << RESET << '\n';
            }
            // Check if it satisfies minimum voxel requirement to be clusterized again
            if(refToVoxels.size() <= fAlgo->GetMinPoints())
            {
                // Delete remaining beam-like cluster
                it = fTPCData->fClusters.erase(it);
            }
            else
            {
                // Reprocess
                it->ReFit();
                // Reset ranges
                it->ReFillSets();
                // Set it is beam-like
                it->SetBeamLike(true);
                // And of course, add to iterator
                it++;
            }
            // 4-> Run cluster algorithm again in the not beam voxels
            std::vector<ActRoot::Cluster> newClusters;
            std::vector<ActRoot::Voxel> noise;
            if(fDoClusterNotBeam)
                std::tie(newClusters, noise) = fAlgo->Run(notBeam);
            // Set flag accordingly
            for(auto& cl : newClusters)
                cl.SetIsBreakBeam(true);
            // Move to vector
            toAppend.insert(toAppend.end(), std::make_move_iterator(newClusters.begin()),
                            std::make_move_iterator(newClusters.end()));
            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "New clusters   = " << newClusters.size() << '\n';
                std::cout << "-------------------" << RESET << '\n';
            }
        }
    }
    // Append clusters to original TPCData
    fTPCData->fClusters.insert(fTPCData->fClusters.end(), std::make_move_iterator(toAppend.begin()),
                               std::make_move_iterator(toAppend.end()));
    if(fDoBreakMultiTracks)
        BreakMultiTrack();
}

void ActAlgorithm::Actions::BreakChi2::BreakMultiTrack()
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 1-> Check whether cluster needs breaking
        bool isBadFit {it->GetLine().GetChi2() > fTrackChi2Thresh};
        if(!isBadFit)
        {
            it++;
            continue;
        }
        else
        {
            // Get ref to voxels
            auto& refVoxels {it->GetRefToVoxels()};
            // Identify GP and window scale
            XYZPointF gravity {};
            double scale {};
            if(it->GetIsBreakBeam()) // comes after running the first part of the action
            {
                for(auto in = fTPCData->fClusters.begin(); in != fTPCData->fClusters.end(); in++)
                    if(in->GetIsBeamLike())
                        gravity = in->GetGravityPointInXRange(fLengthXToBreak);
                scale = fBeamWindowScale;
            }
            else // not broken from beam
            {
                gravity = it->GetLine().GetPoint(); // use fit's gravity point
                scale = 1;
            }
            // Partition to move to newVoxels
            auto toCluster {std::partition(refVoxels.begin(), refVoxels.end(),
                                           [&](const ActRoot::Voxel& voxel)
                                           {
                                               auto pos {voxel.GetPosition()};
                                               pos += XYZVectorF {0.5, 0.5, 0.5};
                                               return ManualIsInBeam(pos, gravity, scale);
                                           })};
            auto inBeamSize {std::distance(refVoxels.begin(), toCluster)};
            if(inBeamSize > 0)
            {
                std::vector<ActRoot::Voxel> newVoxels;
                newVoxels.insert(newVoxels.end(), std::make_move_iterator(toCluster),
                                 std::make_move_iterator(refVoxels.end()));
                refVoxels.erase(toCluster, refVoxels.end());

                // Reprocess
                std::vector<ActRoot::Cluster> newClusters;
                std::vector<ActRoot::Voxel> noise;
                std::tie(newClusters, noise) = fAlgo->Run(newVoxels);
                // Set not to merge these new ones
                // for(auto& ncl : newClusters)
                //     ncl.SetToMerge(false);
                toAppend.insert(toAppend.end(), std::make_move_iterator(newClusters.begin()),
                                std::make_move_iterator(newClusters.end()));

                // Delete current cluster
                it = fTPCData->fClusters.erase(it);
                // Verbose info
                if(fIsVerbose)
                {
                    std::cout << BOLDCYAN << "---- BreakTrack verbose for ID : " << it->GetClusterID() << " ----"
                              << '\n';
                    std::cout << "Gravity point     : " << gravity << '\n';
                    std::cout << "IsFromBreakBeam   ? " << std::boolalpha << it->GetIsBreakBeam() << '\n';
                    std::cout << "inBeamSize        : " << inBeamSize << '\n';
                    std::cout << "N of new clusters : " << newClusters.size() << '\n';
                    std::cout << "-------------------------" << RESET << '\n';
                }
            }
            else
            {
                // Do nothing; chi2 is really high and it will be deleted later in another action
                // Just in case, set flat not to merge
                it->SetToMerge(false);
                it++;
                // Verbose info
                if(fIsVerbose)
                {
                    std::cout << BOLDCYAN << "---- BreakTrack verbose for ID : " << it->GetClusterID() << " ----"
                              << '\n';
                    std::cout << "Gravity point     : " << gravity << '\n';
                    std::cout << "IsFromBreakBeam   ? " << std::boolalpha << it->GetIsBreakBeam() << '\n';
                    std::cout << "Skipping event bc there no voxels inside BeamRegion!" << '\n';
                    std::cout << "-------------------------" << RESET << '\n';
                }
                continue;
            }
        }
    }
    // Write to vector of clusters
    fTPCData->fClusters.insert(fTPCData->fClusters.end(), std::make_move_iterator(toAppend.begin()),
                               std::make_move_iterator(toAppend.end()));
}

void ActAlgorithm::Actions::BreakChi2::Print() const
{
    // This function just prints the current parameters of the action
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  ClusterAlgoPtr    : " << fAlgo << '\n';
    std::cout << "  Chi2Thresh        : " << fChi2Thresh << '\n';
    std::cout << "  MinXRange         : " << fMinXRange << '\n';
    std::cout << "  LengthXToBreak    : " << fLengthXToBreak << '\n';
    std::cout << "  BeamWindowY       : " << fBeamWindowY << '\n';
    std::cout << "  BeamWindowZ       : " << fBeamWindowZ << '\n';
    std::cout << "  DoClusterNotBeam  ? " << std::boolalpha << fDoClusterNotBeam << '\n';
    std::cout << "  DoBreakMultiTracks? " << std::boolalpha << fDoBreakMultiTracks << '\n';
    if(fDoBreakMultiTracks)
    {
        std::cout << "  TrackChi2Thresh   : " << fTrackChi2Thresh << '\n';
        std::cout << "  BeamWindowScale   : " << fBeamWindowScale << '\n';
    }

    std::cout << "······························" << RESET << '\n';
}

bool ActAlgorithm::Actions::BreakChi2::ManualIsInBeam(const XYZPointF& pos, const XYZPointF& gravity, double scale)
{
    bool condY {(gravity.Y() - scale * fBeamWindowY) < pos.Y() && pos.Y() < (gravity.Y() + scale * fBeamWindowY)};
    bool condZ {(gravity.Z() - scale * fBeamWindowZ) < pos.Z() && pos.Z() < (gravity.Z() + scale * fBeamWindowZ)};
    return condY && condZ;
}
