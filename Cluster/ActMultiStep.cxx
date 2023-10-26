#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActInterval.h"

#include "TEnv.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TSystem.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <cmath>
#include <complex>
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
    if(mb->CheckTokenExists("MergeDistThreshold"))
        fMergeDistThreshold = mb->GetDouble("MergeDistThreshold");
    if(mb->CheckTokenExists("MergeChi2Threshold"))
        fMergeChi2Threshold = mb->GetDouble("MergeChi2Threshold");
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

template <typename T>
bool ActCluster::MultiStep::RangesOverlap(T x1, T x2, T y1, T y2)
{
    bool condA {static_cast<int>(x1) <= static_cast<int>(y2)};
    bool condB {static_cast<int>(x2) >= static_cast<int>(y1)};
    return condA && condB;
}

template <typename T>
bool ActCluster::MultiStep::RangesTouch(T x1, T x2, T y1, T y2)
{
    // Convert to int to avoid any issues with float precision
    x1 = static_cast<int>(x1);
    x2 = static_cast<int>(x2);
    y1 = static_cast<int>(y1);
    y2 = static_cast<int>(y2);
    bool touch {};
    if(y1 < x1 && y2 < x1) // [y1, y2] ... [x1, x2]
        touch = std::abs(x1 - y2) == 1;
    else if(y1 > x2 && y2 > x2) // [x1, x2] ... [y1, y2]
        touch = std::abs(y1 - x2) == 1;
    else
        touch = false;
    return touch;
}

