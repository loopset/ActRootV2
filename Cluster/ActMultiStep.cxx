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
#include "TSystem.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
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
        std::cout << BOLDMAGENTA << ".ransac config file not found -> Using built-in configuration" << '\n';
        return;
    }

    // Parse!
    ActRoot::InputParser parser {realfile};
    auto mb {parser.GetBlock("MultiStep")};
    // General parameters
    if(mb->CheckTokenExists("IsEnabled"))
        fIsEnabled = mb->GetBool("IsEnabled");
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
    // Parameters of cleaning of pileup
    if(mb->CheckTokenExists("EnableCleanPileUp"))
        fEnableCleanPileUp = mb->GetBool("EnableCleanPileUp");
    if(mb->CheckTokenExists("PileUpXPercent"))
        fPileUpXPercent = mb->GetDouble("PileUpXPercent");
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
        CleanPileup();
    if(fEnableBreakMultiBeam)
        BreakBeamClusters();
    if(fEnableMerge)
        MergeSimilarTracks();
    if(fEnableCleanZs)
        CleanZs();
    if(fEnableCleanDeltas)
        CleanDeltas();
    ResetIndex();
    // FrontierMatching();
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
        // 1-> Eval condition of X range
        auto [xmin, xmax] {it->GetXRange()};
        auto range {(xmax - xmin)};
        auto tpcRange {fTPC->GetNPADSX() - 1};
        if(range < (tpcRange * fPileUpXPercent))
        {
            it++;
            continue;
        }
        // 2-> If so, delete if it is outside beam region
        auto zm {it->GetLine().GetPoint().Z()};
        bool mustDelete {(zm < fBeamLowerZ) || (zm > fBeamUpperZ)};
        // 3-> Delete if necessary
        if(mustDelete)
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

    std::cout << BOLDCYAN << " ==== Y ==== " << '\n';
    ivsY.Print();
    std::cout << " ==== Z ==== " << '\n';
    ivsZ.Print();

    // Ranges
    auto [xmin, xmax] {it->GetXRange()};
    auto breakY {ivsY.GetKeyAtLength(fBreakLengthThres, 4)};
    std::cout << "X with Y = " << breakY << '\n';
    auto widthY {ivsY.GetMeanSizeInRange(xmin, breakY)};
    std::cout << "Width Y = " << widthY << '\n';
    // Widths
    auto breakZ {ivsZ.GetKeyAtLength(fBreakLengthThres, 4)};
    std::cout << "X with Z = " << breakZ << '\n';
    auto widthZ {ivsZ.GetMeanSizeInRange(xmin, breakZ)};
    std::cout << "Width Z = " << widthZ << '\n';

    return {XYZPoint(std::min(breakY, breakZ), 0, 0), widthY, widthZ * fTPC->GetREBINZ()};
}

void ActCluster::MultiStep::BreakBeamClusters()
{
    std::vector<ActCluster::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // 1-> Check whether we meet conditions to execute this
        if(it->GetLine().GetChi2() < fChi2Threshold)
            continue;
        std::cout << BOLDGREEN << "Original Chi2 = " << it->GetLine().GetChi2() << '\n';
        // 2-> Check cluster has enough X extent
        auto [xmin, xmax] = it->GetXRange();
        if(fMinSpanX > (xmax - xmin))
            continue;
        // 2-> Calculate gravity point in region
        auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};
        // Return if it is nan
        if(std::isnan(gravity.X()) || std::isnan(gravity.Y()) || std::isnan(gravity.Z()))
            continue;
        auto res {DetermineBreakPoint(it)};
        auto bp {std::get<0>(res)};
        auto autoWY {std::get<1>(res)};
        auto autoWZ {std::get<2>(res)};
        std::cout << "Break point = " << bp << '\n';
        bool useBreakingPoint {bp.X() > (xmin + fLengthXToBreak)};
        std::cout << "Using breaking point ? " << std::boolalpha << useBreakingPoint << '\n';
        std::cout << BOLDGREEN << "Running for cluster " << it->GetClusterID() << '\n';
        std::cout << BOLDRED << "Gravity = " << gravity << '\n';
        // 3->Modify original cluster: move non-beam voxels outside to
        //  be clusterized independently
        auto& refToVoxels {it->GetRefToVoxels()};
        std::cout << "Original size = " << refToVoxels.size() << '\n';
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
        std::cout << "Remaining beam = " << refToVoxels.size() << '\n';
        std::cout << "Not beam       = " << notBeam.size() << RESET << '\n';
        // ReFit remaining voxels!
        it->ReFit();
        // Reset ranges
        it->ReFillSets();
        // Set it is beam-like so do not merge
        it->SetBeamLike(true);
        // 4-> Run cluster algorithm again
        std::vector<ActCluster::Cluster> newClusters;
        if(fFitNotBeam)
            newClusters = fClimb->Run(notBeam);
        // Move to vector
        std::move(newClusters.begin(), newClusters.end(), std::back_inserter(toAppend));
    }
    // Append clusters to original TPCData
    std::move(toAppend.begin(), toAppend.end(), std::back_inserter(*fClusters));
}

void ActCluster::MultiStep::MergeSimilarTracks()
{
    for(size_t i = 0; i < fClusters->size(); i++)
    {
        for(size_t j = 0; j < fClusters->size(); j++)
        {
            if(i == j) // exclude comparison of same cluster
                continue;
            // Get clusters as iterators
            auto out {fClusters->begin() + i};
            auto in {fClusters->begin() + j};

            // If any of them is beam-like, do not merge
            if(out->GetIsBeamLike() || in->GetIsBeamLike())
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
                std::cout << BOLDMAGENTA << "dist < distThresh = " << dist << " < " << distThresh << '\n';
                std::cout << "are parallel = " << std::boolalpha << areParallel << '\n';
                std::cout << "newChi2 < oldChi2 = " << newChi2 << " < " << oldChi2 << RESET << '\n';
                if(improvesFit)
                {
                    std::cout << BOLDMAGENTA << "Merging cluster in " << in->GetClusterID() << " with cluster out "
                              << out->GetClusterID() << RESET << '\n';
                    auto& refVoxels {out->GetRefToVoxels()};
                    std::move(inVoxels.begin(), inVoxels.end(), std::back_inserter(refVoxels));
                    // Refit and recompute ranges
                    out->ReFit();
                    out->ReFillSets();
                    // Erase!
                    fClusters->erase(in);
                }
            }
        }
    }
}

bool ActCluster::MultiStep::ManualIsInBeam(const XYZPoint& pos, const XYZPoint& gravity)
{
    bool condY {(gravity.Y() - fBeamWindowY) < pos.Y() && pos.Y() < (gravity.Y() + fBeamWindowY)};
    bool condZ {(gravity.Z() - fBeamWindowZ) < pos.Z() && pos.Z() < (gravity.Z() + fBeamWindowZ)};
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
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // Conditions to meet
        // 1 -> XRange starts in entrance of ACTAR
        auto [xmin, xmax] {it->GetXRange()};
        bool isInEntrance {xmin < 3};
        // 2 -> X direction is close to 1
        auto uDir {it->GetLine().GetDirection().Unit()};
        bool isAlongX {TMath::Abs(uDir.X()) >= 0.92};
        if(isInEntrance && isAlongX)
            it->SetBeamLike(true);
    }
}

void ActCluster::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> IsEnabled        : " << std::boolalpha << fIsEnabled << '\n';
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
            std::cout << "-> PileUpXPercent   : " << fPileUpXPercent << '\n';
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
    }
    std::cout << "=======================" << RESET << '\n';
}
