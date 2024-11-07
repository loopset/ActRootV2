#include "ActAFindRP.h"

#include "ActColors.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActUtils.h"
#include "ActVAction.h"

#include <TMath.h>
#include <TMatrixD.h>
#include <TVectorD.h>

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
    if(block->CheckTokenExists("RPDistThresh"))
        fRPDistThresh = block->GetDouble("RPDistThresh");
    if(block->CheckTokenExists("RPDistCluster"))
        fRPDistCluster = block->GetDouble("RPDistCluster");
    if(block->CheckTokenExists("RPDistValidate"))
        fRPDistValidate = block->GetDouble("RPDistValidate");
    if(block->CheckTokenExists("EnableDeleteInvalidCluster"))
        fEnableDeleteInvalidCluster = block->GetBool("EnableDeleteInvalidCluster");
    if(block->CheckTokenExists("BeamLikeMinVoxels"))
        fBeamLikeMinVoxels = block->GetDouble("BeamLikeMinVoxels");
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
}

void ActAlgorithm::Actions::FindRP::Run()
{
    if(!fIsEnabled)
        return;
    DetermineBeamLikes();
    FindPreliminaryRP();
    if(fEnableDeleteInvalidCluster)
        DeleteInvalidCluster();
    if(fEnableFineRP)
    {
        PerformFinerFits();
        FindPreciseRP();
    }
    ResetIndex();
}

void ActAlgorithm::Actions::FindRP::Print() const
{
    std::cout << BOLDCYAN << "····· " << GetActionID() << " ·····" << '\n';
    if(!fIsEnabled)
    {
        std::cout << "······························" << RESET << '\n';
        return;
    }
    std::cout << "  BeamLikeParallelF         : " << fBeamLikeParallelF << '\n';
    std::cout << "  BeamLikeXMinThresh        : " << fBeamLikeXMinThresh << '\n';
    std::cout << "  RPDistThresh        : " << fRPDistThresh << '\n';
    std::cout << "  RPDistCluster        : " << fRPDistCluster << '\n';
    std::cout << "  RPDistValidate        : " << fRPDistValidate << '\n';
    std::cout << "  BeamLikeMinVoxels        : " << fBeamLikeMinVoxels << '\n';
    std::cout << "  RPMaskXY        : " << fRPMaskXY << '\n';
    std::cout << "  RPMaskZ        : " << fRPMaskZ << '\n';
    std::cout << "  CylinderR        : " << fCylinderR << '\n';
    std::cout << "  RPPivotDist        : " << fRPPivotDist << '\n';
    std::cout << "  RPDefaultMinX        : " << fRPDefaultMinX << '\n';    

    std::cout << "······························" << RESET << '\n';
}

void ActAlgorithm::Actions::FindRP::DetermineBeamLikes()
{
    int nBeam {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
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

typedef std::pair<ActAlgorithm::VAction::XYZPoint, std::pair<int, int>> RPValue; // includes RP and pair
                                                                                 // of indexes as values
void ActAlgorithm::Actions::FindRP::FindPreliminaryRP()
{
    // If there is only one track, set to delete
    if(fTPCData->fClusters.size() == 0)
        fTPCData->fClusters.begin()->SetToDelete(true);

    // Declare vector of RPs
    std::vector<RPValue> rps;
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
            bool checkA {IsRPValid(pA, fTPCPars)};
            bool checkB {IsRPValid(pB, GetTPCParameters())};
            bool checkRP {IsRPValid(rp, GetTPCParameters())};
            auto checkPoints {checkA && checkB && checkRP};
            // Check distance is bellow threshold
            bool checkDist {dist <= fRPDistThresh};
            if(checkPoints && checkDist)
                rps.push_back({rp, {i, j}});
            else
                continue;
        }
    }
    auto proc {ClusterAndSortRPs(rps)};
    std::set<int> toKeep {};
    fTPCData->fRPs.clear();
    if(proc.size() > 0)
    {
        // Set RP as the one with the biggest number
        // of cluster within distance
        fTPCData->fRPs.push_back(ActRoot::TPCData::XYZPoint(static_cast<float>(proc.front().first.X()),
                                                            static_cast<float>(proc.front().first.Y()),
                                                            static_cast<float>(proc.front().first.Z())));
        // Marks its tracks to be kept
        toKeep = proc.front().second;
    }
    // This method ensures always a RP vector with size > 1 always!
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

std::tuple<ActAlgorithm::VAction::XYZPoint, ActAlgorithm::VAction::XYZPoint, double>
ActAlgorithm::Actions::FindRP::ComputeRPIn3D(ActPhysics::Line::XYZPoint pA, ActPhysics::Line::XYZVector vA,
                                             ActPhysics::Line::XYZPoint pB, ActPhysics::Line::XYZVector vB)
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
        return {ActAlgorithm::VAction::XYZPoint(static_cast<double>(pA.X()), static_cast<double>(pA.Y()),
                                                static_cast<double>(pA.Z())),
                ActAlgorithm::VAction::XYZPoint(static_cast<double>(pB.X()), static_cast<double>(pB.Y()),
                                                static_cast<double>(pB.Z())),
                -1}; // done like that because class Line usess float and VAction double
    // 3-> Matrices to solve system of equations in Math StackExchange
    TMatrixD left {3, 3}; // 3x3 matrix with double precision
    // Fill left matrix with columns as each ABC vector
    ActPhysics::Line::XYZVector vecs[3] {vA, -vB, vC};
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
    return {ActAlgorithm::VAction::XYZPoint(static_cast<double>((pA + res[0][0] * vA).X()),
                                            static_cast<double>((pA + res[0][0] * vA).Y()),
                                            static_cast<double>((pA + res[0][0] * vA).Z())),
            ActAlgorithm::VAction::XYZPoint(static_cast<double>((pB + res[1][0] * vB).X()),
                                            static_cast<double>((pB + res[1][0] * vB).Y()),
                                            static_cast<double>((pB + res[1][0] * vB).Z())),
            TMath::Abs(res[2][0])}; // done like that because class Line usess float and VAction double
}


