#include "ActAFindRP.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActUtils.h"
#include "ActVAction.h"
#include "ActVoxel.h"

#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

#include "Math/Point3D.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

void ActAlgorithm::Actions::FindRP::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fIsEnabled = block->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(block->CheckTokenExists("UseExtVoxels"))
        fUseExtVoxels = block->GetBool("UseExtVoxels");
    if(block->CheckTokenExists("BeamLikeMaxAngle"))
        fBeamLikeMaxAngle = block->GetDouble("BeamLikeMaxAngle");
    if(block->CheckTokenExists("BeamLikeXMinThresh"))
        fBeamLikeXMinThresh = block->GetDouble("BeamLikeXMinThresh");
    if(block->CheckTokenExists("RPDistThresh"))
        fRPDistThresh = block->GetDouble("RPDistThresh");
    if(block->CheckTokenExists("RPDistCluster"))
        fRPDistCluster = block->GetDouble("RPDistCluster");
    if(block->CheckTokenExists("RPDistToggleSort"))
        fRPDistToggleSort = block->GetDouble("RPDistToggleSort");
    if(block->CheckTokenExists("EnableDeleteInvalidCluster"))
        fEnableDeleteInvalidCluster = block->GetBool("EnableDeleteInvalidCluster");
    if(block->CheckTokenExists("RPMaskXY"))
        fRPMaskXY = block->GetDouble("RPMaskXY");
    if(block->CheckTokenExists("RPMaskZ"))
        fRPMaskZ = block->GetDouble("RPMaskZ");
    if(block->CheckTokenExists("EnableCylinder"))
        fEnableCylinder = block->GetBool("EnableCylinder");
    if(block->CheckTokenExists("CylinderR"))
        fCylinderR = block->GetDouble("CylinderR");
    if(block->CheckTokenExists("RPPivotDist"))
        fRPPivotDist = block->GetDouble("RPPivotDist");
    if(block->CheckTokenExists("EnableRPDefaultBeam"))
        fEnableRPDefaultBeam = block->GetBool("EnableRPDefaultBeam");
    if(block->CheckTokenExists("RPDefaultMinX"))
        fRPDefaultMinX = block->GetDouble("RPDefaultMinX");
    if(block->CheckTokenExists("EnableFineRP"))
        fEnableFineRP = block->GetBool("EnableFineRP");
    // Break beam to heavy
    if(block->CheckTokenExists("KeepBreakBeam"))
        fKeepBreakBeam = block->GetBool("KeepBreakBeam");
    if(block->CheckTokenExists("EnableFixBreakBeam"))
        fEnableFixBreakBeam = block->GetBool("EnableFixBreakBeam");
    if(block->CheckTokenExists("MaxVoxelsFixBreak"))
        fMaxVoxelsFixBreak = block->GetDouble("MaxVoxelsFixBreak");
    if(block->CheckTokenExists("MinPercentFixBreak"))
        fMinPercentFixBreak = block->GetDouble("MinPercentFixBreak");
    if(block->CheckTokenExists("MinXSepBreakBeam"))
        fMinXSepBreakBeam = block->GetDouble("MinXSepBreakBeam");
    if(block->CheckTokenExists("MinVoxelsBreakBeam"))
        fMinVoxelsBreakBeam = block->GetInt("MinVoxelsBreakBeam");
}

void ActAlgorithm::Actions::FindRP::Run()
{
    if(!fIsEnabled)
        return;
    if(fUseExtVoxels)
        SetExtVoxels();
    DetermineBeamLikes();
    if(!IsDoable())
        return;
    FindPreliminaryRP();
    if(fEnableDeleteInvalidCluster)
        DeleteInvalidCluster();
    if(fEnableFineRP)
    {
        PerformFinerFits();
        FindPreciseRP();
    }
}

void ActAlgorithm::Actions::FindRP::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  UseExtVoxels       ? " << std::boolalpha << fUseExtVoxels << '\n';
    std::cout << "  BeamLikeMaxAngle   : " << fBeamLikeMaxAngle << '\n';
    std::cout << "  BeamLikeXMinThresh : " << fBeamLikeXMinThresh << '\n';
    std::cout << "  RPDistThresh       : " << fRPDistThresh << '\n';
    std::cout << "  RPDistCluster      : " << fRPDistCluster << '\n';
    std::cout << "  RPDistToggle       : " << fRPDistToggleSort << '\n';
    std::cout << "  EnableDelInvalid   ? " << std::boolalpha << fEnableDeleteInvalidCluster << '\n';
    std::cout << "  EnableFineRP       ? " << std::boolalpha << fEnableFineRP << '\n';
    std::cout << "  MinXSepBreakBeam   : " << fMinXSepBreakBeam << '\n';
    std::cout << "  MinVoxelsBreak     : " << fMinVoxelsBreakBeam << '\n';
    std::cout << "  EnableKeepBreak    ? " << std::boolalpha << fKeepBreakBeam << '\n';
    std::cout << "  EnableFixBreak     ? " << std::boolalpha << fEnableFixBreakBeam << '\n';
    std::cout << "  MaxVoxelsFixBreak  : " << fMaxVoxelsFixBreak << '\n';
    std::cout << "  MinPercentFixBreak : " << fMinPercentFixBreak << '\n';
    std::cout << "  EnableDefaultBeam  ? " << std::boolalpha << fEnableRPDefaultBeam << '\n';
    std::cout << "  RPDefaultMinX      : " << fRPDefaultMinX << '\n';
    std::cout << "  EnableCylinder     ? " << std::boolalpha << fEnableCylinder << '\n';
    std::cout << "  CylinderR          : " << fCylinderR << '\n';
    std::cout << "  RPMaskXY           : " << fRPMaskXY << '\n';
    std::cout << "  RPMaskZ            : " << fRPMaskZ << '\n';
    std::cout << "  RPPivotDist        : " << fRPPivotDist << '\n';
    std::cout << "······························" << RESET << '\n';
}

