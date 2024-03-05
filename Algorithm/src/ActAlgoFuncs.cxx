#include "ActAlgoFuncs.h"

#include "ActColors.h"
#include "ActTPCDetector.h"
#include "ActUtils.h"

#include "TMatrixD.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <tuple>

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
    // Sort clusters by decreasing voxel size
    std::sort(clusters->begin(), clusters->end(),
              [](const ActRoot::Cluster& l, const ActRoot::Cluster& r)
              { return l.GetSizeOfVoxels() < r.GetSizeOfVoxels(); });

    // Set of indexes to delete
    std::set<int, std::greater<int>> toDelete {};
    // Run!
    for(size_t i = 0, isize = clusters->size(); i < isize; i++)
    {
        // Get clusters as iterators
        auto out {clusters->begin() + i};
        for(size_t j = 0, jsize = clusters->size(); j < jsize; j++)
        {
            bool isIinSet {toDelete.find(i) != toDelete.end()};
            bool isJinSet {toDelete.find(j) != toDelete.end()};
            if(i == j || isIinSet || isJinSet) // exclude comparison of same cluster and other already to be deleted
                continue;

            // Get inner iterator
            auto in {clusters->begin() + j};

            // If any of them is set not to merge, do not do that :)
            if(!out->GetToMerge() || !in->GetToMerge())
                continue;

            // 1-> Compare by distance from gravity point to line!
            auto gravIn {in->GetLine().GetPoint()};
            auto distIn {out->GetLine().DistanceLineToPoint(gravIn)};
            auto gravOut {out->GetLine().GetPoint()};
            auto distOut {in->GetLine().DistanceLineToPoint(gravOut)};
            auto dist {std::max(distIn, distOut)};
            // Get threshold distance to merge
            bool isBelowThresh {dist <= distThresh};
            // auto threshIn {in->GetLine().GetChi2()};
            // auto threshOut {out->GetLine().GetChi2()};
            // auto distThresh {std::max(threshIn, threshOut)};
            // bool isBelowThresh {dist < std::sqrt(distThresh)};
            // std::cout << "<i, j> : <" << i << ", " << j << ">" << '\n';
            // std::cout << "i size : " << out->GetSizeOfVoxels() << " j size : " << in->GetSizeOfVoxels() << '\n';
            // std::cout << "gravOut" << gravOut << " gravIn : " << gravIn << '\n';
            // std::cout << "dist : " << dist << '\n';
            // std::cout << "distThresh: " << distThresh << '\n';
            // std::cout << "------------------" << '\n';

            // 2-> Compare by paralelity
            auto outDir {out->GetLine().GetDirection().Unit()};
            auto inDir {in->GetLine().GetDirection().Unit()};
            bool areParallel {std::abs(outDir.Dot(inDir)) > minParallelFactor};
            // std::cout << "Parallel factor : " << std::abs(outDir.Dot(inDir)) << '\n';

            // 3-> Check if fits improves
            if(isBelowThresh && areParallel)
            {
                ActPhysics::Line aux {};
                auto outVoxels {out->GetVoxels()};
                auto inVoxels {in->GetVoxels()};
                outVoxels.reserve(outVoxels.size() + inVoxels.size());
                outVoxels.insert(outVoxels.end(), std::make_move_iterator(inVoxels.begin()),
                                 std::make_move_iterator(inVoxels.end()));
                aux.FitVoxels(outVoxels);
                // Compare Chi2
                auto newChi2 {aux.GetChi2()};
                // oldChi2 is obtained by quadratic sum of chi2s
                auto oldChi2 {std::sqrt(std::pow(out->GetLine().GetChi2(), 2) + std::pow(in->GetLine().GetChi2(), 2))};
                // auto oldChi2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
                bool improvesFit {newChi2 < chi2Factor * oldChi2};
                // std::cout << "old chi2 : " << oldChi2 << '\n';
                // std::cout << "new chi2 :  " << newChi2 << '\n';

                if(isVerbose)
                {
                    std::cout << BOLDYELLOW << "---- MergeTracks verbose ----" << '\n';
                    std::cout << "for <i,j> : <" << i << ", " << j << ">" << '\n';
                    std::cout << "i size : " << out->GetSizeOfVoxels() << " j size : " << in->GetSizeOfVoxels() << '\n';
                    std::cout << "dist < distThresh ? : " << dist << " < " << distThresh << '\n';
                    std::cout << "are parallel ? : " << std::boolalpha << areParallel << '\n';
                    std::cout << "newChi2 < f * oldChi2 ? : " << newChi2 << " < " << chi2Factor * oldChi2 << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }

                // Then, move and erase in iterator!
                if(improvesFit)
                {
                    if(isVerbose)
                    {
                        std::cout << BOLDYELLOW << "------------------------------" << '\n';
                        std::cout << "-> merging cluster : " << in->GetClusterID()
                                  << " with size : " << in->GetSizeOfVoxels() << '\n';
                        std::cout << " with cluster out : " << out->GetClusterID()
                                  << " and size : " << out->GetSizeOfVoxels() << '\n';
                        std::cout << "------------------------------" << RESET << '\n';
                    }
                    auto& refVoxels {out->GetRefToVoxels()};
                    refVoxels.insert(refVoxels.end(), std::make_move_iterator(inVoxels.begin()),
                                     std::make_move_iterator(inVoxels.end()));
                    // Refit and recompute ranges
                    out->ReFit();
                    out->ReFillSets();
                    // Mark to delete afterwards!
                    toDelete.insert(j);
                }
            }
        }
    }
    // Indeed delete
    for(const auto& idx : toDelete) // toDelete is sorted in greater order
        clusters->erase(clusters->begin() + idx);
}
