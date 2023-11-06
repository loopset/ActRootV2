#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActInterval.h"
#include "ActLine.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"

#include "TEnv.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TMatrixD.h"
#include "TMatrixDfwd.h"
#include "TMatrixF.h"
#include "TStopwatch.h"
#include "TSystem.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <map>
#include <pstl/glue_algorithm_defs.h>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

void ActCluster::MultiStep::ReadConfigurationFile(const std::string& infile)
{
    // automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/filter.climb";
    std::string realfile {};
    if(!gSystem->AccessPathName(envfile.c_str()))
        realfile = envfile;
    else if(infile.length() > 0)
        realfile = infile;
    else
    {
        std::cout << BOLDMAGENTA << "filter.climb config file not found -> Using built-in configuration" << '\n';
        return;
    }

    // Parse!
    ActRoot::InputParser parser {realfile};
    auto mb {parser.GetBlock("MultiStep")};
    // General parameters
    if(mb->CheckTokenExists("IsEnabled"))
        fIsEnabled = mb->GetBool("IsEnabled");
    if(mb->CheckTokenExists("IsVerbose"))
        fIsVerbose = mb->GetBool("IsVerbose");
    // Parameters of break multibeam
    if(mb->CheckTokenExists("EnableBreakMultiBeam"))
        fEnableBreakMultiBeam = mb->GetBool("EnableBreakMultiBeam");
    if(mb->CheckTokenExists("FitNotBeam"))
        fFitNotBeam = mb->GetBool("FitNotBeam");
    if(mb->CheckTokenExists("Chi2Threshold"))
        fChi2Threshold = mb->GetDouble("Chi2Threshold");
    if(mb->CheckTokenExists("MinSpanX"))
        fMinSpanX = mb->GetDouble("MinSpanX");
    if(mb->CheckTokenExists("LengthXToBreak"))
        fLengthXToBreak = mb->GetDouble("LengthXToBreak");
    if(mb->CheckTokenExists("BeamWindowY"))
        fBeamWindowY = mb->GetDouble("BeamWindowY");
    if(mb->CheckTokenExists("BeamWindowZ"))
        fBeamWindowZ = mb->GetDouble("BeamWindowZ");
    if(mb->CheckTokenExists("BreakLengthThreshold"))
        fBreakLengthThres = mb->GetInt("BreakLengthThreshold");
    // Parameters to break multitracks
    if(mb->CheckTokenExists("EnableBreakMultiTracks"))
        fEnableBreakMultiTracks = mb->GetBool("EnableBreakMultiTracks");
    if(mb->CheckTokenExists("TrackChi2Threshold"))
        fTrackChi2Threshold = mb->GetDouble("TrackChi2Threshold");
    if(mb->CheckTokenExists("BeamWindowScaling"))
        fBeamWindowScaling = mb->GetDouble("BeamWindowScaling");
    // Parameters of cleaning of pileup
    if(mb->CheckTokenExists("EnableCleanPileUp"))
        fEnableCleanPileUp = mb->GetBool("EnableCleanPileUp");
    if(mb->CheckTokenExists("PileUpChangeZThresh"))
        fPileUpChangeZThreshold = mb->GetDouble("PileUpChangeZThresh");
    if(mb->CheckTokenExists("BeamLowerZ"))
        fBeamLowerZ = mb->GetDouble("BeamLowerZ");
    if(mb->CheckTokenExists("BeamUpperZ"))
        fBeamUpperZ = mb->GetDouble("BeamUpperZ");
    // Parameters of cleaning vertical tracks
    if(mb->CheckTokenExists("EnableCleanZs"))
        fEnableCleanZs = mb->GetBool("EnableCleanZs");
    if(mb->CheckTokenExists("ZDirectionThreshold"))
        fZDirectionThreshold = mb->GetDouble("ZDirectionThreshold");
    if(mb->CheckTokenExists("ZMinSpanInPlane"))
        fZMinSpanInPlane = mb->GetDouble("ZMinSpanInPlane");
    // Merge similar tracks
    if(mb->CheckTokenExists("EnableMerge"))
        fEnableMerge = mb->GetBool("EnableMerge");
    if(mb->CheckTokenExists("MergeMinParallelFactor"))
        fMergeMinParallelFactor = mb->GetDouble("MergeMinParallelFactor");
    if(mb->CheckTokenExists("MergeChi2CoverageF"))
        fMergeChi2CoverageFactor = mb->GetDouble("MergeChi2CoverageF");
    // Clean delta electrons and remaining non-apt cluster
    if(mb->CheckTokenExists("EnableCleanDeltas"))
        fEnableCleanDeltas = mb->GetBool("EnableCleanDeltas");
    if(mb->CheckTokenExists("DeltaChi2Threshold"))
        fDeltaChi2Threshold = mb->GetDouble("DeltaChi2Threshold");
    if(mb->CheckTokenExists("DeltaMaxVoxels"))
        fDeltaMaxVoxels = mb->GetDouble("DeltaMaxVoxels");
    // Clean unreacted beam events
    if(mb->CheckTokenExists("EnableRP"))
        fEnableRP = mb->GetBool("EnableRP");
    if(mb->CheckTokenExists("UnreactedMinPercentX"))
        fUnreactedMinPercentX = mb->GetDouble("UnreactedMinPercentX");
    if(mb->CheckTokenExists("UnreactedMinParallelFactor"))
        fUnreactedMinParallelFactor = mb->GetDouble("UnreactedMinParallelFactor");
    if(mb->CheckTokenExists("RPDistThreshold"))
        fRPDistThreshold = mb->GetDouble("RPDistThreshold");
    if(mb->CheckTokenExists("RPMaskXY"))
        fRPMaskXY = mb->GetDouble("RPMaskXY");
    if(mb->CheckTokenExists("RPMaskZ"))
        fRPMaskZ = mb->GetDouble("RPMaskZ");
    if(mb->CheckTokenExists("AllowedMults"))
        fAllowedMults = mb->GetIntVector("AllowedMults");
    if(mb->CheckTokenExists("RPPivotDist"))
        fRPPivotDist = mb->GetDouble("RPPivotDist");

    // Init clocks
    for(int i = 0; i < 8; i++)
    {
        fClocks.push_back(TStopwatch {});
    }
    // Set labels of them
    fCLabels = std::vector<std::string>(8);
    fCLabels[0] = "CleanPileup";
    fCLabels[1] = "CleanZs";
    fCLabels[2] = "BreakBeam";
    fCLabels[3] = "BreakTracks";
    fCLabels[4] = "MergeSimilarTracks";
    fCLabels[5] = "CleanDeltas";
    fCLabels[6] = "FindRP";
    fCLabels[7] = "FindPreciseRP";
}