bool ActAlgorithm::Actions::FindRP::IsRPValid(const XYZPoint& rp, ActRoot::TPCParameters* tpc)
{
    // Need to consider the offset in the voxels
    bool isInX {0.5 <= rp.X() && rp.X() <= (tpc->GetNPADSX() - 1) + 0.5};
    bool isInY {0.5 <= rp.Y() && rp.Y() <= (tpc->GetNPADSY() - 1) + 0.5};
    bool isInZ {0.5 <= rp.Z() && rp.Z() <= (tpc->GetNPADSZ() - 1) + 0.5};
    return isInX && isInY && isInZ;
}

typedef std::pair<ActAlgorithm::VAction::XYZPoint, std::set<int>> RPCluster;
std::vector<RPCluster> ActAlgorithm::Actions::FindRP::ClusterAndSortRPs(std::vector<RPValue>& rps)
{
    std::vector<RPCluster> ret;
    if(rps.empty())
        return ret;
    if(rps.size() == 1)
    {
        ret.push_back({rps.front().first, {rps.front().second.first, rps.front().second.second}});
        return ret;
    }
    // Sort
    auto cmp {[](const RPValue& vl, const RPValue& vr)
              {
                  const auto& l {vl.first};
                  const auto& r {vr.first};
                  if(l.X() != r.X())
                      return l.X() < r.X();
                  if(l.Y() != r.Y())
                      return l.Y() < r.Y();
                  return l.Z() < r.Z();
              }};
    std::sort(rps.begin(), rps.end(), cmp);
    // Cluster
    std::vector<std::vector<RPValue>> clusters;
    for(auto it = rps.begin();;)
    {
        auto last {std::adjacent_find(it, rps.end(), [&](const RPValue& l, const RPValue& r)
                                      { return (l.first - r.first).R() > fRPDistCluster; })};
        if(last == rps.end())
        {
            clusters.emplace_back(it, last);
            break;
            ;
        }

        // Get next-to-last iterator
        auto gap {std::next(last)};

        // Push back and continue
        clusters.emplace_back(it, gap);

        // Prepare for next iteration
        it = gap;
    }
    // Sort by size of cluster
    std::sort(clusters.begin(), clusters.end(),
              [](const std::vector<RPValue>& l, const std::vector<RPValue>& r) { return l.size() > r.size(); });

    // Get mean clusters
    for(const auto& cluster : clusters)
    {
        std::set<int> set {};
        ActAlgorithm::VAction::XYZPoint mean;
        for(const auto& [rps, idx] : cluster)
        {
            mean += ActAlgorithm::VAction::XYZVector {rps};
            set.insert(idx.first);
            set.insert(idx.second);
        }
        mean /= cluster.size();
        ret.push_back({mean, set});
    }
    // Validate once again using distance
    auto distValid = [this](const RPCluster& cluster)
    {
        int count {};
        for(const auto& idx : cluster.second)
        {
            auto it {fTPCData->fClusters.begin() + idx};
            if(it->GetIsBeamLike())
                continue;
            // Sort voxels: ActRoot voxels has a function > and <, work as the cmp built in this function
            std::sort(it->GetRefToVoxels().begin(), it->GetRefToVoxels().end());
            // Min and max
            auto min {it->GetRefToVoxels().front().GetPosition()};
            auto max {it->GetRefToVoxels().back().GetPosition()};
            for(auto p : {min, max})
            {
                p += ROOT::Math::XYZVector {0.5, 0.5, 0.5};
                auto dist {(p - cluster.first).R()};
                if(dist < fRPDistValidate)
                    count++;
            }
        }
        return count;
    };
    // Call to sort
    std::sort(ret.begin(), ret.end(),
              [this, &distValid](RPCluster& l, RPCluster& r)
              {
                  auto lc {distValid(l)};
                  auto rc {distValid(r)};
                  return lc > rc;
              });
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
}

