#include "ActAlgoFuncs.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActTPCDetector.h"
#include "ActUtils.h"
#include "ActVoxel.h"

#include "TMath.h"
#include "TMatrixD.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <ios>
#include <iostream>
#include <tuple>
#include <vector>

std::tuple<ActAlgorithm::XYZPoint, ActAlgorithm::XYZPoint, double>
ActAlgorithm::ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB)
{
    // Using https://math.stackexchange.com/questions/1993953/closest-points-between-two-lines/3334866#3334866
    // 1-> Normalize all directions
    vA = vA.Unit();
    vB = vB.Unit();
    // 2-> Get cross product and normalize it
    auto vC {vB.Cross(vA)};
    vC = vC.Unit();
    // If lines are parallel, skip them
    if(ActRoot::IsEqZero(vC.R()))
        return {pA, pB, -1};
    // 3-> Matrices to solve system of equations in Math StackExchange
    TMatrixD left {3, 3}; // 3x3 matrix with double precision
    // Fill left matrix with columns as each ABC vector
    XYZVector vecs[3] {vA, -vB, vC};
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

bool ActAlgorithm::IsRPValid(const XYZPoint& rp, ActRoot::TPCParameters* tpc)
{
    // This function has to consider the 0.5 offset
    bool isInX {0.5 <= rp.X() && rp.X() <= (tpc->GetNPADSX() - 1) + 0.5};
    bool isInY {0.5 <= rp.Y() && rp.Y() <= (tpc->GetNPADSY() - 1) + 0.5};
    bool isInZ {0.5 <= rp.Z() && rp.Z() <= (tpc->GetNPADSZ() - 1) + 0.5};
    return isInX && isInY && isInZ;
}

void ActAlgorithm::MergeSimilarClusters(std::vector<ActRoot::Cluster>* clusters, double distThresh,
                                        double minParallelFactor, double chi2Factor, bool isVerbose)
{
    // Sort clusters by increasing voxel size
    std::sort(clusters->begin(), clusters->end(),
              [](const ActRoot::Cluster& l, const ActRoot::Cluster& r)
              { return l.GetSizeOfVoxels() < r.GetSizeOfVoxels(); });
    // Verbose
    if(isVerbose)
        std::cout << BOLDYELLOW << "---- MergeSimilarClusters ----" << '\n';

    // Set of indexes to delete
    std::set<int, std::greater<int>> toDelete {};
    // Run!
    for(size_t i = 0, isize = clusters->size(); i < isize; i++)
    {
        // Get clusters as iterators
        auto iit {clusters->begin() + i};
        for(size_t j = 0, jsize = clusters->size(); j < jsize; j++)
        {
            if(isVerbose)
                std::cout << "<i, j> : <" << i << ", " << j << ">" << '\n';

            bool isIinSet {toDelete.find(i) != toDelete.end()};
            bool isJinSet {toDelete.find(j) != toDelete.end()};
            if(i == j || isIinSet || isJinSet) // exclude comparison of same cluster and other already to be deleted
            {
                if(isVerbose)
                    std::cout << "   skipping j" << '\n';
                continue;
            }

            // Get inner iterator
            auto jit {clusters->begin() + j};

            // If any of them is set not to merge, do not do that :)
            if(!iit->GetToMerge() || !jit->GetToMerge())
            {
                if(isVerbose)
                    std::cout << "   i or j are set not to merge" << '\n';
                continue;
            }

            // 1-> Compare by distance from gravity point to line!
            auto gravIn {jit->GetLine().GetPoint()};
            auto distIn {iit->GetLine().DistanceLineToPoint(gravIn)};
            auto gravOut {iit->GetLine().GetPoint()};
            auto distOut {jit->GetLine().DistanceLineToPoint(gravOut)};
            auto dist {std::max(distIn, distOut)};
            // Get threshold distance to merge
            bool isBelowThresh {dist <= distThresh};

            // 2-> Compare by paralelity
            auto outDir {iit->GetLine().GetDirection().Unit()};
            auto inDir {jit->GetLine().GetDirection().Unit()};
            bool areParallel {std::abs(outDir.Dot(inDir)) > minParallelFactor};

            if(isVerbose)
            {
                std::cout << "   dist < distThres ? " << dist << " < " << distThresh << '\n';
                std::cout << "   are parellel ? " << std::boolalpha << areParallel << '\n';
            }

            // 3-> Check if fit improves
            if(isBelowThresh && areParallel)
            {
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
                bool improvesFit {newChi2 < chi2Factor * oldChi2};

                if(isVerbose)
                    std::cout << "   newChi2 < f * oldChi2 ? : " << newChi2 << " < " << chi2Factor * oldChi2 << '\n';

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
                    if(isVerbose)
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
        clusters->erase(clusters->begin() + idx);
    if(isVerbose)
        std::cout << RESET << '\n';
}

void ActAlgorithm::CylinderCleaning(std::vector<ActRoot::Cluster>* cluster, double cylinderR, int minVoxels,
                                    bool isVerbose)
{
    for(auto it = cluster->begin(); it != cluster->end(); it++)
    {
        auto& refVoxels {it->GetRefToVoxels()};
        auto oldSize {refVoxels.size()};
        auto itKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += XYZVector {0.5, 0.5, 0.5};
                                        auto dist {it->GetLine().DistanceLineToPoint(pos)};
                                        // auto proj {it->GetLine().ProjectionPointOnLine(pos)};
                                        // auto distX {std::abs(proj.X() - pos.X())};
                                        // auto distY {std::abs(proj.Y() - pos.Y())};
                                        // auto distZ {std::abs(proj.Z() - pos.Z())};
                                        // std::cout << "Proj : " << proj << '\n';
                                        // std::cout << "Pos  : " << pos << '\n';
                                        // std::cout << "dX : " << distX << " dY : " << distY << " dZ : " << distZ
                                        //           << '\n';
                                        // std::cout << "Overall dist : " << dist << '\n';
                                        // std::cout << "--------------------" << '\n';
                                        return dist <= cylinderR;
                                    })};
        // if enough voxels remain
        auto remain {std::distance(refVoxels.begin(), itKeep)};
        if(remain > minVoxels)
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(isVerbose)
            {
                std::cout << BOLDGREEN << "---- CylinderCleaning ----" << '\n';
                std::cout << "   cluster #" << it->GetClusterID() << '\n';
                std::cout << "   (old - new) sizes : " << (oldSize - remain) << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
        }
    }
}

void ActAlgorithm::Chi2AndSizeCleaning(std::vector<ActRoot::Cluster>* cluster, double chi2Threh, int minVoxels,
                                       bool isVerbose)
{
    for(auto it = cluster->begin(); it != cluster->end();)
    {
        // 1-> Check whether cluster has an exceptionally large Chi2
        bool hasLargeChi {it->GetLine().GetChi2() >= chi2Threh};
        // 2-> If has less voxels than required
        bool isSmall {(minVoxels != -1) ? it->GetSizeOfVoxels() <= minVoxels : false};
        // 3-> If after all there are clusters with Chi2 = -1
        bool isBadFit {it->GetLine().GetChi2() == -1};
        if(isVerbose)
        {
            std::cout << BOLDCYAN << "---- Chi2 cleaning ----" << '\n';
            std::cout << "-> Chi2      : " << it->GetLine().GetChi2() << '\n';
            std::cout << "    < Thresh ? " << std::boolalpha << hasLargeChi << '\n';
            std::cout << "-> Size      : " << it->GetSizeOfVoxels() << '\n';
            std::cout << "    < Thresh ? " << std::boolalpha << isSmall << '\n';
            std::cout << "-> IsBadFit  ? " << std::boolalpha << isBadFit << '\n';
            std::cout << "-------------------" << RESET << '\n';
        }
        if(hasLargeChi || isSmall || isBadFit)
            it = cluster->erase(it);
        else
            it++;
    }
}

ActAlgorithm::RPSet ActAlgorithm::SimplifyRPs(const RPVector& rps, double distThresh)
{
    if(rps.size() == 0)
        return {};
    if(rps.size() == 1)
        return {rps.front().first, {rps.front().second.first, rps.front().second.second}};

    RPVector aux;
    for(int i = 0, size = rps.size(); i < size; i++)
    {
        for(int j = i + 1; j < size; j++)
        {
            auto dist {(rps[i].first - rps[j].first).R()};
            if(dist <= distThresh)
            {
                aux.push_back(rps[i]);
                aux.push_back(rps[j]);
            }
        }
    }
    // If none in distance threshold
    if(aux.size() == 0)
        return {};
    // And compute mean
    RPSet ret;
    XYZPoint mean;
    for(const auto& [rp, idxs] : aux)
    {
        mean += XYZVector {rp};
        ret.second.insert(idxs.first);
        ret.second.insert(idxs.second);
    }
    ret.first = mean / aux.size();
    return ret;
}

void ActAlgorithm::BreakBeamToHeavy(std::vector<ActRoot::Cluster>* clusters, const XYZPoint& rp, int minVoxels,
                                    bool keepSplit, bool isVerbose)
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = clusters->begin(); it != clusters->end(); it++)
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
                                             pos += XYZVector {0.5, 0.5, 0.5};
                                             return (pos.X() < rp.X());
                                         })};
            // Move
            std::vector<ActRoot::Voxel> newVoxels;
            newVoxels.insert(newVoxels.end(), std::make_move_iterator(rpBreak),
                             std::make_move_iterator(refVoxels.end()));
            refVoxels.erase(rpBreak, refVoxels.end());

            // Add new cluster if size is enough!
            if(newVoxels.size() >= minVoxels && keepSplit)
            {
                ActRoot::Cluster newCluster {(int)clusters->size()};
                newCluster.SetVoxels(std::move(newVoxels));
                newCluster.ReFit();
                newCluster.ReFillSets();
                newCluster.SetIsSplitRP(true);
                newCluster.SetHasRP(true);
                toAppend.push_back(std::move(newCluster));

                if(isVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- BreakAfterRP ----" << '\n';
                    std::cout << "-> Added heavy cluster of size : " << newCluster.GetSizeOfVoxels() << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }

            // Refit remanining voxels in beam-like if size is kept enough
            if(refVoxels.size() >= minVoxels)
            {
                it->ReFit();
            }
            else
            {
                if(isVerbose)
                {
                    std::cout << BOLDRED << "---- BreakAfterRP ----" << '\n';
                    std::cout << "-> Remaining beam size is : " << refVoxels.size() << " < " << minVoxels << '\n';
                    std::cout << "   Not refitting !" << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
            }
            it->ReFillSets();
        }
    }
    clusters->insert(clusters->end(), std::make_move_iterator(toAppend.begin()),
                     std::make_move_iterator(toAppend.end()));
}

