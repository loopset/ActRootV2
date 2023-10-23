#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"

#include "TEnv.h"
#include "TSystem.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <cmath>
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
    // Parameters of cleaning deltas and vertical tracks
    if(mb->CheckTokenExists("EnableCleanDeltasAndZs"))
        fEnableCleanDeltasAndZs = mb->GetBool("EnableCleanDeltasAndZs");
    if(mb->CheckTokenExists("ZDirectionThreshold"))
        fZDirectionThreshold = mb->GetDouble("ZDirectionThreshold");
    if(mb->CheckTokenExists("ZMinSpanInPlane"))
        fZMinSpanInPlane = mb->GetDouble("ZMinSpanInPlane");
    // Merge similar tracks
    if(mb->CheckTokenExists("EnableMerge"))
        fEnableMerge = mb->GetBool("EnableMerge");
    if(mb->CheckTokenExists("MergeDistThreshold"))
        fMergeDistThreshold = mb->GetDouble("MergeDistThreshold");
    if(mb->CheckTokenExists("MergeChi2Threshold"))
        fMergeChi2Threshold = mb->GetDouble("MergeChi2Threshold");
}

template <typename T>
bool ActCluster::MultiStep::RangesOverlap(T x1, T x2, T y1, T y2)
{
    bool condA {static_cast<int>(x1) <= static_cast<int>(y2)};
    bool condB {static_cast<int>(x2) >= static_cast<int>(y1)};
    return condA && condB;
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
    if(fEnableBreakMultiBeam)
        BreakBeamClusters();
    if(fEnableMerge)
        MergeSimilarTracks();
    if(fEnableCleanPileUp)
        CleanPileup();
    if(fEnableCleanDeltasAndZs)
        CleanDeltasAndZs();
    ResetIndex();
}

void ActCluster::MultiStep::CleanDeltasAndZs()
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
        if(isVertical && isNarrow)
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
        for(auto in = out + 1; in != fClusters->end();)
        {
            auto gravity {in->GetLine().GetPoint()};
            auto dist {out->GetLine().DistanceLineToPoint(gravity)};
            if(dist < fMergeDistThreshold)
            {
                // Create auxiliar line
                ActPhysics::Line aux {};
                auto test {out->GetVoxels()};
                auto inner {in->GetVoxels()};
                test.insert(test.end(), inner.begin(), inner.end());
                aux.FitVoxels(test);
                if(aux.GetChi2() < fMergeChi2Threshold)
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

void ActCluster::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> IsEnabled        : " << std::boolalpha << fIsEnabled << '\n';
    std::cout << "-----------------------"<<'\n';
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
            std::cout << "-----------------------"<<'\n';
        }
        if(fEnableMerge)
        {
            std::cout << "-> MergeDistThresh  : " << fMergeDistThreshold << '\n';
            std::cout << "-> MergeChi2Thresh  : " << fMergeDistThreshold << '\n';
            std::cout << "-----------------------"<<'\n';}
        if(fEnableCleanPileUp)
        {
            std::cout << "-> PileUpXPercent   : " << fPileUpXPercent << '\n';
            std::cout << "-> BeamLowerZ       : " << fBeamLowerZ << '\n';
            std::cout << "-> BeamUpperZ       : " << fBeamUpperZ << '\n';
            std::cout << "-----------------------"<<'\n';
        }
        if(fEnableCleanDeltasAndZs)
        {
            std::cout << "-> ZDirectionThresh : " << fZDirectionThreshold << '\n';
            std::cout << "-> ZMinSpanInPlane  : " << fZMinSpanInPlane << '\n';
            std::cout << "-----------------------"<<'\n';
        }
    }
    std::cout << "=======================" << RESET << '\n';
}