void ActAlgorithm::Actions::FindRP::PerformFinerFits()
{
    if(fTPCData->fRPs.size() == 0)
        return;
    const auto& rp {fTPCData->fRPs.front()};

    // 2-> Break BL starting on RP
    BreakBeamToHeavy(rp, true);

    // 3-> Delete clusters with less than ClIMB min point or with fBLMinVoxels if BL
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end();)
    {
        if(it->GetIsBeamLike())
        {
            if(it->GetSizeOfVoxels() <= fBeamLikeMinVoxels)
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose ----" << '\n';
                    std::cout << "What: Deleting RP with fVoxels.size() < fBLMinVoxels" << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
                it = fTPCData->fClusters.erase(it);
            }
            else
                it++;
        }
        else
        {
            if(it->GetSizeOfVoxels() <= fAlgo->GetMinPoints())
                it = fTPCData->fClusters.erase(it);
            else
                it++;
        }
    }

    // 4 -> Mask region around RP
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        auto& refVoxels {it->GetRefToVoxels()};
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
        if(remainSize > fAlgo->GetMinPoints())
        {
            refVoxels.erase(toKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(fIsVerbose)
                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose for ID: " << it->GetClusterID() << '\n';
                    std::cout << "What: masking region around RP" << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }
        }
    }

    // 5 -> Clean voxels outside cylinder
    if(fEnableCylinder)
        CylinderCleaning();

    // 6 -> Mask region at the begining and end of the tracks
    MaskBeginEnd(rp);

    // 7 -> Set default fit for BLs
    if(fEnableRPDefaultBeam)
    {
        for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
        {
            if(it->GetIsBeamLike())
            {
                auto [xmin, xmax] {it->GetXRange()};
                bool isShort {(xmax - xmin) <= fRPDefaultMinX};
                if(isShort)
                {
                    it->GetRefToLine().SetDirection({1, 0, 0});
                    if(fIsVerbose)
                    {
                        std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose for ID: " << it->GetClusterID() << '\n';
                        std::cout << "What: short beam : setting (1, 0, 0) direction" << '\n';
                        std::cout << "------------------------------" << RESET << '\n';
                    }
                }
            }
        }
    }
}

void ActAlgorithm::Actions::FindRP::BreakBeamToHeavy(const ROOT::Math::XYZPointF& rp, bool keepSplit)
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        if(it->GetIsBeamLike())
        {
            auto& refVoxels {it->GetRefToVoxels()};
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
            // Move
            std::vector<ActRoot::Voxel> newVoxels;
            newVoxels.insert(newVoxels.end(), std::make_move_iterator(rpBreak),
                             std::make_move_iterator(refVoxels.end()));
            refVoxels.erase(rpBreak, refVoxels.end());

            // Add new cluster if size is enough!
            if(newVoxels.size() >= fAlgo->GetMinPoints() && keepSplit)
            {
                ActRoot::Cluster newCluster {(int)fTPCData->fClusters.size()};
                newCluster.SetVoxels(std::move(newVoxels));
                newCluster.ReFit();
                newCluster.ReFillSets();
                newCluster.SetIsSplitRP(true);
                newCluster.SetHasRP(true);
                newCluster.SetRegionType(RegionType::EBeam);
                toAppend.push_back(std::move(newCluster));

                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- BreakAfterRP ----" << '\n';
                    std::cout << "-> Added heavy cluster of size : " << newCluster.GetSizeOfVoxels() << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }

            // Refit remaining voxels in beam-like if size is kept enough
            if(refVoxels.size() >= fAlgo->GetMinPoints())
            {
                it->ReFit();
            }
            else
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDRED << "---- BreakAfterRP ----" << '\n';
                    std::cout << "-> Remaining beam size is : " << refVoxels.size() << " < " << fAlgo->GetMinPoints()
                              << '\n';
                    std::cout << "   Not refitting !" << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }
            it->ReFillSets();
        }
    }
    fTPCData->fClusters.insert(fTPCData->fClusters.end(), std::make_move_iterator(toAppend.begin()),
                               std::make_move_iterator(toAppend.end()));
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
        if(remain > fAlgo->GetMinPoints())
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