void ActAlgorithm::Actions::FindRP::SetExtVoxels()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // Set if it was not already set before
        // The set method implicitly executes the ReFit
        // To update the line parameters with the extended voxel content
        if(!it->GetUseExtVoxels())
            it->SetUseExtVoxels(true);
    }
}

void ActAlgorithm::Actions::FindRP::DetermineBeamLikes()
{
    int nBeam {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // 1-> Check if xmin is bellow threshold
        auto [xmin, xmax] {it->GetXRange()};
        bool isInEntrance {xmin <= fBeamLikeXMinThresh};
        // 2-> Check if track has small opening angle
        auto uDir {it->GetLine().GetDirection().Unit()};
        auto angle {TMath::ACos(uDir.Dot(XYZVectorF {1, 0, 0})) * TMath::RadToDeg()};
        bool isParallel {angle <= fBeamLikeMaxAngle};
        // 3-> If both conditions true, mark as beam-like
        if(isInEntrance && isParallel)
        {
            it->SetBeamLike(true);
            nBeam++;
        }
    }
    // Print
    if(fIsVerbose)
    {
        std::cout << BOLDYELLOW << "---- FindRP::DetermineBeamLikes ----" << '\n';
        std::cout << "-> N beam clusters  : " << nBeam << '\n';
        std::cout << "-> N total clusters : " << fTPCData->fClusters.size() << '\n';
        std::cout << "-------------------------" << RESET << '\n';
    }
}

bool ActAlgorithm::Actions::FindRP::IsDoable()
{
    // Empty event
    if(fTPCData->fClusters.size() == 0)
    {
        if(fIsVerbose)
        {
            std::cout << BOLDYELLOW << "---- FindRP::IsDoable ----" << '\n';
            std::cout << "  Skipping since empty fClusters" << '\n';
            std::cout << "-------------------------" << RESET << '\n';
        }
        return false;
    }
    // Only pileup
    auto allBeam {std::all_of(fTPCData->fClusters.begin(), fTPCData->fClusters.end(),
                              [](ActRoot::Cluster& cl) { return cl.GetIsBeamLike(); })};
    if(allBeam)
    {
        if(fIsVerbose)
        {
            std::cout << BOLDYELLOW << "---- FindRP::IsDoable ----" << '\n';
            std::cout << "  Skipping since all beam-likes" << '\n';
            std::cout << "-------------------------" << RESET << '\n';
        }
        return false;
    }
    return true;
}

void ActAlgorithm::Actions::FindRP::FindPreliminaryRP()
{
    // If there is only one track, set to delete
    if(fTPCData->fClusters.size() == 1)
        fTPCData->fClusters.begin()->SetToDelete(true);

    // Declare vector of RPs
    std::vector<RPOps> rpops;
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
            // Assert one of them is BL
            if(!(out->GetIsBeamLike() || in->GetIsBeamLike()))
                continue;

            // Compute min distance between both lines
            auto [pA, pB, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                               in->GetLine().GetPoint(), in->GetLine().GetDirection())};

            if(dist < 0) // just in case they are parellel
                continue;
            // Build RP as mean of A and B
            XYZPointF rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};

            // Check that all points are valid
            bool checkA {IsRPValid(pA, fTPCPars)};
            bool checkB {IsRPValid(pB, fTPCPars)};
            bool checkRP {IsRPValid(rp, fTPCPars)};
            auto checkPoints {checkA && checkB && checkRP};
            // Check distance is bellow threshold
            bool checkDist {dist <= fRPDistThresh};
            if(checkPoints && checkDist)
            {
                rpops.push_back(RPOps {});
                rpops.back().fRP = rp;
                rpops.back().fIdxs.insert({i, j});
                // Find BL idx
                if(out->GetIsBeamLike())
                    rpops.back().fBLIdx = i;
                else
                    rpops.back().fBLIdx = j;
            }
            else
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDYELLOW << "---- FindRP::PreliminaryRP ----" << '\n';
                    std::cout << "  <i,j> : <" << i << "," << j << "> not in TPC or below dist threshold" << '\n';
                    std::cout << "  RP    : " << rp << '\n';
                    std::cout << "  dist  : " << dist << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }
                continue;
            }
        }
    }
    // proc means [proc]essed RPs
    auto proc {ClusterAndSortRPs(rpops)};
    std::set<int> toKeep {};
    fTPCData->fRPs.clear();
    if(proc.size() > 0)
    {
        // Set RP as the one with the biggest number
        // of cluster within distance
        fTPCData->fRPs.push_back(proc.front().fRP);
        // Marks its tracks to be kept
        toKeep = proc.front().fIdxs;
    }
    // This method ensures always a RP vector with size > 1 always!
    // Finally, set to delete cluster w/o a valid RP
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
    {
        auto it {fTPCData->fClusters.begin() + i};
        auto isToKeep {toKeep.find(i) != toKeep.end()};
        if(isToKeep)
            it->SetToDelete(false);
        else
            it->SetToDelete(true);
    }
}

