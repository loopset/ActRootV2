#include "ActAFindRP.h"

#include "ActAlgoFuncs.h"
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