void ActCluster::MultiStep::ResetIndex()
{
    int idx {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++, idx++)
    {
        it->SetClusterID(idx);
    }
}

void ActCluster::MultiStep::PrintStep() const
{
    for(const auto& cluster : *fClusters)
    {
        cluster.Print();
        cluster.GetLine().Print();
    }
}

void ActCluster::MultiStep::Run()
{
    // General disable of algorithm
    if(!fIsEnabled)
        return;
    // Set order of algorithms here, and whether they run or not
    if(fEnableCleanPileUp)
    {
        fClocks[0].Start(false);
        CleanPileup();
        fClocks[0].Stop();
    }
    if(fEnableCleanZs)
    {
        fClocks[1].Start(false);
        CleanZs();
        fClocks[1].Stop();
    }
    if(fEnableBreakMultiBeam)
    {
        fClocks[2].Start(false);
        BreakBeamClusters();
        fClocks[2].Stop();
        if(fEnableBreakMultiTracks)
        {
            fClocks[3].Start(false);
            BreakTrackClusters(); // this method is dependent on the previous!
            fClocks[3].Stop();
        }
    }
    if(fEnableMerge)
    {
        fClocks[4].Start(false);
        MergeSimilarTracks();
        fClocks[4].Stop();
    }
    if(fEnableCleanDeltas)
    {
        fClocks[5].Start(false);
        CleanDeltas();
        fClocks[5].Stop();
    }
    // Preliminary: find reaction point and tag unreacted beam clusters
    if(fEnableRP)
    {
        DetermineBeamLikes();
        fClocks[6].Start(false);
        FindRP();
        fClocks[6].Stop();
        DeleteInvalidClusters();
        fClocks[7].Start(false);
        DeterminePreciseRP();
        fClocks[7].Stop();
    }
    ResetIndex();
}

void ActCluster::MultiStep::PrintClocks() const
{
    std::cout << BOLDYELLOW << "==== MultiStep time report ====" << '\n';
    for(int i = 0; i < fCLabels.size(); i++)
    {
        std::cout << "Timer : " << fCLabels[i] << '\n';
        fClocks[i].Print();
    }
    std::cout << RESET << '\n';
}