std::tuple<ActAlgorithm::VAction::XYZPointF, ActAlgorithm::VAction::XYZPointF, double>
ActAlgorithm::Actions::FindRP::ComputeRPIn3D(ActRoot::Line::XYZPointF pA, ActRoot::Line::XYZVectorF vA,
                                             ActRoot::Line::XYZPointF pB, ActRoot::Line::XYZVectorF vB)
{
    // Using https://math.stackexchange.com/questions/1993953/closest-points-between-two-lines/3334866#3334866
    // 1-> Normalize all directions
    vA = vA.Unit();
    vB = vB.Unit();
    // 2-> Get the cross product and normalize it
    auto vC {vB.Cross(vA)};
    vC = vC.Unit();
    // If lines are parallel, skip them
    if(ActRoot::IsEqZero(vC.R()))
        return {pA, pB, -1};
    // 3-> Matrices to solve system of equations in Math StackExchange
    TMatrixD left {3, 3}; // 3x3 matrix with double precision
    // Fill left matrix with columns as each ABC vector
    ActRoot::Line::XYZVectorF vecs[3] {vA, -vB, vC};
    for(int col = 0; col < 3; col++)
    {
        double components[3] {};
        vecs[col].GetCoordinates(components);
        for(int row = 0; row < 3; row++)
            left[row][col] = components[row];
    }
    TMatrixD right {3, 1};
    auto diff {pB - pA};
    double components[3] {};
    diff.GetCoordinates(components);
    right.SetMatrixArray(components);
    // 4-> Invert left to solve system
    TMatrixD invLeft {TMatrixD::kInverted, left};
    // 5-> Solve system of linear eqs
    auto res {TMatrixD(invLeft, TMatrixD::kMult, right)};
    // 6-> Return results {point in A, point in B, distance AB}
    return {pA + res[0][0] * vA, pB + res[1][0] * vB, TMath::Abs(res[2][0])};
}


bool ActAlgorithm::Actions::FindRP::IsRPValid(const XYZPointF& rp, ActRoot::TPCParameters* tpc)
{
    // Need to consider the offset in the voxels
    bool isInX {0.5 <= rp.X() && rp.X() <= (tpc->GetNPADSX() - 1) + 0.5};
    bool isInY {0.5 <= rp.Y() && rp.Y() <= (tpc->GetNPADSY() - 1) + 0.5};
    bool isInZ {0.5 <= rp.Z() && rp.Z() <= (tpc->GetNPADSZ() - 1) + 0.5};
    return isInX && isInY && isInZ;
}

void ActAlgorithm::Actions::FindRP::RPOps::Print() const
{
    std::cout << BOLDYELLOW << "······ RPOps ······" << '\n';
    std::cout << "-> RP    : " << fRP << '\n';
    for(const auto& idx : fIdxs)
        std::cout << "   " << idx << '\n';
    std::cout << "   BLIdx : " << fBLIdx << '\n';
    std::cout << "   Nrp   : " << fNrp << '\n';
    std::cout << "   Size  : " << fIdxs.size() << '\n';
    std::cout << "   Dist  : " << fMinDist << '\n';
    std::cout << "   MinTL : " << fMinTL << '\n';
    std::cout << "····················" << RESET << '\n';
}