void ActAlgorithm::Actions::FindRP::MaskBeginEnd(const ROOT::Math::XYZPointF rp)
{
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++)
    {
        // Declare variables
        const auto& line {it->GetLine()};
        const auto& gp {line.GetPoint()};
        auto& refVoxels {it->GetRefToVoxels()};
        auto oldSize {refVoxels.size()};
        // Sort them
        std::sort(refVoxels.begin(), refVoxels.end());
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
                                        // the one of the last/firt voxel
                                        // TODO: check a better way to mask outling voxels (proj.X() < projInit.X())
                                        // could be troublesome depending on track angle
                                        bool isInCapInit {(proj - projInit).R() <= fRPPivotDist ||
                                                          (proj.X() < projInit.X())};
                                        bool isInCapEnd {(proj - projEnd).R() <= fRPPivotDist || (proj.X() > projEnd.X())};
                                        return !(isInCapInit && isInCapEnd);
                                    })};
        auto newSize {std::distance(refVoxels.begin(), itKeep)};
        // Refit if eneugh voxels remain
        if(newSize >= fAlgo->GetMinPoints())
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets(); // Print
            if(fIsVerbose)
            {
                {
                    std::cout << BOLDYELLOW << "--- Masking beg. and end of #" << it->GetClusterID() << " ----" << '\n';
                    std::cout << "-> Init : " << init.GetPosition() << '\n';
                    std::cout << "-> Proj Init : " << projInit << '\n';
                    std::cout << "-> End : " << end.GetPosition() << '\n';
                    std::cout << "-> Proj End : " << projEnd << '\n';
                    std::cout << "-> (Old - New) sizes : " << (oldSize - refVoxels.size()) << '\n';
                    std::cout << "-> Gravity point : " << it->GetLine().GetPoint() << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                    // it->GetLine().Print();
                }
            }
            else
            {
                //  Print
                if(fIsVerbose)
                {
                    std::cout << BOLDRED << "--- Masking beg. and end of #" << it->GetClusterID() << " ----" << '\n';
                    std::cout << "-> Reaming cluster size : " << refVoxels.size() << " < " << fAlgo->GetMinPoints() << '\n';
                    std::cout << "   Not erasing nor refitting !" << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                    // it->GetLine().Print();
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
    typedef std::pair<double, ActAlgorithm::VAction::XYZPoint> SetValue;
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
                ActAlgorithm::VAction::XYZPoint rp {(a.X() + b.X()) / 2, (a.Y() + b.Y()) / 2, (a.Z() + b.Z()) / 2};
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
        fTPCData->fRPs.push_back(ActRoot::TPCData::XYZPoint(static_cast<float>(set.begin()->second.X()),
                                                            static_cast<float>(set.begin()->second.Y()),
                                                            static_cast<float>(set.begin()->second.Z())));
    }
    else
    {
        ; // keep preliminary RP just in case this finner method fails
    }
}

double ActAlgorithm::Actions::FindRP::GetClusterAngle(const ActPhysics::Line::XYZVector& beam, const ActPhysics::Line::XYZVector& recoil)
{
    auto dot {beam.Unit().Dot((recoil.Unit()))};
    return TMath::ACos(dot) * TMath::RadToDeg();
}

void ActAlgorithm::Actions::FindRP::ResetIndex()
{
    int idx {};
    for(auto it = fTPCData->fClusters.begin(); it != fTPCData->fClusters.end(); it++, idx++)
    {
        it->SetClusterID(idx);
    }
}