void ActCluster::MultiStep::CleanZs()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1->Check tracks is vertical
        bool isVertical {std::abs(it->GetLine().GetDirection().Unit().Z()) >= fZDirectionThreshold};
        // 2->Check any horizontal range is lower than threshold
        auto [xmin, xmax] {it->GetXRange()};
        auto [ymin, ymax] {it->GetYRange()};
        bool isNarrow {(xmax - xmin) <= fZMinSpanInPlane || (ymax - ymin) <= fZMinSpanInPlane};
        // Delete if yes
        if(isVertical && isNarrow) // for Z particles going upwards in Z
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActCluster::MultiStep::CleanDeltas()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether cluster has an exceptionally large Chi2
        bool hasLargeChi {it->GetLine().GetChi2() >= fDeltaChi2Threshold};
        // 2-> If has less voxels than required
        bool isSmall {it->GetSizeOfVoxels() <= fDeltaMaxVoxels};
        if(fIsVerbose)
        {
            std::cout << BOLDCYAN << "---- CleanDeltas verbose ----" << '\n';
            std::cout << "Chi2 : " << it->GetLine().GetChi2() << '\n';
            std::cout << "SizeVoxels: " << it->GetSizeOfVoxels() << '\n';
            std::cout << "-------------------" << RESET << '\n';
        }
        if(hasLargeChi || isSmall)
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActCluster::MultiStep::CleanPileup()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Eval condition of Z range
        auto [zmin, zmax] {it->GetZRange()};
        bool isConstantZ {(double)(zmin - zmax) / fTPC->GetREBINZ() <= fPileUpChangeZThreshold};
        // std::cout << "ZRange : [" << zmin << ", " << zmax << "]" << '\n';
        // 2-> Get condition: out of beam region
        auto zm {it->GetLine().GetPoint().Z()};
        bool isInBeamZ {fBeamLowerZ <= zm && zm <= fBeamUpperZ};
        if(isConstantZ && !isInBeamZ)
        {
            it = fClusters->erase(it);
        }
        else
            it++;
    }
}

std::tuple<ActCluster::MultiStep::XYZPoint, double, double> ActCluster::MultiStep::DetermineBreakPoint(ItType it)
{
    const auto& xyMap {it->GetXYMap()};
    const auto& xzMap {it->GetXZMap()};
    // Create interval object
    IntervalMap<int> ivsY;
    IntervalMap<int> ivsZ;
    for(const auto& [x, yset] : xyMap)
    {
        ivsY.BuildFromSet(x, yset);
        ivsZ.BuildFromSet(x, xzMap.at(x), fTPC->GetREBINZ());
    }
    //
    // std::cout << BOLDCYAN << " ==== Y ==== " << '\n';
    // ivsY.Print();
    // std::cout << " ==== Z ==== " << '\n';
    // ivsZ.Print();
    //
    // Ranges
    auto [xmin, xmax] {it->GetXRange()};
    auto breakY {ivsY.GetKeyAtLength(fBreakLengthThres, 4)};
    // std::cout << "X with Y = " << breakY << '\n';
    auto widthY {ivsY.GetMeanSizeInRange(xmin, breakY)};
    // std::cout << "Width Y = " << widthY << '\n';
    // Widths
    auto breakZ {ivsZ.GetKeyAtLength(fBreakLengthThres, 4)};
    // std::cout << "X with Z = " << breakZ << '\n';
    auto widthZ {ivsZ.GetMeanSizeInRange(xmin, breakZ)};
    // std::cout << "Width Z = " << widthZ << '\n';

    return {XYZPoint(std::min(breakY, breakZ), 0, 0), widthY, widthZ * fTPC->GetREBINZ()};
}