std::vector<ActAlgorithm::Actions::FindRP::RPOps>
ActAlgorithm::Actions::FindRP::ClusterAndSortRPs(std::vector<ActAlgorithm::Actions::FindRP::RPOps>& inirps)
{
    std::vector<RPOps> ret;
    // No reaction point
    if(inirps.empty())
        return ret;
    // Only one RP: direct calculation
    if(inirps.size() == 1)
    {
        ret.push_back(inirps.front());
        ret.back().fNrp = 1;
        // Find BL
        int bl {};
        for(const auto& idx : ret.back().fIdxs)
        {
            auto it {fTPCData->fClusters.begin() + idx};
            if(it->GetIsBeamLike())
            {
                bl = idx;
                break;
            }
        }
        ret.back().fBLIdx = bl;
        if(fIsVerbose)
            ret.front().Print();
        return ret;
    }
    //////////////////////////////////////////////////////////////
    // Several RPs require more work
    // Create map to separate RPs coming from different BLs
    std::map<int, std::vector<RPOps>> rpsbybl;
    for(const auto& rp : inirps)
        rpsbybl[rp.fBLIdx].push_back(rp);
    // Sort function based on Euclidean distance to origin
    auto euclideanDist {[](const RPOps& vl, const RPOps& vr)
                        {
                            // Reference point: origin of coordinates
                            XYZPointF origin {0, 0, 0};
                            auto distleft {(vl.fRP - origin).R()};
                            auto distright {(vr.fRP - origin).R()};
                            return distleft < distright;
                        }};
    // Cluster for each BL
    for(auto& [blidx, vrps] : rpsbybl)
    {
        std::sort(vrps.begin(), vrps.end(), euclideanDist);
        std::vector<std::vector<RPOps>> clusters;
        for(auto it = vrps.begin();;)
        {
            auto last {std::adjacent_find(it, vrps.end(), [&](const RPOps& l, const RPOps& r)
                                          { return (l.fRP - r.fRP).R() > fRPDistCluster; })};
            if(last == vrps.end())
            {
                clusters.emplace_back(it, last);
                break;
            }

            // Get next-to-last iterator
            auto gap {std::next(last)};

            // Push back and continue
            clusters.emplace_back(it, gap);

            // Prepare for next iteration
            it = gap;
        }
        // Build each element out of the cluster
        for(const auto& cluster : clusters)
        {
            ret.push_back(RPOps {});
            auto& back {ret.back()};
            for(const auto& element : cluster)
            {
                back.fRP += XYZVectorF {element.fRP};
                back.fIdxs.insert(element.fIdxs.begin(), element.fIdxs.end());
            }
            // Compute mean and number of rps in cluster
            back.fRP /= cluster.size();
            back.fNrp = cluster.size();
            back.fBLIdx = blidx;
        }
    }
    // Sort by number of tracks in RP
    std::sort(ret.begin(), ret.end(), [](const RPOps& l, const RPOps& r) { return l.fIdxs.size() > r.fIdxs.size(); });
    // Search for tie: adjacent_find returns first iterator of a pair (it, it +1) that evals to true
    auto tie {std::adjacent_find(ret.begin(), ret.end(),
                                 [](const RPOps& l, const RPOps& r) { return l.fIdxs.size() == r.fIdxs.size(); })};
    if(tie != ret.begin() ||
       tie == ret.end()) // Tie is not first element or no ties at all found -> Continue without more analysis
    {
        ;
    }
    else
    {
        // Find range of tie
        auto begin {ret.begin()}; // for sure it is at begin
        // Find last element of tie
        auto value {begin->fIdxs.size()};
        // Find last element of the tie using reverse iterators
        auto it {std::find_if(ret.rbegin(), ret.rend(), [&](const RPOps& ops) { return ops.fIdxs.size() == value; })};
        auto idx {std::distance(it, ret.rend())}; // watch out for reverse iterators!!
        auto end {ret.begin() + idx};

        // Compute min track length
        auto minTL {[this](RPOps& ops)
                    {
                        std::vector<double> tls {};
                        for(const auto& idx : ops.fIdxs)
                        {
                            auto it {fTPCData->fClusters.begin() + idx};
                            if(it->GetIsBeamLike())
                                continue;
                            // Sort based on fit
                            it->SortAlongDir();
                            auto begin {it->GetVoxels().front().GetPosition()};
                            auto end {it->GetRefToVoxels().back().GetPosition()};
                            // Push this preliminary track length
                            auto tl {(begin - end).R()};
                            tls.push_back(tl);
                        }
                        // And get minimum track length
                        ops.fMinTL = *(std::min_element(tls.begin(), tls.end()));
                    }};
        // And minimum distance to RP
        auto minDist {[this](RPOps& ops)
                      {
                          std::vector<double> dists;
                          for(const auto& idx : ops.fIdxs)
                          {
                              auto it {fTPCData->fClusters.begin() + idx};
                              if(it->GetIsBeamLike())
                                  continue;
                              // Sort voxels
                              std::sort(it->GetRefToVoxels().begin(), it->GetRefToVoxels().end());
                              // Get begin and end of cluster
                              auto begin {it->GetRefToVoxels().front().GetPosition()};
                              auto end {it->GetRefToVoxels().back().GetPosition()};
                              double mindist {1111};
                              for(auto point : {begin, end})
                              {
                                  point += XYZVectorF {0.5, 0.5, 0.5};
                                  auto dist {(point - ops.fRP).R()};
                                  if(dist < mindist)
                                      mindist = dist;
                              }
                              dists.push_back(mindist);
                          }
                          // Compute mean
                          ops.fMinDist = (std::accumulate(dists.begin(), dists.end(), 0.0) / dists.size());
                      }};
        std::for_each(begin, end,
                      [&minTL, &minDist](RPOps& ops)
                      {
                          minTL(ops);
                          minDist(ops);
                      });
        // And now sort!
        // 1-> If any of the RPs has a fMinDist < Threshold, sort by min dist
        bool hasMinDist {std::any_of(begin, end, [&](const RPOps& ops) { return ops.fMinDist < fRPDistToggleSort; })};
        if(hasMinDist)
        {
            std::sort(begin, end, [](const RPOps& l, const RPOps& r) { return l.fMinDist < r.fMinDist; });
        }
        // 2-> Else, sort my promoting RPOps with largest smaller track
        else
        {
            std::sort(begin, end, [](const RPOps& l, const RPOps& r) { return l.fMinTL > r.fMinTL; });
        }
    }
    // Print info
    if(fIsVerbose)
    {
        std::cout << BOLDYELLOW << "---- FindRP::SortAndCluster ----" << '\n';
        for(const auto& e : ret)
            e.Print();
        if(ret.size())
            std::cout << "  PreliminaryRP : " << ret.front().fRP << '\n';
        std::cout << "------------------------------" << RESET << '\n';
    }
    return ret;
}