void ActCluster::MultiStep::ResetIndex()
{
    int idx {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++, idx++)
    {
        it->SetClusterID(idx);
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

ActCluster::MultiStep::XYZPoint ActCluster::MultiStep::DetermineBreakPoint(ItType it)
{
    const auto& xy {it->GetXYMap()};
    const auto& xz {it->GetXZMap()};
    // Create interval object
    IntervalMap<int> ivsY;
    IntervalMap<int> ivsZ;
    for(const auto& [x, yset] : xy)
        ivsY.BuildFromSet(x, yset);
    for(const auto& [x, zset] : xz)
        ivsZ.BuildFromSet(x, zset);

    std::cout<<BOLDCYAN<<" ==== Y ==== "<<'\n';
    ivsY.Print();
    std::cout<<" ==== Z ==== "<<'\n';
    ivsZ.Print();
    
    int length {3};
    auto xbreak {ivsY.GetKeyAtLength(length)};
    std::cout<<"X with Y = "<<xbreak<<" when length > "<<length<<'\n';
    auto xbreak2 {ivsZ.GetKeyAtLength(length)};
    std::cout<<"X with Z = "<<xbreak2<<" when length > "<<length<<'\n';
    return {};
    //
    // // Run in increasing X order
    // int diff {2};
    // int idx {};
    // int ibreak {};
    // float xbreak {};
    // int yCount {};
    // std::vector<double> vwidthY {};
    // int zCount {};
    // std::vector<double> vwidthZ {};
    // for(const auto& [x, yset] : xy)
    // {
    //     const auto& zset {xz.at(x)};
    //     int ySize {static_cast<int>(yset.size())};
    //     int zSize {static_cast<int>(zset.size())};
    //     std::cout << "X = " << x << " y.size() = " << ySize << " y.old() = " << yCount << '\n';
    //     if(idx != 0)
    //     {
    //         if(std::abs(ySize - yCount) > diff || std::abs(zSize - zCount) > diff)
    //         {
    //             if(ibreak == 0)
    //             {
    //                 xbreak = x;
    //             }
    //             if(ibreak == 2)
    //                 break;
    //             ibreak++;
    //         }
    //         else
    //         {
    //             yCount = ySize;
    //             zCount = zSize;
    //             vwidthY.push_back(ySize);
    //             vwidthZ.push_back(zSize);
    //         }
    //     }
    //     else
    //     {
    //         yCount = ySize;
    //         zCount = zSize;
    //         // write widths
    //         vwidthY.push_back(ySize);
    //         vwidthZ.push_back(zSize);
    //     }
    //     idx++;
    // }
    // // Set break point
    // // float ybreak {static_cast<float>(TMath::Mean(xy.at(xbreak).begin(), xy.at(xbreak).end()))};
    // // float zbreak {static_cast<float>(TMath::Mean(xz.at(xbreak).begin(), xz.at(xbreak).end()))};
    // // Compute widths
    // auto widthY {static_cast<float>(TMath::Mean(vwidthY.begin(), vwidthY.end()))};
    // auto widthZ {static_cast<float>(TMath::Mean(vwidthZ.begin(), vwidthZ.end()))};
    // std::cout<<"Width Y = "<<widthY<<'\n';
    // std::cout<<"WIdth Z = "<<widthZ<<'\n';
    // return XYZPoint {xbreak, widthY, widthZ};
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
        // auto gravity {it->GetGravityPointInRegion(0, fEntranceBeamRegionX)};
        auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};
        // Return if it is nan
        if(std::isnan(gravity.X()) || std::isnan(gravity.Y()) || std::isnan(gravity.Z()))
            continue;
        // for(const auto& [x, set] : it->GetXYMap())
        // {
        //     std::cout << "X = " << x << " with set" << '\n';
        //     for(const auto& y : set)
        //         std::cout << "  Y = " << y << '\n';
        // }
        // for(const auto& [x, set] : it->GetXZMap())
        // {
        //     std::cout << "X = " << x << " with set" << '\n';
        //     for(const auto& z : set)
        //         std::cout << "  Z = " << z << '\n';
        // }
        auto breakPoint {DetermineBreakPoint(it)};
        std::cout << "Break point = " << breakPoint << '\n';

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
                                        return IsInBeamCylinder(pos, gravity);
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
    for(auto out = fClusters->begin(); out != fClusters->end(); out++)
    {
        for(auto in = fClusters->begin(); in != fClusters->end();)
        {
            if(in == out)
            {
                in++;
                continue;
            }
            auto gravity {in->GetLine().GetPoint()};
            auto dist {out->GetLine().DistanceLineToPoint(gravity)};
            auto maxDm2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
            bool isBellowDistance {dist <= TMath::Sqrt(maxDm2)};
            // use other model
            auto outDir {out->GetLine().GetDirection().Unit()};
            auto inDir {in->GetLine().GetDirection().Unit()};
            bool areParallel {TMath::Abs(outDir.Dot(inDir)) >= fMergeMinParallelFactor};
            if(isBellowDistance || areParallel) // if(areParallel) // if(dist < fMergeDistThreshold)
            {
                // Create auxiliar line
                ActPhysics::Line aux {};
                auto test {out->GetVoxels()};
                auto inner {in->GetVoxels()};
                test.insert(test.end(), inner.begin(), inner.end());
                aux.FitVoxels(test);
                std::cout << "Parallel factor = " << outDir.Dot(inDir) << '\n';
                std::cout << "dist < maxDm2 = " << dist << " < " << maxDm2 << '\n';
                // New chi2
                auto newChi2 {aux.GetChi2()};
                // Determine old chi2
                double oldChi2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
                // if(out->GetSizeOfVoxels() >= in->GetSizeOfVoxels())
                //     oldChi2 = out->GetLine().GetChi2();
                // else
                //     oldChi2 = in->GetLine().GetChi2();
                std::cout << "Old Chi2 = " << oldChi2 << '\n';
                std::cout << "New Chi2 = " << aux.GetChi2() << '\n';
                bool isInCover {newChi2 <= (1. + fMergeChi2CoverageFactor) * oldChi2};
                if(isInCover) // if(aux.GetChi2() < fMergeChi2Threshold)
                {
                    in = fClusters->erase(in);
                    // And push voxels to main
                    std::cout << BOLDMAGENTA << "Merging cluster " << in->GetClusterID() << '\n';
                    auto& refvoxels {out->GetRefToVoxels()};
                    std::cout << "Init size = " << refvoxels.size() << '\n';
                    std::move(inner.begin(), inner.end(), std::back_inserter(refvoxels));
                    std::cout << "After size = " << refvoxels.size() << RESET << '\n';
                    out->ReFit();
                    out->ReFillSets();
                }
                else
                    in++;
            }
            else
                in++;
        }
        // Delete
    }
}

bool ActCluster::MultiStep::IsInBeamCylinder(const XYZPoint& pos, const XYZPoint& gravity)
{
    bool condY {(gravity.Y() - fBeamWindowY) <= pos.Y() && pos.Y() <= (gravity.Y() + fBeamWindowY)};
    bool condZ {(gravity.Z() - fBeamWindowZ) <= pos.Z() && pos.Z() <= (gravity.Z() + fBeamWindowZ)};

    return condY && condZ;
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

bool ActCluster::MultiStep::ClustersOverlap3D(ItType out, ItType in)
{
    // X
    auto [xMinOut, xMaxOut] {out->GetXRange()};
    auto [xMinIn, xMaxIn] {in->GetXRange()};
    bool isInX {RangesOverlap(xMinOut, xMaxOut, xMinIn, xMaxIn)};
    bool touchesX {RangesTouch(xMinOut, xMaxOut, xMinIn, xMaxIn)};
    // Y
    auto [yMinOut, yMaxOut] {out->GetYRange()};
    auto [yMinIn, yMaxIn] {in->GetYRange()};
    bool isInY {RangesOverlap(yMinOut, yMaxOut, yMinIn, yMaxIn)};
    bool touchesY {RangesTouch(yMinOut, yMaxOut, yMinIn, yMaxIn)};
    // Z
    auto [zMinOut, zMaxOut] {out->GetZRange()};
    auto [zMinIn, zMaxIn] {in->GetZRange()};
    bool isInZ {RangesOverlap(zMinOut, zMaxOut, zMinIn, zMaxIn)};
    bool touchesZ {RangesTouch(zMinOut, zMaxOut, zMinIn, zMaxIn)};
    return (isInX || touchesX) && (isInY || touchesY) && (isInZ || touchesZ);
}

void ActCluster::MultiStep::FrontierMatching()
{
    DetermineBeamLikes();
    // Iterate over map
    for(auto out = fClusters->begin(); out != fClusters->end(); out++)
    {
        if(!out->GetIsBeamLike())
            continue;
        std::cout << BOLDCYAN << "Cluster " << out->GetClusterID() << " is beam-like" << '\n';
        for(auto in = fClusters->begin(); in != fClusters->end(); in++)
        {
            if(in == out || in->GetIsBeamLike())
                continue;
            if(ClustersOverlap3D(out, in))
            {
                std::cout << "Overlaps with cluster " << in->GetClusterID() << '\n';
                // Print Y extent of beam
                for(const auto& [pad, count] : out->GetYMap())
                    std::cout << "Pad: " << pad << " count: " << count << '\n';
            }
        }
    }
    std::cout << RESET << '\n';
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
            std::cout << "-> MergeDistThresh  : " << fMergeDistThreshold << '\n';
            std::cout << "-> MergeChi2Thresh  : " << fMergeDistThreshold << '\n';
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