void ActCluster::MultiStep::BreakBeamClusters()
{
    std::vector<ActCluster::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether we meet conditions to execute this
        bool isBadFit {it->GetLine().GetChi2() > fChi2Threshold};
        auto [xmin, xmax] {it->GetXRange()};
        bool hasMinXExtent {fMinSpanX > (xmax - xmin)};
        if(!isBadFit && !hasMinXExtent)
        {
            it++;
            continue;
        }
        else
        {
            // 2-> Compute gravity point
            auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};
            // 3-> PRELIMINARY: experimental method to get finer breaking point (a sort of preliminary reaction point)
            // based on cluster topology
            auto preliminary {DetermineBreakPoint(it)};
            auto bp {std::get<0>(preliminary)};                  // breaking point
            auto autoWY {std::get<1>(preliminary)};              // mean width along Y
            auto autoWZ {std::get<2>(preliminary)};              // mean width along Z
            bool useBreakingPoint {bp.X() > (xmin + fMinSpanX)}; // since it is very preliminary, does not workk all the
                                                                 // times, fallback to default method if so

            // 3->Modify original cluster: move non-beam voxels outside to
            //  be clusterized independently
            auto& refToVoxels {it->GetRefToVoxels()};
            auto initSize {refToVoxels.size()};
            auto toMove {std::partition(refToVoxels.begin(), refToVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            const auto& pos {voxel.GetPosition()};
                                            if(useBreakingPoint)
                                                return AutoIsInBeam(pos, gravity, (double)bp.X(), autoWY, autoWZ);
                                            else
                                                return ManualIsInBeam(pos, gravity);
                                        })};
            // Create vector to move to
            std::vector<ActRoot::Voxel> notBeam {};
            std::move(toMove, refToVoxels.end(), std::back_inserter(notBeam));
            refToVoxels.erase(toMove, refToVoxels.end());

            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "---- BreakBeam verbose for ID : " << it->GetClusterID() << " ----" << '\n';
                std::cout << "Chi2 = " << it->GetLine().GetChi2() << '\n';
                std::cout << "XExtent = [" << xmin << ", " << xmax << "]" << '\n';
                std::cout << "Break point = " << bp << '\n';
                std::cout << "Using breaking point ? " << std::boolalpha << useBreakingPoint << '\n';
                std::cout << "Gravity = " << gravity << '\n';
                std::cout << "Init size beam = " << initSize << '\n';
                std::cout << "Remaining beam = " << refToVoxels.size() << '\n';
                std::cout << "Not beam       = " << notBeam.size() << RESET << '\n';
                std::cout << "-------------------" << RESET << '\n';
            }
            // Check if satisfies minimum voxel requirement to be clusterized again
            if(refToVoxels.size() <= fClimb->GetMinPoints())
            {
                // Delete remaining beam-like cluster
                it = fClusters->erase(it);
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
            // 4-> Run cluster algorithm again (if asked... should delete this flag)
            std::vector<ActCluster::Cluster> newClusters;
            if(fFitNotBeam)
                newClusters = fClimb->Run(notBeam);
            // Move to vector
            std::move(newClusters.begin(), newClusters.end(), std::back_inserter(toAppend));
        }
    }
    // Append clusters to original TPCData
    std::move(toAppend.begin(), toAppend.end(), std::back_inserter(*fClusters));
}

void ActCluster::MultiStep::BreakTrackClusters()
{
    std::vector<ActCluster::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether cluster needs breaking
        bool isBadFit {it->GetLine().GetChi2() > fChi2Threshold};
        if(!isBadFit)
        {
            it++;
            continue;
        }
        else
        {
            // 2-> Get gravity point to break based on previous BreakBeamClusters
            XYZPoint gravity {};
            for(auto in = fClusters->begin(); in != fClusters->end(); in++)
                if(in->GetIsBeamLike())
                    gravity = in->GetGravityPointInXRange(fLengthXToBreak);
            // 3-> Identify voxels to move
            auto& refVoxels {it->GetRefToVoxels()};
            auto initSize {refVoxels.size()};
            auto toMove {std::partition(refVoxels.begin(), refVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            const auto& pos {voxel.GetPosition()};
                                            return ManualIsInBeam(pos, gravity, fBeamWindowScaling);
                                        })};
            if(fIsVerbose)
            {
                std::cout << BOLDCYAN << "---- BreakTrack verbose for ID : " << it->GetClusterID() << " ----" << '\n';
                std::cout << "New gravity : " << gravity << RESET << '\n';
                std::cout << "Init size : " << initSize << '\n';
                std::cout << "After size : " << refVoxels.size() << '\n';
                std::cout << "-------------------------" << '\n';
            }
            // 4-> Move
            std::vector<ActRoot::Voxel> breakable {};
            std::move(toMove, refVoxels.end(), std::back_inserter(breakable));
            refVoxels.erase(toMove, refVoxels.end());
            // 5-> Reprocess or delete if minimum number of points criterium is not met
            if(refVoxels.size() <= fClimb->GetMinPoints())
            {
                it = fClusters->erase(it);
            }
            else
            {
                it->ReFit();
                it->ReFillSets();
                it++;
            }
            // 5-> Re cluster
            auto newClusters {fClimb->Run(breakable)};
            // Set not to merge these new ones
            for(auto& ncl : newClusters)
                ncl.SetToMerge(false);
            std::move(newClusters.begin(), newClusters.end(), std::back_inserter(toAppend));
        }
    }
    // Write to vector of clusters
    std::move(toAppend.begin(), toAppend.end(), std::back_inserter(*fClusters));
}