void ActAlgorithm::Actions::FindRP::DeleteInvalidCluster()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        if(it->GetToDelete())
            it = fTPCData->fClusters.erase(it);
        else
            it++;
    }
    // Reset index bc we could be deleting clusters
    for(int i = 0, size = fTPCData->fClusters.size(); i < size; i++)
        fTPCData->fClusters[i].SetClusterID(i);
}

void ActAlgorithm::Actions::FindRP::PerformFinerFits()
{
    if(fTPCData->fRPs.size() == 0)
        return;
    const auto& rp {fTPCData->fRPs.front()};

    // 2-> Break BL starting on RP
    auto hasBroken {BreakBeamToHeavy(rp, fKeepBreakBeam)};

    // // 3-> Delete clusters with less than ClIMB min point or with fBLMinVoxels if BL
    // for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    // {
    //     if(it->GetIsBeamLike())
    //     {
    //         if(it->GetSizeOfVoxels() <= fBeamLikeMinVoxels)
    //         {
    //             if(fIsVerbose)
    //             {
    //                 std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose ----" << '\n';
    //                 std::cout << "What: Deleting RP with fVoxels.size() < fBLMinVoxels" << '\n';
    //                 std::cout << "-----------------------------" << RESET << '\n';
    //             }
    //             it = fTPCData->fClusters.erase(it);
    //         }
    //         else
    //             it++;
    //     }
    //     else
    //     {
    //         if(it->GetSizeOfVoxels() <= fAlgo->GetMinPoints())
    //             it = fTPCData->fClusters.erase(it);
    //         else
    //             it++;
    //     }
    // }

    // 4 -> Mask region around RP
    MaskAroundRP(rp, hasBroken);

    // 5 -> Clean voxels outside cylinder
    if(fEnableCylinder)
        CylinderCleaning();

    // 6 -> Mask region at the begining and end of the tracks
    MaskBeginEnd(rp);

    // 7 -> Set default fit for BLs
    // UPDATE: this is now included in BreakToBeam!
    // if(fEnableRPDefaultBeam)
    // {
    //     for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    //     {
    //         if(it->GetIsBeamLike())
    //         {
    //             auto [xmin, xmax] {it->GetXRange()};
    //             bool isShort {(xmax - xmin) <= fRPDefaultMinX};
    //             if(isShort)
    //             {
    //                 it->GetRefToLine().SetDirection({1, 0, 0});
    //                 if(fIsVerbose)
    //                 {
    //                     std::cout << BOLDMAGENTA << "---- FineRP::DefaultBeam ----" << '\n';
    //                     std::cout << "-> Cluster #" << it->GetClusterID() << '\n';
    //                     std::cout << "   Short beam => setting (1, 0, 0) direction" << '\n';
    //                     std::cout << "------------------------------" << RESET << '\n';
    //                 }
    //             }
    //         }
    //     }
    // }
}

