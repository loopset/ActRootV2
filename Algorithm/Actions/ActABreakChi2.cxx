#include "ActABreakChi2.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActTPCData.h"
#include "ActVoxel.h"

#include "TMath.h"

#include <ios>
#include <iostream>
#include <memory>
#include <vector>

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
    // Fixing chi2
    if(block->CheckTokenExists("FixMaxAngle"))
        fFixMaxAngle = block->GetDouble("FixMaxAngle");
    if(block->CheckTokenExists("FixMinXRange"))
        fFixMinXRange = block->GetDouble("FixMinXRange");
    if(block->CheckTokenExists("FixChi2Diff"))
        fFixChi2Diff = block->GetDouble("FixChi2Diff");
}

bool ActAlgorithm::Actions::BreakChi2::IsFixable(ActRoot::Cluster* cluster)
{
    // Cluster to be fixed must be "beam-like"
    // This function intends to correct clusters that are not broken
    // due to a small chi2, coming from a charge-weighted fit
    auto dot {cluster->GetLine().GetDirection().Unit().Dot(XYZVectorF {1, 0, 0})};
    auto angle {TMath::ACos(dot)};
    bool isParallell {TMath::Abs(angle) <= fFixMaxAngle * TMath::DegToRad()};
    auto [xmin, xmax] {cluster->GetXRange()};
    bool hasXPercent {(xmax - xmin) > fFixMinXRange};
    if(isParallell && hasXPercent)
        return true;
    return false;
}

void ActAlgorithm::Actions::BreakChi2::Run()
{
    if(!fIsEnabled)
        return;
    std::vector<ActRoot::Cluster> toAppend {};
    // Pointer to cluster vector
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        // 0-> Check whether cluster can be fixed
        auto fixable {IsFixable(&(*it))};
        float chi {it->GetLine().GetChi2()};
        if(fixable)
        {
            // Chi2 DISABLING q-weighting
            ActRoot::Line aux {};
            aux.FitVoxels(it->GetVoxels(), false);
            auto unweight {aux.GetChi2()};
            // And if difference is larger than passed threshold
            if(TMath::Abs(unweight - chi) >= fFixChi2Diff)
            {
                // Use this!
                chi = unweight;
                if(fIsVerbose)
                {
                    std::cout << BOLDGREEN << "---- " << GetActionID() << " verbose for ID : " << it->GetClusterID()
                              << " ----" << '\n';
                    std::cout << "-> Fixing Chi2 :" << '\n';
                    std::cout << "  Q-chi2       : " << it->GetLine().GetChi2() << '\n';
                    std::cout << "  Not Q-chi2   : " << unweight << '\n';
                    std::cout << RESET << '\n';
                }
            }
        }
        // 1-> Check whether we meet conditions to execute this
        bool isBadFit {chi > fChi2Thresh};
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
            // // Determination of gravity point
            // XYZPointF manuGP {};
            // bool isAuto {};
            // // Try to do it automatically
            // std::vector<ActRoot::Voxel> autovoxels {};
            // for(const auto& v : it->GetVoxels())
            // {
            //     auto dist {it->GetLine().DistanceLineToPoint(v.GetPosition())};
            //     if(dist < fMaxDistGP)
            //         autovoxels.push_back(v);
            // }
            // ActRoot::Line autoline {};
            // if(autovoxels.size() < 2) // manual in this case
            // {
            //     manuGP = it->GetGravityPointInXRange(fLengthXToBreak);
            // }
            // else
            // {
            //     autoline.FitVoxels(autovoxels);
            //     // And set default direction just in case
            //     autoline.SetDirection({1, 0, 0});
            //     isAuto = true;
            // }
            // // // 2-> Compute gravity point in the range
            // // // [xmin, xmin + fLengthXToBreak]
            // // auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};
            // if(fIsVerbose)
            // {
            //     std::cout << BOLDYELLOW << "---- BreakChi2 gravity point  for " << it->GetClusterID() << " ----"
            //               << '\n';
            //     std::cout << "  isAuto ? " << std::boolalpha << isAuto << '\n';
            //     if(isAuto)
            //     {
            //         std::cout << "  With " << autovoxels.size() << " voxels" << '\n';
            //         autoline.Print();
            //     }
            //     else
            //     {
            //         std::cout << "  ManualGP : " << manuGP << RESET << '\n';
            //     }
            // }

            // Determine gravity point
            auto gp {it->GetGravityPointInXRange(fLengthXToBreak)};
            // 3->Modify original cluster: move non-beam voxels outside to
            //  be clusterized independently
            auto& refToVoxels {it->GetRefToVoxels()};
            auto initSize {refToVoxels.size()};
            auto toMove {std::partition(refToVoxels.begin(), refToVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            auto pos {voxel.GetPosition()};
                                            pos += XYZVectorF {0.5, 0.5, 0.5};
                                            return ManualIsInBeam(pos, gp);
                                            // must move to centre of bin aka voxel
                                            // if(isAuto)
                                            //     return AutoIsInBeam(pos, &autoline);
                                            // else
                                            //     return ManualIsInBeam(pos, manuGP);
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
                // std::cout << "Gravity        = " << gravity << '\n';
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
                // INFO: update March 2025. Disable this since it can
                // create false positives of beam-likes
                // it->SetBeamLike(true);

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
    // Reset clusterID
    for(int i = 0; i < fTPCData->fClusters.size(); i++)
        fTPCData->fClusters[i].SetClusterID(i);
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
            XYZPointF gravity {-1, -1, -1};
            double scale {};
            if(it->GetIsBreakBeam()) // comes after running the first part of the action
            {
                for(auto in = fTPCData->fClusters.begin(); in != fTPCData->fClusters.end(); in++)
                    if(in->GetIsBeamLike())
                        gravity = in->GetGravityPointInXRange(fLengthXToBreak);
                scale = fBeamWindowScale;
                // If it doesnt success
                // WARNING: this is temporary and should be addressed in other way
                // because it comes from an activation of BreakChi2 in a cluster that is not beam like
                // Pending of major update of this algorithm
                if(gravity.X() == -1)
                {
                    gravity = it->GetLine().GetPoint();
                    scale = 1;
                }
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
                // Delete current cluster
                it = fTPCData->fClusters.erase(it);
            }
            else
            {
                // Do nothing; chi2 is really high and it will be deleted later in another action
                // Just in case, set flat not to merge
                it->SetToMerge(false);
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
                it++;
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
    std::cout << "  FixMaxAngle       : " << fFixMaxAngle << '\n';
    std::cout << "  FixMinXRange      : " << fFixMinXRange << '\n';
    std::cout << "  FixChi2Diff       : " << fFixChi2Diff << '\n';
    // std::cout << "  MaxDistAutoGP     : " << fMaxDistGP << '\n';
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

// bool ActAlgorithm::Actions::BreakChi2::AutoIsInBeam(const XYZPointF& pos, ActRoot::Line* line)
// {
//     auto dist {line->DistanceLineToPoint(pos)};
//     if(dist <= fBeamWindowY)
//         return true;
//     else
//         return false;
// }