void ActCluster::MultiStep::MergeSimilarTracks()
{
    std::set<int, std::greater<int>> toDelete {};
    for(size_t i = 0, isize = fClusters->size(); i < isize; i++)
    {
        for(size_t j = 0, jsize = fClusters->size(); j < jsize; j++)
        {
            bool isIinSet {toDelete.find(i) != toDelete.end()};
            bool isJinSet {toDelete.find(j) != toDelete.end()};
            if(i == j || isIinSet || isJinSet) // exclude comparison of same cluster and other already to be deleted
                continue;
            // Get clusters as iterators
            auto out {fClusters->begin() + i};
            auto in {fClusters->begin() + j};

            // If any of them is set not to merge, do not do that :)
            if(!out->GetToMerge() || !in->GetToMerge())
                continue;

            // 1-> Compare by distance from gravity point to line!
            auto gravity {in->GetLine().GetPoint()};
            auto dist {out->GetLine().DistanceLineToPoint(gravity)};
            // Get threshold distance to merge
            auto distThresh {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
            bool isBelowThresh {dist < std::sqrt(distThresh)};

            // 2-> Compare by paralelity
            auto outDir {out->GetLine().GetDirection().Unit()};
            auto inDir {in->GetLine().GetDirection().Unit()};
            bool areParallel {std::abs(outDir.Dot(inDir)) > fMergeMinParallelFactor};

            // 3-> Check if fits improves
            if(isBelowThresh || areParallel)
            {
                ActPhysics::Line aux {};
                auto outVoxels {out->GetVoxels()};
                auto inVoxels {in->GetVoxels()};
                outVoxels.insert(outVoxels.end(), inVoxels.begin(), inVoxels.end());
                aux.FitVoxels(outVoxels);
                // Compare Chi2
                auto newChi2 {aux.GetChi2()};
                auto oldChi2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
                bool improvesFit {newChi2 < fMergeChi2CoverageFactor * oldChi2};
                // Then, move and erase in iterator!
                if(improvesFit)
                {
                    if(fIsVerbose)
                    {
                        std::cout << BOLDYELLOW << "---- MergeTracks verbose ----" << '\n';
                        std::cout << "dist < distThresh ? : " << dist << " < " << distThresh << '\n';
                        std::cout << "are parallel ? : " << std::boolalpha << areParallel << '\n';
                        std::cout << "newChi2 < oldChi2 ? : " << newChi2 << " < " << oldChi2 << RESET << '\n';
                        std::cout << "Merging cluster in : " << in->GetClusterID()
                                  << " with cluster out : " << out->GetClusterID() << '\n';
                        std::cout << "-----------------------" << RESET << '\n';
                    }
                    auto& refVoxels {out->GetRefToVoxels()};
                    std::move(inVoxels.begin(), inVoxels.end(), std::back_inserter(refVoxels));
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
        fClusters->erase(fClusters->begin() + idx);
}

bool ActCluster::MultiStep::ManualIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, double scale)
{
    bool condY {(gravity.Y() - scale * fBeamWindowY) < pos.Y() && pos.Y() < (gravity.Y() + scale * fBeamWindowY)};
    bool condZ {(gravity.Z() - scale * fBeamWindowZ * fTPC->GetREBINZ()) < pos.Z() &&
                pos.Z() < (gravity.Z() + scale * fBeamWindowZ * fTPC->GetREBINZ())};
    return condY && condZ;
}

template <typename T>
bool ActCluster::MultiStep::AutoIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, T xBreak, T widthY, T widthZ,
                                         T offset)
{
    bool condX {pos.X() < xBreak + offset};
    bool condY {(gravity.Y() - widthY) <= pos.Y() && pos.Y() <= (gravity.Y() + widthY)};
    bool condZ {(gravity.Z() - widthZ) <= pos.Z() && pos.Z() <= (gravity.Z() + widthZ)};
    return condX && condY && condZ;
}

void ActCluster::MultiStep::DetermineBeamLikes()
{
    int nBeam {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // 1-> Check if XRange constitutes an important amount of XLength of ACTAR
        auto [xmin, xmax] {it->GetXRange()};
        bool isLongEnough {(xmax - xmin) >= fUnreactedMinPercentX * fTPC->GetNPADSX()};
        // 2-> Is mainly along X direction
        auto uDir {it->GetLine().GetDirection().Unit()};
        bool isAlongX {TMath::Abs(uDir.X()) >= fUnreactedMinParallelFactor};
        // 3-> Tag it as beam-like and count
        if(isLongEnough && isAlongX)
        {
            it->SetBeamLike(true);
            nBeam++;
        }
    }
    // Particular case in which there is only one beam cluster (RP search wont run in that case)
    if(fClusters->size() == 1 && fClusters->begin()->GetIsBeamLike())
        fClusters->begin()->SetToDelete(true);
    // Print
    if(fIsVerbose)
    {
        std::cout << BOLDYELLOW << "---- Beam-Like ID verbose ----" << '\n';
        std::cout << "-> N beam clusters  : " << nBeam << '\n';
        std::cout << "-> N total clusters : " << fClusters->size() << '\n';
        std::cout << "-------------------------" << RESET << '\n';
    }
}

std::tuple<ActCluster::MultiStep::XYZPoint, ActCluster::MultiStep::XYZPoint, double>
ActCluster::MultiStep::ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB)
{
    // Using https://math.stackexchange.com/questions/1993953/closest-points-between-two-lines/3334866#3334866
    // 1-> Normalize all directions
    vA = vA.Unit();
    vB = vB.Unit();
    // 2-> Get cross product and normalize it
    auto vC {vB.Cross(vA)};
    vC = vC.Unit();
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

bool ActCluster::MultiStep::IsRPValid(const XYZPoint& rp)
{
    // This function has to consider the 0.5 offset
    bool isInX {0.5 <= rp.X() && rp.X() <= fTPC->GetNPADSX() + 0.5};
    bool isInY {0.5 <= rp.Y() && rp.Y() <= fTPC->GetNPADSY() + 0.5};
    bool isInZ {0.5 <= rp.Z() && rp.Z() <= fTPC->GetNPADSZ() + 0.5};
    return isInX && isInY && isInZ;
}

void ActCluster::MultiStep::FindRP()
{
    // Explanation: distance between clusters in pair <i, j>, storing RP as 3rd value
    typedef std::tuple<double, std::pair<int, int>, XYZPoint> SetValue;
    // Build comparator func
    auto comp {[](const SetValue& left, const SetValue& right) { return std::get<0>(left) < std::get<0>(right); }};
    // Declare set
    std::set<SetValue, decltype(comp)> set {comp};
    // Run
    for(int i = 0, size = fClusters->size(); i < size; i++)
    {
        // Get clusters as iterators
        auto out {fClusters->begin() + i};
        for(int j = i + 1; j < size; j++)
        {
            // Get clusters as iterators
            auto in {fClusters->begin() + j};

            // Run function
            auto [pA, pB, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                               in->GetLine().GetPoint(), in->GetLine().GetDirection())};
            // Build RP as mean of A and B
            XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};

            // Check that all points are valid
            bool checkA {IsRPValid(pA)};
            bool checkB {IsRPValid(pB)};
            bool checkRP {IsRPValid(rp)};
            // And finally that distance AB is bellow threshold
            bool checkDist {dist <= fRPDistThreshold};
            if(checkA && checkB && checkRP && checkDist)
            {
                set.insert({dist, {i, j}, rp});
                // Set flag has valid RP
                out->SetHasValidRP(true);
                in->SetHasValidRP(true);
            }
            else
            {
                continue;
            }
            // we indeed confirmed that distance is correctly calculated in ComputeRPin3D
            // auto manual {TMath::Sqrt(TMath::Power(pA.X() - pB.X(), 2) + TMath::Power(pA.Y() - pB.Y(), 2) +
            // TMath::Power(pA.Z() - pB.Z(), 2))}; std::cout<<"Manual dist = "<<manual<<'\n'; std::cout<<"Auto dist =
            // "<<dist<<'\n';
        }
    }
    // Mark to delete according to whether they have valid or not RP
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        if(!it->GetHasValidRP())
            it->SetToDelete(true);
    }
    // Write to map and print, only the first one (with the best score)
    // for(auto it = set.begin(); it != set.begin()++; it++)
    // {
    if(set.size() > 0)
    {
        // Unpack values to write
        auto i {std::get<1>(*set.begin()).first};
        auto j {std::get<1>(*set.begin()).second};
        auto rp {std::get<2>(*set.begin())};
        // Insert in map
        fRPs->insert({{i, j}, rp});
        // Print
        // std::cout << BOLDYELLOW << "<i,j> : <" << i << ", " << j << ">" << '\n';
        // std::cout << "dist : " << std::get<0>(*set.begin()) << " with RP : " << rp << RESET << '\n';
    }
    // }
}