bool ActAlgorithm::Actions::FindRP::BreakBeamToHeavy(const ActAlgorithm::VAction::XYZPointF& rp, bool keepSplit)
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // Determine whether is breakable
        auto [xmin, xmax] {it->GetXRange()};
        // In order to be breakable, besides being BeamLike,
        // there must be a certain distance between the RP.X() and the last voxel of the cluster
        auto dist {std::abs(xmax - fTPCData->fRPs.front().X())};
        bool isBreakable {dist >= fMinXSepBreakBeam};
        if(it->GetIsBeamLike() && isBreakable)
        {
            auto& refVoxels {it->GetRefToVoxels()};
            // Initial size
            auto before {refVoxels.size()};
            // Sort them
            std::sort(refVoxels.begin(), refVoxels.end());
            // Partition
            auto rpBreak {std::partition(refVoxels.begin(), refVoxels.end(),
                                         [&](const ActRoot::Voxel& voxel)
                                         {
                                             auto pos {voxel.GetPosition()};
                                             pos += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                                             return (pos.X() < rp.X());
                                         })};
            // Sizes
            auto after {std::distance(refVoxels.begin(), rpBreak)};
            auto newsize {before - after};
            // If BL has enough voxels remaining
            if(after >= fMinVoxelsBreakBeam)
            {
                // Move
                std::vector<ActRoot::Voxel> newVoxels;
                newVoxels.insert(newVoxels.end(), std::make_move_iterator(rpBreak),
                                 std::make_move_iterator(refVoxels.end()));
                refVoxels.erase(rpBreak, refVoxels.end());

                // Add heavy cluster if has minimum size of voxels
                // and algorithm is configured to keep it
                if(newsize >= fAlgo->GetMinPoints() && keepSplit)
                {
                    ActRoot::Cluster newCluster {(int)fTPCData->fClusters.size()};
                    newCluster.SetVoxels(std::move(newVoxels));
                    // Propagate ExtVoxels to new cluster
                    if(fUseExtVoxels)
                        newCluster.SetUseExtVoxels(true); // implicitly executes ReFit()
                    else
                        newCluster.ReFit();
                    newCluster.ReFillSets();
                    newCluster.SetIsSplitRP(true);
                    newCluster.SetHasRP(true);
                    newCluster.SetRegionType(ActRoot::RegionType::EBeam);
                    toAppend.push_back(std::move(newCluster));

                    if(fIsVerbose)
                    {
                        std::cout << BOLDMAGENTA << "---- BreakAfterRP ----" << '\n';
                        std::cout << "-> Added heavy cluster of size : " << newsize << '\n';
                        std::cout << "-----------------------------" << RESET << '\n';
                    }
                }
                it->ReFit();
                it->ReFillSets();
                auto [xmin, xmax] {it->GetXRange()};
                bool isShort {(xmax - xmin) <= fRPDefaultMinX};
                // Check if default beam is enabled
                if(isShort && fEnableRPDefaultBeam)
                {
                    if(fIsVerbose)
                    {
                        std::cout << BOLDRED << "---- FineRP::BreakBeam ----" << '\n';
                        std::cout << " -> Cluster #" << it->GetClusterID() << '\n';
                        std::cout << "    XRange : " << xmax - xmin << '\n';
                        std::cout << "    is too short (" << (xmax - xmin) << " < " << fRPDefaultMinX
                                  << ") => set {1, 0, 0} dir" << '\n';
                        std::cout << "------------------------------" << RESET << '\n';
                    }
                    // Compute beam in Y and Z to set the point of the BL cluster as a compromise
                    // in resolution between the fit and the PreliminaryRP
                    auto& line {it->GetLine()};
                    auto& rp {fTPCData->fRPs.front()};
                    XYZPointF p {line.GetPoint().X(), 0.5f * (line.GetPoint().Y() + rp.Y()),
                                 0.5f * (line.GetPoint().Z() + rp.Z())};
                    it->GetRefToLine().SetPoint(p);
                    it->GetRefToLine().SetDirection({1, 0, 0});
                    it->SetIsDefault(true);
                }
            }
            else
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- BreakAfterRP ----" << '\n';
                    std::cout << "-> Cluster #" << it->GetClusterID() << '\n';
                    std::cout << "   is left with very few voxels : " << after << " < " << fMinVoxelsBreakBeam << '\n';
                    std::cout << "   => Not breaking! : " << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }
        }
        // Print info
        if(it->GetIsBeamLike() && !isBreakable && fIsVerbose)
        {
            std::cout << BOLDMAGENTA << "---- BreakAfterRP ----" << '\n';
            std::cout << "-> IsNotBreakable!" << '\n';
            std::cout << "-> Xmax   : " << xmax << '\n';
            std::cout << "-> RP.X() : " << fTPCData->fRPs.front().X() << '\n';
            std::cout << "-> Dist   : " << dist << " < " << fMinXSepBreakBeam << '\n';
            std::cout << "-----------------------------" << RESET << '\n';
        }
    }
    fTPCData->fClusters.insert(fTPCData->fClusters.end(), std::make_move_iterator(toAppend.begin()),
                               std::make_move_iterator(toAppend.end()));
    // Return whether function has provided a new cluster or not
    bool ret {static_cast<bool>(toAppend.size())};
    if(!fEnableFixBreakBeam)
        return ret;
    // Attempt to fix BreakBeam
    auto size {fTPCData->fClusters.size()};
    auto added {toAppend.size()};
    std::map<int, std::set<int>> toAdd;
    for(int i = (size - 1); i > (size - added - 1); i--)
    {
        // Get iterator
        auto iit {fTPCData->fClusters.begin() + i};
        // Check added cluster verifies condition of being small
        if(iit->GetSizeOfVoxels() <= fMaxVoxelsFixBreak)
        {
            int idx {-1};
            double percent {-1};
            for(int j = 0; j <= (size - added - 1); j++)
            {
                // Get iterator
                auto jit {fTPCData->fClusters.begin() + j};
                if(i == j || jit->GetIsBeamLike())
                    continue;
                // Count number of voxels within cylinder radius
                // WARNING: using same cylinder radius as for the other function!
                auto count {
                    std::count_if(iit->GetVoxels().begin(), iit->GetVoxels().end(), [&](const ActRoot::Voxel& v)
                                  { return jit->GetLine().DistanceLineToPoint(v.GetPosition()) < fCylinderR; })};
                double aux {(double)count / iit->GetSizeOfVoxels()};
                if(fIsVerbose)
                {
                    std::cout << BOLDYELLOW << "---- FindRP::BreakBeamToHeavy ----" << '\n';
                    std::cout << "  <i,j> : <" << i << "," << j << "> with percent : " << aux << RESET << '\n';
                }
                if(aux > percent)
                {
                    percent = aux;
                    idx = j;
                }
            }
            if(percent >= fMinPercentFixBreak)
            {
                toAdd[idx].insert(i);
                if(fIsVerbose)
                {
                    std::cout << BOLDRED << "-> Fixing break beam" << '\n';
                    std::cout << "   Small cluster sized : " << iit->GetSizeOfVoxels() << " with percent " << percent
                              << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }
        }
    }
    // Add to clusters
    std::set<int, std::greater<int>> toDelete;
    for(const auto& [out, set] : toAdd)
    {
        auto& outvoxels {fTPCData->fClusters[out].GetRefToVoxels()};
        for(const auto& in : set)
        {
            toDelete.insert(in);
            auto& invoxels {fTPCData->fClusters[in].GetRefToVoxels()};
            outvoxels.insert(outvoxels.end(), std::make_move_iterator(invoxels.begin()),
                             std::make_move_iterator(invoxels.end()));
        }
        fTPCData->fClusters[out].ReFit();
        fTPCData->fClusters[out].ReFillSets();
    }
    // Erase
    for(const auto& idx : toDelete)
        fTPCData->fClusters.erase(fTPCData->fClusters.begin() + idx);
    return ret;
}