void ActAlgorithm::MaskBeginEnd(std::vector<ActRoot::Cluster>* clusters, const XYZPoint& rp, double pivotDist,
                                int minVoxels, bool isVerbose)
{
    for(auto it = clusters->begin(); it != clusters->end(); it++)
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
        //// PROJECTIONS ON LINE, relative to GP of line (correct by +0.5 offset)
        auto projInit {line.ProjectionPointOnLine(init.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        auto projEnd {line.ProjectionPointOnLine(end.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        // Partition: get iterator to last element to be kept
        auto itKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += XYZVector {0.5, 0.5, 0.5};
                                        auto proj {line.ProjectionPointOnLine(pos)};
                                        // delete all points over projInit/end
                                        // bc due to ordering and angle, some voxel could have a proj larger than
                                        // the one of the last/first voxel
                                        // TODO: check a better way to mask outling voxels (proj.X() < projInit.X()
                                        // could) be troublesome depending on track angle
                                        bool isInCapInit {(proj - projInit).R() <= pivotDist ||
                                                          (proj.X() < projInit.X())};
                                        bool isInCapEnd {(proj - projEnd).R() <= pivotDist || (proj.X() > projEnd.X())};
                                        // if(it->GetIsBeamLike())
                                        // {
                                        //     std::cout << "Proj : " << proj << '\n';
                                        //     std::cout << "isInCapInit : " << std::boolalpha << isInCapInit << '\n';
                                        //     std::cout << "isInCapEnd : " << std::boolalpha << isInCapEnd << '\n';
                                        //     std::cout << "--------------------" << '\n';
                                        // }
                                        return !(isInCapInit || isInCapEnd);
                                    })};
        auto newSize {std::distance(refVoxels.begin(), itKeep)};
        // Refit if enough voxels remain
        if(newSize >= minVoxels)
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets(); //  Print
            if(isVerbose)
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
            if(isVerbose)
            {
                std::cout << BOLDRED << "--- Masking beg. and end of #" << it->GetClusterID() << " ----" << '\n';
                std::cout << "-> Reaming cluster size : " << refVoxels.size() << " < " << minVoxels << '\n';
                std::cout << "   Not erasing nor refitting !" << '\n';
                std::cout << "------------------------------" << RESET << '\n';
                // it->GetLine().Print();
            }
        }
    }
}

double ActAlgorithm::GetClusterAngle(const XYZVector& beam, const XYZVector& recoil)
{
    auto dot {beam.Unit().Dot((recoil.Unit()))};
    return TMath::ACos(dot) * TMath::RadToDeg();
}