void ActCluster::MultiStep::DeleteInvalidClusters()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        if(it->GetToDelete())
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActCluster::MultiStep::DeterminePreciseRP()
{
    // Must be executed after cleaning of invalid clusters
    // 1-> Get best RP: the one with the lowest dist
    const auto& rp {fRPs->begin()->second};
    std::vector<ActCluster::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // 2-> Create sphere-like masked zone around RP
        auto& refVoxels {it->GetRefToVoxels()};
        // And delete them
        refVoxels.erase(
            std::remove_if(refVoxels.begin(), refVoxels.end(),
                           [&](const ActRoot::Voxel& voxel)
                           {
                               // Correct voxel by +0.5
                               const auto& pos {voxel.GetPosition()};
                               ROOT::Math::XYZPoint point {pos.X() + 0.5, pos.Y() + 0.5, pos.Z() + 0.5};
                               bool condX {(rp.X() - fRPMaskXY) <= point.X() && point.X() <= (rp.X() + fRPMaskXY)};
                               bool condY {(rp.Y() - fRPMaskXY) <= point.Y() && point.Y() <= (rp.Y() + fRPMaskXY)};
                               bool condZ {(rp.Z() - fRPMaskZ * fTPC->GetREBINZ()) <= point.Z() &&
                                           point.Z() <= (rp.Z() + fRPMaskZ * fTPC->GetREBINZ())};
                               return condX && condY && condZ;
                           }),
            refVoxels.end());
        // 3-> If cluster is beam-like, recluster the voxels AFTER RP
        if(it->GetIsBeamLike() && fClusters->size() == 2) // only when heavy recoil was not separeted from the rest
        {
            // Init size
            auto initSize {it->GetSizeOfVoxels()};
            std::vector<ActRoot::Voxel> recluster {};
            auto toMove {std::partition(refVoxels.begin(), refVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            const auto& pos {voxel.GetPosition()};
                                            ROOT::Math::XYZPoint point {pos.X() + 0.5, pos.Y() + 0.5, pos.Z() + 0.5};
                                            bool condX {point.X() > (rp.X() + fRPMaskXY)};
                                            return condX;
                                        })};
            auto afterSize {std::distance(refVoxels.begin(), toMove)};
            // Cross check to avoid deleting voxels before RP when
            // SetBeamLike == true due to a bad breaking point
            if(afterSize != 0)
            {
                std::move(toMove, refVoxels.end(), std::back_inserter(recluster));
                refVoxels.erase(toMove, refVoxels.end());
                // Re cluster
                if(recluster.size() > fClimb->GetMinPoints())
                {
                    auto newClusters {fClimb->Run(recluster)};
                    if(fIsVerbose)
                    {
                        std::cout << BOLDGREEN << "---- FindRP verbose ----" << '\n';
                        std::cout << "-> For event with beam + light tracks" << '\n';
                        std::cout << "-> Original init size : " << initSize << '\n';
                        std::cout << "-> Original end size : " << afterSize << '\n';
                        std::cout << "-> To recluster size : " << recluster.size() << '\n';

                        for(const auto& ncl : newClusters)
                        {
                            std::cout << "-> New cluster size : " << ncl.GetSizeOfVoxels() << '\n';
                        }
                        std::cout << RESET;
                    }
                    std::move(newClusters.begin(), newClusters.end(), std::back_inserter(toAppend));
                }
            }
        }
        // 3-> Refit them
        it->ReFit();
        it->ReFillSets();
    }
    std::move(toAppend.begin(), toAppend.end(), std::back_inserter(*fClusters));

    // Delete clusters with 0 voxels
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        if(it->GetSizeOfVoxels() <= fClimb->GetMinPoints())
            it = fClusters->erase(it);
        else
            it++;
    }
    ResetIndex();

    // Refit using finer method
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // Declare variables
        const auto& line {it->GetLine()};
        const auto& gp {line.GetPoint()};
        auto& refVoxels {it->GetRefToVoxels()};
        // Set same sign as rp
        it->GetRefToLine().AlignUsingPoint(rp);
        // Sort them according to relative position to RP
        if(gp.X() < rp.X())
            std::sort(refVoxels.begin(), refVoxels.end(), std::greater<ActRoot::Voxel>());
        else
            std::sort(refVoxels.begin(), refVoxels.end());
        // Get init point
        const auto& init {refVoxels.front()};
        // Get end point
        const auto& end {refVoxels.back()};
        //// PROJECTIONS ON LINE, relative to GP of line (correct by +0.5 offset)
        auto projInit {line.ProjectionPointOnLine(init.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        auto projEnd {line.ProjectionPointOnLine(end.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        // Get pivot points, according to position respect to RP
        auto pivotInit {projInit + fRPPivotDist * it->GetLine().GetDirection().Unit()};
        auto pivotEnd {projEnd - fRPPivotDist * it->GetLine().GetDirection().Unit()};
        // Remove points outside regions
        refVoxels.erase(std::remove_if(refVoxels.begin(), refVoxels.end(),
                                       [&](const ActRoot::Voxel& voxel)
                                       {
                                           const auto& pos {voxel.GetPosition()};
                                           auto point {pos + XYZVector {0.5, 0.5, 0.5}};
                                           auto proj {line.ProjectionPointOnLine(point)};
                                           bool isInCapInit {(proj - projInit).R() <= fRPPivotDist};
                                           bool isInCapEnd {(proj - projEnd).R() <= fRPPivotDist};
                                           return isInCapInit || isInCapEnd;
                                       }),
                        refVoxels.end());
        // Refit if enough voxels remain
        if(refVoxels.size() > fClimb->GetMinPoints())
        {
            it->ReFit();
            it->ReFillSets();
        }
        // Else, keep old fit... have to check whether this is fine... we should not delete voxels in this case
        //  Print
        if(fIsVerbose)
        {
            std::cout << BOLDMAGENTA << "--- FindPreciseRP verbose for ID : " << it->GetClusterID() << " ----" << '\n';
            std::cout << "Init : " << refVoxels.front().GetPosition() << '\n';
            std::cout << "Proj Init : " << projInit << '\n';
            std::cout << "Pivot Init : " << pivotInit << '\n';
            std::cout << "End : " << refVoxels.back().GetPosition() << '\n';
            std::cout << "Proj End : " << projEnd << '\n';
            std::cout << "Pivot End : " << pivotEnd << '\n';
            std::cout << "Distance : " << (projEnd - projInit).R() << '\n';
            std::cout << "Gravity point : " << it->GetLine().GetPoint() << '\n';
            std::cout << "------------------------------" << RESET << '\n';
        }
    }
}

void ActCluster::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> IsEnabled        : " << std::boolalpha << fIsEnabled << '\n';
    std::cout << "-> IsVerbose        : " << std::boolalpha << fIsVerbose << '\n';
    std::cout << "-----------------------" << '\n';
    if(fIsEnabled)
    {
        if(fEnableBreakMultiBeam)
        {
            std::cout << "-> FitNotBeam       : " << std::boolalpha << fFitNotBeam << '\n';
            std::cout << "-> Chi2 thresh      : " << fChi2Threshold << '\n';
            std::cout << "-> MinSpanX         : " << fMinSpanX << '\n';
            std::cout << "-> LengthXToBreak   : " << fLengthXToBreak << '\n';
            std::cout << "-> BeamWindowY      : " << fBeamWindowY << '\n';
            std::cout << "-> BeamWindowZ      : " << fBeamWindowZ << '\n';
            std::cout << "-> BreakLenThresh   : " << fBreakLengthThres << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableBreakMultiTracks)
        {
            std::cout << "-> TrackChi2Thresh  : " << fTrackChi2Threshold << '\n';
            std::cout << "-> BeamWindowScale  : " << fBeamWindowScaling << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableMerge)
        {
            std::cout << "-> MergeMinParallel : " << fMergeMinParallelFactor << '\n';
            std::cout << "-> MergeChi2CoverF  : " << fMergeChi2CoverageFactor << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanPileUp)
        {
            std::cout << "-> PileUpChangeZ    : " << fPileUpChangeZThreshold << '\n';
            std::cout << "-> BeamLowerZ       : " << fBeamLowerZ << '\n';
            std::cout << "-> BeamUpperZ       : " << fBeamUpperZ << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanZs)
        {
            std::cout << "-> ZDirectionThresh : " << fZDirectionThreshold << '\n';
            std::cout << "-> ZMinSpanInPlane  : " << fZMinSpanInPlane << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanDeltas)
        {
            std::cout << "-> DeltaChi2Thresh  : " << fDeltaChi2Threshold << '\n';
            std::cout << "-> DeltaMaxVoxels   : " << fDeltaMaxVoxels << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableRP)
        {
            std::cout << "-> UnreactMinPerX   : " << fUnreactedMinPercentX << '\n';
            std::cout << "-> UnreactMinParal  : " << fUnreactedMinParallelFactor << '\n';
            std::cout << "-> RPDistThresh     : " << fRPDistThreshold << '\n';
            std::cout << "-> RPMaskXY         : " << fRPMaskXY << '\n';
            std::cout << "-> RPMaskZ          : " << fRPMaskZ << '\n';
            std::cout << "-> RPPivotDist      : " << fRPPivotDist << '\n';
        }
    }
    std::cout << "=======================" << RESET << '\n';
}