void ActAlgorithm::Actions::FindRP::CylinderCleaning()
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        auto& refVoxels {it->GetRefToVoxels()};
        auto oldSize {refVoxels.size()};
        auto itKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                                        auto dist {it->GetLine().DistanceLineToPoint(pos)};
                                        return dist <= fCylinderR;
                                    })};
        // if enough voxels remain
        auto remain {std::distance(refVoxels.begin(), itKeep)};
        if((oldSize != remain) && remain >= fAlgo->GetMinPoints())
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "---- CylinderCleaning ----" << '\n';
                std::cout << "   cluster #" << it->GetClusterID() << '\n';
                std::cout << "   (old - new) sizes : " << (oldSize - remain) << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
        }
    }
}

void ActAlgorithm::Actions::FindRP::MaskAroundRP(const ActAlgorithm::VAction::XYZPointF& rp, bool blHasBroken)
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // Skip BL cluster that has not been broken
        if(it->GetIsBeamLike() && !blHasBroken)
            continue;
        auto& refVoxels {it->GetRefToVoxels()};
        auto initial {refVoxels.size()};
        auto toKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                                        bool condX {std::abs(pos.X() - rp.X()) <= fRPMaskXY};
                                        bool condY {std::abs(pos.Y() - rp.Y()) <= fRPMaskXY};
                                        bool condZ {std::abs(pos.Z() - rp.Z()) <= fRPMaskZ};
                                        return !(condX && condY && condZ);
                                    })};
        // Compare sizes
        auto remainSize {std::distance(refVoxels.begin(), toKeep)};
        // Indeed delete region and refit
        if((initial != remainSize) && remainSize >= fAlgo->GetMinPoints())
        {
            refVoxels.erase(toKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(fIsVerbose)
            {
                std::cout << BOLDMAGENTA << "---- FineRP::MaskAroundRP ----" << '\n';
                std::cout << "-> Cluster #" << it->GetClusterID() << '\n';
                std::cout << "   Masked " << (initial - remainSize) << " voxels" << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
        }
    }
}

void ActAlgorithm::Actions::FindRP::MaskBeginEnd(const ActAlgorithm::VAction::XYZPointF& rp)
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // Declare variables
        const auto& line {it->GetLine()};
        const auto& gp {line.GetPoint()};
        auto& refVoxels {it->GetRefToVoxels()};
        auto oldSize {refVoxels.size()};
        // Sort them using direction given by line fit
        it->SortAlongDir();
        // std::sort(refVoxels.begin(), refVoxels.end());
        // Set same sign as rp
        it->GetRefToLine().AlignUsingPoint(rp);
        // Get init point
        auto init {refVoxels.front()};
        // Get end point
        auto end {refVoxels.back()};
        auto projInit {line.ProjectionPointOnLine(init.GetPosition() + ROOT::Math::XYZVector {0.5, 0.5, 0.5})};
        auto projEnd {line.ProjectionPointOnLine(end.GetPosition() + ROOT::Math::XYZVector {0.5, 0.5, 0.5})};
        // Partition: get iterator to last element to be kept
        auto itKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                                        auto proj {line.ProjectionPointOnLine(pos)};
                                        // delete all points over projInit/end
                                        // bc due to ordering and angle, some voxel could have a proj larger than
                                        // the one of the last/first voxel
                                        // TODO: check a better way to mask outling voxels (proj.X() < projInit.X())
                                        // could be troublesome depending on track angle
                                        // bool isInCapInit {(proj - projInit).R() <= fRPPivotDist || (proj.X() <
                                        // projInit.X())}; bool isInCapEnd {(proj - projEnd).R() <= fRPPivotDist ||
                                        // (proj.X() > projEnd.X())};
                                        bool isInCapInit {(proj - projInit).R() <= fRPPivotDist};
                                        bool isInCapEnd {(proj - projEnd).R() <= fRPPivotDist};
                                        return !(isInCapInit || isInCapEnd);
                                    })};
        auto newSize {std::distance(refVoxels.begin(), itKeep)};
        // Refit if eneugh voxels remain
        if((oldSize != newSize) && newSize >= fAlgo->GetMinPoints())
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(fIsVerbose)
            {
                {
                    std::cout << BOLDYELLOW << "--- FineRP::MaskBeginEnd ----" << '\n';
                    std::cout << "-> Cluster #" << it->GetClusterID() << '\n';
                    std::cout << "   Init : " << init.GetPosition() << '\n';
                    std::cout << "   Proj Init : " << projInit << '\n';
                    std::cout << "   End : " << end.GetPosition() << '\n';
                    std::cout << "   Proj End : " << projEnd << '\n';
                    std::cout << "   (Old - New) sizes : " << (oldSize - refVoxels.size()) << '\n';
                    std::cout << "   Gravity point : " << it->GetLine().GetPoint() << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }
            }
            else
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDRED << "--- FineRP::MaskBeginEnd  ----" << '\n';
                    std::cout << "-> Cluster #" << it->GetClusterID() << '\n';
                    std::cout << "   Remaining size : " << refVoxels.size() << " < " << fAlgo->GetMinPoints() << '\n';
                    std::cout << "   => Not erasing nor refitting !" << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }
            }
        }
    }
}

void ActAlgorithm::Actions::FindRP::FindPreciseRP()
{
    if(fTPCData->fRPs.size() == 0)
        return;
    // Precise RP is found by intersection of a BL cluster with the track with larger angle
    // We sort them in this way using a set
    typedef std::pair<double, ActAlgorithm::VAction::XYZPointF> SetValue;
    auto lambda {[](const SetValue& l, const SetValue& r) { return l.first > r.first; }};
    std::set<SetValue, decltype(lambda)> set {lambda};
    for(auto out = fTPCData->fClusters.begin(); out != fTPCData->fClusters.end(); out++)
    {
        // Find a BL cluster
        if(out->GetIsBeamLike())
        {
            // Ensure direction of BL is always positive along X
            auto& outLine {out->GetRefToLine()};
            const auto& oldDir {outLine.GetDirection()};
            outLine.SetDirection({std::abs(oldDir.X()), oldDir.Y(), oldDir.Z()});

            for(auto in = fTPCData->fClusters.begin(); in != fTPCData->fClusters.end(); in++)
            {
                if(in == out)
                    continue;
                // Ensure direction signs are defined from preliminary RP
                in->GetRefToLine().AlignUsingPoint(fTPCData->fRPs.front());
                // Compute angle theta
                auto theta {GetClusterAngle(out->GetLine().GetDirection(), in->GetLine().GetDirection())};
                // Compute RPs
                auto [a, b, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                                 in->GetLine().GetPoint(), in->GetLine().GetDirection())};
                if(dist < 0) // just in case lines were parallel
                    continue;
                ActAlgorithm::VAction::XYZPointF rp {(a.X() + b.X()) / 2, (a.Y() + b.Y()) / 2, (a.Z() + b.Z()) / 2};
                // Check that all points are valid
                bool checkA {IsRPValid(a, fTPCPars)};
                bool checkB {IsRPValid(b, fTPCPars)};
                bool checkRP {IsRPValid(rp, fTPCPars)};
                auto checkPoints {checkA && checkB && checkRP};
                // And finally that distance AB is bellow threshold
                bool checkDist {dist <= fRPDistThresh};
                if(checkPoints && checkDist)
                    set.insert({std::abs(theta), rp});
            }
        }
    }
    // Write
    if(set.size() > 0)
    {
        fTPCData->fRPs.clear();
        fTPCData->fRPs.push_back(set.begin()->second);
    }
    else
    {
        ; // keep preliminary RP just in case this finner method fails
    }
}

double ActAlgorithm::Actions::FindRP::GetClusterAngle(const ActRoot::Line::XYZVectorF& beam,
                                                      const ActRoot::Line::XYZVectorF& recoil)
{
    auto dot {beam.Unit().Dot((recoil.Unit()))};
    return TMath::ACos(dot) * TMath::RadToDeg();
}
